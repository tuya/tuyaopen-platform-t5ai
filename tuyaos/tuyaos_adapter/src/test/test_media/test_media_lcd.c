/*
 * test_media_lcd.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "test_media.h"
#include "tal_display_service.h"

STATIC TY_DISPLAY_HANDLE tkl_test_lcd_handle = NULL;
STATIC USHORT_T *p_frame_buff = NULL;
STATIC USHORT_T *p_frame_buff1 = NULL;

static const ty_display_cfg ai_board_rgb_cfg = {
    .rgb_cfg = {
        .spi_csx = TUYA_GPIO_NUM_48,
        .spi_clk = TUYA_GPIO_NUM_49,
        .spi_sda = TUYA_GPIO_NUM_50,
        .bl = {
            .pin = TUYA_GPIO_NUM_9,
            .active_level = TUYA_GPIO_LEVEL_HIGH
        },
        .reset = {
            .pin = TUYA_GPIO_NUM_53,
        },
        .power_ctrl = {
            .pin = TUYA_GPIO_NUM_MAX,
        },
    }
};

static void set_random_color(USHORT_T *p_buff, uint16_t w, uint16_t h)
{
    USHORT_T color = 0;
    int i = 0;

    color = tkl_system_get_random(0xFFFFFFFF);

    for(i = 0; i < w * h; i++) {
        p_buff[i] = color;
    }
    bk_printf("--- trace %s %d, %x\r\n", __func__, __LINE__, color);
}

static TKL_THREAD_HANDLE test_random_test_thread = NULL;
static VOID_T func_random_test(VOID_T *args)
{
    ty_display_device_s *lcd_handle = (ty_display_device_s *)args;

    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
    uint16_t width = lcd_handle->rgb.cfg.width;
    uint16_t height = lcd_handle->rgb.cfg.height;

    p_frame_buff = tkl_system_psram_malloc(width * height * 2);
    p_frame_buff1 = tkl_system_psram_malloc(width * height * 2);
    if(NULL == p_frame_buff || NULL == p_frame_buff1) {
        bk_printf("malloc failed\r\n");
        return;
    }

    ty_frame_buffer_t ty_frame_buff = {.type = 1, .fmt = 0, .width =width, .height = height, .free_cb = NULL, .len = width * height * 2, .frame = p_frame_buff};
    ty_frame_buffer_t ty_frame_buff1 = {.type = 1, .fmt = 0, .width =width, .height = height, .free_cb = NULL, .len = width * height * 2, .frame = p_frame_buff1};
    bk_printf("frame_buf addr : %p  %p *****..**8\r\n", p_frame_buff,&ty_frame_buff);
    bk_printf("frame_buf addr : %p  %p *****..**8\r\n", p_frame_buff1,&ty_frame_buff1);
    USHORT_T *buf = NULL;
    uint32_t cnt = 0;
    while(1) {
        if(++cnt % 2 == 0) {
            set_random_color(p_frame_buff, width, height);
            tal_display_flush(lcd_handle, &ty_frame_buff);
        }else {
          set_random_color(p_frame_buff1, width, height);
          tal_display_flush(lcd_handle, &ty_frame_buff1);
        }

        bk_printf("flush...**..%d\r\n", cnt);
        tkl_system_sleep(1000);
        bk_printf("*********%d\r\n", cnt);
    }
}

void test_media_lcd_open(int pipeline)
{
    extern VOID *tdd_lcd_driver_query(CONST CHAR_T *name, UINT_T type);
    TY_DISPLAY_HANDLE *dev = (TY_DISPLAY_HANDLE *)tdd_lcd_driver_query("t35p128cq", DISPLAY_RGB);

    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
    tkl_test_lcd_handle = tal_display_open(dev, &ai_board_rgb_cfg);
    if (tkl_test_lcd_handle == NULL) {
        bk_printf("tal_display_open error\r\n");
        return;
    }
    tal_display_bl_open(tkl_test_lcd_handle);
    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);

    // tkl_vi_set_lcd(tkl_test_lcd_handle);
    // tkl_thread_create(&test_random_test_thread, "random", 2048, 6, func_random_test, tkl_test_lcd_handle);
    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
}

void test_media_lcd_close(void)
{
    bk_printf("TODO ... %s, %d\r\n", __func__, __LINE__);
    return;
}

void test_media_lcd_rotate(void)
{
    bk_printf("TODO ... %s, %d\r\n", __func__, __LINE__);
}


