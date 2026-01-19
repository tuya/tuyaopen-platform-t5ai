/*
 * Copyright 2020-2025 Beken

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pbuf.h"
#include "wdrv_co_list.h"
#include "bk_wifi_types.h"
//#include "bk_rw.h"
#include <modules/wifi_types.h>
#include "bk_wifi_private.h"


#define MAX_SCAN_AP_NUM             48
#define SSID_MAX_LEN                33     /**< Maximum **NULL-terminated** WiFi SSID length */
#define PASSWORD_MAX_LEN            64     /**< Maximum **NULL-terminated** WiFi password length */
#define NETIF_IP4_STR_LEN           16
#define WIFI_BSSID_LEN              6      /**< Length of BSSID */
#define ETH_ALEN                    6
#define WIFI_MAX_SCAN_CHAN_DUR      200    /**< scan duration param, need less than 200ms */
#define WIFI_CHANNEL_NUM_2G4        14    /**< Maximum supported 2.4G channel number */
#define WIFI_MIN_CHAN_NUM           1      /**< Minimum supported channel number */
#define WIFI_MAX_CHAN_NUM           14     /**< Maximum supported channel number */
#define WIFI_CHANNEL_NUM_5G         28    /**< Maximum supported 5G channel numbe*/
#define WIFI_2BAND_MAX_CHAN_NUM     (WIFI_CHANNEL_NUM_2G4 + WIFI_CHANNEL_NUM_5G)
// #define DEFAULT_CHANNEL_AP          1     /**< Default Channel of SoftAP */
// #define WIFI_MAC_LEN                6      /**< Length of MAC */

/* host cmd setting */
#define WDRV_CMD_WAITCFM           1
#define WDRV_CMD_NOWAITCFM         0
#define WDRV_CMDCFM_TIMEOUT        2000
#define WDRV_MAX_MSG_CNT                      (0x800)
#define WDRV_CMD_CFM_OFFSET                   (0x8000)

#define BK_CFM_GET_CMD_ID(cfm_id)           (cfm_id - WDRV_CMD_CFM_OFFSET)


typedef struct _wdrv_cmd_cfm
{
    struct co_list_hdr list;
    uint32_t waitcfm;
    uint16_t cfm_id;
    uint16_t cfm_sn;
    uint8_t *cfm_buf;
    uint16_t cfm_len;
    beken_semaphore_t sema;
}wdrv_cmd_cfm;

/*CMD header from AP to CP */
typedef struct _wdrv_cmd_hdr
{
    uint32_t rsv0;
    uint16_t cmd_id;
    uint16_t cmd_sn;
    uint16_t rsv1;
    uint16_t len;
}wdrv_cmd_hdr;

/*Event or CMD-CFM header from CP to AP */
typedef struct _wdrv_rx_msg
{
    uint16_t id;                ///< Message id.
    uint16_t cfm_sn;            /// confirm msg sequence
    uint16_t rsv;               /// reserve
    uint16_t param_len;         ///< Parameter embedded struct length.
    uint32_t pattern;           ///< Used to stamp a valid MSG buffer
    uint32_t param[1];          ///< Parameter embedded struct. Must be word-aligned.
}wdrv_rx_msg;

struct wdrv_connect_req
{
    wdrv_cmd_hdr cmd_hdr;
    char ssid[SSID_MAX_LEN];
    char pw[PASSWORD_MAX_LEN];
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_connect_ind
{
    uint8_t ussid[SSID_MAX_LEN];
    int8_t  rssi;
    u32  ip;
    u32  mk;
    u32  gw;
    u32  dns;
};

struct wdrv_mac_addr_cfm
{
    uint8_t mac_addr[6];
};

struct wdrv_media_mode_req
{
    wdrv_cmd_hdr cmd_hdr;
    bool media_flag;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_media_quality_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t media_quality;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_start_ap_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t band;
    char ssid[SSID_MAX_LEN];
    char pw[PASSWORD_MAX_LEN];
    uint8_t channel;
    uint8_t hidden;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_stop_ap_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_start_scan_req
{
    wdrv_cmd_hdr cmd_hdr;
    char ssid[SSID_MAX_LEN];
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_interval_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_wifi_status_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_ap_config_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_ip4_config_req
{
    wdrv_cmd_hdr cmd_hdr;
	uint8_t flag;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_staipup_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_apipup_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};


struct wdrv_set_csa_coexist_mode_flag_req
{
    wdrv_cmd_hdr cmd_hdr;
    bool is_close;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_set_mac_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t mac_addr[6];
    wdrv_cmd_cfm cmd_cfm;
};

enum BK_CMD_TYPE
{
    // Wi-Fi command
    BK_CMD_SCAN_WIFI           = 0x1,
    BK_CMD_CONNECT             = 0x2,
    BK_CMD_DISCONNECT          = 0x3,
    BK_CMD_START_AP            = 0x4,
    BK_CMD_CHANGE_AP_MODE      = 0x5,
    BK_CMD_STOP_AP             = 0x6,
    BK_CMD_GET_WLAN_STATUS     = 0x7,
    BK_CMD_WIFI_MMD_CONFIG     = 0x8,
    BK_CMD_SET_NET_INFO        = 0x9,
    BK_CMD_SET_AUTO_RECONNECT  = 0xA,
    BK_CMD_SET_MEDIA_MODE      = 0xB,
    BK_CMD_SET_MEDIA_QUALITY   = 0xC,
    BK_CMD_SET_COEX_CSA         = 0xF,

    //Debug info section
    BK_INTERFACE_DEBUG_CMD     = 0x110,

    // system command
    BK_CMD_SET_MAC_ADDR        = 0x201,
    BK_CMD_GET_MAC_ADDR        = 0x202,
    BK_CMD_ENTER_SLEEP         = 0x203,
    BK_CMD_EXIT_SLEEP          = 0x204,
    BK_CMD_CONTROLLER_AT       = 0x205,
    BK_CMD_KEEPALIVE_CFG       = 0x206,
    BK_CMD_SET_TIME            = 0x207,
    BK_CMD_GET_TIME            = 0x208,
    BK_CMD_CUSTOMER_DATA       = 0x209,
    BK_CMD_START_OTA           = 0x20A,
    BK_CMD_SEND_OTA_PKT        = 0x20B,
    BK_CMD_STOP_OTA            = 0x20C,

    BK_CMD_WIFI_API_START      = 0x300,
    BK_CMD_WIFI_API_END        = 0x5FF,

    BK_CMD_BUTT                = WDRV_MAX_MSG_CNT - 1
};

/* CP Wi-Fi Mode */
enum WLAN_MODE
{
    WIFI_MODE_IDLE = 0,                 /* Wi-Fi Idle Mode */
    WIFI_MODE_STA,                      /* Wi-Fi Station Mode */
    WIFI_MODE_AP,                       /* Wi-Fi SoftAP Mode */
    WIFI_MODE_MAX
};

#define WLAN_DEFAULT_IP         "192.168.188.1"
#define WLAN_DEFAULT_GW         "192.168.188.1"
#define WLAN_DEFAULT_MASK       "255.255.255.0"

struct wdrv_ap_status_cfm
{
    uint8_t  status;
    uint32_t ip;
    uint32_t gw;
    uint32_t mk;
};

struct wdrv_ap_assoc_sta_ind
{
    uint8_t sub_sta_addr[6];
};

struct wdrv_wlan_status_cfm
{
    uint8_t status;
    int8_t  rssi;
    uint8_t ussid[SSID_MAX_LEN];
    char  ip[NETIF_IP4_STR_LEN];
    char  mk[NETIF_IP4_STR_LEN];
    char  gw[NETIF_IP4_STR_LEN];
    char  dns[NETIF_IP4_STR_LEN];
};

struct wdrv_scan_result_cfm
{
    uint8_t  scan_num;
    int8_t   rssi;
    uint8_t  bssid[6];
    uint8_t  ssid[SSID_MAX_LEN];
    uint32_t  akm;
    int      channal;
};


/* event-table from CP to AP */
enum BK_EVENT_TYPE
{
    BK_EVT_CONNECT_IND          = 0x1,
    BK_EVT_DISCONNECT_IND       = 0x2,
    BK_EVT_START_AP_IND         = 0x3,
    BK_EVT_ASSOC_AP_IND         = 0x4,
    BK_EVT_DISASSOC_AP_IND      = 0x5,
    BK_EVT_STOP_AP_IND          = 0x6,
    BK_EVT_SCAN_WIFI_IND        = 0x7,
    BK_EVT_WIFI_FAIL_IND        = 0x8,
    BK_EVT_BCN_CC_RXED          = 0x9,
    BK_EVT_CSI_INFO_IND         = 0xA,

    // BLE event
    // BK_EVT_BLE_XX            = 0x101

    // system event
    BK_EVT_CONTROLLER_AT_IND    = 0x201,
    BK_EVT_CUSTOMER_IND         = 0x202,

    // Wi-Fi API event
    BK_EVT_WIFI_API_START       = 0x300,
    BK_EVT_WIFI_API_END         = 0x3FF,

    // throughput test
    BK_EVT_TP_TEST              = 0x401,


    BK_EVT_BUTT                 = WDRV_MAX_MSG_CNT - 1
};
/* cmd-table from app to netdrv */

typedef struct _wdrv_wlan {
    int8_t  wlan_mode;
    uint8_t wlan_link_sta_status;
    bool    comp_sign_get_mac_ready;

    beken_mutex_t cfm_lock;

    struct co_list cfm_pending_list;

    struct wdrv_mac_addr_cfm macaddr_cfm;
    struct wdrv_wlan_status_cfm get_wlan_cfm;
    struct wdrv_connect_ind connect_ind;
    struct wdrv_ap_status_cfm ap_status_cfm;
    struct wdrv_ap_assoc_sta_ind ap_assoc_sta_addr_ind;
    struct wdrv_scan_result_cfm *scan_wifi_cfm_ptr;
    struct wdrv_scan_result_cfm scan_wifi_cfm[MAX_SCAN_AP_NUM];

}wdrv_wlan;


/* A-Core Wi-Fi AP Mode State */
enum WLAN_LINK_AP_STATUS
{
    CONTROLLER_AP_START,                /* A-Core AP Mode Started */
    CONTROLLER_AP_CLOSE,                /* A-Core AP Mode Closed  */
};


#define CIFD_CUST_DEBUG_CODE_MAGIC                  (0xAABBCCDD)

#define CIFD_CUST_PATTERN  (0xa5a6)
#define MAX_CIFD_CUST_SIZE   256

typedef enum{
    CIFD_CMD_BLE_DATA_TO_APK             = 0x0001,

    CIFD_EVENT_BLE_DATA_TO_USER          = 0x1001,

}CIFD_CMD_EVENT;

typedef struct{
    uint16_t magic;
    uint16_t cid;
    uint16_t ctl;
    uint16_t seq;
    uint16_t checksum;
    uint16_t len;
}CIFD_PROTO_HDR;

typedef struct{
    CIFD_PROTO_HDR header;
    uint8_t data[MAX_CIFD_CUST_SIZE];
}CIFD_CUST_DATA;

typedef struct cifd_cust_msg_hdr
{
    uint16_t cmd_id;
    uint16_t len;
    uint8_t payload[0];
}cifd_cust_msg_hdr_t;

struct wdrv_customer_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t data[MAX_CIFD_CUST_SIZE];
};

#define OPCODE_LEN                      (2)
#define DATA_LEN_LEN                    (2)
#define SSID_LEN_LEN                    (2)
#define PW_LEN_LEN                      (2)


/*   API    */
void wdrv_host_init(void);
int wdrv_get_mac_addr();
bk_err_t bk_wdrv_get_mac(uint8_t *mac, mac_type_t type);
uint32_t wdrv_param_init(void);
int bk_platform_get_wlan_status(void);
extern void wdrv_rx_handle_event(wdrv_rx_msg *msg);
extern void wdrv_rx_handle_cmd_confirm(wdrv_rx_msg *msg);
void wdrv_notify_sta_connected(void);
void wdrv_notify_sta_got_ip(void);
void bk_rx_handle_customer_event(void *data, uint16_t len);
int bk_wdrv_send_customer_data(uint8_t *data, uint16_t len);
int bk_wdrv_customer_transfer(uint16_t cmd_id, uint8_t * data, uint16_t len);
void wdrv_notify_sta_disconnected(void *data, uint16_t len);
void wdrv_notify_sap_sta_connected(void);
void wdrv_notify_sap_sta_disconnected(void);
bk_err_t bk_wifi_bcn_cc_rxed_cb(void *data, uint16_t len);
void bk_wifi_csi_info_cb(void *data);


FUNC_1PARAM_PTR bk_wlan_get_status_cb(void);
void wifi_netif_call_status_cb_when_sta_got_ip(void);
void mhdr_set_station_status(wifi_linkstate_reason_t info);
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);

typedef void (* rx_handle_customer_event_cb)(void *data, uint16_t len);
void bk_customer_event_register_callback(rx_handle_customer_event_cb callback);
bk_err_t wdrv_cntrl_get_cif_stats();
extern wdrv_wlan wdrv_host_env;

extern general_param_t *g_wlan_general_param;
extern ap_param_t *g_ap_param_ptr;
extern sta_param_t *g_sta_param_ptr;

#ifdef __cplusplus
}
#endif
