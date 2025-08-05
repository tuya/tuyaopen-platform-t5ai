#include <stdint.h>
#include <string.h>
#include "hal_hci_core.h"
#include <os/mem.h>
#include <os/str.h>
#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_ble.h"
#include "bt_ipc_core.h"


static ble_err_t ble_hci_to_host_evt_cb(uint8_t *buf, uint16_t len)
{
    event_hdr_t *event_hdr = (event_hdr_t *)buf;
    bt_ipc_hci_send_event(event_hdr->event_code, event_hdr->param, event_hdr->param_len);
    return 0;
}

static ble_err_t ble_hci_to_host_acl_cb(uint8_t *buf, uint16_t len)
{
    acl_hdr_t *acl_hdr = (acl_hdr_t *)buf;
    bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
    return 0;
}

static ble_err_t ble_hci_to_host_sco_cb(uint8_t *buf, uint16_t len)
{
    sco_hdr_t *sco_hdr = (sco_hdr_t *)buf;
    bt_ipc_hci_send_sco_data(sco_hdr->conhdl_psf, sco_hdr->param, sco_hdr->datalen);
    return 0;
}

static void hal_hci_driver_send(uint8_t *buf, uint16_t len)
{
    uint8_t type = buf[0];
    //HCI_LOGD("%s, type %d,len %d\r\n",__func__, type,len);

    switch (type) {
        case HCI_CMD_TYPE:
        {
            bk_ble_hci_cmd_to_controller(&buf[1], len - 1);
        }
        break;
        case HCI_ACL_TYPE:
        {
            bk_ble_hci_acl_to_controller(&buf[1], len - 1);
        }
        break;
        case HCI_SYNC_TYPE:
        {
            bk_ble_hci_to_controller(HCI_SYNC_TYPE, &buf[1], len - 1);
        }
        break;
        default:
            HCI_LOGW("unknown type (%d)\n", type);
        break;
    }
}

int hal_hci_driver_open(void)
{
    int ret;

    ret = bk_ble_reg_hci_recv_callback(ble_hci_to_host_evt_cb,ble_hci_to_host_acl_cb);
    bk_ble_reg_sco_hci_recv_callback(ble_hci_to_host_sco_cb);
    bt_ipc_register_hci_send_callback(hal_hci_driver_send);
    return ret;
}

int hal_hci_driver_close(void)
{
    int ret;

    bt_ipc_register_hci_send_callback(NULL);
    ret = bk_ble_reg_hci_recv_callback(NULL,NULL);
    bk_ble_reg_sco_hci_recv_callback(NULL);

    return ret;
}
