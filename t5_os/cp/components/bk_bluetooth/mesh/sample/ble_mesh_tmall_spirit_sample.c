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
#include "ble_mesh_tmall_spirit_sample.h"
#include "driver/pwm.h"
#include "os/str.h"

#include "tinycrypt/constants.h"
#include "tinycrypt/sha256.h"


#define UINT8_TO_STREAM(p, u8)   do{*(p)++ = (uint8_t)(u8);} while(0)
#define UINT16_TO_STREAM(p, u16) do{*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);} while(0)
#define UINT32_TO_STREAM(p, u32) do{*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);} while(0)
#define ARRAY_TO_STREAM(p, s, size) do{memcpy(p, s, size); p+= size; } while(0)
#define ARRAY_TO_STREAM_REV(p, s, size) do{for(size_t i = 0; i < size; i++) {p[i] = s[size - i - 1];  } p += size; } while(0)


#define OP_ONOFF_GET       BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_ONOFF_SET       BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_ONOFF_STATUS    BT_MESH_MODEL_OP_2(0x82, 0x04)

#define ceiling_fraction(numerator, divider) (((numerator) + ((divider) - 1)) / (divider))

static struct k_work button_work;
static struct k_work_delayable button_pwm_work;

const uint32_t led_inter_period = 200; //ms

uint32_t s_product_id = 14963418;//zc
extern int bk_bluetooth_get_address(uint8_t *mac);

static struct
{
    bool val;
    uint8_t tid;
    uint16_t src;
    uint32_t transition_time;
    struct k_work_delayable work;

    uint16_t lightness_val;

    uint16_t ctrl_light;
    uint16_t ctrl_temp;
    uint16_t ctrl_uv;

    //    uint8_t scene_status;
    uint16_t scene_num;
} onoff =
{
    .lightness_val = (uint16_t)~0u,
};




//static struct
//{
//    uint16_t local;
//    uint16_t dst;
//    uint16_t net_idx;
//} net =
//{
//    .local = BT_MESH_ADDR_UNASSIGNED,
//    .dst = BT_MESH_ADDR_UNASSIGNED,
//};

static uint16_t primary_addr;
static uint16_t primary_net_idx;

static void board_led_set_int(bool val)
{
    extern void ble_gpio_debug(uint32_t pin, uint8_t up);
    ble_gpio_debug(34, val);
}

static void board_led_set(bool val)
{
    bk_err_t ret = 0;



    //    board_led_set_int(val);
    //    k_work_reschedule(&button_pwm_work, (k_timeout_t) {0});

    ret = bk_pwm_stop(LED_DEV_ID);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_stop err %d", ret);
    }

#if 0

    pwm_period_duty_config_t config;

    memset(&config, 0, sizeof(config));

    config.period_cycle = (uint16_t)~0u;
    config.duty_cycle = (val ? onoff.lightness_val : 0);

    BT_INFO("%d %d %d", val, config.period_cycle, config.duty_cycle);

    ret = bk_pwm_set_period_duty(LED_DEV_ID, &config);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_set_period_duty err %d", ret);
    }

#else

    pwm_init_config_t config;
    memset(&config, 0, sizeof(config));

    config.period_cycle = (uint16_t)~0u;
    config.duty_cycle = (val ? onoff.lightness_val : 0);
    config.psc = 25;

    BT_INFO("%d %d %d", val, config.period_cycle, config.duty_cycle);

    ret = bk_pwm_init(LED_DEV_ID, &config);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_set_period_duty err %d", ret);
    }

#endif

    ret = bk_pwm_start(LED_DEV_ID);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_start err %d", ret);
    }
}

static void board_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("%d %d", action, number);
}

static void board_prov_complete(void)
{
    BT_INFO("%d");
}

static void attention_on(struct bt_mesh_model *mod)
{
    BT_INFO("");
    //    board_led_set(true);
}

static void attention_off(struct bt_mesh_model *mod)
{
    BT_INFO("");
    //    board_led_set(false);
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

    BT_INFO("set: %s delay: %d ms time: %d ms\n", onoff_str[val], delay, trans);

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

const struct bt_mesh_model_op gen_onoff_srv_op[] =
{
    { OP_ONOFF_GET,       BT_MESH_LEN_EXACT(0), gen_onoff_get },
    { OP_ONOFF_SET,       BT_MESH_LEN_MIN(2),   gen_onoff_set },
    { OP_ONOFF_SET_UNACK, BT_MESH_LEN_MIN(2),   gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

/* Generic OnOff Client */

static int gen_onoff_cli_status(struct bt_mesh_model *model,
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

static const struct bt_mesh_model_op gen_onoff_cli_op[] =
{
    {OP_ONOFF_STATUS, BT_MESH_LEN_MIN(1), gen_onoff_cli_status},
    BT_MESH_MODEL_OP_END,
};

#define OP_LIGHTNESS_SERVER_LIGHTNESS_GET       BT_MESH_MODEL_OP_2(0x82, 0x4b)
#define OP_LIGHTNESS_SERVER_LIGHTNESS_SET       BT_MESH_MODEL_OP_2(0x82, 0x4c)
#define OP_LIGHTNESS_SERVER_LIGHTNESS_STATUS    BT_MESH_MODEL_OP_2(0x82, 0x4e)

#define OP_LIGHT_CTL_SERVER_GET       BT_MESH_MODEL_OP_2(0x82, 0x5d)
#define OP_LIGHT_CTL_SERVER_SET       BT_MESH_MODEL_OP_2(0x82, 0x5e)
#define OP_LIGHT_CTL_SERVER_STATUS    BT_MESH_MODEL_OP_2(0x82, 0x60)

#define OP_SCENE_SERVER_GET       BT_MESH_MODEL_OP_2(0x82, 0x41)
#define OP_SCENE_SERVER_RECALL    BT_MESH_MODEL_OP_2(0x82, 0x42)
#define OP_SCENE_SERVER_STATUS    0x5e


static int32_t lightness_server_lightness_status_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    uint32_t remaining;

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_LIGHTNESS_SERVER_LIGHTNESS_STATUS, 5);
    bt_mesh_model_msg_init(&buf, OP_LIGHTNESS_SERVER_LIGHTNESS_STATUS);

    //    remaining = k_ticks_to_ms_floor32(
    //                    k_work_delayable_remaining_get(&onoff.work)) +
    //                onoff.transition_time;
    //
    //
    //    if (remaining)
    //    {
    //        net_buf_simple_add_u8(&buf, !onoff.val);
    //        net_buf_simple_add_u8(&buf, onoff.val);
    //        net_buf_simple_add_u8(&buf, model_time_encode(remaining));
    //    }
    //    else
    //    {
    //        net_buf_simple_add_u8(&buf, onoff.val);
    //    }

    net_buf_simple_add_le16(&buf, onoff.lightness_val);

    (void)remaining;
    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}

static int lightness_server_lightness_get(struct bt_mesh_model *model,
        struct bt_mesh_msg_ctx *ctx,
        struct net_buf_simple *buf)
{
    lightness_server_lightness_status_send(model, ctx);
    return 0;
}

static int lightness_server_lightness_set(struct bt_mesh_model *model,
        struct bt_mesh_msg_ctx *ctx,
        struct net_buf_simple *buf)
{
    uint16_t lightness = net_buf_simple_pull_le16(buf);
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
        return 0;
    }

    BT_INFO("set: %d delay: %d ms time: %d ms", lightness, delay, trans);

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.lightness_val = lightness;
    onoff.transition_time = trans;
    k_work_reschedule(&onoff.work, K_MSEC(delay));

    lightness_server_lightness_status_send(model, ctx);
    return 0;
}




static int32_t light_ctrl_server_status_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    uint32_t remaining;

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_LIGHT_CTL_SERVER_STATUS, 9);
    bt_mesh_model_msg_init(&buf, OP_LIGHT_CTL_SERVER_STATUS);

    net_buf_simple_add_le16(&buf, onoff.ctrl_light);
    net_buf_simple_add_le16(&buf, onoff.ctrl_temp);

    (void)remaining;
    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}

static int light_ctrl_server_get(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
    light_ctrl_server_status_send(model, ctx);
    return 0;
}

static int light_ctrl_server_set(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
    uint16_t ctrl_light = net_buf_simple_pull_le16(buf);
    uint16_t ctrl_temp = net_buf_simple_pull_le16(buf);
    uint16_t ctrl_uv = net_buf_simple_pull_le16(buf);
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
        return 0;
    }

    BT_INFO("set: %d %d %d delay: %d ms time: %d ms", ctrl_light, ctrl_temp, ctrl_uv, delay, trans);

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.ctrl_light = ctrl_light;
    onoff.ctrl_temp = ctrl_temp;
    onoff.ctrl_uv = ctrl_uv;
    onoff.transition_time = trans;
    //    k_work_reschedule(&onoff.work, K_MSEC(delay));

    light_ctrl_server_status_send(model, ctx);
    return 0;
}



static int32_t scene_server_status_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    uint32_t remaining;

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_SCENE_SERVER_STATUS, 6);
    bt_mesh_model_msg_init(&buf, OP_SCENE_SERVER_STATUS);

    net_buf_simple_add_u8(&buf, 0);
    net_buf_simple_add_le16(&buf, onoff.scene_num);

    (void)remaining;
    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}

static int scene_server_get(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    scene_server_status_send(model, ctx);
    return 0;
}

static int scene_server_recall(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple *buf)
{
    uint16_t scene_num = net_buf_simple_pull_le16(buf);
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
        return 0;
    }

    BT_INFO("set: %d %d %d delay: %d ms time: %d ms", scene_num, delay, trans);

    onoff.tid = tid;
    onoff.src = ctx->addr;
    onoff.scene_num = scene_num;

    onoff.transition_time = trans;
    //    k_work_reschedule(&onoff.work, K_MSEC(delay));

    scene_server_status_send(model, ctx);
    return 0;
}


static const struct bt_mesh_model_op lightness_srv_op[] =
{
    { OP_LIGHTNESS_SERVER_LIGHTNESS_GET,       BT_MESH_LEN_EXACT(0), lightness_server_lightness_get },
    { OP_LIGHTNESS_SERVER_LIGHTNESS_SET,       BT_MESH_LEN_MIN(2),   lightness_server_lightness_set },
    BT_MESH_MODEL_OP_END,
};


static const struct bt_mesh_model_op light_ctl_srv_op[] =
{
    { OP_LIGHT_CTL_SERVER_GET,       BT_MESH_LEN_EXACT(0), light_ctrl_server_get },
    { OP_LIGHT_CTL_SERVER_SET,       BT_MESH_LEN_MIN(7),   light_ctrl_server_set },
    BT_MESH_MODEL_OP_END,
};


static const struct bt_mesh_model_op scene_srv_op[] =
{
    { OP_SCENE_SERVER_GET,       BT_MESH_LEN_EXACT(0), scene_server_get },
    { OP_SCENE_SERVER_RECALL,       BT_MESH_LEN_MIN(3),   scene_server_recall },
    BT_MESH_MODEL_OP_END,
};

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

/* This application only needs one element to contain its models */
static struct bt_mesh_model models[] =
{
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op, NULL, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_SCENE_SRV, scene_srv_op, NULL, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, lightness_srv_op, NULL, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_SRV, light_ctl_srv_op, NULL, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_HSL_SRV, NULL, NULL, NULL),
};

static struct bt_mesh_elem elements[] =
{
    BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE),
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
    BT_INFO("%d %d", net_idx, addr);

    primary_addr = addr;
    primary_net_idx = net_idx;

    (void)primary_net_idx;
    board_prov_complete();
}

static void prov_reset(void)
{
    bt_mesh_prov_enable(BT_MESH_PROV_ADV);// | BT_MESH_PROV_GATT);
}

static uint8_t dev_uuid[16] = {0xdd, 0xdd, 0};
//static uint8_t dev_uuid[16] = {0xa8, 0x01, BIT(0) | BIT(4) | BIT(5) | (0x3 << 6), };

static uint8_t static_val[16] = {0x68, 0xd3, 0xee, 0xf8, 0xb9, 0x44, 0xc0, 0xc2, 0x4f, 0xbf, 0x80, 0x66, 0x87, 0x67, 0x02, 0xc5};

static struct bt_mesh_prov prov =
{
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = BT_MESH_NO_OUTPUT,
    .output_number = NULL,
    .complete = prov_complete,
    .reset = prov_reset,
    .static_val = (const uint8_t *)static_val,
    .static_val_len = sizeof(static_val),
};


/** Send an OnOff Set message from the Generic OnOff Client to all nodes. */
static int gen_onoff_cli_set_send(bool val)
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
        BT_INFO("The Generic OnOff Client must be bound to a key before "
                "sending.\n");
        return -ENOENT;
    }

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_ONOFF_SET_UNACK, 2);
    bt_mesh_model_msg_init(&buf, OP_ONOFF_SET_UNACK);
    net_buf_simple_add_u8(&buf, val);
    net_buf_simple_add_u8(&buf, tid++);

    BT_INFO("Sending OnOff Set: %s\n", onoff_str[val]);

    return bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);
}

static int gen_onoff_srv_status_publish(void)
{
    int err = 0;
    struct bt_mesh_model *model = &models[2];

    struct net_buf_simple *msg = model->pub->msg;

    //    struct bt_mesh_msg_ctx ctx =
    //    {
    //        .addr = 0 + primary_addr,
    //    };


    BT_INFO("publish last 0x%02x cur 0x%02x\n", !onoff.val, onoff.val);

    bt_mesh_model_msg_init(msg, OP_ONOFF_STATUS);
    net_buf_simple_add_u8(msg, onoff.val);

    err = bt_mesh_model_publish(model);

    if (err)
    {
        BT_ERR("bt_mesh_model_publish err %d\n", err);
    }

    return err;
}

static void button_pwm_work_cb(struct k_work *work)
{
    static uint8_t status = 0;

    if (!status)
    {
        uint32_t ms = (uint32_t)(onoff.lightness_val * (float)led_inter_period / ~(uint16_t)0);
        status = 1;
        board_led_set_int(1);
        BT_INFO("on %d", ms);
        k_work_reschedule(CONTAINER_OF(work, struct k_work_delayable, work), (k_timeout_t) {ms});
    }
    else
    {
        uint32_t ms = led_inter_period - (uint32_t)(onoff.lightness_val * (float)led_inter_period / ~(uint16_t)0);
        status = 0;
        board_led_set_int(0);
        BT_INFO("off %d", ms);
        k_work_reschedule(CONTAINER_OF(work, struct k_work_delayable, work), (k_timeout_t) {ms});
    }
}

static void button_pressed(struct k_work *work)
{
    if (bt_mesh_is_provisioned())
    {
        //        (void)gen_onoff_send(!onoff.val);
        (void)gen_onoff_cli_set_send;
        onoff.val = (onoff.val ? 0 : 1);
        gen_onoff_srv_status_publish();


        return;
    }

    /* Self-provision with an arbitrary address.
     *
     * NOTE: This should never be done in a production environment.
     *       Addresses should be assigned by a provisioner, and keys should
     *       be generated from true random numbers. It is done in this
     *       sample to allow testing without a provisioner.
     */
    static uint8_t net_key[16] = {0};
    static uint8_t dev_key[16] = {0};
    static uint8_t app_key[16] = {0};
    uint16_t addr;
    int err;

    if (IS_ENABLED(CONFIG_HWINFO))
    {
        addr = sys_get_le16(&dev_uuid[0]) & BIT_MASK(15);
    }
    else
    {
        addr = k_uptime_get_32() & BIT_MASK(15);
    }

    BT_INFO("Self-provisioning with address 0x%04x\n", addr);
    err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);

    if (err)
    {
        BT_INFO("Provisioning failed (err: %d)\n", err);
        return;
    }

    /* Add an application key to both Generic OnOff models: */
    err = bt_mesh_app_key_add(0, 0, app_key);

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

    BT_INFO("Provisioned and configured!\n");
}

static void bt_mesh_tmall_spirit_sample_ready(int err)
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
    bt_mesh_prov_enable(BT_MESH_PROV_ADV);// | BT_MESH_PROV_GATT);

    BT_INFO("Mesh initialized\n");
}


static void bt_mesh_tmall_spirit_triggle_button(void)
{
    k_work_submit(&button_work);
}

uint32_t ble_mesh_get_product_id(void)
{
    switch (BLE_ZEPHYR_MESH_TMALL_TYPE)
    {
    case BLE_ZEPHYR_MESH_TMALL_TYPE_ESPRESSIF:
        return 12506752;
        break;

    default:
        return s_product_id;
        break;
    }
}

uint8_t ble_mesh_get_addr(uint8_t *buff)
{
    switch (BLE_ZEPHYR_MESH_TMALL_TYPE)
    {
    case BLE_ZEPHYR_MESH_TMALL_TYPE_ESPRESSIF:
    {
        const uint8_t mesh_addr[] = {0x3c, 0x5d, 0x29, 0x56, 0x68, 0x43 - 1};
        memcpy(buff, mesh_addr, sizeof(mesh_addr));
    }
    break;

    default:
        break;
    }

    return 0;
}


static void pwm_init(void)
{
    bk_err_t ret = 0;
    const pwm_init_config_t pwm_config =
    {
        .period_cycle = (uint16_t)~0u,
        .duty_cycle = 0,
        .psc = 25,
    };

    ret = bk_pwm_driver_init();

    if (ret != 0)
    {
        BT_ERR("bk_pwm_driver_init err %d", ret);
    }

    ret = bk_pwm_init(LED_DEV_ID, &pwm_config);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_init err %d", ret);
    }

    ret = bk_pwm_set_init_signal_low(LED_DEV_ID);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_set_init_signal_low err %d", ret);
    }

    ret = bk_pwm_start(LED_DEV_ID);

    if (ret != 0)
    {
        BT_ERR("bk_pwm_start err %d", ret);
    }
}


static int bt_mesh_tmall_spirit_sample_init(void)
{
    int err = 0;
    uint8_t *index = dev_uuid;
    uint8_t mac[6] = {0};
    const uint32_t product_id = ble_mesh_get_product_id();
    //    const uint32_t product_id = 12506752;//espressif

    (void)index;

    pwm_init();
#if 1

    bk_bluetooth_get_address(mac);

    memset(dev_uuid, 0, sizeof(dev_uuid));
    UINT16_TO_STREAM(index, 0x01A8);
    UINT8_TO_STREAM(index, BIT(0) | BIT(4) | BIT(5) | (0x3 << 6));
    UINT32_TO_STREAM(index, product_id);
    ARRAY_TO_STREAM(index, mac, sizeof(mac));
    UINT8_TO_STREAM(index, (0 << 1) | (0 << 0));

    switch (BLE_ZEPHYR_MESH_TMALL_TYPE)
    {
    case BLE_ZEPHYR_MESH_TMALL_TYPE_ESPRESSIF:
    {
        const uint8_t tmp_static_val[] = {0x50, 0x26, 0xc8, 0x2c, 0xda, 0xf6, 0x19, 0x82, 0xab, 0x35, 0xfe, 0x51, 0x96, 0x81, 0x3e, 0x2e};
        memcpy((uint8_t *)prov.static_val, tmp_static_val, sizeof(tmp_static_val));
        prov.static_val_len = sizeof(tmp_static_val);
    }
    break;

    default:
        break;
    }

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
    k_work_init(&button_work, button_pressed);
    k_work_init_delayable(&button_pwm_work, button_pwm_work_cb);

    if (err)
    {
        BT_INFO("Board init failed (err: %d)\n", err);
        return err;
    }

    k_work_init_delayable(&onoff.work, onoff_timeout);

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(bt_mesh_tmall_spirit_sample_ready);

    if (err)
    {
        BT_INFO("Bluetooth init failed (err %d)\n", err);
    }

    return err;
}


static void hexstring_to_array(uint8_t *out, uint32_t *out_len, char *buff, uint8_t len)
{
#if 0
    char temp[3] = {0};

    uint32_t i = 0, j = 0;

    for (i = 0; i < len && j < *out_len;)
    {
        memcpy(temp, buff + i, 2);
        i = i + 2;
        out[j++] = strtoul(temp, NULL, 16) & 0xFF;
    }

    *out_len = j;
#endif
}

void ble_mesh_tmall_spirit_sample_shell(int32_t argc, char **argv)
{
    (void)hexstring_to_array;
    int ret = 0;

    if (argc <= 0)
    {
        BT_ERR("argc is 0");
        return;
    }

    if (!strcasecmp(argv[0], "init"))
    {

        if (argc >= 2)
        {
            uint32_t pid = 0;
            ret = sscanf(argv[1], "%d", &pid);

            if (ret != 1)
            {
                BT_ERR("product id err");
                return;
            }

            BT_INFO("product id %d", pid);

            s_product_id = pid;
        }

#if 0

        if (argc >= 3)
        {
            uint8_t tmp_val[16] = {0};
            uint32_t len = sizeof(tmp_val);
            hexstring_to_array(tmp_val, &len, argv[2], strlen(argv[2]));

            if (len != 16)
            {
                BT_ERR("static oob val err ! len %d", len);
                return;
            }

            BT_INFO("static oob val %s", argv[2]);

            memcpy(static_val, tmp_val, sizeof(static_val));
        }

#endif

        if (argc >= 3)
        {
            if (strlen(argv[2]) != 32)
            {
                BT_ERR("device_secret err");
                return;
            }

            struct tc_sha256_state_struct tmp_sha256;

            uint8_t mac[6] = {0};

            char tmp_org[72] = {0};

            uint32_t index = 0;

            uint8_t output_sha256[32] = {0};

            memset(&tmp_sha256, 0, sizeof(tmp_sha256));

            ret = tc_sha256_init(&tmp_sha256);

            if (ret != TC_CRYPTO_SUCCESS)
            {
                BT_ERR("tc_sha256_init err %d", ret);
            }

            index += sprintf(tmp_org, "%08x,", s_product_id);

            bk_bluetooth_get_address(mac);

            for (int32_t i = 5; i >= 0; --i)
            {
                index += sprintf(tmp_org + index, "%02x", mac[i]);
            }

            strcat(tmp_org, ",");
            index++;

            strcat(tmp_org, argv[2]);
            index += 16;

            BT_INFO("tmp_org %s len %d", tmp_org, strlen(tmp_org));

            ret = tc_sha256_update(&tmp_sha256, (const uint8_t *)tmp_org, strlen(tmp_org));

            if (ret != TC_CRYPTO_SUCCESS)
            {
                BT_ERR("tc_sha256_update err %d", ret);
            }

            tc_sha256_final(output_sha256, &tmp_sha256);

            if (ret != TC_CRYPTO_SUCCESS)
            {
                BT_ERR("tc_sha256_update err %d", ret);
            }

            index = 0;
            memset(tmp_org, 0, sizeof(tmp_org));

            for (int32_t i = 0; i < sizeof(output_sha256); ++i)
            {
                index += sprintf(tmp_org + index, "%02X", output_sha256[i]);
            }

            BT_INFO("sha256 %s", tmp_org);

            memcpy(static_val, output_sha256, sizeof(static_val));
        }

        bt_mesh_tmall_spirit_sample_init();
    }
    else if (!strcasecmp(argv[0], "click"))
    {
        bt_mesh_tmall_spirit_triggle_button();
    }
    else
    {
        BT_ERR("unknow cmd %s", argv[0]);
    }

    return;
}

