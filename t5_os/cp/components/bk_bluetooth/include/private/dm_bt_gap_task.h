#ifndef __DM_BT_GAP_TASK_H__
#define __DM_BT_GAP_TASK_H__

#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "dm_bluetooth_task.h"

enum
{
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_ENABLE = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_GAP_API_REQ),
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_SCAN_ENABLE,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_INQUIRY,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_INQUIRY_CANCLE,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_REMOTE_SERVICES,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_REMOTE_SERVICE_RECODE,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_COD,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_COD,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_READ_RSSI,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PIN,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_PIN_REPLY,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_READ_REMOTE_NAME,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_AFH_CHANNELS,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SSP_PASSKEY_REPLY,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SSP_CONFIRM_REPLY,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SECURITY_PARAM,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_CONNECTION,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_DISCONNECTION,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_CONN_CANCEL,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PAGE_TIMEOUT,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PAGE_SCAN_ACTIVITY,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_AUTHENTICATION_REQUEST,
    BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_AUTO_SNIFF_POLICY,
};


typedef struct
{
    uint32_t   lap;
    uint16_t   len;
    uint8_t    num_response;
} bt_inquiry_msg_t;

typedef struct
{
    bd_addr_t addr;
    bk_bt_uuid_t uuid;
} remote_service_record_t;

typedef struct
{
    bk_bt_pin_type_t pin_type;
    uint8_t pin[16];
    uint8_t pin_length;
} bk_bt_gap_set_pin_t;

typedef struct
{
    bd_addr_t addr;
    bool accept;
    uint8_t pin[16];
    uint8_t pin_length;
} bk_bt_gap_pin_reply_t;

typedef struct
{
    bk_bd_addr_t bd_addr;
    bool accept;
    uint32_t passkey;
} ssp_passkey_reply_t;

typedef struct
{
    bk_bd_addr_t bd_addr;
    bool accept;
} ssp_confirm_reply_t;

typedef struct
{
    bk_bt_sp_param_t param_type;
    uint8_t value[16];
    uint8_t len;
} set_security_param_t;

typedef struct
{
    uint8_t addr[6];
    uint16_t packet_type;
    uint8_t page_scan_repetition_mode;
    uint8_t page_scan_mode;
    uint16_t clock_offset;
    uint8_t allow_role_switch;
} bk_bt_connection_msg_t;

typedef struct
{
    uint8_t addr[6];
    uint8_t reason;
} bk_bt_disconnection_msg_t;

typedef struct
{
    uint8_t addr[6];
} bt_conn_cancel_msg_t;

typedef struct
{
    uint16_t interval;
    uint16_t window;
} bt_page_scan_activity_msg_t;

typedef struct
{
    uint8_t addr[6];
    uint32_t sec;
    uint16_t sniff_max_interval;
    uint16_t sniff_min_interval;
    uint16_t sniff_attempt;
    uint16_t sniff_timeout;
} bk_bt_set_auto_sniff_policy_msg_t;



void bk_bt_call_callback(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param);
bk_bt_gap_cb_t bk_bt_gap_get_callback(void);
void bk_bt_gap_set_callback(bk_bt_gap_cb_t cb);
uint32_t bk_bt_gap_get_class_of_device();
void bk_bt_gap_set_class_of_device(uint32_t cod);
bt_err_t bk_bt_gap_get_remote_service_record_private(bk_bd_addr_t remote_bda, bk_bt_uuid_t *uuid);

#endif //__DM_BT_GAP_TASK_H__

