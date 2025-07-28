#include "os/mem.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

#include "dm_bt_gap_task.h"

bk_err_t bk_bt_gap_register_callback(bk_bt_gap_cb_t callback)
{
    bk_bt_gap_set_callback(callback);
    return BK_OK;
}

bk_err_t bk_bt_gap_set_scan_mode(bk_bt_conn_mode_t c_mode, bk_bt_disc_mode_t d_mode)
{
    ble_err_t ret = BK_OK;
    uint8_t scan_enable = 0;
    if (c_mode)
    {
        scan_enable |= 0x02;
    }
    if (d_mode)
    {
        scan_enable |= 0x01;
    }
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_ENABLE, &scan_enable, 1, NULL);
    return ret;
}

bk_err_t bk_bt_gap_get_scan_mode()
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_SCAN_ENABLE, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_gap_start_discovery(bk_bt_inq_mode_t mode, uint8_t inq_len, uint8_t num_rsps)
{
    ble_err_t ret = BK_OK;
    bt_inquiry_msg_t tmp = {0};
    if (mode == BK_BT_INQ_MODE_GENERAL_INQUIRY)
    {
        tmp.lap = 0x9e8B33U;
    }
    else
    {
        tmp.lap = 0x9e8B33U;
    }
    tmp.len = inq_len;
    if (inq_len > BK_BT_GAP_MAX_INQ_LEN || inq_len < BK_BT_GAP_MIN_INQ_LEN)
    {
        return BK_ERR_PARAM;
    }
    tmp.num_response = num_rsps;

    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_INQUIRY, &tmp, sizeof(tmp), NULL);
    return ret;
}

bk_err_t bk_bt_gap_cancel_discovery(void)
{
    ble_err_t ret = BK_OK;

    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_INQUIRY_CANCLE, NULL, 0, NULL);
    return ret;
}

bk_err_t bk_bt_gap_get_remote_services(bk_bd_addr_t remote_bda)
{
    ble_err_t ret = BK_OK;
    bd_addr_t addr = {0};
    os_memcpy(addr.addr, remote_bda, 6);
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_REMOTE_SERVICES, &addr, 6, NULL);
    return ret;
}

bk_err_t bk_bt_gap_get_remote_service_record(bk_bd_addr_t remote_bda, bk_bt_uuid_t *uuid)
{
    ble_err_t ret = BK_OK;
    ret = bk_bt_gap_get_remote_service_record_private(remote_bda, uuid);

    return ret;
}

bk_err_t bk_bt_gap_config_eir_data(bk_bt_eir_data_t *eir_data)
{
    ble_err_t ret = BK_OK;
    //  ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_GET_REMOTE_SERVICES, eir_data, sizeof(bk_bt_eir_data_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_set_cod(bk_bt_cod_t cod, bk_bt_cod_mode_t mode)
{
    ble_err_t ret = BK_OK;
    uint32_t class_of_device = bk_bt_gap_get_class_of_device();
    if (mode == BK_BT_INIT_COD)
    {
        bk_bt_gap_set_class_of_device(cod.cod);
        class_of_device = cod.cod;
    }
    else if (mode == BK_BT_SET_COD_MAJOR_MINOR)
    {
        class_of_device &= ~0x1FFC;
        class_of_device |= (cod.cod & 0x1FFC);
    }
    else if (mode == BK_BT_SET_COD_SERVICE_CLASS)
    {
        class_of_device &= ~0xFFE0000;
        class_of_device |= (cod.cod & 0xFFE0000);
    }
    else if (mode == BK_BT_CLR_COD_SERVICE_CLASS)
    {
        class_of_device &= ~0xFFE0000;
    }
    else
    {
        class_of_device |= (cod.cod & 0xFFFFFFC);
    }
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_COD, &class_of_device, sizeof(uint32_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_read_rssi_delta(bk_bd_addr_t remote_addr)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_READ_RSSI, remote_addr, sizeof(bk_bd_addr_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_remove_bond_device(bk_bd_addr_t bd_addr)
{
    return BK_OK;
}

int bk_bt_gap_get_bond_device_num(void)
{
    return BK_OK;
}

bk_err_t bk_bt_gap_get_bond_device_list(int *dev_num, bk_bd_addr_t *dev_list)
{
    return BK_OK;
}

bk_err_t bk_bt_gap_set_pin(bk_bt_pin_type_t pin_type, uint8_t pin_code_len, bk_bt_pin_code_t pin_code)
{
    ble_err_t ret = BK_OK;
    bk_bt_gap_set_pin_t param = {0};
    os_memcpy(param.pin, pin_code, pin_code_len);
    param.pin_type = pin_type;
    param.pin_length = pin_code_len;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PIN, &param, sizeof(bk_bt_gap_set_pin_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_pin_reply(bk_bd_addr_t bd_addr, bool accept, uint8_t pin_code_len, bk_bt_pin_code_t pin_code)
{
    ble_err_t ret = BK_OK;
    bk_bt_gap_pin_reply_t param = {0};
    os_memcpy(param.addr.addr, bd_addr, 6);
    os_memcpy(param.pin, pin_code, pin_code_len);
    param.pin_length = pin_code_len;
    param.accept = accept;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_PIN_REPLY, &param, sizeof(bk_bt_gap_pin_reply_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_set_security_param(bk_bt_sp_param_t param_type,
                                      void *value, uint8_t len)
{
    ble_err_t ret = BK_OK;
    set_security_param_t param = {0};
    os_memcpy(param.value, value, len);
    param.param_type = param_type;
    param.len = len;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SECURITY_PARAM, &param, sizeof(ssp_passkey_reply_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_ssp_passkey_reply(bk_bd_addr_t bd_addr, bool accept, uint32_t passkey)
{
    ble_err_t ret = BK_OK;
    ssp_passkey_reply_t reply = {0};
    os_memcpy(reply.bd_addr, bd_addr, 6);
    reply.accept = accept;
    reply.passkey = passkey;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SSP_PASSKEY_REPLY, &reply, sizeof(ssp_passkey_reply_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_ssp_confirm_reply(bk_bd_addr_t bd_addr, bool accept)
{
    ble_err_t ret = BK_OK;
    ssp_confirm_reply_t reply = {0};
    os_memcpy(reply.bd_addr, bd_addr, 6);
    reply.accept = accept;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SSP_CONFIRM_REPLY, &reply, sizeof(ssp_confirm_reply_t), NULL);
    return ret;
}

bk_err_t bk_bt_gap_set_afh_channels(bk_bt_gap_afh_channels channels)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_AFH_CHANNELS, channels, sizeof(bk_bt_gap_afh_channels), NULL);
    return ret;
}

bk_err_t bk_bt_gap_read_remote_name(bk_bd_addr_t remote_bda)
{
    ble_err_t ret = BK_OK;
    ret = bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_READ_REMOTE_NAME, remote_bda, sizeof(bk_bd_addr_t), NULL);
    return ret;
}


#define CONNECTION_PACKET_TYPE 0xcc18
#define CONNECTION_PAGE_SCAN_REPETITIOIN_MODE 0x01
#define CONNECTION_CLOCK_OFFSET 0x00

bt_err_t bk_bt_gap_connect(bk_bd_addr_t addr, uint8_t allow_role_switch)
{
    bt_err_t ret = BK_ERR_BLE_SUCCESS;
    bk_bt_connection_msg_t msg = {0};
    os_memcpy(msg.addr, addr, BK_BT_ADDR_LEN);
    msg.packet_type = CONNECTION_PACKET_TYPE;
    msg.page_scan_repetition_mode = CONNECTION_PAGE_SCAN_REPETITIOIN_MODE;
    msg.page_scan_mode = 0;
    msg.clock_offset = CONNECTION_CLOCK_OFFSET;
    msg.allow_role_switch = allow_role_switch;
    ret =  bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_CONNECTION, &msg, sizeof(msg), NULL);
    return ret;
}

bt_err_t bk_bt_gap_disconnect(bk_bd_addr_t addr, uint8_t reason)
{
    bt_err_t ret = BK_ERR_BLE_SUCCESS;
    bk_bt_disconnection_msg_t msg = {0};
    os_memcpy(msg.addr, addr, BK_BT_ADDR_LEN);
    msg.reason = reason;
    ret =  bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_DISCONNECTION, &msg, sizeof(msg), NULL);
    return ret;
}

bt_err_t bk_bt_gap_create_conn_cancel(uint8_t *addr)
{
    bt_conn_cancel_msg_t msg = {0};
    os_memcpy(msg.addr, addr, BK_BT_ADDR_LEN);
    return bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_CONN_CANCEL, &msg, sizeof(msg), NULL);
}


bt_err_t bk_bt_gap_set_page_timeout(uint16_t timeout)
{
    return bt_ethermind_post_msg(BT_ETHERMIND_MSG_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PAGE_TIMEOUT, &timeout, sizeof(timeout), NULL);
}

bt_err_t bk_bt_gap_set_page_scan_activity(uint16_t interval, uint16_t window)
{
    bt_page_scan_activity_msg_t msg = {0};
    msg.interval = interval;
    msg.window = window;

    return bt_ethermind_post_msg(BT_ETHERMIND_MSG_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PAGE_SCAN_ACTIVITY, &msg, sizeof(msg), NULL);
}

bt_err_t bk_bt_gap_authentication_request(uint8_t *addr)
{
    return bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_AUTHENTICATION_REQUEST, addr, 6, NULL);
}

bk_err_t bk_bt_gap_set_auto_sniff_policy(bk_bd_addr_t remote_bda, uint32_t sec, bk_bt_gap_sniff_config *config)
{
    bk_bt_set_auto_sniff_policy_msg_t msg;

    os_memset(&msg, 0, sizeof(msg));
    os_memcpy(msg.addr, remote_bda, BK_BT_ADDR_LEN);

    msg.sec = sec;
    if (config)
    {
        msg.sniff_max_interval = config->sniff_max_interval;
        msg.sniff_min_interval = config->sniff_min_interval;
        msg.sniff_attempt = config->sniff_attempt;
        msg.sniff_timeout = config->sniff_timeout;
    }

    return bt_ethermind_post_msg(BT_ETHERMIND_MSG_GAP_API_REQ, BT_ETHERMIND_GAP_API_REQ_SUBMSG_SET_AUTO_SNIFF_POLICY, &msg, sizeof(msg), NULL);
}

