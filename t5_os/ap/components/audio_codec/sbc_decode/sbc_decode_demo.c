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


#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include "sys_driver.h"
#include "sbc_driver.h"
#include <driver/sbc.h>
#include <driver/sbc_types.h>
#include "audio_play.h"
#include "ff.h"
#include "diskio.h"
#include "cli.h"


#define SBC_PLAY_TAG  "sbc_play"
#define LOGI(...) BK_LOGI(SBC_PLAY_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(SBC_PLAY_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(SBC_PLAY_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(SBC_PLAY_TAG, ##__VA_ARGS__)


static sbcdecodercontext_t g_sbc_decoder;
static char sbc_file_name[50];
static FIL sbc_file;
static uint8_t *sbc_data_buffer = NULL;
static uint32_t g_frame_length = 0;
static audio_play_t *audio_play = NULL;

static bool sbc_play_run = false;
static beken_thread_t sbc_play_task_hdl = NULL;
static beken_semaphore_t sbc_play_sem = NULL;


static void cli_sbc_decoder_help(void)
{
	LOGD("sbc_decoder_test {start|stop} {xxx.sbc}\n");
}


static void sbc_play_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;
	uint8_t sbc_info[4];
	uint8_t sbc_pcm_length = 0;
	uint8_t sbc_sample_rate_index = 0;
	uint8_t sbc_blocks, sbc_subbands, sbc_bitpool, sbc_channel_mode, sbc_channel_number;
    bool sbc_file_is_empty = false;

    char *file_name = (char *)param_data;

    /* init sbc decoder */
    bk_sbc_decoder_init(&g_sbc_decoder);

    os_memset(sbc_file_name, 0, sizeof(sbc_file_name)/sizeof(sbc_file_name[0]));
    sprintf(sbc_file_name, "1:/%s", file_name);
    fr = f_open(&sbc_file, sbc_file_name, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        LOGE("open %s fail.\n", sbc_file_name);
        goto exit;
    }

    /* file_name is invalid */
    rtos_set_semaphore(&sbc_play_sem);

    /* parse sbc header to get frame information */
    fr = f_read(&sbc_file, (void *)sbc_info, 4, &uiTemp);
    if (fr == FR_OK) {
        if (sbc_info[0] == SBC_SYNCWORD) {
            sbc_blocks = (((sbc_info[1] >> 4) & 0x03) + 1) << 2;
            sbc_subbands = ((sbc_info[1] & 0x01) + 1) << 2;
            sbc_bitpool = sbc_info[2];
            sbc_channel_mode = (sbc_info[1] >> 2) & 0x03;
            sbc_sample_rate_index = (sbc_info[1] >> 6) & 0x03;
        } else if (sbc_info[0] == MSBC_SYNCWORD) {
            sbc_blocks = 15;
            if (sbc_info[1] || sbc_info[2]) {
                sbc_sample_rate_index = (sbc_info[1] >> 6) & 0x03;
                sbc_channel_mode = (sbc_info[1] >> 2) & 0x03;
                sbc_subbands = ((sbc_info[1] & 0x01) + 1) << 2;
                sbc_bitpool = sbc_info[2];
            } else {
                sbc_sample_rate_index = 0;
                sbc_channel_mode = 0;
                sbc_subbands = 8;
                sbc_bitpool = 26;
            }
        } else {
            LOGW("Not find syncword, valid sbc file\n");
            goto exit;
        }

        sbc_channel_number = (sbc_channel_mode == SBC_CHANNEL_MODE_MONO) ? 1 : 2;
        sbc_pcm_length = sbc_blocks * sbc_subbands;

        g_frame_length = 4 + ((4 * sbc_subbands * sbc_channel_number) >> 3);
        if (sbc_channel_mode < 2) {
            g_frame_length += ((uint32_t)sbc_blocks * (uint32_t)sbc_channel_number * (uint32_t)sbc_bitpool + 7) >> 3;
        } else {
            g_frame_length += ((sbc_channel_mode == SBC_CHANNEL_MODE_JOINT_STEREO) * sbc_subbands + sbc_blocks * ((uint32_t)sbc_bitpool) + 7) >> 3;
        }
        f_lseek(&sbc_file, 0);
    } else {
        LOGE("read sbc info fail\n");
        goto exit;
    }

    /* malloc read buffer */
    sbc_data_buffer = os_malloc(g_frame_length);
    if (sbc_data_buffer == NULL) {
        LOGE("sbc data buffer malloc failed!\r\n");
        goto exit;
    }

    /* init audio play */
    audio_play_cfg_t audio_play_config = DEFAULT_AUDIO_PLAY_CONFIG();
    audio_play_config.nChans = sbc_channel_number;
    if (sbc_sample_rate_index == 0) {
        audio_play_config.sampRate = 16000;
    } else if (sbc_sample_rate_index == 1) {
        audio_play_config.sampRate = 32000;
    } else if (sbc_sample_rate_index == 2){
        audio_play_config.sampRate = 44100;
    } else {
        audio_play_config.sampRate = 48000;
    }
    audio_play_config.bitsPerSample = 16;
    audio_play_config.volume = 0x2d;
    audio_play_config.play_mode = AUDIO_PLAY_MODE_DIFFEN;
    audio_play_config.frame_size = sbc_pcm_length * audio_play_config.bitsPerSample / 8 * audio_play_config.nChans;
    audio_play_config.pool_size = audio_play_config.frame_size * 4;
    audio_play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &audio_play_config);
    if (!audio_play)
    {
        LOGE("create audio play fail\n");
        return;
    }

    ret = audio_play_open(audio_play);
    if (ret != BK_OK)
    {
        LOGE("open audio play fail, ret: %d\n", ret);
        goto exit;
    }

    sbc_play_run = true;

    while (sbc_play_run)
    {
        if (sbc_file_is_empty) {
            LOGD("sbc file is empty, stop play\n");
            goto exit;
        }

        /* read data from file */
        fr = f_read(&sbc_file, (void *)sbc_data_buffer, g_frame_length, &uiTemp);
        if (fr != FR_OK) {
            LOGE("read %s fail, fr: %d\n", sbc_file_name, fr);
            goto exit;
        }

        if (uiTemp == 0) {
            sbc_file_is_empty = true;
            LOGD("the %s is empty\n", sbc_file_name);
        }

        ret = bk_sbc_decoder_frame_decode(&g_sbc_decoder, sbc_data_buffer, uiTemp);
        if (ret != BK_OK) {
            LOGE("bk_sbc_decoder_frame_decode fail, ret: %d\n", ret);
            goto exit;
        }

        /* write a frame speaker data to play */
        ret = audio_play_write_data(audio_play, (char *)g_sbc_decoder.pcm_sample, audio_play_config.frame_size);
        if (ret < 0) {
            LOGE("write spk data fail\n");
            goto exit;
        }
    }

exit:
    sbc_play_run = false;

    if (audio_play)
    {
        ret = audio_play_close(audio_play);
        if (ret != BK_OK) {
            LOGE("audio play close fail, ret:%d\n", ret);
        }

        audio_play_destroy(audio_play);
        audio_play = NULL;
    }

    fr = f_close(&sbc_file);
    if (fr != FR_OK) {
        LOGE("close %s fail\n", sbc_file_name);
    }

    if (sbc_data_buffer) {
        os_free(sbc_data_buffer);
        sbc_data_buffer = NULL;
    }

    bk_sbc_decoder_deinit();

    /* delete task */
    sbc_play_task_hdl = NULL;

    rtos_set_semaphore(&sbc_play_sem);

    rtos_delete_thread(NULL);
}


bk_err_t sbc_play_start(char *file_name)
{
    bk_err_t ret = BK_OK;

    ret = rtos_init_semaphore(&sbc_play_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_create_thread(&sbc_play_task_hdl,
                             (BEKEN_DEFAULT_WORKER_PRIORITY - 1),
                             "sbc_play",
                             (beken_thread_function_t)sbc_play_main,
                             2048,
                             (beken_thread_arg_t)file_name);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, create spk data read task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&sbc_play_sem, BEKEN_NEVER_TIMEOUT);

    LOGD("init sbc play task complete\n");

    return BK_OK;

fail:

    if (sbc_play_sem)
    {
        rtos_deinit_semaphore(&sbc_play_sem);
        sbc_play_sem = NULL;
    }

    return BK_FAIL;
}

bk_err_t sbc_play_stop(void)
{
    if (!sbc_play_run)
    {
        return BK_OK;
    }

    sbc_play_run = false;

    rtos_get_semaphore(&sbc_play_sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&sbc_play_sem);
    sbc_play_sem = NULL;

    return BK_OK;
}

void cli_sbc_decoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 3) {
		cli_sbc_decoder_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
        if (BK_OK != sbc_play_start(argv[2])) {
            LOGD("start sbc play fail\n");
        } else {
            LOGD("start sbc play ok\n");
        }
	}else if (os_strcmp(argv[1], "stop") == 0) {
        sbc_play_stop();
	} else {
		cli_sbc_decoder_help();
		return;
	}
}

#define SBC_CMD_CNT	(sizeof(s_sbc_commands) / sizeof(struct cli_command))

static const struct cli_command s_sbc_commands[] = {
	{"sbc_decoder_test", "sbc_decoder_test {start|stop} {xxx.sbc}", cli_sbc_decoder_test_cmd},
};

int cli_sbc_init(void)
{
	return cli_register_commands(s_sbc_commands, SBC_CMD_CNT);
}

