#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#include "bt_ipc_core.h"
#include "cli.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG  "bt_ipc"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

static void bk_bluetooth_send_init_deinit_status(uint16_t opcode, uint8_t status);

typedef struct 
{
    uint8_t state;
    beken_thread_t thd;
    beken_queue_t queue;
    beken_semaphore_t send_sema;
}bt_ipc_t;


bt_ipc_t bt_ipc_env = {
    .state = BT_IPC_STATE_IDLE,
    .thd = NULL,
    .queue = NULL,
    .send_sema = NULL,
};

#define BT_IPC_CMD_CHNL     MB_CHNL_BT_CMD
#define BT_IPC_SEND_TIMEOUT_MS  4000

#define HCI_COMMAND_COMPLETE_EVT_CODE    0x0E
#define HCI_VENDOR_EVT_CODE    0xFE
#define HCI_VENDOR_OPCODE      0xFEFE

enum
{
    BT_IPC_MSG_NULL = 0,
    BT_IPC_CMD_IND_MSG = 1,
    BT_IPC_EVNET_IND_MSG = 2,
    BT_IPC_FREE_MSG = 3,
    BT_IPC_EXIT_MSG = 4,
};

static void bt_ipc_mailbox_rx_isr(void *param, void *cmd_buf)
{
    hci_hdr_t *hci_hdr = (hci_hdr_t *)cmd_buf;
    switch(hci_hdr->pkt_type) {
        case HCI_COMMAND_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_CMD_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGE("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_EVENT_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_EVNET_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGE("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_FREE_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_FREE_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGE("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        default:
            LOGE("%s, unknown type %d\r\n", __func__,hci_hdr->pkt_type);
            break;

    }

}

static void bt_ipc_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
    rtos_set_semaphore(&bt_ipc_env.send_sema);
}

static void bt_ipc_mailbox_send_msg(hci_hdr_t *msg)
{
    if (BT_IPC_STATE_READY != bt_ipc_env.state)
    {
        LOGW("%s bt ipc is not ready!\r\n", __func__);
        return;
    }

    bt_ipc_cmd_t bt_ipc_cmd;
    int ret = BK_OK;

    bt_ipc_cmd.hci_hdr = *msg;

    ret = mb_chnl_write(BT_IPC_CMD_CHNL, (mb_chnl_cmd_t*)&bt_ipc_cmd);
    if (ret != BK_OK)
    {
        LOGE("mb_chnl_write failed\n");
        return;
    }

    ret = rtos_get_semaphore(&bt_ipc_env.send_sema, BT_IPC_SEND_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGE("get bt ipc send_sema failed\n");
    }
}

void bt_ipc_hci_send_vendor_cmd(uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(cmd_hdr_t) + len;
    cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)os_malloc(data_len);

    LOGD("malloc ptr %p\n",cmd_hdr);

    if (cmd_hdr == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }
    cmd_hdr->opcode = HCI_VENDOR_OPCODE;
    cmd_hdr->param_len = len;
    os_memcpy(cmd_hdr->param, data, len);

    msg.pkt_type = HCI_COMMAND_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)cmd_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_vendor_event(uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(event_hdr_t) + len;
    event_hdr_t *event_hdr = (event_hdr_t *)os_malloc(data_len);

    LOGD("malloc ptr %p\n",event_hdr);

    if (event_hdr == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }
    event_hdr->event_code = HCI_VENDOR_EVT_CODE;
    event_hdr->param_len = len;
    os_memcpy(event_hdr->param, data, len);

    msg.pkt_type = HCI_EVENT_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)event_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_complete_event(uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(event_hdr_t) + len;
    event_hdr_t *event_hdr = (event_hdr_t *)os_malloc(data_len);

    LOGD("malloc ptr %p\n",event_hdr);

    if (event_hdr == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }
    event_hdr->event_code = HCI_COMMAND_COMPLETE_EVT_CODE;
    event_hdr->param_len = len;
    os_memcpy(event_hdr->param, data, len);

    msg.pkt_type = HCI_EVENT_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)event_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_free_pkt(uint32_t ptr)
{
    hci_hdr_t msg;

    msg.pkt_type = HCI_FREE_PKT;
    msg.hdr_ptr = ptr;

    bt_ipc_mailbox_send_msg(&msg);
}

static void bt_ipc_mailbox_config(uint8_t channel)
{
    bk_err_t ret;

    LOGD("open channel: %d on CPU\n", channel);
    /* reigster a mailbox logical channel */
    ret = mb_chnl_open(channel, NULL);
    if (ret != BK_OK) {
        LOGE(" mb_chnl_open open fail \r\n");
        return;
    }  
    /* register mailbox logical channel rx callbcak */
    mb_chnl_ctrl(channel, MB_CHNL_SET_RX_ISR, bt_ipc_mailbox_rx_isr);
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_ISR, NULL);
    /* register mailbox logical channel rx complete callback */
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_CMPL_ISR, bt_ipc_mailbox_tx_cmpl_isr);
}

static void bt_ipc_message_handle(void)
{
    bk_err_t ret = BK_OK;
    bt_ipc_msg_t msg;

    while (1)
    {

        ret = rtos_pop_from_queue(&bt_ipc_env.queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.type)
            {
                case BT_IPC_CMD_IND_MSG:
                {
                    LOGD("BT_IPC_CMD_IND_MSG\n");
                    cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(uintptr_t)msg.param;
                    LOGD("opcode 0x%04x, param_len %d\n",cmd_hdr->opcode, cmd_hdr->param_len);
                    if (cmd_hdr->opcode == HCI_VENDOR_OPCODE)
                    {
                        if(cmd_hdr->param_len >= 2)
                        {
                            uint16_t op = (cmd_hdr->param[0]<<8)|(cmd_hdr->param[1]);
                            LOGD("op :0x%04x\n", op);
                            if(op == BT_INIT_VENDOR_SUB_OPCODE)
                            {
                                bk_bluetooth_init();
                                bk_bluetooth_send_init_deinit_status(BT_INIT_VENDOR_SUB_OPCODE, BT_EVENT_STATUS_NOERROR);
                            }else if(op == BT_DEINIT_VENDOR_SUB_OPCODE)
                            {
                                bk_bluetooth_deinit();
                                bk_bluetooth_send_init_deinit_status(BT_DEINIT_VENDOR_SUB_OPCODE, BT_EVENT_STATUS_NOERROR);
                            }
                        }
                    }
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_EVNET_IND_MSG:
                {
                    LOGD("BT_IPC_EVNET_IND_MSG\n");
                    event_hdr_t *event_hdr = (event_hdr_t *)(uintptr_t)msg.param;
                    LOGD("evt_code 0x%02x, param_len %d\n",event_hdr->event_code, event_hdr->param_len);
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_FREE_MSG:
                {
                    LOGD("BT_IPC_FREE_MSG\n");
                    LOGD("free ptr %p\n",msg.param);
                    os_free((void*)(uintptr_t)msg.param);
                }
                break;

                case BT_IPC_EXIT_MSG:
                    goto exit;

                default:
                    break;
            }
        }
    }

exit:

    /* delate msg queue */
    ret = rtos_deinit_queue(&bt_ipc_env.queue);

    if (ret != kNoErr)
    {
        LOGE("delete message queue fail\n");
    }

    bt_ipc_env.queue = NULL;

    LOGE("delete message queue complete\n");

    /* delate task */
    rtos_delete_thread(NULL);

    bt_ipc_env.thd = NULL;

    LOGE("delete task complete\n");
}

void bt_ipc_init(void)
{
    if (BT_IPC_STATE_READY == bt_ipc_env.state)
    {
        LOGW("%s bt ipc already initialised\r\n", __func__);
        return;
    }

    bk_err_t ret;

    ret = rtos_init_queue(&bt_ipc_env.queue,
                          "bt_ipc_queue",
                          sizeof(bt_ipc_msg_t),
                          BT_IPC_QUEUE_LEN);

    if (ret != BK_OK)
    {
        LOGE("%s, create bt ipc queue failed\n", __func__);
        return;
    }

    ret = rtos_create_thread(&bt_ipc_env.thd,
                             BT_IPC_TASK_PRIO,
                             "bt_ipc_thd",
                             (beken_thread_function_t)bt_ipc_message_handle,
                             2048,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create bt ipc thread fail\n");
        return;
    }

    /* register a mailbox channel */
    bt_ipc_mailbox_config(BT_IPC_CMD_CHNL);

    /* init semaphore */
    ret = rtos_init_semaphore(&bt_ipc_env.send_sema, 1);
    if (ret != BK_OK) {
        LOGE("init send_sema fail!\r\n");
    }

    bt_ipc_env.state = BT_IPC_STATE_READY;
    LOGD("%s success\n", __func__);
}

static void bk_bluetooth_send_init_deinit_status(uint16_t opcode, uint8_t status)
{
    uint8_t event_data[4];
    event_data[0] = opcode>>8;
    event_data[1] = opcode&0xff;
    event_data[2] = 1; //Num_HCI_Command_Packets
    event_data[3] = status;
    bt_ipc_hci_send_complete_event(event_data, sizeof(event_data));
}