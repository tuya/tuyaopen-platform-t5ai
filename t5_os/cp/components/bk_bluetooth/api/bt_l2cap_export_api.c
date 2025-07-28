#include "components/bluetooth/bk_dm_l2cap.h"
#include "dm_bluetooth_task.h"
#include "dm_bt_l2cap_task.h"
#include "os/mem.h"

bt_err_t bk_bt_l2cap_register_callback(bk_bt_l2cap_cb_t callback)
{
    if (callback == NULL)
    {
        return BT_FAIL;
    }
    else
    {
        bt_l2cap_register_interall_callback(callback);
    }
    return BT_OK;
}

bt_err_t bk_bt_l2cap_init(void)
{
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_INIT, NULL, 0, NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_bt_l2cap_deinit(void)
{
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_DEINIT, NULL, 0, NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_bt_l2cap_connect(bk_bt_l2cap_cntl_flags_t cntl_flag, uint16_t remote_psm, bk_bd_addr_t peer_bd_addr)
{
    bt_ethermind_l2cap_conn_msg_t msg;
    msg.sec_mask = (cntl_flag & 0xffff);
    msg.remote_psm = remote_psm;
    os_memcpy(msg.peer_addr, peer_bd_addr, BK_BD_ADDR_LEN);
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_CONNECT, &msg, sizeof(msg), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_bt_l2cap_start_srv(bk_bt_l2cap_cntl_flags_t cntl_flag, uint16_t local_psm)
{
    bt_ethermind_l2cap_start_srv_msg_t msg;
    msg.sec_mask = (cntl_flag & 0xffff);
    msg.local_psm = local_psm;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_START_SRV, &msg, sizeof(msg), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_bt_l2cap_stop_all_srv(void)
{
    bt_ethermind_l2cap_stop_srv_msg_t msg;
    msg.local_psm = BT_L2CAP_INVALID_PSM;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_STOP_ALL_SRV, &msg, sizeof(msg), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_bt_l2cap_stop_srv(uint16_t local_psm)
{
    bt_ethermind_l2cap_stop_srv_msg_t msg;
    msg.local_psm = local_psm;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_STOP_SRV, &msg, sizeof(msg), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

int bk_bt_l2cap_read(int fd, void *data, uint16_t size)
{
    return bt_l2cap_read(fd, data, size);
}


int bk_bt_l2cap_write(int fd, const void *data, uint16_t size)
{
    return bt_l2cap_write(fd, data, size);
}

bk_err_t bk_bt_l2cap_set_data_callback(bk_bt_l2cap_data_cb_t callback)
{
    if (callback == NULL)
    {
        return BT_FAIL;
    }
    else
    {
        bt_l2cap_register_data_callback(callback);
    }
    return BT_OK;
}

bk_err_t bk_bt_l2cap_close(int fd)
{
    bt_ethermind_l2cap_close_msg_t msg;
    msg.lcid = (uint16_t)fd;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_L2CAP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_L2CAP_CLOSE, &msg, sizeof(msg), NULL) == BK_OK ? BT_OK : BT_FAIL);
}


