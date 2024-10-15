// Copyright 2024-2025 Beken
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
#include "aud_intf.h"
#include "aud_intf_types.h"

#include "media_unit_test.h"
#include "media_unit_case.h"

#define TAG "MUT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

int audio_on_off_test(int argc, char **argv, int delay, int times)
{
	int ret, i;

	LOGI("%s\n", __func__);

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_voc_setup_t aud_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();
	aud_voc_setup.mic_type = media_unit_test_config->mic_type;
	aud_voc_setup.spk_type = media_unit_test_config->spk_type;

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

	ret = rtos_get_semaphore(&wait_sem, 10000);

	if (ret == BK_OK)
	{
		LOGI("wait uvc callback success\n");
	}
	else
	{
		LOGI("wait uvc callback failed\n");
		goto error;
	}


	ret = media_app_lcd_pipeline_open(&lcd_open);

	if (ret == BK_OK)
	{
		LOGI("lcd open success\n");
	}
	else
	{
		LOGI("lcd open failed\n");
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
		//ut_mem_show();

		audio_data_count = 0;

		aud_intf_drv_setup.aud_intf_tx_mic_data = ut_audio_send_data_callback;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_drv_init fail, ret:%d\n", ret);
			goto error;
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_VOICE);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_set_mode fail, ret:%d\n", ret);
			goto error;
		}

		/* uac recover connection */
		if (aud_voc_setup.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_voc_setup.spk_type == AUD_INTF_SPK_TYPE_UAC)
		{
			ret = bk_aud_intf_register_uac_connect_state_cb(ut_uac_connect_callback);
			if (ret != BK_ERR_AUD_INTF_OK)
			{
				LOGE("bk_aud_intf_register_uac_connect_state_cb fail, ret:%d\n", ret);
				goto error;
			}
		}

		ret = bk_aud_intf_voc_init(aud_voc_setup);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_voc_init fail, ret:%d\n", ret);
			goto error;
		}

		if (aud_voc_setup.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_voc_setup.spk_type == AUD_INTF_SPK_TYPE_UAC)
		{
			ret = rtos_get_semaphore(&wait_sem, 10000);
			if (ret == BK_OK)
			{
				LOGI("wait uac callback success\n");
			}
			else
			{
				LOGI("wait uac callback failed\n");
				goto error;
			}
		}

		ret = bk_aud_intf_voc_start();
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_voc_start fail, ret:%d\n", ret);
			goto error;
		}

		rtos_delay_milliseconds(delay);

		if (ut_check_part(1, 1, 1, 1, 0, 1) == false)
		{
			media_app_pipeline_dump();
			break;
		}

		ret = bk_aud_intf_voc_stop();
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_voc_stop fail, ret:%d\n", ret);
			goto error;
		}
		ret = bk_aud_intf_voc_deinit();
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_voc_deinit fail, ret:%d\n", ret);
			goto error;
		}
		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_set_mode fail, ret:%d\n", ret);
			goto error;
		}
		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			LOGE("bk_aud_intf_drv_deinit fail, ret:%d\n", ret);
			goto error;
		}

		LOGI("#### Test Loop [%s] End: %d/%d\n", __func__, i, times);
		//ut_mem_show();

		rtos_delay_milliseconds(delay);

		media_app_pipeline_mem_show();

	}

	rtos_delay_milliseconds(100);
	ut_mem_show();
	rtos_delay_milliseconds(100);
	media_app_pipeline_mem_show();
	media_app_pipeline_mem_leak();
	rtos_delay_milliseconds(100);

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



