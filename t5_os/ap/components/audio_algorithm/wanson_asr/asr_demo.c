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
#include <os/os.h>
#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>
#include "asr.h"
#include "ff.h"
#include "diskio.h"
//#include "BK7256_RegList.h"
#include "cli.h"


#define ASR_BUFF_SIZE 8000  //>960*2

//static uint8_t *asr_ring_buff = NULL;
//static RingBufferContext asr_rb;
int8_t *asr_buff = NULL;
const static char *text;
static float score;
static int rs;

static char result0[13] = {0xE5,0xB0,0x8F,0xE8,0x9C,0x82,0xE7,0xAE,0xA1,0xE5,0xAE,0xB6,0x00};//小蜂管家
static char result1[13] = {0xE9,0x98,0xBF,0xE5,0xB0,0x94,0xE7,0xB1,0xB3,0xE8,0xAF,0xBA,0x00};//阿尔米诺
static char result2[13] = {0xE4,0xBC,0x9A,0xE5,0xAE,0xA2,0xE6,0xA8,0xA1,0xE5,0xBC,0x8F,0x00};//会客模式
static char result3[13] = {0xE7,0x94,0xA8,0xE9,0xA4,0x90,0xE6,0xA8,0xA1,0xE5,0xBC,0x8F,0x00};//用餐模式
static char resulta[13] = {0xE7,0xA6,0xBB,0xE5,0xBC,0x80,0xE6,0xA8,0xA1,0xE5,0xBC,0x8F,0x00};//离开模式
static char resultc[13] = {0xE5,0x9B,0x9E,0xE5,0xAE,0xB6,0xE6,0xA8,0xA1,0xE5,0xBC,0x8F,0x00};//回家模式

static void cli_audio_asr_help(void)
{
	BK_LOGD(NULL, "asr_file_test {xxx.pcm} \r\n");
}

/* mic file format: signal channel, 16K sample rate, 16bit width */
void cli_asr_file_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char mic_file_name[50];
	FIL file_mic;
	FRESULT fr;
	uint32 uiTemp = 0;
	//uint32_t encoder_size = 0;
	uint8_t ucInBuff[960] = {0};
	bool empty_flag = false;

	if (argc != 2) {
		cli_audio_asr_help();
		return;
	}

	sprintf(mic_file_name, "1:/%s", argv[1]);
	fr = f_open(&file_mic, mic_file_name, FA_READ);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "open %s fail.\r\n", mic_file_name);
		return;
	}

	if (Wanson_ASR_Init() < 0)
	{
		BK_LOGD(NULL, "Wanson_ASR_Init Failed!\n");
		return;
	}
	Wanson_ASR_Reset();
	BK_LOGD(NULL, "Wanson_ASR_Init OK!\n");

	while (!empty_flag)
	{
		fr = f_read(&file_mic, (uint8_t *)ucInBuff, 960, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read ref file fail.\r\n");
			break;
		}

		if (uiTemp < 960)
			empty_flag = true;

		if (uiTemp == 960) {
//			GPIO_UP(44);
			rs = Wanson_ASR_Recog((short*)ucInBuff, 480, &text, &score);
//			GPIO_DOWN(44);
			if (rs == 1) {
				BK_LOGD(NULL, " ASR Result: %s\n", text);    //识别结果打印
				if (os_strcmp(text, result0) == 0) {    //识别出唤醒词 小蜂管家
					BK_LOGD(NULL, "%s \n", "xiao feng guan jia ");
				} else if (os_strcmp(text, result1) == 0) {    //识别出唤醒词 阿尔米诺
					BK_LOGD(NULL, "%s \n", "a er mi nuo ");
				} else if (os_strcmp(text, result2) == 0) {    //识别出 会客模式
					BK_LOGD(NULL, "%s \n", "hui ke mo shi ");
				} else if (os_strcmp(text, result3) == 0) {	 //识别出 用餐模式
					BK_LOGD(NULL, "%s \n", "yong can mo shi ");
				} else if (os_strcmp(text, resulta) == 0) {  //识别出 离开模式
					BK_LOGD(NULL, "%s \n", "li kai mo shi ");
				} else if (os_strcmp(text, resultc) == 0) {  //识别出 回家模式
					BK_LOGD(NULL, "%s \n", "hui jia mo shi ");
				} else {
					//BK_LOGD(NULL, " \n");
				}
			}
		}
	}

	empty_flag = false;

	Wanson_ASR_Release();

	fr = f_close(&file_mic);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "close out file %s fail!\r\n", mic_file_name);
		return;
	}
	BK_LOGD(NULL, "wanson asr test complete \r\n");

	BK_LOGD(NULL, "test finish \r\n");
}


#define ASR_CMD_CNT (sizeof(s_asr_commands) / sizeof(struct cli_command))
static const struct cli_command s_asr_commands[] = {
	{"asr_file_test", "asr_file_test {xxx.pcm}", cli_asr_file_test_cmd},
};

int cli_asr_init(void)
{
	return cli_register_commands(s_asr_commands, ASR_CMD_CNT);
}


