#include <settings/settings.h>

#include <sys/byteorder.h>

#include "ble_mesh_ui.h"
#include "mesh/mesh.h"
#include "mesh/subnet.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/mesh.h"
#include "host/ethermind_impl.h"
#include "ble_mesh_led_sample.h"
#include "common/log.h"


#define UINT8_TO_STREAM(p, u8)   do{*(p)++ = (uint8_t)(u8);} while(0)
#define UINT16_TO_STREAM(p, u16) do{*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);} while(0)
#define UINT32_TO_STREAM(p, u32) do{*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);} while(0)
#define ARRAY_TO_STREAM(p, s, size) do{memcpy(p, s, size); p+= size; } while(0)
#define ARRAY_TO_STREAM_REV(p, s, size) do{for(size_t i = 0; i < size; i++) {p[i] = s[size - i - 1];  } p += size; } while(0)


#define OP_ONOFF_GET       BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_ONOFF_SET       BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_ONOFF_STATUS    BT_MESH_MODEL_OP_2(0x82, 0x04)

#define OP_VND_SET_COUNT BT_MESH_MODEL_OP_3(0x01, BEKEN_VND_COMPANY_ID)
#define OP_VND_SET_COUNT_UNACK BT_MESH_MODEL_OP_3(0x02, BEKEN_VND_COMPANY_ID)
#define OP_VND_COUNT_STATUS BT_MESH_MODEL_OP_3(0x03, BEKEN_VND_COMPANY_ID)


#define ceiling_fraction(numerator, divider) (((numerator) + ((divider) - 1)) / (divider))

static struct k_work button_work;
static struct k_work_delayable send_count_work;
static uint32_t send_count_inter = 0;

static uint16_t prov_addr = 0x0002;
static uint16_t current_dev_addr = 0;

static uint8_t provision_type = PROVISION_TYPE_WHEN_INIT;
static uint32_t client_send_count = 0;

const static uint16_t app_idx = 0;
const static uint16_t net_idx = 0;

#if 1//BLE_ZEPHRY_MESH_IS_PROVISIONER
static uint8_t net_key[16] = {0};
static uint8_t dev_key[16] = {0};
static uint8_t app_key[16] = {0};
#else
static uint8_t net_key[16] = {1};
static uint8_t dev_key[16] = {1};
static uint8_t app_key[16] = {1};

#endif

static void board_led_set(bool val)
{
    BT_INFO("%d", val);
}

static void board_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("%d %d", action, number);
}

static void board_prov_complete(void)
{
    BT_INFO("");
}

static void attention_on(struct bt_mesh_model *mod)
{
    BT_INFO("");
    board_led_set(true);
}

static void attention_off(struct bt_mesh_model *mod)
{
    BT_INFO("");
    board_led_set(false);
}

static const struct bt_mesh_health_srv_cb health_cb =
{
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv =
{
    .cb = &health_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static const char *const onoff_str[] = { "off", "on" };

static struct
{
    bool val;
    uint32_t count_value;
    uint8_t tid;
    uint16_t src;
    uint32_t transition_time;
    struct k_work_delayable work;
} onoff;

/* OnOff messages' transition time and remaining time fields are encoded as an
 * 8 bit value with a 6 bit step field and a 2 bit resolution field.
 * The resolution field maps to:
 * 0: 100 ms
 * 1: 1 s
 * 2: 10 s
 * 3: 20 min
 */
static const uint32_t time_res[] =
{
    100,
    MSEC_PER_SEC,
    10 * MSEC_PER_SEC,
    10 * 60 * MSEC_PER_SEC,
};

static inline int32_t model_time_decode(uint8_t val)
{
    uint8_t resolution = (val >> 6) & BIT_MASK(2);
    uint8_t steps = val & BIT_MASK(6);

    if (steps == 0x3f)
    {
        return SYS_FOREVER_MS;
    }

    return steps * time_res[resolution];
}

static inline uint8_t model_time_encode(int32_t ms)
{
    if (ms == SYS_FOREVER_MS)
    {
        return 0x3f;
    }

    for (int i = 0; i < ARRAY_SIZE(time_res); i++)
    {
        if (ms >= BIT_MASK(6) * time_res[i])
        {
            continue;
        }

        uint8_t steps = ceiling_fraction(ms, time_res[i]);

        return steps | (i << 6);
    }

    return 0x3f;
}

static int onoff_status_send(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx)
{
    uint32_t remaining;

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_ONOFF_STATUS, 3);
    bt_mesh_model_msg_init(&buf, OP_ONOFF_STATUS);

    remaining = k_ticks_to_ms_floor32(
                    k_work_delayable_remaining_get(&onoff.work)) +
                onoff.transition_time;

    /* Check using remaining time instead of "work pending" to make the
     * onoff status send the right value on instant transitions. As the
     * work item is executed in a lower priority than the mesh message
     * handler, the work will be pending even on instant transitions.
     */
    if (remaining)
    {
        net_buf_simple_add_u8(&buf, !onoff.val);
        net_buf_simple_add_u8(&buf, onoff.val);
        net_buf_simple_add_u8(&buf, model_time_encode(remaining));
    }
    else
    {
        net_buf_simple_add_u8(&buf, onoff.val);
    }

    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}

static void onoff_timeout(struct k_work *work)
{
    if (onoff.transition_time)
    {
        /* Start transition.
         *
         * The LED should be on as long as the transition is in
         * progress, regardless of the target value, according to the
         * Bluetooth Mesh Model specification, section 3.1.1.
         */
        board_led_set(true);

        k_work_reschedule(&onoff.work, K_MSEC(onoff.transition_time));
        onoff.transition_time = 0;
        return;
    }

    board_led_set(onoff.val);
}

/* Generic OnOff Server message handlers */

static int gen_onoff_get(struct bt_mesh_model *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct net_buf_simple *buf)
{
    onoff_status_send(model, ctx);
    return 0;
}

static int gen_onoff_set_unack(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple *buf)
{
    uint8_t val = net_buf_simple_pull_u8(buf);
    uint8_t tid = net_buf_simple_pull_u8(buf);
    int32_t trans = 0;
    int32_t delay = 0;

    if (buf->len)
    {
        trans = model_time_decode(net_buf_simple_pull_u8(buf));
        delay = net_buf_simple_pull_u8(buf) * 5;
    }

    /* Only perform change if the message wasn't a duplicate and the
     * value is different.
     */
    if (tid == onoff.tid && ctx->addr == onoff.src)
    {
        /* Duplicate */
        return 0;
    }

    if (val == onoff.val)
    {
        /* No change */
        return 0;
    }

    BT_INFO("set: %s delay: %d ms time: %d ms click_count %d\n", onoff_str[val], delay, trans);

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.val = val;
    onoff.transition_time = trans;

    /* Schedule the next action to happen on the delay, and keep
     * transition time stored, so it can be applied in the timeout.
     */
    k_work_reschedule(&onoff.work, K_MSEC(delay));

    return 0;
}

static int gen_onoff_set(struct bt_mesh_model *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct net_buf_simple *buf)
{
    (void)gen_onoff_set_unack(model, ctx, buf);
    onoff_status_send(model, ctx);

    return 0;
}


static int vnd_srv_count_status_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint32_t peer_count)
{
    BT_MESH_MODEL_BUF_DEFINE(buf, OP_VND_COUNT_STATUS, 10);
    bt_mesh_model_msg_init(&buf, OP_VND_COUNT_STATUS);

    net_buf_simple_add_le32(&buf, peer_count);
    net_buf_simple_add_le32(&buf, onoff.count_value);

    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}


static int vnd_srv_set_count_unack(struct bt_mesh_model *model,
                                   struct bt_mesh_msg_ctx *ctx,
                                   struct net_buf_simple *buf)
{
    uint32_t val = net_buf_simple_pull_le32(buf);
    uint8_t tid = net_buf_simple_pull_u8(buf);
    int32_t trans = 0;
    int32_t delay = 0;
    ARG_UNUSED(delay);

    if (buf->len)
    {
        trans = model_time_decode(net_buf_simple_pull_u8(buf));
        delay = net_buf_simple_pull_u8(buf) * 5;
    }

    if (tid == onoff.tid && ctx->addr == onoff.src)
    {
        /* Duplicate */
        BT_WARN("dup, tid %d addr %d", tid, ctx->addr);
        return 0;
    }

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.transition_time = trans;
    onoff.count_value++;

    BT_ERR("peer count %d recv all count %d", val, onoff.count_value);

    return 0;
}


static int vnd_srv_set_count(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple *buf)
{
    uint32_t peer_count = net_buf_simple_pull_le32(buf);
    uint8_t tid = net_buf_simple_pull_u8(buf);
    int32_t trans = 0;
    int32_t delay = 0;
    ARG_UNUSED(delay);

    if (buf->len)
    {
        trans = model_time_decode(net_buf_simple_pull_u8(buf));
        delay = net_buf_simple_pull_u8(buf) * 5;
    }

    if (tid == onoff.tid && ctx->addr == onoff.src)
    {
        BT_WARN("dup, tid %d addr %d", tid, ctx->addr);
        return 0;
    }

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.transition_time = trans;
    onoff.count_value++;

    BT_ERR("peer count %d server recv count %d", peer_count, onoff.count_value);


    //    vnd_srv_set_count_unack(model, ctx, buf);
    vnd_srv_count_status_send(model, ctx, peer_count);

    return 0;
}




static const struct bt_mesh_model_op gen_onoff_srv_op[] =
{
    { OP_ONOFF_GET,       BT_MESH_LEN_EXACT(0), gen_onoff_get },
    { OP_ONOFF_SET,       BT_MESH_LEN_MIN(2),   gen_onoff_set },
    { OP_ONOFF_SET_UNACK, BT_MESH_LEN_MIN(2),   gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

static const struct bt_mesh_model_op vnd_srv_op[] =
{
    { OP_VND_SET_COUNT, BT_MESH_LEN_MIN(5), vnd_srv_set_count},
    { OP_VND_SET_COUNT_UNACK, BT_MESH_LEN_MIN(5), vnd_srv_set_count_unack},
    BT_MESH_MODEL_OP_END,
};

/* Generic OnOff Client */

static int gen_onoff_status(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    uint8_t present = net_buf_simple_pull_u8(buf);

    if (buf->len)
    {
        uint8_t target = net_buf_simple_pull_u8(buf);
        int32_t remaining_time =
            model_time_decode(net_buf_simple_pull_u8(buf));

        BT_INFO("OnOff status: %s -> %s: (%d ms)\n", onoff_str[present], onoff_str[target], remaining_time);
        return 0;
    }

    BT_INFO("OnOff status: %s\n", onoff_str[present]);

    return 0;
}

static int vnd_cli_count_status(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    uint32_t peer_our_send_count = net_buf_simple_pull_le32(buf);
    uint32_t peer_recv_count = net_buf_simple_pull_le32(buf);

    BT_ERR("our send count %d, peer send our count %d, peer recv count %d", client_send_count, peer_our_send_count, peer_recv_count);

    return 0;
}


static const struct bt_mesh_model_op gen_onoff_cli_op[] =
{
    {OP_ONOFF_STATUS, BT_MESH_LEN_MIN(1), gen_onoff_status},
    BT_MESH_MODEL_OP_END,
};

static const struct bt_mesh_model_op vnd_cli_op[] =
{
    {OP_VND_COUNT_STATUS, BT_MESH_LEN_MIN(5), vnd_cli_count_status},
    BT_MESH_MODEL_OP_END,
};

/* This application only needs one element to contain its models */
static struct bt_mesh_model models[] =
{
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, NULL, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op, NULL, NULL),
};

static struct bt_mesh_model vnd_models[] =
{
    BT_MESH_MODEL_VND(BEKEN_VND_COMPANY_ID, BEKEN_VND_MODEL_COUNT_SERVER, vnd_srv_op, NULL, NULL),
    BT_MESH_MODEL_VND(BEKEN_VND_COMPANY_ID, BEKEN_VND_MODEL_COUNT_CLIENT, vnd_cli_op, NULL, NULL),
};

static struct bt_mesh_elem elements[] =
{
    BT_MESH_ELEM(0, models, vnd_models),
};

static const struct bt_mesh_comp comp =
{
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

/* Provisioning */

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("OOB Number: %u 0x%0x\n", number, action);

    board_output_number(action, number);

    return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
    BT_INFO("net_idx %d addr %d", net_idx, addr);
    board_prov_complete();
}

static void prov_reset(void)
{
    bt_mesh_prov_enable(BT_MESH_PROV_ADV|BT_MESH_PROV_GATT);
}



#if BLE_ZEPHRY_MESH_IS_PROVISIONER
static uint8_t dev_uuid[16] = {0xdd, 0xdd + 1, 0};
#else
static uint8_t dev_uuid[16] = {0xdd, 0xdd, 0};
#endif


static void unprovisioned_beacon(uint8_t uuid[16],
                                 bt_mesh_prov_oob_info_t oob_info,
                                 uint32_t *uri_hash)
{
    if (provision_type == PROVISION_TYPE_AUTO)
    {
        LOG_INF("recv uuid %d", bt_hex(uuid, 16));


        int ret = 0;

        if (! BLE_ZEPHRY_MESH_IS_PROVISIONER)
        {
            return;
        }

        if (1)//0 == memcmp(uuid, dev_uuid, sizeof(dev_uuid)))
        {
            ret = bt_mesh_provision_adv(uuid, 0, prov_addr, 0);

            if (ret)
            {
                LOG_ERR("%s bt_mesh_provision_adv ret err %d", ret);
            }
        }

    }
}


static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
                            uint8_t num_elem)
{
    if (provision_type == PROVISION_TYPE_AUTO)
    {
        LOG_INF("Device addr 0x%04x net_idx %d provisioned", prov_addr, net_idx);
        current_dev_addr = prov_addr++;
        //  k_sem_give(&prov_sem);
    }

    (void)current_dev_addr;

}


static const struct bt_mesh_prov prov =
{
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = output_number,
    .complete = prov_complete,
    .reset = prov_reset,
    .unprovisioned_beacon = unprovisioned_beacon,
    .node_added = prov_node_added,
};


/** Send an OnOff Set message from the Generic OnOff Client to all nodes. */
static int gen_onoff_send(bool val)
{
    struct bt_mesh_msg_ctx ctx =
    {
        .app_idx = models[3].keys[0], /* Use the bound key */
        .addr = BT_MESH_ADDR_ALL_NODES,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    static uint8_t tid;

    if (ctx.app_idx == BT_MESH_KEY_UNUSED)
    {
        BT_INFO("The Generic OnOff Client must be bound to a key before sending.");
        return -ENOENT;
    }

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_ONOFF_SET, 2);
    bt_mesh_model_msg_init(&buf, OP_ONOFF_SET);
    net_buf_simple_add_u8(&buf, val);
    net_buf_simple_add_u8(&buf, tid++);

    BT_INFO("Sending OnOff Set: %s", onoff_str[val]);

    return bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);
}

static void shell_send_click_count(struct k_work *work)
{
    struct bt_mesh_msg_ctx ctx =
    {
        .app_idx = models[3].keys[0], /* Use the bound key */
        .addr = BT_MESH_ADDR_ALL_NODES,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    static uint8_t tid;


    if (ctx.app_idx == BT_MESH_KEY_UNUSED)
    {
        BT_INFO("The Generic OnOff Client must be bound to a key before sending.");
        return;
    }

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_VND_SET_COUNT, 5);

    client_send_count++;
    bt_mesh_model_msg_init(&buf, OP_VND_SET_COUNT);
    net_buf_simple_add_le32(&buf, client_send_count);
    net_buf_simple_add_u8(&buf, tid++);

    BT_WARN("Sending count %d", client_send_count);

    //    bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);
    bt_mesh_model_send(&vnd_models[1], &ctx, &buf, NULL, NULL);

    if (send_count_inter)
    {
        k_work_reschedule(CONTAINER_OF(work, struct k_work_delayable, work), (k_timeout_t) {send_count_inter});
    }
}


static void setup_cdb(uint16_t netidx, uint16_t appidx)
{
    struct bt_mesh_cdb_app_key *key;

    key = bt_mesh_cdb_app_key_alloc(netidx, appidx);

    if (key == NULL)
    {
        BT_ERR("Failed to allocate app-key 0x%04x\n", appidx);
        return;
    }

    bt_rand(key->keys[0].app_key, 16);

    if (IS_ENABLED(CONFIG_BT_SETTINGS))
    {
        bt_mesh_cdb_app_key_store(key);
    }
}

static void self_provision(void)
{
    /* Self-provision with an arbitrary address.
     *
     * NOTE: This should never be done in a production environment.
     *       Addresses should be assigned by a provisioner, and keys should
     *       be generated from true random numbers. It is done in this
     *       sample to allow testing without a provisioner.
     */

    uint16_t addr = k_uptime_get_32() & BIT_MASK(15);
    int err;

    //    bt_rand(net_key, 16);

    BT_INFO("net_key %s", bt_hex(net_key, sizeof(net_key)));
    BT_INFO("app_key %s", bt_hex(app_key, sizeof(app_key)));
    BT_INFO("dev_key %s", bt_hex(dev_key, sizeof(dev_key)));

#if 0//PROVISION_TYPE
    err = bt_mesh_cdb_create(net_key);

    if (err == -EALREADY)
    {
        BT_INFO("Using stored CDB\n");
    }
    else if (err)
    {
        BT_ERR("Failed to create CDB (err %d)\n", err);
        return;
    }
    else
    {
        BT_INFO("Created CDB\n");
        //        setup_cdb(net_idx, app_idx);
    }

#endif

    BT_INFO("Self-provisioning with address 0x%04x\n", addr);
    err = bt_mesh_provision(net_key, net_idx, 0, 0, addr, dev_key);

    if (err)
    {
        BT_INFO("Provisioning failed (err: %d)\n", err);
        return;
    }

    /* Add an application key to both Generic OnOff models: */
    err = bt_mesh_app_key_add(app_idx, net_idx, app_key);

    if (err)
    {
        BT_INFO("App key add failed (err: %d)\n", err);
        return;
    }

    /* Models must be bound to an app key to send and receive messages with
     * it:
     */
    models[2].keys[0] = 0;
    models[3].keys[0] = 0;

    vnd_models[0].keys[0] = 0;
    vnd_models[1].keys[0] = 0;

    BT_INFO("Provisioned and configured!\n");
}


static void button_pressed(struct k_work *work)
{
    (void)setup_cdb;

    if (bt_mesh_is_provisioned())
    {
        (void)gen_onoff_send(!onoff.val);
        return;
    }

    self_provision();
}


static void ble_mesh_led_sample_ready(int err)
{
    (void)output_number;

    if (err)
    {
        BT_INFO("Bluetooth init failed (err %d)\n", err);
        return;
    }

    BT_INFO("Bluetooth initialized\n");

    err = bt_mesh_init(&prov, &comp);

    if (err)
    {
        BT_INFO("Initializing mesh failed (err %d)\n", err);
        return;
    }

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    /* This will be a no-op if settings_load() loaded provisioning info */
    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

    BT_INFO("Mesh initialized\n");

    if (provision_type == PROVISION_TYPE_WHEN_INIT)
    {
        self_provision();
    }
}


static void bt_mesh_triggle_button(void)
{
    k_work_submit(&button_work);
}

int ble_mesh_led_sample_init(void)
{
    int err = 0;
    uint8_t *index = dev_uuid;

    (void)index;
#if 0
#if 1
    extern int bk_bluetooth_get_address(uint8_t *mac);
    bk_bluetooth_get_address(mac);

    memset(dev_uuid, 0, sizeof(dev_uuid));
    UINT16_TO_STREAM(index, 0x01A8);
    UINT8_TO_STREAM(index, BIT(0) | BIT(4) | BIT(5) | (0x3 << 6));
    UINT32_TO_STREAM(index, product_id);
    ARRAY_TO_STREAM(index, mac, sizeof(mac));
    UINT8_TO_STREAM(index, (0 << 1) | (0 << 0));
#else
    //uint8_t uuid_data[] = {0xA8, 0x01, 0x51, 0x99, 0x54, 0x57, 0, 0x9F, 0x22, 0x71, 0x6C, 0x14, 0x18, 0x02, 0x07, 0};//led
    uint8_t uuid_data[] = {0xA8, 0x01, 0x71, 0xCC, 0x15, 0x00, 0x00, 0x02, 0x23, 0x1D, 0x63, 0xA7, 0xF8, 0x02, 0x00, 0x00};//socket
    extern int bk_bluetooth_get_address(uint8_t *mac);
    bk_bluetooth_get_address(mac);
    memcpy(dev_uuid, uuid_data, sizeof(uuid_data));

    memcpy(dev_uuid + sizeof(dev_uuid) - 2 - 1 - 6, mac, sizeof(mac));

    index += (2 + 1);
    UINT32_TO_STREAM(index, product_id);

#endif
#endif

    k_work_init(&button_work, button_pressed);
    k_work_init_delayable(&send_count_work, shell_send_click_count);

    if (err)
    {
        BT_INFO("Board init failed (err: %d)\n", err);
        return err;
    }

    k_work_init_delayable(&onoff.work, onoff_timeout);

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(ble_mesh_led_sample_ready);

    if (err)
    {
        BT_INFO("Bluetooth init failed (err %d)\n", err);
    }

    return err;
}


void ble_mesh_led_sample_shell(int32_t argc, char **argv)
{
    int ret = 0;

    if (argc <= 0)
    {
        BT_ERR("argc is 0");
        return;
    }

    if (!strcasecmp(argv[0], "init"))
    {
        uint8_t type = 0;

        if (argc == 2)
        {
            ret = sscanf(argv[1], "%hhu", &type);

            if (ret != 1)
            {
                BT_ERR("provision type err !");
                provision_type = PROVISION_TYPE_WHEN_INIT;
                return;
            }

            provision_type = type;
        }

        ble_mesh_led_sample_init();
    }
    else if (!strcasecmp(argv[0], "click"))
    {
        bt_mesh_triggle_button();
    }
    else if (!strcasecmp(argv[0], "send_count"))
    {
        uint32_t intv = 0;

        if (argc == 1)
        {
            if (send_count_inter)
            {
                BT_ERR("send timer is already set, cant single send !");

                return;
            }

            k_work_reschedule(&send_count_work, (k_timeout_t) {0});
        }

        if (argc >= 2)
        {
            ret = sscanf(argv[1], "%u", &intv);

            if (ret != 1)
            {
                BT_ERR("interv err !");
                send_count_inter = 0;
                return;
            }

            if (intv != 0 && intv < 1500)
            {
                BT_ERR("intv too short, at least 1500 ms !");
                return;
            }

            if (send_count_inter && intv)
            {
                BT_ERR("interv is already set, set to 0 first !");
                return;
            }
            else if (send_count_inter && intv == 0)
            {
                k_work_cancel_delayable(&send_count_work);

                BT_WARN("cancel send timer");
                return;
            }

            BT_WARN("send count timer !, intv %d ms", intv);
            send_count_inter = intv;
            k_work_reschedule(&send_count_work, (k_timeout_t) {send_count_inter});
        }
    }
    else
    {
        BT_ERR("unknow cmd %s", argv[0]);
    }

    return;
}

