#include "customer_msg.h"
#include "bk_ef.h"
#include <components/log.h>
#include "cif_main.h"
#include "doorbell_comm.h"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "cust"


int cifd_send_cust_event(uint16_t event_id, uint8_t *data, uint16_t len)
{
    int ret = 0;
    CIFD_CUST_DATA event = {0};

    if (len > MAX_CIFD_CUST_SIZE)
    {
        LOGI(TAG, "cifd_send_customer_event error: len[%d] is greater than max_len[%d]\n", len, MAX_CIFD_CUST_SIZE);
        return -1;
    }

    LOGI(TAG, "cifd_send_cust_event event_id:%d len:%d\n", event_id, len);
    event.header.cid = event_id;
    event.header.len = len;
    event.header.magic = CIFD_CUST_PATTERN;

    if (data && len)
        os_memcpy(event.data, data, len);

    ret = cif_send_customer_event((uint8_t *)&event, sizeof(CIFD_PROTO_HDR) + len);
    return ret;
}

int cifd_cust_msg_handler(struct bk_msg_hdr *msg)
{
    int ret = 0;
    uint8_t *cfm_data = NULL;
    uint16_t cfm_data_len = 0;
    cifd_cust_msg_hdr_t *cifd_cmd_hdr = NULL;

    if (msg == NULL)
    {
        LOGI(TAG, "%s data invalid\n",__func__);
        return -1;
    }

    cifd_cmd_hdr = (cifd_cust_msg_hdr_t *)(msg + 1);
    LOGI(TAG, "%s cmd id:0x%x\n",__func__, cifd_cmd_hdr->cmd_id);
    switch (cifd_cmd_hdr->cmd_id)
    {
        case CIFD_CMD_BLE_DATA_TO_APK:
        {
            LOGI(TAG, "add ble data \n");

            doorbell_msg_t msg = {0};
            uint8_t *cmd_data = NULL;

            cmd_data = (uint8_t*)os_malloc(cifd_cmd_hdr->len);
            if (cmd_data)
            {
                os_memcpy(cmd_data, cifd_cmd_hdr->payload, cifd_cmd_hdr->len);
                msg.event = DBEVT_DATA_TO_APK;
                msg.data = cmd_data;
                msg.len = cifd_cmd_hdr->len;
                doorbell_send_msg(&msg);
            }
            else
            {
                LOGE(TAG, " %s malloc failed \n",__func__);
            }
            break;
        }
        default:
            break;
    }

    if (cfm_data && cfm_data_len)
    {
        ret = cif_send_customer_cmd_cfm(cfm_data, cfm_data_len, msg);
    }
    else
    {
        ret = cif_send_customer_cmd_cfm(NULL, 0, msg);
    }

    return ret;
}


void cifd_cust_msg_init(void)
{
    LOGI(TAG, "%s\n",__func__);
    cif_register_customer_msg_handler(cifd_cust_msg_handler);
}
void cifd_cust_msg_deinit(void)
{
    LOGI(TAG, "%s\n",__func__);
}

