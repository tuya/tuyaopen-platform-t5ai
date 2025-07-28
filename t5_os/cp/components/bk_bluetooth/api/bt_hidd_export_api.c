#include "dm_bt_hidd_task.h"
#include "components/bluetooth/bk_dm_hidd.h"
#include "os/mem.h"


bk_err_t bk_bt_hid_device_register_callback(bk_hd_cb_t callback)
{
    BK_BT_HIDD_EN_CHECK();
    if (callback == NULL)
    {
        return BK_FAIL;
    }
    else
    {
        bk_bt_hidd_set_internal_callback(callback);
    }
    return BK_OK;
}

bk_err_t bk_bt_hid_device_init(void)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_INIT, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_deinit(void)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_DEINIT, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_register_app(bk_hidd_app_param_t *app_param)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_REGISTER_APP, app_param, sizeof(bk_hidd_app_param_t), NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_unregister_app(void)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_UNREGISTER_APP, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_connect(bk_bd_addr_t bd_addr)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_CONNECT, bd_addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_disconnect(void)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_DISCONNECT, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_send_report(uint16_t len, uint8_t *data)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    hidd_send_report_t send_report = {0};
    send_report.len = len;
    send_report.data = data;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_SEND_REPORT, &send_report, sizeof(hidd_send_report_t), NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_send_response(uint8_t message_type, uint8_t res_status, uint16_t len, uint8_t *data)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    hidd_send_response_t send_response = {0};
    send_response.message_type = message_type;
    send_response.status = res_status;
    send_response.len = len;
    send_response.data = data;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_SEND_RESPONSE, &send_response, sizeof(hidd_send_response_t), NULL);
    return ret;
}

bk_err_t bk_bt_hid_device_virtual_cable_unplug(void)
{
    BK_BT_HIDD_EN_CHECK();
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_HIDD_API_REQ, BT_ETHERMIND_HIDD_API_REQ_SUBMSG_VIRTUAL_CABLE_UNPLUG, NULL, 0, NULL);
    return ret;
}
