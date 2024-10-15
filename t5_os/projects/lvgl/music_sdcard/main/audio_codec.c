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
#include <modules/mp3dec.h>
#include "aud_intf.h"
#include "aud_intf_types.h"
#include "audio_player.h"
#include "audio_codec.h"
#include "fatfs_stream_player.h"


#define TAG  "audio_codec"

#define PCM_SIZE_MAX		(MAX_NSAMP * MAX_NCHAN * MAX_NGRAN)

HMP3Decoder fs_hMP3Decoder;
MP3FrameInfo fs_mp3FrameInfo;
unsigned char *fs_readBuf;
short *fs_pcmBuf;
int fs_bytesLeft = 0;
int fs_offset = 0;
unsigned char *fs_g_readptr;
static unsigned char *psram_ptr = NULL;
static uint8_t aud_player_spk_init_flag = 0;
extern uint32_t psram_buffer;
extern unsigned char *psram_end_addr;


static const uint16_t AUDIO_SAMPLE_RATES[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};

static bk_err_t audio_codec_mp3_decoder_init(void)
{
	fs_readBuf = os_malloc(MAINBUF_SIZE);
	if (fs_readBuf == NULL) {
		BK_LOGI(TAG,"fs_readBuf malloc failed!\r\n");
		return BK_FAIL;
	}

	fs_pcmBuf = os_malloc(PCM_SIZE_MAX * 2);
	if (fs_pcmBuf == NULL) {
		BK_LOGI(TAG,"fs_pcmBuf malloc failed!\r\n");
		return BK_FAIL;
	}

	fs_hMP3Decoder = MP3InitDecoder();
	if (fs_hMP3Decoder == 0) {
		os_free(fs_readBuf);
		os_free(fs_pcmBuf);
		BK_LOGI(TAG,"MP3Decoder init failed!\r\n");
		return BK_FAIL;
	}

	fs_g_readptr = fs_readBuf;
	psram_ptr = (unsigned char *)psram_buffer;

	return BK_OK;
}

static void audio_codec_mp3_decoder_deinit(void)
{
	fs_bytesLeft = 0;
	fs_offset = 0;
	aud_player_spk_init_flag = 0;

	MP3FreeDecoder(fs_hMP3Decoder);
	os_free(fs_readBuf);
	os_free(fs_pcmBuf);
}

static void audio_codec_mp3_find_id3(void)
{
	char tag_header[10];
	int tag_size = 0;

	os_memcpy((void *)tag_header, (const void *)psram_buffer, 10);

	if (os_memcmp(tag_header, "ID3", 3) == 0) {
		tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
		BK_LOGI(TAG, "tag_size = %d\r\n", tag_size);
		psram_ptr += tag_size + 10;
	} else {
		BK_LOGI(TAG, "tag_header not found!\r\n");
	}
}

static void audio_codec_mp3_print_err_fram(unsigned char *inbuf, int bytesLeft, unsigned char *psram_addr)
{
	BK_LOGE(TAG, "sram fram:\n\r");
	for (int i = 0; i < 100; i++)
	{
		os_printf("[%d]%x ", i, inbuf[i]);
	}
	BK_LOGE(TAG, "\n\rpsram fram:\n\r");
	for (int i = 0; i < 100; i++)
	{
		os_printf("[%d]%x ", i, psram_addr[i]);
	}
	BK_LOGE(TAG, "\n\r");
}

void down_current_song_data_from_psram(void)
{
	os_printf("%s start: %x\n\r", __func__, psram_ptr);
	unsigned char *psram_ptr_bak = NULL;
	psram_ptr_bak = (unsigned char *)(psram_ptr - MAINBUF_SIZE);

	for (int i = 0; i < 2 *  MAINBUF_SIZE; i++)
	{
		os_printf("%x ", psram_ptr_bak[i]);
	}
	os_printf("down_current_song_data_from_psram end\n\r");
}

uint8_t audio_player_pause_flag = 0;
bk_err_t audio_codec_mp3_decoder_handler(unsigned int size)
{
	bk_err_t ret = BK_OK;
	uint8_t idx = 0;
	static uint32_t song_rate_tag = 0; 

	if (psram_ptr >= psram_end_addr || song_time.song_current_time == song_time.song_time) {
		
		BK_LOGI(TAG,"play current song end, song_time: %ds\n\r", song_time.song_current_time);
		audio_player_msg_t msg;
		msg.op = AUDIO_PLAYER_NEXT;
		ret = audio_player_send_msg(msg);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "audio player send msg: %d fail\r\n", msg.op);
			return ret;
		}
		song_rate_tag = 0;
		return BK_OK;
	}

	if (audio_player_pause_flag != 1)
	{
		if (fs_bytesLeft < MAINBUF_SIZE) {
			os_memmove(fs_readBuf, fs_g_readptr, fs_bytesLeft);
			os_memcpy((void *)(fs_readBuf + fs_bytesLeft), (const void *)psram_ptr, MAINBUF_SIZE - fs_bytesLeft);
			psram_ptr += MAINBUF_SIZE - fs_bytesLeft;
			fs_bytesLeft = MAINBUF_SIZE;
			fs_g_readptr = fs_readBuf;
			song_rate_tag++;
			song_time.song_current_time = song_rate_tag / 38;
		}
		
		fs_offset = MP3FindSyncWord(fs_g_readptr, fs_bytesLeft);
		if (fs_offset < 0) {
			BK_LOGI(TAG, "MP3FindSyncWord not find!\r\n");
			fs_bytesLeft = 0;
		} else {
			fs_g_readptr += fs_offset;
			fs_bytesLeft -= fs_offset;

			ret = MP3Decode(fs_hMP3Decoder, &fs_g_readptr, &fs_bytesLeft, fs_pcmBuf, 0);
			if (ret == ERR_MP3_INVALID_HUFFCODES)
			{
				BK_LOGE(TAG, "MP3Decode failed, code is %d", ret);
				return BK_OK;
			}
			else if (ret != ERR_MP3_NONE && ret != ERR_MP3_INVALID_HUFFCODES)
			{
				BK_LOGE(TAG, "MP3Decode failed, code is %d fs_offset: %d, fs_bytesLeft: %d\n\r", ret, fs_offset, fs_bytesLeft);
				//audio_codec_mp3_print_err_fram(fs_g_readptr, fs_bytesLeft, (psram_ptr_bak + fs_offset-fs_bytesLeft_bak));
				//rtos_delay_milliseconds(5);
				///down_current_song_data_from_psram();
				//rtos_delay_milliseconds(3000);	
				// audio_player_stop();
				bk_reboot();
				return ret;
			}

			MP3GetLastFrameInfo(fs_hMP3Decoder, &fs_mp3FrameInfo);

			/* write a frame speaker data to speaker_ring_buff */
			ret = bk_aud_intf_write_spk_data((uint8_t*)fs_pcmBuf, fs_mp3FrameInfo.outputSamps * 2);
			if (ret != BK_OK) {
				BK_LOGE(TAG, "write spk data fail \r\n");
				return ret;
			}

			if (!aud_player_spk_init_flag) {
				aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();

				for (idx = 0; idx < sizeof(AUDIO_SAMPLE_RATES) / sizeof(AUDIO_SAMPLE_RATES[0]); idx++) {
					if (AUDIO_SAMPLE_RATES[idx] == fs_mp3FrameInfo.samprate) {
						aud_intf_spk_setup.samp_rate = idx;
						break;
					}
				}
				aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_DUAL;
				aud_intf_spk_setup.frame_size = fs_mp3FrameInfo.outputSamps * 2;
				aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
				ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
				if (ret != BK_ERR_AUD_INTF_OK) {
					BK_LOGE(TAG, "audio player spk init fail, ret:%d \r\n", ret);
					return ret;
				}
				song_time.song_current_time = 0;
				song_rate_tag = 0;
				BK_LOGI(TAG,"set mp3 decode samp_rate: %d, fs_mp3FrameInfo.outputSamps: %d\n\r",  fs_mp3FrameInfo.samprate, fs_mp3FrameInfo.outputSamps);
				aud_player_spk_init_flag = 1;
			}
		}
	}

	return ret;
}

bk_err_t audio_codec_mp3_decoder_start(void)
{
	bk_err_t ret = BK_OK;

	ret = audio_codec_mp3_decoder_init();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "audio codec mp3 decoder init fail\r\n");
		return ret;
	}

	BK_LOGI(TAG, "audio codec mp3 decoder init complete\r\n");

	audio_codec_mp3_find_id3();

	ret = audio_codec_mp3_decoder_handler(0);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "audio player decode fail\r\n");
		return ret;
	}

	return ret;
}

bk_err_t audio_codec_mp3_decoder_stop(void)
{
	if (aud_player_spk_init_flag == 1)
	{
		audio_codec_mp3_decoder_deinit();
	}

	return BK_OK;
}


