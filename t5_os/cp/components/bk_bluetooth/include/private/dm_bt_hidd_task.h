#ifndef __DM_BT_HIDD_TASK_H__
#define __DM_BT_HIDD_TASK_H__

#include "components/bluetooth/bk_dm_hidd.h"
#include "dm_bluetooth_task.h"

#define BK_BT_HIDD_EN_CHECK()               \
    if(!bk_bt_hidd_status_enabled())        \
    {                                       \
        return BK_ERR_BT_CMD_NOT_SUPPORT;   \
    }                                       \

enum
{
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_INIT = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_HIDD_API_REQ),
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_DEINIT,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_REGISTER_APP,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_UNREGISTER_APP,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_CONNECT,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_DISCONNECT,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_SEND_REPORT,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_SEND_RESPONSE,
    BT_ETHERMIND_HIDD_API_REQ_SUBMSG_VIRTUAL_CABLE_UNPLUG,

};

typedef struct
{
    uint8_t id;
    uint16_t len;
    uint8_t *data;
} hidd_send_report_t;

typedef struct
{
    uint8_t message_type;
    uint8_t status;
    uint16_t len;
    uint8_t *data;
} hidd_send_response_t;

typedef struct
{
    bk_hidd_app_param_t app_param;
} hidd_register_app_t;

uint8_t bk_bt_hidd_status_enabled();

void bk_bt_hidd_set_internal_callback(bk_hd_cb_t cb);

#endif


