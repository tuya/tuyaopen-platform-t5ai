/*
 * test_adc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

#include <os/os.h>
#include "tuya_cloud_types.h"
#include "tkl_mic.h"
#include "tkl_speaker.h"
static int32_t buffer[1024] = {0};

void cli_mic_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    
    TKL_MIC_CFG_T mic_cfg = {
        .chl_num = 1,        //adc通道
        .sample_rate = 16000,   // 采样率
        .card = TKL_MIC_TYPE_BOARD,
        .volume = 50,
        .datebits = TKL_MIC_DATABITS_16,
        .codectype = TKL_CODEC_MIC_PCM,
    };
    tkl_mic_init(&mic_cfg);
    tkl_mic_start();
    tkl_mic_set_gain(50);

    TKL_SPK_CFG_T speaker_cfg = {
        .chl_num = 1,        //adc通道
        .sample_rate = 16000,   // 采样率
        .card = TKL_SPK_TYPE_BOARD,
        .volume = 50,
        .spk_gpio = 28,
        .spk_gpio_polarity = 0,
        .datebits = TKL_SPK_DATABITS_8,
        .codectype = TKL_CODEC_SPK_PCM,
    };
    tkl_speaker_init(&speaker_cfg);
    tkl_speaker_start();
    tkl_speaker_set_gain(50);
    bk_printf("adc output voltage:\r\n");

    memset(buffer, 0, sizeof(buffer));

    
    while (1)
    {
        tkl_mic_read(buffer, sizeof(buffer));
        tkl_speaker_write(buffer, sizeof(buffer));
    }
    bk_printf("adc input voltage:\r\n");
    for (int i = 0; i < sizeof(buffer); i++) {
        bk_printf(" %d", *(buffer + i));
    }
    tkl_mic_stop();
    tkl_mic_deinit();
    tkl_speaker_stop();
    tkl_speaker_deinit();
    bk_printf("\r\n");
}


