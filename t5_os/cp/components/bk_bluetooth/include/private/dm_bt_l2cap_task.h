#ifndef __DM_BT_L2CAP_TASK_H__
#define __DM_BT_L2CAP_TASK_H__

#include "dm_bluetooth_task.h"
#include "components/bluetooth/bk_dm_l2cap.h"

enum
{
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_INIT = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_L2CAP_REQ),
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_DEINIT,
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_CONNECT,
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_START_SRV,
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_STOP_SRV,
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_STOP_ALL_SRV,
    BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_CLOSE,
};

typedef struct
{
    uint16_t sec_mask;
    uint16_t remote_psm;
    uint8_t peer_addr[6];
} bt_ethermind_l2cap_conn_msg_t;

typedef struct
{
    uint16_t sec_mask;
    uint16_t local_psm;
} bt_ethermind_l2cap_start_srv_msg_t;

typedef struct
{
    uint16_t local_psm;
} bt_ethermind_l2cap_stop_srv_msg_t;

typedef struct
{
    uint16_t lcid;
} bt_ethermind_l2cap_close_msg_t;

#define BT_L2CAP_INVALID_PSM 0x00

void bt_l2cap_register_interall_callback(bk_bt_l2cap_cb_t callback);
void bt_l2cap_register_data_callback(bk_bt_l2cap_data_cb_t callback);
int bt_l2cap_write(int fd, const void *data, uint16_t size);
int bt_l2cap_read(int fd, void *data, uint16_t size);
#endif //__DM_BT_L2CAP_TASK_H__ 