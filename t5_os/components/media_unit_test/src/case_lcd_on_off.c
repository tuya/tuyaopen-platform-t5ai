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
#include "cli.h"
#include "media_app.h"

#include <driver/dvp_camera.h>
#include <driver/jpeg_enc.h>
#include "lcd_act.h"
#include "storage_act.h"
#include <driver/audio_ring_buff.h>
#if (CONFIG_AUD_INTF)
#include "aud_intf.h"
#endif
#include <driver/uvc_camera.h>
#include <driver/lcd.h>

#include <driver/media_types.h>
#include <lcd_decode.h>
#include <lcd_rotate.h>


/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

#include "bk_cli.h"
#include "media_evt.h"

#include "media_unit_test.h"
#include "media_unit_case.h"

#define TAG "MUT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)




int lcd_on_off_test(int argc, char **argv, int delay, int times)
{

	int ret, i;

	LOGI("%s\n", __func__);

	media_camera_device_t device = {0};
	device.type = media_unit_test_config->camera_type;
	device.mode = JPEG_MODE;
	device.fmt = PIXEL_FMT_JPEG;
	device.info.resolution.width = media_unit_test_config->camera_ppi >> 16;
	device.info.resolution.height = media_unit_test_config->camera_ppi & 0xFFFF;
	device.info.fps = FPS25;
	lcd_open_t lcd_open;
	lcd_open.device_ppi = PPI_DEFAULT;
	lcd_open.device_name = media_unit_test_config->lcd_name;

	media_app_pipline_set_rotate(media_unit_test_config->rotate);

	ret = rtos_init_semaphore(&wait_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, wait_sem failed\r\n", __func__);
		return -1;
	}

	ret = media_app_uvc_register_info_notify_cb(ut_uvc_connect_callback);

	if (ret == BK_OK)
	{
		LOGI("register uvc callback success\n");
	}
	else
	{
		LOGI("register uvc callback failed\n");
		goto error;
	}

	ret = media_app_camera_open(&device);

	if (ret == BK_OK)
	{
		LOGI("uvc open success\n");
	}
	else
	{
		LOGI("uvc open failed\n");
		goto error;
	}

	ret = rtos_get_semaphore(&wait_sem, BEKEN_NEVER_TIMEOUT);

	if (ret == BK_OK)
	{
		LOGI("wait uvc callback success\n");
	}
	else
	{
		LOGI("wait uvc callback failed\n");
		goto error;
	}

	ret = media_app_h264_pipeline_open();

	if (ret == BK_OK)
	{
		LOGI("h264 open success\n");
	}
	else
	{
		LOGI("h264 open failed\n");
		goto error;
	}

	rtos_delay_milliseconds(100);
	ut_mem_show();
	rtos_delay_milliseconds(100);
	media_app_pipeline_mem_show();
	media_app_pipeline_mem_leak();
	rtos_delay_milliseconds(100);

	for (i = 0; i < times; i++)
	{
		if (get_media_ut_exit())
		{
			break;
		}

		LOGI("#### Test Loop [%s] Start: %d/%d\n", __func__, i, times);

		ret = media_app_lcd_pipeline_open(&lcd_open);

		rtos_delay_milliseconds(delay);

		if (ret == BK_OK)
		{
			LOGI("lcd open success\n");
		}
		else
		{
			LOGI("lcd open failed\n");
			goto error;
		}

		if (ut_check_part(1, 1, 1, 1, 0, 0) == false)
		{
			media_app_pipeline_dump();
			break;
		}

		ret = media_app_lcd_pipeline_close();
		
		if (ret == BK_OK)
		{
			LOGI("lcd close success\n");
		}
		else
		{
			LOGI("lcd close failed\n");
			goto error;
		}

		LOGI("#### Test Loop [%s] End: %d/%d\n", __func__, i, times);
//		ut_mem_show();

		rtos_delay_milliseconds(delay);

		media_app_pipeline_mem_show();
	}

	rtos_delay_milliseconds(100);
	ut_mem_show();
	rtos_delay_milliseconds(100);
	media_app_pipeline_mem_show();
	media_app_pipeline_mem_leak();
	rtos_delay_milliseconds(100);

	ret = media_app_h264_pipeline_close();

	if (ret == BK_OK)
	{
		LOGI("h264 close success\n");
	}
	else
	{
		LOGI("h264 close failed\n");
		goto error;
	}

	ret = media_app_camera_close(UVC_CAMERA);

	if (ret == BK_OK)
	{
		LOGI("uvc close success\n");
	}
	else
	{
		LOGI("uvc close failed\n");
		goto error;
	}

error:
	rtos_deinit_semaphore(&wait_sem);

	return ret;
}
