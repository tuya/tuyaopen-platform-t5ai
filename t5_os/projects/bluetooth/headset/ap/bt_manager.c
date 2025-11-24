#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "bluetooth_storage.h"
#include "bt_manager.h"
#include "headset_user_config.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG "btm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define LOCAL_NAME "soundbar"
#define MAX_PROFILE_NUM 10

typedef struct
{
    uint8_t mode;
    uint8_t connect_state;
    uint8_t manual_enter_pairing;
    beken2_timer_t recon_tmr;
    uint8_t peer_addr[6];
    uint8_t recon_addr[6];
    uint8_t tmp_link_key[16];//BT_LINK_KEY_SIZE];
} btm_env_s;

static btm_env_s btm_env={0};
static btm_callback_s btm_cbs[MAX_PROFILE_NUM] = {0};


void bt_stop_reconnect_timeout_check(void)
{
    if (rtos_is_oneshot_timer_init(&btm_env.recon_tmr))
    {
        if (rtos_is_oneshot_timer_running(&btm_env.recon_tmr))
        {
            rtos_stop_oneshot_timer(&btm_env.recon_tmr);
        }
        rtos_deinit_oneshot_timer(&btm_env.recon_tmr);
    }

    for(uint8_t i=0;i<MAX_PROFILE_NUM;i++)
    {
        if(btm_cbs[i].stop_connect_cb)
        {
            btm_cbs[i].stop_connect_cb();
        }
    }
}

void bk_bt_enter_pairing_mode(void)
{
    bt_stop_reconnect_timeout_check();

    if (BT_STATE_RECONNECTING == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing= 1;
        bk_bt_gap_create_conn_cancel(btm_env.recon_addr);
    }
    else if (BT_STATE_LINK_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing= 1;
        bk_bt_gap_disconnect(btm_env.peer_addr, 0x13);
    }
    else if (BT_STATE_PROFILE_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = 1;
        for(int i=0;i<MAX_PROFILE_NUM;i++)
        {
            if(btm_cbs[i].start_disconnect_cb)
            {
                btm_cbs[i].start_disconnect_cb(btm_env.peer_addr);
            }
        }
    }
    else
    {
        bt_manager_set_mode(BT_MNG_MODE_PAIRING);
    }

    btm_env.connect_state = BT_STATE_IDLE;
}

static char *bt_manager_mode_2_str(uint8_t mode)
{
    switch(mode)
    {
    case BT_MNG_MODE_PAIRING:
        return "paring-connable-inqable";
    case BT_MNG_MODE_RECONNECTING:
        return "reconnectin-conndisable-inqdisable";
    case BT_MNG_MODE_CONNECTEED:
        return "connected-conndisable-inqdisable";
    case BT_MNG_MODE_CONNECTABLE:
        return "connable-inqdisable";
    }
    return "unknow mode";
}


void bt_manager_set_mode(uint8_t mode)
{
    LOGI("%s: %d -> %d\n", __func__, btm_env.mode, mode);
    LOGI("-> %s \n", bt_manager_mode_2_str(mode));

    if (btm_env.mode == mode) return;

    switch(mode)
    {
    case BT_MNG_MODE_PAIRING:
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        break;
    case BT_MNG_MODE_RECONNECTING:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_CONNECTEED:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_CONNECTABLE:
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    default:
        break;
    }

    btm_env.mode = mode;
}

void link_timeout_start_reconnect_timer_hdl(void *param, unsigned int ulparam)
{
    rtos_deinit_oneshot_timer(&btm_env.recon_tmr);

    for(int i=0; i<MAX_PROFILE_NUM; i++)
    {
        if(btm_cbs[i].start_connect_cb)
        {
            btm_cbs[i].start_connect_cb(btm_env.recon_addr);
            if(btm_env.connect_state == BT_STATE_WAIT_FOR_RECONNECT)
            {
                return;
            }
        }
    }
    btm_env.connect_state = BT_STATE_RECONNECTING;
}

void bt_manager_start_reconnect(uint8_t *addr, uint8_t immediate)
{
    uint32_t time_ms = 200;

    btm_env.connect_state = BT_STATE_IDLE;

    os_memcpy(btm_env.recon_addr, addr, 6);
    bt_manager_set_mode(BT_MNG_MODE_RECONNECTING);

    if (!immediate)
    {
        time_ms = CONFIG_RECONN_INTERVAL;
    }

    if (!rtos_is_oneshot_timer_init(&btm_env.recon_tmr))
    {
        rtos_init_oneshot_timer(&btm_env.recon_tmr, time_ms, (timer_2handler_t)link_timeout_start_reconnect_timer_hdl, NULL, 0);
        rtos_start_oneshot_timer(&btm_env.recon_tmr);
    }
}

static void bt_clear_reconnect_info(void)
{
    os_memset(btm_env.recon_addr, 0, 6);
    btm_env.connect_state = BT_STATE_IDLE;
}


void gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    switch (event)
    {
        case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        {
            uint8_t *addr = param->acl_disconn_cmpl_stat.bda;
            LOGI("Disconnected from %x %x %x %x %x %x, reason 0x%02x \r\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], param->acl_disconn_cmpl_stat.reason);
            //bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);

            if (btm_env.manual_enter_pairing)
            {
                bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                btm_env.manual_enter_pairing = 0;
                break;
            }

            if ((BK_BT_STATUS_REMOTE_USER_TERM_CON == param->acl_disconn_cmpl_stat.reason || BK_BT_STATUS_CON_TERM_BY_LOCAL_HOST == param->acl_disconn_cmpl_stat.reason)
                && (BT_STATE_PROFILE_CONNECTED == btm_env.connect_state || BT_STATE_KEY_MISSING == btm_env.connect_state))
            {
                os_memset(btm_env.peer_addr, 0, 6);
                bt_clear_reconnect_info();
#if 1
                bluetooth_storage_sync_to_flash();
                LOGI("%s sync to flash done\n", __func__);
#endif
                if (bluetooth_storage_find_linkkey_info_index(addr, NULL) < 0)
                {
                    bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                }
                else
                {
                    bt_manager_set_mode(BT_MNG_MODE_CONNECTABLE);
                }
            }
            else //if (BK_BT_STATUS_CON_TIMEOUT == cb->acl_disconn_cmpl_stat.reason)
            {
                bt_manager_start_reconnect(addr, 1);
            }
        }
        break;

        case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        {
            uint8_t *addr = param->acl_conn_cmpl_stat.bda;
            if (0 == param->acl_conn_cmpl_stat.stat)
            {
                LOGI("Connected to %02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                os_memcpy(btm_env.peer_addr, addr, 6);
                btm_env.connect_state = BT_STATE_LINK_CONNECTED;
                bt_manager_set_mode(BT_MNG_MODE_CONNECTEED);
            }
            else
            {
                LOGI("Connect the %02x:%02x:%02x:%02x:%02x:%02x Failed, status 0x%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],param->acl_conn_cmpl_stat.stat);

                if (btm_env.manual_enter_pairing)
                {
                    bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                    btm_env.manual_enter_pairing = 0;
                    break;
                }

                if ((BK_BT_STATUS_PAGE_TIMEOUT == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_CON_ALREADY_EXISTS == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_UNKNOWN_CONNECTION_ID == param->acl_conn_cmpl_stat.stat)
                    && (BT_STATE_RECONNECTING == btm_env.connect_state))
                {
                    bt_manager_start_reconnect(addr, 0);
                }
                else
                {
                    bt_clear_reconnect_info();
                    bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                }
            }
        }
        break;

        case BK_BT_GAP_AUTH_CMPL_EVT:
    {
        uint8_t *addr = param->auth_cmpl.bda;
        if (0 == param->auth_cmpl.stat)
        {
            LOGI("(%02x:%02x:%02x:%02x:%02x:%02x)authentication success\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        }
        else
        {
            if (BK_BT_STATUS_PIN_MISSING == param->auth_cmpl.stat || BK_BT_STATUS_AUTH_FAILURE == param->auth_cmpl.stat)
            {
                bluetooth_storage_del_linkkey_info(addr);
                btm_env.connect_state = BT_STATE_KEY_MISSING;
            }
            LOGI("(%02x:%02x:%02x:%02x:%02x:%02x)authentication failed, status: 0x%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],param->auth_cmpl.stat);
        }
    }
    break;

    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
    {
        LOGI("%s recv linkkey %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                  param->link_key_notif.bda[5],
                  param->link_key_notif.bda[4],
                  param->link_key_notif.bda[3],
                  param->link_key_notif.bda[2],
                  param->link_key_notif.bda[1],
                  param->link_key_notif.bda[0]);

        uint8_t log_buff[16 * 2 + 10] = {0};
        for (int i = 0; i < sizeof(param->link_key_notif.link_key); ++i)
        {
            sprintf((char *)(log_buff + i * 2), "%02X", param->link_key_notif.link_key[i]);
        }

        LOGI("%s %s\n", __func__, log_buff);

        int ret = bluetooth_storage_save_linkkey_info(param->link_key_notif.bda, param->link_key_notif.link_key);
        // s_a2dp_vol = DEFAULT_A2DP_VOLUME;
        // bluetooth_storage_save_volume(param->link_key_notif.bda, s_a2dp_vol);

        if (ret <= 0)
        {
            LOGE("%s save link key fail %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      param->link_key_notif.bda[5],
                      param->link_key_notif.bda[4],
                      param->link_key_notif.bda[3],
                      param->link_key_notif.bda[2],
                      param->link_key_notif.bda[1],
                      param->link_key_notif.bda[0]);
        }
        else
        {
            bluetooth_storage_update_to_newest(param->link_key_notif.bda);
#if 1
            bluetooth_storage_sync_to_flash();
            LOGI("%s sync to flash done\n", __func__);
#endif
        }

    }
    break;

    case BK_BT_GAP_LINK_KEY_REQ_EVT:
    {
        uint8_t *addr = param->link_key_req.bda;
        bk_bt_linkkey_storage_t tmp;
        int ret = 0;
        uint8_t zero_linkkey[16] = {0};
        uint8_t ff_linkkey[16] = {0};
        uint8_t found_key = 0;

        memset(&tmp, 0, sizeof(tmp));
        memcpy(tmp.addr, addr, sizeof(tmp.addr));

        os_memset(ff_linkkey, 0xff, sizeof(ff_linkkey));

        ret = bluetooth_storage_find_linkkey_info_index(addr, tmp.link_key);

        if (ret >= 0)
        {
            LOGI("%s found link key %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            uint8_t log_buff[16 * 2 + 10] = {0};
            for (int i = 0; i < sizeof(tmp.link_key); ++i)
            {
                sprintf((char *)(log_buff + i * 2), "%02X", tmp.link_key[i]);
            }

            LOGI("%s %s\n", __func__, log_buff);

            found_key = 1;
        }
        else if(os_memcmp(btm_env.tmp_link_key, zero_linkkey, sizeof(btm_env.tmp_link_key)) &&
                        os_memcmp(btm_env.tmp_link_key, ff_linkkey, sizeof(btm_env.tmp_link_key)))
        {
            LOGI("%s use tmp linkkey\n", __func__);
            os_memcpy(tmp.link_key, btm_env.tmp_link_key, sizeof(btm_env.tmp_link_key));
            found_key = 1;
        }

        if(found_key)
        {
            bk_bt_gap_linkkey_reply(1, &tmp);
        }
        else
        {
            LOGI("%s not found link key in storage %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            memset(tmp.link_key, 0, sizeof(tmp.link_key));

            bk_bt_gap_linkkey_reply(0, &tmp);
        }
    }
    break;

    default:
        break;
    }

    for(int i=0;i<MAX_PROFILE_NUM;i++)
    {
        if(btm_cbs[i].gap_cb != NULL)
        {
            btm_cbs[i].gap_cb(event, param);
        }
    }
}

int bt_manager_init()
{
    LOGI("%s\r\n", __func__);
    int ret = 0;
    ret = bluetooth_storage_init();

    if (ret)
    {
        LOGE("%s bluetooth_storage_init err %d\n", __func__, ret);
        return -1;
    }
    os_memset(&btm_env, 0, sizeof(btm_env_s));
    os_memset(&btm_cbs, 0, sizeof(btm_cbs));
    bk_bt_gap_register_callback(gap_event_cb);
    bk_bt_gap_set_device_class(COD_SOUNDBAR);
    uint8_t bt_mac[6];
    char local_name[30] = {0};
    bk_bluetooth_get_address((uint8_t *)bt_mac);
    snprintf(local_name, 30, "%s_%02x%02x%02x", LOCAL_NAME, bt_mac[2], bt_mac[1], bt_mac[0]);
    bk_bt_gap_set_local_name((uint8_t *)local_name, os_strlen(local_name));

    bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);

    bk_bt_gap_set_page_timeout(CONFIG_PAGE_TIMEOUT);
    bk_bt_gap_set_page_scan_activity(PAGE_SCAN_INTV, PAGE_SCAN_WIN);
    return 0;
}

int bt_manager_register_callback(btm_callback_s *cb)
{
    int i=0;
    for(;i<MAX_PROFILE_NUM;i++)
    {
        if(
            btm_cbs[i].gap_cb == NULL
            && btm_cbs[i].start_connect_cb == NULL
            && btm_cbs[i].stop_connect_cb == NULL
        )
        {
            btm_cbs[i].gap_cb = cb->gap_cb;
            btm_cbs[i].start_connect_cb = cb->start_connect_cb;
            btm_cbs[i].stop_connect_cb = cb->stop_connect_cb;
            return i;
        }
    }
    LOGE("%s, callback max resource, reg fail !! \n", __func__);
    return MAX_PROFILE_NUM;
}

uint8_t bt_manager_get_connect_state()
{
    return btm_env.connect_state;
}

void bt_manager_set_connect_state(uint8_t state)
{
    btm_env.connect_state = state;
}

uint8_t* bt_manager_get_reconnect_device()
{
    return btm_env.recon_addr;
}

uint8_t* bt_manager_get_connected_device()
{
    return btm_env.peer_addr;
}

void bt_manager_set_tmp_linkkey(uint8_t *addr, uint8_t *linkkey)
{
    LOGI("%s set tmp linkkey\n", __func__);
    os_memcpy(btm_env.tmp_link_key, linkkey, sizeof(btm_env.tmp_link_key));
}
