#pragma once

typedef enum
{
    BLE_ETHERMIND_MSG_INVAILD,
    BLE_ETHERMIND_MSG_REPORT_EVT,
    BLE_ETHERMIND_MSG_API_REQ, // for old api
    BLE_ETHERMIND_MSG_GAP_API_REQ,
    BLE_ETHERMIND_MSG_GATT_COMMON_API_REQ,
    BLE_ETHERMIND_MSG_GATTC_API_REQ,
    BLE_ETHERMIND_MSG_GATTS_API_REQ,
    BLE_ETHERMIND_MSG_SELF_REQ_EVT, //for self

    BT_ETHERMIND_MSG_REPORT_EVT,
    BT_ETHERMIND_MSG_API_REQ,// for old api
    BT_ETHERMIND_MSG_SELF_REQ_EVT, //for self
    BT_ETHERMIND_MSG_L2CAP_REQ,
    BT_ETHERMIND_MSG_GAP_API_REQ,
    BT_ETHERMIND_MSG_A2DP_API_REQ,
    BT_ETHERMIND_MSG_SPP_REQ,
    BT_ETHERMIND_MSG_HIDD_API_REQ,
    BT_ETHERMIND_MSG_HIDH_API_REQ,
    BT_ETHERMIND_MSG_HFP_API_REQ,
    BT_ETHERMIND_MSG_REQ_MAX = 0xFF, // do not define after this.
} BLUETOOTH_ETHERMIND_MSG_ENUM;

#define BLUETOOTH_API_GROUP_SHIFT   (8)
#define BLUETOOTH_API_GROUP_MASK    (~((1ul << BLUETOOTH_API_GROUP_SHIFT) - 1))
#define BLUETOOTH_API_OP_MASK       ((1ul << BLUETOOTH_API_GROUP_SHIFT) - 1)
#define BLUETOOTH_API_GROUP(x)      (((uint32_t)(x)) << BLUETOOTH_API_GROUP_SHIFT)

typedef int (*req_api_handle_t)(void *param, uint32_t len, void *callback);

typedef struct
{
    uint32_t submsg;
    req_api_handle_t handle;
} ble_ethermind_post_msg_req_api_handle_t;


typedef ble_ethermind_post_msg_req_api_handle_t bt_ethermind_post_msg_req_api_handle_t; //same as ble_ethermind_post_msg_req_api_handle_t

typedef struct
{
    uint32_t msg_id;
    uint32_t sub_msg_id;
    uint16_t len;
    void *data;
    void *cb;
} BLUETOOTH_ETHERMIND_MSG_T;

int32_t ble_ethermind_post_msg(uint32_t msg_id, uint32_t sub_msg_id, void *data, uint32_t len, void *cb);
int32_t bt_ethermind_post_msg(uint32_t msg_id, uint32_t sub_msg_id, void *data, uint32_t len, void *cb);
