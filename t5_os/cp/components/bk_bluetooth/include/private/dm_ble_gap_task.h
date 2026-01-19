#ifndef __DM_BLE_GAP_TASK_H__
#define __DM_BLE_GAP_TASK_H__

#include "dm_bluetooth_task.h"

#define MAX_ADV_LEN 251
#define MAX_ADV_NUM 0X3F
#define MAX_PER_ADV_LEN 252
#define HCI_PER_ADV_DATA_FRAG_MAX_LEN    252
#define ADV_DATA_LEN        0x1F

enum
{
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_START = BLUETOOTH_API_GROUP(BLE_ETHERMIND_MSG_GAP_API_REQ),
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_PARAMS,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_DATA,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_RSP_DATA,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_ENABLE,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_PARAMS,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_ENABLE,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_UPDATE_CONNECTION_PARAMS,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_RANDOM_ADDR,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_DATA_LENGTH,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PRIVACY_MODE,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_UPDATE_WHILELIST,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CLEAR_WHILELIST,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_GET_WHILELIST_SIZE,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_LOCAL_NAME,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_GET_LOCAL_NAME,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_LOCAL_ADDR,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_RSSI,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_SET_RANDOM_ADDR,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_CREATE_SYNC,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_SYNC_CANCEL,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_SYNC_TERMINATE,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_ADD_DEV_TO_LIST,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_REMOVE_DEV_FROM_LIST,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_CLEAR_DEV,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_START,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_STOP,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_PARAMS,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_REMOVE_ADV_SET,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CLEAR_ADV_SET,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PREFER_CONNECT_PARAM,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_PHY,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PREFERRED_DEFAULT_PHY,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PREFERRED_PHY,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_CHANNELS,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_DISCONNECT,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SECURITY_PARAM,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PAIR_REPLY,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_KEY_REPLY,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_BOND,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_BOND_DEV_LIST_OP,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CONNECT,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CONNECT_CANCEL,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SEC_REQ_RSP,
    BLE_ETHERMIND_GAP_API_REQ_SUBMSG_GENERATE_RPA,
};

typedef enum
{
    SCAN_ENABLE_CMD,
    ADV_DATA_CMD,
    SCAN_RSP_DATA_CMD,
    ADV_ENABLE_CMD,
    PERIODIC_ADV_DATA_CMD,
    PERIODIC_ADV_ENABLE_CMD,
    SECURITY_PARAM_CMD,
} bk_ble_cmd_type_t;

typedef struct
{
    uint8_t   adv_handle;
    uint16_t  adv_event_properties;
    uint32_t  primary_adv_interval_min;
    uint32_t  primary_adv_interval_max;
    uint8_t   primary_adv_channel_map;
    uint8_t   own_addr_type;
    uint8_t   peer_addr_type;
    bd_addr_t peer_addr;
    uint8_t   adv_filter_policy;
    int8_t    adv_tx_power;
    uint8_t   primary_adv_phy;
    uint8_t   secondary_adv_max_skip;
    uint8_t   secondary_adv_phy;
    uint8_t   adv_set_id;
    uint8_t   scan_req_nfy_enable;
} ble_gap_adv_params_t;

typedef struct
{
    uint8_t   adv_handle;
    uint8_t   operation;
    uint8_t   frag_pref;
    uint8_t   adv_data_len;
    uint8_t   adv_data[MAX_ADV_LEN];
} ble_gap_adv_data_t;

typedef struct
{
    uint8_t   adv_handle;
    uint8_t   operation;
    uint8_t   frag_pref;
    uint8_t   scan_response_data_len;
    uint8_t   scan_response_data[MAX_ADV_LEN];
} ble_gap_scan_rsp_data_t;

typedef struct
{
    uint8_t    enable;
    uint8_t    number_of_sets;
    uint8_t    adv_handle[MAX_ADV_NUM];
    uint16_t   duration[MAX_ADV_NUM];
    uint8_t    max_extd_adv_evts[MAX_ADV_NUM];
} ble_gap_adv_enable_t;

typedef struct
{
    uint8_t   enable;
    uint8_t   filter_duplicates;
    uint16_t  duration;
    uint16_t  period;
} ble_gap_scan_enable_t;

typedef struct
{
    uint8_t    own_addr_type;
    uint8_t    scanning_filter_policy;
    uint8_t    scanning_phy;
    uint8_t    scan_type[2];
    uint16_t   scan_interval[2];
    uint16_t   scan_window[2];
} ble_gap_scan_params_ex_t;

typedef struct
{
    uint8_t    peer_addr_type;
    bd_addr_t  peer_addr;
    uint16_t   tx_data_len;
    uint16_t   tx_time;
} ble_gap_pkt_data_len_t;

typedef struct
{
    uint8_t    peer_addr_type;
    bd_addr_t  peer_addr;
    uint8_t    privacy_enable;
} ble_gap_privacy_mode_t;

typedef struct
{
    uint8_t    peer_addr_type;
    bd_addr_t peer_addr;
    uint8_t update_mode;
} ble_gap_update_whitelist_t;

typedef struct
{
    uint8_t    peer_addr_type;
    bd_addr_t peer_addr;
} ble_gap_read_adv_report_t;

typedef struct
{
    uint32_t type;
    uint8_t *data;
    uint32_t len;
} ble_gap_security_param_t;


typedef struct
{
    bd_addr_t peer_addr;
    uint8_t method; //0 number compare 1 passkey
    uint8_t accept;
    uint32_t passkey;
} ble_gap_pair_reply_t;

typedef struct
{
    bd_addr_t peer_addr;
    bk_ble_key_type_t type;
    bk_ble_key_value_t key;
    uint8_t accept;
} ble_gap_key_reply_t;

typedef struct
{
    uint8_t filter_policy;
    uint8_t sid;
    uint8_t addr_type;
    bd_addr_t addr;
    uint16_t skip;
    uint16_t sync_timeout;
    uint8_t sync_cte_type;
} ble_gap_periodic_adv_create_sync_t;

typedef struct
{
    uint8_t addr_type;
    bd_addr_t addr;
    uint8_t sid;
} ble_gap_periodic_adv_dev_t;

typedef struct
{
    uint8_t adv_handler;
    uint8_t enable;
} ble_gap_periodic_adv_status_t;

typedef struct
{
    uint8_t adv_handler;
    uint8_t operation;
    uint16_t len;
    uint8_t data[MAX_PER_ADV_LEN];
} ble_gap_periodic_adv_data_t;

typedef struct
{
    uint8_t adv_handler;
    uint16_t interval_min;
    uint16_t interval_max;
    uint8_t  properties;
} ble_gap_periodic_adv_params_t;

typedef struct
{
    uint8_t adv_handler;
    uint8_t addr_type;
    bd_addr_t addr;
} ble_gap_adv_set_rand_addr_t;

typedef struct
{
    uint8_t own_addr_type;
    uint8_t peer_addr_type;
    bd_addr_t peer_addr;
    uint8_t init_filr_policy;
    uint8_t phy_mask;
    bk_ble_gap_conn_params_t phy_1m_conn_params;
    bk_ble_gap_conn_params_t phy_2m_conn_params;
    bk_ble_gap_conn_params_t phy_coded_conn_params;
} ble_gap_conn_params_t;

typedef struct
{
    uint8_t peer_addr_type;
    bd_addr_t peer_addr;
    uint8_t allphy;
    uint8_t txphy;
    uint8_t rxphy;
    uint16_t phy_options;
} ble_gap_preferred_phy_t;

typedef struct
{
    uint8_t peer_addr_type;
    bd_addr_t peer_addr;
    uint8_t allphy;
    uint8_t txphy;
    uint8_t rxphy;
    uint16_t phy_options;
} ble_gap_read_phy_t;

typedef struct
{
    uint16_t opcode;

    uint8_t adv_handle;
    uint8_t subevent;

    uint8_t initiator_filter_policy;
    uint8_t local_addr_type;
    bd_addr_t peer_addr;
    uint8_t peer_addr_type;
    uint8_t init_phy;

    struct
    {
        uint16_t scan_interval;
        uint16_t scan_window;
        uint16_t conn_interval_min;
        uint16_t conn_interval_max;
        uint16_t conn_latency;
        uint16_t supervision_timeout;
        uint16_t min_ce;
        uint16_t max_ce;
    } *param_array;

} ble_gap_create_connect_t;

typedef struct
{
    bd_addr_t addr;
    uint8_t addr_type;
} ble_gap_disconnect_t;

typedef struct
{
    bd_addr_t peer_addr;
} ble_gap_create_bond_t;

typedef struct
{
    bk_ble_bond_dev_t info;
    uint8_t action; // 0 add; 1 remove; 2 clean
} ble_gap_bond_dev_list_t;

typedef struct
{
    bd_addr_t peer_addr;
    uint8_t accept;
} ble_gap_sec_req_rsp_t;

typedef struct
{
    uint8_t irk[16];
} ble_gap_generate_rpa_t;

enum
{
    CMD_TYPE_ADV_NORMAL,
    CMD_TYPE_ADV_RAW,
    CMD_TYPE_SCAN_RSP_NORMAL,
    CMD_TYPE_SCAN_RSP_RAW,
};

ble_err_t set_cmd_type(int type, int cmd_type);
bk_ble_gap_cb_t bk_ble_gap_get_callback(void);
void bk_ble_gap_set_callback(bk_ble_gap_cb_t cb);
uint16_t bk_ble_gap_get_whitelist_avail_size_private(void);;
const char *bk_ble_gap_get_local_name_private(void);
bk_ble_scan_duplicate_t bk_ble_gap_get_scan_duplicate(void);
void bk_ble_gap_set_scan_duplicate(bk_ble_scan_duplicate_t scan_duplicate);
#endif //__DM_BLE_GAP_TASK_H__
