#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"
#include "bluetooth_storage.h"

typedef int32_t (* dm_ble_gap_app_cb)(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param);

static beken_semaphore_t s_ble_sema = NULL;
static dm_ble_gap_app_cb s_gap_cb_list[4];
static bk_bd_addr_t s_peer_bdaddr;
static bk_ble_bond_dev_t s_dm_gatt_bond_dev_list[GATT_MAX_BOND_COUNT];
static uint8_t s_dm_gatt_is_inited;

#if 1
    static uint8_t s_dm_gatt_iocap = BK_IO_CAP_NONE;
    static uint8_t s_dm_gatt_auth_req = BK_LE_AUTH_REQ_BOND_MITM;
    static uint8_t s_dm_gatt_key_distr = BK_BLE_KEY_DISTR_ENC_KEY_MASK | BK_BLE_KEY_DISTR_ID_KEY_MASK | BK_BLE_KEY_DISTR_CSR_KEY_MASK;
#elif 0
    static uint8_t s_dm_gatt_iocap = BK_IO_CAP_DISPLAY_YESNO;
    static uint8_t s_dm_gatt_auth_req = BK_LE_AUTH_REQ_BOND_MITM;
    static uint8_t s_dm_gatt_key_distr = BK_BLE_KEY_DISTR_ENC_KEY_MASK | BK_BLE_KEY_DISTR_ID_KEY_MASK | BK_BLE_KEY_DISTR_CSR_KEY_MASK;
#elif 0
    static uint8_t s_dm_gatt_iocap = BK_IO_CAP_DISPLAY_YESNO;
    static uint8_t s_dm_gatt_auth_req = BK_LE_AUTH_REQ_SC_MITM_BOND;
    static uint8_t s_dm_gatt_key_distr = BK_BLE_KEY_DISTR_ENC_KEY_MASK | BK_BLE_KEY_DISTR_ID_KEY_MASK | BK_BLE_KEY_DISTR_CSR_KEY_MASK;
#elif 1
    static uint8_t s_dm_gatt_iocap = BK_IO_CAP_DISPLAY_YESNO;
    static uint8_t s_dm_gatt_auth_req = BK_LE_AUTH_REQ_SC_MITM_BOND;
    static uint8_t s_dm_gatt_key_distr = BK_BLE_KEY_DISTR_ENC_KEY_MASK | BK_BLE_KEY_DISTR_ID_KEY_MASK | BK_BLE_KEY_DISTR_CSR_KEY_MASK | BK_BLE_KEY_DISTR_LINK_KEY_MASK;
#endif

static bk_ble_bond_dev_t *dm_ble_find_bond_info_by_nominal_addr(uint8_t *addr, bk_ble_addr_type_t addr_type)
{
    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    const uint8_t ff_addr[BK_BD_ADDR_LEN] = {[0 ... (BK_BD_ADDR_LEN - 1)] = 0xff};

    if (!os_memcmp(null_addr, addr, BK_BD_ADDR_LEN) ||
            !os_memcmp(ff_addr, addr, BK_BD_ADDR_LEN) )
    {
        return NULL;
    }

    for (int i = 0; i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]); ++i)
    {
        if (os_memcmp(null_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) &&
                os_memcmp(ff_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) &&
                !os_memcmp(s_dm_gatt_bond_dev_list[i].bd_addr, addr, sizeof(addr))
                //&& s_dm_gatt_bond_dev_list[i].key.pid_key.addr_type == addr_type  //todo: addr_type not used now.
           )
        {
            return &s_dm_gatt_bond_dev_list[i];
        }
    }

    return NULL;
}

static uint8_t dm_ble_del_bond_info_by_nominal_addr(uint8_t *addr)
{
    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    const uint8_t ff_addr[BK_BD_ADDR_LEN] = {[0 ... (BK_BD_ADDR_LEN - 1)] = 0xff};

    if (!os_memcmp(null_addr, addr, BK_BD_ADDR_LEN) ||
            !os_memcmp(ff_addr, addr, BK_BD_ADDR_LEN) )
    {
        return 1;
    }

    for (int i = 0; i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]); ++i)
    {
        if (!os_memcmp(s_dm_gatt_bond_dev_list[i].bd_addr, addr, BK_BD_ADDR_LEN))
        {
            os_memset(&s_dm_gatt_bond_dev_list[i], 0, sizeof(s_dm_gatt_bond_dev_list[i]));
            return 0;
        }
    }

    return 1;
}

static uint8_t dm_ble_clean_bond_info(void)
{
    os_memset(s_dm_gatt_bond_dev_list, 0, sizeof(s_dm_gatt_bond_dev_list));

    return 0;
}

static bk_ble_bond_dev_t *dm_ble_alloc_bond_info_by_nominal_addr(uint8_t *addr)
{
    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    bk_ble_bond_dev_t *tmp = NULL;

    tmp = dm_ble_find_bond_info_by_nominal_addr(addr, 0);

    if (tmp)
    {
        return tmp;
    }

    for (int i = 0; i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]); ++i)
    {
        if (!os_memcmp(s_dm_gatt_bond_dev_list[i].bd_addr, null_addr, BK_BD_ADDR_LEN))
        {
            return &s_dm_gatt_bond_dev_list[i];
        }
    }

    return NULL;
}


static void dm_ble_gap_private_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
    int32_t ret;

    for (int i = 0; i < sizeof(s_gap_cb_list) / sizeof(s_gap_cb_list[0]); ++i)
    {
        if (s_gap_cb_list[i])
        {
            ret = s_gap_cb_list[i](event, param);

            if (ret == DM_BLE_GAP_APP_CB_RET_PROCESSED)
            {
                return;
            }
        }
    }
}

static int32_t dm_ble_gap_common_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
    dm_gatt_app_env_t *dm_gatt_env_p = NULL;
    bk_ble_bond_dev_t *dm_bond_dev_p = NULL;

    gatt_logd("event %d", event);

    switch (event)
    {
    case BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT:
    {
        struct ble_update_conn_params_evt_param *pm = (typeof(pm))param;

        gatt_logi("BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT %d 0x%x 0x%x", pm->status, pm->max_int, pm->min_int);

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }

    break;

    case BK_BLE_GAP_AUTH_CMPL_EVT: //todo: report when second connect
    {
        bk_ble_sec_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_AUTH_CMPL_EVT %s %d %02X:%02X:%02X:%02X:%02X:%02X, 0x%x 0x%x 0x%x",
                  pm->auth_cmpl.success ? "sucess" : "fail", pm->auth_cmpl.fail_reason,
                  pm->auth_cmpl.bd_addr[5],
                  pm->auth_cmpl.bd_addr[4],
                  pm->auth_cmpl.bd_addr[3],
                  pm->auth_cmpl.bd_addr[2],
                  pm->auth_cmpl.bd_addr[1],
                  pm->auth_cmpl.bd_addr[0],
                  pm->auth_cmpl.addr_type,
                  pm->auth_cmpl.dev_type,
                  pm->auth_cmpl.auth_mode
                 );

        dm_gatt_env_p = dm_ble_find_app_env_by_addr(pm->auth_cmpl.bd_addr);

        if (!dm_gatt_env_p)
        {
            gatt_loge("app env not found !!!");
        }

        if (pm->auth_cmpl.success)
        {
            if (dm_gatt_env_p)
            {
                dm_gatt_env_p->is_authen = 1;
            }
        }
        else
        {
            if (dm_gatt_env_p)
            {
                dm_gatt_env_p->is_authen = 0;
            }

            dm_ble_del_bond_info_by_nominal_addr(pm->auth_cmpl.bd_addr);
        }
    }
    break;

    case BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT:
    {
        struct ble_bond_dev_list_evt_param *pm = (typeof(pm))param;

        gatt_logi("BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT %d %d", pm->status, pm->op);

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_BOND_KEY_GENERATE_EVT:
    {
        bk_ble_gap_cb_param_t *tmp = (typeof(tmp))param;
        struct ble_bond_dev_key_evt_param *pm = &tmp->bond_dev_key_generate_evt;
        char tmp_log[128] = {0};
        int index = 0;

        gatt_logi("BK_BLE_GAP_BOND_KEY_GENERATE_EVT type 0x%x %02x:%02x:%02x:%02x:%02x:%02x %d",
                  pm->bond_dev.bond_key.key_mask,
                  pm->bond_dev.bd_addr[5],
                  pm->bond_dev.bd_addr[4],
                  pm->bond_dev.bd_addr[3],
                  pm->bond_dev.bd_addr[2],
                  pm->bond_dev.bd_addr[1],
                  pm->bond_dev.bd_addr[0],
                  pm->bond_dev.addr_type);

        dm_bond_dev_p = dm_ble_alloc_bond_info_by_nominal_addr(pm->bond_dev.bd_addr);

        if (!dm_bond_dev_p)
        {
            gatt_loge("app env buff full !!!");
            break;
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_PENC)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "peer enc\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "ltk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.penc_key.ltk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.penc_key.ltk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            gatt_logi("%s", tmp_log);
            os_memset(tmp_log, 0, sizeof(tmp_log));
            index = 0;

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "rand:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.penc_key.rand); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.penc_key.rand[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "ediv 0x%x keysize %d seclevel 0x%x\n",
                              pm->bond_dev.bond_key.penc_key.ediv,
                              pm->bond_dev.bond_key.penc_key.key_size,
                              pm->bond_dev.bond_key.penc_key.sec_level);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.penc_key, &pm->bond_dev.bond_key.penc_key, sizeof(pm->bond_dev.bond_key.penc_key));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_PID)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "peer idkey\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "irk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.pid_key.irk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.pid_key.irk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "addr:%02X:%02X:%02X:%02X:%02X:%02X %d\n",
                              pm->bond_dev.bond_key.pid_key.static_addr[5],
                              pm->bond_dev.bond_key.pid_key.static_addr[4],
                              pm->bond_dev.bond_key.pid_key.static_addr[3],
                              pm->bond_dev.bond_key.pid_key.static_addr[2],
                              pm->bond_dev.bond_key.pid_key.static_addr[1],
                              pm->bond_dev.bond_key.pid_key.static_addr[0],
                              pm->bond_dev.bond_key.pid_key.addr_type);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.pid_key, &pm->bond_dev.bond_key.pid_key, sizeof(pm->bond_dev.bond_key.pid_key));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_PCSRK)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "peer csrk\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "csrk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.pcsrk_key.csrk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.pcsrk_key.csrk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "counter: %d seclevel %d\n",
                              pm->bond_dev.bond_key.pcsrk_key.counter,
                              pm->bond_dev.bond_key.pcsrk_key.sec_level);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.pcsrk_key, &pm->bond_dev.bond_key.pcsrk_key, sizeof(pm->bond_dev.bond_key.pcsrk_key));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_PLK)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_LLK)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "local linkkey\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "linkkey:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.llink_key.key); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.llink_key.key[i]);
            }

            gatt_logi("%s", tmp_log);
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_LENC)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "local enc\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "ltk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.lenc_key.ltk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.lenc_key.ltk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");


            gatt_logi("%s", tmp_log);
            os_memset(tmp_log, 0, sizeof(tmp_log));
            index = 0;

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "rand:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.lenc_key.rand); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.lenc_key.rand[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");


            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "div 0x%x keysize %d seclevel %d\n",
                              pm->bond_dev.bond_key.lenc_key.div,
                              pm->bond_dev.bond_key.lenc_key.key_size,
                              pm->bond_dev.bond_key.lenc_key.sec_level);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.lenc_key, &pm->bond_dev.bond_key.lenc_key, sizeof(pm->bond_dev.bond_key.lenc_key));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_LID)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "local idkey\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "irk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.lid_key.irk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.lid_key.irk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "addr:%02X:%02X:%02X:%02X:%02X:%02X %d\n",
                              pm->bond_dev.bond_key.lid_key.static_addr[5],
                              pm->bond_dev.bond_key.lid_key.static_addr[4],
                              pm->bond_dev.bond_key.lid_key.static_addr[3],
                              pm->bond_dev.bond_key.lid_key.static_addr[2],
                              pm->bond_dev.bond_key.lid_key.static_addr[1],
                              pm->bond_dev.bond_key.lid_key.static_addr[0],
                              pm->bond_dev.bond_key.lid_key.addr_type);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.lid_key, &pm->bond_dev.bond_key.lid_key, sizeof(pm->bond_dev.bond_key.lid_key));
        }

        if (pm->bond_dev.bond_key.key_mask & BK_LE_KEY_LCSRK)
        {
            index = 0;
            os_memset(tmp_log, 0, sizeof(tmp_log));

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "local csrk\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "csrk:");

            for (int i = 0; i < sizeof(pm->bond_dev.bond_key.lcsrk_key.csrk); ++i)
            {
                index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", pm->bond_dev.bond_key.lcsrk_key.csrk[i]);
            }

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "\n");

            index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "counter: %d seclevel %d\n",
                              pm->bond_dev.bond_key.lcsrk_key.counter,
                              pm->bond_dev.bond_key.lcsrk_key.sec_level);

            gatt_logi("%s", tmp_log);

            //os_memcpy(&dm_bond_dev_p->key.lcsrk_key, &pm->bond_dev.bond_key.lcsrk_key, sizeof(pm->bond_dev.bond_key.lcsrk_key));
        }

        os_memcpy(dm_bond_dev_p, &pm->bond_dev, sizeof(*dm_bond_dev_p));
#if 0

        if ((pm->bond_dev.bond_key.key_mask & (BK_LE_KEY_PID | BK_LE_KEY_LLK)) == (BK_LE_KEY_PID | BK_LE_KEY_LLK))
        {
            extern int32_t bt_a2dp_source_demo_set_linkkey(uint8_t *addr, uint8_t *linkkey);
            bt_a2dp_source_demo_set_linkkey(pm->bond_dev.bond_key.pid_key.static_addr, pm->bond_dev.bond_key.llink_key.key);
        }

#endif

#if BLE_USE_STORAGE
        bluetooth_storage_save_ble_key_info(s_dm_gatt_bond_dev_list, sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]));
        bluetooth_storage_sync_to_flash();
#endif
    }
    break;

    case BK_BLE_GAP_SEC_REQ_EVT:
    {
        bk_ble_gap_cb_param_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_SEC_REQ_EVT");
    }
    break;

    case BK_BLE_GAP_PASSKEY_NOTIF_EVT:
    {
        bk_ble_gap_cb_param_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_PASSKEY_NOTIF_EVT %06d, pls input on peer device !!!!", pm->ble_security.key_notif.passkey);
    }
    break;

    case BK_BLE_GAP_PASSKEY_REQ_EVT:
    {
        bk_ble_gap_cb_param_t *pm = (typeof(pm))param;

        gatt_logi("BK_BLE_GAP_PASSKEY_REQ_EVT, pls input num that peer device display !!!!");

        os_memcpy(s_peer_bdaddr, pm->ble_security.ble_req.bd_addr, sizeof(pm->ble_security.ble_req.bd_addr));
        //bk_ble_passkey_reply(pm->ble_req.bd_addr, true, 0);
    }
    break;

    case BK_BLE_GAP_NC_REQ_EVT:
    {
        bk_ble_gap_cb_param_t *pm = (typeof(pm))param;

        gatt_logi("BK_BLE_GAP_NC_REQ_EVT number compare %06d", pm->ble_security.key_notif.passkey);

        bk_ble_confirm_reply(pm->ble_security.key_notif.bd_addr, true);
    }
    break;

    case BK_BLE_GAP_OOB_REQ_EVT:
    {
        bk_ble_gap_cb_param_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_OOB_REQ_EVT");
    }
    break;

#if 0

    case BK_BLE_GAP_LOCAL_IR_EVT:
    {
        bk_ble_sec_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_LOCAL_IR_EVT");
    }
    break;

    case BK_BLE_GAP_LOCAL_ER_EVT:
    {
        bk_ble_sec_t *pm = (typeof(pm))param;
        gatt_logi("BK_BLE_GAP_LOCAL_ER_EVT");
    }
    break;
#endif

    case BK_BLE_GAP_BOND_KEY_REQ_EVT:
    {
        bk_ble_gap_cb_param_t *tmp = (typeof(tmp))param;
        struct ble_bond_dev_key_req_evt_param *pm = &tmp->bond_key_req_evt;
        gatt_logi("BK_BLE_GAP_BOND_KEY_REQ_EVT 0x%x %02x:%02x:%02x:%02x:%02x:%02x",
                  pm->key_req.key_type,
                  pm->key_req.peer_addr[5],
                  pm->key_req.peer_addr[4],
                  pm->key_req.peer_addr[3],
                  pm->key_req.peer_addr[2],
                  pm->key_req.peer_addr[1],
                  pm->key_req.peer_addr[0]);

        if (pm->key_req.key_type == BK_LE_KEY_LENC)
        {
            if ((dm_bond_dev_p = dm_ble_find_bond_info_by_nominal_addr(pm->key_req.peer_addr, 0)) != NULL )
            {
                char tmp_log[128] = {0};
                int32_t index = 0;

                for (int i = 0; i < sizeof(dm_bond_dev_p->bond_key.lenc_key.ltk); ++i)
                {
                    index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", dm_bond_dev_p->bond_key.lenc_key.ltk[i]);
                }

                gatt_logi("pair key reply accept, lenc_key %s", tmp_log);

                os_memset(tmp_log, 0, sizeof(tmp_log));
                index = 0;

                for (int i = 0; i < sizeof(dm_bond_dev_p->bond_key.lenc_key.rand); ++i)
                {
                    index += snprintf(tmp_log + index, sizeof(tmp_log) - 1 - index, "%02x", dm_bond_dev_p->bond_key.lenc_key.rand[i]);
                }

                gatt_logi("div 0x%04x rand %s", dm_bond_dev_p->bond_key.lenc_key.div, tmp_log);

                bk_ble_pair_key_reply(pm->key_req.peer_addr, true, pm->key_req.key_type, &dm_bond_dev_p->bond_key.lenc_key, sizeof(dm_bond_dev_p->bond_key.lenc_key));
            }
            else
            {
                gatt_logi("pair key reply negative");
                bk_ble_pair_key_reply(pm->key_req.peer_addr, false, pm->key_req.key_type, NULL, 0);
            }
        }
    }
    break;

    default:
        return DM_BLE_GAP_APP_CB_RET_NO_INTERESTING;
        break;
    }

    return DM_BLE_GAP_APP_CB_RET_PROCESSED;
}


int dm_gatt_add_gap_callback(void *param)
{
    dm_ble_gap_app_cb cb = (typeof(cb))param;

    if (!cb)
    {
        return -1;
    }

    for (int i = 0; i < sizeof(s_gap_cb_list) / sizeof(s_gap_cb_list[0]); ++i)
    {
        if (!s_gap_cb_list[i])
        {
            s_gap_cb_list[i] = cb;
            return 0;
        }
    }

    return -1;
}

int dm_gatt_passkey_reply(uint8_t accept, uint32_t passkey)
{
    gatt_logi("accept %d %06d", accept, passkey);
    return bk_ble_passkey_reply(s_peer_bdaddr, accept, passkey);
}

static int32_t dm_gatt_set_security_method_private(void)
{
    int32_t ret = 0;
    gatt_logi("iocap 0x%x authen mode 0x%x key_dist 0x%x", s_dm_gatt_iocap, s_dm_gatt_auth_req, s_dm_gatt_key_distr);

    //    if(!s_ble_sema)
    //    {
    //        ret = rtos_init_semaphore(&s_ble_sema, 1);
    //
    //        if (ret != 0)
    //        {
    //            gatt_loge("rtos_init_semaphore err %d", ret);
    //            return -1;
    //        }
    //    }

    //todo: add completed evt
    ret = bk_ble_gap_set_security_param(BK_BLE_SM_IOCAP_MODE, (void *)&s_dm_gatt_iocap, sizeof(s_dm_gatt_iocap));

    if (ret)
    {
        gatt_loge("set security param BK_BLE_SM_IOCAP_MODE err %d", ret);
        goto error;
    }

    //    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);
    //
    //    if (ret != kNoErr)
    //    {
    //        gatt_loge("wait set security param 1 err %d", ret);
    //        goto error;
    //    }

    ret = bk_ble_gap_set_security_param(BK_BLE_SM_AUTHEN_REQ_MODE, (void *)&s_dm_gatt_auth_req, sizeof(s_dm_gatt_auth_req));

    if (ret)
    {
        gatt_loge("set security param BK_BLE_SM_AUTHEN_REQ_MODE err %d", ret);
        goto error;
    }

    //    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);
    //
    //    if (ret != kNoErr)
    //    {
    //        gatt_loge("wait set security param 2 err %d", ret);
    //        goto error;
    //    }

    ret = bk_ble_gap_set_security_param(BK_BLE_SM_SET_INIT_KEY, (void *)&s_dm_gatt_key_distr, sizeof(s_dm_gatt_key_distr));

    if (ret)
    {
        gatt_loge("set security param BK_BLE_SM_SET_INIT_KEY err %d", ret);
        goto error;
    }

    //    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);
    //
    //    if (ret != kNoErr)
    //    {
    //        gatt_loge("wait set security param 2 err %d", ret);
    //        goto error;
    //    }

    ret = bk_ble_gap_set_security_param(BK_BLE_SM_SET_RSP_KEY, (void *)&s_dm_gatt_key_distr, sizeof(s_dm_gatt_key_distr));

    if (ret)
    {
        gatt_loge("set security param BK_BLE_SM_SET_RSP_KEY err %d", ret);
        goto error;
    }

    //    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);
    //
    //    if (ret != kNoErr)
    //    {
    //        gatt_loge("wait set security param 2 err %d", ret);
    //        goto error;
    //    }


    rtos_delay_milliseconds(100);

error:;
    //    ret = rtos_deinit_semaphore(&s_ble_sema);
    //
    //    if (ret != 0)
    //    {
    //        gatt_loge("rtos_deinit_semaphore err %d", ret);
    //        return -1;
    //    }
    //
    //    s_ble_sema = NULL;

    return ret;
}

int dm_gatt_set_security_method(uint8_t iocap, uint8_t auth_req, uint8_t key_distr)
{
    s_dm_gatt_iocap = iocap;
    s_dm_gatt_auth_req = auth_req;
    s_dm_gatt_key_distr = key_distr;
    return dm_gatt_set_security_method_private();
}

bool dm_gatt_is_linkkey_distr_from_ltk(void)
{
#if CONFIG_BT
    return (s_dm_gatt_key_distr & BK_BLE_KEY_DISTR_LINK_KEY_MASK) ? true : false;
#endif
    return 0;
}

int32_t dm_gatt_get_authen_status(uint8_t *nominal_addr, uint8_t *nominal_addr_type, uint8_t *identity_addr, uint8_t *identity_addr_type)
{
    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    const uint8_t ff_addr[BK_BD_ADDR_LEN] = {[0 ... (BK_BD_ADDR_LEN - 1)] = 0xff};
    uint32_t i = 0;

    for (i = 0; i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]); ++i)
    {
        if (os_memcmp(null_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) && os_memcmp(ff_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) )
        {
            break;
        }
    }

    if (i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]))
    {
        gatt_logi("find storage device 0x%x, nominal addr 0x%x %02x:%02x:%02x:%02x:%02x:%02x, identity addr 0x%x %02x:%02x:%02x:%02x:%02x:%02x",
                  s_dm_gatt_bond_dev_list[i].bond_key.key_mask,
                  s_dm_gatt_bond_dev_list[i].addr_type,
                  s_dm_gatt_bond_dev_list[i].bd_addr[5],
                  s_dm_gatt_bond_dev_list[i].bd_addr[4],
                  s_dm_gatt_bond_dev_list[i].bd_addr[3],
                  s_dm_gatt_bond_dev_list[i].bd_addr[2],
                  s_dm_gatt_bond_dev_list[i].bd_addr[1],
                  s_dm_gatt_bond_dev_list[i].bd_addr[0],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.addr_type,
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[5],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[4],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[3],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[2],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[1],
                  s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr[0]);

        os_memcpy(nominal_addr, s_dm_gatt_bond_dev_list[i].bd_addr, sizeof(s_dm_gatt_bond_dev_list[i].bd_addr));
        *nominal_addr_type = s_dm_gatt_bond_dev_list[i].addr_type;

        if (s_dm_gatt_bond_dev_list[i].bond_key.key_mask & BK_LE_KEY_PID)
        {
            os_memcpy(identity_addr, s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr, sizeof(s_dm_gatt_bond_dev_list[i].bond_key.pid_key.static_addr));
            *identity_addr_type = s_dm_gatt_bond_dev_list[i].bond_key.pid_key.addr_type;
        }

        return 0;
    }

    return -1;
}

int dm_ble_gap_create_bond(uint8_t *addr)
{
    return bk_ble_gap_create_bond(addr);
}

int dm_ble_gap_remove_bond(uint8_t *addr)
{
    int32_t ret = 0;
    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    const uint8_t ff_addr[BK_BD_ADDR_LEN] = {[0 ... (BK_BD_ADDR_LEN - 1)] = 0xff};
    bk_ble_bond_dev_t *tmp_dev = NULL;

    if (!os_memcmp(null_addr, addr, BK_BD_ADDR_LEN) ||
            !os_memcmp(ff_addr, addr, BK_BD_ADDR_LEN) )
    {
        gatt_loge("addr invalid");

        return -1;
    }

    if ((tmp_dev = dm_ble_find_bond_info_by_nominal_addr(addr, 0)) == NULL)
    {
        gatt_loge("addr %02x:%02x:%02x:%02x:%02x:%02x bond info not found",
                  addr[5],
                  addr[4],
                  addr[3],
                  addr[2],
                  addr[1],
                  addr[0]);
        return -1;
    }

    if (!s_ble_sema)
    {
        ret = rtos_init_semaphore(&s_ble_sema, 1);

        if (ret != 0)
        {
            gatt_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    bk_ble_bond_dev_t bond_dev;

    os_memset(&bond_dev, 0, sizeof(bond_dev));

    os_memcpy(&bond_dev, tmp_dev, sizeof(*tmp_dev));

    ret = bk_ble_gap_bond_dev_list_operation(BK_GAP_BOND_DEV_LIST_OPERATION_REMOVE, &bond_dev);

    if (ret)
    {
        gatt_loge("remove bond dev list op err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait bond dev list op err %d", ret);
        return -1;
    }

    if (dm_ble_del_bond_info_by_nominal_addr(addr))
    {
        gatt_loge("addr %02x:%02x:%02x:%02x:%02x:%02x bond info del err",
                  addr[5],
                  addr[4],
                  addr[3],
                  addr[2],
                  addr[1],
                  addr[0]);
    }

    ret = rtos_deinit_semaphore(&s_ble_sema);

    if (ret != 0)
    {
        gatt_loge("rtos_deinit_semaphore err %d", ret);
        return -1;
    }

    s_ble_sema = NULL;

#if BLE_USE_STORAGE
    bluetooth_storage_save_ble_key_info(s_dm_gatt_bond_dev_list, sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]));
    bluetooth_storage_sync_to_flash();
#endif

    return ret;
}

int32_t dm_ble_gap_clean_bond(void)
{
    int32_t ret = 0;

    if (!s_dm_gatt_is_inited)
    {
        gatt_loge("dm_gatt not init");
        return -1;
    }

    if (!s_ble_sema)
    {
        ret = rtos_init_semaphore(&s_ble_sema, 1);

        if (ret != 0)
        {
            gatt_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    ret = bk_ble_gap_bond_dev_list_operation(BK_GAP_BOND_DEV_LIST_OPERATION_CLEAN, NULL);

    if (ret)
    {
        gatt_loge("remove bond dev list op err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait bond dev list op err %d", ret);
        return -1;
    }

    dm_ble_clean_bond_info();

    ret = rtos_deinit_semaphore(&s_ble_sema);

    if (ret != 0)
    {
        gatt_loge("rtos_deinit_semaphore err %d", ret);
        return -1;
    }

    s_ble_sema = NULL;

#if BLE_USE_STORAGE
    bluetooth_storage_clean_ble_key_info();
    bluetooth_storage_sync_to_flash();
#endif

    return ret;
}

int dm_ble_gap_update_param(uint8_t *addr, uint16_t interval, uint16_t tout)
{
    int32_t ret = 0;
    bk_ble_conn_update_params_t param;

    if (!s_ble_sema)
    {
        ret = rtos_init_semaphore(&s_ble_sema, 1);

        if (ret != 0)
        {
            gatt_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    os_memset(&param, 0, sizeof(param));

    os_memcpy(param.bda, addr, sizeof(param.bda));
    param.latency = 0;
    param.timeout = tout;
    param.min_int = param.max_int = interval;

    bk_ble_gap_update_conn_params(&param);

    if (ret)
    {
        gatt_loge("remove bond dev list op err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait bond dev list op err %d", ret);
        return -1;
    }

    ret = rtos_deinit_semaphore(&s_ble_sema);

    if (ret != 0)
    {
        gatt_loge("rtos_deinit_semaphore err %d", ret);
        return -1;
    }

    return 0;
}

int dm_gatt_main(void)
{
    ble_err_t ret = 0;

    if (s_dm_gatt_is_inited)
    {
        gatt_loge("already init");
        return -1;
    }

#if BLE_USE_STORAGE
    uint32_t list_count = GATT_MAX_BOND_COUNT;

    bluetooth_storage_init();
    bluetooth_storage_read_ble_key_info(s_dm_gatt_bond_dev_list, &list_count);
#endif

    ret = rtos_init_semaphore(&s_ble_sema, 1);

    if (ret != 0)
    {
        gatt_loge("rtos_init_semaphore err %d", ret);
        return -1;
    }

    bk_ble_gap_register_callback(dm_ble_gap_private_cb);
    dm_gatt_add_gap_callback(dm_ble_gap_common_cb);

    bk_ble_gatt_set_local_mtu(517);

    //    bk_ble_gatt_set_local_mtu(100);
    //    bk_ble_gatt_set_local_mtu(40);

    const uint8_t null_addr[BK_BD_ADDR_LEN] = {0};
    const uint8_t ff_addr[BK_BD_ADDR_LEN] = {[0 ... (BK_BD_ADDR_LEN - 1)] = 0xff};

    for (int i = 0; i < sizeof(s_dm_gatt_bond_dev_list) / sizeof(s_dm_gatt_bond_dev_list[0]); ++i)
    {
        if (os_memcmp(null_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) && os_memcmp(ff_addr, s_dm_gatt_bond_dev_list[i].bd_addr, BK_BD_ADDR_LEN) )
        {
            bk_ble_bond_dev_t bond_dev;

            os_memset(&bond_dev, 0, sizeof(bond_dev));

            os_memcpy(&bond_dev, s_dm_gatt_bond_dev_list + i, sizeof(bond_dev));

            ret = bk_ble_gap_bond_dev_list_operation(BK_GAP_BOND_DEV_LIST_OPERATION_ADD, &bond_dev);

            if (ret)
            {
                gatt_loge("add bond dev list op err %d", ret);
                return -1;
            }

            ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

            if (ret != kNoErr)
            {
                gatt_loge("wait bond dev list op err %d", ret);
                return -1;
            }
        }
    }

    dm_gatt_set_security_method_private();

    ret = rtos_deinit_semaphore(&s_ble_sema);

    if (ret != 0)
    {
        gatt_loge("rtos_deinit_semaphore err %d", ret);
        return -1;
    }

    s_ble_sema = NULL;

    s_dm_gatt_is_inited = 1;

    return 0;
}

