/*
 * test_media_h264.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "test_media.h"

#define TEST_H264_DATA_LEN_PER_FRAME  (256 * 1024)  // 实测数据，要根据摄像头设置
#define TEST_H264_FILE_SIZE_LIMITED   (TEST_MAX_FILE_SIZE_LIMITED - TEST_H264_DATA_LEN_PER_FRAME)

static TUYA_FILE h264_file = NULL;
static uint32_t max_h264_frame = 0;
static TKL_VENC_CONFIG_T test_h264_config;
static TaskHandle_t __h264_record_thread = NULL;
static uint32_t __test_media_h264_running = 0;

static int32_t __h264_cb(TKL_VENC_FRAME_T *pframe)
{
    if (max_h264_frame < pframe->buf_size) {
        max_h264_frame = pframe->buf_size;
        bk_printf("%s, max frame size: %d\r\n", __func__, max_h264_frame);
    }

    if (__test_media_h264_running && h264_file) {
        tkl_fwrite(pframe->pbuf, pframe->buf_size, h264_file);
        tkl_fsync((int)h264_file);
    } else {
        return 0;
    }

    uint32_t size = tkl_ftell(h264_file) & 0xFFFFFFFF;
    if (size > TEST_H264_FILE_SIZE_LIMITED) {
        tkl_fclose(h264_file);

        char fp[128] = {'\0'};
        uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
        snprintf(fp, sizeof(fp), "%s/%u-%s", TEST_MEDIA_MOUNT_POINT, tick, H264_RECORD_FILE);
        bk_printf("%s, open new file\r\n", __func__);

        h264_file = tkl_fopen(fp, "w+");
        if (h264_file == NULL) {
            bk_printf("h264 test error, open new failed\r\n");
            __test_media_h264_running = 0; // 文件创建失败，停止录制
            return -1;
        }
    }

    return 0;
}

static void __test_media_h264_record(void *arg)
{
    char fp[128] = {'\0'};
    TKL_VENC_CONFIG_T *h264_config = (TKL_VENC_CONFIG_T *)arg;

    uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
    snprintf(fp, sizeof(fp), "%s/%u-%s", TEST_MEDIA_MOUNT_POINT, tick, H264_RECORD_FILE);

    int ret = test_fs_mount(TEST_MEDIA_MOUNT_POINT, DEV_SDCARD);
    if (ret != 0) {
        bk_printf("h264 test error, mount %s failed\r\n", TEST_MEDIA_MOUNT_POINT);
        return;
    }

    h264_file = tkl_fopen(fp, "w+");
    if (h264_file == NULL) {
        test_fs_unmount(TEST_MEDIA_MOUNT_POINT);
        bk_printf("h264 test error, open %s failed\r\n", H264_RECORD_FILE);
        return;
    }
    bk_printf("file %s fd: %d\r\n", fp, (int)h264_file);

    tkl_venc_init(0, h264_config, 0);

    __test_media_h264_running = 1;
    while (1) {
        if (__test_media_h264_running == 0) {
            bk_printf("h264 ready to stop\r\n");
            goto __test_h264_thread_end;
        }
        tkl_system_sleep(100);
    }

__test_h264_thread_end:
    bk_printf("%s exit\r\n", __func__);

    tkl_system_sleep(100);
    tkl_venc_uninit(0, h264_config);

    tkl_system_sleep(300);

    tkl_fclose(h264_file);
    h264_file = NULL;

    test_fs_unmount(TEST_MEDIA_MOUNT_POINT);

    __h264_record_thread = NULL;
    vTaskDelete(NULL);
}

void test_media_h264_open(void)
{
    TKL_VI_CAMERA_TYPE_E camera = test_media_camera_get_type();

    if (__h264_record_thread != NULL) {
        bk_printf("h264 test thread exist\r\n");
        return;
    }

    if (camera == TKL_VI_CAMERA_TYPE_DVP)
        test_h264_config.enable_h264_pipeline = 0;
    else if (camera == TKL_VI_CAMERA_TYPE_UVC)
        test_h264_config.enable_h264_pipeline = 1;
    else {
        bk_printf("Open camera first\r\n");
        return;
    }

    test_h264_config.put_cb = __h264_cb;
    xTaskCreate(__test_media_h264_record, "h264", 4096, &test_h264_config, 5, &__h264_record_thread);
}

void test_media_h264_close(void)
{
    __test_media_h264_running = 0;
    while (__h264_record_thread != NULL) {
        tkl_system_sleep(100);
    }
}


