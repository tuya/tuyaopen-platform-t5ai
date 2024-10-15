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
#include "aud_intf.h"
#include "aud_intf_types.h"

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

extern media_share_ptr_t *media_share_ptr;
uint32_t audio_data_count = 0;

void ut_mem_show(void)
{
	uint32_t total_size,free_size,mini_size;
//	LOGI("================Static memory================\r\n");
//	os_show_memory_config_info();

	LOGI("================Dynamic memory================\r\n");
	LOGI("%-5s   %-5s   %-5s	 %-5s	%-5s\r\n",
		"name", "total", "free", "minimum", "peak");

	total_size = rtos_get_total_heap_size();
	free_size  = rtos_get_free_heap_size();
	mini_size  = rtos_get_minimum_free_heap_size();
	LOGI("heap\t%d\t%d\t%d\t%d\r\n",	total_size,free_size,mini_size,total_size-mini_size);

#if CONFIG_PSRAM_AS_SYS_MEMORY
	total_size = rtos_get_psram_total_heap_size();
	free_size  = rtos_get_psram_free_heap_size();
	mini_size  = rtos_get_psram_minimum_free_heap_size();
	LOGI("psram\t%d\t%d\t%d\t%d\r\n", total_size,free_size,mini_size,total_size-mini_size);
#endif
}

bool ut_check_part(uint8_t jpeg_check, uint8_t dec_check, uint8_t lcd_check, uint8_t h264_check, uint8_t fps_wifi_check, uint8_t audio_check)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 jpeg_count = media_debug->isr_jpeg;
	uint16 dec_count = media_debug->isr_decoder;
	uint16 lcd_count = media_debug->isr_lcd;
	uint16 h264_count = media_debug->isr_h264;
	uint16 fps_wifi_count = media_debug->fps_wifi;
	uint32_t audio_data_count_cache = audio_data_count;

	if (jpeg_count == 0 || dec_count == 0 || lcd_count == 0 || h264_count == 0 || fps_wifi_count == 0)
	{
		LOGE("%s should not be: %u %u %u %u %u %u \n", __func__, jpeg_count, dec_count, lcd_count, h264_count, fps_wifi_count, audio_data_count_cache);
	}

	rtos_delay_milliseconds(200);

	if (jpeg_check && media_debug->isr_jpeg - jpeg_count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_jpeg - jpeg_count == 0)
        {
            LOGE("%s jpeg failed %x %x\n", __func__, media_debug->isr_jpeg, jpeg_count);
            return false;
        }
	}

	if (dec_check && media_debug->isr_decoder - dec_count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_decoder - dec_count == 0)
        {
            LOGE("%s dec failed %x %x\n", __func__, media_debug->isr_decoder, dec_count);
            return false;
        }
	}

	if (lcd_check && media_debug->isr_lcd - lcd_count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_lcd - lcd_count == 0)
        {
            LOGE("%s lcd failed %x %x\n", __func__, media_debug->isr_lcd, lcd_count);
            return false;
        }
	}

	if (h264_check && media_debug->isr_h264 - h264_count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_h264 - h264_count == 0)
        {
            LOGE("%s h264 failed %x %x\n", __func__, media_debug->isr_h264, h264_count);
            return false;
        }
	}

	if (fps_wifi_check && media_debug->fps_wifi - fps_wifi_count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->fps_wifi - fps_wifi_count == 0)
        {
            LOGE("%s fps_wifi failed %x %x\n", __func__, media_debug->fps_wifi, fps_wifi_count);
            return false;
        }
	}

	if (audio_check && audio_data_count - audio_data_count_cache == 0)
	{
		rtos_delay_milliseconds(500);
		if (audio_data_count - audio_data_count_cache == 0)
		{
			LOGE("%s audio failed %u %u \n", __func__, audio_data_count, audio_data_count_cache);
			return false;
		}
	}

	return true;
}

bool ut_check_jpeg(void)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 count = media_debug->isr_jpeg;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (media_debug->isr_jpeg - count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_jpeg - count == 0)
        {
            LOGE("%s failed %x %x\n", __func__, media_debug->isr_jpeg, count);
            return false;
        }
	}

	return true;
}

bool ut_check_h264(void)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 count = media_debug->isr_h264;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (media_debug->isr_h264 - count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_h264 - count == 0)
        {
            LOGE("%s failed %x %x\n", __func__, media_debug->isr_h264, count);
            return false;
        }
	}

	return true;
}

bool ut_check_lcd(void)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 count = media_debug->isr_lcd;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (media_debug->isr_lcd - count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_lcd - count == 0)
        {
		    LOGE("%s failed %x %x\n", __func__, media_debug->isr_lcd, count);
            return false;
        }
	}
	return true;
}

bool ut_check_dec(void)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 count = media_debug->isr_decoder;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (media_debug->isr_decoder - count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->isr_decoder - count == 0)
        {
            LOGE("%s failed %x %x\n", __func__, media_debug->isr_decoder, count);
            return false;
        }
	}

	return true;
}

bool ut_check_read_frame(void)
{
	media_debug_t *media_debug = media_share_ptr->media_debug;
	uint16 count = media_debug->fps_wifi;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (media_debug->fps_wifi - count == 0)
	{
        rtos_delay_milliseconds(500);
        if (media_debug->fps_wifi - count == 0)
        {
            LOGE("%s failed %x %x\n", __func__, media_debug->fps_wifi, count);
            return false;
        }
	}

	return true;
}

bool ut_check_audio(void)
{
	uint16 count = audio_data_count;

	if (count == 0)
	{
		LOGE("%s should not be: %u\n", __func__, count);
	}

	rtos_delay_milliseconds(200);

	if (audio_data_count - count == 0)
	{
		rtos_delay_milliseconds(500);
		if (audio_data_count - count == 0)
		{
			LOGE("%s failed %x %x\n", __func__, audio_data_count, count);
			return false;
		}
	}

	return true;
}


void ut_uvc_connect_callback(bk_uvc_device_brief_info_t *info, uvc_state_t state)
{
	bk_err_t ret;

	if (state == UVC_CONNECTED)
	{
		bk_uvc_config_t uvc_config_param = {0};
		uint8_t format_index = 0;
		uint8_t frame_num = 0;
		uint8_t index = 0, i = 0;
		uvc_config_param.vendor_id = info->vendor_id;
		uvc_config_param.product_id = info->product_id;
		LOGI("%s uvc_get_param VID:0x%x\r\n",__func__, info->vendor_id);
		LOGI("%s uvc_get_param PID:0x%x\r\n",__func__, info->product_id);

		format_index = info->format_index.mjpeg_format_index;
		frame_num = info->all_frame.mjpeg_frame_num;
		if(format_index > 0){
			LOGI("%s uvc_get_param MJPEG format_index:%d\r\n",__func__, format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param MJPEG width:%d heigth:%d index:%d\r\n",
							info->all_frame.mjpeg_frame[index].width,
							info->all_frame.mjpeg_frame[index].height,
							info->all_frame.mjpeg_frame[index].index);
				for(i = 0; i < info->all_frame.mjpeg_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param MJPEG fps:%d\r\n", info->all_frame.mjpeg_frame[index].fps[i]);
				}

				if (info->all_frame.mjpeg_frame[index].width == (media_unit_test_config->camera_ppi >> 16)
					&& info->all_frame.mjpeg_frame[index].height == (media_unit_test_config->camera_ppi & 0xFFFF))
				{
					uvc_config_param.frame_index = info->all_frame.mjpeg_frame[index].index;
					uvc_config_param.fps = info->all_frame.mjpeg_frame[index].fps[0];
					uvc_config_param.width = media_unit_test_config->camera_ppi >> 16;
					uvc_config_param.height = media_unit_test_config->camera_ppi & 0xFFFF;
				}
			}
		}

		uvc_config_param.format_index = format_index;

		if (media_app_set_uvc_device_param(&uvc_config_param) != BK_OK)
		{
			LOGE("%s, failed\r\n", __func__);
		}

		ret = rtos_set_semaphore(&wait_sem);

		if (ret != BK_OK)
		{
			LOGI("set semaphore failed\n");
		}

	}
	else
	{
		LOGI("%s, %d\r\n", __func__, state);
	}
}

void ut_read_frame_callback(frame_buffer_t *frame)
{
	if ((frame->sequence % 25) == 0)
	{
		LOGI("%s, seq:%d, size:%d, len:%d, data:%p\r\n", __func__, frame->sequence,
				frame->size, frame->length, frame->frame);
	}
}

int ut_audio_send_data_callback(unsigned char *data, unsigned int len)
{
	if (len > 0) {
		audio_data_count++;
	}

	return len;
}

void ut_uac_connect_callback(uint8_t state)
{
	bk_err_t ret;
	LOGI("%s, %d\r\n", __func__, state);

	if (state == AUD_INTF_UAC_CONNECTED)
	{
		ret = rtos_set_semaphore(&wait_sem);
		if (ret != BK_OK)
		{
			LOGI("set semaphore failed\n");
		}
	}
	else
	{
		LOGI("%s, %d\r\n", __func__, state);
	}
}

