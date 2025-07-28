#ifndef __BT_IPC_CORE_H__
#define __BT_IPC_CORE_H__

#include <driver/mailbox_channel.h>

enum {
    BT_IPC_STATE_IDLE,
    BT_IPC_STATE_READY,
};

#define BT_IPC_QUEUE_LEN      64
#define BT_IPC_TASK_PRIO       4

#define BT_INIT_VENDOR_SUB_OPCODE 0x0001
#define BT_DEINIT_VENDOR_SUB_OPCODE 0x0002
#define BT_EVENT_STATUS_NOERROR 0x00

typedef struct
{
    uint8_t type;
    uint32_t param;
} bt_ipc_msg_t;

typedef struct __attribute__((packed))
{
    uint16_t opcode;
    uint8_t param_len;
    uint8_t param[];
}cmd_hdr_t;

typedef struct __attribute__((packed))
{
    uint8_t event_code;
    uint8_t param_len;
    uint8_t param[];
}event_hdr_t;

typedef struct __attribute__((packed))
{
    mb_chnl_hdr_t hdr;
    uint8_t pkt_type;
#if 0
    union
    {
        cmd_hdr_t *cmd_hdr;
        event_hdr_t *event_hdr;
    };
#else
    uint32_t hdr_ptr;
#endif
} hci_hdr_t;

typedef union
{
    hci_hdr_t hci_hdr;
    mb_chnl_cmd_t mb_cmd;
} bt_ipc_cmd_t;

enum
{
    HCI_COMMAND_PKT = 0x1,//A core
    HCI_ACL_DATA_PKT = 0x2,
    HCI_SCO_DATA_PKT = 0x3,
    HCI_EVENT_PKT = 0x4, //M core
    HCI_FREE_PKT = 0xa,
};


void bt_ipc_init(void);
void bt_ipc_hci_send_vendor_event(uint8_t *data, uint16_t len);
void bt_ipc_hci_send_vendor_cmd(uint8_t *data, uint16_t len);
void bk_bluetooth_init_deinit_compelete();
#endif
