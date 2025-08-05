// Copyright 2023-2024 Beken
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

#include "cli.h"

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>
#include "ff.h"
#include "diskio.h"
#include "modules/audio_agc.h"
#include "modules/audio_agc_types.h"

//#include "BK7256_RegList.h"


static void cli_audio_agc_help(void)
{
	BK_LOGD(NULL, "agc_file_test {xxx.pcm, xxx.pcm, sample_rate} \r\n");
}

/* input audio data: 16bits, mono */
void cli_agc_file_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char in_file_name[50];
	char out_file_name[50];
	FIL file_in;
	FIL file_out;
	FRESULT fr;
	uint32 uiTemp = 0;

	void *agc = NULL;
	int16_t *ucInBuff = NULL;
	int16_t *ucOutBuff = NULL;
	uint32_t sample_rate = 0;
	int frameSize = 0;

	if (argc != 4) {
		cli_audio_agc_help();
		return;
	}

	sprintf(in_file_name, "1:/%s", argv[1]);
	fr = f_open(&file_in, in_file_name, FA_READ);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "open %s fail.\r\n", in_file_name);
		goto exit;
	}

	sprintf(out_file_name, "1:/%s", argv[2]);
	fr = f_open(&file_out, out_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "open %s fail.\r\n", out_file_name);
		goto exit;
	}

	BK_LOGD(NULL, "open file ok \n");

	sample_rate = strtoul(argv[3], NULL, 10);
	if (sample_rate == 0) {
		BK_LOGD(NULL, "sample_rate: %d is error \n", sample_rate);
		goto exit;
	} else {
		BK_LOGD(NULL, "sample_rate: %d \n", sample_rate);
	}

	if (sample_rate ==8000) frameSize = 80;
	if (sample_rate == 16000) frameSize = 160;

	ucInBuff = os_malloc(frameSize * 2);
	if (ucInBuff == NULL) {
		BK_LOGD(NULL, "malloc ucInBuff fail, size: %d \n", frameSize * 2);
		goto exit;
	}

	ucOutBuff = os_malloc(frameSize * 2);
	if (ucOutBuff == NULL) {
		BK_LOGD(NULL, "malloc ucOutBuff fail, size: %d \n", frameSize * 2);
		goto exit;
	}

	/* init agc */
	if (0 != bk_aud_agc_create(&agc)) {
		BK_LOGD(NULL, "create agc fail \r\n");
		goto exit;
	}

	bk_aud_agc_init(agc, 0, 255, sample_rate);
	bk_agc_config_t agc_config;
	agc_config.compressionGaindB = 16;		// 最大增益能力;超过此分贝的值会放大到最大限制
	agc_config.limiterEnable = 1;			// 最大值限制开启
	agc_config.targetLevelDbfs = 3;			// 最大分贝值限制, 值越大幅度越小
	bk_aud_agc_set_config(agc, agc_config);
	BK_LOGD(NULL, "Init agc ok \n");

	uint32_t total_data_size = f_size(&file_in);
	while(total_data_size >= (frameSize * 2))
	{
		fr = f_read(&file_in, (uint8_t *)ucInBuff, frameSize * 2, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read ref file fail.\r\n");
			break;
		}

//		addAON_GPIO_Reg0x8 = 2;
		int res = bk_aud_agc_process(agc, ucInBuff, frameSize, ucOutBuff);
		if (0 != res) {
			BK_LOGD(NULL, "failed in WebRtcAgc_Process, res: %d \n", res);
			break;
		}
//		addAON_GPIO_Reg0x8 = 0;

		//write output data to sd
		fr = f_write(&file_out, (void *)ucOutBuff, frameSize * 2, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "write output data %s fail.\r\n", out_file_name);
			break;
		}
		total_data_size -= frameSize * 2;
	}

exit:
	//close file
	fr = f_close(&file_in);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "close out file %s fail!\r\n", in_file_name);
	}
	fr = f_close(&file_out);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "close out file %s fail!\r\n", out_file_name);
	}
	BK_LOGD(NULL, "close files complete \r\n");

	//free source
	if (ucInBuff) {
		os_free(ucInBuff);
		ucInBuff = NULL;
	}
	if (ucOutBuff) {
		os_free(ucOutBuff);
		ucOutBuff = NULL;
	}
	BK_LOGD(NULL, "free buffers complete \r\n");

	if (agc) {
		bk_aud_agc_free(agc);
		agc = NULL;
	}
	BK_LOGD(NULL, "free agc complete \r\n");

	BK_LOGD(NULL, "agc test complete \r\n");
}

#define AGC_CMD_CNT (sizeof(s_agc_commands) / sizeof(struct cli_command))
static const struct cli_command s_agc_commands[] = {
	{"agc_file_test", "agc_file_test {xxx.pcm, xxx.pcm, sample_rate}", cli_agc_file_test_cmd},
};

int cli_agc_init(void)
{
	return cli_register_commands(s_agc_commands, AGC_CMD_CNT);
}

