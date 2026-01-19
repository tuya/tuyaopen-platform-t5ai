#include <settings/settings.h>

#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "ble_mesh_ui.h"
#include "mesh/mesh.h"
#include "mesh/subnet.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/mesh.h"
#include "host/ethermind_impl.h"
#include "ble_mesh_provision_sample.h"
#include "common/log.h"
#include "ble_mesh_ui.h"
#include "os/os.h"
#include "components/bluetooth/bk_ble_types.h"
#include "mesh/rpl.h"

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

extern ble_err_t bk_bluetooth_get_address(uint8_t *mac);
static struct k_work button_work;
static struct k_work_delayable send_count_work;
static uint32_t send_count_inter = 0;

//static uint16_t current_dev_addr = BT_MESH_ADDR_UNASSIGNED;

struct provision_ctx_struct
{
    uint8_t status;
    uint16_t local_addr;
    uint16_t net_idx;
    uint16_t netkey_idx;
} s_provision_ctx =
{
    .local_addr = BT_MESH_ADDR_UNASSIGNED,
};

struct provisioner_ctx_struct
{
    uint8_t status;
    uint16_t peer_addr;
    uint32_t recv_count;

    uint8_t peer_uuid_mac[6];
} s_provisioner_ctx[20] =
{
    {PROVISION_STATUS_IDLE, 2, 0},
    {PROVISION_STATUS_IDLE, 3, 0},
    {PROVISION_STATUS_IDLE, 4, 0},
    {PROVISION_STATUS_IDLE, 5, 0},
    {PROVISION_STATUS_IDLE, 6, 0},
    {PROVISION_STATUS_IDLE, 7, 0},
    {PROVISION_STATUS_IDLE, 8, 0},
    {PROVISION_STATUS_IDLE, 9, 0},
    {PROVISION_STATUS_IDLE, 10, 0},
    {PROVISION_STATUS_IDLE, 11, 0},
    {PROVISION_STATUS_IDLE, 12, 0},
    {PROVISION_STATUS_IDLE, 13, 0},
    {PROVISION_STATUS_IDLE, 14, 0},
    {PROVISION_STATUS_IDLE, 15, 0},
    {PROVISION_STATUS_IDLE, 16, 0},
    {PROVISION_STATUS_IDLE, 17, 0},
    {PROVISION_STATUS_IDLE, 18, 0},
    {PROVISION_STATUS_IDLE, 19, 0},
    {PROVISION_STATUS_IDLE, 20, 0},
    {PROVISION_STATUS_IDLE, 21, 0},
};

//static uint8_t provision_type = PROVISION_TYPE_WHEN_INIT;
static uint32_t client_send_count = 0;

//static uint16_t s_app_idx = 0;
static uint16_t s_appkey_idx = 0;
static uint16_t s_net_idx = 0;
static uint16_t s_netkey_idx = 0;


static uint8_t dev_uuid[16] = {BEKEN_VND_PROVISIONEE_UUID_HEAD, 0};
const static uint8_t provisioner_uuid[16] = {BEKEN_VND_PROVISIONER_UUID_HEAD, 0};
const static uint8_t provisionee_uuid[16] = {BEKEN_VND_PROVISIONEE_UUID_HEAD, 0};

static uint8_t provision_role = PROVISION_ROLE_UNKNOW;


static uint8_t net_key[16] = {1, 0};
static uint8_t dev_key[16] = {0};
static uint8_t app_key[16] = {0};


static void board_led_set_int(void)
{
    static uint8_t on = 1;

    extern void ble_gpio_debug(uint32_t pin, uint8_t up);
    ble_gpio_debug(25, on ? 1 : 0);
    on = (on ? 0 : 1);
}

static void board_led_set(bool val)
{
    BT_INFO("%d", val);
}

static void board_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("%d %d", action, number);
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

    (void)(delay);
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

    BT_WARN("peer count %d recv all count %d", val, onoff.count_value);
    board_led_set_int();
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
    (void)(delay);

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

    BT_WARN("peer count %d server recv count %d", peer_count, onoff.count_value);
    board_led_set_int();

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

    (void)(peer_our_send_count);
    for (uint32_t i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
    {
        if (s_provisioner_ctx[i].peer_addr == ctx->addr)
        {

            //            BT_WARN("remote add:%02X:%02X:%02X:%02X:%02X:%02X, mesh addr %d",
            //                    s_provisioner_ctx[i].peer_uuid_mac[5],
            //                    s_provisioner_ctx[i].peer_uuid_mac[4],
            //                    s_provisioner_ctx[i].peer_uuid_mac[3],
            //                    s_provisioner_ctx[i].peer_uuid_mac[2],
            //                    s_provisioner_ctx[i].peer_uuid_mac[1],
            //                    s_provisioner_ctx[i].peer_uuid_mac[0],
            //                    ctx->addr
            //                   );
            //
            //            BT_WARN("our send count %d, peer send our count %d, peer recv count %d",
            //                    client_send_count, peer_our_send_count, peer_recv_count);

            s_provisioner_ctx[i].recv_count++;

            BT_INFO("ble:W(%d):remote %02X:%02X:%02X:%02X:%02X:%02X mesh %d, we send %d, we recv %d, peer recv %d\n",
                      rtos_get_time(),
                      s_provisioner_ctx[i].peer_uuid_mac[5],
                      s_provisioner_ctx[i].peer_uuid_mac[4],
                      s_provisioner_ctx[i].peer_uuid_mac[3],
                      s_provisioner_ctx[i].peer_uuid_mac[2],
                      s_provisioner_ctx[i].peer_uuid_mac[1],
                      s_provisioner_ctx[i].peer_uuid_mac[0],
                      ctx->addr,
                      client_send_count, s_provisioner_ctx[i].recv_count, peer_recv_count);

            break;
        }
    }

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

static struct bt_mesh_cfg_cli cfg_cli =
{
};

/* This application only needs one element to contain its models */
static struct bt_mesh_model models[] =
{
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_CFG_CLI(&cfg_cli),
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

static void reset_device_ctx(uint8_t i)
{
    uint16_t addr = s_provisioner_ctx[i].peer_addr;

    memset(s_provisioner_ctx + i, 0, sizeof(s_provisioner_ctx[i]));

    s_provisioner_ctx[i].peer_addr = addr;
}

static void deprovision_device_inter(uint16_t addr)
{
    int err = 0;
    bool reset = false;
    (void)(reset);

    err = bt_mesh_cfg_node_reset(s_netkey_idx, addr, &reset);

    if (err)
    {
        BT_ERR("Unable to send Remote Node Reset (err %d)", err);
    }

    struct bt_mesh_cdb_node *node = bt_mesh_cdb_node_get(addr);

    if (node == NULL)
    {
        BT_ERR("No node with address 0x%04x", addr);
        //                return 0;
    }
    else
    {
        bt_mesh_cdb_node_del(node, true);
    }

    //this is a unsafe perform, you will under replay attack !! Dont't do it unless debug !!!
    bt_mesh_rpl_clear();
}

static void deprovision_device(uint8_t *mac)
{
    for (uint8_t i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
    {
        if (s_provisioner_ctx[i].status == PROVISION_STATUS_PROVISION_COMPL &&
                0 == memcmp(mac, s_provisioner_ctx[i].peer_uuid_mac, sizeof(s_provisioner_ctx[i].peer_uuid_mac)))
        {
            deprovision_device_inter(s_provisioner_ctx[i].peer_addr);
            reset_device_ctx(i);

            BT_WARN("Remote node reset complete");

            return;
        }
    }
}

static uint8_t is_provisioning(void)
{
    for (uint8_t i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
    {
        if (s_provisioner_ctx[i].status != PROVISION_STATUS_IDLE && s_provisioner_ctx[i].status != PROVISION_STATUS_PROVISION_COMPL)
        {
            return 1;
        }
    }

    return 0;
}


static int user_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("OOB Number: %u 0x%0x\n", number, action);

    board_output_number(action, number);

    return 0;
}


static void user_prov_reset(void)
{
    BT_WARN("The local node has been reset and needs reprovisioning");
    //    bt_mesh_prov_enable(BT_MESH_PROV_ADV);// | BT_MESH_PROV_GATT);
    onoff.count_value = 0;
    memset(&s_provision_ctx, 0, sizeof(s_provision_ctx));
}

static void user_unprovisioned_beacon(uint8_t uuid[16],
                                      bt_mesh_prov_oob_info_t oob_info,
                                      uint32_t *uri_hash)
{
    if (provision_role == PROVISION_ROLE_PROVISIONER)
    {
        int ret = 0;

        if (is_provisioning())
        {
            return;
        }

        if (0 == memcmp(uuid, provisionee_uuid, 4))
        {
            uint32_t i = 0;

            for (i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
            {
                if (s_provisioner_ctx[i].status != PROVISION_STATUS_IDLE &&
                        0 == memcmp(s_provisioner_ctx[i].peer_uuid_mac, uuid + 4, sizeof(s_provisioner_ctx[i].peer_uuid_mac)))
                {
                    //already provision
                    BT_WARN("already provision, %02X:%02X:%02X:%02X:%02X:%02X status %d",
                            s_provisioner_ctx[i].peer_uuid_mac[5],
                            s_provisioner_ctx[i].peer_uuid_mac[4],
                            s_provisioner_ctx[i].peer_uuid_mac[3],
                            s_provisioner_ctx[i].peer_uuid_mac[2],
                            s_provisioner_ctx[i].peer_uuid_mac[1],
                            s_provisioner_ctx[i].peer_uuid_mac[0],
                            s_provisioner_ctx[i].status);
                    return;
                }
            }

            for (i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
            {
                if (s_provisioner_ctx[i].status == PROVISION_STATUS_IDLE)
                {
                    break;
                }
            }

            if (i >= sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]))
            {
                //prov is full, ignore
                BT_WARN("provision is full, ignore");
                return;
            }

            BT_WARN("recv pb-adv mac %02X:%02X:%02X:%02X:%02X:%02X i %d", uuid[9], uuid[8], uuid[7], uuid[6], uuid[5], uuid[4], i);

            s_provisioner_ctx[i].status = PROVISION_STATUS_PROVISIONING;
            memcpy(s_provisioner_ctx[i].peer_uuid_mac, uuid + 4, 6);

            ret = bt_mesh_provision_adv(uuid, s_netkey_idx, s_provisioner_ctx[i].peer_addr, 0);

            if (ret)
            {
                BT_ERR("bt_mesh_provision_adv ret err %d", ret);
                return;
            }

            //todo: log "Public keys are identical", indicate same dhkey
        }
    }
}

static void user_prov_complete(uint16_t netkey_idx, uint16_t addr)
{
    BT_WARN("netkey_idx %d addr %d", netkey_idx, addr);
    s_provision_ctx.local_addr = addr;
    s_provision_ctx.netkey_idx = netkey_idx;
}


static void user_prov_node_added(uint16_t netkey_idx, uint8_t uuid[16], uint16_t addr,
                                 uint8_t num_elem)
{
    BT_INFO("netkey_idx %d addr %d num %d uuid %s", netkey_idx, addr, num_elem, bt_hex(uuid, 16));

    if (provision_role == PROVISION_ROLE_PROVISIONER)
    {
        for (uint32_t i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
        {
            if (0 == memcmp(s_provisioner_ctx[i].peer_uuid_mac, uuid + 4, sizeof(s_provisioner_ctx[i].peer_uuid_mac)) &&
                    s_provisioner_ctx[i].status == PROVISION_STATUS_PROVISIONING &&
                    addr == s_provisioner_ctx[i].peer_addr)
            {
                BT_WARN("success, mac %02X:%02X:%02X:%02X:%02X:%02X addr %d i %d", uuid[9], uuid[8], uuid[7], uuid[6], uuid[5], uuid[4],
                        addr, i);
                s_provisioner_ctx[i].status = PROVISION_STATUS_WAIT_CLOSE;


                return;
            }
        }
    }

    return;

    //    if (0)//provision_type == PROVISION_TYPE_AUTO)
    //    {
    //        LOG_INF("Device addr 0x%04x net_idx %d provisioned", prov_addr_index, net_idx);
    //        current_dev_addr = prov_addr_index++;
    //        //  k_sem_give(&prov_sem);
    //    }



}

static int32_t do_add_appkey_cb(void *arg)
{
    int err = 0;
    uint8_t status = 0;
    uint32_t i = (typeof(i))arg;

    s_provisioner_ctx[i].status = PROVISION_STATUS_PROVISION_COMPL;


    models[2].keys[0] = s_appkey_idx;
    models[3].keys[0] = s_appkey_idx;

    vnd_models[0].keys[0] = s_appkey_idx;
    vnd_models[1].keys[0] = s_appkey_idx;

    BT_WARN("%d", i);

    err = bt_mesh_cfg_app_key_add(s_net_idx, s_provisioner_ctx[i].peer_addr, s_netkey_idx, s_appkey_idx, app_key, &status);

    if (err)
    {
        BT_ERR("Unable to send App Key Add (err %d)", err);
        deprovision_device_inter(s_provisioner_ctx[i].peer_addr);
        reset_device_ctx(i);
        return -1;
    }

    if (status)
    {
        BT_ERR("AppKeyAdd failed with status 0x%02x", status);
        deprovision_device_inter(s_provisioner_ctx[i].peer_addr);
        reset_device_ctx(i);
        return -1;
    }

    BT_WARN("AppKey send, NetKeyIndex 0x%04x AppKeyIndex 0x%04x", s_netkey_idx, s_appkey_idx);


    err = bt_mesh_app_key_add(s_appkey_idx, s_netkey_idx, app_key);

    if (err)
    {
        BT_ERR("App key add failed (err: %d)", err);
        return -1;
    }

    status = 0;

    err = bt_mesh_cfg_mod_app_bind_vnd(s_netkey_idx, s_provisioner_ctx[i].peer_addr,
                                       s_provisioner_ctx[i].peer_addr, s_appkey_idx,
                                       BEKEN_VND_MODEL_COUNT_SERVER, BEKEN_VND_COMPANY_ID, &status);

    if (err)
    {
        BT_ERR("Unable to send Model App Bind (err %d)", err);
        deprovision_device_inter(s_provisioner_ctx[i].peer_addr);
        reset_device_ctx(i);
        return -1;
    }

    if (status)
    {
        BT_ERR("Model App Bind failed with status 0x%02x", status);
        deprovision_device_inter(s_provisioner_ctx[i].peer_addr);
        reset_device_ctx(i);
        return -1;
    }

    BT_WARN("bind success, %02X:%02X:%02X:%02X:%02X:%02X addr %d appaddr %d",
            s_provisioner_ctx[i].peer_uuid_mac[5],
            s_provisioner_ctx[i].peer_uuid_mac[4],
            s_provisioner_ctx[i].peer_uuid_mac[3],
            s_provisioner_ctx[i].peer_uuid_mac[2],
            s_provisioner_ctx[i].peer_uuid_mac[1],
            s_provisioner_ctx[i].peer_uuid_mac[0],
            s_provisioner_ctx[i].peer_addr, s_provisioner_ctx[i].peer_addr);
    return 0;
}

static void user_link_close(bt_mesh_prov_bearer_t bearer)
{
    if (provision_role == PROVISION_ROLE_PROVISIONER)
    {
        for (uint32_t i = 0; i < sizeof(s_provisioner_ctx) / sizeof(s_provisioner_ctx[0]); ++i)
        {
            if (s_provisioner_ctx[i].status == PROVISION_STATUS_WAIT_CLOSE)
            {
                BT_WARN("success close, bear %d index %d", bearer, i);

                zephyr_ble_mesh_push_msg(BLE_MESH_MSG_FUNC, do_add_appkey_cb, sizeof(do_add_appkey_cb), (void *)i, sizeof(i));

                break;
            }
            else if (PROVISION_STATUS_PROVISIONING == s_provisioner_ctx[i].status)
            {
                BT_WARN("fail close, bear %d index %d", bearer, i);
                reset_device_ctx(i);
            }
        }
    }
}

static const struct bt_mesh_prov prov =
{
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = user_output_number,
    .reset = user_prov_reset,
    .unprovisioned_beacon = user_unprovisioned_beacon,
    .complete = user_prov_complete, //provisioner self/ provisionee callback
    .node_added = user_prov_node_added, //provisioner callback
    .link_close = user_link_close,
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
        .app_idx = vnd_models[1].keys[0], /* Use the bound key */
        .addr = BT_MESH_ADDR_ALL_NODES,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    static uint8_t tid;


    if (ctx.app_idx == BT_MESH_KEY_UNUSED)
    {
        BT_INFO("The Generic OnOff Client must be bound to a key before sending 0x%X.", ctx.app_idx);
        //        return;
        ctx.app_idx = s_appkey_idx;
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

static void button_pressed(struct k_work *work)
{
    (void)setup_cdb;

    if (bt_mesh_is_provisioned())
    {
        (void)gen_onoff_send(!onoff.val);
        return;
    }

    //    self_provision();
}

static void ble_mesh_provision_sample_ready(int err)
{
    (void)user_output_number;

    if (err)
    {

        BT_ERR("Bluetooth init failed (err %d)", err);
        return;

    }

    BT_WARN("Bluetooth initialized");

    err = bt_mesh_init(&prov, &comp);

    if (err)
    {
        BT_ERR("Initializing mesh failed (err %d)", err);
        return;
    }

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    if (provision_role == PROVISION_ROLE_PROVISIONER)
    {
        //        self_provision();

        bt_rand(net_key, sizeof(net_key));

        BT_WARN("netkey %s", bt_hex(net_key, sizeof(net_key)));

        err = bt_mesh_cdb_create(net_key);

        if (err == -EALREADY)
        {
            BT_INFO("Using stored CDB");
        }
        else if (err)
        {
            BT_ERR("Failed to create CDB (err %d)", err);
            return;
        }

        s_provision_ctx.local_addr = 1 & BIT_MASK(15);
        err = bt_mesh_provision(net_key, s_netkey_idx, 0, 0, s_provision_ctx.local_addr, dev_key);

        if (err)
        {
            BT_ERR("Provisioning failed (err %d)", err);
            return;
        }
    }
    else if (provision_role == PROVISION_ROLE_PROVISIONEE)
    {
        bt_mesh_prov_enable(BT_MESH_PROV_ADV);// | BT_MESH_PROV_GATT);
    }

    BT_WARN("Mesh initialized");

}


static void bt_mesh_triggle_button(void)
{
    k_work_submit(&button_work);
}

int bt_mesh_provision_sample_init(void)
{
    int err = 0;
    uint8_t *index = dev_uuid;

    (void)index;

    k_work_init(&button_work, button_pressed);
    k_work_init_delayable(&send_count_work, shell_send_click_count);

    if (err)
    {
        BT_ERR("Board init failed (err: %d)\n", err);
        return err;
    }

    k_work_init_delayable(&onoff.work, onoff_timeout);

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(ble_mesh_provision_sample_ready);

    if (err)
    {
        BT_ERR("Bluetooth init failed (err %d)\n", err);
    }

    return err;
}


//static void hexstring_to_array(uint8_t *out, uint32_t *out_len, char *buff, uint8_t len)
//{
//    char temp[3] = {0};
//
//    uint32_t i = 0, j = 0;
//
//    for (i = 0; i < len && j < *out_len;)
//    {
//        memcpy(temp, buff + i, 2);
//        i = i + 2;
//        out[j++] = strtoul(temp, NULL, 16) & 0xFF;
//    }
//
//    *out_len = j;
//}

void bt_mesh_provision_sample_shell(int32_t argc, char **argv)
{
    int ret = 0;
    uint8_t mac[6] = {0};

    if (argc <= 0)
    {
        BT_ERR("argc is 0");
        return;
    }

    if (!strcasecmp(argv[0], "init"))
    {
        if (argc < 2)
        {
            BT_ERR("param err, need provisioner or provisionee");
            return;
        }

        if (!strcasecmp(argv[1], "provisioner"))
        {
            bk_bluetooth_get_address(mac);
            memcpy(dev_uuid, provisioner_uuid, sizeof(provisioner_uuid));
            memcpy(dev_uuid + 4, mac, sizeof(mac));

            provision_role = PROVISION_ROLE_PROVISIONER;
            bt_mesh_provision_sample_init();
        }
        else if (!strcasecmp(argv[1], "provisionee"))
        {
            if (provision_role == PROVISION_ROLE_UNKNOW)
            {
                bk_bluetooth_get_address(mac);
                memcpy(dev_uuid, provisionee_uuid, sizeof(provisionee_uuid));
                memcpy(dev_uuid + 4, mac, sizeof(mac));

                provision_role = PROVISION_ROLE_PROVISIONEE;
                bt_mesh_provision_sample_init();
            }
            else if (provision_role == PROVISION_ROLE_PROVISIONEE)
            {
                bt_mesh_prov_enable(BT_MESH_PROV_ADV);
            }
        }
        else
        {
            BT_ERR("param err, need provisioner or provisionee");
            return;
        }
    }
    else if (!strcasecmp(argv[0], "deprovision"))
    {
        if (argc < 2)
        {
            BT_ERR("param err, need provisionee's addr");
            return;
        }

        uint32_t mac[6] = {0};
        uint8_t mac_final[6] = {0};
        uint32_t len = sizeof(mac);
        (void)(len);
        //sscanf bug: cant detect uint8_t size point
        ret = sscanf(argv[1], "%02x:%02x:%02x:%02x:%02x:%02x", //argv[1], "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                     mac + 5,
                     mac + 4,
                     mac + 3,
                     mac + 2,
                     mac + 1,
                     mac);

        if (ret != 6)
        {
            BT_ERR("param err, need mac, %d %s", ret, argv[1]);
            return;
        }

        for (uint8_t i = 0; i < sizeof(mac_final) / sizeof(mac_final[0]); ++i)
        {
            mac_final[i] = mac[i];
        }

        BT_WARN("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                mac_final[5],
                mac_final[4],
                mac_final[3],
                mac_final[2],
                mac_final[1],
                mac_final[0]);

        deprovision_device(mac_final);
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
                send_count_inter = 0;
                BT_WARN("cancel send timer");
                return;
            }

            BT_WARN("send count timer intv %d ms", intv);
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

