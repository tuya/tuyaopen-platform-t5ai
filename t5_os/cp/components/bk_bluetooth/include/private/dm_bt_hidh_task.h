#ifndef __DM_BT_HIDH_TASK_H__
#define __DM_BT_HIDH_TASK_H__

#include "components/bluetooth/bk_dm_hidh.h"
#include "dm_bluetooth_task.h"


#define BT_HID_MAX_REPORT_LEN 25

enum
{
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_INIT = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_HIDH_API_REQ),
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_DEINIT,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_CONNECT,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_DISCONNECT,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_VIRTUAL_CABLE_UNPLUG,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_INFO,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_GET_PROTOCOL,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_PROTOCOL,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_GET_REPORT,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_REPORT,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_DATA,
    BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SDP_DESC,
};


typedef struct
{
    bk_bd_addr_t addr;
    uint8_t addr_type;
    bk_hidh_hid_info_t hid_info;
} hidh_set_info_t;

typedef struct
{
    bk_bd_addr_t addr;
    uint8_t addr_type;
    uint8_t protocol;
} hidh_set_protocol_t;

typedef struct
{
    bk_bd_addr_t addr;
    uint8_t addr_type;
    uint8_t report_type;
    uint8_t report_id;
    int buffer_size;
} hidh_get_report_t;

typedef struct
{
    bk_bd_addr_t addr;
    uint8_t addr_type;
    uint8_t *report;
    uint16_t report_len;
    uint8_t report_type;
} hidh_set_report_t;

void bk_bt_hidh_set_callback(bk_hh_cb_t cb);

#endif