
// Copyright 2023-2024 Beken
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
#include <os/mem.h>
#include <os/str.h>
#include "media_mailbox_list_util.h"

#include <driver/int.h>
#include <driver/rott_driver.h>

#include "frame_buffer.h"
#include <driver/gpio.h>
#include "media_evt.h"

#include <driver/media_types.h>
#include "modules/image_scale.h"
#include <driver/dma2d.h>
#include <driver/dma2d_types.h>
#include <driver/psram.h>
#include "draw_blend.h"
#include "lcd_draw_blend.h"

#include <blend_logo.h>

#include "bk_list.h"

#define TAG "draw_blend"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#ifdef DRAW_DIAG_DEBUG
#define DRAW_START()		do { GPIO_UP(GPIO_DVP_D3); } while (0)
#define DRAW_END()			do { GPIO_DOWN(GPIO_DVP_D3); } while (0)
#else
#define DRAW_START()
#define DRAW_END()
#endif

static lcd_blend_data_t g_blend_data = {0};

bk_err_t lcd_dma2d_handle(frame_buffer_t *frame,  uint16_t lcd_width, uint16_t lcd_height)
{
#if (CONFIG_LCD_DMA2D_BLEND)
    lcd_blend_t lcd_blend = {0};

    if(g_blend_data.lcd_blend_type == 0)
    {
        return BK_OK;
    }
    if ((g_blend_data.lcd_blend_type & LCD_BLEND_WIFI) != 0)      /// start display lcd (xpos,ypos)
    {
        if(g_blend_data.wifi_data > WIFI_LEVEL_MAX - 1)
            return BK_FAIL;
        lcd_blend.pfg_addr = (uint8_t *)wifi_logo[g_blend_data.wifi_data];
        lcd_blend.pbg_addr = (uint8_t *)(frame->frame);
        lcd_blend.xsize = WIFI_LOGO_W;
        lcd_blend.ysize = WIFI_LOGO_H;
        lcd_blend.xpos = lcd_width - WIFI_LOGO_W;
        lcd_blend.ypos = WIFI_LOGO_YPOS;
        lcd_blend.fg_alpha_value = FG_ALPHA;
        lcd_blend.fg_data_format = ARGB8888;
        lcd_blend.bg_data_format = frame->fmt;
        lcd_blend.bg_width = frame->width;
        lcd_blend.bg_height = frame->height;
        lcd_blend.lcd_width = lcd_width;
        lcd_blend.lcd_height = lcd_height;
        lcd_dma2d_driver_blend(&lcd_blend);
    }
#endif
return BK_OK;
}

bk_err_t lcd_font_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height)
{
#if (CONFIG_LCD_FONT_BLEND)
    lcd_font_config_t lcd_font_config = {0};

    uint16_t start_x = 0;
    uint16_t start_y = 0;
    uint32_t frame_addr_offset = 0;
    uint8_t pixel_bytes;
    if(g_blend_data.lcd_blend_type == 0)
    {
        return BK_OK;
    }

    if ((lcd_width < frame->width) || (lcd_height < frame->height)) //for lcd size is small then frame image size
    {
        if (lcd_width < frame->width)
            start_x = (frame->width - lcd_width) / 2;
        if (lcd_height < frame->height)
            start_y = (frame->height - lcd_height) / 2;
    }
    if (frame->fmt == PIXEL_FMT_RGB888)
        pixel_bytes = 3;
    else
        pixel_bytes = 2;

    if ((g_blend_data.lcd_blend_type & LCD_BLEND_TIME) != 0)         /// start display lcd (0,0)
    {
		// Modified by TUYA Start
        frame_addr_offset = ((start_y + lcd_height - 160) * frame->width + start_x + (lcd_width - CLOCK_LOGO_W)/ 2) * pixel_bytes; // - CLOCK_LOGO_W;
		// Modified by TUYA End

        lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
        lcd_font_config.bg_offline = frame->width - CLOCK_LOGO_W;
        lcd_font_config.xsize = CLOCK_LOGO_W;
        lcd_font_config.ysize = CLOCK_LOGO_H;
        lcd_font_config.str_num = 1;
        if (frame->fmt == PIXEL_FMT_VUYY)
            lcd_font_config.font_format = FONT_VUYY;
        else if (frame->fmt == PIXEL_FMT_YUYV)
            lcd_font_config.font_format = FONT_YUYV;
        else
            lcd_font_config.font_format = FONT_RGB565;

        lcd_font_config.str[0] = (font_str_t){(const char *)g_blend_data.time_data, FONT_WHITE, font_digit_Roboto53, 0,0};
        lcd_font_config.bg_data_format = frame->fmt;
        lcd_font_config.bg_width = frame->width;
        lcd_font_config.bg_height = frame->height;
        lcd_driver_font_blend(&lcd_font_config);

		// Modified by TUYA Start
        uint32_t year_len = strlen((const char *)g_blend_data.year_to_data);
        if (year_len != 0) {
            if (year_len > DATA_LOGO_W)
                year_len = DATA_LOGO_W;

            //frame_addr_offset = ((start_y + lcd_height - 160 + CLOCK_LOGO_H + 20) * frame->width + start_x + (lcd_width - year_len)/ 2) * pixel_bytes;
            frame_addr_offset = ((start_y + lcd_height - 160 + CLOCK_LOGO_H + 20) * frame->width + start_x + (lcd_width - DATA_LOGO_W)/ 2) * pixel_bytes;

            lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
            lcd_font_config.bg_offline = frame->width - DATA_LOGO_W;
            lcd_font_config.xsize = DATA_LOGO_W;
            lcd_font_config.ysize = DVP_LOGO_H;
            lcd_font_config.str_num = 1;
            if (frame->fmt == PIXEL_FMT_VUYY)
                lcd_font_config.font_format = FONT_VUYY;
            else if (frame->fmt == PIXEL_FMT_YUYV)
                lcd_font_config.font_format = FONT_YUYV;
            else
                lcd_font_config.font_format = FONT_RGB565;

            lcd_font_config.str[0] = (font_str_t){(const char *)g_blend_data.year_to_data, FONT_WHITE, font_digit_black24, 0, 0};
            lcd_font_config.bg_data_format = frame->fmt;
            lcd_font_config.bg_width = frame->width;
            lcd_font_config.bg_height = frame->height;
            lcd_driver_font_blend(&lcd_font_config);
        }
        // Modified by TUYA Start
	}
#if (CONFIG_SOC_BK7258)
    if ((g_blend_data.lcd_blend_type & LCD_BLEND_WIFI) != 0)      /// start display lcd (lcd_width,0)
    {
        lcd_blend_t lcd_blend = {0};
        LOGD("lcd wifi blend level =%d \n", g_blend_data.wifi_data);
		// Modified by TUYA Start
        frame_addr_offset = (start_y * frame->width + start_x) * pixel_bytes;
		// Modified by TUYA End

        lcd_blend.pfg_addr = (uint8_t *)wifi_logo[g_blend_data.wifi_data];
        lcd_blend.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
        lcd_blend.fg_offline = 0;
        lcd_blend.bg_offline = frame->width - WIFI_LOGO_W;
        lcd_blend.xsize = WIFI_LOGO_W;
        lcd_blend.ysize = WIFI_LOGO_H;
        lcd_blend.fg_alpha_value = FG_ALPHA;
        lcd_blend.fg_data_format = ARGB8888;
        lcd_blend.bg_data_format = frame->fmt;
        lcd_blend.bg_width = frame->width;
        lcd_blend.bg_height = frame->height;
        lcd_driver_blend(&lcd_blend);
    }
#else
        lcd_dma2d_handle(frame, lcd_width, lcd_height);
#endif
    if ((g_blend_data.lcd_blend_type & LCD_BLEND_DATA) != 0)   /// tart display lcd (DATA_POSTION_X,DATA_POSTION_Y)
    {
        if ((DATA_POSTION_X + DATA_LOGO_W) > lcd_width)
            frame_addr_offset = ((start_y + DATA_POSTION_Y + lcd_height - DATA_LOGO_H) * frame->width + start_x) * pixel_bytes;
        else
            frame_addr_offset = ((start_y + DATA_POSTION_Y) * frame->width + start_x + DATA_POSTION_X) * pixel_bytes;
        lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
        lcd_font_config.bg_offline = frame->width - DATA_LOGO_W;
        lcd_font_config.xsize = DATA_LOGO_W;
        lcd_font_config.ysize = DVP_LOGO_H;
        lcd_font_config.str_num = 1;
        if (frame->fmt == PIXEL_FMT_VUYY)
            lcd_font_config.font_format = FONT_VUYY;
        else if (frame->fmt == PIXEL_FMT_YUYV)
            lcd_font_config.font_format = FONT_YUYV;
        else
            lcd_font_config.font_format = FONT_RGB565;

        lcd_font_config.str[0] = (font_str_t){(const char *)("晴转多云, 27℃"), FONT_WHITE, font_digit_black24, 0, 2};
        lcd_font_config.bg_data_format = frame->fmt;
        lcd_font_config.bg_width = frame->width;
        lcd_font_config.bg_height = frame->height;
        lcd_driver_font_blend(&lcd_font_config);

        lcd_font_config.pbg_addr += DVP_LOGO_H * frame->width * 2;
        lcd_font_config.str[0] = (font_str_t){(const char *)("2022-12-12 星期三"), FONT_WHITE, font_digit_black24, 0, 0};
        lcd_driver_font_blend(&lcd_font_config);
    }

    if ((g_blend_data.lcd_blend_type & LCD_BLEND_VERSION) != 0) /// start display lcd (VERSION_POSTION_X,VERSION_POSTION_Y)
    {
		// Modified by TUYA Start
        frame_addr_offset = (start_y * frame->width + start_x + (lcd_width - VERSION_LOGO_W)) * pixel_bytes;
		// Modified by TUYA End
        lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
        lcd_font_config.bg_offline = frame->width - VERSION_LOGO_W;
        lcd_font_config.xsize = VERSION_LOGO_W;
        lcd_font_config.ysize = VERSION_LOGO_H;
        lcd_font_config.str_num = 1;
        if (frame->fmt == PIXEL_FMT_VUYY)
            lcd_font_config.font_format = FONT_VUYY;
        else if (frame->fmt == PIXEL_FMT_YUYV)
            lcd_font_config.font_format = FONT_YUYV;
        else
            lcd_font_config.font_format = FONT_RGB565;

        lcd_font_config.str[0] = (font_str_t){(const char *)g_blend_data.ver_data, FONT_WHITE, font_digit_black24, 0, 0};
        lcd_font_config.bg_data_format = frame->fmt;
        lcd_font_config.bg_width = frame->width;
        lcd_font_config.bg_height = frame->height;
        lcd_driver_font_blend(&lcd_font_config);
    }
    
#endif
    return BK_OK;
}


bk_err_t lcd_blend_font_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	lcd_blend_msg_t  *blend_data = (lcd_blend_msg_t *)msg->param;
	if(blend_data != NULL)
	{
		LOGD("lcd_EVENT_LCD_BLEND_IND type=%d on=%d\n", blend_data->lcd_blend_type, blend_data->blend_on);

		if ((blend_data->lcd_blend_type & LCD_BLEND_WIFI) == LCD_BLEND_WIFI)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_WIFI);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_WIFI;
				g_blend_data.wifi_data = blend_data->data[0];

				LOGD("g_blend_data.wifi_data =%d\n", g_blend_data.wifi_data );
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_TIME) == LCD_BLEND_TIME)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_TIME);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_TIME;
				// Modified by TUYA Start
                int i = 0;
                for (; i < MAX_BLEND_NAME_LEN; i++) {
                    if (blend_data->data[i] == '#') {
                        break;
                    }
                }

                if (i == MAX_BLEND_NAME_LEN) {
                    os_memcpy(g_blend_data.time_data, blend_data->data, MAX_BLEND_NAME_LEN);
                } else {
                    os_memset(g_blend_data.year_to_data, 0, MAX_BLEND_NAME_LEN);
                    os_memcpy(g_blend_data.year_to_data, blend_data->data, i);
                    os_memcpy(g_blend_data.time_data, &blend_data->data[i+1], strlen((const char *)blend_data->data) - i);
                }

				//os_memcpy(g_blend_data.time_data, blend_data->data, MAX_BLEND_NAME_LEN);
				//LOGE("g_blend_data.time_data =%s\n", g_blend_data.time_data );
				LOGD("blend_data.time_data = %s\n", blend_data->data);
				// Modified by TUYA End
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_DATA) == LCD_BLEND_DATA)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_DATA);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_DATA;
				os_memcpy(g_blend_data.year_to_data, blend_data->data, MAX_BLEND_NAME_LEN);
				LOGD("g_blend_data.chs =%s\n", g_blend_data.year_to_data);
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_VERSION) == LCD_BLEND_VERSION)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_VERSION);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_VERSION;
				os_memcpy(g_blend_data.ver_data, blend_data->data, MAX_BLEND_NAME_LEN);
				LOGE("g_blend_data.ver_data =%s\n", g_blend_data.ver_data );
			}
		}
	}
	return ret;
}




