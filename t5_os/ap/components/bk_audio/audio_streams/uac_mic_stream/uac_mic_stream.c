// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/uac_mic_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <driver/audio_ring_buff.h>
#include <driver/int.h>
#include <components/usbh_hub_multiple_classes_api.h>


#define TAG  "UAC_MIC"


//#define UAC_MIC_DEBUG   //GPIO debug

#ifdef UAC_MIC_DEBUG

#define UAC_MIC_URB_COMPLETE_CALLBACK()                 do { GPIO_DOWN(32); GPIO_UP(32); GPIO_DOWN(32);} while (0)

#define UAC_MIC_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define UAC_MIC_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define UAC_MIC_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define UAC_MIC_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define UAC_MIC_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define UAC_MIC_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define UAC_MIC_URB_COMPLETE_CALLBACK()

#define UAC_MIC_PROCESS_START()
#define UAC_MIC_PROCESS_END()

#define UAC_MIC_INPUT_START()
#define UAC_MIC_INPUT_END()

#define UAC_MIC_OUTPUT_START()
#define UAC_MIC_OUTPUT_END()

#endif


/* uac mic data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count uac mic data. */
#if CONFIG_ADK_UTILS

#define UAC_MIC_DATA_COUNT

#endif  //CONFIG_ADK_UTILS


#ifdef UAC_MIC_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t uac_mic_count_util = {0};
#define UAC_MIC_DATA_COUNT_INTERVAL     (1000 * 4)
#define UAC_MIC_DATA_COUNT_TAG          "UAC_MIC"

#define UAC_MIC_DATA_COUNT_OPEN()               count_util_create(&uac_mic_count_util, UAC_MIC_DATA_COUNT_INTERVAL, UAC_MIC_DATA_COUNT_TAG)
#define UAC_MIC_DATA_COUNT_CLOSE()              count_util_destroy(&uac_mic_count_util)
#define UAC_MIC_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&uac_mic_count_util, size)

#else

#define UAC_MIC_DATA_COUNT_OPEN()
#define UAC_MIC_DATA_COUNT_CLOSE()
#define UAC_MIC_DATA_COUNT_ADD_SIZE(size)

#endif  //UAC_MIC_DATA_COUNT


typedef enum
{
    UAC_MIC_DRV_IDLE = 0,
    UAC_MIC_DRV_EXIT,
    UAC_MIC_DRV_DATA,
    UAC_MIC_DRV_CONT,
    UAC_MIC_DRV_DISCONT,
} uac_mic_drv_op_t;

typedef struct
{
    uac_mic_drv_op_t op;
    void *param;
} uac_mic_drv_msg_t;

/* the audio uac connection status */
typedef enum
{
    UAC_MIC_IDLE = 0,                 /**< Idle: uac mic idle status after normal disconnection */
    UAC_MIC_CONNECTED,                /**< Connected: uac automatically restore connection after abnormal disconnection */
    UAC_MIC_NORMAL_DISCONNECTED,      /**< Normal disconnected: uac normal disconnection */
    UAC_MIC_ABNORMAL_DISCONNECTED,    /**< Abnormal disconnected: uac abnormal disconnection */
    UAC_MIC_CONNECT_FAIL,             /**< Cannot automatically connect: uac automatically connect fail */
} uac_mic_cont_sta_t;


typedef enum
{
    UAC_MIC_STA_DISCONNECT = 0,    /**< Disconnect: uac mic disconnect status */
    UAC_MIC_STA_CONNECT,           /**< Connected: uac mic connect status after connected */
    UAC_MIC_STA_OPEN,              /**< Open: uac mic open status after open */
    UAC_MIC_STA_WORKING,           /**< Working: uac mic working status */

    UAC_MIC_STA_CHECKING,          /**< Checking: uac mic parameters checking status, protect port_info */
} uac_mic_sta_t;

typedef struct uac_mic_stream
{
    E_USB_HUB_PORT_INDEX          port_index;            /**< usb hub port index */
    uint16_t                      format;                /**< uac mic data format */
    uint8_t                       chl_num;               /**< mic channel number */
    uint8_t                       bits;                  /**< Bit wide (8, 16 bits) */
    uint32_t                      samp_rate;             /**< mic sample rate */
    uint32_t                      frame_size;            /**< size of one frame mic data */
    int                           out_block_size;        /**< Size of output block */
    int                           out_block_num;         /**< Number of output block */
    bool                          auto_connect;          /**< Automatic connect enable */

    bool                          is_open;               /**< mic enable, true: enable, false: disable */
    beken_semaphore_t             uac_connect;           /**< uac connect semaphore */
    beken_semaphore_t             can_process;           /**< can process */

    beken_semaphore_t             uac_mic_sem;           /**< uac mic driver task synchron */
    bk_usb_hub_port_info *        mic_port_info;         /**< uac mic port information */
    struct usbh_urb *             uac_mic_urb;           /**< uac mic urb */
    uint32_t                      urb_buff_size;         /**< uac mic urb buffer size(one frame size) */
    uint8_t *                     urb_buff_addr;         /**< uac mic urb buffer address */

    uint16_t                      format_tag;            /**< uac mic device format used for check */
    uint8_t                       samples_frequence_num; /**< uac mic device frequences number used for check */
    uint32_t *                    samples_frequence;     /**< uac mic device frequences used for check */

    int                           task_stack;            /**< Task stack size */
    int                           task_core;             /**< Task running in core (0 or 1) */
    int                           task_prio;             /**< Task priority (based on freeRTOS priority) */
    beken_thread_t                uac_mic_drv_thread;    /**< uac mic driver task handle */
    beken_queue_t                 uac_mic_drv_queue;     /**< uac mic driver task queue */
    RingBufferContext             mic_rb;                /**< uac mic rb handle */
    uint32_t                      mic_ring_buff_size;    /**< uac mic ring buffer address size */
    int8_t *                      mic_ring_buff;         /**< uac mic ring buffer address */
    uac_mic_cont_sta_t            cont_sta;              /**< uac mic connect status */
    uac_mic_sta_t                 status;                /**< uac mic status */
} uac_mic_stream_t;


bk_err_t uac_mic_drv_send_msg(uac_mic_stream_t *uac_mic, uac_mic_drv_op_t op, void *param)
{
    bk_err_t ret;
    uac_mic_drv_msg_t msg;

    msg.op = op;
    msg.param = param;
    if (uac_mic && uac_mic->uac_mic_drv_queue)
    {
        ret = rtos_push_to_queue(&uac_mic->uac_mic_drv_queue, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret)
        {
            BK_LOGE(TAG, "%s, %d, uac_mic_drv_send_msg fail \n", __func__, __LINE__);
            return kOverrunErr;
        }

        return ret;
    }
    return kNoResourcesErr;
}

static bk_err_t uac_mic_disconnect_handle(uac_mic_stream_t *uac_mic)
{
    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    if (!uac_mic)
    {
        BK_LOGE(TAG, "%s, %d, uac_mic is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (uac_mic->cont_sta == UAC_MIC_IDLE && uac_mic->status == UAC_MIC_STA_DISCONNECT)
    {
        if (BK_OK != rtos_set_semaphore(&uac_mic->uac_connect))
        {
            BK_LOGE(TAG, "%s, %d, set uac_connect semaphore fail \n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;
}

#if 0
static void usb_hub_print_port_info(bk_usb_hub_port_info *port_info)
{
//    BK_LOGD(TAG, "%s port_info:0x%x\n", __func__, port_info);
    BK_LOGD(TAG, "%s hport_index:%d\n", __func__, port_info->port_index);
    BK_LOGD(TAG, "%s device_index:%d\n", __func__, port_info->device_index);
//    BK_LOGD(TAG, "%s hport:0x%x\n", __func__, port_info->hport);
//    BK_LOGD(TAG, "%s hport_address:%d\n", __func__, port_info->hport->dev_addr);
//    BK_LOGD(TAG, "%s usb_device:0x%x\n", __func__, port_info->usb_device);
//    BK_LOGD(TAG, "%s interface_num:%d\n", __func__, port_info->interface_num);
//    BK_LOGD(TAG, "%s usb_device_param:0x%x\n", __func__, port_info->usb_device_param);
//    BK_LOGD(TAG, "%s usb_device_param_config:0x%x\n", __func__, port_info->usb_device_param_config);
}
#endif

static void uac_mic_disconnect_cb(bk_usb_hub_port_info *port_info, void *arg)
{
    bk_err_t ret = BK_OK;

    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    //usb_hub_print_port_info(port_info);

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)arg;

    if (port_info->port_index != uac_mic->port_index)
    {
        BK_LOGE(TAG, "%s, %d, port_index: %d is not right \n", __func__, __LINE__, port_info->port_index);
        return;
    }

    /* reset port_info */
    if (port_info->device_index == USB_UAC_MIC_DEVICE)
    {
        /* wait checking complete */
        while (uac_mic->status == UAC_MIC_STA_CHECKING)
        {
            rtos_delay_milliseconds(2);
        }

        uac_mic->status = UAC_MIC_STA_DISCONNECT;
        uac_mic->mic_port_info = NULL;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, device_index: %d is not support \n", __func__, __LINE__, port_info->device_index);
        return;
    }

    if (uac_mic->cont_sta == UAC_MIC_CONNECTED)
    {
        uac_mic->cont_sta = UAC_MIC_ABNORMAL_DISCONNECTED;
    }
    else if (uac_mic->cont_sta == UAC_MIC_NORMAL_DISCONNECTED)
    {
        uac_mic->cont_sta = UAC_MIC_IDLE;
    }
    else
    {
        //
    }

    ret = uac_mic_drv_send_msg(uac_mic, UAC_MIC_DRV_DISCONT, uac_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send msg: UAC_MIC_DRV_DISCONT fail: %d \n", __func__, __LINE__);
    }
}

static void usb_hub_uac_mic_port_dev_complete_callback(void *pCompleteParam, int nbytes)
{
    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)pCompleteParam;

    bk_err_t ret = BK_OK;

    UAC_MIC_URB_COMPLETE_CALLBACK();
    UAC_MIC_DATA_COUNT_ADD_SIZE(nbytes);

    if (uac_mic->status == UAC_MIC_STA_WORKING)
    {
        uac_mic->uac_mic_urb->transfer_buffer = uac_mic->urb_buff_addr;
        uac_mic->uac_mic_urb->transfer_buffer_length = uac_mic->urb_buff_size;
        uac_mic->uac_mic_urb->actual_length = 0;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, uac mic status: %d, not need read uac mic data \n", __func__, __LINE__, uac_mic->status);
        return;
    }

    if (nbytes > 0)
    {
        if (ring_buffer_get_free_size(&uac_mic->mic_rb) >= nbytes)
        {
            ring_buffer_write(&uac_mic->mic_rb, (uint8_t *)uac_mic->uac_mic_urb->transfer_buffer, nbytes);
        }
    }

    ret = rtos_set_semaphore(&uac_mic->can_process);
    if (ret != BK_OK)
    {
        BK_LOGV(TAG, "%s, %d, rtos_set_semaphore fail \n", __func__, __LINE__);
    }

    return;
}

static bk_err_t usb_hub_uac_mic_port_device_urb_fill(uac_mic_stream_t *uac_mic)
{
    struct usbh_audio *mic_device;

    if (!uac_mic)
    {
        BK_LOGE(TAG, "%s, %d, uac_mic is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (uac_mic->uac_mic_urb && uac_mic->mic_port_info)
    {
        mic_device = (struct usbh_audio *)(uac_mic->mic_port_info->usb_device);
        uac_mic->uac_mic_urb->pipe = (usbh_pipe_t)(mic_device->isoin);
        uac_mic->uac_mic_urb->complete = (usbh_complete_callback_t)usb_hub_uac_mic_port_dev_complete_callback;
        uac_mic->uac_mic_urb->arg = (void *)uac_mic;
        uac_mic->uac_mic_urb->timeout = 0;

        uac_mic->uac_mic_urb->transfer_buffer = uac_mic->urb_buff_addr;
        uac_mic->uac_mic_urb->transfer_buffer_length = uac_mic->urb_buff_size;
        uac_mic->uac_mic_urb->num_of_iso_packets = 1;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, parameters is invalid, urb: %p, port_info: %p\n", __func__, __LINE__, uac_mic->uac_mic_urb, uac_mic->mic_port_info);
        return BK_FAIL;
    }

    return BK_OK;
}


static bk_err_t uac_mic_connect_handle(uac_mic_stream_t * uac_mic)
{
    bk_err_t ret = BK_OK;
    uint8_t i = 0;

    /* config uac */
    BK_LOGD(TAG, "%s, %d, config uac mic\n", __func__, __LINE__);

    if (uac_mic->status == UAC_MIC_STA_CONNECT)
    {
        /* check whether format and sample_rate is support */
        if (uac_mic->format != uac_mic->format_tag)
        {
            BK_LOGE(TAG, "%s, %d, format: %d is not support, invalid value: %d \n", __func__, __LINE__, uac_mic->format, uac_mic->format_tag);
            goto fail;
        }
        if (uac_mic->samples_frequence_num)
        {
            for (i = 0; i < uac_mic->samples_frequence_num; i++)
            {
                if (uac_mic->samples_frequence[i] == uac_mic->samp_rate)
                {
                    break;
                }
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, mic not support, mic_samples_frequence_num: %d \n", __func__, __LINE__, uac_mic->samples_frequence_num);
            goto fail;
        }
        if (i >= uac_mic->samples_frequence_num)
        {
            BK_LOGE(TAG, "%s, %d, mic sample_rate: %d not support \n", __func__, __LINE__, uac_mic->samp_rate);
            goto fail;
        }
    }

    if (uac_mic->status == UAC_MIC_STA_CONNECT)
    {
        /* open device */
        bk_uac_mic_config_t *uac_mic_param_config = (bk_uac_mic_config_t *)uac_mic->mic_port_info->usb_device_param_config;
        bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_mic->mic_port_info->usb_device_param;
        uac_mic_param_config->mic_format_tag = uac_mic->format;
        uac_mic_param_config->mic_samples_frequence = uac_mic->samp_rate;
        uac_mic_param_config->mic_ep_desc = uac_device_param->mic_ep_desc;
        ret = bk_usbh_hub_port_dev_open(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic->mic_port_info);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, open uac mic: %d fail, ret: %d \n", __func__, __LINE__, uac_mic->port_index, ret);
            goto fail;
        }
        uac_mic->status = UAC_MIC_STA_OPEN;
    }

    /* Check whether uac mic element is open already.
     * If true, this connection is callback connection, so restart uac mic to get data.
     * If false, this connection is the first connection when open uac mic.
     */
    if (uac_mic->is_open == true)
    {
        BK_LOGD(TAG, "%s, uac restart mic \n", __func__);

        if (uac_mic->status == UAC_MIC_STA_OPEN)
        {
            usb_hub_uac_mic_port_device_urb_fill(uac_mic);
            ret = bk_usbh_hub_dev_request_data(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic->uac_mic_urb);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, start uac mic port: %d, ret: %d \n", __func__, __LINE__, uac_mic->port_index, ret);
                return BK_FAIL;
            }
            else
            {
                uac_mic->status = UAC_MIC_STA_WORKING;
            }
        }
    }
    else
    {
        ret = rtos_set_semaphore(&uac_mic->uac_connect);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, set uac_connect semaphore fail \n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;

fail:

    ret = rtos_set_semaphore(&uac_mic->uac_connect);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, set uac_connect semaphore fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_FAIL;
}


static void uac_mic_connect_cb(bk_usb_hub_port_info *port_info, void *arg)
{
    bk_err_t ret = BK_OK;
    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)arg;

    if (port_info->port_index != uac_mic->port_index)
    {
        BK_LOGE(TAG, "%s, %d, port_index: %d is not right \n", __func__, __LINE__, port_info->port_index);
        return;
    }

    /* save port_info */
    if (port_info->device_index == USB_UAC_MIC_DEVICE)
    {
        uac_mic->status = UAC_MIC_STA_CONNECT;
        uac_mic->mic_port_info = port_info;

#if 0
        bk_uac_mic_config_t *uac_device_param_canfig = (bk_uac_mic_config_t *)port_info->usb_device_param_config;
        struct usb_endpoint_descriptor *mic_ep_desc = (struct usb_endpoint_descriptor *)uac_device_param_canfig->mic_ep_desc;
        BK_LOGD(TAG, "     ------------ Audio Data Mic Endpoint Descriptor -----------  \n");
        BK_LOGD(TAG, "bLength                        : 0x%x (%d bytes)\n", mic_ep_desc->bLength, mic_ep_desc->bLength);
        BK_LOGD(TAG, "bDescriptorType                : 0x%x (Audio Endpoint Descriptor)\n", mic_ep_desc->bDescriptorType);
        BK_LOGD(TAG, "bEndpointAddress               : 0x%x (General)\n", mic_ep_desc->bEndpointAddress);
        BK_LOGD(TAG, "bmAttributes                   : 0x%x\n", mic_ep_desc->bmAttributes);
        BK_LOGD(TAG, "wMaxPacketSize                 : 0x%x\n", mic_ep_desc->wMaxPacketSize);
        BK_LOGD(TAG, "bInterval                      : 0x%x\n", mic_ep_desc->bInterval);
#endif

        /* Copy uac mic device parameters and then check parameters.
         * Avoid device disconnect and parameters ptr was been change to NULL when using parameters ptr.
         */
        if (uac_mic->mic_port_info)
        {
            bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_mic->mic_port_info->usb_device_param;
            if (uac_device_param)
            {
                uac_mic->format_tag = uac_device_param->mic_format_tag;
                uac_mic->samples_frequence_num = uac_device_param->mic_samples_frequence_num;
                if (uac_mic->samples_frequence_num > 0)
                {
                    uac_mic->samples_frequence = audio_calloc(1, sizeof(uint32_t) * uac_mic->samples_frequence_num);
                    if (uac_mic->samples_frequence)
                    {
                        os_memcpy(uac_mic->samples_frequence, uac_device_param->mic_samples_frequence, sizeof(uint32_t) * uac_mic->samples_frequence_num);
                    }
                    else
                    {
                        BK_LOGE(TAG, "%s, %d, malloc samples_frequence fail\n", __func__, __LINE__);
                    }
                }
                else
                {
                    BK_LOGE(TAG, "%s, %d, samples_frequence_num: %d\n", __func__, __LINE__, uac_mic->samples_frequence_num);
                }
            }
            else
            {
                BK_LOGE(TAG, "%s, %d, uac_device_param: %p is NULL\n", __func__, __LINE__, uac_device_param);
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, mic_port_info: %p is NULL\n", __func__, __LINE__, uac_mic->mic_port_info);
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, device_index: %d is not support \n", __func__, __LINE__, port_info->device_index);
        return;
    }

    //usb_hub_print_port_info(port_info);

    /* check automatic recover uac connect */
    if (uac_mic->cont_sta == UAC_MIC_ABNORMAL_DISCONNECTED)
    {
        /* uac automatically connect */
        if (!uac_mic->auto_connect)
        {
            BK_LOGD(TAG, "%s, %d, uac not automatically connect \n", __func__, __LINE__);
            return;
        }
    }

    uac_mic->cont_sta = UAC_MIC_CONNECTED;
    ret = uac_mic_drv_send_msg(uac_mic, UAC_MIC_DRV_CONT, uac_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send msg: UAC_MIC_DRV_CONT fail: %d \n", __func__, __LINE__);
    }
}

static void uac_mic_drv_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)param_data;

    rtos_set_semaphore(&uac_mic->uac_mic_sem);

    uac_mic_drv_msg_t msg;
    while (1)
    {
        ret = rtos_pop_from_queue(&uac_mic->uac_mic_drv_queue, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case UAC_MIC_DRV_IDLE:
                    break;

                case UAC_MIC_DRV_DATA:
                    break;

                case UAC_MIC_DRV_CONT:
                    uac_mic_connect_handle((uac_mic_stream_t *)msg.param);
                    break;

                case UAC_MIC_DRV_DISCONT:
                    uac_mic_disconnect_handle((uac_mic_stream_t *)msg.param);
                    break;

                case UAC_MIC_DRV_EXIT:
                    BK_LOGD(TAG, "%s, %d, goto: UAC_MIC_DRV_EXIT \n", __func__, __LINE__);
                    goto uac_mic_drv_exit;

                default:
                    break;
            }
        }
    }

uac_mic_drv_exit:

    /* delete msg queue */
    ret = rtos_deinit_queue(&uac_mic->uac_mic_drv_queue);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, delete message queue fail \n", __func__, __LINE__);
    }
    uac_mic->uac_mic_drv_queue = NULL;
    BK_LOGD(TAG, "%s, %d, delete aud_tras_int_msg_que \n", __func__, __LINE__);

    /* delete task */
    uac_mic->uac_mic_drv_thread = NULL;

    rtos_set_semaphore(&uac_mic->uac_mic_sem);

    rtos_delete_thread(NULL);
}

static bk_err_t uac_mic_drv_init(uac_mic_stream_t *uac_mic)
{
    bk_err_t ret = BK_OK;

    ret = rtos_init_queue(&uac_mic->uac_mic_drv_queue,
                          "uac_mic_drv_queue",
                          sizeof(uac_mic_drv_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create uac mic message queue fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    BK_LOGD(TAG, "%s, %d, create uac mic driver message queue complete \n", __func__, __LINE__);

    ret = audio_create_thread(&uac_mic->uac_mic_drv_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "uac_mic_drv",
                             (beken_thread_function_t)uac_mic_drv_main,
                             1536,
                             (void *)uac_mic,
                             uac_mic->task_core);
    if (ret != kNoErr)
    {
        /* delete msg queue */
        ret = rtos_deinit_queue(&uac_mic->uac_mic_drv_queue);
        if (ret != kNoErr)
        {
            BK_LOGE(TAG, "%s, %d, delete message queue fail \n", __func__, __LINE__);
        }
        uac_mic->uac_mic_drv_queue = NULL;
        BK_LOGE(TAG, "%s, %d, create uac mic driver task fail \n", __func__, __LINE__);
        return kGeneralErr;
    }

    rtos_get_semaphore(&uac_mic->uac_mic_sem, BEKEN_NEVER_TIMEOUT);

    BK_LOGD(TAG, "%s, %d, create uac mic driver task complete \n", __func__, __LINE__);

    return BK_OK;
}

static bk_err_t uac_mic_drv_deinit(uac_mic_stream_t *uac_mic)
{
    bk_err_t ret;
    uac_mic_drv_msg_t msg;

    msg.op = UAC_MIC_DRV_EXIT;
    msg.param = NULL;
    if (uac_mic && uac_mic->uac_mic_drv_queue)
    {
        ret = rtos_push_to_queue_front(&uac_mic->uac_mic_drv_queue, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret)
        {
            BK_LOGE(TAG, "%s, %d, send msg: UAC_MIC_DRV_EXIT fail \n", __func__, __LINE__);
            return kOverrunErr;
        }

        return ret;
    }

    return BK_OK;
}

static int _uac_mic_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    bk_err_t ret = BK_OK;

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)audio_element_getdata(self);

    if (uac_mic->is_open)
    {
        return BK_OK;
    }

    /* set read data timeout */
    //  audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);

    /* register uac connect and disconnect callback */
    ret = bk_usbh_hub_port_register_disconnect_callback(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic_disconnect_cb, uac_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, register uac mic disconnect cb fail \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_usbh_hub_port_register_connect_callback(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic_connect_cb, uac_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, register uac mic connect cb fail \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "%s, %d, init uac mic\n", __func__, __LINE__);
    /* check whether device power on */
    bk_usb_hub_port_info *port_dev_info = NULL;
    ret = bk_usbh_hub_port_check_device(uac_mic->port_index, USB_UAC_MIC_DEVICE, &port_dev_info);
    if (ret == BK_OK)
    {
        /* already power on */
        uac_mic->status = UAC_MIC_STA_CONNECT;
        uac_mic->mic_port_info = port_dev_info;

        /* set uac status and send  */
        uac_mic->cont_sta = UAC_MIC_CONNECTED;

        uac_mic->status = UAC_MIC_STA_CHECKING;

        /* Copy uac mic device parameters and then check parameters.
         * Avoid device disconnect and parameters ptr was been change to NULL when using parameters ptr.
         */
        if (uac_mic->mic_port_info)
        {
            bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_mic->mic_port_info->usb_device_param;
            if (uac_device_param)
            {
                uac_mic->format_tag = uac_device_param->mic_format_tag;
                uac_mic->samples_frequence_num = uac_device_param->mic_samples_frequence_num;
                if (uac_mic->samples_frequence_num > 0)
                {
                    if (uac_mic->samples_frequence)
                    {
                        audio_free(uac_mic->samples_frequence);
                        uac_mic->samples_frequence = NULL;
                    }
                    uac_mic->samples_frequence = audio_calloc(1, sizeof(uint32_t) * uac_mic->samples_frequence_num);
                    if (uac_mic->samples_frequence)
                    {
                        os_memcpy(uac_mic->samples_frequence, uac_device_param->mic_samples_frequence, sizeof(uint32_t) * uac_mic->samples_frequence_num);
                    }
                    else
                    {
                        BK_LOGE(TAG, "%s, %d, malloc samples_frequence fail\n", __func__, __LINE__);
                    }
                }
                else
                {
                    BK_LOGE(TAG, "%s, %d, samples_frequence_num: %d\n", __func__, __LINE__, uac_mic->samples_frequence_num);
                }
            }
            else
            {
                BK_LOGE(TAG, "%s, %d, uac_device_param: %p is NULL\n", __func__, __LINE__, uac_device_param);
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, mic_port_info: %p is NULL\n", __func__, __LINE__, uac_mic->mic_port_info);
        }

        uac_mic->status = UAC_MIC_STA_CONNECT;

        ret = uac_mic_drv_send_msg(uac_mic, UAC_MIC_DRV_CONT, uac_mic);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] %s, %d, send msg: UAC_MIC_DRV_CONT fail: %d \n", audio_element_get_tag(self), __func__, __LINE__);
        }
    }

    /* 
     * If uac power on already, vote uac mic to avoid power down when other devices power down.
     * If uac power down, vote uac mic and wait uac mic connect callback.
     */
    ret = bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, uac_mic->port_index, USB_UAC_MIC_DEVICE);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, power on uac mic port: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, uac_mic->port_index);
        return BK_FAIL;
    }

    if (kNoErr != rtos_get_semaphore(&uac_mic->uac_connect, 10000))
    {
        BK_LOGE(TAG, "[%s] %s, %d, uac connect timeout \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "[%s] uac start mic \n", audio_element_get_tag(self));

    /* Check whether uac mic status is UAC_MIC_STA_OPEN.
     * If false, open uac mic fail.(configurations are not support)
     * If true, open uac mic complete and send urb to start uac mic work.
     */
    if (uac_mic->status == UAC_MIC_STA_OPEN)
    {
        usb_hub_uac_mic_port_device_urb_fill(uac_mic);
        ret = bk_usbh_hub_dev_request_data(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic->uac_mic_urb);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] %s, %d, start uac mic port: %d, ret: %d \n", audio_element_get_tag(self), __func__, __LINE__, uac_mic->port_index, ret);
            return BK_FAIL;
        }
        else
        {
            uac_mic->status = UAC_MIC_STA_WORKING;
        }
    }
    else
    {
        BK_LOGE(TAG, "[%s] %s, %d, open uac mic device fail, device not support and please check\n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    uac_mic->is_open = true;

    return BK_OK;
}

static int _uac_mic_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d\n", audio_element_get_tag(el), __func__, len);

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)audio_element_getdata(el);
    int ret = BK_OK;
    uint32_t read_size = 0;

    UAC_MIC_INPUT_START();

    if (len)
    {
        BK_LOGV(TAG, "[%s] %s, mic_fill: %d \n", audio_element_get_tag(el), __func__, ring_buffer_get_fill_size(&uac_mic->mic_rb));
        if (ring_buffer_get_fill_size(&uac_mic->mic_rb) >= len)
        {
            //BK_LOGV(TAG, "[%s] _uac_mic_read, mic_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&uac_mic->mic_rb));
            read_size = ring_buffer_read(&uac_mic->mic_rb, (uint8_t *)buffer, len);
            if (read_size == len)
            {
                ret = read_size;
            }
            else
            {
                /* Shouldn't happen, return -1 to stop uac_mic_stream for debug. */
                BK_LOGE(TAG, "The error is happened in read data. read_size: %d \n", read_size);
                ret = -1;
            }
        }
        else
        {
            /* Can't get uac mic data, return timeout and uac_mic_stream not stop. */
            ret = AEL_IO_TIMEOUT;
        }
    }
    else
    {
        ret = len;
    }

    UAC_MIC_INPUT_END();

    return ret;
}

static int _uac_mic_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)audio_element_getdata(self);

    UAC_MIC_PROCESS_START();

    if (ring_buffer_get_fill_size(&uac_mic->mic_rb) < in_len)
    {
        if (kNoErr != rtos_get_semaphore(&uac_mic->can_process, 2000))  //portMAX_DELAY, 25 / portTICK_RATE_MS/2000
        {
            /* wait one frame uac mic data, and avoid frequent processing.
             * If get semaphore fail, uac mic don't have data, wait reconnect.
             * If get semaphore ok, uac mic have new data.
             */
            BK_LOGW(TAG, "[%s] %s, get semaphore fail \n", audio_element_get_tag(self), __func__);
        }
    }

    BK_LOGV(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    /* read input data */
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;
    if (r_size > 0)
    {
        //audio_element_multi_output(self, in_buffer, r_size, 0);

        UAC_MIC_OUTPUT_START();
        //w_size = audio_element_output(self, in_buffer, r_size);
        w_size = audio_element_output(self, in_buffer, r_size);
        UAC_MIC_OUTPUT_END();

        /* write data to multiple audio port */
        /* unblock write, and not check write result */
        //TODO
        audio_element_multi_output(self, in_buffer, r_size, 0);

        //更新处理数据的指针
        //audio_element_update_byte_pos(self, w_size);
    }
    else
    {
        w_size = r_size;
    }

    UAC_MIC_PROCESS_END();

    return w_size;
}

static int _uac_mic_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)audio_element_getdata(self);

    /* uac mic close
     * steps:
     * 1. Set uac mic status to "UAC_MIC_STA_OPEN" to stop get mic data.
     * 2. Set uac connect status to "UAC_MIC_NORMAL_DISCONNECTED".
     * 3. Deregister connect and disconnect callback to NULL, avoid processing disconenct and connect handle.
     * 4. Close usb port device.
     * 5. Power down.
     */

    uac_mic->status = UAC_MIC_STA_OPEN;

    uac_mic->cont_sta = UAC_MIC_NORMAL_DISCONNECTED;

    /* deregister connect and disconnect callback */
    bk_usbh_hub_port_register_disconnect_callback(uac_mic->port_index, USB_UAC_MIC_DEVICE, NULL, NULL);
    bk_usbh_hub_port_register_connect_callback(uac_mic->port_index, USB_UAC_MIC_DEVICE, NULL, NULL);

    /* close usb port device */
    if (uac_mic->mic_port_info)
    {
        bk_usbh_hub_port_dev_close(uac_mic->port_index, USB_UAC_MIC_DEVICE, uac_mic->mic_port_info);
        uac_mic->status = UAC_MIC_STA_CONNECT;
        //uac_mic->status = UAC_MIC_STA_DISCONNECT;
    }

    /* power down */
    bk_err_t ret = bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, uac_mic->port_index, USB_UAC_MIC_DEVICE);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, uac mic power down fail, ret: %d\n", __func__, __LINE__, ret);
    }
    else
    {
        uac_mic->status = UAC_MIC_STA_DISCONNECT;
        uac_mic->mic_port_info = NULL;
        uac_mic->cont_sta = UAC_MIC_IDLE;
    }

    /*
     * Don't wait for uac mic disconnect callback.
     * Because uac mic disconnect callback will not been call when other devices are working.
     */
#if 0
    if (kNoErr != rtos_get_semaphore(&uac_mic->uac_connect, 10000))
    {
        BK_LOGE(TAG, "[%s], %s, %d, uac disconnect timeout \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }
#endif

    /* free uac_mic_urb and urb buffer */
    if (uac_mic->uac_mic_urb)
    {
        audio_free(uac_mic->uac_mic_urb);
        uac_mic->uac_mic_urb = NULL;
    }

    if (uac_mic->urb_buff_addr)
    {
        audio_free(uac_mic->urb_buff_addr);
        uac_mic->urb_buff_addr = NULL;
    }

    uac_mic->is_open = false;

    return BK_OK;
}

static int _uac_mic_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    uac_mic_stream_t *uac_mic = (uac_mic_stream_t *)audio_element_getdata(self);

    uac_mic_drv_deinit(uac_mic);

    rtos_get_semaphore(&uac_mic->uac_mic_sem, BEKEN_NEVER_TIMEOUT);

    if (uac_mic && uac_mic->can_process)
    {
        rtos_deinit_semaphore(&uac_mic->can_process);
        uac_mic->can_process = NULL;
    }
    if (uac_mic && uac_mic->uac_connect)
    {
        rtos_deinit_semaphore(&uac_mic->uac_connect);
        uac_mic->uac_connect = NULL;
    }
    if (uac_mic && uac_mic->uac_mic_sem)
    {
        rtos_deinit_semaphore(&uac_mic->uac_mic_sem);
        uac_mic->uac_mic_sem = NULL;
    }
    if (uac_mic && uac_mic->mic_ring_buff)
    {
        ring_buffer_clear(&uac_mic->mic_rb);
        audio_free(uac_mic->mic_ring_buff);
        uac_mic->mic_ring_buff = NULL;
    }

    /* clear device parameters */
    if (uac_mic->samples_frequence)
    {
        audio_free(uac_mic->samples_frequence);
        uac_mic->samples_frequence = NULL;
    }

    if (uac_mic)
    {
        audio_free(uac_mic);
        uac_mic = NULL;
    }

    UAC_MIC_DATA_COUNT_CLOSE();

    return BK_OK;
}

audio_element_handle_t uac_mic_stream_init(uac_mic_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    uac_mic_stream_t *uac_mic = audio_calloc(1, sizeof(uac_mic_stream_t));
    AUDIO_MEM_CHECK(TAG, uac_mic, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _uac_mic_open;
    cfg.close = _uac_mic_close;
    cfg.process = _uac_mic_process;
    cfg.destroy = _uac_mic_destroy;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read = _uac_mic_read;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->frame_size;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.tag = "uac_mic";

    BK_LOGE(TAG, "%s, %d, buffer_len: %d, out_block_size: %d \n", __func__, __LINE__, cfg.buffer_len, cfg.out_block_size);

    /* init uac mic */
    uac_mic->port_index = config->port_index;
    uac_mic->format = config->format;
    uac_mic->chl_num = config->chl_num;
    uac_mic->bits = config->bits;
    uac_mic->samp_rate = config->samp_rate;
    uac_mic->frame_size = config->frame_size;
    uac_mic->out_block_size = config->out_block_size;
    uac_mic->out_block_num = config->out_block_num;
    uac_mic->auto_connect = config->auto_connect;
    uac_mic->is_open = false;
    ret = rtos_init_semaphore(&uac_mic->uac_connect, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_mic_init_exit;
    }
    ret = rtos_init_semaphore(&uac_mic->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_mic_init_exit;
    }

    ret = rtos_init_semaphore(&uac_mic->uac_mic_sem, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_mic_init_exit;
    }
    uac_mic->task_stack = config->task_stack;
    uac_mic->task_core = config->task_core;
    uac_mic->task_prio = config->task_prio;

    /* init ringbuffer (include two frame) */
    uac_mic->mic_ring_buff_size = uac_mic->frame_size * 2;
    uac_mic->mic_ring_buff = (int8_t *)audio_calloc(1, uac_mic->mic_ring_buff_size);
    AUDIO_MEM_CHECK(TAG, uac_mic->mic_ring_buff, goto _uac_mic_init_exit);
    ring_buffer_init(&uac_mic->mic_rb, (uint8_t *)uac_mic->mic_ring_buff, uac_mic->frame_size * 2, DMA_ID_MAX, RB_DMA_TYPE_NULL);
    uac_mic->cont_sta = UAC_MIC_IDLE;

    uac_mic->uac_mic_urb = NULL;
    uac_mic->uac_mic_urb = audio_calloc(1, sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));
    AUDIO_MEM_CHECK(TAG, uac_mic->uac_mic_urb, goto _uac_mic_init_exit);

    uac_mic->urb_buff_size = uac_mic->frame_size;
    uac_mic->urb_buff_addr = (uint8_t *)audio_calloc(1, uac_mic->urb_buff_size);
    AUDIO_MEM_CHECK(TAG, uac_mic->urb_buff_addr, goto _uac_mic_init_exit);

    ret = uac_mic_drv_init(uac_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, uac_mic_drv_init fail\n", __func__, __LINE__);
        goto _uac_mic_init_exit;
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _uac_mic_init_exit);
    audio_element_setdata(el, uac_mic);

    audio_element_info_t info = {0};
    info.sample_rates = config->samp_rate;
    info.channels = config->chl_num;
    info.bits = config->bits;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    UAC_MIC_DATA_COUNT_OPEN();

    return el;
_uac_mic_init_exit:

    if (uac_mic->can_process)
    {
        rtos_deinit_semaphore(&uac_mic->can_process);
        uac_mic->can_process = NULL;
    }
    if (uac_mic->uac_connect)
    {
        rtos_deinit_semaphore(&uac_mic->uac_connect);
        uac_mic->uac_connect = NULL;
    }
    if (uac_mic->uac_mic_sem)
    {
        rtos_deinit_semaphore(&uac_mic->uac_mic_sem);
        uac_mic->uac_mic_sem = NULL;
    }
    if (uac_mic->mic_ring_buff)
    {
        ring_buffer_clear(&uac_mic->mic_rb);
        audio_free(uac_mic->mic_ring_buff);
        uac_mic->mic_ring_buff = NULL;
    }

    if (uac_mic->uac_mic_urb)
    {
        audio_free(uac_mic->uac_mic_urb);
        uac_mic->uac_mic_urb = NULL;
    }

    if (uac_mic->urb_buff_addr)
    {
        audio_free(uac_mic->urb_buff_addr);
        uac_mic->urb_buff_addr = NULL;
    }

    audio_free(uac_mic);
    uac_mic = NULL;
    return NULL;
}

