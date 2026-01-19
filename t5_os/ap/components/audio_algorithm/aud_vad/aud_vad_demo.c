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


#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <modules/audio_vad.h>
#include "audio_record.h"
#include "ff.h"
#include "diskio.h"
#include "cli.h"


#define VAD_TEST_TAG "vad_test"
#define LOGI(...) BK_LOGI(VAD_TEST_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(VAD_TEST_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(VAD_TEST_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(VAD_TEST_TAG, ##__VA_ARGS__)


#define FARME_SIZE 320

static beken_thread_t vad_thread_handle = NULL;
static beken_queue_t vad_int_msg_que = NULL;
static audio_record_t *audio_record = NULL;
static beken_semaphore_t vad_sem = NULL;


typedef enum {
	VAD_CTRL_OP_SET_START = 0,
	VAD_CTRL_OP_SET_CONTINUE,
	VAD_CTRL_OP_EXIT,
	VAD_CTRL_OP_MAX,
} vad_ctrl_op_t;

typedef struct {
	vad_ctrl_op_t op;
	int value;
} vad_ctrl_msg_t;

static void cli_aud_vad_help(void)
{
	LOGD("aud_vad_test {8000|16000|32000|48000 xxx.pcm}\n");
	LOGD("aud_vad_all_test {start|stop|set_start|set_continue xx}\n");
}

bk_err_t vad_send_msg(vad_ctrl_op_t op, int param)
{
	bk_err_t ret;
	vad_ctrl_msg_t msg;

	msg.op = op;
	msg.value = param;
	if (vad_int_msg_que) {
		ret = rtos_push_to_queue(&vad_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("vad_send_msg fail\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void vad_test_task_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	vad_ctrl_msg_t msg;
	int read_size = 0;

	uint8_t *aud_temp_data = os_malloc(FARME_SIZE);
	if (!aud_temp_data)
	{
		LOGE("malloc aud_temp_data\n");
		goto vad_exit;
	}
	os_memset(aud_temp_data, 0, FARME_SIZE);

    /* init audio vad */
	ret = bk_aud_vad_init(FARME_SIZE/2, 8000);
	if (ret != BK_OK) {
		LOGD("init vad fail\n");
		goto vad_exit;
	}

    /* init audio record */
    audio_record_cfg_t config = DEFAULT_AUDIO_RECORD_CONFIG();
    config.sampRate = 8000;
    config.frame_size = FARME_SIZE;
    config.pool_size = FARME_SIZE * 2;
    audio_record = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &config);
    if (!audio_record)
    {
        LOGE("create audio record fail\n");
        goto vad_exit;
    }

    if (BK_OK != audio_record_open(audio_record))
    {
        LOGE("open audio record fail\n");
        goto vad_exit;
    }

    rtos_set_semaphore(&vad_sem);

	while (1) {
		ret = rtos_pop_from_queue(&vad_int_msg_que, &msg, 0);//BEKEN_WAIT_FOREVER
		if (kNoErr == ret) {
			switch (msg.op) {
				case VAD_CTRL_OP_SET_START:
					LOGD("goto: VAD_CTRL_OP_SET_START\n");
					bk_aud_vad_set_start(msg.value);
					break;

				case VAD_CTRL_OP_SET_CONTINUE:
					LOGD("goto: VAD_CTRL_OP_SET_CONTINUE\n");
					bk_aud_vad_set_continue(msg.value);
					break;

				case VAD_CTRL_OP_EXIT:
					LOGD("goto: VAD_CTRL_OP_EXIT\n");
					goto vad_exit;
					break;

				default:
					break;
			}
		}

        read_size = audio_record_read_data(audio_record, (char *)aud_temp_data, FARME_SIZE);
        if (read_size == FARME_SIZE) {
            int result = bk_aud_vad_process((int16_t *)aud_temp_data);
            if (result == 1) {
                LOGD("speech\n");
            } else if (result == 0) {
                LOGD("noise/silence\n");
            }
        } else {
            LOGE("vad_read_mic_data fail, read_size: %d\n", read_size);
            goto vad_exit;
        }
	}

vad_exit:
	if (aud_temp_data) {
		os_free(aud_temp_data);
		aud_temp_data == NULL;
	}

    if (audio_record) {
        audio_record_close(audio_record);
        audio_record_destroy(audio_record);
        os_free(audio_record);
    }

	bk_aud_vad_deinit();

	/* delete msg queue */
	ret = rtos_deinit_queue(&vad_int_msg_que);
	if (ret != kNoErr) {
		LOGD("delete message queue fail\n");
	}
	vad_int_msg_que = NULL;
	LOGD("delete message queue complete\n");

    rtos_set_semaphore(&vad_sem);

	/* delete task */
	vad_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

void cli_aud_vad_all_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	int param = 0;

	if (argc != 2 && argc != 3) {
		cli_aud_vad_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {

        ret = rtos_init_semaphore(&vad_sem, 1);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
            goto fail;
        }

		ret = rtos_init_queue(&vad_int_msg_que,
							  "vad_int_que",
							  sizeof(vad_ctrl_msg_t),
							  5);
		if (ret != kNoErr) {
			LOGE("ceate vad internal message queue fail\n");
			goto fail;
		}

		ret = rtos_create_thread(&vad_thread_handle,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "vad_test",
							 (beken_thread_function_t)vad_test_task_main,
							 4096,
							 NULL);
		if (ret != BK_OK) {
			LOGE("create cli vad task fail\n");
            goto fail;
        }

        rtos_get_semaphore(&vad_sem, BEKEN_NEVER_TIMEOUT);

		LOGD("start vad test\n");

fail:
        if (vad_sem)
        {
            rtos_deinit_semaphore(&vad_sem);
            vad_sem = NULL;
        }

        if (vad_int_msg_que)
        {
            rtos_deinit_queue(&vad_int_msg_que);
            vad_int_msg_que = NULL;
        }

        LOGD("start vad test fail\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		vad_send_msg(VAD_CTRL_OP_EXIT, 0);
		LOGD("stop vad test\n");
	} else if (os_strcmp(argv[1], "set_start") == 0) {
		param = os_strtoul(argv[2], NULL, 10);
		vad_send_msg(VAD_CTRL_OP_SET_START, param);
		LOGD("set start\n");
	} else if (os_strcmp(argv[1], "set_continue") == 0) {
		param = os_strtoul(argv[2], NULL, 10);
		vad_send_msg(VAD_CTRL_OP_SET_CONTINUE, param);
		LOGD("set continue\n");
	} else {
		cli_aud_vad_help();
		return;
	}
}

void cli_aud_vad_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char in_file_name[50];
	FIL file_in;
	int16_t *in_addr = NULL;
	FRESULT fr;
	uint32 uiTemp = 0;
	uint32_t rsp_size = 0;

	bk_err_t ret = BK_OK;
	int result = 0;
	int samp_rate = 0;

	if (argc != 3) {
		cli_aud_vad_help();
		return;
	}

	samp_rate = os_strtoul(argv[1], NULL, 10);
	sprintf(in_file_name, "1:/%s", argv[2]);
	fr = f_open(&file_in, in_file_name, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK) {
		LOGE("open %s fail.\n", in_file_name);
		return;
	}

    /* init audio vad */
	ret = bk_aud_vad_init(FARME_SIZE/2, samp_rate);
	if (ret != BK_OK) {
		LOGE("init vad fail\n");
		return;
	}

	in_addr = (int16_t *)os_malloc(FARME_SIZE);
	if (!in_addr) {
		LOGE("malloc fail\n");
		return;
	}
	os_memset(in_addr, 0, FARME_SIZE);

	rsp_size = f_size(&file_in);
	LOGD("rsp_size = %d\n", rsp_size);
	while (1) {
		fr = f_read(&file_in, in_addr, FARME_SIZE, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read in data fail.\r\n");
			break;
		}

		if (uiTemp == 0 || uiTemp < FARME_SIZE)
			break;

        result = bk_aud_vad_process(in_addr);
		if (result == 1) {
			LOGD("speech\n");
		} else if (result == 0) {
			LOGD("noise/silence\n");
		}
		else if (result == BK_FAIL)
		{
			LOGD("bk_aud_vad_process fail %d\n", result);
			break;
		}
	}

	LOGD("break while\n");

	fr = f_close(&file_in);
	if (fr != FR_OK) {
		LOGE("close out file %s fail!\n", in_file_name);
		return;
	}

    bk_aud_vad_deinit();

	LOGD("vad test complete\n");
}

#define AUD_VAD_CMD_CNT (sizeof(s_aud_vad_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_vad_commands[] = {
	{"aud_vad_test", "aud_vad_test {8000|16000|32000|48000 xxx.pcm}", cli_aud_vad_test_cmd},
	{"aud_vad_all_test", "aud_vad_all_test {start|stop|set_start|set_continue xx}", cli_aud_vad_all_test_cmd},
};

int cli_aud_vad_init(void)
{
	return cli_register_commands(s_aud_vad_commands, AUD_VAD_CMD_CNT);
}

