#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "ble_boarding.h"
#include "doorbell_comm.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "cli.h"

#include "customer_msg.h"


#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "db-core"


#define ADV_MAX_SIZE (251)
#define ADV_NAME_HEAD "doorbell"

#define ADV_TYPE_FLAGS                      (0x01)
#define ADV_TYPE_LOCAL_NAME                 (0x09)
#define ADV_TYPE_SERVICE_UUIDS_16BIT        (0x14)
#define ADV_TYPE_SERVICE_DATA               (0x16)
#define ADV_TYPE_MANUFACTURER_SPECIFIC      (0xFF)

#define BEKEN_COMPANY_ID                    (0x05F0)

#define BOARDING_UUID                       (0xFE01)

#define OPCODE_LEN                      (2)
#define DATA_LEN_LEN                    (2)
#define SSID_LEN_LEN                    (2)
#define PW_LEN_LEN                      (2)


typedef struct
{
    uint32_t enabled : 1;
    uint32_t service : 6;

    char *id;
    beken_thread_t thd;
    beken_queue_t queue;
} doorbell_info_t;

typedef struct
{
    ble_boarding_info_t boarding_info;
    uint16_t channel;
} doorbell_boarding_info_t;

doorbell_info_t *db_info = NULL;
static doorbell_boarding_info_t *doorbell_boarding_info = NULL;

static void doorbell_boarding_operation_handle(uint16_t opcode, uint16_t length, uint8_t *data)
{
    LOGE("%s, opcode: %04X, length: %u\n", __func__, opcode, length);
    doorbell_msg_t msg = {0};
    uint8_t *cmd_data = NULL;
    uint16_t cmd_len = 0;

    switch (opcode)
    {
        case BOARDING_OP_STATION_START:
        {
        /**
        *  0 - 1  | 2 - 3 | 4 - 5 | N1 - Nx | Nx+1 - Nx+2 | Nx+3 - Nx+n |
        * --------|-------|-------|---------|-------------|-------------|
        *  opcode |  len  |ssidlen|   ssid  |    pwlen    |   password  |
        */
            cmd_len = OPCODE_LEN + DATA_LEN_LEN 
                    + SSID_LEN_LEN + doorbell_boarding_info->boarding_info.ssid_length
                    + PW_LEN_LEN + doorbell_boarding_info->boarding_info.password_length;

            cmd_data = (uint8_t*)os_malloc(cmd_len);
            if (!cmd_data)
            {
                LOGI("%s, malloc failed\n", __func__);
                return;
            }

            uint8_t *p_data = cmd_data;

            UINT16_TO_STREAM(p_data, opcode);
            UINT16_TO_STREAM(p_data, (cmd_len - OPCODE_LEN - DATA_LEN_LEN));
            UINT16_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.ssid_length);
            ARRAY_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.ssid_value, doorbell_boarding_info->boarding_info.ssid_length);
            UINT16_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.password_length);
            ARRAY_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.password_value, doorbell_boarding_info->boarding_info.password_length);
        }
        break;

        case BOARDING_OP_SOFT_AP_START:
        {
        /**
        *  0 - 1  | 2 - 3 | 4 - 5 | N1 - Nx | Nx+1 - Nx+2 | Nx+3 - Nx+n | Nx+n+1 - Nx+n+2 |
        * --------|-------|-------|---------|-------------|-------------|---------------- |
        *  opcode |  len  |ssidlen|   ssid  |    pwlen    |   password  |      channel    |
        */
            cmd_len = OPCODE_LEN + DATA_LEN_LEN 
                    + SSID_LEN_LEN + doorbell_boarding_info->boarding_info.ssid_length
                    + PW_LEN_LEN + doorbell_boarding_info->boarding_info.password_length
                    + sizeof(doorbell_boarding_info->channel);

            cmd_data = (uint8_t*)os_malloc(cmd_len);
            if (!cmd_data)
            {
                LOGI("%s, malloc failed\n", __func__);
                return;
            }

            uint8_t *p_data = cmd_data;

            UINT16_TO_STREAM(p_data, opcode);
            UINT16_TO_STREAM(p_data, (cmd_len - OPCODE_LEN - DATA_LEN_LEN));
            UINT16_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.ssid_length);
            ARRAY_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.ssid_value, doorbell_boarding_info->boarding_info.ssid_length);
            UINT16_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.password_length);
            ARRAY_TO_STREAM(p_data, doorbell_boarding_info->boarding_info.password_value, doorbell_boarding_info->boarding_info.password_length);
            UINT16_TO_STREAM(p_data, doorbell_boarding_info->channel);
        }
        break;

        case BOARDING_OP_SET_WIFI_CHANNEL:
        {
            STREAM_TO_UINT16(doorbell_boarding_info->channel, data);

            LOGI("%s, BOARDING_OP_SET_WIFI_CHANNEL: %u\n", __func__, doorbell_boarding_info->channel);
            return;
        }
        break;

        case BOARDING_OP_BLE_DISABLE:
        {
            msg.event = DBEVT_CLOSE_BLUETOOTH;
            msg.data = NULL;
            msg.len = 0;
            doorbell_send_msg(&msg);
            return;
        }
        break;

        default:
        {
        /**
        *  0 - 1  | 2 - 3 | N1 - Nx |
        * --------|-------|---------|
        *  opcode |  len  |   data  |
        */
            cmd_len = OPCODE_LEN + DATA_LEN_LEN + length;

            cmd_data = (uint8_t*)os_malloc(cmd_len);
            if (!cmd_data)
            {
                LOGI("%s, malloc failed\n", __func__);
                return;
            }

            uint8_t *p_data = cmd_data;

            UINT16_TO_STREAM(p_data, opcode);
            UINT16_TO_STREAM(p_data, length);
            if (length != 0)
                ARRAY_TO_STREAM(p_data, data, length);
        }
        break;
    }

    msg.event = DBEVT_DATA_TO_USER;
    msg.data = cmd_data;
    msg.len = cmd_len;
    doorbell_send_msg(&msg);
}


int doorbell_boarding_init(void)
{
    uint8_t adv_data[ADV_MAX_SIZE] = {0};
    uint8_t adv_index = 0;
    uint8_t len_index = 0;
    uint8_t mac[6];
    int ret;

    LOGI("%s\n", __func__);

    /* flags */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_FLAGS;
    adv_data[adv_index++] = 0x06;
    adv_data[len_index] = 2;

    /* local name */
    bk_bluetooth_get_address(mac);

    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

    ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X",
               ADV_NAME_HEAD, mac[0], mac[1], mac[2]);

    adv_index += ret;
    adv_data[len_index] = ret + 1;

    /* 16bit uuid */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
    adv_data[adv_index++] = BOARDING_UUID & 0xFF;
    adv_data[adv_index++] = BOARDING_UUID >> 8;
    adv_data[len_index] = 3;

    /* manufacturer */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
    adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
    adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
    adv_data[len_index] = 3;

    /*
    os_printf("adv data:\n");

    int i = 0;
    for (i = 0; i < adv_index; i++)
    {
     os_printf("%02X ", adv_data[i]);
    }

    os_printf("\n");
    */

    if (doorbell_boarding_info == NULL)
    {
     doorbell_boarding_info = os_malloc(sizeof(doorbell_boarding_info_t));

     if (doorbell_boarding_info == NULL)
     {
         LOGE("doorbell_boarding_info malloc failed\n");

         goto error;
     }

     os_memset(doorbell_boarding_info, 0, sizeof(doorbell_boarding_info_t));
    }

    doorbell_boarding_info->boarding_info.cb = doorbell_boarding_operation_handle;

    ble_boarding_init(&doorbell_boarding_info->boarding_info);
    ble_boarding_adv_start(adv_data, adv_index);

    return BK_OK;
     error:
    return BK_FAIL;
}

bk_err_t doorbell_send_msg(doorbell_msg_t *msg)
{
    bk_err_t ret = BK_OK;

    if (db_info->queue)
    {
        ret = rtos_push_to_queue(&db_info->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

static void doorbell_message_handle(void)
{
    bk_err_t ret = BK_OK;
    doorbell_msg_t msg;

    while (1)
    {

        ret = rtos_pop_from_queue(&db_info->queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case DBEVT_DATA_TO_USER:
                {
                    LOGI("DBEVT_DATA_TO_USER\n");
                    if (msg.data && msg.len)
                    {
                        cifd_send_cust_event(CIFD_EVENT_BLE_DATA_TO_USER, msg.data, msg.len);
                        os_free(msg.data);
                    }
                }
                break;

                case DBEVT_DATA_TO_APK:
                {
                    LOGI("DBEVT_DATA_TO_APK\n");
                    if (msg.data && msg.len)
                    {
                        ble_boarding_notify(msg.data, msg.len);
                        os_free(msg.data);
                    }
                }
                break;

                case DBEVT_CLOSE_BLUETOOTH:
                {
                    LOGI("DBEVT_CLOSE_BLUETOOTH\n");
                    bk_bluetooth_deinit();
                }
                break;

                case DBEVT_EXIT:
                    goto exit;
                    break;

                default:
                    break;
            }
        }
    }

exit:


    /* delate msg queue */
    ret = rtos_deinit_queue(&db_info->queue);

    if (ret != kNoErr)
    {
        LOGE("delete message queue fail\n");
    }

    db_info->queue = NULL;

    LOGE("delete message queue complete\n");

    /* delate task */
    rtos_delete_thread(NULL);

    db_info->thd = NULL;

    LOGE("delete task complete\n");
}


void doorbell_core_init(void)
{
    bk_err_t ret = BK_OK;

    if (db_info == NULL)
    {
        db_info = os_malloc(sizeof(doorbell_info_t));

        if (db_info == NULL)
        {
            LOGE("%s, malloc db_info failed\n", __func__);
            goto error;
        }

        os_memset(db_info, 0, sizeof(doorbell_info_t));
    }


    if (db_info->queue != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->queue allready init, exit!\n", __func__);
        goto error;
    }

    if (db_info->thd != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->thd allready init, exit!\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&db_info->queue,
                          "db_info->queue",
                          sizeof(doorbell_msg_t),
                          30);

    if (ret != BK_OK)
    {
        LOGE("%s, ceate doorbell message queue failed\n", __func__);
        goto error;
    }

    ret = rtos_create_thread(&db_info->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "db_info->thd",
                             (beken_thread_function_t)doorbell_message_handle,
                             1024 * 2,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create media major thread fail\n");
        goto error;
    }

    doorbell_boarding_init();

    db_info->enabled = BK_TRUE;

    LOGE("%s success\n", __func__);

    return;

error:

    LOGE("%s fail\n", __func__);
}

