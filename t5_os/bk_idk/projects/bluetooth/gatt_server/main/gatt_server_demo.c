#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bk_private/bk_init.h"
#include <common/bk_typedef.h>
#include <components/system.h>
//#include "common.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "gatt_server_demo.h"
#include "cli.h"
#if CONFIG_BLE
#include "ble_api_5_x.h"
#include "bluetooth_legacy_include.h"

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define BLE_GATT_SERVER_TAG "BLE-GATTS"

#define BLEGATTS_LOGI(...) BK_LOGI(BLE_GATT_SERVER_TAG, ##__VA_ARGS__)
#define BLEGATTS_LOGW(...) BK_LOGW(BLE_GATT_SERVER_TAG, ##__VA_ARGS__)
#define BLEGATTS_LOGE(...) BK_LOGE(BLE_GATT_SERVER_TAG, ##__VA_ARGS__)
#define BLEGATTS_LOGD(...) BK_LOGD(BLE_GATT_SERVER_TAG, ##__VA_ARGS__)

#define ADV_MAX_SIZE (251)
#define ADV_NAME_HEAD "BK"

#define ADV_TYPE_FLAGS                      (0x01)
#define ADV_TYPE_LOCAL_NAME                 (0x09)
#define ADV_TYPE_SERVICE_UUIDS_16BIT        (0x14)
#define ADV_TYPE_SERVICE_DATA               (0x16)
#define ADV_TYPE_MANUFACTURER_SPECIFIC      (0xFF)

#define BEKEN_COMPANY_ID                    (0x05F0)

#define GATTS_UUID                       (0xFE01)


#define BLE_GATT_CMD_CNT sizeof(s_gatt_commands)/sizeof(struct cli_command)

#define CMD_RSP_SUCCEED               "BLE GATTS RSP:OK\r\n"
#define CMD_RSP_ERROR                 "BLE GATTS RSP:ERROR\r\n"

#define GATT_SYNC_CMD_TIMEOUT_MS          4000
#define INVALID_HANDLE          0xFF

#define GATTS_SERVICE_UUID 					(0xFA00)

#define GATTS_CHARA_PROPERTIES_UUID 			(0xEA01)
#define GATTS_CHARA_N1_UUID 			(0xEA02)

#define GATTS_CHARA_N2_UUID 				(0xEA05)
#define GATTS_CHARA_N3_UUID 			(0xEA06)

#define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DECL_CHARACTERISTIC_128      {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DESC_CLIENT_CHAR_CFG_128     {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

#define BLE_GATTS_ADV_INTERVAL_MIN 120
#define BLE_GATTS_ADV_INTERVAL_MAX 160


#define UNKNOW_ACT_IDX         0xFFU

#define BLE_MAX_ACTV                  bk_ble_get_max_actv_idx_count()
#define BLE_MAX_CONN                  bk_ble_get_max_conn_idx_count()

static beken_semaphore_t gatt_sema = NULL;
static ble_err_t gatt_cmd_status = BK_ERR_BLE_SUCCESS;
static uint8_t gatt_conn_ind = INVALID_HANDLE;

//N2
static char *s_v = 0;
static uint8_t s_len = 0;
//N3
static char *p_v = 0;
static uint8_t p_len = 0;
uint8_t notify_v[2];

static int gatts_demo_cli_init(void);

enum
{
	PRF_TASK_ID_GATTS = 10,
	PRF_TASK_ID_MAX,
};

enum {
    GATTS_IDX_SVC,
    GATTS_IDX_CHAR_DECL,
    GATTS_IDX_CHAR_VALUE,
	GATTS_IDX_CHAR_DESC,

	GATTS_IDX_CHAR_N1_DECL,
	GATTS_IDX_CHAR_N1_VALUE,

	GATTS_IDX_CHAR_N2_DECL,
	GATTS_IDX_CHAR_N2_VALUE,

	GATTS_IDX_CHAR_N3_DECL,
	GATTS_IDX_CHAR_N3_VALUE,

	GATTS_IDX_NB,
};

static ble_attm_desc_t gatts_service_db[GATTS_IDX_NB] = {
    //  Service Declaration
    [GATTS_IDX_SVC]        = {DECL_PRIMARY_SERVICE_128 , BK_BLE_PERM_SET(RD, ENABLE), 0, 0},

    [GATTS_IDX_CHAR_DECL]  = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    // Characteristic Value
    [GATTS_IDX_CHAR_VALUE] = {{GATTS_CHARA_PROPERTIES_UUID & 0xFF, GATTS_CHARA_PROPERTIES_UUID >> 8}, BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
	//Client Characteristic Configuration Descriptor
	[GATTS_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},

    //opreation
    [GATTS_IDX_CHAR_N1_DECL]  = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [GATTS_IDX_CHAR_N1_VALUE] = {{GATTS_CHARA_N1_UUID & 0xFF, GATTS_CHARA_N1_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

    //s_v
    [GATTS_IDX_CHAR_N2_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [GATTS_IDX_CHAR_N2_VALUE]   = {{GATTS_CHARA_N2_UUID & 0xFF, GATTS_CHARA_N2_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

    //p_v
    [GATTS_IDX_CHAR_N3_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [GATTS_IDX_CHAR_N3_VALUE]   = {{GATTS_CHARA_N3_UUID & 0xFF, GATTS_CHARA_N3_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
};

static void ble_gatt_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
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
        case BLE_SET_ADV_RANDOM_ADDR:
            if (gatt_sema != NULL)
                rtos_set_semaphore( &gatt_sema );
            break;
        default:
            break;
    }

}

static bk_ble_key_t s_ble_enc_key;
static void ble_gatts_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice) {
    case BLE_5_STACK_OK:
        BLEGATTS_LOGI("ble stack ok");
        break;
    case BLE_5_WRITE_EVENT: {
        ble_write_req_t *w_req = (ble_write_req_t *)param;
        BLEGATTS_LOGI("write_cb:conn_idx:%d, prf_id:%d, att_idx:%d, len:%d, data[0]:0x%02x\r\n",
                w_req->conn_idx, w_req->prf_id, w_req->att_idx, w_req->len, w_req->value[0]);
//#if (CONFIG_BTDM_5_2)

		if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2
            && w_req->prf_id == PRF_TASK_ID_GATTS) {
			switch(w_req->att_idx)
            {
            case GATTS_IDX_CHAR_DECL:
                break;
            case GATTS_IDX_CHAR_VALUE:
                break;
			case GATTS_IDX_CHAR_DESC: {
				BLEGATTS_LOGI("write notify: %02X %02X, length: %d\n", w_req->value[0], w_req->value[1], w_req->len);
				break;
			}

			case GATTS_IDX_CHAR_N1_DECL:
				break;

			case GATTS_IDX_CHAR_N1_VALUE:
				break;

            case GATTS_IDX_CHAR_N2_DECL:
                break;
            case GATTS_IDX_CHAR_N2_VALUE:
				{
					if (s_v != NULL)
					{
						os_free(s_v);
						s_v = NULL;
					}

					s_len = w_req->len;
					s_v = os_malloc(s_len + 1);


					os_memset((uint8_t *)s_v, 0, s_len + 1);
	                os_memcpy((uint8_t *)s_v, w_req->value, s_len);

					BLEGATTS_LOGI("write N2: %s, length: %d\n", s_v, s_len);
				}
                break;

            case GATTS_IDX_CHAR_N3_DECL:
                break;
            case GATTS_IDX_CHAR_N3_VALUE:
				{
					if (p_v != NULL)
					{
						os_free(p_v);
						p_v = NULL;
					}

					p_len = w_req->len;
					p_v = os_malloc(p_len + 1);

					os_memset((uint8_t *)p_v, 0, p_len + 1);
	                os_memcpy((uint8_t *)p_v, w_req->value, p_len);

					BLEGATTS_LOGI("write N3: %s, length: %d\n", p_v, p_len);
				}
				break;

            default:
                break;
            }
        }
//#endif
        break;
    }
    case BLE_5_READ_EVENT: {
        ble_read_req_t *r_req = (ble_read_req_t *)param;
        BLEGATTS_LOGI("read_cb:conn_idx:%d, prf_id:%d, att_idx:%d\r\n",
                r_req->conn_idx, r_req->prf_id, r_req->att_idx);

        if (r_req->prf_id == PRF_TASK_ID_GATTS) {
            switch(r_req->att_idx)
            {
                case GATTS_IDX_CHAR_DECL:
                    break;
                case GATTS_IDX_CHAR_VALUE:
                    break;
				case GATTS_IDX_CHAR_DESC:
					bk_ble_read_response_value(r_req->conn_idx, sizeof(notify_v), &notify_v[0], r_req->prf_id, r_req->att_idx);
					break;

                case GATTS_IDX_CHAR_N2_DECL:
                    break;
                case GATTS_IDX_CHAR_N2_VALUE:
                    bk_ble_read_response_value(r_req->conn_idx, s_len, (uint8_t*)s_v, r_req->prf_id, r_req->att_idx);
					BLEGATTS_LOGI("read N2: %s, length: %d\n", s_v, s_len);
                    break;

                case GATTS_IDX_CHAR_N3_DECL:
                    break;
                case GATTS_IDX_CHAR_N3_VALUE:
                    bk_ble_read_response_value(r_req->conn_idx, p_len, (uint8_t*)p_v, r_req->prf_id, r_req->att_idx);
					BLEGATTS_LOGI("read N3: %s, length: %d\n", p_v, p_len);
                    break;

                default:
                    break;
            }
        }
        break;
    }
    case BLE_5_REPORT_ADV: {
        break;
    }
    case BLE_5_MTU_CHANGE: {
        ble_mtu_change_t *m_ind = (ble_mtu_change_t *)param;
        BLEGATTS_LOGI("%s m_ind:conn_idx:%d, mtu_size:%d\r\n", __func__, m_ind->conn_idx, m_ind->mtu_size);
        break;
    }
    case BLE_5_CONNECT_EVENT: {
            ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
            BLEGATTS_LOGI("c_ind:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                    c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
            gatt_conn_ind = c_ind->conn_idx;
        break;
    }
    case BLE_5_DISCONNECT_EVENT: {
        ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
        BLEGATTS_LOGI("d_ind:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        gatt_conn_ind = ~0;
        break;
    }
    case BLE_5_ATT_INFO_REQ: {
        ble_att_info_req_t *a_ind = (ble_att_info_req_t *)param;
        BLEGATTS_LOGI("a_ind:conn_idx:%d\r\n", a_ind->conn_idx);
        a_ind->length = 128;
        a_ind->status = BK_ERR_BLE_SUCCESS;
        break;
    }
    case BLE_5_CREATE_DB: {
            ble_create_db_t *cd_ind = (ble_create_db_t *)param;

            BLEGATTS_LOGI("cd_ind:prf_id:%d, status:%d\r\n", cd_ind->prf_id, cd_ind->status);
            gatt_cmd_status = cd_ind->status;
            if (gatt_sema != NULL)
                rtos_set_semaphore( &gatt_sema );
        break;
    }
    case BLE_5_INIT_CONNECT_EVENT: {
        ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
        BLEGATTS_LOGI("BLE_5_INIT_CONNECT_EVENT:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
        break;
    }
    case BLE_5_INIT_DISCONNECT_EVENT: {
        ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
        BLEGATTS_LOGI("BLE_5_INIT_DISCONNECT_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
        break;
    }
    case BLE_5_SDP_REGISTER_FAILED:
        BLEGATTS_LOGI("BLE_5_SDP_REGISTER_FAILED\r\n");
        break;
    case BLE_5_READ_PHY_EVENT: {
        ble_read_phy_t *phy_param = (ble_read_phy_t *)param;
        BLEGATTS_LOGI("BLE_5_READ_PHY_EVENT:tx_phy:0x%02x, rx_phy:0x%02x\r\n",phy_param->tx_phy, phy_param->rx_phy);
        break;
    }
    case BLE_5_TX_DONE:
        break;
    case BLE_5_CONN_UPDATA_EVENT: {
        ble_conn_param_t *updata_param = (ble_conn_param_t *)param;
        BLEGATTS_LOGI("BLE_5_CONN_UPDATA_EVENT:conn_interval:0x%04x, con_latency:0x%04x, sup_to:0x%04x\r\n", updata_param->intv_max,
        updata_param->con_latency, updata_param->sup_to);
        break;
    }
    case BLE_5_PAIRING_REQ:
        bk_printf("BLE_5_PAIRING_REQ\r\n");
        ble_smp_ind_t *s_ind = (ble_smp_ind_t *)param;
        bk_ble_sec_send_auth_mode(s_ind->conn_idx, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
            GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);
        break;

    case BLE_5_PARING_PASSKEY_REQ:
        bk_printf("BLE_5_PARING_PASSKEY_REQ\r\n");
        break;

    case BLE_5_ENCRYPT_EVENT:
        bk_printf("BLE_5_ENCRYPT_EVENT\r\n");
        break;

    case BLE_5_PAIRING_SUCCEED:
        bk_printf("BLE_5_PAIRING_SUCCEED\r\n");
        break;

    case BLE_5_KEY_EVENT:
    {
        BLEGATTS_LOGI("BLE_5_KEY_EVENT\r\n");
        s_ble_enc_key = *((bk_ble_key_t*)param);
        break;
    }

    case BLE_5_BOND_INFO_REQ_EVENT:
    {
        BLEGATTS_LOGI("BLE_5_BOND_INFO_REQ_EVENT\r\n");
        bk_ble_bond_info_req_t *bond_info_req = (bk_ble_bond_info_req_t*)param;
        if ((bond_info_req->key.peer_addr_type == s_ble_enc_key.peer_addr_type)
            && (!os_memcmp(bond_info_req->key.peer_addr, s_ble_enc_key.peer_addr, 6)))
        {
            bond_info_req->key_found = 1;
            bond_info_req->key = s_ble_enc_key;
        }
        break;
    }

    case BLE_5_GAP_CMD_CMP_EVENT:
    {
        ble_cmd_cmp_evt_t *event = (ble_cmd_cmp_evt_t *)param;

        switch(event->cmd) {
        case BLE_CONN_ENCRYPT:
            {
                BLEGATTS_LOGI("BLE_5_GAP_CMD_CMP_EVENT(BLE_CONN_ENCRYPT) , status %x\r\n",event->status);
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

    case BLE_5_PAIRING_FAILED:
    {
        BLEGATTS_LOGI("BLE_5_PAIRING_FAILED\r\n");
        os_memset(&s_ble_enc_key, 0 ,sizeof(s_ble_enc_key));
        break;
    }
    default:
        break;
    }
}


void gatt_server_demo_init()
{
    gatts_demo_cli_init();

    bt_err_t ret = BK_FAIL;
    struct bk_ble_db_cfg ble_db_cfg;

    ret = rtos_init_semaphore(&gatt_sema, 1);
    if(ret != BK_OK){
        BLEGATTS_LOGE("gatts init sema fial \n");
        return;
    }

    bk_ble_set_notice_cb(ble_gatts_notice_cb);

    ble_db_cfg.att_db = (ble_attm_desc_t *)gatts_service_db;
    ble_db_cfg.att_db_nb = GATTS_IDX_NB;
    ble_db_cfg.prf_task_id = PRF_TASK_ID_GATTS;
    ble_db_cfg.start_hdl = 0;
    ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
    ble_db_cfg.uuid[0] = GATTS_SERVICE_UUID & 0xFF;
    ble_db_cfg.uuid[1] = GATTS_SERVICE_UUID >> 8;

    ret = bk_ble_create_db(&ble_db_cfg);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BLEGATTS_LOGE("create gatt db failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BLEGATTS_LOGE("wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    else
    {
        BLEGATTS_LOGI("create gatt db success\n");
    }

    //set adv data
    uint8_t adv_data[ADV_MAX_SIZE] = {0};
	uint8_t adv_index = 0;
	uint8_t len_index = 0;
	uint8_t mac[6];

	/* flags */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_FLAGS;
	adv_data[adv_index++] = 0x06;
	adv_data[len_index] = 2;

	/* local name */
	bk_bluetooth_get_address(mac);

	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

	ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X",
	              ADV_NAME_HEAD, mac[0], mac[1], mac[2]);

    bk_ble_appm_set_dev_name(ret, &adv_data[adv_index]);
    BLEGATTS_LOGI("%s, dev_name:%s, ret:%d \n", __func__, (char *)&adv_data[adv_index], ret);
	adv_index += ret;
	adv_data[len_index] = ret + 1;

	/* 16bit uuid */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
	adv_data[adv_index++] = GATTS_UUID & 0xFF;
	adv_data[adv_index++] = GATTS_UUID >> 8;
	adv_data[len_index] = 3;

	/* manufacturer */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
	adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
	adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
	adv_data[len_index] = 3;
    BLEGATTS_LOGI("adv data length :%d \n", adv_index);

	/* set adv paramters */
    ble_adv_param_t adv_param;
	int actv_idx = 0;

	os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
	adv_param.chnl_map = ADV_ALL_CHNLS;
	adv_param.adv_intv_min = BLE_GATTS_ADV_INTERVAL_MIN;
	adv_param.adv_intv_max = BLE_GATTS_ADV_INTERVAL_MAX;

#if 1
	adv_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
#else
	adv_param.own_addr_type = OWN_ADDR_TYPE_RANDOM_ADDR;
#endif

	adv_param.adv_type = ADV_TYPE_LEGACY;
	adv_param.adv_prop = ADV_PROP_CONNECTABLE_BIT | ADV_PROP_SCANNABLE_BIT;
	adv_param.prim_phy = INIT_PHY_TYPE_LE_1M;
	adv_param.second_phy = INIT_PHY_TYPE_LE_1M;

	ret = bk_ble_create_advertising(actv_idx, &adv_param, ble_gatt_cmd_cb);

	if (ret != BK_ERR_BLE_SUCCESS)
	{
		BLEGATTS_LOGE("config adv paramters failed %d\n", ret);
		goto error;
	}

	ret = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

	if (ret != BK_OK)
	{
		BLEGATTS_LOGE("wait semaphore failed at %d, %d\n", ret, __LINE__);
		goto error;
	}
	else
	{
		BLEGATTS_LOGI("set adv paramters success\n");
	}

	/* set adv paramters */
	ret = bk_ble_set_adv_data(actv_idx, adv_data, adv_index, ble_gatt_cmd_cb);

	if (ret != BK_ERR_BLE_SUCCESS)
	{
		BLEGATTS_LOGE("set adv data failed %d\n", ret);
		goto error;
	}

	ret = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

	if (ret != BK_OK)
	{
		BLEGATTS_LOGE("wait semaphore failed at %d, %d\n", ret, __LINE__);
		goto error;
	}
	else
	{
		BLEGATTS_LOGI("set adv data success\n");
	}

#if 0
    const uint8_t tmp_addr[6] = {0, 1, 2, 0x22, 0x11, 0xca};
    ret = bk_ble_set_adv_random_addr(actv_idx, (uint8_t *)tmp_addr, ble_gatt_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BLEGATTS_LOGE("set adv random addr failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BLEGATTS_LOGE("wait adv random addr semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    else
    {
        BLEGATTS_LOGI("set adv random addr success\n");
    }
#endif

	/* sart adv */
	ret = bk_ble_start_advertising(actv_idx, 0, ble_gatt_cmd_cb);

	if (ret != BK_ERR_BLE_SUCCESS)
	{
		BLEGATTS_LOGE("start adv failed %d\n", ret);
		goto error;
	}

	ret = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

	if (ret != BK_OK)
	{
		BLEGATTS_LOGE("wait semaphore failed at %d, %d\n", ret, __LINE__);
		goto error;
	}
	else
	{
		BLEGATTS_LOGI("sart adv success\n");
	}

    BLEGATTS_LOGI("%s success\n", __func__);
    return;
error:
    BLEGATTS_LOGE("%s fail \n", __func__);
}

void gatts_demo_event_notify(uint16_t opcode, int status)
{
	uint8_t data[] =
	{
		opcode & 0xFF, opcode >> 8,
		                      status & 0xFF,                                                          /* status           */
		                      0, 0,                                                                   /* payload length   */
	};
	if(gatt_conn_ind == INVALID_HANDLE)
    {
        BLEGATTS_LOGI("BLE is disconnected, can not send data !!!\r\n");
    }else
    {
        bk_ble_send_noti_value(gatt_conn_ind, sizeof(data), data, PRF_TASK_ID_GATTS, GATTS_IDX_CHAR_VALUE);
    }
}

static void gatt_server_help()
{
    BLEGATTS_LOGI("\r\n");
    BLEGATTS_LOGI("ble_gatts help\n");
    BLEGATTS_LOGI("ble_gatts notify\n");
    BLEGATTS_LOGI("ble_gatts adv_en [1|0]\n");
    BLEGATTS_LOGI("\r\n");
}

void gatt_server_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *cmd_rsp = CMD_RSP_SUCCEED;
    if (strcmp(argv [ 1 ], "help") == 0)
    {
        gatt_server_help();
    }else if(strcmp(argv [ 1 ], "notify") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTS_LOGE("please connect first \n");
            goto error;
        }
        static uint16_t notify_test_value = 0;
        gatts_demo_event_notify(notify_test_value++, BK_OK);
    }else if(strcmp(argv [ 1 ], "adv_en") == 0)
    {
        int actv_idx = 0;
        int err = BK_FAIL;
        uint8_t en = os_strtoul(argv[2], NULL, 10);
        if (en)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(BLE_ACTV_ADV_CREATED);
        }
        else
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(BLE_ACTV_ADV_STARTED);
        }

        if (actv_idx == BLE_MAX_ACTV)
        {
            BLEGATTS_LOGE("ble is advertising or create a new ble adv\n");
            goto error;
        }

        if (en)
        {
            err = bk_ble_start_advertising(actv_idx, 0, ble_gatt_cmd_cb);
        }
        else
        {
            err = bk_ble_stop_advertising(actv_idx, ble_gatt_cmd_cb);
        }
        if(err != BK_OK)
        {
            BLEGATTS_LOGE("start or stop adv fail \n");
            goto error;
        }
        err = rtos_get_semaphore(&gatt_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    	if (err != BK_OK)
    	{
    		BLEGATTS_LOGE("wait semaphore failed at %d, %d\n", err, __LINE__);
    		goto error;
    	}
    	else
    	{
    		BLEGATTS_LOGI("sart adv success\n");
    	}
    }
    else if(strcmp(argv [ 1 ], "bond") == 0)
    {
        if(gatt_conn_ind == INVALID_HANDLE)
        {
            BLEGATTS_LOGE("please connect first \n");
            goto error;
        }
        bk_ble_create_bond(gatt_conn_ind, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
                                GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);;
    }else
    {
        goto error;
    }

    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
    return;
error:
    cmd_rsp = CMD_RSP_ERROR;
    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
}

static const struct cli_command s_gatt_commands[] =
{
#if CONFIG_BLUETOOTH
    {"ble_gatts", "ble_gatts arg1 arg2 ... argn",  gatt_server_command},
#endif
};

static int gatts_demo_cli_init(void)
{
    return cli_register_commands(s_gatt_commands, BLE_GATT_CMD_CNT);
}

#endif
