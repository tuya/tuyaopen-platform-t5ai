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
#include "media_cli.h"
#include "aud_api.h"

#include "media_cli_comm.h"
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
#include <lcd_blend.h>

#define TAG "mcli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define UNKNOW_ERROR (-686)
#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)
#define GET_PPI(value)     get_ppi_from_cmd(argc, argv, value)
#define GET_NAME(value)    get_name_from_cmd(argc, argv, value)
#define GET_ROTATE()       get_rotate_from_cmd(argc, argv)

#if (CONFIG_USB_UVC) && (CONFIG_SYS_CPU0)
void uvc_connect_state_callback(uint8_t state)
{
	LOGI("%s %d+++\n", __func__, state);
}
#endif

void media_read_frame_callback(frame_buffer_t *frame)
{
	LOGI("frame_id:%d, length:%d, frame_addr:%p\r\n", frame->sequence,
		frame->length, frame->frame);
}

char * get_string_to_name(char *string, char * pre)
{
	char* value = pre;
	if (os_strcmp(string, "nt35512") == 0)
	{
		value = "nt35512";
	}
	if (os_strcmp(string, "gc9503v") == 0)
	{
		value = "gc9503v";
	}
	if (os_strcmp(string, "st7282") == 0)
	{
		value = "st7282";
	}
	if (os_strcmp(string, "st7796s") == 0)
	{
		value = "st7796s";
	}
	if (os_strcmp(string, "hx8282") == 0)
	{
		value = "hx8282";
	}
	if (os_strcmp(string, "nt35510") == 0)
	{
		value = "nt35510";
	}
	if (os_strcmp(string, "nt35510_mcu") == 0)
	{
		value = "nt35510_mcu";
	}
	if (os_strcmp(string, "h050iwv") == 0)
	{
		value = "h050iwv";
	}
	if (os_strcmp(string, "md0430r") == 0)
	{
		value = "md0430r";
	}
	if (os_strcmp(string, "md0700r") == 0)
	{
		value = "md0700r";
	}
	if (os_strcmp(string, "st7710s") == 0)
	{
		value = "st7710s";
	}
	if (os_strcmp(string, "st7701sn") == 0)
	{
		value = "st7701sn";
	}
	if (os_strcmp(string, "st7701s") == 0)
	{
		value = "st7701s";
	}
	if (os_strcmp(string, "st7789v") == 0)
	{
		value = "st7789v";
	}
	return value;
}

uint32_t get_ppi_from_cmd(int argc, char **argv, uint32_t pre)
{
	int i;
	uint32_t value = pre;

	for (i = 0; i < argc; i++)
	{
		value = get_string_to_ppi(argv[i]);

		if (value != PPI_DEFAULT)
		{
			break;
		}
	}

	if (value == PPI_DEFAULT)
	{
		value = pre;
	}

	LOGI("%s %d-%d+++\n", __func__, value >> 16, value & 0xFFFF);

	return value;
}

char * get_name_from_cmd(int argc, char **argv, char * pre)
{
	int i;
	char* value = pre;

	for (i = 3; i < argc; i++)
	{
		value = get_string_to_name(argv[i], pre);
		if (value != pre)
		{
			break;
		}
	}

	return value;
}

media_rotate_t get_rotate_from_cmd(int argc, char **argv)
{
	int i;
	media_rotate_t value = ROTATE_90;
	for (i = 3; i < argc; i++)
	{
		if (os_strcmp(argv[i], "90") == 0)
			value = ROTATE_90;
		else if (os_strcmp(argv[i], "270") == 0)
			value = ROTATE_270;
		else if (os_strcmp(argv[i], "0") == 0)
			value = ROTATE_NONE;
		else if (os_strcmp(argv[i], "180") == 0)
			value = ROTATE_180;
		else
			value = ROTATE_90;
	}
	return value;
}

bool cmd_contain(int argc, char **argv, char *string)
{
	int i;
	bool ret = false;

	for (i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], string) == 0)
		{
			ret = true;
		}
	}

	return ret;
}

void media_cli_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = UNKNOW_ERROR;
	char *msg = NULL;

	LOGI("%s +++\n", __func__);

	if (argc <= 2)
	{
		ret = kParamErr;
		goto output;
	}
	else
	{
		if (os_strcmp(argv[1], "dvp") == 0)
		{
#if ((CONFIG_DVP_CAMERA) && (CONFIG_SYS_CPU0))
			media_ppi_t ppi = GET_PPI(PPI_640X480);
			media_camera_device_t device = {0};
			device.type = DVP_CAMERA;
			device.mode = JPEG_MODE;
			device.fmt = PIXEL_FMT_JPEG;
			device.info.resolution.width = ppi >> 16;
			device.info.resolution.height = ppi & 0xFFFF;
			device.info.fps = FPS25;

			if (CMD_CONTAIN("yuv"))
			{
				device.mode = YUV_MODE;
				device.fmt = PIXEL_FMT_YUYV;
			}

			if (CMD_CONTAIN("jpeg"))
			{
				if (CMD_CONTAIN("enc_yuv"))
				{
					device.mode = JPEG_YUV_MODE;
				}
				else
				{
					device.mode = JPEG_MODE;
				}

				device.fmt = PIXEL_FMT_JPEG;
			}

			if (CMD_CONTAIN("h264"))
			{
				if (CMD_CONTAIN("enc_yuv"))
				{
					device.mode = H264_YUV_MODE;
				}
				else
				{
					device.mode = H264_MODE;
				}

				device.fmt = PIXEL_FMT_H264;
			}

			if (os_strcmp(argv[2], "open") == 0)
			{
				ret = media_app_camera_open(&device);
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_camera_close(DVP_CAMERA);
			}

			if (os_strcmp(argv[2], "auto_encode") == 0)
			{
				uint8_t auto_enable = 0;
				uint16_t up_size = 0, low_size = 0;

				auto_enable = os_strtoul(argv[3], NULL, 10) & 0xF;
				if (auto_enable)
				{
					up_size = os_strtoul(argv[4], NULL, 10) * 1024;
					low_size = os_strtoul(argv[5], NULL, 10) * 1024;
				}

				ret = bk_jpeg_enc_encode_config(auto_enable, up_size, low_size);
			}
#endif
		}

#if 0
		if (os_strcmp(argv[1], "adc") == 0)
		{
			ret = adc_open();
		}

		if (os_strcmp(argv[1], "dac") == 0)
		{
			ret = dac_open();
		}
#endif

#if (CONFIG_IMAGE_STORAGE && CONFIG_SYS_CPU0)

		if (os_strcmp(argv[1], "storage") == 0)
		{
			if (os_strcmp(argv[2], "open") == 0)
			{
				ret = media_app_storage_open();
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_storage_close();
			}
		}

		if (os_strcmp(argv[1], "capture") == 0)
		{
			LOGI("capture\n");

			if (argc >= 3)
			{
				ret = media_app_capture(argv[2]);
			}
			else
			{
				ret = media_app_capture("unknow.jpg");
			}
		}

		if (os_strcmp(argv[1], "save_start") == 0)
		{
			if (argc >= 3)
			{
				ret = media_app_save_start(argv[2]);
			}
			else
			{
				ret = media_app_save_start("unknow.264");
			}
		}

		if (os_strcmp(argv[1], "save_stop") == 0)
		{
			ret = media_app_save_stop();
		}
#endif

		if (os_strcmp(argv[1], "lcd") == 0)
		{
#if (CONFIG_SYS_CPU0)
			media_ppi_t ppi = PPI_480X272;
			char *name = "NULL";

			ppi = GET_PPI(PPI_480X272);
			name = GET_NAME(name);
			media_rotate_t rotate = ROTATE_NONE;

			if (CMD_CONTAIN("rotate"))
			{
				rotate = GET_ROTATE();
				ret = media_app_lcd_rotate(rotate);
			}
			if (os_strcmp(argv[2], "open") == 0)
			{
				if (argc >= 4)
				{
					if(os_strcmp(argv[3], "dec_major") == 0)
					{
						media_app_lcd_decode(SOFTWARE_DECODING_MAJOR);
					}
					else if(os_strcmp(argv[3], "dec_minor") == 0)
					{
						media_app_lcd_decode(SOFTWARE_DECODING_MINOR);
					}
					else
					{
						media_app_lcd_decode(HARDWARE_DECODING);
					}
				}
				lcd_open_t lcd_open;
				lcd_open.device_ppi = ppi;
				lcd_open.device_name = name;
				ret = media_app_lcd_open(&lcd_open);
                lcd_driver_backlight_open();
			}

			if (os_strcmp(argv[2], "resize") == 0)
			{
				if (argc >= 4)
				{
					ppi = get_string_to_ppi(argv[3]);
					if (ppi == PPI_DEFAULT)
					{
						LOGE("param ppi error!\r\n");
					}
					else
					{
						ret = media_app_lcd_resize(ppi);
					}
				}
				else
				{
					LOGE("param error!\r\n");
				}
			}
#if (CONFIG_LVGL) && (CONFIG_LVGL_DEMO)
			else if(os_strcmp(argv[2], "demoui") == 0)
			{
				if(argc >= 4)
				{
					int value = atoi(argv[3]);

					bk_printf("value:%d\r\n", value);
					if(0 == value)
					{
						void lv_example_img_display_init(int switch_time_ms);
						lv_example_img_display_init(3000);
					}
				}
				else
				{
					void lv_example_meter(void);
					lv_example_meter();
				}
				ret = kNoErr;
			}
#endif

			if (os_strcmp(argv[2], "close") == 0)
			{
    			lcd_driver_backlight_close();
				ret = media_app_lcd_close();
			}

			if (os_strcmp(argv[2], "backlight") == 0)
			{
				uint8_t level = os_strtoul(argv[3], NULL, 10) & 0xFF;
				ret = media_app_lcd_set_backlight(level);
			}

			if (os_strcmp(argv[2], "step") == 0)
			{
				if (CMD_CONTAIN("enable"))
				{
					ret = media_app_lcd_step_mode(true);
				}

				if (CMD_CONTAIN("disable"))
				{
					ret = media_app_lcd_step_mode(false);
				}

				if (CMD_CONTAIN("trigger"))
				{
					ret = media_app_lcd_step_trigger();
				}
			}

			if (os_strcmp(argv[2], "display") == 0)
			{
				if (argc >= 4)
				{
					ret = media_app_lcd_display_file(argv[3]);
				}
			}
//			if (os_strcmp(argv[2], "flash_display") == 0)
//			{
//#if (CONFIG_IMAGE_STORAGE)
//				lcd_display_t lcd_display;
//				lcd_display.image_addr = storge_flash.flash_image_addr;
//				lcd_display.img_length = storge_flash.flasg_img_length;
//				ret = media_app_lcd_display(&lcd_display);
//#endif
//			}
//			if (os_strcmp(argv[2], "beken_logo") == 0)
//			{
//				lcd_display_t lcd_display;
//				lcd_display.image_addr = os_strtoul(argv[3], NULL, 16) & 0xFFFFFFFF;
//				lcd_display.img_length = os_strtoul(argv[4], NULL, 16) & 0xFFFFFFFF;
//				ret = media_app_lcd_display_beken(&lcd_display);
//			}
			if (os_strcmp(argv[2], "dma2d_blend") == 0)
			{
#if CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND
				if (argc < 4)
				{
					ret = kParamErr;
					goto output;
				}

				lcd_blend_msg_t blend = {0} ;
				if(os_strcmp(argv[3], "open") == 0)
				{
					ret = media_app_lcd_blend_open(1);
				}
				else if (os_memcmp (argv[3], "wifi", 4) == 0)
				{
					if (argc < 5)
					{
						ret = kParamErr;
						goto output;
					}

					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_WIFI;
					blend.data[0] =  os_strtoul(argv[4], NULL, 10) & 0xFFFF;
					LOGD("wifi dma2d blend data = %d\r\n", blend.data[0]);
				}
				else if (os_memcmp (argv[3], "clock", 4) == 0)
				{
					if (argc < 5)
					{
						ret = kParamErr;
						goto output;
					}

					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_TIME;
					os_memcpy(blend.data, argv[4], 5);
					LOGD("time dma2d blend data = %s\r\n", blend.data);
				}
				else if (os_memcmp (argv[3], "data", 4) == 0)
				{
					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_DATA;
					//os_memcpy(blend.data, argv[4], 5);
				}
				else if (os_memcmp (argv[3], "ver", 3) == 0)
				{
					uint8_t version[] = "VL4 V1.23.34";
					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_VERSION;
					os_memcpy(blend.data, version, sizeof(version));
					LOGD("ver dma2d blend data = %s\r\n", blend.data);
				}
				else if (os_strcmp(argv[3], "close") == 0)
				{
					if (argc < 5)
					{
						blend.blend_on = 0;
						blend.lcd_blend_type = LCD_BLEND_VERSION | LCD_BLEND_TIME | LCD_BLEND_WIFI | LCD_BLEND_DATA;
						ret = media_app_lcd_blend_open(0);
						LOGI(" dma2d blend close deinit\r\n");
					}
					else
					{
						if (os_strcmp(argv[4], "wifi") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_WIFI;
						}
						else if (os_strcmp(argv[4], "clock") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_TIME;
						}
						else if (os_strcmp(argv[4], "ver") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_VERSION;
						}
						else if (os_strcmp(argv[4], "data") == 0)
						{
							blend.blend_on = 0;

							blend.lcd_blend_type = LCD_BLEND_DATA;
						}
						else
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_VERSION | LCD_BLEND_TIME | LCD_BLEND_WIFI | LCD_BLEND_DATA;
						}
					}
				}
				else
				{
					LOGI("cmd not support \r\n");
				}
				ret = media_app_lcd_blend(&blend);
				
#endif
			}
#endif
		}
		if (os_strcmp(argv[1], "uvc") == 0)
		{
#if (CONFIG_USB_UVC) && (CONFIG_SYS_CPU0)
			media_ppi_t ppi = GET_PPI(PPI_640X480);
			media_camera_device_t device = {0};
			device.type = UVC_CAMERA;
			device.mode = JPEG_MODE;
			device.fmt = PIXEL_FMT_JPEG;
			device.info.fps = FPS25;

			if (CMD_CONTAIN("h264"))
			{
				device.mode = H264_MODE;
				device.fmt = PIXEL_FMT_H264;
			}

			if (os_strcmp(argv[2], "open") == 0)
			{
				if (ppi == 0)
				{
					LOGI("resolution not support\r\n");
					ret = BK_FAIL;
				}
				else
				{
					device.info.resolution.width = ppi >> 16;
					device.info.resolution.height = ppi & 0xFFFF;
					media_app_register_uvc_connect_state_cb(uvc_connect_state_callback);

					ret = media_app_camera_open(&device);
				}
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_camera_close(UVC_CAMERA);
			}
#endif
		}

#if CONFIG_SYS_CPU0
		if (os_strcmp(argv[1], "mb") == 0)
		{
			ret = media_app_mailbox_test();
		}
#endif

		if (os_strcmp(argv[1], "dump") == 0)
		{
#if (CONFIG_LCD) && (CONFIG_SYS_CPU0)

			if (CMD_CONTAIN("decoder"))
			{
				ret = media_app_dump_decoder_frame();
			}

			if (CMD_CONTAIN("jpeg"))
			{
				ret = media_app_dump_jpeg_frame();
			}

			if (CMD_CONTAIN("display"))
			{
				ret = media_app_dump_display_frame();
			}
#endif

		}

		if (os_strcmp(argv[1], "read_frame") == 0)
		{
			uint32_t fmt = PIXEL_FMT_JPEG;
			if (os_strcmp(argv[2], "open") == 0)
			{
				if (CMD_CONTAIN("h264"))
				{
					fmt = PIXEL_FMT_H264;
				}
				else
				{
					fmt = PIXEL_FMT_JPEG;
				}

				ret = media_app_register_read_frame_callback(fmt, media_read_frame_callback);
			}
			else // close
			{
				ret = media_app_unregister_read_frame_callback();
			}
		}

		if (os_strcmp(argv[1], "avi") == 0)
		{
#if (CONFIG_DVP_CAMERA) && (CONFIG_SYS_CPU0) && defined(CONFIG_VIDEO_AVI)
		if (0 == os_strcmp(argv[2], "v_open"))
		{
			ret = media_app_avi_open();
			LOGI("media_app_avi_open\r\n");
		}

		if (0 == os_strcmp(argv[2], "v_close"))
		{
			ret = media_app_avi_close();
			LOGI("media_app_avi_close\r\n");
		}
#endif

#if (CONFIG_AUD_INTF && CONFIG_VIDEO_AVI)
			if (0 == os_strcmp(argv[2], "aud_open"))
			{
				ret = bk_aud_intf_mic_save_start();
				LOGI("bk_aud_intf_mic_save_start\r\n");
			}

			if (0 == os_strcmp(argv[2], "aud_close"))
			{
				ret = bk_aud_intf_mic_save_stop();
				LOGI("bk_aud_intf_mic_save_stop\r\n");
			}
#endif
		}
	}

output:

	if (ret == UNKNOW_ERROR)
	{
		LOGE("%s unknow cmd\n", __func__);
	}

	if (ret == kParamErr)
	{
		LOGE("%s param error cmd\n", __func__);
	}

	if (ret != BK_OK)
	{
		msg = CLI_CMD_RSP_ERROR;
	}
	else
	{
		msg = CLI_CMD_RSP_SUCCEED;
	}

	LOGI("%s ---\n", __func__);

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define MEDIA_CMD_CNT   (sizeof(s_media_commands) / sizeof(struct cli_command))

static const struct cli_command s_media_commands[] =
{
	{"media", "meida...", media_cli_test_cmd},
};

int media_cli_init(void)
{
	return cli_register_commands(s_media_commands, MEDIA_CMD_CNT);
}


