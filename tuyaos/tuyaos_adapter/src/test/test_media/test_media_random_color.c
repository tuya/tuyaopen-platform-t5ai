/*
 * test_media_random_color.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "test_media.h"
#include "cli_tuya_test.h"

 #include "tkl_system.h"
 #include "tkl_rgb.h"

 #include "ty_frame_buff.h"
 #include "tal_display_service.h"
/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/
#define LCD_WIDTH   320
#define LCD_HEIGHT  480

#define DISPLAY_LCD_BL_PIN 9
#define DISPLAY_LCD_BL_POLARITY_LEVEL TUYA_GPIO_LEVEL_HIGH
/***********************************************************
***********************variable define**********************
***********************************************************/
/* app thread handle */
STATIC THREAD_HANDLE ty_app_thread = NULL;


STATIC USHORT_T *p_frame_buff = NULL;


const uint8_t cILI9488_INIT_SEQ[] = {
    3,  0,   ILI9488_PWCTR1,   0x0E, 0x0E,
    2,  0,   ILI9488_PWCTR2,   0x46,
    4,  0,   ILI9488_VMCTR1,   0x00, 0x2D, 0x80,
    2,  0,   ILI9488_IFMODE,   0x00,
    2,  0,   ILI9488_FRMCTR1,  0xA0,
    2,  0,   ILI9488_INVCTR,   0x02,
    5,  0,   ILI9488_PRCTR,    0x08, 0x0C, 0x50, 0x64,
    3,  0,   ILI9488_DFUNCTR,  0x32, 0x02,
    2,  0,   ILI9488_MADCTL,   0x48,
    2,  0,   ILI9488_PIXFMT,   0x70,
    2,  0,   ILI9488_INVON,    0x00,
    2,  0,   ILI9488_SETIMAGE, 0x01,
    5,  0,   ILI9488_ACTRL3,   0xA9, 0x51, 0x2C, 0x82,
    3,  0,   ILI9488_ACTRL4,   0x21, 0x05,
    16, 0,   ILI9488_GMCTRP1,  0x00, 0x0C, 0x10, 0x03, 0x0F, 0x05, 0x37, 0x66, 0x4D, 0x03, 0x0C, 0x0A, 0x2F, 0x35, 0x0F,
    16, 0,   ILI9488_GMCTRN1,  0x00, 0x0F, 0x16, 0x06, 0x13, 0x07, 0x3B, 0x35, 0x51, 0x07, 0x10, 0x0D, 0x36, 0x3B, 0x0F,
    1,  120, ILI9488_SLPOUT,
    1,  20,  ILI9488_DISPON,
    0,
};

/***********************************************************
***********************function define**********************
***********************************************************/

void ty_rgb_isr(ty_rgb_event_e event)
{
    TAL_PR_NOTICE("ty_rgb_isr event:%d", event);
}

void set_random_color(USHORT_T *p_buff)
{
    USHORT_T color = 0;
    int i = 0;

    color = tkl_system_get_random(0xFFFFFFFF);

    for(i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        p_buff[i] = color;
    }
}

void user_main(void)
{
#if 0
    tkl_rgb_init(&lcd_ili9488_rgb);
    tkl_rgb_irq_cb_register(ty_rgb_isr);
#else
    TY_DISPLAY_HANDLE *test_lcd_dev = tal_display_open(&i8080_st7796S_device, &i7796_cfg);
    tal_display_bl_open(test_lcd_dev);
#endif

    p_frame_buff = tkl_system_psram_malloc(LCD_WIDTH * LCD_HEIGHT*2);
    USHORT_T *p_frame_buff1 = tkl_system_psram_malloc(LCD_WIDTH * LCD_HEIGHT*2);
    if(NULL == p_frame_buff || NULL == p_frame_buff1) {
        TAL_PR_ERR("malloc failed");
        return;
    }

    ty_frame_buffer_t ty_frame_buff = {.type = 1, .fmt = 0, .width =LCD_WIDTH, .height = LCD_HEIGHT, .free_cb = NULL, .len = LCD_WIDTH * LCD_HEIGHT * 2, .frame = p_frame_buff};
    ty_frame_buffer_t ty_frame_buff1 = {.type = 1, .fmt = 0, .width =LCD_WIDTH, .height = LCD_HEIGHT, .free_cb = NULL, .len = LCD_WIDTH * LCD_HEIGHT * 2, .frame = p_frame_buff1};
    bk_printf("frame_buf addr : %p  %p *****..**8\r\n", p_frame_buff,&ty_frame_buff);
    bk_printf("frame_buf addr : %p  %p *****..**8\r\n", p_frame_buff1,&ty_frame_buff1);
    USHORT_T *buf = NULL;
    uint32_t cnt = 0;
    while(1) {
        if(++cnt % 2 == 0) {
            set_random_color(p_frame_buff);
            tal_display_flush(test_lcd_dev, &ty_frame_buff);
        }else {
          set_random_color(p_frame_buff1);
          tal_display_flush(test_lcd_dev, &ty_frame_buff1);
        }

        bk_printf("flush...**..%d\r\n", cnt);
        tkl_system_sleep(3000);
        bk_printf("*********%d\r\n", cnt);

        #if 0
        if(cnt % 60 == 0) {
            bk_printf("close rgb.....%d\r\n", cnt);
            tal_display_close(test_lcd_dev);

            tkl_system_sleep(10000);
            bk_printf("close open.....\r\n");
            test_lcd_dev = tal_display_open(&i8080_st7789P3_device, &st7789p3_cfg);
            tal_display_bl_open(test_lcd_dev);

        }
        #endif
    }

}


/**
* @brief  task thread
*
* @param[in] arg:Parameters when creating a task
* @return none
*/
STATIC VOID_T tuya_app_thread(VOID_T *arg)
{
    /* Initialization LWIP first!!! */
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    TUYA_LwIP_Init();
#endif

    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

/**
 * @brief user entry function
 *
 * @param[in] none:
 *
 * @return none
 */
#if OPERATING_SYSTEM == SYSTEM_LINUX
INT_T main(INT_T argc, CHAR_T **argv)
#else
VOID_T tuya_app_main(VOID)
#endif
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
#if OPERATING_SYSTEM == SYSTEM_LINUX
    while (1) {
        tal_system_sleep(1000);
    }
#endif
}

