/*
 * test_media_audio.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "test_media.h"

#define TEST_AUDIO_DELAY    1
#define TEST_AUDIO_SAMPLE_BITS  16
#define TEST_AUDIO_SAMPLE_RATE  16000
#define TEST_AUDIO_RB_SIZE   (256*1024)
#define TEST_AUDIO_DATA_LEN_PER_SECOND  (TEST_AUDIO_SAMPLE_RATE * TEST_AUDIO_SAMPLE_BITS / 8)

static TaskHandle_t __audio_play_thread = NULL;
static TKL_AUDIO_TYPE_E aud_type = TKL_AUDIO_TYPE_BOARD;
static TEST_AUDIO_TYPE_E current_dev = 0xff;
static TUYA_FILE mic_record_file = NULL;
static uint32_t __test_media_audio_running = 0;
struct ringbuffer rb;

#ifdef CONFIG_VOICE_SERVICE

static int __test_media_audio_mic_cb(TKL_AUDIO_FRAME_INFO_T *pframe)
{
    int ret = 0;
    if (current_dev == AUDIO_SPK) {
        return pframe->used_size;
    }
    // TKL_AUDIO_FRAME_INFO_T testframe = {0};
    // testframe.pbuf = PCM_SPK16000;
    // testframe.used_size = sizeof(PCM_SPK16000);
    ret = tkl_ao_put_frame(0, 0, NULL, pframe);
    if (ret != 0)
    {
        bk_printf("tkl_ao_put_frame ... %d %s %d\r\n",ret, __func__, __LINE__);
    }
    
    if (__test_media_audio_running) {
        tkl_rb_in(&rb, pframe->pbuf, pframe->used_size);
    }

    return pframe->used_size;
}

static int __test_media_audio_init(void)
{
    int ret = 0;
    TKL_AUDIO_CONFIG_T config ={0};

    config.enable = true;
    config.card = TKL_AUDIO_TYPE_BOARD;
    config.ai_chn = 1;
    config.spk_gpio = 28;     // sample
    config.spk_gpio_polarity= 0;     // sample
    config.sample = TEST_AUDIO_SAMPLE_RATE;     // sample
    config.datebits = TEST_AUDIO_SAMPLE_BITS;   // datebit
    config.channel = 1;                         // channel num
    config.codectype = TKL_CODEC_AUDIO_PCM;     // codec type
    config.fps = 25;                            // frame per second，suggest 25
    config.mic_volume = 0x2d;
    config.spk_volume = 0x2d;

    config.spk_gpio = 28;

    config.put_cb = __test_media_audio_mic_cb;

    bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);

    ret = tkl_ai_init(&config, 1);

    bk_printf("type: %d, ret, %d\r\n", aud_type, ret);
    ret |= tkl_ai_set_vol(0, 0, 80);
    bk_printf("type: %d, ret1, %d\r\n", aud_type, ret);

    ret |= tkl_ai_start(0,0);
    bk_printf("type: %d, ret2, %d\r\n", aud_type, ret);

    return ret;
}

/*
 * mic:
 *  audio ---> callback push data to rb ---> thread pop data ---> write to sd card
 *  resource: audio / cb / rb / sd card / file
 *
 * spk:
 *  audio ---> thread read sd card ---> push to spk
 *  resource: audio / sd card / file
 *
 * dual:
 *  audio ---> callback push data to rb ---> thread pop data ---> push to spk
 *  resource: audio / cb / rb
 *
 *  */

static void __test_media_audio_play(void *arg)
{
    uint32_t len = 0;
    TKL_AUDIO_FRAME_INFO_T frame_info;
    uint8_t *data_buffer = NULL;
    struct ringbuffer *r = &rb;
    char fp[128] = {'\0'};
    int ret = 0;

    // spk not need
    ret = tkl_rb_init(&rb, TEST_AUDIO_RB_SIZE, 1);
    if (ret != 0) {
        bk_printf("audio rb malloc failed\r\n");
        return;
    }
    bk_printf("rb %p\r\n", rb.data);

    if ((current_dev == AUDIO_SPK) || (current_dev == AUDIO_MIC)) {
        ret = test_fs_mount(TEST_MEDIA_MOUNT_POINT, DEV_SDCARD);
        if (ret != 0) {
            bk_printf("audio test error, mount failed\r\n");
            goto __test_audio_thread_end;
        }

        snprintf(fp, sizeof(fp), "%s/%s", TEST_MEDIA_MOUNT_POINT, MIC_RECORD_FILE);

        if (current_dev == AUDIO_SPK)
            mic_record_file = tkl_fopen(fp, "r");
        else
            mic_record_file = tkl_fopen(fp, "w+");

        if (mic_record_file == NULL) {
            bk_printf("audio test error, open %s failed\r\n", MIC_RECORD_FILE);
            goto __test_audio_thread_end;
        }
        bk_printf("file %s fd: %d\r\n", fp, (int)mic_record_file);
    }

    data_buffer = tkl_system_psram_malloc(TEST_AUDIO_DATA_LEN_PER_SECOND);
    if (data_buffer == NULL) {
        bk_printf("data_buffer malloc failed\r\n");
        goto __test_audio_thread_end;
    }

    ret = __test_media_audio_init();
    if (ret != 0) {
        bk_printf("audio init failed\r\n");
        goto __test_audio_thread_end;
    }

    bk_printf("delay %ds, stream: %d\r\n", TEST_AUDIO_DELAY, TEST_AUDIO_DATA_LEN_PER_SECOND >> 10);
    tkl_system_sleep(TEST_AUDIO_DELAY * 1000);

    __test_media_audio_running = 1;

    while (1) {
        if (current_dev == AUDIO_DUAL) {
            if (tkl_rb_avail(r) >= TEST_AUDIO_DATA_LEN_PER_SECOND) {
                len = tkl_rb_out(r, data_buffer, TEST_AUDIO_DATA_LEN_PER_SECOND);
                frame_info.pbuf = data_buffer;
                frame_info.used_size = len;
                bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);

                tkl_ao_put_frame(0, 0, NULL, &frame_info);

            }
        } else if (current_dev == AUDIO_MIC) {
            if (tkl_rb_avail(r) >= TEST_AUDIO_DATA_LEN_PER_SECOND) {
                len = tkl_rb_out(r, data_buffer, TEST_AUDIO_DATA_LEN_PER_SECOND);
                tkl_fwrite(data_buffer, len, mic_record_file);
                tkl_fsync((int)mic_record_file);
                bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);
                bk_printf("write %d to %s\r\n", len, fp);
            }
        } else if (current_dev == AUDIO_SPK) {
            len = tkl_fread(data_buffer, TEST_AUDIO_DATA_LEN_PER_SECOND, mic_record_file);
            if (len <= 0) {
                bk_printf("End of file reached\r\n");
                __test_media_audio_running = 0;
            }
            frame_info.pbuf = data_buffer;
            frame_info.used_size = len;
            bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);

            tkl_ao_put_frame(0, 0, NULL, &frame_info);

        }

        if (__test_media_audio_running == 0) {
            bk_printf("exit audio thread\r\n");
            goto __test_audio_thread_end;
        }

        tkl_system_sleep(50);
    }

__test_audio_thread_end:

    bk_printf("TODO ... %s %d\r\n", __func__, __LINE__);

    tkl_ai_stop(0, 0);
    tkl_ai_uninit();

    tkl_system_sleep(500);

    tkl_rb_deinit(&rb);

    if ((current_dev == AUDIO_SPK) || (current_dev == AUDIO_MIC)) {
        test_fs_unmount(TEST_MEDIA_MOUNT_POINT);
        if (mic_record_file) tkl_fclose(mic_record_file);
    }

    if (data_buffer != NULL) {
        tkl_system_psram_free(data_buffer);
        data_buffer = NULL;
    }

    __audio_play_thread = NULL;
    vTaskDelete(NULL);

    return;
}

void test_media_audio_open(TEST_AUDIO_TYPE_E dev_type)
{
    if (__test_media_audio_running) {
        bk_printf("audio test is already running, stop first %d\r\n", current_dev);
        return;
    }

    TKL_VI_CAMERA_TYPE_E camera = test_media_camera_get_type();
    if (camera == TKL_VI_CAMERA_TYPE_UVC)
        aud_type = TKL_AUDIO_TYPE_UAC;
    else if (camera == TKL_VI_CAMERA_TYPE_DVP)
        aud_type = TKL_AUDIO_TYPE_BOARD;

    bk_printf("aud: %s / %d\r\n", (aud_type == TKL_AUDIO_TYPE_UAC)? "uac": "on_board", dev_type);
    current_dev = dev_type;
    xTaskCreate(__test_media_audio_play, "taudio", 4096, NULL, 5, &__audio_play_thread);
}

void test_media_audio_close(void)
{
    __test_media_audio_running = 0;
    while(__audio_play_thread != NULL) {
        tkl_system_sleep(50);
    }
    bk_printf("audio test stop\r\n");
}
#endif
