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
#include "ff.h"
#include "diskio.h"
#include <modules/flac_enc.h>
#include <modules/flac_enc_types.h>
//#include "BK7256_RegList.h"
#include "cli.h"

static bool file_empty_flag = false;
static char in_file_name[50];
static char out_file_name[50];
static FIL *file_out;
static FIL file_in;

#define READSIZE 1024

static uint8_t *buffer = NULL;
static int32_t *pcm = NULL;

static flac_enc_setup_t *enc_setup;

static void cli_aud_flac_help(void)
{
	BK_LOGD(NULL, "aud_flac_encoder_test {xxx.flac xxx.wav} \r\n");
}

static void enc_metadata_cb(void * data)
{
#if 0
	FIL *enc_data = (FIL *)data;

	LOGD("sample rate	 : %u Hz\n", enc_data->meta_info->sample_rate);
	LOGD("channels		 : %u\n", enc_data->meta_info->channels);
	LOGD("bits per sample: %u\n", enc_data->meta_info->bps);
	LOGD("total samples  : %ds\n", enc_data->meta_info->total_samples);
#endif
}

static unsigned int enc_write_cb(unsigned char *buffer, long unsigned int bytes, void *data)
{
	uint32 uiTemp = 0;
	FIL *file_out = (FIL *)data;
	f_write(file_out, buffer, bytes, &uiTemp);

	return uiTemp;
}


bk_err_t aud_dec_wav_header(uint8_t      *head_buf, uint8_t size, flac_enc_meta_info_t *meta_info)
{
	BK_LOGD(NULL, "%s: decoder wav header \r\n", __func__);

	if (size != 44)
		return BK_FAIL;

	/* check wav header and validate it */
	if (os_memcmp(head_buf, "RIFF", 4) ||
		os_memcmp(head_buf+8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
		os_memcmp(head_buf+32, "\004\000\020\000data", 8)
	) {
		BK_LOGD(NULL, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed \n");
		return BK_FAIL;
	}
	meta_info->sample_rate = ((((((unsigned)head_buf[27] << 8) | head_buf[26]) << 8) | head_buf[25]) << 8) | head_buf[24];
	meta_info->channels = 2;
	meta_info->bps = 16;
	meta_info->total_samples = (((((((unsigned)head_buf[43] << 8) | head_buf[42]) << 8) | head_buf[41]) << 8) | head_buf[40]) / 4;

	return BK_OK;
}


void cli_aud_flac_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	bk_err_t ret = BK_OK;
	uint32 uiTemp = 0;
	uint8_t *head_buf = NULL;

	if (argc != 3) {
		cli_aud_flac_help();
		return;
	}

	buffer = os_malloc(READSIZE*2*2);
	pcm = os_malloc(READSIZE*2*4);

	enc_setup = os_malloc(sizeof(flac_enc_setup_t));
	if (enc_setup == NULL) {
		BK_LOGD(NULL, "malloc enc_setup fail \r\n");
		return;
	}

	enc_setup->meta_info = os_malloc(sizeof(flac_enc_meta_info_t));
	if (enc_setup->meta_info == NULL) {
		BK_LOGD(NULL, "malloc enc_setup->meta_info fail \r\n");
		return;
	}

	file_out = os_malloc(sizeof(FIL));
	if (file_out == NULL) {
		BK_LOGD(NULL, "malloc file out fail \r\n");
		return;
	}

	sprintf(in_file_name, "1:/%s", argv[1]);
	fr = f_open(&file_in, in_file_name, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "open %s fail.\r\n", in_file_name);
		return;
	}
	sprintf(out_file_name, "1:/%s", argv[2]);
	fr = f_open(file_out, out_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "open %s fail.\r\n", out_file_name);
		return;
	}

	enc_setup->metadata_callback = enc_metadata_cb;
	enc_setup->write_callback = enc_write_cb;
	enc_setup->param = file_out;

	head_buf = os_malloc(44);
	f_read(&file_in, head_buf, 44, &uiTemp);
	aud_dec_wav_header(head_buf, 44, enc_setup->meta_info);
	os_free(head_buf);

	BK_LOGD(NULL, "enc_setup->meta_info->total_samples:%d \r\n", enc_setup->meta_info->total_samples);
	BK_LOGD(NULL, "enc_setup->meta_info->sample_rate:%d \r\n", enc_setup->meta_info->sample_rate);
	BK_LOGD(NULL, "enc_setup->meta_info->channels:%d \r\n", enc_setup->meta_info->channels);
	BK_LOGD(NULL, "enc_setup->meta_info->bps:%d \r\n", enc_setup->meta_info->bps);

	/* init flac decoder */
	ret = bk_aud_flac_enc_init(enc_setup);
	if (ret != BK_OK) {
		BK_LOGD(NULL, "init flac encoder fail \r\n");
		return;
	}

	size_t left = (size_t)enc_setup->meta_info->total_samples;
	BK_LOGD(NULL, "left:%d \r\n", left);
	while(left) {
		size_t need = (left > READSIZE? (size_t)READSIZE : (size_t)left);
		f_read(&file_in, buffer, enc_setup->meta_info->channels*(enc_setup->meta_info->bps/8) * need, &uiTemp);
		if(uiTemp != enc_setup->meta_info->channels*(enc_setup->meta_info->bps/8) * need) {
			BK_LOGD(NULL, "ERROR: reading from WAVE file \n");
			break;
		}
		else {
			/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
			uint32_t i;
			for(i = 0; i < need*enc_setup->meta_info->channels; i++) {
				/* inefficient but simple and works on big- or little-endian machines */
				pcm[i] = (int32_t)(((int16_t)(int8_t)buffer[2*i+1] << 8) | (int16_t)buffer[2*i]);
			}
			/* feed samples to encoder */
			//flac_ret = FLAC__stream_encoder_process_interleaved(encoder, (FLAC__int32 *)buffer, need);
			ret = bk_aud_flac_enc_process((int32_t *)pcm, need);
			if (ret != BK_OK) {
				BK_LOGD(NULL, "encoder ok, left:%d \r\n", left);
				break;
			}
		}
		left -= need;
	}

	BK_LOGD(NULL, "break while \r\n");
	fr = f_close(file_out);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "close out file %s fail!\r\n", out_file_name);
		return;
	}

	fr = f_close(&file_in);
	if (fr != FR_OK) {
		BK_LOGD(NULL, "close out file %s fail!\r\n", in_file_name);
		return;
	}

	bk_aud_flac_enc_deinit();

	file_empty_flag = false;
	os_free(buffer);
	os_free(pcm);
	os_free(file_out);
	os_free(enc_setup->meta_info);
	os_free(file_out);
	os_free(enc_setup);

	BK_LOGD(NULL, "test finish \r\n");
}

#define AUD_FLAC_ENC_CMD_CNT (sizeof(s_aud_flac_enc_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_flac_enc_commands[] = {
	/* memory is not enough */
	{"aud_flac_encoder_test", "aud_flac_encoder_test {xxx.wav xxx.flac}", cli_aud_flac_encoder_test_cmd},
};

int cli_aud_flac_enc_init(void)
{
	return cli_register_commands(s_aud_flac_enc_commands, AUD_FLAC_ENC_CMD_CNT);
}


