/*
 * test_media.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli_tuya_test.h"

#include <os/os.h>
#include "media_app.h"
#include "media_evt.h"
#include "tkl_display.h"
#include "tkl_thread.h"
#include "tkl_audio.h"
#include "tkl_video_in.h"
#include "tkl_lvgl.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"
#include "test_media.h"

#define DEV_CLOSED      0
#define DEV_OPEN        1



static volatile uint8_t rotate = 3;

static void __test_app_lcd_component(void);

/**********************************  lvgl  **********************************/
void __attribute__((weak)) app_recv_lv_event(UCHAR_T *buf, UINT_T len, VOID *args)
{
    os_printf("%s , this function should be defined in app\r\n", __func__);
}

static void __test_media_open_lvgl(void)
{
#if 0
    TKL_DISP_INFO_S lvgl_info;

    uint8_t stat = tkl_disp_get_lcd_state();
    if (!stat) {
        os_printf("open lcd first\r\n");
        return;
    }

    extern uint8_t tkl_lvgl_get_status(void);
    uint8_t lv_status = tkl_lvgl_get_status();
    if (lv_status) {
        os_printf("lvgl alerady opened\r\n");
        return;
    }

    memset(&lvgl_info, 0, sizeof(TKL_DISP_INFO_S));

    test_media_get_lcd_ppi(&lvgl_info.width, &lvgl_info.height);
    os_printf("%s , %d %d\r\n", __func__, lvgl_info.width, lvgl_info.height);

    memcpy(lvgl_info.ll_ctrl.ic_name, "lcd123456", 9);

    TKL_LVGL_CFG_T lv_cfg = {
        .recv_cb = app_recv_lv_event,
        .recv_arg = NULL,
    };
    tkl_lvgl_init(&lvgl_info, &lv_cfg);

    tkl_lvgl_start();
#endif
}

extern VOID __tuya_lcd_test_func(void);
static void __test_app_lcd_component(void)
{
    // __tuya_lcd_test_func();
}

extern void bk_printf_raw(int level, char *tag, const char *fmt, ...);
extern void test_custom_dvp_open(int usage);
void cli_tuya_media_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("Usage: xmt open|close lvgl|uvc|lcd|h264|audio\r\n");
        return;
    }
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf_raw(0, NULL, " %s", argv[i]);
    }
    bk_printf_raw(0, NULL, "\r\n");

    uint32_t tick = xTaskGetTickCount();
    if (tick < 1000) {
        bk_printf("Wait startup complete, ignore\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "open")) {
        //lpmgr_register(TY_LP_KEEP_ALIVE);
        if (argc == 2) {
            bk_printf("no spec open parameter\r\n");
        } else if (!os_strcmp(argv[2], "uvc")) {
         // test_media_camera_open(TKL_VI_CAMERA_TYPE_UVC);
            bk_printf("TODO\r\n");
        } else if (!os_strcmp(argv[2], "dvp")) {
            test_media_camera_open(TKL_VI_CAMERA_TYPE_DVP);
        } else if (!os_strcmp(argv[2], "lvgl")) {
            __test_media_open_lvgl();
        } else if (!os_strcmp(argv[2], "lcd")) {
            bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
            if (argv[3] == NULL) {
                bk_printf("no spec pipeline\r\n");
                return;
            } else {
                bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
                if (!os_strcmp(argv[3], "0")) {
                    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
                    test_media_lcd_open(0);
                } else if (!os_strcmp(argv[3], "1")) {
                    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
                    test_media_lcd_open(1);
                } else {
                    bk_printf("parameter error\r\n");
                }
                bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
            }
        } else if (!os_strcmp(argv[2], "h264")) {
            test_media_h264_open();
#ifdef CONFIG_VOICE_SERVICE
        } else if (!os_strcmp(argv[2], "audio")) {
            test_media_audio_open(AUDIO_DUAL);
        } else if (!os_strcmp(argv[2], "mic")) {
            test_media_audio_open(AUDIO_MIC);
        } else if (!os_strcmp(argv[2], "spk")) {
            test_media_audio_open(AUDIO_SPK);
        }
#endif
        else if (!os_strcmp(argv[2], "custom_dvp")) {
            bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
            if (argv[3] == NULL) {
                bk_printf("not specify the dvp purpose\r\n");
                return;
            } else {
                if (!os_strcmp(argv[3], "display")) {
                    test_custom_dvp_open(0);
                } else if (!os_strcmp(argv[3], "h264")) {
                    test_custom_dvp_open(1);
                } else if (!os_strcmp(argv[3], "mjpeg")) {
                    test_custom_dvp_open(2);
                } else if (!os_strcmp(argv[3], "display_h264")) {
                    test_custom_dvp_open(3);
                } else if (!os_strcmp(argv[3], "display_mjepg")) {
                    test_custom_dvp_open(4);
                } else {
                    bk_printf("parameter error\r\n");
                }
            }
        }
    } else if (!os_strcmp(argv[1], "close")) {
        if (argc == 2) {
            bk_printf("no spec close parameter\r\n");
        // } else if (!os_strcmp(argv[2], "uvc")) {
        //     test_media_camera_close(TKL_VI_CAMERA_TYPE_UVC);
        // } else if (!os_strcmp(argv[2], "dvp")) {
        //     test_media_camera_close(TKL_VI_CAMERA_TYPE_DVP);
        } else if (!os_strcmp(argv[2], "lvgl")) {
            // tkl_lvgl_stop();
            bk_printf("TODO lvgl\r\n");
        // } else if (!os_strcmp(argv[2], "lcd")) {
        //     test_media_lcd_close();
#ifdef CONFIG_VOICE_SERVICE
        } else if (!os_strcmp(argv[2], "audio")) {
            test_media_audio_close();
        } else if (!os_strcmp(argv[2], "mic")) {
            test_media_audio_close();
        } else if (!os_strcmp(argv[2], "spk")) {
            test_media_audio_close();
#endif
        // } else if (!os_strcmp(argv[2], "h264")) {
        //     test_media_h264_close();
        }
        //lpmgr_unregister(TY_LP_KEEP_ALIVE);
    } else if (!os_strcmp(argv[1], "rotate")) {
        bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);
#if 0
        uint8_t stat = tkl_disp_get_lcd_state();
        if (stat) {
            test_media_lcd_rotate();
        } else {
            bk_printf("lcd not init\r\n");
        }
#endif
    } else if (!os_strcmp(argv[1], "ac")) {
        __test_app_lcd_component();
    } else {
        bk_printf("Usage: xmt rotate\r\n");
        bk_printf("       xmt set [lcd_name]\r\n");
        bk_printf("       xmt open|close lvgl|uvc|lcd|h264|audio\r\n");
    }
    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
    return;
}




