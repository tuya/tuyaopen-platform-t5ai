#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <common/bk_include.h>
#include "pbuf.h"
#include "co_list.h"
//#include "sdio_slave_driver.h"
#include <string.h>
#include <os/mem.h>
#include <os/os.h>
#include "rwnx_rx.h"
#include <common/bk_include.h>
#include "os/str.h"
#include "bk_pm_internal_api.h"
#include <driver/gpio.h>
#include "cif_wifi_dp.h"
#include "cif_cntrl.h"
#include "cif_ps.h"
#include "rw_msdu.h"
#include "cif_mem_mgmt.h"

#define CIF_TAG "CIF"
#define CIF_LOGI(...)       BK_LOGI(CIF_TAG, ##__VA_ARGS__)
#define CIF_LOGW(...)       BK_LOGW(CIF_TAG, ##__VA_ARGS__)
#define CIF_LOGE(...)       BK_LOGE(CIF_TAG, ##__VA_ARGS__)
#define CIF_LOGD(...)       BK_LOGD(CIF_TAG, ##__VA_ARGS__)
#define CIF_LOGV(...)       BK_LOGV(CIF_TAG, ##__VA_ARGS__)
#define CIF_LOG_RAW(...)    BK_LOG_RAW(CIF_TAG, ##__VA_ARGS__)

#define CIF_STATS_INC(x) do{uint32_t int_level = 0;int_level = rtos_disable_int();++cif_stats_ptr->x;rtos_enable_int(int_level);BK_ASSERT(cif_stats_ptr->x >= 0);}while(0)
#define CIF_STATS_DEC(x) do{uint32_t int_level = 0;int_level = rtos_disable_int();--cif_stats_ptr->x;rtos_enable_int(int_level);BK_ASSERT(cif_stats_ptr->x >= 0);}while(0)

#define CNTRL_IF_QUEUE_LEN                          128
#define CNTRL_IF_TASK_PRIO                          2

#define TASK_BASE_INDEX                             12
#define TASK_BK                                     (TASK_BASE_INDEX << 10)
#define AP_SSID_BUF_MAX                             33
#define AP_PSWD_BUF_MAX                             64
#define NETIF_IP4_STR_LEN                           16

#define CPDU_LENGTH sizeof(struct cpdu_t)
#define MAX_MSDU_LENGTH 1500+14//(MTU + Ethernet header)

#define CIF_ALIGN_BYTES (4)
#if CIF_ALIGN_BYTES
#define CIF_ALIGN_LENGTH(x) ((x+(CIF_ALIGN_BYTES-1))&(~(CIF_ALIGN_BYTES-1)))
#else
#define CIF_ALIGN_LENGTH(x) (x)
#endif

#define MAX_TX_DATA_LENGTH CIF_ALIGN_LENGTH(CPDU_LENGTH + MAX_MSDU_LENGTH)
#define MAX_RX_DATA_LENGTH CIF_ALIGN_LENGTH(CPDU_LENGTH + MAX_MSDU_LENGTH)

#define INIT_NUM_IN_CTRLIF 10
#define MAX_NUM_IN_CTRLIF 30
#define MAX_NUM_TX_BUFFERS ((CONFIG_LWIP_MEM_MAX_TX_SIZE/MAX_TX_DATA_LENGTH) * 95 / 100) // 5% buffer reserved for local tcpip stack
#define MAX_NUM_RX_BUFFERS 60

#define MAX_NUM_CMD_SHORT_BUF 3
#define MAX_NUM_CMD_LONG_BUF 1

#define PATTERN_BUSY 0xCAFEBABE
#define PATTERN_FREE 0xF3EEF3EE
#define EVENT_HEAD_LEN 4

#define BK_RX_MSG_HDR_LEN                               offset(struct bk_rx_msg_hdr, param)
#define CIF_MAX_SCAN_AP_CNT_TO_HOST                     48
#define CIF_MAX_CFM_DATA_LEN                            2560
#define CIF_MAX_CFM_SHORT_LEN                           800
#define CIF_MAX_CFM_SHORT_PAYLOAD_LEN                   (CIF_MAX_CFM_SHORT_LEN - sizeof(cpdu_t) - EVENT_HEAD_LEN -16)

#define BK_MAX_MSG_CNT                      (0x800)
#define BK_CMD_CFM_OFFSET                   (0x8000)
#define BK_PRIVATE_MSG_OFFSET               BK_MAX_MSG_CNT
#define BK_CMD_GET_CFM_ID(cmd_id)           (cmd_id + BK_CMD_CFM_OFFSET)
#define BK_CFM_GET_CMD_ID(cfm_id)           (cfm_id - BK_CMD_CFM_OFFSET)


#define CIF_IRQ_DISABLE(int_level) do { int_level = rtos_disable_int(); } while(0)
#define CIF_IRQ_ENABLE(int_level)  do { rtos_enable_int(int_level); } while(0)

/* cmd-table from host app to controller */
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
    BK_CMD_SET_AUTOCONNECT     = 0xA,
    BK_CMD_SET_MEDIA_MODE      = 0xB,
    BK_CMD_SET_MEDIA_QUALITY   = 0xC,
    BK_CMD_SET_COEX_CSA        = 0xF,

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

    BK_CMD_BUTT                = BK_MAX_MSG_CNT - 1
};

/* event-table from controller to host app */
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

    BK_EVT_BUTT                 = BK_MAX_MSG_CNT - 1
};
/* cmd-table from app to netdrv */
enum BK_PRIVATE_CMD_TYPE
{
    BK_PRIVATE_CMD_HOST_KEEP_ALIVE_REQ     = BK_PRIVATE_MSG_OFFSET + 1, //0x801
    BK_PRIVATE_CMD_GET_VNET_SETTING_REQ    = BK_PRIVATE_MSG_OFFSET + 2, //0x802
};
/* cmd-table from app to netdrv */
enum BK_PRIVATE_EVENT_TYPE
{
    BK_PRIVATE_EVT_XX     = BK_PRIVATE_MSG_OFFSET + 1, //0x801
};

enum cif_task_msg_evt
{
    CIF_TASK_MSG_CMD        = 0,
    CIF_TASK_MSG_DATA       = 1,
    CIF_TASK_MSG_EVT        = 2,
    CIF_TASK_MSG_RX_DATA    = 3,//RX data list and TXC list
};
/* Controller Wi-Fi AP Mode State */
enum WLAN_LINK_AP_STATUS
{
    CONTROLLER_AP_START,                /* Controller AP Mode Started */
    CONTROLLER_AP_CLOSE,                /* Controller AP Mode Closed  */
};

enum wakeup_host_type
{
    CIF_WAKEUP_HOST_TYPE_WIFI,
    CIF_WAKEUP_HOST_TYPE_GPIO,
    CIF_WAKEUP_HOST_TYPE_RTC,
    CIF_WAKEUP_HOST_TYPE_SW,
};
enum wakeup_host_wifi_subtype
{
    CIF_WAKEUP_HOST_WIFI_DISCONNECT,
};

struct cif_msg {
  uint32_t arg;
  uint16_t type;
  uint8_t retry_flag;
};

struct cif_stats
{
    //TX buffer numbers in data path
    int16_t buf_in_txdata;
    //RX buffer numbers in ctrlif data
    int16_t cif_rx_data;//CONFIG_CONTROLLER_RX_DIRECT_PSH=0
    //If rx malloc fail, will increase
    uint16_t rx_drop_cnt;//CONFIG_CONTROLLER_RX_DIRECT_PSH=0

    uint32_t cif_tx_dnld_cnt;
    uint32_t cif_tx_cnt;
    uint32_t cif_txc_cnt;

    uint32_t cif_rxc_cnt;
    uint32_t cif_rx_cnt;


    uint32_t ipc_tx_cnt;
    uint32_t ipc_txc_cnt;
    uint32_t ipc_tx_fail_cnt;

    uint32_t cif_msg_snder_fail;
    uint32_t cif_tx_buf_leak;
};
struct cif_rx_filter_t
{
    uint32_t src_ip;
    uint32_t src_port;
};

enum buffer_direction
{
    BUFFER_TX = 0,
    BUFFER_RX = 1,
    BUFFER_MAX
};
enum data_path_special_type
{
    RX_FILTER_TYPE = 1,
    RX_MONITOR_TYPE = 2,
    SPECIAL_DATA_TYPE_MAX
};
struct common_header
{
    uint16_t length;//sdio whole buffer length(include common header)
    uint8_t type:4;
    uint8_t dst_index:4;//station index connected to bk softap
    uint8_t need_free:1;//tx data addr flag, this addr need be freed.
    uint8_t is_buf_bank:1;
    uint8_t vif_idx:2;
    uint8_t special_type:3;
    uint8_t rsve:1;
};

typedef struct
{
    //Common Header
    uint32_t next;
    struct common_header co_hdr;

    //IPC ADDR bank
    uint16_t length;
    uint8_t  dir:1;//(0:TX,1:RX)
    uint8_t  mem_status:1;//(0:free,1:done)
    uint8_t  rsve:6;
    uint8_t  num;
    uint32   addr[1];
}cif_addr_bank_t;

typedef struct cpdu_t
{
    struct cpdu_t* next;
    struct common_header co_hdr;
}cpdu_t;

struct bk_msg_hdr
{
    uint32_t rsv0;
    uint16_t cmd_id;
    uint16_t cmd_sn;
    uint16_t rsv1;
    uint16_t len;//msg payload length
};
/// Temporarily rx cmd structure
struct bk_rx_msg_hdr
{
    uint16_t id;                ///< Message id.
    uint16_t cfm_sn;
    uint16_t rsv;
    uint16_t param_len;         ///< Parameter embedded struct length.
    uint32_t pattern;           ///< Used to stamp a valid MSG buffer
    uint32_t param[1];        ///< Parameter embedded struct. Must be word-aligned.
};
struct ctrl_cmd_hdr
{
    struct cpdu_t* next;
    struct common_header co_hdr;
    struct bk_rx_msg_hdr msg_hdr;
};
struct bk_msg_common
{
    int8_t status;
};
struct bk_msg_start_ap_req
{
    uint8_t band;
    uint8_t ssid[AP_SSID_BUF_MAX];
    uint8_t pw[AP_PSWD_BUF_MAX];
    uint8_t channel;
    uint8_t hidden;
};
struct bk_msg_ap_status_cfm
{
    uint8_t  status;
    uint32_t ip;
    uint32_t gw;
    uint32_t mk;
};
struct bk_msg_ap_assoc_sta_ind
{
    uint8_t sub_sta_addr[6];
};
struct bk_msg_connect_ind
{
    uint8_t ussid[AP_SSID_BUF_MAX];
    uint8_t  rssi;
    uint32_t  ip;
    uint32_t  mk;
    uint32_t  gw;
    uint32_t  dns;
    uint8_t status;
};
struct bk_msg_connect_req
{
    uint8_t ssid[AP_SSID_BUF_MAX];
    uint8_t pw[AP_PSWD_BUF_MAX];
};

struct bk_msg_wlan_status_cfm
{
    uint8_t status;
    uint8_t rssi;
    uint8_t ussid[AP_SSID_BUF_MAX];
    char ip[NETIF_IP4_STR_LEN];
    char mask[NETIF_IP4_STR_LEN];
    char gateway[NETIF_IP4_STR_LEN];
    char dns[NETIF_IP4_STR_LEN];
};
struct bk_msg_net_info_req
{
    char ip[NETIF_IP4_STR_LEN];
    char mask[NETIF_IP4_STR_LEN];
    char gw[NETIF_IP4_STR_LEN];
    char dns[NETIF_IP4_STR_LEN];
};

#define OTA_BUF_MAX_LEN 1024
struct bk_msg_ota_req
{
    int offset;
    uint16_t size;
    int finish;
    char ota_data[OTA_BUF_MAX_LEN];
};

struct bk_msg_scan_wifi_result_ind
{
    uint8_t  scan_num;
    int8_t      rssi;
    uint8_t  bssid[6];
    uint8_t  ssid[AP_SSID_BUF_MAX];
    uint32_t akm;
    uint32_t channel;
};
struct bk_msg_keepalive_cfg_req
{
    uint8_t      infotype;
    char         server[32];
    uint16_t     port;
    uint8_t      devId[32];
    uint8_t      idLen;
    uint8_t      key[64];
    uint8_t      keyLen;
};

struct cif_rx_bank_t
{
    uint32_t rx_buf_bank[MAX_NUM_RX_BUFFERS];
    uint16_t rx_buf_bank_cnt;
};

struct bk_msg_scan_start_req
{
    uint8_t ssid[AP_SSID_BUF_MAX];
};

struct bk_msg_get_ip_config_req
{
    uint8_t flag;
};


typedef int(*cif_customer_msg_cb_t)(struct bk_msg_hdr *msg);
struct cif_env_t
{
    bool host_wifi_init;
    bool host_powerup;
    // indicate controller only
    bool no_host;
    beken2_timer_t enter_lv_timer;
    void *io_queue;
    void *handle;
    uint8 cur_ps_state;
    uint8 pre_ps_state;
    bool cif_sleeping;
    pm_sleep_mode_e cif_sleep_mode;
    struct cif_stats stats;
    event_handler_t keepalive_handler;
    struct cif_rx_filter_t filter;
    cif_customer_msg_cb_t customer_msg_cb;

    //For mem mgmt
    uint32_t cmd_addr[MAX_NUM_CMD_LONG_BUF];
    uint32_t cmd_addr_short[MAX_NUM_CMD_SHORT_BUF];

    //For store rx buf bank
    struct cif_rx_bank_t rx_bank;
};

extern struct cif_env_t cif_env;
extern struct cif_stats * cif_stats_ptr;

extern bk_err_t cif_init();

extern uint8_t cif_dnld_buffer(void *param, void *node);
bk_err_t cif_rxbuf_push(uint8_t channel,void* head,void* tail,uint8_t num);
bk_err_t cif_rxdata_pre_process(uint8_t channel,void* head,uint8_t need_retry);
void cif_rx_data_complete(void *param, void *ack_buf);
void cif_rx_evt_complete(void *param, void *ack_buf);
bk_err_t cif_msg_sender(void* head,enum cif_task_msg_evt type,uint8_t retry);
void cif_register_customer_msg_handler(cif_customer_msg_cb_t func);
void cif_print_debug_info();
#ifdef __cplusplus
}
#endif
