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
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <driver/audio_ring_buff.h>
#include <driver/int.h>
#include <components/usbh_hub_multiple_classes_api.h>


#define TAG  "UAC_SPK"


//#define UAC_SPK_DEBUG   //GPIO debug

#ifdef UAC_SPK_DEBUG

#define UAC_SPK_URB_COMPLETE_CALLBACK()                 do { GPIO_DOWN(32); GPIO_UP(32); GPIO_DOWN(32);} while (0)

#define UAC_SPK_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define UAC_SPK_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define UAC_SPK_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define UAC_SPK_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define UAC_SPK_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define UAC_SPK_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define UAC_SPK_URB_COMPLETE_CALLBACK()

#define UAC_SPK_PROCESS_START()
#define UAC_SPK_PROCESS_END()

#define UAC_SPK_INPUT_START()
#define UAC_SPK_INPUT_END()

#define UAC_SPK_OUTPUT_START()
#define UAC_SPK_OUTPUT_END()

#endif


/* uac spk data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count uac spk data. */
#if CONFIG_ADK_UTILS

#define UAC_SPK_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef UAC_SPK_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t uac_spk_count_util = {0};
#define UAC_SPK_DATA_COUNT_INTERVAL     (1000 * 4)
#define UAC_SPK_DATA_COUNT_TAG          "UAC_SPK"

#define UAC_SPK_DATA_COUNT_OPEN()               count_util_create(&uac_spk_count_util, UAC_SPK_DATA_COUNT_INTERVAL, UAC_SPK_DATA_COUNT_TAG)
#define UAC_SPK_DATA_COUNT_CLOSE()              count_util_destroy(&uac_spk_count_util)
#define UAC_SPK_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&uac_spk_count_util, size)

#else

#define UAC_SPK_DATA_COUNT_OPEN()
#define UAC_SPK_DATA_COUNT_CLOSE()
#define UAC_SPK_DATA_COUNT_ADD_SIZE(size)

#endif  //UAC_SPK_DATA_COUNT


typedef enum
{
    UAC_SPK_DRV_IDLE = 0,
    UAC_SPK_DRV_EXIT,
    UAC_SPK_DRV_DATA,
    UAC_SPK_DRV_CONT,
    UAC_SPK_DRV_DISCONT,
} uac_spk_drv_op_t;

typedef struct
{
    uac_spk_drv_op_t op;
    void *param;
} uac_spk_drv_msg_t;

/* the audio uac connection status */
typedef enum
{
    UAC_SPK_IDLE = 0,                     /**< Idle: uac speaker idle status after normal disconnection */
    UAC_SPK_CONNECTED,                /**< Connected: uac automatically restore connection after abnormal disconnection */
    UAC_SPK_NORMAL_DISCONNECTED,      /**< Normal disconnected: uac normal disconnection */
    UAC_SPK_ABNORMAL_DISCONNECTED,    /**< Abnormal disconnected: uac abnormal disconnection */
    UAC_SPK_CONNECT_FAIL,             /**< Cannot automatically connect: uac automatically connect fail */
} uac_spk_cont_sta_t;

typedef enum
{
    UAC_SPK_STA_DISCONNECT = 0,    /**< Disconnect: uac speaker disconnect status */
    UAC_SPK_STA_CONNECT,           /**< Connected: uac speaker connect status after connected */
    UAC_SPK_STA_OPEN,              /**< Open: uac speaker open status after open */
    UAC_SPK_STA_WORKING,           /**< Working: uac speaker working status */

    UAC_SPK_STA_CHECKING,          /**< Checking: uac speaker parameters checking status, protect port_info */
} uac_spk_sta_t;

typedef struct uac_speaker_stream
{
    E_USB_HUB_PORT_INDEX          port_index;            /**< usb hub port index */
    uint16_t                      format;                /**< uac speaker data format */
    uint8_t                       chl_num;               /**< speaker channel number */
    uint8_t                       bits;                  /**< Bit wide (8, 16 bits) */
    uint32_t                      samp_rate;             /**< speaker sample rate */
    uint32_t                      frame_size;            /**< size of one frame speaker data */
    uint32_t                      volume;                /**< speaker volume */
    bool                          auto_connect;          /**< Automatic connect enable */
    bool                          is_open;               /**< speaker enable, true: enable, false: disable */
    beken_semaphore_t             uac_connect;           /**< uac connect semaphore */
    int                           multi_out_port_num;    /**< The number of multiple output audio port */

    uint32_t                      pool_size;             /**< speaker data pool size, the unit is byte */
    uint32_t                      pool_play_thold;       /**< the play threshold of pool, the unit is byte */
    uint32_t                      pool_pause_thold;      /**< the pause threshold of pool, the unit is byte */
    RingBufferContext             pool_rb;               /**< the pool ringbuffer handle */
    int8_t *                      pool_ring_buff;        /**< pool ring buffer addr */
    bool                          pool_can_read;         /**< the pool if can read */
    beken_semaphore_t             can_process;           /**< can process */

    beken_semaphore_t             uac_spk_sem;           /**< uac speaker driver task synchron */
    bk_usb_hub_port_info *        spk_port_info;         /**< uac speaker port information */
    struct usbh_urb *             uac_spk_urb;           /**< uac speaker urb */
    uint32_t                      urb_buff_size;         /**< uac speaker urb buffer size(one frame size) */
    uint8_t *                     urb_buff_addr;         /**< uac speaker urb buffer address */
    uint8_t *                     urb_buff_ping;         /**< uac speaker urb buffer ping address used to save data written to uac speaker */
    uint8_t *                     urb_buff_pang;         /**< uac speaker urb buffer pang address used to save data written to uac speaker */
    uint8_t *                     urb_buff_use;          /**< uac speaker urb buffer pang address used to save data written to uac speaker */
    bool                          wr_spk_rb_done;        /**< write one farme data to speaker ring buffer done */

    uint16_t                      format_tag;            /**< uac speaker device format used for check */
    uint8_t                       samples_frequence_num; /**< uac speaker device frequences number used for check */
    uint32_t *                    samples_frequence;     /**< uac speaker device frequences used for check */

    int                           task_stack;            /**< Task stack size */
    int                           task_core;             /**< Task running in core (0 or 1) */
    int                           task_prio;             /**< Task priority (based on freeRTOS priority) */
    beken_thread_t                uac_spk_drv_thread;    /**< uac speaker driver task handle */
    beken_queue_t                 uac_spk_drv_queue;     /**< uac speaker driver task queue */
    uac_spk_cont_sta_t            cont_sta;              /**< speaker ring buffer address */
    uac_spk_sta_t                 status;                /**< uac speaker status */
} uac_speaker_stream_t;


//#define AEC_MIC_DELAY_POINTS_DEBUG

#ifdef AEC_MIC_DELAY_POINTS_DEBUG

static void aec_mic_delay_debug(int16_t *data, uint32_t size)
{
    static uint32_t mic_delay_num = 0;
    mic_delay_num++;
    os_memset(data, 0, size);
    if (mic_delay_num == 50)
    {
        data[0] = 0x2FFF;
        mic_delay_num = 0;
        BK_LOGD(TAG, "AEC_MIC_DELAY_POINTS_DEBUG \n");
    }
}
#endif


bk_err_t uac_spk_drv_send_msg(uac_speaker_stream_t *uac_spk, uac_spk_drv_op_t op, void *param)
{
    bk_err_t ret;
    uac_spk_drv_msg_t msg;

    msg.op = op;
    msg.param = param;
    if (uac_spk && uac_spk->uac_spk_drv_queue)
    {
        ret = rtos_push_to_queue(&uac_spk->uac_spk_drv_queue, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret)
        {
            BK_LOGE(TAG, "%s, %d, uac_spk_drv_send_msg fail \n", __func__, __LINE__);
            return kOverrunErr;
        }

        return ret;
    }
    return kNoResourcesErr;
}

static bk_err_t uac_spk_disconnect_handle(uac_speaker_stream_t *uac_spk)
{
    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    if (!uac_spk)
    {
        BK_LOGE(TAG, "%s, %d, uac_spk is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (uac_spk->cont_sta == UAC_SPK_IDLE && uac_spk->status == UAC_SPK_STA_DISCONNECT)
    {
        if (BK_OK != rtos_set_semaphore(&uac_spk->uac_connect))
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

static void uac_spk_disconnect_cb(bk_usb_hub_port_info *port_info, void *arg)
{
    bk_err_t ret = BK_OK;

    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    //usb_hub_print_port_info(port_info);

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)arg;

    if (port_info->port_index != uac_spk->port_index)
    {
        BK_LOGE(TAG, "%s, %d, port_index: %d is not right \n", __func__, __LINE__, port_info->port_index);
        return;
    }

    /* reset port_info */
    if (port_info->device_index == USB_UAC_SPEAKER_DEVICE)
    {
        /* wait checking complete */
        while (uac_spk->status == UAC_SPK_STA_CHECKING)
        {
            rtos_delay_milliseconds(2);
        }

        uac_spk->status = UAC_SPK_STA_DISCONNECT;
        uac_spk->spk_port_info = NULL;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, device_index: %d is not support \n", __func__, __LINE__, port_info->device_index);
        return;
    }

    if (uac_spk->cont_sta == UAC_SPK_CONNECTED)
    {
        uac_spk->cont_sta = UAC_SPK_ABNORMAL_DISCONNECTED;
    }
    else if (uac_spk->cont_sta == UAC_SPK_NORMAL_DISCONNECTED)
    {
        uac_spk->cont_sta = UAC_SPK_IDLE;
    }
    else
    {
        //
    }

    ret = uac_spk_drv_send_msg(uac_spk, UAC_SPK_DRV_DISCONT, uac_spk);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send msg: UAC_SPK_DRV_DISCONT fail: %d \n", __func__, __LINE__);
    }
}

static void usb_hub_uac_spk_port_dev_complete_callback(void *pCompleteParam, int nbytes)
{
    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)pCompleteParam;

    bk_err_t ret = BK_OK;

    UAC_SPK_URB_COMPLETE_CALLBACK();
    UAC_SPK_DATA_COUNT_ADD_SIZE(nbytes);

    if (uac_spk->status == UAC_SPK_STA_WORKING)
    {
        /* reset urb transfer buffer data to 0 after data is written to uac speaker */
        os_memset(uac_spk->uac_spk_urb->transfer_buffer, 0x00, uac_spk->uac_spk_urb->transfer_buffer_length);

        uac_spk->urb_buff_addr = uac_spk->urb_buff_use;
        uac_spk->uac_spk_urb->transfer_buffer = uac_spk->urb_buff_addr;
        uac_spk->uac_spk_urb->transfer_buffer_length = uac_spk->urb_buff_size;
        uac_spk->uac_spk_urb->actual_length = 0;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, uac spk status: %d, not need write uac speaker data \n", __func__, __LINE__, uac_spk->status);
        return;
    }

    ret = rtos_set_semaphore(&uac_spk->can_process);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_set_semaphore fail \n", __func__, __LINE__);
    }

    uac_spk->wr_spk_rb_done = false;

    return;
}

static bk_err_t usb_hub_uac_spk_port_device_urb_fill(uac_speaker_stream_t *uac_spk)
{
    struct usbh_audio *spk_device;

    if (!uac_spk)
    {
        BK_LOGE(TAG, "%s, %d, uac_spk is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (uac_spk->uac_spk_urb && uac_spk->spk_port_info)
    {
        spk_device = (struct usbh_audio *)(uac_spk->spk_port_info->usb_device);
        uac_spk->uac_spk_urb->pipe = (usbh_pipe_t)(spk_device->isoout);
        uac_spk->uac_spk_urb->complete = (usbh_complete_callback_t)usb_hub_uac_spk_port_dev_complete_callback;
        uac_spk->uac_spk_urb->arg = (void *)uac_spk;
        uac_spk->uac_spk_urb->timeout = 0;

        uac_spk->uac_spk_urb->transfer_buffer = uac_spk->urb_buff_addr;
        uac_spk->uac_spk_urb->transfer_buffer_length = uac_spk->urb_buff_size;
        uac_spk->uac_spk_urb->num_of_iso_packets = 1;
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, parameters is invalid, urb: %p, port_info: %p\n", __func__, __LINE__, uac_spk->uac_spk_urb, uac_spk->spk_port_info);
        return BK_FAIL;
    }

    return BK_OK;
}


static bk_err_t uac_spk_connect_handle(uac_speaker_stream_t *uac_spk)
{
    bk_err_t ret = BK_OK;
    uint8_t i = 0;

    /* config uac */
    BK_LOGD(TAG, "%s, %d, config uac spk\n", __func__, __LINE__);

    bk_uac_spk_config_t *uac_spk_param_config = (bk_uac_spk_config_t *)uac_spk->spk_port_info->usb_device_param_config;
    bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_spk->spk_port_info->usb_device_param;
    /* check whether format and sample_rate is support */
    if (uac_spk->format != uac_device_param->spk_format_tag)
    {
        BK_LOGE(TAG, "%s, %d, format: %d is not support, invalid value: %d \n", __func__, __LINE__, uac_spk->format, uac_device_param->spk_format_tag);
        goto fail;
    }
    if (uac_device_param->spk_samples_frequence_num)
    {
        for (i = 0; i < uac_device_param->spk_samples_frequence_num; i++)
        {
            if (uac_device_param->spk_samples_frequence[i] == uac_spk->samp_rate)
            {
                break;
            }
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, spk not support, spk_samples_frequence_num: %d \n", __func__, __LINE__, uac_device_param->spk_samples_frequence_num);
        goto fail;
    }
    if (i >= uac_device_param->spk_samples_frequence_num)
    {
        BK_LOGE(TAG, "%s, %d, spk sample_rate: %d not support \n", __func__, __LINE__, uac_spk->samp_rate);
        goto fail;
    }

    if (uac_spk->status == UAC_SPK_STA_CONNECT)
    {
        uac_spk_param_config->spk_format_tag = uac_spk->format;
        uac_spk_param_config->spk_samples_frequence = uac_spk->samp_rate;
        uac_spk_param_config->spk_ep_desc = uac_device_param->mic_ep_desc;
        ret = bk_usbh_hub_port_dev_open(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk->spk_port_info);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, open uac spk: %d fail, ret: %d \n", __func__, __LINE__, uac_spk->port_index, ret);
            goto fail;
        }
        uac_spk->status = UAC_SPK_STA_OPEN;
    }

    /* Check whether uac spk element is open already.
     * If true, this connection is callback connection, so restart uac spk to get data.
     * If false, this connection is the first connection when open uac spk.
     */
    if (uac_spk->is_open == true)
    {
        if (uac_spk->status == UAC_SPK_STA_OPEN)
        {
            BK_LOGD(TAG, "%s, uac restart spk \n", __func__);
            usb_hub_uac_spk_port_device_urb_fill(uac_spk);
            ret = bk_usbh_hub_dev_request_data(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk->uac_spk_urb);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, start uac spk port: %d, ret: %d \n", __func__, __LINE__, uac_spk->port_index, ret);
                return BK_FAIL;
            }
            else
            {
                uac_spk->status = UAC_SPK_STA_WORKING;
            }
        }
    }
    else
    {
        ret = rtos_set_semaphore(&uac_spk->uac_connect);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, set uac_connect semaphore fail \n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;

fail:

    ret = rtos_set_semaphore(&uac_spk->uac_connect);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, set uac_connect semaphore fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_FAIL;
}


static void uac_spk_connect_cb(bk_usb_hub_port_info *port_info, void *arg)
{
    bk_err_t ret = BK_OK;
    BK_LOGD(TAG, "%s, %d \n", __func__, __LINE__);

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)arg;

    if (port_info->port_index != uac_spk->port_index)
    {
        BK_LOGE(TAG, "%s, %d, port_index: %d is not right \n", __func__, __LINE__, port_info->port_index);
        return;
    }

    /* save port_info */
    if (port_info->device_index == USB_UAC_SPEAKER_DEVICE)
    {
        uac_spk->status = UAC_SPK_STA_CONNECT;
        uac_spk->spk_port_info = port_info;
#if 0
        bk_uac_spk_config_t *uac_device_param_canfig = (bk_uac_spk_config_t *)port_info->usb_device_param_config;
        struct usb_endpoint_descriptor *spk_ep_desc = (struct usb_endpoint_descriptor *)uac_device_param_canfig->spk_ep_desc;
        BK_LOGD(TAG, "     ------------ Audio Data Spk Endpoint Descriptor -----------  \n");
        BK_LOGD(TAG, "bLength                        : 0x%x (%d bytes)\n", spk_ep_desc->bLength,spk_ep_desc->bLength);
        BK_LOGD(TAG, "bDescriptorType                : 0x%x (Audio Endpoint Descriptor)\n", spk_ep_desc->bDescriptorType);
        BK_LOGD(TAG, "bEndpointAddress               : 0x%x (General)\n", spk_ep_desc->bEndpointAddress);
        BK_LOGD(TAG, "bmAttributes                   : 0x%x\n", spk_ep_desc->bmAttributes);
        BK_LOGD(TAG, "wMaxPacketSize                 : 0x%x\n", spk_ep_desc->wMaxPacketSize);
        BK_LOGD(TAG, "bInterval                      : 0x%x\n", spk_ep_desc->bInterval);
#endif

        /* Copy uac speaker device parameters and then check parameters.
         * Avoid device disconnect and parameters ptr was been change to NULL when using parameters ptr.
         */
        if (uac_spk->spk_port_info)
        {
            bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_spk->spk_port_info->usb_device_param;
            if (uac_device_param)
            {
                uac_spk->format_tag = uac_device_param->spk_format_tag;
                uac_spk->samples_frequence_num = uac_device_param->spk_samples_frequence_num;
                if (uac_spk->samples_frequence_num > 0)
                {
                    uac_spk->samples_frequence = audio_calloc(1, sizeof(uint32_t) * uac_spk->samples_frequence_num);
                    if (uac_spk->samples_frequence)
                    {
                        os_memcpy(uac_spk->samples_frequence, uac_device_param->mic_samples_frequence, sizeof(uint32_t) * uac_spk->samples_frequence_num);
                    }
                    else
                    {
                        BK_LOGE(TAG, "%s, %d, malloc samples_frequence fail\n", __func__, __LINE__);
                    }
                }
                else
                {
                    BK_LOGE(TAG, "%s, %d, samples_frequence_num: %d\n", __func__, __LINE__, uac_spk->samples_frequence_num);
                }
            }
            else
            {
                BK_LOGE(TAG, "%s, %d, uac_device_param: %p is NULL\n", __func__, __LINE__, uac_device_param);
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, spk_port_info: %p is NULL\n", __func__, __LINE__, uac_spk->spk_port_info);
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, device_index: %d is not support \n", __func__, __LINE__, port_info->device_index);
        return;
    }

    //usb_hub_print_port_info(port_info);

    /* check automatic recover uac connect */
    if (uac_spk->cont_sta == UAC_SPK_ABNORMAL_DISCONNECTED)
    {
        /* uac automatically connect */
        if (!uac_spk->auto_connect)
        {
            BK_LOGD(TAG, "%s, %d, uac not automatically connect \n", __func__, __LINE__);
            return;
        }
    }

    uac_spk->cont_sta = UAC_SPK_CONNECTED;
    ret = uac_spk_drv_send_msg(uac_spk, UAC_SPK_DRV_CONT, uac_spk);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send msg: UAC_SPK_DRV_CONT fail: %d \n", __func__, __LINE__);
    }
}

static void uac_spk_drv_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)param_data;

    rtos_set_semaphore(&uac_spk->uac_spk_sem);

    uac_spk_drv_msg_t msg;
    while (1)
    {
        ret = rtos_pop_from_queue(&uac_spk->uac_spk_drv_queue, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case UAC_SPK_DRV_IDLE:
                    break;

                case UAC_SPK_DRV_DATA:
                    break;

                case UAC_SPK_DRV_CONT:
                    uac_spk_connect_handle((uac_speaker_stream_t *)msg.param);
                    break;

                case UAC_SPK_DRV_DISCONT:
                    uac_spk_disconnect_handle((uac_speaker_stream_t *)msg.param);
                    break;

                case UAC_SPK_DRV_EXIT:
                    BK_LOGD(TAG, "%s, %d, goto: UAC_SPK_DRV_EXIT \n", __func__, __LINE__);
                    goto uac_spk_drv_exit;

                default:
                    break;
            }
        }
    }

uac_spk_drv_exit:

    /* delete msg queue */
    ret = rtos_deinit_queue(&uac_spk->uac_spk_drv_queue);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, delete message queue fail \n", __func__, __LINE__);
    }
    uac_spk->uac_spk_drv_queue = NULL;
    BK_LOGD(TAG, "%s, %d, delete aud_tras_int_msg_que \n", __func__, __LINE__);

    /* delete task */
    uac_spk->uac_spk_drv_thread = NULL;

    rtos_set_semaphore(&uac_spk->uac_spk_sem);

    rtos_delete_thread(NULL);
}

static bk_err_t uac_spk_drv_init(uac_speaker_stream_t *uac_spk)
{
    bk_err_t ret = BK_OK;

    ret = rtos_init_queue(&uac_spk->uac_spk_drv_queue,
                          "uac_spk_drv_queue",
                          sizeof(uac_spk_drv_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create uac speaker message queue fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    BK_LOGD(TAG, "%s, %d, create uac speaker driver message queue \n", __func__, __LINE__);

    ret = audio_create_thread(&uac_spk->uac_spk_drv_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "uac_spk_drv",
                             (beken_thread_function_t)uac_spk_drv_main,
                             1536,
                             (void *)uac_spk,
                             uac_spk->task_core);
    if (ret != kNoErr)
    {
        /* delete msg queue */
        ret = rtos_deinit_queue(&uac_spk->uac_spk_drv_queue);
        if (ret != kNoErr)
        {
            BK_LOGE(TAG, "%s, %d, delete message queue fail \n", __func__, __LINE__);
        }
        uac_spk->uac_spk_drv_queue = NULL;
        BK_LOGE(TAG, "%s, %d, create uac speaker driver task fail \n", __func__, __LINE__);
        return kGeneralErr;
    }

    rtos_get_semaphore(&uac_spk->uac_spk_sem, BEKEN_NEVER_TIMEOUT);

    BK_LOGD(TAG, "%s, %d, create uac speaker driver task \n", __func__, __LINE__);

    return BK_OK;
}

static bk_err_t uac_spk_drv_deinit(uac_speaker_stream_t *uac_spk)
{
    bk_err_t ret;
    uac_spk_drv_msg_t msg;

    msg.op = UAC_SPK_DRV_EXIT;
    msg.param = NULL;
    if (uac_spk && uac_spk->uac_spk_drv_queue)
    {
        ret = rtos_push_to_queue_front(&uac_spk->uac_spk_drv_queue, &msg, BEKEN_NO_WAIT);
        if (kNoErr != ret)
        {
            BK_LOGE(TAG, "%s, %d, send msg: UAC_SPK_DRV_EXIT fail \n", __func__, __LINE__);
            return kOverrunErr;
        }

        return ret;
    }

    return BK_OK;
}

static int _uac_speaker_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    bk_err_t ret = BK_OK;

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)audio_element_getdata(self);

    if (uac_spk->is_open)
    {
        return BK_OK;
    }

    /* set read data timeout */
    audio_element_set_input_timeout(self, 0);   //10 / portTICK_RATE_MS

    BK_LOGV(TAG, "[%s] %s, %d, pool fill size: %d \n", audio_element_get_tag(self), __func__, __LINE__, ring_buffer_get_fill_size(&uac_spk->pool_rb));

    /* register uac connect and disconnect callback */
    ret = bk_usbh_hub_port_register_disconnect_callback(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk_disconnect_cb, uac_spk);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, register uac spk disconnect cb fail \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_usbh_hub_port_register_connect_callback(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk_connect_cb, uac_spk);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, register uac spk connect cb fail \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "%s, %d, init uac spk\n", __func__, __LINE__);
    /* check whether device power on */
    bk_usb_hub_port_info *port_dev_info = NULL;
    ret = bk_usbh_hub_port_check_device(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, &port_dev_info);
    if (ret == BK_OK)
    {
        /* already power on */
        uac_spk->status = UAC_SPK_STA_CONNECT;
        uac_spk->spk_port_info = port_dev_info;

        /* set uac status and send  */
        uac_spk->cont_sta = UAC_SPK_CONNECTED;

        uac_spk->status = UAC_SPK_STA_CHECKING;

        /* Copy uac speaker device parameters and then check parameters.
         * Avoid device disconnect and parameters ptr was been change to NULL when using parameters ptr.
         */
        if (uac_spk->spk_port_info)
        {
            bk_uac_device_brief_info_t *uac_device_param = (bk_uac_device_brief_info_t *)uac_spk->spk_port_info->usb_device_param;
            if (uac_device_param)
            {
                uac_spk->format_tag = uac_device_param->spk_format_tag;
                uac_spk->samples_frequence_num = uac_device_param->spk_samples_frequence_num;
                if (uac_spk->samples_frequence_num > 0)
                {
                    if (uac_spk->samples_frequence)
                    {
                        audio_free(uac_spk->samples_frequence);
                        uac_spk->samples_frequence = NULL;
                    }
                    uac_spk->samples_frequence = audio_calloc(1, sizeof(uint32_t) * uac_spk->samples_frequence_num);
                    if (uac_spk->samples_frequence)
                    {
                        os_memcpy(uac_spk->samples_frequence, uac_device_param->mic_samples_frequence, sizeof(uint32_t) * uac_spk->samples_frequence_num);
                    }
                    else
                    {
                        BK_LOGE(TAG, "%s, %d, malloc samples_frequence fail\n", __func__, __LINE__);
                    }
                }
                else
                {
                    BK_LOGE(TAG, "%s, %d, samples_frequence_num: %d\n", __func__, __LINE__, uac_spk->samples_frequence_num);
                }
            }
            else
            {
                BK_LOGE(TAG, "%s, %d, uac_device_param: %p is NULL\n", __func__, __LINE__, uac_device_param);
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, spk_port_info: %p is NULL\n", __func__, __LINE__, uac_spk->spk_port_info);
        }

        uac_spk->status = UAC_SPK_STA_CONNECT;

        ret = uac_spk_drv_send_msg(uac_spk, UAC_SPK_DRV_CONT, uac_spk);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] %s, %d, send msg: UAC_SPK_DRV_CONT fail: %d \n", audio_element_get_tag(self), __func__, __LINE__);
        }
    }

    /* 
     * If uac power on already, vote uac spk to avoid power down when other devices power down.
     * If uac power down, vote uac mic and wait uac spk connect callback.
     */
    ret = bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, uac_spk->port_index, USB_UAC_SPEAKER_DEVICE);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, power on uac spk port: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, uac_spk->port_index);
        return BK_FAIL;
    }

    if (kNoErr != rtos_get_semaphore(&uac_spk->uac_connect, 10000))
    {
        BK_LOGE(TAG, "[%s] %s, %d, uac connect timeout \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    /* Check whether uac spk status is UAC_SPK_STA_OPEN.
     * If false, open uac spk fail.(configurations are not support)
     * If true, open uac spk complete and send urb to start uac spk work.
     */
    if (uac_spk->status == UAC_SPK_STA_OPEN)
    {
        BK_LOGD(TAG, "[%s] uac start spk \n", audio_element_get_tag(self));
        usb_hub_uac_spk_port_device_urb_fill(uac_spk);
        ret = bk_usbh_hub_dev_request_data(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk->uac_spk_urb);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] %s, %d, start uac spk port: %d, ret: %d \n", audio_element_get_tag(self), __func__, __LINE__, uac_spk->port_index, ret);
            return BK_FAIL;
        }
        else
        {
            uac_spk->status = UAC_SPK_STA_WORKING;
        }
    }
    else
    {
        BK_LOGE(TAG, "[%s] %s, %d, open uac spk device fail, device not support and please check\n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    uac_spk->is_open = true;
    if (uac_spk->pool_ring_buff)
    {
        uac_spk->pool_can_read = true;
    }

    return BK_OK;
}

static int _uac_speaker_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)audio_element_getdata(el);

    if (uac_spk->pool_ring_buff)
    {
        /* check pool pause threshold */
        if (uac_spk->pool_can_read)
        {
            if (ring_buffer_get_fill_size(&uac_spk->pool_rb) <= uac_spk->pool_pause_thold)
            {
                BK_LOGD(TAG, "pause pool read, pool_fill: %d <= %d \n", ring_buffer_get_fill_size(&uac_spk->pool_rb), uac_spk->pool_pause_thold);
                uac_spk->pool_can_read = false;
            }
        }
        else
        {
            if (ring_buffer_get_fill_size(&uac_spk->pool_rb) >= uac_spk->pool_play_thold)
            {
                BK_LOGD(TAG, "start pool read \n");
                uac_spk->pool_can_read = true;
            }
        }
    }

    return len;
}


static int _uac_speaker_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)audio_element_getdata(self);
    uint32_t read_size = 0;

    UAC_SPK_PROCESS_START();

    if (kNoErr != rtos_get_semaphore(&uac_spk->can_process, 2000)) //portMAX_DELAY, 25 / portTICK_RATE_MS
    {
        BK_LOGW(TAG, "[%s] %s, get semaphore fail \n", audio_element_get_tag(self), __func__);
        //return -1;
    }

    BK_LOGV(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    /*check urb_buff currently used */
    if (uac_spk->urb_buff_addr == uac_spk->urb_buff_ping)
    {
        uac_spk->urb_buff_use = uac_spk->urb_buff_pang;
    }
    else
    {
        uac_spk->urb_buff_use = uac_spk->urb_buff_ping;
    }

    /* check whether pool enable */
    /* write data to urb buffer immediately if data is enough in pool */
    if (uac_spk->pool_ring_buff && uac_spk->pool_can_read && ring_buffer_get_fill_size(&uac_spk->pool_rb) >= uac_spk->urb_buff_size)
    {
        read_size = ring_buffer_read(&uac_spk->pool_rb, uac_spk->urb_buff_use, uac_spk->frame_size);
        if (read_size != uac_spk->frame_size)
        {
            BK_LOGE(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, uac_spk->frame_size);
            return -1;
        }
        else
        {
            uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
            aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
            /* write data to ref ring buffer */
            UAC_SPK_OUTPUT_START();
            audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
            UAC_SPK_OUTPUT_END();
        }
    }

    /* read input data, not wait */
    UAC_SPK_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    UAC_SPK_INPUT_END();
    if (r_size > 0)
    {
        if (uac_spk->pool_ring_buff)
        {
            /* write data to pool if pool enable */
            uint32_t w_size = ring_buffer_write(&uac_spk->pool_rb, (uint8_t *)in_buffer, r_size);
            if (w_size != r_size)
            {
                BK_LOGE(TAG, "The error is happened in writing data. write_size: %d \n", w_size);
                return -1;
            }

            /* check whether pool enable */
            /* write data to urb buffer immediately if data is enough in pool */
            if (uac_spk->wr_spk_rb_done == false && uac_spk->pool_can_read && ring_buffer_get_fill_size(&uac_spk->pool_rb) >= uac_spk->urb_buff_size)
            {
                read_size = ring_buffer_read(&uac_spk->pool_rb, uac_spk->urb_buff_use, uac_spk->frame_size);
                if (read_size != uac_spk->frame_size)
                {
                    BK_LOGE(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, uac_spk->frame_size);
                    return -1;
                }
                else
                {
                    UAC_SPK_OUTPUT_START();
                    uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                    aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
                    /* write data to ref ring buffer */
                    audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
                    UAC_SPK_OUTPUT_END();
                }
            }
            else
            {
                /* fill silence data */
                os_memset(uac_spk->urb_buff_use, 0x00, uac_spk->urb_buff_size);
                uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
                audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
                BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
            }
        }
        else
        {
            /* Write data to urb buffer if data readed is enough. Otherwise fill silence data. */
            if (r_size == uac_spk->urb_buff_size)
            {
                UAC_SPK_OUTPUT_START();
                os_memcpy(uac_spk->urb_buff_use, in_buffer, uac_spk->urb_buff_size);
                uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
                audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
                UAC_SPK_OUTPUT_END();
            }
            else
            {
                /* fill silence data */
                /* save data if data is enough */
                //TODO
                os_memset(uac_spk->urb_buff_use, 0x00, uac_spk->urb_buff_size);
                uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
                audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
                BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
            }
        }
    }
    else
    {
        /* fill silence data */
        os_memset(uac_spk->urb_buff_use, 0x00, uac_spk->urb_buff_size);
        uac_spk->wr_spk_rb_done = true;
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
        aec_mic_delay_debug((int16_t *)uac_spk->urb_buff_use, uac_spk->frame_size);
#endif
        audio_element_multi_output(self, (char *)uac_spk->urb_buff_use, uac_spk->frame_size, 0);
        //BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
    }

    audio_element_output(self, in_buffer, uac_spk->frame_size);

    UAC_SPK_PROCESS_END();

    return uac_spk->frame_size;
}

static int _uac_speaker_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)audio_element_getdata(self);

    /* uac speaker close
     * steps:
     * 1. Set uac speaker status to "UAC_SPK_STA_OPEN" to stop write speaker data.
     * 2. Set uac connect status to "UAC_SPK_NORMAL_DISCONNECTED".
     * 3. Deregister connect and disconnect callback to NULL, avoid processing disconenct and connect handle.
     * 4. Close usb port device.
     * 5. Power down.
     */

    uac_spk->status = UAC_SPK_STA_OPEN;

    uac_spk->cont_sta = UAC_SPK_NORMAL_DISCONNECTED;

    /* deregister connect and disconnect callback */
    bk_usbh_hub_port_register_disconnect_callback(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, NULL, NULL);
    bk_usbh_hub_port_register_connect_callback(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, NULL, NULL);

    /* close usb port device */
    if (uac_spk->spk_port_info)
    {
        bk_usbh_hub_port_dev_close(uac_spk->port_index, USB_UAC_SPEAKER_DEVICE, uac_spk->spk_port_info);
        uac_spk->status = UAC_SPK_STA_CONNECT;
    }

    /* power down */
    bk_err_t ret = bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, uac_spk->port_index, USB_UAC_SPEAKER_DEVICE);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, uac mic power down fail, ret: %d\n", __func__, __LINE__, ret);
    }
    else
    {
        uac_spk->status = UAC_SPK_STA_DISCONNECT;
        uac_spk->spk_port_info = NULL;
        uac_spk->cont_sta = UAC_SPK_IDLE;
    }

    /* 
     * Don't wait for uac spk disconnect callback.
     * Because uac spk disconnect callback will not been call when other devices are working.
     */
#if 0
    if (kNoErr != rtos_get_semaphore(&uac_spk->uac_connect, 10000))
    {
        BK_LOGE(TAG, "[%s], %s, %d, uac disconnect timeout \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }
#endif

    /* free uac_spk_urb and urb buffer */
    if (uac_spk->uac_spk_urb)
    {
        audio_free(uac_spk->uac_spk_urb);
        uac_spk->uac_spk_urb = NULL;
    }
    uac_spk->urb_buff_addr = NULL;
    if (uac_spk->urb_buff_ping)
    {
        audio_free(uac_spk->urb_buff_ping);
        uac_spk->urb_buff_ping = NULL;
    }
    if (uac_spk->urb_buff_pang)
    {
        audio_free(uac_spk->urb_buff_pang);
        uac_spk->urb_buff_pang = NULL;
    }

    uac_spk->is_open = false;
    uac_spk->pool_can_read = false;
    uac_spk->wr_spk_rb_done = false;

    return BK_OK;
}

static int _uac_speaker_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    uac_speaker_stream_t *uac_spk = (uac_speaker_stream_t *)audio_element_getdata(self);

    uac_spk_drv_deinit(uac_spk);

    rtos_get_semaphore(&uac_spk->uac_spk_sem, BEKEN_NEVER_TIMEOUT);

    if (uac_spk && uac_spk->can_process)
    {
        rtos_deinit_semaphore(&uac_spk->can_process);
        uac_spk->can_process = NULL;
    }
    if (uac_spk && uac_spk->uac_connect)
    {
        rtos_deinit_semaphore(&uac_spk->uac_connect);
        uac_spk->uac_connect = NULL;
    }
    if (uac_spk && uac_spk->uac_spk_sem)
    {
        rtos_deinit_semaphore(&uac_spk->uac_spk_sem);
        uac_spk->uac_spk_sem = NULL;
    }
    if (uac_spk && uac_spk->pool_ring_buff)
    {
        ring_buffer_clear(&uac_spk->pool_rb);
        audio_free(uac_spk->pool_ring_buff);
        uac_spk->pool_ring_buff = NULL;
    }

    /* clear device parameters */
    if (uac_spk->samples_frequence)
    {
        audio_free(uac_spk->samples_frequence);
        uac_spk->samples_frequence = NULL;
    }

    if (uac_spk)
    {
        audio_free(uac_spk);
        uac_spk = NULL;
    }

    UAC_SPK_DATA_COUNT_CLOSE();

    return BK_OK;
}

audio_element_handle_t uac_speaker_stream_init(uac_speaker_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    uac_speaker_stream_t *uac_spk = audio_calloc(1, sizeof(uac_speaker_stream_t));
    AUDIO_MEM_CHECK(TAG, uac_spk, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _uac_speaker_open;
    cfg.close = _uac_speaker_close;
    cfg.process = _uac_speaker_process;
    cfg.destroy = _uac_speaker_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write = _uac_speaker_write;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->frame_size;
    cfg.out_block_size = config->frame_size;
    cfg.out_block_num = 1;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.tag = "uac_spk";

    BK_LOGD(TAG, "%s, %d, buffer_len: %d, out_rb_size: %d \n", __func__, __LINE__, cfg.buffer_len, cfg.out_block_size);

    /* init uac spk */
    uac_spk->port_index = config->port_index;
    uac_spk->format = config->format;
    uac_spk->chl_num = config->chl_num;
    uac_spk->bits = config->bits;
    uac_spk->samp_rate = config->samp_rate;
    uac_spk->frame_size = config->frame_size;
    uac_spk->auto_connect = config->auto_connect;
    uac_spk->is_open = false;
    uac_spk->multi_out_port_num = config->multi_out_port_num;
    uac_spk->pool_size = config->pool_size;
    uac_spk->pool_play_thold = config->pool_play_thold;
    uac_spk->pool_pause_thold = config->pool_pause_thold;

    /* init speaker data pool */
    if (uac_spk->pool_size > 0 && uac_spk->pool_size > uac_spk->frame_size)
    {
        uac_spk->pool_ring_buff = (int8_t *)audio_calloc(1, uac_spk->pool_size);
        AUDIO_MEM_CHECK(TAG, uac_spk->pool_ring_buff, goto _uac_spk_init_exit);
        ring_buffer_init(&uac_spk->pool_rb, (uint8_t *)uac_spk->pool_ring_buff, uac_spk->pool_size, DMA_ID_MAX, RB_DMA_TYPE_NULL);
    }
    /* init urb info and buffer */
    uac_spk->uac_spk_urb = NULL;
    uac_spk->uac_spk_urb = audio_calloc(1, sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));
    AUDIO_MEM_CHECK(TAG, uac_spk->uac_spk_urb, goto _uac_spk_init_exit);
    uac_spk->urb_buff_size = uac_spk->frame_size;
    uac_spk->urb_buff_ping = (uint8_t *)audio_calloc(1, uac_spk->urb_buff_size);
    AUDIO_MEM_CHECK(TAG, uac_spk->urb_buff_ping, goto _uac_spk_init_exit);
    os_memset(uac_spk->urb_buff_ping, 0x00, uac_spk->frame_size);
    uac_spk->urb_buff_pang = (uint8_t *)audio_calloc(1, uac_spk->urb_buff_size);
    AUDIO_MEM_CHECK(TAG, uac_spk->urb_buff_pang, goto _uac_spk_init_exit);
    os_memset(uac_spk->urb_buff_pang, 0x00, uac_spk->frame_size);
    BK_LOGD(TAG, "%s, %d, pool_size: %d, temp_buff_size: %d \n", __func__, __LINE__, uac_spk->pool_size + uac_spk->frame_size, uac_spk->frame_size);
    uac_spk->urb_buff_addr = uac_spk->urb_buff_ping;
    uac_spk->urb_buff_use = uac_spk->urb_buff_pang;

    ret = rtos_init_semaphore(&uac_spk->uac_connect, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_spk_init_exit;
    }
    ret = rtos_init_semaphore(&uac_spk->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_spk_init_exit;
    }

    ret = rtos_init_semaphore(&uac_spk->uac_spk_sem, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _uac_spk_init_exit;
    }
    uac_spk->task_stack = config->task_stack;
    uac_spk->task_core = config->task_core;
    uac_spk->task_prio = config->task_prio;

    uac_spk->cont_sta = UAC_SPK_IDLE;

    ret = uac_spk_drv_init(uac_spk);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, uac_spk_drv_init fail\n", __func__, __LINE__);
        goto _uac_spk_init_exit;
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _uac_spk_init_exit);
    audio_element_setdata(el, uac_spk);

    audio_element_info_t info = {0};
    info.sample_rates = config->samp_rate;
    info.channels = config->chl_num;
    info.bits = config->bits;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    UAC_SPK_DATA_COUNT_OPEN();

    return el;
_uac_spk_init_exit:

    if (uac_spk->can_process)
    {
        rtos_deinit_semaphore(&uac_spk->can_process);
        uac_spk->can_process = NULL;
    }
    if (uac_spk->uac_connect)
    {
        rtos_deinit_semaphore(&uac_spk->uac_connect);
        uac_spk->uac_connect = NULL;
    }
    if (uac_spk->uac_spk_sem)
    {
        rtos_deinit_semaphore(&uac_spk->uac_spk_sem);
        uac_spk->uac_spk_sem = NULL;
    }
    if (uac_spk->pool_ring_buff)
    {
        ring_buffer_clear(&uac_spk->pool_rb);
        audio_free(uac_spk->pool_ring_buff);
        uac_spk->pool_ring_buff = NULL;
    }

    if (uac_spk->uac_spk_urb)
    {
        audio_free(uac_spk->uac_spk_urb);
        uac_spk->uac_spk_urb = NULL;
    }
    uac_spk->urb_buff_addr = NULL;

    if (uac_spk->urb_buff_ping)
    {
        audio_free(uac_spk->urb_buff_ping);
        uac_spk->urb_buff_ping = NULL;
    }
    if (uac_spk->urb_buff_pang)
    {
        audio_free(uac_spk->urb_buff_pang);
        uac_spk->urb_buff_pang = NULL;
    }

    audio_free(uac_spk);
    uac_spk = NULL;
    return NULL;
}

