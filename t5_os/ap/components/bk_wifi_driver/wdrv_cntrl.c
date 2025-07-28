/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_cntrl.c 
 * 
 * @brief Beken Wi-Fi Driver Platform Entry
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "wdrv_cntrl.h"
#include "wdrv_main.h"
#include "components/log.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_rx.h"
#include "wifi_api.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"
#include <components/netif.h>
#include "components/event.h"
#include "wifi_api_ipc.h"

#define TAG "wdrv_cntrl"

wdrv_wlan wdrv_host_env;

wifi_linkstate_reason_t connect_flag = {WIFI_LINKSTATE_STA_IDLE, WIFI_REASON_MAX};

FUNC_1PARAM_PTR connection_status_cb = 0;


static rx_handle_customer_event_cb s_rx_handle_cust_event_cb = NULL;

void bk_customer_event_register_callback(rx_handle_customer_event_cb callback)
{
    s_rx_handle_cust_event_cb = callback ;
}

void wdv_rx_handle_customer_event(void *data, uint16_t len)
{
    if (s_rx_handle_cust_event_cb) {
        s_rx_handle_cust_event_cb(data, len);
    }

}

uint32_t wdrv_param_init(void)
{
    if (NULL == g_wlan_general_param) {
        g_wlan_general_param = (general_param_t *)os_zalloc(sizeof(general_param_t));
        BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */
    }

    if (NULL == g_ap_param_ptr) {
        g_ap_param_ptr = (ap_param_t *)os_zalloc(sizeof(ap_param_t));
        BK_ASSERT(g_ap_param_ptr); /* ASSERT VERIFIED */
    }

    if (NULL == g_sta_param_ptr) {
        g_sta_param_ptr = (sta_param_t *)os_zalloc(sizeof(sta_param_t));
        BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    }

    return 0;
}

bk_err_t bk_wdrv_get_mac(uint8_t *mac, mac_type_t type)
{
    uint8_t mac_mask = (0xff & (2/*NX_VIRT_DEV_MAX*/ - 1));
    uint8_t mac_low;

    if (wdrv_get_mac_addr() != 0)
    {
        WDRV_LOGE("bk_wdrv_get_mac  failed\n");
        return BK_FAIL;
    }

    switch (type) {
    case MAC_TYPE_BASE:
        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        break;

    case MAC_TYPE_AP:
        mac_mask = (0xff & (2/*NX_VIRT_DEV_MAX*/ - 1));

        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        mac_low = mac[5];

        // if  NX_VIRT_DEV_MAX == 4.
        // if support AP+STA, mac addr should be equal with each other in byte0-4 & byte5[7:2],
        // byte5[1:0] can be different
        // ie: mac[5]= 0xf7,  so mac[5] can be 0xf4, f5, f6. here wre chose 0xf4
        mac[5] &= ~mac_mask;
        mac_low = ((mac_low & mac_mask) ^ mac_mask);
        mac[5] |= mac_low;
        break;

    case MAC_TYPE_STA:
        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        break;

    default:
        return BK_ERR_INVALID_MAC_TYPE;
    }

    return BK_OK;
}

int wdrv_get_mac_addr()
{
    wdrv_cmd_hdr req = {0};
    wdrv_cmd_cfm cmd_cfm = {0};
    req.cmd_id = BK_CMD_GET_MAC_ADDR;
    cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    cmd_cfm.cfm_id = 0;

    WDRV_LOGD("wdrv_host_cmd_id : %d\n", req.cmd_id);
    //ToDo
    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &cmd_cfm, NULL);
    return 0;
}

void wdrv_notify_scan_done(void *data, uint16_t len)
{
    wifi_event_scan_done_t event_data = {0};
    /* post event scan done*/
    os_memset(&event_data, 0, sizeof(event_data));
    os_memcpy(&event_data, data, len);
    bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, &event_data,
    sizeof(event_data), BEKEN_NEVER_TIMEOUT);
}

void wdrv_notify_sta_connected(void)
{
    wifi_event_sta_connected_t sta_connected = {0};
    /* post event sta_connected*/
    os_memset(&sta_connected, 0, sizeof(sta_connected));
    os_memcpy(&sta_connected.ssid, wdrv_host_env.connect_ind.ussid, sizeof(wdrv_host_env.connect_ind.ussid));

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_STA_CONNECTED,
                                &sta_connected, sizeof(sta_connected), BEKEN_NEVER_TIMEOUT));
}

void wdrv_notify_sta_disconnected(void *data, uint16_t len)
{
    wifi_event_sta_disconnected_t sta_disconnected = {0};
    os_memcpy(&sta_disconnected, data, len);

    /* post event */
    WDRV_LOGV("sta disconnect reason %d,local %d\n",
    sta_disconnected.disconnect_reason, sta_disconnected.local_generated);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
                             &sta_disconnected, sizeof(sta_disconnected), BEKEN_NEVER_TIMEOUT));
}

void wdrv_notify_sap_sta_connected(void)
{
    wifi_event_ap_connected_t ap_connected = {0};
    /* post evevnt EVENT_WIFI_AP_CONNECTED */
    os_memset(&ap_connected, 0, sizeof(ap_connected));
    os_memcpy(ap_connected.mac, wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr, ETH_ALEN);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_AP_CONNECTED,
                                &ap_connected, sizeof(ap_connected), BEKEN_NEVER_TIMEOUT));
}

void wdrv_notify_sap_sta_disconnected(void)
{
    wifi_event_ap_connected_t ap_disconnected = {0};
    /* post evevnt EVENT_WIFI_AP_DISCONNECTED */
    os_memset(&ap_disconnected, 0, sizeof(ap_disconnected));
    os_memcpy(ap_disconnected.mac, wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr, ETH_ALEN);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_AP_DISCONNECTED,
                                &ap_disconnected, sizeof(ap_disconnected), BEKEN_NEVER_TIMEOUT));
}

void mhdr_set_station_status(wifi_linkstate_reason_t info)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	connect_flag.state = info.state;
	connect_flag.reason_code = info.reason_code;
	GLOBAL_INT_RESTORE();
}

wifi_linkstate_reason_t mhdr_get_station_status(void)
{
	return connect_flag;
}

FUNC_1PARAM_PTR bk_wlan_get_status_cb(void)
{
	return connection_status_cb;
}

void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb)
{
	connection_status_cb = cb;
}

void wifi_netif_call_status_cb_when_sta_got_ip(void)
{
	FUNC_1PARAM_PTR fn;
	wifi_linkstate_reason_t info = mhdr_get_station_status();

	fn = (FUNC_1PARAM_PTR)bk_wlan_get_status_cb();
	if(fn) {
		info.state = WIFI_LINKSTATE_STA_GOT_IP;
		info.reason_code = WIFI_REASON_MAX;
		(*fn)(&info);
	}
}

void wdrv_notify_sta_got_ip(void)
{
    wifi_linkstate_reason_t info;

    /* set wifi status */
    info.state = WIFI_LINKSTATE_STA_GOT_IP;
    info.reason_code = WIFI_REASON_MAX;
    mhdr_set_station_status(info);

    /* post event GOT_IP4 */
    netif_event_got_ip4_t event_data = {0};
    event_data.netif_if = NETIF_IF_STA;

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
                                &event_data, sizeof(event_data), BEKEN_NEVER_TIMEOUT));
}

int bk_wdrv_send_customer_data(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        WDRV_LOGE("bk_wdrv_send_customer_data : invalid input parameters\n");
        return -1;
    }
    struct wdrv_customer_req cust_req = {0};
    wdrv_cmd_cfm cmd_cfm = {0};
    cust_req.cmd_hdr.cmd_id = BK_CMD_CUSTOMER_DATA;
    cust_req.cmd_hdr.len = len;
    cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    cmd_cfm.cfm_id = 0;

    os_memcpy(cust_req.data, data, len);
    WDRV_LOGV("bk_wdrv_send_customer_data : %d, len: %d\n", cust_req.cmd_hdr.cmd_id, len + sizeof(wdrv_cmd_hdr));
#if 0
    WDRV_LOGD("customer_data: ");
    for (int i = 0; i < len + sizeof(wdrv_cmd_hdr); i++) {
        WDRV_LOGD("%02x ", cust_req.data[i]);
    }
    WDRV_LOGD("\n");
#endif
    wdrv_tx_msg((uint8_t *)&cust_req, len + sizeof(wdrv_cmd_hdr) , &cmd_cfm, NULL);
    return 0;
}

int bk_wdrv_customer_transfer(uint16_t cmd_id, uint8_t *data, uint16_t len)
{
    int ret = 0;

    if (data == NULL || len == 0) {
        WDRV_LOGE("bk_wdrv_customer_transfer : Invalid input parameters");
        return -1;
    }

    cifd_cust_msg_hdr_t *cust_trans = os_malloc(sizeof(cifd_cust_msg_hdr_t) + len);

    cust_trans->cmd_id = cmd_id;
    cust_trans->len = len;

    WDRV_LOGV("bk_wdrv_customer_transfer : %d, len: %d\n", cust_trans->cmd_id, sizeof(cifd_cust_msg_hdr_t) + cust_trans->len);
    os_memcpy((uint8_t*)cust_trans + sizeof(cifd_cust_msg_hdr_t), data, len);
    ret = bk_wdrv_send_customer_data((uint8_t*)cust_trans, sizeof(cifd_cust_msg_hdr_t) + cust_trans->len);

    os_free(cust_trans);

    return ret;
}

bk_err_t wdrv_cntrl_get_cif_stats()
{
    struct get_cif_stats
    {
        wdrv_cmd_hdr cmd_hdr;
        wdrv_cmd_cfm cmd_cfm;
    };
    struct get_cif_stats req = {0};

    req.cmd_hdr.cmd_id =  BK_INTERFACE_DEBUG_CMD;
    req.cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    req.cmd_cfm.cfm_id = 0;
    WDRV_LOGD("%s,%d\n",__func__,__LINE__);

    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &req.cmd_cfm, NULL);

    return BK_OK;
}

void wdrv_rx_handle_cmd_confirm(wdrv_rx_msg *msg)
{
    WDRV_LOGV("%s,%d\n",__func__,__LINE__);

    switch(BK_CFM_GET_CMD_ID(msg->id)) {
        case BK_CMD_GET_MAC_ADDR:
            os_memcpy(&wdrv_host_env.macaddr_cfm, msg->param, sizeof(struct wdrv_mac_addr_cfm));
            WDRV_LOGV("MAC addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
            wdrv_host_env.macaddr_cfm.mac_addr[0], wdrv_host_env.macaddr_cfm.mac_addr[1],
            wdrv_host_env.macaddr_cfm.mac_addr[2], wdrv_host_env.macaddr_cfm.mac_addr[3],
            wdrv_host_env.macaddr_cfm.mac_addr[4], wdrv_host_env.macaddr_cfm.mac_addr[5]);
            break;
        case BK_CMD_GET_WLAN_STATUS:
            break;
        case BK_CMD_CONNECT:
            WDRV_LOGD("SET-MCU-WLAN: start connect\r\n");
            break;
        case BK_CMD_DISCONNECT:
            WDRV_LOGV("SET-MCU-WLAN: start disconnect\r\n");
            break;
        case BK_CMD_SET_MEDIA_MODE:
            WDRV_LOGV("SET-MCU-WLAN: set media mode\r\n");
            break;
        case BK_CMD_SET_MEDIA_QUALITY:
            WDRV_LOGV("SET-MCU-WLAN: set media quality\r\n");
            break;
        case BK_CMD_START_AP:
            WDRV_LOGV("MCU-AP-STATE: start AP\r\n");
            break;
        default:
            WDRV_LOGV("%s,%d\n",__func__,__LINE__);
            break;
    }
    wdrv_rx_confirm_tx_msg(msg);

}
void wdrv_rx_handle_wifi_api_event(wdrv_rx_msg *msg)
{
    wifi_handle_api_evt(msg->id, (uint8_t *)msg->param, msg->param_len);
}
void wdrv_rx_handle_wifi_cntrl_event(wdrv_rx_msg *msg)
{
    WDRV_LOGD("%s,%d\n",__func__,__LINE__);
    //int loop_idx = 0;
    switch(msg->id) {
        case BK_EVT_CONNECT_IND:
            wdrv_host_env.wlan_link_sta_status = WIFI_LINKSTATE_STA_CONNECTED;
            wdrv_host_env.wlan_mode = WIFI_MODE_STA;
            os_memcpy(&wdrv_host_env.connect_ind, msg->param, sizeof(struct wdrv_connect_ind));
            WDRV_LOGD(TAG, "WLAN-INDICATE: connected\n");
#if 0
            BK_LOGD(NULL, "WLAN-INDICATE: connect to \'%s\' (%3d dBm)\r\n",
                wdrv_host_env.connect_ind.ussid, wdrv_host_env.connect_ind.rssi);
            BK_LOGD(NULL, "ip: %d.%d.%d.%d, mk: %d.%d.%d.%d, gw: %d.%d.%d.%d, dns: %d.%d.%d.%d\n",
                (wdrv_host_env.connect_ind.ip >> 0 ) & 0xff, (wdrv_host_env.connect_ind.ip >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.ip >> 16) & 0xff, (wdrv_host_env.connect_ind.ip >> 24) & 0xff,
                (wdrv_host_env.connect_ind.mk >> 0 ) & 0xff, (wdrv_host_env.connect_ind.mk >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.mk >> 16) & 0xff, (wdrv_host_env.connect_ind.mk >> 24) & 0xff,
                (wdrv_host_env.connect_ind.gw >> 0 ) & 0xff, (wdrv_host_env.connect_ind.gw >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.gw >> 16) & 0xff, (wdrv_host_env.connect_ind.gw >> 24) & 0xff,
                (wdrv_host_env.connect_ind.dns >> 0 ) & 0xff, (wdrv_host_env.connect_ind.dns >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.dns >> 16) & 0xff, (wdrv_host_env.connect_ind.dns >> 24) & 0xff);
#endif
            wdrv_notify_sta_connected();

            break;
        case BK_EVT_DISCONNECT_IND:
            wdrv_host_env.wlan_link_sta_status = WIFI_LINKSTATE_STA_DISCONNECTED;
            wdrv_host_env.wlan_mode = WIFI_MODE_IDLE;
            WDRV_LOGD("WLAN-INDICATE: disconected and stop send data\n");
            wdrv_notify_sta_disconnected(msg->param, msg->param_len);
            break;
        case BK_EVT_CUSTOMER_IND:
            WDRV_LOGD(TAG, "Smart Config\n");
            wdv_rx_handle_customer_event(msg->param, msg->param_len);
            break;
        case BK_EVT_START_AP_IND:
            os_memcpy(&wdrv_host_env.ap_status_cfm, msg->param, sizeof(struct wdrv_ap_status_cfm));
            if (wdrv_host_env.ap_status_cfm.status == CONTROLLER_AP_START) {
                WDRV_LOGD("MCU-AP-STATE: start AP Success\n");
#if 0
                BK_LOGD(NULL, "ip: %d.%d.%d.%d, mk: %d.%d.%d.%d, gw: %d.%d.%d.%d, dns: %d.%d.%d.%d\n",
                    (wdrv_host_env.ap_status_cfm.ip >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.ip >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.ip >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.ip >> 24) & 0xff,
                    (wdrv_host_env.ap_status_cfm.mk >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.mk >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.mk >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.mk >> 24) & 0xff,
                    (wdrv_host_env.ap_status_cfm.gw >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.gw >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.gw >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.gw >> 24) & 0xff,);
#endif
                wdrv_host_env.wlan_mode = WIFI_MODE_AP;
            } else if (wdrv_host_env.ap_status_cfm.status == CONTROLLER_AP_CLOSE) {
                WDRV_LOGE("MCU-AP-STATE: start AP Fail\n");
            }
            break;
        case BK_EVT_SCAN_WIFI_IND:
            WDRV_LOGI("BK_EVT_SCAN_WIFI_IND\n");
            wdrv_notify_scan_done(msg->param, msg->param_len);
            break;
        case BK_EVT_ASSOC_AP_IND:
            os_memcpy(&wdrv_host_env.ap_assoc_sta_addr_ind, msg->param, sizeof(struct wdrv_ap_assoc_sta_ind));
            WDRV_LOGD("AP-INDICATE: %x:%x:%x:%x:%x:%x connected\n",
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[0], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[1],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[2], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[3],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[4], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[5]);

            wdrv_notify_sap_sta_connected();
            break;
        case BK_EVT_DISASSOC_AP_IND:
            WDRV_LOGV("AP-INDICATE: disassoc\n");
            os_memcpy(&wdrv_host_env.ap_assoc_sta_addr_ind, msg->param, sizeof(struct wdrv_ap_assoc_sta_ind));
            WDRV_LOGV("%x:%x:%x:%x:%x:%x\n",
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[0], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[1],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[2], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[3],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[4], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[5]);

            wdrv_notify_sap_sta_disconnected();
            break;
        case BK_EVT_STOP_AP_IND:
            wdrv_host_env.ap_status_cfm.status = CONTROLLER_AP_CLOSE;
            wdrv_host_env.wlan_mode = WIFI_MODE_IDLE;
            WDRV_LOGV("MCU-AP-STATE: stop AP success\n");
            break;
        default:
            WDRV_LOGD("%s msg %x invaild\n", __func__, msg->id);
            return;
    }
}
void wdrv_rx_handle_event(wdrv_rx_msg *msg)
{
    WDRV_LOGD("%s,%d\n",__func__,__LINE__);
    if ((msg->id >= BK_EVT_WIFI_API_START) && (msg->id <= BK_EVT_WIFI_API_END))
    {
        wdrv_rx_handle_wifi_api_event(msg);
    }
    else
    {
        wdrv_rx_handle_wifi_cntrl_event(msg);
    }
}

void wdrv_host_init(void)
{
    WDRV_LOGV("%s, %d\r\n", __func__, __LINE__);

    co_list_init((struct co_list *)&wdrv_host_env.cfm_pending_list);
    rtos_init_mutex(&wdrv_host_env.cfm_lock);

    if(wdrv_get_mac_addr() != 0)
        return;

    /* init wdrv common params */
    wdrv_param_init();

    /* Init Wi-Fi driver netif */
    //bk_wifi_init();
}

