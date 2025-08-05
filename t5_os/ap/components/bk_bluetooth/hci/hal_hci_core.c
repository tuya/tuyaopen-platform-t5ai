#include <stdint.h>
#include <string.h>
#include "hal_hci_core.h"
#include <os/mem.h>
#include <os/str.h>
#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "bt_ipc_core.h"

#if CONFIG_BT
static bt_err_t dual_hci_data_to_cp_cb(uint8_t type, uint8_t *buf, uint16_t len)
{
    switch (type) {
        case HCI_CMD_TYPE:
        {
            cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)buf;
            bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
        }
        break;
        case HCI_ACL_TYPE:
        {
            acl_hdr_t *acl_hdr = (acl_hdr_t *)buf;
            bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
        }
        break;
        case HCI_SYNC_TYPE:
        {
            sco_hdr_t *sco_hdr = (sco_hdr_t *)buf;
            bt_ipc_hci_send_sco_data(sco_hdr->conhdl_psf, sco_hdr->param, sco_hdr->datalen);
        }
        break;
        default:
            HCI_LOGW("unknown type (%d)\n", type);
        break;
    }
    return 0;
}
#else
static ble_err_t ble_hci_data_to_cp_cb(uint8_t *buf, uint16_t len)
{
    uint8_t type = buf[0];
    switch (type) {
        case HCI_CMD_TYPE:
        {
            cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(buf + 1);
            bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
        }
        break;
        case HCI_ACL_TYPE:
        {
            acl_hdr_t *acl_hdr = (acl_hdr_t *)(buf + 1);
            bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
        }
        break;
        default:
            HCI_LOGW("unknown type (%d)", type);
        break;
    }
    return 0;
}
#endif

static void hal_hci_driver_send(uint8_t *buf, uint16_t len)
{
    //HCI_LOGD("%s, type %d,len %d\r\n",__func__, buf[0],len);
#if CONFIG_BT
    bk_dual_hci_send_to_host(buf, len);
#else
    bk_ble_hci_send_to_host(buf, len);
#endif
}

int hal_hci_driver_open(void)
{
    int ret;
#if CONFIG_BT
    ret = bk_dual_host_register_hci_callback(dual_hci_data_to_cp_cb);
#else
    ret = bk_ble_host_register_hci_callback(ble_hci_data_to_cp_cb);
#endif
    bt_ipc_register_hci_send_callback(hal_hci_driver_send);
    return ret;
}

int hal_hci_driver_close(void)
{
    int ret;

    bt_ipc_register_hci_send_callback(NULL);
#if CONFIG_BT
    ret = bk_dual_host_register_hci_callback(NULL);
#else
    ret = bk_ble_host_register_hci_callback(NULL);
#endif
    return ret;
}


