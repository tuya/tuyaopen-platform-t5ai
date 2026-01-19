#include "dm_bt_hidh_task.h"
#include "components/bluetooth/bk_dm_hidh.h"

#include "os/mem.h"

bk_err_t bk_bt_hid_host_register_callback(bk_hh_cb_t callback)
{
    bk_bt_hidh_set_callback(callback);

    return BK_OK;
}

bk_err_t bk_bt_hid_host_init(void)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_INIT, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_deinit(void)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_DEINIT, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_connect(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_CONNECT, bd_addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_disconnect(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_DISCONNECT, bd_addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_virtual_cable_unplug(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_VIRTUAL_CABLE_UNPLUG, bd_addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_set_info(bk_bd_addr_t bd_addr, bk_hidh_hid_info_t *hid_info)
{
    ble_err_t ret = BK_OK;
    hidh_set_info_t set_info = {0};
    os_memcpy(&set_info.hid_info, hid_info, sizeof(bk_hidh_hid_info_t));
    os_memcpy(set_info.addr, bd_addr, sizeof(bk_bd_addr_t));

    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_INFO, &set_info, sizeof(set_info), NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_get_protocol(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_GET_PROTOCOL, bd_addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_set_protocol(bk_bd_addr_t bd_addr, bk_hidh_protocol_mode_t protocol_mode)
{
    ble_err_t ret = BK_OK;
    hidh_set_protocol_t set_protocol = {0};
    os_memcpy(set_protocol.addr, bd_addr, 6);
    set_protocol.protocol = protocol_mode;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_PROTOCOL, &set_protocol, sizeof(set_protocol), NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_get_report(bk_bd_addr_t bd_addr, bk_hidh_report_type_t report_type, uint8_t report_id,
                                   int buffer_size)
{
    ble_err_t ret = BK_OK;
    hidh_get_report_t get_report = {0};
    os_memcpy(get_report.addr, bd_addr, 6);
    get_report.report_type = report_type;
    get_report.report_id = report_id;
    get_report.buffer_size = buffer_size;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_GET_REPORT, &get_report, sizeof(get_report), NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_set_report(bk_bd_addr_t bd_addr, bk_hidh_report_type_t report_type, uint8_t *report,
                                   size_t len)
{
    ble_err_t ret = BK_OK;
    hidh_set_report_t set_report = {0};
    os_memcpy(set_report.addr, bd_addr, 6);
    os_memcpy(set_report.report, report, len);
    set_report.report_type = report_type;
    set_report.report_len = len;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_REPORT, &set_report, sizeof(set_report), NULL);
    return ret;
}

bk_err_t bk_bt_hid_host_send_data(bk_bd_addr_t bd_addr, uint8_t *data, size_t len)
{
    ble_err_t ret = BK_OK;
    hidh_set_report_t set_report = {0};
    os_memcpy(set_report.addr, bd_addr, 6);
    os_memcpy(set_report.report, data, len);
    set_report.report_len = len;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SET_DATA, &set_report, sizeof(set_report), NULL);
    return ret;
}
bk_err_t bk_bt_hid_host_sdp_dscp(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDH_API_REQ, BT_ETHERMIND_HIDH_API_REQ_SUBMSG_SDP_DESC, bd_addr, 6, NULL);
    return ret;
}

