//#include "appl_service.h"

#include "string.h"

//#include <os/mem.h>

//#include "gatt.h"
//#include "app_ble.h"
//#include "app_ble_init.h"
//#include "app_sdp.h"
//#include "appl_sm.h"
//#include "BT_version.h"
//#include <common/bk_err.h>
//#include "ble_ui.h"
//#include "ke_list.h"
//#include "att_internal.h"
//#include "att_extern.h"
#include "ble_mesh_ui.h"
#include "mesh/mesh.h"
#include "mesh/subnet.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/mesh.h"
#include "host/ethermind_impl.h"
#include "bluetooth/conn.h"




static void heartbeat(const struct bt_mesh_hb_sub *sub, uint8_t hops,
                      uint16_t feat)
{
    //  board_heartbeat(hops, feat);
    //  board_play("100H");
    BT_INFO("");
}


static struct
{
    uint16_t local;
    uint16_t dst;
    uint16_t net_idx;
    uint16_t app_idx;
} net =
{
    .local = BT_MESH_ADDR_UNASSIGNED,
    .dst = BT_MESH_ADDR_UNASSIGNED,
};

#define CUR_FAULTS_MAX 4

static uint8_t cur_faults[CUR_FAULTS_MAX];
static uint8_t reg_faults[CUR_FAULTS_MAX * 2];

static void* ble_mesh_msg_que = NULL;
void* ble_mesh_thread_handle = NULL;

static uint8_t (*s_init_finish_cb)(int32_t reason) = NULL;

static void get_faults(uint8_t *faults, uint8_t faults_size, uint8_t *dst, uint8_t *count)
{
    uint8_t i, limit = *count;

    for (i = 0U, *count = 0U; i < faults_size && *count < limit; i++)
    {
        if (faults[i])
        {
            *dst++ = faults[i];
            (*count)++;
        }
    }
}

static int fault_get_cur(struct bt_mesh_model *model, uint8_t *test_id,
                         uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
    BT_INFO("Sending current faults");

    *test_id = 0x00;
    *company_id = BT_COMP_ID_LF;

    get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

    return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t cid,
                         uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
    if (cid != BT_COMP_ID_LF)
    {
        BT_ERR("Faults requested for unknown Company ID"
               " 0x%04x", cid);
        return -EINVAL;
    }

    BT_INFO("Sending registered faults");

    *test_id = 0x00;

    get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

    return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
    if (cid != BT_COMP_ID_LF)
    {
        return -EINVAL;
    }

    (void)memset(reg_faults, 0, sizeof(reg_faults));

    return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
                      uint16_t cid)
{
    if (cid != BT_COMP_ID_LF)
    {
        return -EINVAL;
    }

    if (test_id != 0x00)
    {
        return -EINVAL;
    }

    return 0;
}

static const struct bt_mesh_health_srv_cb health_srv_cb =
{
    .fault_get_cur = fault_get_cur,
    .fault_get_reg = fault_get_reg,
    .fault_clear = fault_clear,
    .fault_test = fault_test,
};

static struct bt_mesh_health_srv health_srv =
{
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);

static struct bt_mesh_cfg_cli cfg_cli =
{
};

void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
    size_t i;

    if (!fault_count)
    {
        BT_INFO("Health Test ID 0x%02x Company ID "
                "0x%04x: no faults", test_id, cid);
        return;
    }

    BT_INFO("Health Test ID 0x%02x Company ID 0x%04x Fault "
            "Count %zu:", test_id, cid, fault_count);

    for (i = 0; i < fault_count; i++)
    {
        BT_INFO("\t0x%02x", faults[i]);
    }
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
                                  uint8_t test_id, uint16_t cid, uint8_t *faults,
                                  size_t fault_count)
{
    BT_INFO("Health Current Status from 0x%04x", addr);
    show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli =
{
    .current_status = health_current_status,
};

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static struct bt_mesh_model root_models[] =
{
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_CFG_CLI(&cfg_cli),
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL_HEALTH_CLI(&health_cli),
};

static struct bt_mesh_elem elements[] =
{
    BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp =
{
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static void prov_complete(uint16_t net_idx, uint16_t addr)
{

    BT_INFO("Local node provisioned, net_idx 0x%04x address "
            "0x%04x", net_idx, addr);

    net.local = addr;
    net.net_idx = net_idx,
    net.dst = addr;
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
                            uint8_t num_elem)
{
    BT_INFO("Node provisioned, net_idx 0x%04x address "
            "0x%04x elements %d", net_idx, addr, num_elem);

    net.net_idx = net_idx,
    net.dst = addr;
}

static void prov_input_complete(void)
{
    BT_INFO("Input complete");
}

static void prov_reset(void)
{
    BT_INFO("The local node has been reset and needs "
            "reprovisioning");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
    BT_INFO("OOB Number: %u", number);
    return 0;
}

static int output_string(const char *str)
{
    BT_INFO("OOB String: %s", str);
    return 0;
}

static bt_mesh_input_action_t input_act;
static uint8_t input_size;

static int input(bt_mesh_input_action_t act, uint8_t size)
{
    switch (act)
    {
    case BT_MESH_ENTER_NUMBER:
        BT_INFO("Enter a number (max %u digits) with: "
                "input-num <num>", size);
        break;

    case BT_MESH_ENTER_STRING:
        BT_INFO("Enter a string (max %u chars) with: "
                "input-str <str>", size);
        break;

    default:
        BT_INFO("Unknown input action %u (size %u) "
                "requested!", act, size);
        return -EINVAL;
    }

    input_act = act;
    input_size = size;
    return 0;
}

static const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
    switch (bearer)
    {
    case BT_MESH_PROV_ADV:
        return "PB-ADV";

    case BT_MESH_PROV_GATT:
        return "PB-GATT";

    default:
        return "unknown";
    }
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
    BT_INFO("Provisioning link opened on %s",
              bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
    BT_INFO("Provisioning link closed on %s",
              bearer2str(bearer));
}

static struct bt_mesh_prov prov =
{
    .uuid = dev_uuid,
    .link_open = link_open,
    .link_close = link_close,
    .complete = prov_complete,
    .node_added = prov_node_added,
    .reset = prov_reset,
    .static_val = NULL,
    .static_val_len = 0,
    .output_size = 6,
    .output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
    .output_number = output_number,
    .output_string = output_string,
    .input_size = 6,
    .input_actions = (BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING),
    .input = input,
    .input_complete = prov_input_complete,
};

#define DEVICE_NAME "bk7231n-zephyr"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
static const struct bt_data ad[] =
{
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
    BT_DATA_BYTES(BT_DATA_SVC_DATA16,
                  0xaa, 0xfe, /* Eddystone UUID */
                  0x10, /* Eddystone-URL frame type */
                  0x00, /* Calibrated Tx power at 0m */
                  0x00, /* URL Scheme Prefix http://www. */
                  'z', 'e', 'p', 'h', 'y', 'r',
                  'p', 'r', 'o', 'j', 'e', 'c', 't',
                  0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] =
{
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};


void ble_mesh_zephyr_ui_ready_cb(int err)
{
    if (err != 0)
    {
        BT_ERR("init err");
        s_init_finish_cb(err);
        return;
    }

    //    err = bt_id_init();

    err = bt_mesh_init(&prov, &comp);

    if (err)
    {
        BT_ERR("Mesh initialization failed (err %d)", err);
        assert(0);
        s_init_finish_cb(err);
    }

    //    bt_mesh_prov_enable(BT_MESH_PROV_ADV);//| BT_MESH_PROV_GATT);
    BT_INFO("Mesh initialized");


    //    char addr_s[BT_ADDR_LE_STR_LEN];
    //    bt_addr_le_t addr = {0};
    //    size_t count = 1;


    /* Start advertising */
    //        err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));


    if (err)
    {
        BT_ERR("Advertising failed to start (err %d)", err);
        s_init_finish_cb(err);
        return;
    }

    BT_INFO("Beacon started");
    (void)sd;
    (void)ad;

    s_init_finish_cb(err);
}


static void zephyr_mesh_thread(void *arg)
{
    int err = 0;
    //_delay_milliseconds_wrapper(2000);
    s_init_finish_cb = (typeof(s_init_finish_cb))arg;

#if 0
    err = bt_enable(ble_mesh_zephyr_ui_ready_cb);
#elif 0
    extern int bt_mesh_tmall_spirit_sample_init(void);
    extern int ble_mesh_led_sample_init(void);

    //    err = bt_mesh_tmall_spirit_sample_init();
    //    err = ble_mesh_led_sample_init();

#else
    s_init_finish_cb(0);
#endif

    if (err != 0)
    {
        BT_ERR("bt_enable fail %d", err);
    }

    while (1)
    {
        bk_err_t err;
        BLE_MESH_MSG_T msg;

        err = rtos_pop_from_queue(&ble_mesh_msg_que, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == err)
        {
            switch (msg.msg_id)
            {
            case BLE_MESH_MSG_FUNC:
            {
                ble_mesh_msg_func func = (ble_mesh_msg_func)msg.data;

                if (func)
                {
                    func(msg.data2);
                }
            }
            break;

            default:
                break;
            }
        }
        else
        {
            BT_ERR("pop_quene err %d", err);
            assert(0);
        }
    }

    rtos_deinit_queue(&ble_mesh_msg_que);
    rtos_delete_thread(NULL);
    ble_mesh_msg_que = NULL;
    ble_mesh_thread_handle = NULL;
}


uint32_t zephyr_ble_mesh_push_msg(uint32_t msg_id, void *data, uint32_t len, void *data2, uint32_t len2)
{
    bk_err_t ret = 0;

    if (ble_mesh_msg_que)
    {
        BLE_MESH_MSG_T msg = {0};

        msg.msg_id = msg_id;

        msg.data = data;
        msg.data2 = data2;

        ret = rtos_push_to_queue(&ble_mesh_msg_que, &msg, BEKEN_NO_WAIT);

        if (ret != 0)
        {
            BT_ERR("push err %d", ret);
        }
    }

    return 0;
}

void zephyr_ble_mesh_init(void *arg)
{
    bk_err_t ret;

    ARG_UNUSED(ret);
    if (!ble_mesh_thread_handle && !ble_mesh_msg_que)
    {

        ret = rtos_init_queue(&ble_mesh_msg_que,
                              "ble_mesh_msg_que",
                              sizeof(BLE_MESH_MSG_T),
                              8);
        __ASSERT(0 == ret, "create ble_mesh_msg_que error");

        ret = rtos_create_thread(&ble_mesh_thread_handle,
                                 4,
                                 "zephyr_mesh",
                                 (void*)zephyr_mesh_thread,
                                 2 * 1024,
                                 (void*)arg);

        __ASSERT(0 == ret, "create zephyr_mesh_thread error");
    }
}




#if 0
void ethermind_ble_mesh_init(void)
{
    int32_t err = bt_mesh_init(&prov, &comp);

    if (err)
    {
        printk("Initializing mesh failed (err %d)\n", err);
        return;
    }

    printk("Mesh initialized\n");

    if (IS_ENABLED(CONFIG_BT_SETTINGS))
    {
        printk("Loading stored settings\n");
        settings_load();
    }

    err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
                            dev_key);

    if (err == -EALREADY)
    {
        printk("Using stored settings\n");
    }
    else if (err)
    {
        printk("Provisioning failed (err %d)\n", err);
        return;
    }
    else
    {
        printk("Provisioning completed\n");
        configure();
    }

#if NODE_ADDR != PUBLISHER_ADDR
    /* Heartbeat subcscription is a temporary state (due to there
     * not being an "indefinite" value for the period, so it never
     * gets stored persistently. Therefore, we always have to configure
     * it explicitly.
     */
    {
        struct bt_mesh_cfg_hb_sub sub =
        {
            .src = PUBLISHER_ADDR,
            .dst = GROUP_ADDR,
            .period = 0x10,
        };

        bt_mesh_cfg_hb_sub_set(net_idx, addr, &sub, NULL);
        printk("Subscribing to heartbeat messages\n");
    }
#endif
}
#endif




BT_MESH_HB_CB_DEFINE(hb_cb) =
{
    .recv = heartbeat,
};

static void friend_established(uint16_t net_idx, uint16_t lpn_addr,
                               uint8_t recv_delay, uint32_t polltimeout)
{
    LOG_INF("Friend: established with 0x%04x", lpn_addr);
    //  friend_lpn_addr = lpn_addr;
    //  evt_signal(FRIEND_ESTABLISHED);
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
    LOG_INF("Friend: terminated with 0x%04x", lpn_addr);
    //  evt_signal(FRIEND_TERMINATED);
}

static void friend_polled(uint16_t net_idx, uint16_t lpn_addr)
{
    LOG_INF("Friend: Poll from 0x%04x", lpn_addr);
    //  evt_signal(FRIEND_POLLED);
}

BT_MESH_FRIEND_CB_DEFINE(friend) =
{
    .established = friend_established,
    .terminated = friend_terminated,
    .polled = friend_polled,
};


static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
                            uint8_t queue_size, uint8_t recv_window)
{
    LOG_INF("LPN: established with 0x%04x", friend_addr);
    //  evt_signal(LPN_ESTABLISHED);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
    LOG_INF("LPN: terminated with 0x%04x", friend_addr);
    //  evt_signal(LPN_TERMINATED);
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
    LOG_INF("LPN: Polling 0x%04x (%s)", friend_addr,
            retry ? "retry" : "initial");
    //  evt_signal(LPN_POLLED);
}

BT_MESH_LPN_CB_DEFINE(lpn) =
{
    .established = lpn_established,
    .polled = lpn_polled,
    .terminated = lpn_terminated,
};

int32_t mesh_foreach_k_mem_slab_cb(int32_t (*cb)(struct k_mem_slab *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                   void *arg1, void *arg2, void *arg3, void *arg4)
{
    extern struct k_mem_slab segs;
    extern struct k_mem_slab att_slab;
    extern struct k_mem_slab chan_slab;
    extern struct k_mem_slab req_slab;

    uint8_t i = 0;
    int32_t ret = 0;
    struct k_mem_slab *slab[] = {&segs,&att_slab,&chan_slab,&req_slab};
    for (i = 0; i < sizeof(slab) / sizeof(slab[0]); i++)
    {
        ret = cb(slab[i], arg1, arg2, arg3, arg4);

        if (ret != 0)
        {
            return ret;
        }
    }
    return 0;
}

void mesh_foreach_bt_mesh_lpn_cb(void (*cb)(struct bt_mesh_lpn_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                 void *arg1, void *arg2, void *arg3, void *arg4)
{
    cb((struct bt_mesh_lpn_cb *)&bt_mesh_lpn_cb_lpn, arg1, arg2, arg3, arg4);
}


void mesh_foreach_bt_mesh_hb_cb(void (*cb)(struct bt_mesh_hb_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                void *arg1, void *arg2, void *arg3, void *arg4)
{
    cb((struct bt_mesh_hb_cb *)&hb_cb, arg1, arg2, arg3, arg4);
}

void mesh_foreach_bt_mesh_subnet_cb(//struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt
    void (*cb)(struct bt_mesh_subnet_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
    void *arg1, void *arg2, void *arg3, void *arg4)
{
    //BT_MESH_SUBNET_CB_DEFINE
    extern struct bt_mesh_subnet_cb bt_mesh_subnet_cb_app_keys[1];
    extern struct bt_mesh_subnet_cb bt_mesh_subnet_cb_beacon[1];

    uint8_t i = 0;

    for (i = 0; i < sizeof(bt_mesh_subnet_cb_app_keys) / sizeof(bt_mesh_subnet_cb_app_keys[0]); i++)
    {
        cb(&bt_mesh_subnet_cb_app_keys[i], arg1, arg2, arg3, arg4);
    }

    for (i = 0; i < sizeof(bt_mesh_subnet_cb_beacon) / sizeof(bt_mesh_subnet_cb_beacon[0]); i++)
    {
        cb(&bt_mesh_subnet_cb_beacon[i], arg1, arg2, arg3, arg4);
    }

#if CONFIG_BT_MESH_FRIEND
    extern struct bt_mesh_subnet_cb bt_mesh_subnet_cb_friend[1];
    for (i = 0; i < sizeof(bt_mesh_subnet_cb_friend) / sizeof(bt_mesh_subnet_cb_friend[0]); i++)
    {
        cb(&bt_mesh_subnet_cb_friend[i], arg1, arg2, arg3, arg4);
    }

#endif

#if CONFIG_BT_MESH_LOW_POWER
    extern struct bt_mesh_subnet_cb bt_mesh_subnet_cb_lpn[1];
    for (i = 0; i < sizeof(bt_mesh_subnet_cb_lpn) / sizeof(bt_mesh_subnet_cb_lpn[0]); i++)
    {
        cb(&bt_mesh_subnet_cb_lpn[i], arg1, arg2, arg3, arg4);
    }

#endif
}

void mesh_foreach_bt_mesh_app_key_cb(void (*cb)(struct bt_mesh_app_key_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                     void *arg1, void *arg2, void *arg3, void *arg4)
{
    //BT_MESH_APP_KEY_CB_DEFINE
    extern struct bt_mesh_app_key_cb bt_mesh_app_key_cb_app_key_evt[1];

    uint8_t i = 0;

    for (i = 0; i < sizeof(bt_mesh_app_key_cb_app_key_evt) / sizeof(bt_mesh_app_key_cb_app_key_evt[0]); i++)
    {
        cb(&bt_mesh_app_key_cb_app_key_evt[i], arg1, arg2, arg3, arg4);
    }

}

void mesh_foreach_bt_mesh_friend_cb(void (*cb)(struct bt_mesh_friend_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                    void *arg1, void *arg2, void *arg3, void *arg4)
{
    //BT_MESH_FRIEND_CB_DEFINE
    //extern struct bt_mesh_friend_cb bt_mesh_friend_cb_friend[1];

    //    uint8_t i = 0;
    //
    //    for (i = 0; i < sizeof(bt_mesh_friend_cb_friend) / sizeof(bt_mesh_friend_cb_friend[0]); i++)
    //    {
    //      cb(&bt_mesh_friend_cb_friend[i], arg1, arg2, arg3, arg4);
    //    }

    cb((struct bt_mesh_friend_cb *)&bt_mesh_friend_cb_friend, arg1, arg2, arg3, arg4);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    LOG_INF("Connected: %d", err);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected: (reason 0x%02x)", reason);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
    LOG_INF("LE conn  param req: int (0x%04x, 0x%04x) lat %d"
            " to %d", param->interval_min, param->interval_max,
            param->latency, param->timeout);

    return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
                 uint16_t latency, uint16_t timeout)
{
    LOG_INF("LE conn param updated: int 0x%04x lat %d "
             "to %d", interval, latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                  const bt_addr_le_t *identity)
{
    LOG_INF("Identity resolved");
}
#endif

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
    LOG_INF("security changed");
}
#endif

#if defined(CONFIG_BT_REMOTE_INFO)
static void remote_info_available(struct bt_conn *conn,
				  struct bt_conn_remote_info *remote_info)
{
    LOG_INF("remote_info_available");
}
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void le_data_len_updated(struct bt_conn *conn,
			 struct bt_conn_le_data_len_info *info)
{
	LOG_INF("LE data len updated: TX (len: %d time: %d)"
		    " RX (len: %d time: %d)", info->tx_max_len,
		    info->tx_max_time, info->rx_max_len, info->rx_max_time);
}
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
void le_phy_updated(struct bt_conn *conn,
		    struct bt_conn_le_phy_info *info)
{
	LOG_INF("LE PHY updated: TX PHY %s, RX PHY %s",
		    phy2str(info->tx_phy), phy2str(info->rx_phy));
}
#endif


BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_req = le_param_req,
    .le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
    .identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	.security_changed = security_changed,
#endif
#if defined(CONFIG_BT_REMOTE_INFO)
    .remote_info_available = remote_info_available,
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
    .le_data_len_updated = le_data_len_updated,
#endif
#if defined(CONFIG_BT_USER_PHY_UPDATE)
    .le_phy_updated = le_phy_updated,
#endif
};

int32_t mesh_foreach_bt_conn_cb(int32_t (*cb)(struct bt_conn_cb *param, void *arg1, void *arg2, void *arg3, void *arg4),
                                     void *arg1, void *arg2, void *arg3, void *arg4)
{
    //BT_MESH_APP_KEY_CB_DEFINE
    const struct bt_conn_cb *bt_conn_cb_conn_cb[1] = {&bt_conn_cb_conn_callbacks};

    uint8_t i = 0;

    for (i = 0; i < sizeof(bt_conn_cb_conn_cb) / sizeof(bt_conn_cb_conn_cb[0]); i++)
    {
        if(!cb((struct bt_conn_cb *)bt_conn_cb_conn_cb[i], arg1, arg2, arg3, arg4))
        {
            return false;
        }
    }
    return true;
}

