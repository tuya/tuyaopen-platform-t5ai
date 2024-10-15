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

#include "media_mailbox_list_util.h"
#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_LCD_INTF_BASE              (BK_ERR_LCD_BASE - 0x50)

#define BK_ERR_LCD_INTF_OK                BK_OK                               /**< ok */
#define BK_ERR_LCD_INTF_FAIL              BK_FAIL                             /**< fail */
#define BK_ERR_LCD_INTF_NO_MEM            (BK_ERR_LCD_INTF_BASE - 1)          /**< audio interface module status is error */
#define BK_LCD_DECODE_ERR                 (BK_ERR_LCD_INTF_BASE - 2)          /**< parameter is error */
#define BK_LCD_DECODE_TIMEOUT              (BK_ERR_LCD_INTF_BASE - 3)          /**< malloc memory fail */

#define  USE_DMA2D_BLEND_ISR_CALLBACKS  1

typedef enum
{
	DISPLAY_EVENT,
	LOAD_JPEG_EVENT,
	DISPLAY_EXIT_EVENT,
} display_event_t;

typedef struct
{
	const lcd_device_t *lcd_device;      /**< lcd open  lcd device */
	uint16_t lcd_width;
	uint16_t lcd_height;
	uint8_t enable : 1;
	uint8_t decoder_en : 1;
	uint8_t rotate_en : 1;
	uint8_t scale_en : 1;
	uint8_t display_en : 1;
	
	uint8_t debug : 1;
	uint8_t step_mode : 1;
	uint8_t step_trigger : 1;
	uint8_t font_draw  : 1;
	uint8_t dma2d_blend : 1;
	uint8_t picture_echo : 1;
	uint8_t use_gui : 1;
	uint8_t sw_decode : 1;
	uint8_t hw_yuv2rgb : 1;
	uint8_t result;
    uint8_t jpg_fmt_check: 1;  //first check jpeg fmt is YUV422 and YUV420

	media_decode_mode_t decode_mode;
	media_rotate_mode_t rotate_mode;
	media_rotate_t rotate;
	pixel_format_t fmt;          /**< display module input data format */
	media_ppi_t scale_ppi;
	media_lcd_state_t state;

	frame_buffer_t *decoder_frame;
	frame_buffer_t *rotate_frame;
	frame_buffer_t *scale_frame;
	frame_buffer_t *display_frame;
	frame_buffer_t *lvgl_frame;
	frame_buffer_t *pingpong_frame;

	beken_semaphore_t disp_sem;
	beken_mutex_t dec_lock;
	beken_mutex_t rot_lock;
	beken_mutex_t scale_lock;
	beken_mutex_t disp_lock;

	void (*fb_free) (frame_buffer_t *frame);
	frame_buffer_t *(*fb_malloc) (uint32_t size);
} lcd_info_t;


typedef struct
{
	lcd_open_t lcd_open;  /**< lcd device config */
	uint8_t    decode_open;
	uint8_t    rotate_open;
	media_decode_mode_t decode_mode;
	media_rotate_mode_t rotate_mode;
	media_rotate_t      rotate_angle;
	uint8_t hw_yuv2rgb;
	uint8_t use_gui;
} lcd_config_t;




#define DEFAULT_LCD_CONFIG() {                              \
        .lcd_open = {                                       \
                       .device_ppi = PPI_480X854,          \
                       .device_name = "lcd_st7701sn",         \
                    },                                      \
        .decode_open = 1,                                   \
        .rotate_open = 1,                                   \
        .decode_mode =  HARDWARE_DECODING ,                 \
	    .rotate_mode = /*only set there*/ HW_ROTATE,        \
	    .rotate_angle = ROTATE_NONE,                        \
        .hw_yuv2rgb = 1,                                    \
        .use_gui = 0,                                       \
    }




typedef struct {
	uint32_t image_addr;  /**< normally flash jpeg  addr */
	uint32_t img_length;
	uint16_t x_start; 
	uint16_t y_start;
	uint16_t x_end;
	uint16_t y_end;
	uint8_t  display_type;
} lcd_display_t;

///added end


void lcd_event_handle(media_mailbox_msg_t *msg);

media_lcd_state_t get_lcd_state(void);
void set_lcd_state(media_lcd_state_t state);

uint8_t get_decode_mode(void);
void set_decode_mode(uint8_t mode);

void lcd_init(void);

//void lcd_frame_complete_notify(frame_buffer_t *buffer);

//void lcd_jpeg_dec_sw(uint32_t param);



void lcd_decoder_task_stop(void);
void camera_display_task_stop(void);
void camera_display_task_start(media_rotate_t rotate);
void jpeg_display_task_start(media_rotate_t rotate);
void jpeg_display_task_stop(void);
bk_err_t lcd_driver_display_frame(frame_buffer_t *frame);


frame_buffer_t *lcd_driver_rotate_frame(frame_buffer_t *frame, media_rotate_t rotate);
frame_buffer_t *lcd_driver_decoder_frame(frame_buffer_t *frame, media_decode_mode_t decode_mode);

bk_err_t lcd_display_echo_event_handle(media_mailbox_msg_t *msg);
frame_buffer_t *lcd_driver_scale_frame(frame_buffer_t *frame, media_ppi_t ppi);


#ifdef __cplusplus
}
#endif


