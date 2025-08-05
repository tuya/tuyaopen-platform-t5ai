/*
 * test_media_camera.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "test_media.h"
#include "media_app.h"
#include "media_evt.h"

static TKL_VI_CAMERA_TYPE_E __test_current_camera = CAMERA_CONFIG_IDLE;

static void __test_media_open_uvc(void)
{
    uint8_t uvc_status = 0xff;

    if (__test_current_camera == TKL_VI_CAMERA_TYPE_UVC) {
        bk_printf("uvc already opened\r\n");
        return;
    } else if (__test_current_camera != CAMERA_CONFIG_IDLE) {
        bk_printf("open uvc error, current camera : %d\r\n", __test_current_camera);
        return;
    }

    TKL_VI_CONFIG_T vi_config;
    TKL_VI_EXT_CONFIG_T ext_conf;

    uvc_status = tkl_vi_get_status(UVC_CAMERA);
    if (uvc_status == 1) {
        bk_printf("uvc already opend\r\n");
        return;
    }

    ext_conf.type = TKL_VI_EXT_CONF_CAMERA;
    ext_conf.camera.camera_type = TKL_VI_CAMERA_TYPE_UVC;
    ext_conf.camera.fmt = TKL_CODEC_VIDEO_MJPEG;
    ext_conf.camera.power_pin = TUYA_GPIO_NUM_28;
    ext_conf.camera.active_level = TUYA_GPIO_LEVEL_HIGH;

    vi_config.isp.width = 800;
    vi_config.isp.height = 480;
    vi_config.isp.fps = 15;
    vi_config.pdata = &ext_conf;

    tkl_vi_init(&vi_config, 0);

    __test_current_camera = TKL_VI_CAMERA_TYPE_UVC;
}

static void __test_media_close_uvc(void)
{
    uint8_t uvc_status = 0xff;

    uvc_status = tkl_vi_get_status(UVC_CAMERA);
    if (uvc_status == 1) {
        bk_printf("meida test: close uvc\r\n");
        tkl_vi_uninit(TKL_VI_CAMERA_TYPE_UVC);
        __test_current_camera = CAMERA_CONFIG_IDLE;
        return;
    }
}

static void __test_media_open_dvp(void)
{
    uint8_t uvc_status = 0xff;
    TKL_VI_CONFIG_T vi_config;
    TKL_VI_EXT_CONFIG_T ext_conf;

    if (__test_current_camera == TKL_VI_CAMERA_TYPE_DVP) {
        bk_printf("dvp already opened\r\n");
        return;
    } else if (__test_current_camera != CAMERA_CONFIG_IDLE) {
        bk_printf("open dvp error, current camera : %d\r\n", __test_current_camera);
        return;
    }

    uvc_status = tkl_vi_get_status(DVP_CAMERA);
    if (uvc_status == 1) {
        bk_printf("dvp already opend\r\n");
        return;
    }

    ext_conf.type = TKL_VI_EXT_CONF_CAMERA;
    ext_conf.camera.camera_type = TKL_VI_CAMERA_TYPE_DVP;
    ext_conf.camera.fmt = TKL_CODEC_VIDEO_MJPEG;

#if 0   // bk board
    ext_conf.camera.power_pin = TUYA_GPIO_NUM_28;
    ext_conf.camera.active_level = TUYA_GPIO_LEVEL_HIGH;
    ext_conf.camera.i2c.clk = TUYA_GPIO_NUM_0;
    ext_conf.camera.i2c.sda = TUYA_GPIO_NUM_1;

    vi_config.isp.width = 864;
    vi_config.isp.height = 480;
    vi_config.isp.fps = 25;
#else
    ext_conf.camera.power_pin = TUYA_GPIO_NUM_51;
    ext_conf.camera.active_level = TUYA_GPIO_LEVEL_HIGH;
    ext_conf.camera.i2c.clk = TUYA_GPIO_NUM_13;
    ext_conf.camera.i2c.sda = TUYA_GPIO_NUM_15;
    ext_conf.camera.fps = 15;
    ext_conf.camera.height = 480;
    ext_conf.camera.width = 480;

    vi_config.isp.width = 480;
    vi_config.isp.height = 320;
    vi_config.isp.fps = 15;
#endif

    vi_config.pdata = &ext_conf;

    tkl_vi_init(&vi_config, 0);
    __test_current_camera = TKL_VI_CAMERA_TYPE_DVP;
}

static void __test_media_close_dvp(void)
{
    uint8_t dvp_status = 0xff;

    dvp_status = tkl_vi_get_status(DVP_CAMERA);
    if (dvp_status == 1) {
        bk_printf("meida test: close dvp\r\n");
        __test_current_camera = CAMERA_CONFIG_IDLE;
        tkl_vi_uninit(TKL_VI_CAMERA_TYPE_DVP);
        return;
    }
}

static TKL_THREAD_HANDLE test_dvp_test_thread = NULL;
void test_media_camera_open(TKL_VI_CAMERA_TYPE_E type)
{
    if (type == TKL_VI_CAMERA_TYPE_UVC) {
        __test_media_open_uvc();
    } else if (type == TKL_VI_CAMERA_TYPE_DVP) {
        tkl_thread_create(&test_dvp_test_thread, "dvp", 8192, 6, __test_media_open_dvp, NULL);
    } else {
        bk_printf("Error camera type: %d", type);
    }
}

void test_media_camera_close(TKL_VI_CAMERA_TYPE_E type)
{
    if (type == TKL_VI_CAMERA_TYPE_UVC) {
        __test_media_close_uvc();
    } else if (type == TKL_VI_CAMERA_TYPE_DVP) {
        __test_media_close_dvp();
    } else {
        bk_printf("Error camera type: %d", type);
    }
}

TKL_VI_CAMERA_TYPE_E test_media_camera_get_type(void)
{
    bk_printf("meida test: current camera, %d\r\n", __test_current_camera);
    return __test_current_camera;
}



