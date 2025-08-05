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
#include <driver/lcd.h>
#include <modules/lcd_font.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  USE_DMA2D_BLEND_ISR_CALLBACKS  0
#define  USE_DMA2D_BLEND_IMAGE          0

#define DRAW_IMG_DECLARE(name) extern const bk_blend_t name;

/***< default malloc size 15KB in sram, if blend size is over, change malloc in psram */
#define BLEND_MALLOC_SRAM           0
#define LCD_BLEND_MALLOC_SIZE      (1024 * 15)
#define LCD_BLEND_MALLOC_RGB_SIZE  (0)
#define MAX_BLEND_NAME_LEN    20
#define MAX_BLEND_CONTENT_LEN 31



/**< struct image blend used by api bk_image_blend(image_blend_cfg_t *cfg), include dma2d blend and cpu draw blend*/
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
} image_blend_cfg_t;

/**< struct font draw used by api bk_font_blend(font_blend_cfg_t *cfg), cpu draw blend*/
typedef struct
{
    const char *str;               /**< background data format */
    font_colot_t font_color;       /**< 1: white; 0:black */
    const gui_font_digit_struct *font_digit_type;                   /**< lcd blend logo width */
    int x_pos;                    /**< based on param xsize, to config really draw pos, value 0 is draw in start  xsize */
    int y_pos;                    /**< based on param ysize, to config really draw pos, value 0 is draw in start  xsize */
} font_str_t;
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
    uint16_t xpos;               /**< blend to bg  x pos based on bg_width */
    uint16_t ypos;               /**< blend to bg  y pos based on bg_height */
    uint16_t lcd_width;         /**< the lcd width */
    uint16_t lcd_height;         /**< the lcd width */
    uint8_t font_rotate;
} font_blend_cfg_t;

bk_err_t bk_image_blend(image_blend_cfg_t *cfg);
bk_err_t bk_font_blend(font_blend_cfg_t *cfg);

bk_err_t bk_blend_init(void);
bk_err_t bk_blend_deinit(void);
bk_err_t bk_dma2d_image_blend(image_blend_cfg_t *cfg);



/*========================================user may used API==========================================================*/
typedef enum
{
     BLEND_TYPE_IMAGE = 0,
     BLEND_TYPE_FONT,
}blend_type_t;

typedef struct {
    uint8_t format;             /**< data_format_t, should be ARGB8888 (no used)                    */
    uint32_t data_len;          /**< ARGB8888 image size, should be: (xsize * ysize * 4) (no used)  */
    const uint8_t *data;        /**< ARGB8888 image data                                  */
}blend_image_t;

typedef struct {
    const gui_font_digit_struct *const font_digit_type;   /**< character database */
    uint32_t color;            /**< font color value used by RGB565 date*/
}blend_font_t;

typedef struct 
{
    uint8_t version;
    blend_type_t blend_type;       /**< 0: image, 1:font */
    const char name[MAX_BLEND_NAME_LEN];        /**< image name like "wifi","clock", "weather" */
    uint32_t width;         /**< icon width   */
    uint32_t height;        /**< icon height  */
    uint16_t xpos;        /**< blend to lcd, x pos based on lcd width  */
    uint16_t ypos;        /**< blend to lcd, y pos based on lcd height */
    union
    {
        blend_image_t image;
        blend_font_t font;
    };
}bk_blend_t;


typedef struct 
{
    char name[MAX_BLEND_NAME_LEN];
    //const bk_blend_t *(*find_addr)[];  //the pointer, pointer to the struct pointer array
    const bk_blend_t *addr;            //the pointer, pointer to the struct
    char content[MAX_BLEND_CONTENT_LEN];
}blend_info_t;

typedef struct{
    blend_info_t *entry;
    size_t size;
    size_t capacity;
}dynamic_array_t;

extern dynamic_array_t g_dyn_array;

/**
 * @brief  blend icon of ARGB888 image by cpu or hardware dma2d
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @param  struct addr of image type bk_blend_t
 * @example:
 *          extern GUI_CONST_STORAGE bk_blend_t img_wifi_rssi1;
 *          bk_display_blend_img_handle(frame, lcd_w, lcd_h, &img_wifi_rssi1);
 *
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_img_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const bk_blend_t *img_info);

/**
 * @brief  draw font by cpu
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @param  struct addr of image type blend_info_t
 * @example:
 *          extern GUI_CONST_STORAGE bk_blend_t font_clock;
 *          blend_info_t blend_info = {.name = clock, .addr = &font_clock, .content = "12:30"};
 *          bk_display_blend_font_handle(frame, lcd_w, lcd_h, &blend_info);
 *
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_font_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *font_info);

/**
 * @brief  blend array include image and font(no used), use API bk_display_blend_handle replease
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @param  struct array add
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_handle_by_array(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *array, uint8_t array_size);

/**
 * @brief  blend array include image and font
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @example:
 *          extern GUI_CONST_STORAGE bk_blend_t blend_info[];
 *          GUI_CONST_STORAGE blend_info_t blend_info[] =
 *          {
 *              {.name = "clock", addr = &font_clock, .content = "12:30"},
 *              {.name = "date", .addr = &font_dates, .content = "2025/1/2 周四"},
 *              {.name = "ver", .addr = &font_ver, .content = "v 1.0.0"},
 *              {.name = "wifi",.addr = &img_wifi_rssi0, .content = "wifi1"},
 *              {.addr = NULL}
 *          }
 *          bk_display_blend_handle(frame, lcd_w, lcd_h, &blend_info[0]);

 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *array_ptr);


bk_err_t bk_draw_blend_init(void);

bk_err_t bk_draw_blend_deinit(void);

bk_err_t bk_draw_blend_update(blend_info_t *blend);

void get_blend_assets_array(const blend_info_t *assets);

void get_blend_default_array(const blend_info_t *assets);

#ifdef __cplusplus
}
#endif

