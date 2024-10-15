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

#include "media_core.h"
#include "camera_act.h"
#include "media_evt.h"
#include "storage_act.h"
#include "media_app.h"
#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include "camera.h"
#include "frame_buffer.h"
#include "camera_driver.h"


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "components/bluetooth/bk_dm_bluetooth.h"

#include <driver/timer.h>

#define TAG "cam_act"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DEBUG_INTERVAL (1000 * 2)

typedef void (*camera_connect_state_t)(uint8_t state);

extern media_debug_t *media_debug;
extern media_debug_t *media_debug_cached;

camera_info_t camera_info = {0};
bool dvp_camera_reset_open_ind = false;
beken_semaphore_t camera_act_sema = NULL;
beken_timer_t camera_debug_timer = {0};

static beken_thread_t disc_task = NULL;

camera_connect_state_t camera_connect_state_change_cb = NULL;

static void camera_debug_dump(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	uint16_t jpg = (media_debug->isr_jpeg - media_debug_cached->isr_jpeg) / (DEBUG_INTERVAL / 1000);
	uint16_t h264 = (media_debug->isr_h264 - media_debug_cached->isr_jpeg) / (DEBUG_INTERVAL / 1000);
	uint16_t dec = (media_debug->isr_decoder - media_debug_cached->isr_decoder) / (DEBUG_INTERVAL / 1000);
	uint16_t lcd = (media_debug->isr_lcd - media_debug_cached->isr_lcd) / (DEBUG_INTERVAL / 1000);
	uint16_t fps = (media_debug->fps_lcd - media_debug_cached->fps_lcd) / (DEBUG_INTERVAL / 1000);
	uint16_t wifi = (media_debug->fps_wifi - media_debug_cached->fps_wifi) / (DEBUG_INTERVAL / 1000);
	uint16_t err_dec = (media_debug->err_dec - media_debug_cached->err_dec) / (DEBUG_INTERVAL / 1000);

	media_debug_cached->isr_jpeg = media_debug->isr_jpeg;
	media_debug_cached->isr_h264 = media_debug->isr_h264;
	media_debug_cached->isr_decoder = media_debug->isr_decoder;
	media_debug_cached->isr_lcd = media_debug->isr_lcd;
	media_debug_cached->fps_lcd = media_debug->fps_lcd;
	media_debug_cached->fps_wifi = media_debug->fps_wifi;
	media_debug_cached->err_dec = media_debug->err_dec;

	GLOBAL_INT_RESTORE();

	LOGI("jpg: %d[%d, %d], dec: %d[%d], h264: %d[%d-%d], lcd: %d[%d], fps: %d[%d], wifi: %d[%d], size:%dKB\n",
			jpg, media_debug->isr_jpeg, media_debug->psram_busy,
			h264, media_debug->isr_h264,
			dec, media_debug->isr_decoder, err_dec,
			lcd, media_debug->isr_lcd,
			fps, media_debug->fps_lcd,
			wifi, media_debug->fps_wifi,
			media_debug->jpeg_length / 1024);

	/*if ((jpg == 0) && (camera_info.type == MEDIA_DVP_MJPEG || camera_info.type == MEDIA_DVP_MIX))
	{
		if (CAMERA_STATE_DISABLED == get_camera_state())
		{
			// have been closed by up layer
			return;
		}

		media_msg_t msg;

		msg.event = EVENT_CAM_DVP_RESET_OPEN_IND;
		msg.param = camera_info.param;

		media_send_msg(&msg);

	}*/
}

void camera_dvp_reset_open_handle(uint32_t param)
{
#ifdef CONFIG_DVP_CAMERA

	bk_err_t ret = kNoErr;

	if (dvp_camera_reset_open_ind)
	{
		bk_dvp_camera_close();
		dvp_camera_reset_open_ind = false;
	}

	ret = bk_dvp_camera_open(camera_info.device);
	if (ret != kNoErr)
	{
		dvp_camera_reset_open_ind = false;
	}
	else
	{
		dvp_camera_reset_open_ind = true;
	}
#endif
}

void camera_dvp_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);
#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	os_memcpy(camera_info.device, (media_camera_device_t *)param->param, sizeof(media_camera_device_t));

	LOGI("%s, %d\r\n", __func__, __LINE__);

	ret = bk_dvp_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	//dvp_camera_reset_open_ind = true;

	set_camera_state(CAMERA_STATE_ENABLED);

	if (camera_info.debug)
	{
		// maybe change to send task msg
		rtos_init_timer(&camera_debug_timer, DEBUG_INTERVAL, (timer_handler_t)camera_debug_dump, NULL);
		rtos_start_timer(&camera_debug_timer);
	}

out:
#endif

	MEDIA_EVT_RETURN(param, ret);
}

void camera_dvp_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_dvp_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	dvp_camera_reset_open_ind = false;

	if (camera_info.debug)
	{
		rtos_stop_timer(&camera_debug_timer);
		rtos_deinit_timer(&camera_debug_timer);
	}

out:
#endif

	MEDIA_EVT_RETURN(param, ret);
}

static void camera_uvc_connect_state_change_task_entry(beken_thread_arg_t data)
{
	uint8_t state = *(uint8_t *)data;

	LOGI("%s, state:%d\r\n", __func__, state);

	if (camera_connect_state_change_cb)
	{
		camera_connect_state_change_cb(state);
	}
	else
	{
		// restart by self
		if (CAMERA_STATE_ENABLED != get_camera_state())
		{
			LOGI("%s, state:%d\r\n", __func__, state);
			if (state == UVC_CONNECTED)
			{
				int ret = bk_uvc_camera_open(camera_info.device);

				if (ret != BK_OK)
				{
					LOGE("%s open failed\n", __func__);
					goto out;
				}

				set_camera_state(CAMERA_STATE_ENABLED);

				if (camera_info.debug)
				{
					rtos_init_timer(&camera_debug_timer, DEBUG_INTERVAL, (timer_handler_t)camera_debug_dump, NULL);
					rtos_start_timer(&camera_debug_timer);
				}
			}
		}
	}

out:
	disc_task = NULL;
	rtos_delete_thread(NULL);
}

void camera_uvc_conect_state(uint8_t state)
{
	int ret = rtos_create_thread(&disc_task,
						 4,
						 "disc_task",
						 (beken_thread_function_t)camera_uvc_connect_state_change_task_entry,
						 1024 * 2,
						 (beken_thread_arg_t)&state);

	if (BK_OK != ret)
	{
		LOGE("%s camera_uvc_disconnect_task_entry init failed\n");
		return;
	}
}

void camera_uvc_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

#if CONFIG_USB_UVC

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif

	os_memcpy(camera_info.device, (media_camera_device_t *)param->param, sizeof(media_camera_device_t));

	ret = bk_uvc_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_camera_state(CAMERA_STATE_ENABLED);

	if (camera_info.debug)
	{
		rtos_init_timer(&camera_debug_timer, DEBUG_INTERVAL, (timer_handler_t)camera_debug_dump, NULL);
		rtos_start_timer(&camera_debug_timer);
	}

out:
#endif

	MEDIA_EVT_RETURN(param, ret);
}

void camera_uvc_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

#ifdef CONFIG_USB_UVC

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	if (camera_info.debug)
	{
		rtos_stop_timer(&camera_debug_timer);
		rtos_deinit_timer(&camera_debug_timer);
	}

	bk_uvc_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	//frame_buffer_enable(false);

	LOGI("%s complete!\n", __func__);

out:
#endif

	MEDIA_EVT_RETURN(param, ret);
}

void camera_uvc_reset_handle(uint32_t param)
{
	LOGI("%s\n", __func__);

#ifdef CONFIG_USB_UVC

	if (param == UVC_DISCONNECT_ABNORMAL)
	{
		if (CAMERA_STATE_DISABLED == get_camera_state())
		{
			LOGI("%s already close\n", __func__);
			return;
		}

		if (camera_info.debug)
		{
			rtos_stop_timer(&camera_debug_timer);
			rtos_deinit_timer(&camera_debug_timer);
		}

		bk_uvc_camera_close();

		set_camera_state(CAMERA_STATE_DISABLED);

		//camera_uvc_disconect();
	}

	camera_uvc_conect_state((uint8_t)param);

#endif

	LOGI("%s, complete!\n", __func__);

}

void camera_net_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

#ifdef CONFIG_WIFI_TRANSFER

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif

	os_memcpy(camera_info.device, (media_camera_device_t *)param->param, sizeof(media_camera_device_t));

	ret = bk_net_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_camera_state(CAMERA_STATE_ENABLED);

	if (camera_info.debug)
	{
		rtos_init_timer(&camera_debug_timer, DEBUG_INTERVAL, (timer_handler_t)camera_debug_dump, NULL);
		rtos_start_timer(&camera_debug_timer);
	}

out:
#endif

	MEDIA_EVT_RETURN(param, ret);

}

void camera_net_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

#ifdef CONFIG_WIFI_TRANSFER
	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_net_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

out:
#endif

	MEDIA_EVT_RETURN(param, ret);
}

void camera_rtsp_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_dvp_camera_init((video_config_t *)(param->param));

	set_camera_state(CAMERA_STATE_DISABLED);

out:

	MEDIA_EVT_RETURN(param, ret);
}

void camera_rtsp_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_dvp_camera_deinit();

	set_camera_state(CAMERA_STATE_DISABLED);

out:

	MEDIA_EVT_RETURN(param, ret);
}

void camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_CAM_DVP_OPEN_IND:
			camera_dvp_open_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_DVP_CLOSE_IND:
			camera_dvp_close_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_DVP_RESET_OPEN_IND:
			camera_dvp_reset_open_handle(param);
			break;

		case EVENT_CAM_UVC_OPEN_IND:
			camera_uvc_open_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_UVC_CLOSE_IND:
			camera_uvc_close_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_UVC_RESET_IND:
			camera_uvc_reset_handle(param);
			break;

		case EVENT_CAM_NET_OPEN_IND:
			camera_net_open_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_NET_CLOSE_IND:
			camera_net_close_handle((param_pak_t *)param);
			break;
		
		case EVENT_CAM_RTSP_OPEN_IND:
			camera_rtsp_open_handle((param_pak_t *)param);
			break;

		case EVENT_CAM_RTSP_CLOSE_IND:
			camera_rtsp_close_handle((param_pak_t *)param);
			break;

		default:
			break;
	}
}

media_camera_state_t get_camera_state(void)
{
	return camera_info.state;
}

void set_camera_state(media_camera_state_t state)
{
	camera_info.state = state;
}

void camera_init(void)
{
	camera_info.state = CAMERA_STATE_DISABLED;
	camera_info.debug = true;
	camera_info.device = (media_camera_device_t *)os_malloc(sizeof(media_camera_device_t));

	BK_ASSERT(camera_info.device != NULL);
}

bk_err_t media_app_register_uvc_connect_state_cb(void *cb)
{
	camera_connect_state_change_cb = cb;

	return BK_OK;
}


