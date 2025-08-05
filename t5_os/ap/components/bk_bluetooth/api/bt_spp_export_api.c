#include "dm_bt_spp_task.h"
#include "os/mem.h"

bt_err_t bk_spp_register_callback(bk_spp_cb_t callback)
{
    if (callback == NULL)
    {
        return BT_FAIL;
    }
    else
    {
        bt_spp_register_internall_callback(callback);
    }
    return BT_OK;
}

bt_err_t bk_spp_init()
{
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_INIT, NULL, 0, NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_deinit(void)
{
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_DEINIT, NULL, 0, NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_start_discovery(bk_bd_addr_t bd_addr)
{
    bt_ethermind_spp_msg_t msg = {0};
    os_memcpy(msg.start_disc.remote_addr, bd_addr, BK_BD_ADDR_LEN);
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_START_DISCOVERY, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_connect(bk_spp_role_t role, uint8_t remote_scn, bk_bd_addr_t peer_bd_addr)
{
    bt_ethermind_spp_msg_t msg = {0};
    os_memcpy(msg.conn.remote_addr, peer_bd_addr, BK_BD_ADDR_LEN);
    msg.conn.role = role;
    msg.conn.remote_scn = remote_scn;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_CONN, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_disconnect(uint32_t handle)
{
    bt_ethermind_spp_msg_t msg = {0};
    msg.disconn.handle = handle;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_DISCONN, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);

}

bt_err_t bk_spp_start_srv(bk_spp_role_t role, uint8_t local_scn, const char *name)
{
    bt_ethermind_spp_msg_t msg = {0};
    msg.start_srv.role = role;
    msg.start_srv.local_scn = local_scn;
    msg.start_srv.name = name;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_START_SRV, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_stop_srv(void)
{
    bt_ethermind_spp_msg_t msg = {0};
    msg.stop_srv_scn.scn = 0;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_STOP_SRV, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_stop_srv_scn(uint8_t scn)
{
    bt_ethermind_spp_msg_t msg = {0};
    msg.stop_srv_scn.scn = scn;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_STOP_SRV_SCN, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}

bt_err_t bk_spp_write(uint32_t handle, int len, uint8_t *p_data)
{
    bt_ethermind_spp_msg_t msg = {0};
    msg.write.handle = handle;
    msg.write.len = len;
    msg.write.p_data = p_data;
    return (bt_ethermind_post_msg(BT_ETHERMIND_MSG_SPP_REQ, BT_ETHERMIND_API_REQ_SUBMSG_SPP_WIRTE, &msg, sizeof(bt_ethermind_spp_msg_t), NULL) == BK_OK ? BT_OK : BT_FAIL);
}



