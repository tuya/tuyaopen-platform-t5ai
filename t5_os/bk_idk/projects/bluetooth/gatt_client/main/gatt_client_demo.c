#include <stdio.h>
#include "bk_private/bk_init.h"
#include <common/bk_typedef.h>
#include <components/system.h>
//#include "common.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "gatt_client_demo.h"
#include "cli.h"

#if CONFIG_BLE
#include "ble_api_5_x.h"
#include "bluetooth_legacy_include.h"

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define BLE_GATT_CLIENT_TAG "BLE-GATTC"

#define BLEGATTC_LOGI(...) BK_LOGI(BLE_GATT_CLIENT_TAG, ##__VA_ARGS__)
#define BLEGATTC_LOGW(...) BK_LOGW(BLE_GATT_CLIENT_TAG, ##__VA_ARGS__)
#define BLEGATTC_LOGE(...) BK_LOGE(BLE_GATT_CLIENT_TAG, ##__VA_ARGS__)
#define BLEGATTC_LOGD(...) BK_LOGD(BLE_GATT_CLIENT_TAG, ##__VA_ARGS__)

#define BLE_GATT_CMD_CNT sizeof(s_gatt_commands)/sizeof(struct cli_command)

#define CMD_RSP_SUCCEED               "BLE GATTC RSP:OK\r\n"
#define CMD_RSP_ERROR                 "BLE GATTC RSP:ERROR\r\n"

#define BLE_SYNC_CMD_TIMEOUT_MS          4000
#define BLE_DISCON_CMD_TIMEOUT_MS        6000

#define INVALID_HANDLE          0xFF

#define GATTC_CONN_PARA_INTERVAL 0x09
#define GATTC_CONN_PARA_LATENCY  0
#define GATTC_CONN_PARA_SUPERVISION_TIMEOUT 0x1f4
#define GATTC_SCAN_PARAM_INTERVAL 0x64
#define GATTC_SCAN_PARAM_WINDOW   0X64


#define UNKNOW_ACT_IDX         0xFFU

#define BLE_MAX_ACTV                  bk_ble_get_max_actv_idx_count()
#define BLE_MAX_CONN                  bk_ble_get_max_conn_idx_count()


#define GATTC_MTU 255

static beken_semaphore_t gatt_sema = NULL;
static uint8_t gatt_conn_ind = INVALID_HANDLE;
static ble_read_phy_t gatt_ble_phy = {0, 0};
static ble_err_t gatt_cmd_status = BK_ERR_BT_SUCCESS;
static uint8_t s_ble_disconnecting = 0;

static void ble_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param);


static int ble_convert_128b_2_16b_uuid(uint8_t *uuid128, uint16_t *uuid16)
{
    if(uuid128  == NULL || uuid16 == NULL)
    {
        return 1;
    }

    uint8_t base_uuid[16] = {0};

    base_uuid[0U] = 0xFBU;
    base_uuid[1U] = 0x34U;
    base_uuid[2U] = 0x9BU;
    base_uuid[3U] = 0x5FU;
    base_uuid[4U] = 0x80U;
    base_uuid[5U] = 0x00U;
    base_uuid[6U] = 0x00U;
    base_uuid[7U] = 0x80U;
    base_uuid[8U] = 0x00U;
    base_uuid[9U] = 0x10U;
    base_uuid[10U] = 0x00U;
    base_uuid[11U] = 0x00U;
    base_uuid[12U] = uuid128[12U];
    base_uuid[13U] = uuid128[13U];
    base_uuid[14U] = 0x00U;
    base_uuid[15U] = 0x00U;
    if(0 == os_memcmp(uuid128, base_uuid, 16))
    {
        *uuid16 = (uuid128[13U] << 8) | (uuid128[12U]);
        return 0;
    }
    return 1;
}

static void ble_convert_2_128b_uuid(uint8_t *uuid128, const uint8_t *uuid, uint8_t uuid_len)
{
    uint8_t auc_128UUIDBase[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t cursor = 0;

    if((uuid_len == 4) || (uuid_len == 2))
    {
        cursor = 12;
    }
    else
    {
        uuid_len  = 16;
    }

    os_memcpy(&(auc_128UUIDBase[cursor]), uuid, uuid_len);
    os_memcpy(&uuid128[0], &auc_128UUIDBase[0], 16);
}



static int get_addr_from_param(bd_addr_t *bdaddr, char *input_param)
{
    int err = kNoErr;
    uint8_t addr_len = os_strlen(input_param);
    char temp [ 3 ];
    uint8_t j = 1;
    uint8_t k = 0;

    if (addr_len != (BK_BD_ADDR_LEN * 2 + 5))
    {
        err = kParamErr;
        return err;
    }

    for (uint8_t i = 0; i < addr_len; i++)
    {
        if (input_param [ i ] >= '0' && input_param [ i ] <= '9')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] >= 'a' && input_param [ i ] <= 'f')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] >= 'A' && input_param [ i ] <= 'F')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] == ':')
        {
            temp [ k ] = '\0';
            bdaddr->addr [ BK_BD_ADDR_LEN - j ] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
            j++;
        }
        else
        {
            err = kParamErr;
            return err;
        }

        if (i == (addr_len - 1))
        {
            temp [ k ] = '\0';
            bdaddr->addr [ BK_BD_ADDR_LEN - j ] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
        }
    }

    return kNoErr;
}


static void gatt_client_help()
{
    BLEGATTC_LOGI("\r\n");
    BLEGATTC_LOGI("ble_gattc help\n");
    BLEGATTC_LOGI("ble_gattc scan [1|0]\n");
    BLEGATTC_LOGI("ble_gattc conn [addr]\n");
    BLEGATTC_LOGI("ble_gattc disconn \n");
    BLEGATTC_LOGI("ble_gattc notifyindcate_en [enable|disable] [desc_handle]\n");
    BLEGATTC_LOGI("ble_gattc write [val_handle] [data]\n");
    BLEGATTC_LOGI("ble_gattc read [val_handle] \n");
    BLEGATTC_LOGI("\r\n");
}

static int32_t strallcmp(char *str1, char *str2)
{
    if(os_strlen(str1) == os_strlen(str2) && os_strcmp(str1, str2) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void gatt_client_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *cmd_rsp = CMD_RSP_SUCCEED;
    int ret = BK_FAIL;
    int actv_idx = 0;
    if (strcmp(argv [ 1 ], "help") == 0)
    {
        gatt_client_help();
    }else if(strcmp(argv [ 1 ], "scan") == 0)
    {
        uint8_t en = os_strtoul(argv[2], NULL, 10);
        if(en)
        {
            ret = bk_ble_start_scaning_ex(actv_idx, 0, 0, 10, ble_cmd_cb);
        }else
        {
            ret = bk_ble_stop_scaning(actv_idx, ble_cmd_cb);
        }
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble set scan enable fail \n");
            goto error;
        }
        if(gatt_sema != NULL)
        {
            ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
            if(ret != BK_OK)
            {
                BLEGATTC_LOGE("ble set scan enable timeout \n");
                goto error;
            }
        }
    }else if(strcmp(argv [ 1 ], "conn") == 0)
    {
        bd_addr_t addr;
        uint8_t addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
        uint8_t actv_idx = 0;
        ble_conn_param_t conn_param;
        os_memset(&conn_param, 0, sizeof(ble_conn_param_t));
        conn_param.intv_max = conn_param.intv_min = GATTC_CONN_PARA_INTERVAL;
        conn_param.con_latency = GATTC_CONN_PARA_LATENCY;
        conn_param.sup_to = GATTC_CONN_PARA_SUPERVISION_TIMEOUT;
        conn_param.init_phys = INIT_PHY_TYPE_LE_1M | INIT_PHY_TYPE_LE_2M;
        if(!get_addr_from_param(&addr, argv[2]))
        {
            if(argc >= 4)
            {
                addr_type = os_strtoul(argv[3], NULL, 10);
            }

            actv_idx = bk_ble_get_idle_conn_idx_handle();

            if (actv_idx == UNKNOW_ACT_IDX)
            {
                BLEGATTC_LOGE("ble conn fail, no resource \n");
                goto error;
            }
            ret = bk_ble_create_init(actv_idx, &conn_param, ble_cmd_cb);
            if (ret != BK_OK)
            {
                BLEGATTC_LOGE("ble create init fail \n");
                goto error;
            }
            if(gatt_sema == NULL)
            {
                BLEGATTC_LOGE("ble conn fail, gatt_sema need init \n");
                goto error;
            }
            ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
            if(ret != BK_OK)
            {
                BLEGATTC_LOGE("ble create init timeout \n");
                goto error;
            }
            if(gatt_cmd_status != BK_OK)
            {
                BLEGATTC_LOGE("ble create init fail, ret_sta:%d \n", gatt_cmd_status);
                goto error;
            }
            ret = bk_ble_init_set_connect_dev_addr(actv_idx, &addr, addr_type);
            if (ret != BK_OK)
            {
                BLEGATTC_LOGE("ble set init fail \n");
                goto error;
            }
            ret = bk_ble_init_start_conn(actv_idx, ble_cmd_cb);
            if (ret != BK_OK)
            {
                BLEGATTC_LOGE("ble start init fail \n");
                goto error;
            }
            ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
            if(ret != BK_OK)
            {
                BLEGATTC_LOGE("ble start init timeout \n");
                goto error;
            }
        }else
        {
            BLEGATTC_LOGE("param addr error \r\n");
            goto error;
        }
    }
    else if(strcmp(argv [ 1 ], "disconn") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }
        ret = bk_ble_disconnect(gatt_conn_ind);
        s_ble_disconnecting = 1;
        if (ret != BK_OK)
        {
            BLEGATTC_LOGE("ble disconn fail :%d\n", ret);
            goto error;
        }
        if(gatt_sema == NULL)
        {
            BLEGATTC_LOGE("ble disconn fail, gatt_sema need init \n");
            goto error;
        }
        ret = rtos_get_semaphore(&gatt_sema, BLE_DISCON_CMD_TIMEOUT_MS);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble disconn timeout \n");
            goto error;
        }
    }
    else if(strcmp(argv [ 1 ], "notifyindcate_en") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }
        uint16_t ccc_value = os_strtoul(argv[2], NULL, 10);
        uint16_t char_handle = 0xFF;
        char_handle = os_strtoul(argv[3], NULL, 16);

        ret = bk_ble_gatt_write_ccc(gatt_conn_ind, char_handle, ccc_value);

        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble notify|indcate en fail :%d\n", ret);
            goto error;
        }
    }
    else if(strallcmp(argv [ 1 ], "read") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }
        uint16_t char_handle = 0xFF;
        char_handle = os_strtoul(argv[2], NULL, 16);
        ret = bk_ble_att_read(gatt_conn_ind, char_handle);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble read att :%d\n", ret);
            goto error;
        }
    }
    else if(strallcmp(argv [ 1 ], "write") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }
        uint16_t char_handle = 0xFF;
        char_handle = os_strtoul(argv[2], NULL, 16);
        char *data = argv[3];
        uint8_t len = os_strlen(data);
        ret = bk_ble_gatt_write_value(gatt_conn_ind, char_handle, len, (uint8_t *)data);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble read att :%d\n", ret);
            goto error;
        }
    }
    else if(strcmp(argv [ 1 ], "discover_service") == 0)
    {
        uint16_t uuid = 0;//0x1801;//BK_GATT_UUID_GAP_SVC;
        uint8_t att_req_uuid_len = 2;
        uint16_t sh = 1;
        uint16_t eh = 0xffff;

        if(argc >= 3)
        {
            sh = os_strtoul(argv[2], NULL, 16);
        }

        if(argc >= 4)
        {
            eh = os_strtoul(argv[3], NULL, 16);
        }

        if(argc >= 5)
        {
            uuid = os_strtoul(argv[4], NULL, 16);
        }

        if(argc >= 6)
        {
            att_req_uuid_len = os_strtoul(argv[5], NULL, 16);
        }

        if(!uuid)
        {
            ret = bk_ble_discover_primary_service(gatt_conn_ind, sh, eh);
        }
        else if(att_req_uuid_len == 2)
        {
            ret = bk_ble_discover_primary_service_by_uuid(gatt_conn_ind, sh, eh, uuid);
        }
        else if(att_req_uuid_len == 16)
        {
            uint8_t uuid_128[16] = {0};
            ble_convert_2_128b_uuid(uuid_128, (uint8_t *)&uuid, sizeof(uuid));
            ret = bk_ble_discover_primary_service_by_128uuid(gatt_conn_ind, sh, eh, uuid_128);
        }
        else
        {
            ret = -1;
        }
    }
    else if(strcmp(argv [ 1 ], "discover_char") == 0)
    {
        uint16_t uuid = 0;//0x2803;//BK_GATT_UUID_CHAR_DECLARE;
        uint8_t att_req_uuid_len = 2;
        uint16_t sh = 1;
        uint16_t eh = 0xffff;

        if(argc >= 3)
        {
            sh = os_strtoul(argv[2], NULL, 16);
        }

        if(argc >= 4)
        {
            eh = os_strtoul(argv[3], NULL, 16);
        }

        if(argc >= 5)
        {
            uuid = os_strtoul(argv[4], NULL, 16);
        }

        if(argc >= 6)
        {
            att_req_uuid_len = os_strtoul(argv[5], NULL, 16);
        }

        if(!uuid)
        {
            ret = bk_ble_discover_characteristic(gatt_conn_ind, sh, eh);
        }
        else if(att_req_uuid_len == 2)
        {
            ret = bk_ble_discover_characteristic_by_uuid(gatt_conn_ind, sh, eh, uuid);
        }
        else if(att_req_uuid_len == 16)
        {
            uint8_t uuid_128[16] = {0};
            ble_convert_2_128b_uuid(uuid_128, (uint8_t *)&uuid, sizeof(uuid));
            ret = bk_ble_discover_characteristic_by_128uuid(gatt_conn_ind, sh, eh, uuid_128);
        }
        else
        {
            ret = -1;
        }
    }
    else if(strcmp(argv [ 1 ], "discover_desc") == 0)
    {
        uint16_t sh = 1;
        uint16_t eh = 0xffff;

        if(argc >= 3)
        {
            sh = os_strtoul(argv[2], NULL, 16);
        }

        if(argc >= 4)
        {
            eh = os_strtoul(argv[3], NULL, 16);
        }

        ret = bk_ble_discover_characteristic_descriptor(gatt_conn_ind, sh, eh);
    }
    else if(strcmp(argv [ 1 ], "bond") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("please connect first \n");
            goto error;
        }
        bk_ble_create_bond(gatt_conn_ind, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
                                GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);;
    }
    else if(strallcmp(argv [ 1 ], "read_ext") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }
        uint16_t attr_handle = 1;
        uint16_t offset = 0;

        attr_handle = os_strtoul(argv[2], NULL, 16);

        if(argc > 3)
        {
            offset = os_strtoul(argv[3], NULL, 10);
        }

        ret = bk_ble_gattc_read(gatt_conn_ind, attr_handle, offset);

        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble read att :%d\n", ret);
            goto error;
        }
    }
    else if(strallcmp(argv [ 1 ], "read_by_uuid") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }

        uint16_t uuid = 0;
        uint8_t uuid_len = 2;

        uint16_t sh = 1;
        uint16_t eh = 0xffff;

        if(argc > 2)
        {
            sh = os_strtoul(argv[2], NULL, 16);

            if(argc > 3)
            {
                eh = os_strtoul(argv[3], NULL, 16);
            }

            if(argc > 4)
            {
                uuid = os_strtoul(argv[4], NULL, 16);
            }
        }

        ret = bk_ble_gattc_read_by_uuid(gatt_conn_ind, sh, eh, (uint8_t *)&uuid, uuid_len);

        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble read att :%d\n", ret);
            goto error;
        }
    }
    else if(strallcmp(argv [ 1 ], "write_ext") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTC_LOGE("plese conn first \n");
            goto error;
        }

        uint16_t attr_handle = 1;
        uint8_t len = 4;
        uint8_t *tmp_buff = NULL;
        uint8_t is_write_cmd = 0;

        if(argc > 2)
        {
            attr_handle = os_strtoul(argv[2], NULL, 16);
        }

        if(argc > 3)
        {
            len = os_strtoul(argv[3], NULL, 10);
        }

        if(argc > 4)
        {
            is_write_cmd = os_strtoul(argv[4], NULL, 10);
        }

        tmp_buff = os_malloc(len);

        if(!tmp_buff)
        {
            BLEGATTC_LOGE("alloc err\n");
            goto error;
        }

        for (uint32_t i = 0; i < len; ++i)
        {
            tmp_buff[i] = i;
        }

        ret = bk_ble_gattc_write(gatt_conn_ind, attr_handle, tmp_buff, len, is_write_cmd);

        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble write att :%d\n", ret);
            os_free(tmp_buff);
            goto error;
        }

        os_free(tmp_buff);
    }
    else if(strcmp(argv [ 1 ], "add_whl") == 0)
    {
        bd_addr_t addr;
        uint8_t addr_type = 0;
        if(!get_addr_from_param(&addr, argv[2]))
        {
            if(argc >= 4)
            {
                addr_type = os_strtoul(argv[3], NULL, 10);
            }
            //0 public, 1 random
            BLEGATTC_LOGI("add whl: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, addr_type:%d\n", 
                            addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3], addr.addr[4], addr.addr[5],
                            addr_type);
            ret = bk_ble_add_devices_to_while_list(&addr, addr_type);
        }else
        {
            BLEGATTC_LOGE("param addr error \r\n");
            goto error;
        }
    }else if(strcmp(argv [ 1 ], "rmv_whl") == 0)
    {
        bd_addr_t addr;
        uint8_t addr_type = 0;
        if(!get_addr_from_param(&addr, argv[2]))
        {
            if(argc >= 4)
            {
                addr_type = os_strtoul(argv[3], NULL, 10);
            }
            BLEGATTC_LOGI("add whl: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, addr_type:%d\n", 
                          addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3], addr.addr[4], addr.addr[5],
                          addr_type);
            ret = bk_ble_remove_devices_from_while_list(&addr, addr_type);
        }else
        {
            BLEGATTC_LOGE("param addr error \r\n");
            goto error;
        }
    }else if(strcmp(argv [ 1 ], "clear_whl") == 0)
    {
        ret = bk_ble_clear_white_list();
    }else if(strcmp(argv [ 1 ], "scan_filter") == 0)
    {
        bk_ble_delete_scaning(actv_idx, ble_cmd_cb);
        ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("%s ,delete scan timeout!\n", __func__);
            goto error;
        }
        uint8_t filter_policy = os_strtoul(argv[2], NULL, 10);

        ble_scan_param_t scan_param;
        os_memset(&scan_param, 0, sizeof(ble_scan_param_t));
        scan_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
        scan_param.scan_phy = INIT_PHY_TYPE_LE_1M;
        scan_param.scan_intv = GATTC_SCAN_PARAM_INTERVAL;
        scan_param.scan_wd = GATTC_SCAN_PARAM_WINDOW;
        scan_param.scan_type = PASSIVE_SCANNING;
        if(filter_policy == 0)
        {
            scan_param.scan_filter = BASIC_UNFILTER_SCAN_POLICY;
        }else
        {
            scan_param.scan_filter = BASIC_FILTER_SCAN_POLICY;
        }
        ret = bk_ble_create_scaning(actv_idx, &scan_param, ble_cmd_cb);

        if(ret != BK_OK){
            BLEGATTC_LOGE("%s ,create scan error!\n", __func__);
            goto error;
        }
        ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("%s ,create scan timeout!\n", __func__);
            goto error;
        }
    }
    else
    {
        goto error;
    }

    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
    s_ble_disconnecting = 0;
    return;
error:
    cmd_rsp = CMD_RSP_ERROR;
    s_ble_disconnecting = 0;
    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
}

static const struct cli_command s_gatt_commands[] =
{
#if CONFIG_BLUETOOTH
    {"ble_gattc", "ble_gattc arg1 arg2 ... argn",  gatt_client_command},
#endif
};

int gattc_demo_cli_init(void)
{
    return cli_register_commands(s_gatt_commands, BLE_GATT_CMD_CNT);
}

static void ble_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    BLEGATTC_LOGI("--------%s %d\n", __func__, cmd);
    gatt_cmd_status = param->status;
    switch (cmd)
    {
        case BLE_CREATE_ADV:
        case BLE_SET_ADV_DATA:
        case BLE_SET_RSP_DATA:
        case BLE_START_ADV:
        case BLE_STOP_ADV:
        case BLE_CREATE_SCAN:
        case BLE_START_SCAN:
        case BLE_STOP_SCAN:
        case BLE_INIT_CREATE:
        case BLE_INIT_START_CONN:
        case BLE_INIT_STOP_CONN:
        case BLE_CONN_DIS_CONN:
        case BLE_CONN_UPDATE_PARAM:
        case BLE_DELETE_ADV:
        case BLE_DELETE_SCAN:
        case BLE_CONN_READ_PHY:
        case BLE_CONN_SET_PHY:
        case BLE_CONN_UPDATE_MTU:
        case BLE_CREATE_PERIODIC:
        case BLE_START_PERIODIC:
        case BLE_STOP_PERIODIC:
        case BLE_DELETE_PERIODIC:
        case BLE_SET_LOCAL_NAME:
        case BLE_GET_LOCAL_NAME:
        case BLE_READ_LOCAL_ADDR:
        case BLE_SET_RANDOM_ADDR:
            if (gatt_sema != NULL)
                rtos_set_semaphore( &gatt_sema );
            break;
        default:
            break;
    }
}

static bk_ble_key_t s_ble_enc_key;
static void gattc_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice)
    {
    case BLE_5_WRITE_EVENT:
        break;
    case BLE_5_READ_EVENT:
        break;
    case BLE_5_REPORT_ADV:
    {
        ble_recv_adv_t *r_ind = (ble_recv_adv_t *)param;
        uint8_t adv_type = r_ind->evt_type & REPORT_INFO_REPORT_TYPE_MASK;
        //BLEGATTC_LOGI("r_ind:actv_idx:%d,", r_ind->actv_idx);
        char *type_log = NULL;
        switch (adv_type)
        {
            case REPORT_TYPE_ADV_EXT:
                type_log = "EXT_ADV,";
                break;
            case REPORT_TYPE_ADV_LEG:
                {
                    switch (r_ind->evt_type)
                    {
                        case (REPORT_INFO_SCAN_ADV_BIT | REPORT_INFO_CONN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_IND,";
                            break;
                        case (REPORT_INFO_DIR_ADV_BIT | REPORT_INFO_CONN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_DIRECT_IND,";
                            break;
                        case (REPORT_INFO_SCAN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_SCAN_IND,";
                            break;
                        case (REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_NONCONN_IND,";
                            break;
                        default:
                            type_log = "ERR_LEG_ADV,";
                            break;
                    }
                }
                break;
            case REPORT_TYPE_SCAN_RSP_LEG:
                type_log = "SCAN_RSP,";
                break;
            case REPORT_TYPE_SCAN_RSP_EXT:
                type_log = "AUX_SCAN_RSP,";
                break;
            case REPORT_TYPE_PER_ADV:
                type_log = "PER_ADV,";
                break;
            default:
                type_log = "ERR_ADV,";
                break;
        }
        BLEGATTC_LOGI("%s addr_type:%d, adv_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n", type_log,
                r_ind->adv_addr_type, r_ind->adv_addr[0], r_ind->adv_addr[1], r_ind->adv_addr[2],
                r_ind->adv_addr[3], r_ind->adv_addr[4], r_ind->adv_addr[5]);
    }
    break;
    case BLE_5_MTU_CHANGE:
    {
        ble_mtu_change_t *m_ind = (ble_mtu_change_t *)param;
        BLEGATTC_LOGI("%s m_ind:conn_idx:%d, mtu_size:%d\r\n", __func__, m_ind->conn_idx, m_ind->mtu_size);
        break;
    }
    case BLE_5_CONNECT_EVENT:
    {
            ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
            BLEGATTC_LOGI("c_ind:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                    c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
        break;
    }
    case BLE_5_DISCONNECT_EVENT:
    {
        ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
        BLEGATTC_LOGI("d_ind:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        gatt_conn_ind = INVALID_HANDLE;
        break;
    }
    case BLE_5_INIT_CONNECT_EVENT:
    {
        ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
        BLEGATTC_LOGI("BLE_5_INIT_CONNECT_EVENT:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
        gatt_conn_ind = c_ind->conn_idx;
        break;
    }
    case BLE_5_INIT_DISCONNECT_EVENT:
    {
        ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
        BLEGATTC_LOGI("BLE_5_INIT_DISCONNECT_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        break;
    }

    case BLE_5_INIT_CONNECT_FAILED_EVENT:
    {
        ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
        BLEGATTC_LOGI("BLE_5_INIT_CONNECT_FAILED_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        break;
    }

    case BLE_5_SDP_REGISTER_FAILED:
        BLEGATTC_LOGI("BLE_5_SDP_REGISTER_FAILED\r\n");
        break;
    case BLE_5_READ_PHY_EVENT:
    {
        gatt_ble_phy = *(ble_read_phy_t *)param;
        BLEGATTC_LOGI("BLE_5_READ_PHY_EVENT:tx_phy:0x%02x, rx_phy:0x%02x\r\n",gatt_ble_phy.tx_phy, gatt_ble_phy.rx_phy);
        break;
    }
    case BLE_5_TX_DONE:
    {
            BK_LOGI("ble_at","BLE_5_TX_DONE\r\n");
    }
        break;
    case BLE_5_CONN_UPDATA_EVENT:
    {
        ble_conn_param_t *updata_param = (ble_conn_param_t *)param;
        BLEGATTC_LOGI("BLE_5_CONN_UPDATA_EVENT:conn_interval:0x%04x, con_latency:0x%04x, sup_to:0x%04x\r\n", updata_param->intv_max,
        updata_param->con_latency, updata_param->sup_to);
        break;
    }

    case BLE_5_PERIODIC_SYNC_CMPL_EVENT:
        BLEGATTC_LOGI("BLE_5_PERIODIC_SYNC_CMPL_EVENT \n");
        break;

    case BLE_5_CONN_UPD_PAR_ASK:
    {
        ble_conn_update_para_ind_t *tmp = (typeof(tmp))param;
        BLEGATTC_LOGI("%s BLE_5_CONN_UPD_PAR_ASK accept\n", __func__);
        tmp->is_agree = 1;
    }
        break;

    case BLE_5_PAIRING_REQ:
    {
        BLEGATTC_LOGI("BLE_5_PAIRING_REQ\r\n");
        ble_smp_ind_t *s_ind = (ble_smp_ind_t *)param;
        bk_ble_sec_send_auth_mode(s_ind->conn_idx, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
            GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);
        break;
    }

    case BLE_5_PARING_PASSKEY_REQ:
        BLEGATTC_LOGI("BLE_5_PARING_PASSKEY_REQ\r\n");
        break;

    case BLE_5_ENCRYPT_EVENT:
        BLEGATTC_LOGI("BLE_5_ENCRYPT_EVENT\r\n");
        break;

    case BLE_5_PAIRING_SUCCEED:
        BLEGATTC_LOGI("BLE_5_PAIRING_SUCCEED\r\n");
        break;

    case BLE_5_PAIRING_FAILED:
        BLEGATTC_LOGI("BLE_5_PAIRING_FAILED\r\n");
        os_memset(&s_ble_enc_key, 0 ,sizeof(s_ble_enc_key));
        break;

    case BLE_5_GAP_CMD_CMP_EVENT:
    {
        ble_cmd_cmp_evt_t *event = (ble_cmd_cmp_evt_t *)param;

        switch(event->cmd) {
        case BLE_CONN_DIS_CONN:
            BLEGATTC_LOGI("BLE_CONN_DIS_CONN\r\n");
            if ((gatt_sema != NULL) && (s_ble_disconnecting))
            {
                rtos_set_semaphore( &gatt_sema );
                gatt_cmd_status = event->status;
            }
            break;
        case BLE_ADD_WHITE_LIST:
        case BLE_RMV_WHITE_LIST:
        case BLE_CLE_WHITE_LIST:
            BLEGATTC_LOGI("BLE_WHITE_LIST_DONE, status 0x%x\r\n", event->status);
            break;
        case BLE_CONN_UPDATE_PARAM:
        case BLE_CONN_SET_PHY:
        case BLE_CONN_READ_PHY:
        case BLE_CONN_UPDATE_MTU:
        case BLE_SET_MAX_MTU:
        {
            break;
        }
        case BLE_CONN_ENCRYPT:
        {
            BLEGATTC_LOGI("BLE_5_GAP_CMD_CMP_EVENT(BLE_CONN_ENCRYPT) , status %x\r\n",event->status);
            if (event->status)
            {
                os_memset(&s_ble_enc_key, 0 ,sizeof(s_ble_enc_key));
                bk_ble_disconnect(event->conn_idx);
            }
        }
        break;
        default:
            break;
        }

        break;
    }

    case BLE_5_KEY_EVENT:
    {
        BLEGATTC_LOGI("BLE_5_KEY_EVENT\r\n");
        s_ble_enc_key = *((bk_ble_key_t*)param);
        break;
    }

    case BLE_5_BOND_INFO_REQ_EVENT:
    {
        BLEGATTC_LOGI("BLE_5_BOND_INFO_REQ_EVENT\r\n");
        bk_ble_bond_info_req_t *bond_info_req = (bk_ble_bond_info_req_t*)param;
        if ((bond_info_req->key.peer_addr_type == s_ble_enc_key.peer_addr_type)
            && (!os_memcmp(bond_info_req->key.peer_addr, s_ble_enc_key.peer_addr, 6)))
        {
            bond_info_req->key_found = 1;
            bond_info_req->key = s_ble_enc_key;

        }
        break;
    }

    default:
        break;
    }
}

static void gattc_sdp_comm_callback(MASTER_COMMON_TYPE type,uint8 conidx,void *param)
{
//    BLEGATTC_LOGI("%s type:%d \n", __func__, type);
    uint16_t uuid = 0xFF;
    if(MST_TYPE_SVR_UUID == type)
    {
        struct ble_sdp_svc_ind *srv_ind = (struct ble_sdp_svc_ind*)param;
        if(srv_ind->uuid_len == 16)
        {
            if(ble_convert_128b_2_16b_uuid(srv_ind->uuid, &uuid) == 0)
            {
                BLEGATTC_LOGI("====>Get GATT Service UUID:0x%04X, start_handle:0x%02X eh 0x%x\n", uuid, srv_ind->start_hdl, srv_ind->end_hdl);
            }else
            {
                uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
                BLEGATTC_LOGI("Custom UUID\n");
                BLEGATTC_LOGI("===>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
            }
        }else if(srv_ind->uuid_len == 2)
        {
            uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
            BLEGATTC_LOGI("==>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
        }
    }else if (MST_TYPE_ATT_UUID == type)
    {
        struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf*)param;
        if(char_inf->uuid_len == 16)
        {
            if(ble_convert_128b_2_16b_uuid(char_inf->uuid, &uuid) == 0)
            {
                BLEGATTC_LOGI("====>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
            }else
            {
                uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
                BLEGATTC_LOGI("Custom UUID\n");
                BLEGATTC_LOGI("===>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
            }
        }else if(char_inf->uuid_len == 2)
        {
            uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
            BLEGATTC_LOGI("==>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
        }
    }else if(MST_TYPE_ATT_DESC == type)
    {
        struct ble_sdp_char_desc_inf *desc_inf = (struct ble_sdp_char_desc_inf*)param;
        if(desc_inf->uuid_len == 16)
        {
            if(ble_convert_128b_2_16b_uuid(desc_inf->uuid, &uuid) == 0)
            {
                BLEGATTC_LOGI("====>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
            }else
            {
                uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
                BLEGATTC_LOGI("Custom UUID\n");
                BLEGATTC_LOGI("===>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
            }
        }else if(desc_inf->uuid_len == 2)
        {
            uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
            BLEGATTC_LOGI("==>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0x%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
        }
    }else if (MST_TYPE_SDP_END == type)
    {
        BLEGATTC_LOGI("=============\r\n");
        bk_ble_gatt_mtu_change(conidx);
    }
    else if(type == MST_TYPE_UPP_ASK)
    {
        struct mst_comm_updata_para *tmp = (typeof(tmp))param;
        BLEGATTC_LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
        tmp->is_agree = 1;
    }

    switch(type)
    {
    case MST_TYPE_DISCOVER_PRI_SERVICE_RSP:
    case MST_TYPE_DISCOVER_PRI_SERVICE_BY_UUID_RSP:
    case MST_TYPE_DISCOVER_PRI_SERVICE_BY_128_UUID_RSP:
    {
        struct ble_sdp_svc_ind *srv_ind = (typeof(srv_ind))param;
        uint16_t uuid = 0;

        if(srv_ind->uuid_len == 16)
        {
            ble_convert_128b_2_16b_uuid(srv_ind->uuid, &uuid);
        }
        else
        {
            os_memcpy(&uuid, srv_ind->uuid, sizeof(uuid));
        }

        BLEGATTC_LOGI("service rsp uuid 0x%04x sh 0x%04x eh 0x%04x len %d\n", uuid, srv_ind->start_hdl, srv_ind->end_hdl, srv_ind->uuid_len);
    }
    break;

    case MST_TYPE_DISCOVER_CHAR_RSP:
    case MST_TYPE_DISCOVER_CHAR_BY_UUID_RSP:
    case MST_TYPE_DISCOVER_CHAR_BY_128_UUID_RSP:
    {
        struct ble_sdp_char_inf *char_inf = (typeof(char_inf))param;
        uint16_t uuid = 0;

        if(char_inf->uuid_len == 16)
        {
            ble_convert_128b_2_16b_uuid(char_inf->uuid, &uuid);
        }
        else
        {
            os_memcpy(&uuid, char_inf->uuid, sizeof(uuid));
        }

        BLEGATTC_LOGI("char rsp uuid 0x%04x decl handle 0x%04x value handle 0x%04x prop 0x%x len %d\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop, char_inf->uuid_len);
    }
    break;

    case MST_TYPE_DISCOVER_CHAR_DESC:
    {
        struct ble_sdp_char_desc_inf *desc_inf = (typeof(desc_inf))param;
        uint16_t uuid = 0;

        if(desc_inf->uuid_len == 16)
        {
            ble_convert_128b_2_16b_uuid(desc_inf->uuid, &uuid);
        }
        else
        {
            os_memcpy(&uuid, desc_inf->uuid, sizeof(uuid));
        }

        BLEGATTC_LOGI("char desc rsp uuid 0x%04x handle 0x%04x len %d\n", uuid, desc_inf->desc_hdl, desc_inf->uuid_len);
    }
    break;

    case MST_TYPE_DISCOVER_COMPLETED:
    {
        struct ble_descover_complete_inf *desc_inf = (typeof(desc_inf))param;
        BLEGATTC_LOGI("discover complete action 0x%x con_id 0x%x\n", desc_inf->action_id, conidx);
    }
    break;
    default:
        break;
    }
}

static void gattc_sdp_charac_callback(CHAR_TYPE type,uint8 conidx,uint16_t hdl,uint16_t len,uint8 *data)
{
//    BLEGATTC_LOGI("%s type:%d len:%d, data:%s\n",__func__, type, len, data);
    if (CHARAC_NOTIFY == type || CHARAC_INDICATE == type)
    {
        BLEGATTC_LOGI("CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x%02x, len:%d \n", hdl, len);
    }
    else if (CHARAC_WRITE_DONE == type)
    {
        BLEGATTC_LOGI("CHARAC_WRITE_DONE, handle:0x%02x, len:%d \n", hdl, len);
    }
    else if(CHARAC_READ == type || CHARAC_READ_DONE==type)
    {
        BLEGATTC_LOGI("CHARAC_READ|CHARAC_READ_DONE, handle:0x%02x, len:%d \n", hdl, len);
    }

    if(len)
    {
        BLEGATTC_LOGI("\n==================\n");
        char s[100] = {0};
        os_memcpy(s, data, len < sizeof(s) - 1 ? len: sizeof(s) - 1);
        BLEGATTC_LOGI("%s \n", s);
        BLEGATTC_LOGI("\n==================\n");

        os_memset(s, 0, sizeof(s));
        for (int i = 0; i < 5 && i < len; ++i)
        {
            sprintf(s + i * 2, "%02x", data[i]);
        }

        BLEGATTC_LOGI("0x%s\n", s);
        BLEGATTC_LOGI("\n==================\n");
    }
}

void gatt_client_demo_init()
{
    gattc_demo_cli_init();

    uint8_t actv_idx = 0;
    ble_err_t ret = BK_FAIL;

    ret = rtos_init_semaphore(&gatt_sema, 1);
    if(ret != BK_OK){
        BLEGATTC_LOGE("%s ,init sema error!\n", __func__);
        return;
    }

    bk_ble_set_notice_cb(gattc_notice_cb);
    bk_ble_register_app_sdp_common_callback(gattc_sdp_comm_callback);
    bk_ble_register_app_sdp_charac_callback(gattc_sdp_charac_callback);

    bk_ble_set_max_mtu(GATTC_MTU);

    ble_scan_param_t scan_param;
    os_memset(&scan_param, 0, sizeof(ble_scan_param_t));
    scan_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
    scan_param.scan_phy = INIT_PHY_TYPE_LE_1M;
    scan_param.scan_intv = GATTC_SCAN_PARAM_INTERVAL;
    scan_param.scan_wd = GATTC_SCAN_PARAM_WINDOW;
    scan_param.scan_type = PASSIVE_SCANNING;
    scan_param.scan_filter = BASIC_UNFILTER_SCAN_POLICY;
    ret = bk_ble_create_scaning(actv_idx, &scan_param, ble_cmd_cb);

    if(ret != BK_OK){
        BLEGATTC_LOGE("%s ,create scan error!\n", __func__);
        return;
    }
    ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("%s ,create scan timeout!\n", __func__);
        return;
    }
    BLEGATTC_LOGI("%s success \n", __func__);
}


#endif
