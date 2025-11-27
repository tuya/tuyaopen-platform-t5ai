// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <components/log.h>


#include "media_evt.h"
#include <driver/int.h>
#include <os/mem.h>

#include "camera.h"
#include "frame_buffer.h"
#include "yuv_encode.h"

#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/video_common_driver.h>
#include <driver/uvc_camera.h>
#include <driver/h264.h>
#include <driver/jpeg_enc.h>
#include "camera_act.h"

#define TAG "cam_act"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

camera_state_cb_t uvc_state_cb = NULL;

static void camera_uvc_device_info_notify_to_cp0(bk_usb_hub_port_info *info, uint32_t state)
{
    if (uvc_state_cb)
    {
        uvc_state_cb(info, state);
    }
#if CONFIG_MEDIA_PIPELINE
    if (state == BK_UVC_DISCONNECT)
    {
        jpeg_decode_restart();
    }
#endif
}

bk_err_t camera_open_handle(media_device_t *dev)
{
    int ret = BK_FAIL;

    media_device_t *device = dev;
    camera_handle_t *handle = (camera_handle_t *)device->param1;
    media_camera_device_t *config = (media_camera_device_t *)device->param2;
    LOGD("%s\n", __func__);

    if (config->type == DVP_CAMERA)
    {
#ifdef CONFIG_DVP_CAMERA
        ret = bk_dvp_camera_open(handle, config);
#endif
    }
    else if (config->type == UVC_CAMERA)
    {
#ifdef CONFIG_USB_CAMERA
        bk_uvc_register_connect_state_cb(camera_uvc_device_info_notify_to_cp0);
        ret = bk_uvc_camera_open(handle, config);
#endif
    }
    else if (config->type == NET_CAMERA)
    {
        ret = bk_net_camera_open(handle, config);
    }
    else
    {
        LOGD("%s, not support\n", __func__);
    }

    return ret;
}

 bk_err_t camera_close_handle(camera_handle_t *handle)
{
	int ret = BK_FAIL;

	camera_config_t *config = (camera_config_t *)*handle;

	if (config->type == DVP_CAMERA)
	{
#ifdef CONFIG_DVP_CAMERA
		ret = bk_dvp_camera_close(handle);
#endif
	}
	else if (config->type == UVC_CAMERA)
	{
#ifdef CONFIG_USB_CAMERA
		ret = bk_uvc_camera_close(handle);
#endif
	}
	else if (config->type == NET_CAMERA)
	{
		ret = bk_net_camera_close(handle);
	}
	else
	{
		LOGW("%s, not support %x %x\n", __func__, config->id, config->type);
	}

	return ret;
}

bk_err_t camera_dvp_h264_reset_handle(void)
{
    int ret = BK_FAIL;

#ifdef CONFIG_DVP_CAMERA
    ret = bk_dvp_h264_idr_reset();
#endif

    LOGD("%s complete\n", __func__);

    return ret;
}

bk_err_t camera_uvc_register_device_info_cb_handle(camera_state_cb_t cb)
{
    int ret = BK_FAIL;
#ifdef CONFIG_USB_CAMERA
    uvc_state_cb = cb;
#endif

    return ret;
}

static bk_err_t camera_compression_ratio_config_handle(compress_ratio_t *ratio)
{
    int ret = BK_FAIL;

    LOGD("%s\n", __func__);

#if (defined(CONFIG_H264) || defined(CONFIG_JPEGENC_HW))

    if (ratio->mode == JPEG_MODE)
    {
        ret = bk_jpeg_enc_encode_config(ratio->enable, ratio->jpeg_up, ratio->jpeg_low);
    }
    else
    {
        ret = bk_h264_set_base_config(ratio);
    }
#endif

    return ret;
}

static bk_err_t camera_get_h264_encode_param_handle(h264_base_config_t *config)
{
	int ret = BK_FAIL;

#ifdef CONFIG_H264
	h264_base_config_t base_config = {0};

	ret = bk_h264_get_h264_base_config(&base_config);

	os_memcpy(config, &base_config, sizeof(h264_base_config_t));
#endif

	return ret;
}

static bk_err_t camera_switch_main_stream_handle(uint8_t id, uint8_t type, uint16_t image_format)
{
    int ret = BK_FAIL;

    ret = frame_buffer_list_set_main_stream(id, type, image_format);

    return ret;
}

bk_err_t camera_get_main_stream_handle(frame_list_node_t *node)
{
    int ret = BK_FAIL;
    node = frame_buffer_list_get_main_stream();

    if (node != NULL)
    {
        ret = BK_OK;
    }
    return ret;
}

bk_err_t camera_set_stream_state_handle(uint32_t state)
{
    int ret = BK_FAIL;

#ifdef CONFIG_USB_CAMERA
    ret = bk_uvc_set_stream_state(state);
#endif

#ifdef CONFIG_DVP_CAMERA
    ret = bk_dvp_set_stream_state(state);
#endif

    return ret;
}

#if 0
static bk_err_t camera_net_frame_buffer_malloc_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_FAIL;

	media_device_t *device = (media_device_t*)msg->param;
	camera_handle_t *handle = (camera_handle_t *)device->param1;
	frame_buffer_t **frame = (frame_buffer_t **)device->param2;

	*frame = bk_net_camera_frame_buffer_malloc(handle);

	if (frame)
	{
		ret = BK_OK;
	}

	//msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_net_frame_buffer_push_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_FAIL;

	media_device_t *device = (media_device_t*)msg->param;
	camera_handle_t *handle = (camera_handle_t *)device->param1;
	frame_buffer_t *frame = (frame_buffer_t *)device->param2;

	ret = bk_net_camera_frame_buffer_push(handle, frame);

	//msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_net_frame_buffer_free_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_FAIL;

	media_device_t *device = (media_device_t*)msg->param;
	camera_handle_t *handle = (camera_handle_t *)device->param1;
	frame_buffer_t *frame = (frame_buffer_t *)device->param2;

	ret = bk_net_camera_frame_buffer_free(handle, frame);

	//msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}
#endif

