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

#pragma once

#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>
#include <modules/lcd_font.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  USE_DMA2D_BLEND_ISR_CALLBACKS  1


typedef enum {
	WIFI_LEVEL_0 = 0,
	WIFI_LEVEL_1,
	WIFI_LEVEL_2,
	WIFI_LEVEL_3,
	WIFI_LEVEL_FULL,
	WIFI_LEVEL_MAX
}WIFI_LEVVEL_T;

//added for blend 
#define MAX_BLEND_NAME_LEN 16
typedef struct {
	uint8_t time_data[MAX_BLEND_NAME_LEN]; 
	uint8_t ver_data[MAX_BLEND_NAME_LEN]; 
	uint8_t year_to_data[MAX_BLEND_NAME_LEN]; 
	uint8_t wifi_data; 
	uint8_t lcd_blend_type; 
} lcd_blend_data_t;

typedef enum {
	LCD_BLEND_NONE=0x0,
	LCD_BLEND_WIFI=0x01,
	LCD_BLEND_TIME=0x02,
	LCD_BLEND_VERSION=0x04,
	LCD_BLEND_DATA=0x08,
} lcd_blend_type_em;

typedef struct {
	uint8_t data[MAX_BLEND_NAME_LEN]; 
	uint8_t blend_on;
	lcd_blend_type_em lcd_blend_type; 
} lcd_blend_msg_t;


/** lcd blend config */
typedef struct
{
	void *pfg_addr;                /**< lcd blend background addr */
	void *pbg_addr;                /**< lcd blend foregound addr */
	uint16_t bg_width;             /**< background img width*/
	uint16_t bg_height;            /**< background img height*/
	uint32_t fg_offline;           /**< foregound addr offset */
	uint32_t bg_offline;           /**< background addr offset*/
	uint32 xsize;                  /**< lcd blend logo width */
	uint32 ysize;                  /**< lcd blend logo height */
	uint8_t fg_alpha_value;        /**< foregound logo alpha value,depend on alpha_mode*/
	uint8_t bg_alpha_value;        /**< background logo alpha value,depend on alpha_mode*/
	data_format_t fg_data_format;  /**< foregound data format */
	pixel_format_t bg_data_format; /**< background data format */
	uint16_t xpos;               /**< blend to bg  x pos based on bg_width */
	uint16_t ypos;               /**< blend to bg  y pos based on bg_height */
	uint16_t lcd_width;         /**< the lcd width */
	uint16_t lcd_height;         /**< the lcd width */
	uint8_t flag; /**< background data format */
	uint8_t blend_rotate;
}lcd_blend_t;

typedef struct
{
	const char * str;              /**< background data format */
	font_colot_t font_color;       /**< 1: white; 0:black */
	const gui_font_digit_struct * font_digit_type;                  /**< lcd blend logo width */
	int x_pos;                    /**< based on param xsize, to config really draw pos, value 0 is draw in start  xsize */
	int y_pos;                    /**< based on param ysize, to config really draw pos, value 0 is draw in start  xsize */
}font_str_t;



typedef struct
{
	void *pbg_addr;                /**< lcd draw font foregound addr */
	uint32_t bg_offline;           /**< background addr offset*/
	uint16_t bg_width;             /**< background img width*/
	uint16_t bg_height;            /**< background img height*/
	pixel_format_t bg_data_format; /**< background data format */
	uint32 xsize;                  /**< lcd draw font logo width */
	uint32 ysize;                  /**< lcd draw font logo height */
	uint8_t str_num;
	font_str_t str[3];
	font_format_t font_format;
	uint8_t font_rotate;
}lcd_font_config_t;


bk_err_t lcd_driver_blend(lcd_blend_t *lcd_blend);
void dma2d_memcpy_psram(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline);
bk_err_t lcd_dma2d_driver_blend(lcd_blend_t *lcd_blend);
bk_err_t lcd_blend_malloc_buffer(void);
bk_err_t lcd_blend_free_buffer(void);

bk_err_t lcd_dma2d_blend_init(void);
bk_err_t lcd_dma2d_blend_deinit(void);
bk_err_t lcd_font_blend_deinit(void);
bk_err_t lcd_font_blend_init(void);

bk_err_t lcd_driver_font_blend(lcd_font_config_t *lcd_font);

#ifdef __cplusplus
}
#endif
