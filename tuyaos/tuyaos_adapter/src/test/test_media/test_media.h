/*
 * test_media.h
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#ifndef TEST_MEDIA_H
#define TEST_MEDIA_H

#include "FreeRTOS.h"
#include "task.h"

#include "tuya_cloud_types.h"
#include "tkl_display.h"
#include "tkl_video_enc.h"
#include "tkl_audio.h"
#include "tkl_fs.h"
#include "tkl_thread.h"
#include "tkl_system.h"

#include "rb.h"

#define TEST_MEDIA_MOUNT_POINT "/sdcard"
#define TEST_MAX_FILE_SIZE_LIMITED    (4 * 1024 * 1024 * 1024)

// lcd
void test_media_lcd_set(const char *lcd);
const char *test_media_lcd_get(void);
void test_media_get_lcd_ppi(uint16_t *w, uint16_t *h);
void test_media_lcd_open(int pipeline);
void test_media_lcd_close(void);
void test_media_lcd_rotate(void);

// camera
#define CAMERA_CONFIG_IDLE  0xff
void test_media_camera_open(TKL_VI_CAMERA_TYPE_E type);
void test_media_camera_close(TKL_VI_CAMERA_TYPE_E type);
TKL_VI_CAMERA_TYPE_E test_media_camera_get_type(void);

// h264
#define H264_RECORD_FILE    "h264_record.h264"
void test_media_h264_open(void);
void test_media_h264_close(void);

// audio
typedef enum{
    AUDIO_MIC = 0,
    AUDIO_SPK,
    AUDIO_DUAL
} TEST_AUDIO_TYPE_E;
#define MIC_RECORD_FILE     "mic_record.pcm"
void test_media_audio_open(TEST_AUDIO_TYPE_E type);
void test_media_audio_close(void);


#endif /* !TEST_MEDIA_H */
