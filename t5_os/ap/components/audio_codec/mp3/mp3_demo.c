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

#include "cli.h"

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/dma.h>
#include "sys_driver.h"
#include "mp3_demo_types.h"
#include <driver/audio_ring_buff.h>
#include <modules/mp3dec.h>
#include "ff.h"
#include "diskio.h"

#define TU_QITEM_COUNT      (60)
#define PCM_SIZE_MAX		(MAX_NSAMP * MAX_NCHAN * MAX_NGRAN)

static beken_thread_t audio_mp3_play_thread_hdl = NULL;
static beken_queue_t audio_mp3_play_msg_que = NULL;

HMP3Decoder hMP3Decoder;
MP3FrameInfo mp3FrameInfo;
unsigned char *readBuf;
short *pcmBuf;
int bytesLeft = 0;
int g_mp3_decode_status = 0;
int g_mp3_decode_complete_status = 0;
int g_mp3_play_is_running = 0;
short *output;

FIL mp3file;
FIL pcmfile;
uint32 uiTemp = 0;
uint32 br = 0;
static char *g_mp3_name;
static char mp3_file_name[50];
int offset = 0;
unsigned char *g_readptr;

static dma_id_t mp3_dac_dma_id = DMA_ID_MAX;
static int32_t *mp3_play_ring_buff = NULL;
static RingBufferContext mp3_play_rb;


static bk_err_t audio_send_msg(audio_mp3_play_msg_t msg)
{
	bk_err_t ret = BK_OK;

	if (audio_mp3_play_msg_que) {
		ret = rtos_push_to_queue(&audio_mp3_play_msg_que, &msg, BEKEN_NO_WAIT);
		if (ret != kNoErr) {
			BK_LOGD(NULL, "audio_send_msg failed\r\n");
			return kGeneralErr;
		}
		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t bk_audio_mp3_play_dac_config(void)
{
	bk_err_t ret = BK_OK;
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	dac_config.dac_chl = AUD_DAC_CHL_LR;
	/* init audio driver and config dac */
	ret = bk_aud_dac_init(&dac_config);
	if (ret != BK_OK) {
		BK_LOGD(NULL, "init audio dac fail \r\n");
		goto aud_dac_exit;
	}

	return BK_OK;

aud_dac_exit:
	BK_LOGD(NULL, "audio dac config fail \r\n");
	bk_aud_dac_deinit();
	return BK_FAIL;

}

static void audio_mp3_play_dma_finish_isr(void)
{
	bk_err_t ret = BK_OK;
	audio_mp3_play_msg_t msg;
	
	msg.op = AUDIO_MP3_PLAY_START;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "mp3 play send msg: %d fails \r\n", msg.op);
	}

}

static bk_err_t bk_audio_mp3_play_dma_config(dma_id_t dma_id, int32_t *ring_buff_addr, uint32_t ring_buff_size, uint32_t transfer_len)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t dac_port_addr;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_AUDIO;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	/* get dac fifo address */
	if (bk_aud_dac_get_fifo_addr(&dac_port_addr) != BK_OK) {
		BK_LOGD(NULL, "get dac fifo address failed\r\n");
		return BK_FAIL;
	} else {
		dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.dst.start_addr = dac_port_addr;
		dma_config.dst.end_addr = dac_port_addr + 4;
	}
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = (uint32_t)ring_buff_addr;
	dma_config.src.end_addr = (uint32_t)(ring_buff_addr) + ring_buff_size;

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		BK_LOGD(NULL, "audio dac dma channel init fail \r\n");
		return BK_FAIL;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, transfer_len);

	//register isr
	bk_dma_register_isr(dma_id, NULL, (void *)audio_mp3_play_dma_finish_isr);
	bk_dma_enable_finish_interrupt(dma_id);

	return BK_OK;

}

void bk_audio_mp3_play_decode_init(void)
{
	readBuf = os_malloc(MAINBUF_SIZE);
	if (readBuf == NULL) {
		BK_LOGD(NULL, "readBuf malloc failed!\r\n");
		return;
	}

	pcmBuf = os_malloc(PCM_SIZE_MAX * 2);
	if (pcmBuf == NULL) {
		BK_LOGD(NULL, "pcmBuf malloc failed!\r\n");
		return;
	}

	hMP3Decoder = MP3InitDecoder();
	if (hMP3Decoder == 0) {
		os_free(readBuf);
		os_free(pcmBuf);
		BK_LOGD(NULL, "MP3Decoder init failed!\r\n");
		return;
	}

}

static void bk_audio_mp3_play_decode_start(DISK_NUMBER disk_id, char *mp3_name, unsigned char **mp3_readptr)
{
	bk_err_t ret = BK_OK;
	int offset = 0;
	char tag_header[10];
	int tag_size = 0;
	unsigned char *readptr = *mp3_readptr;
	audio_mp3_play_msg_t msg;

	FRESULT fr;

	if (!g_mp3_decode_status) {
		/*open file to read mp3 data */
		os_memset(mp3_file_name, 0, sizeof(mp3_file_name)/sizeof(mp3_file_name[0]));
		sprintf(mp3_file_name, "%d:/%s", disk_id, mp3_name);
		fr = f_open(&mp3file, mp3_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "open %s failed!\r\n", mp3_file_name);
			return;
		}

		fr = f_read(&mp3file, (void *)tag_header, 10, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read %s failed!\r\n", mp3_file_name);
			return;
		}

		if (os_memcmp(tag_header, "ID3", 3) == 0) {
			tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
//			BK_LOGD(NULL, "tag_size = %d\r\n", tag_size);
			f_lseek(&mp3file, tag_size + 10);
		} else {
			BK_LOGD(NULL, "tag_header not found!\r\n");
			f_lseek(&mp3file, 0);
		}

		g_mp3_decode_status = 1;
	}

	/* start mp3 decode */
	if (bytesLeft < MAINBUF_SIZE) {
		os_memmove(readBuf, readptr, bytesLeft);
//		BK_LOGD(NULL, "mp3file bytesLeft = %d!\r\n", bytesLeft);
		fr = f_read(&mp3file, (void *)(readBuf + bytesLeft), MAINBUF_SIZE - bytesLeft, &br);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read %s failed!\r\n", mp3_file_name);
			return;
		}

		if ((br == 0) && (bytesLeft == 0)) {
			BK_LOGD(NULL, "uiTemp = 0 and bytesLeft = 0!\r\n");
			msg.op = AUDIO_MP3_PLAY_EXIT;
			ret = audio_send_msg(msg);
			if (ret != kNoErr) {
				BK_LOGD(NULL, "mp3 play send msg: %d fails \r\n", msg.op);
			}
			return;
		}

		bytesLeft = bytesLeft + br;
		readptr = readBuf;
	}

	offset = MP3FindSyncWord(readptr, bytesLeft);
//	BK_LOGD(NULL, "MP3FindSyncWord offset = %d!\r\n", offset);

	if (offset < 0) {
		BK_LOGD(NULL, "MP3FindSyncWord not find!\r\n");
		bytesLeft = 0;
		msg.op = AUDIO_MP3_PLAY_EXIT;
		ret = audio_send_msg(msg);
		if (ret != kNoErr) {
			BK_LOGD(NULL, "mp3 play send msg: %d fails \r\n", msg.op);
		}
	} else {
		readptr += offset;
		bytesLeft -= offset;
		
		ret = MP3Decode(hMP3Decoder, &readptr, &bytesLeft, pcmBuf, 0);
		if (ret != ERR_MP3_NONE) {
			BK_LOGD(NULL, "MP3Decode failed, err is %d", ret);
			return;
		}

		MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
//		BK_LOGD(NULL, "Bitrate: %d kb/s, Samprate: %d\r\n", (mp3FrameInfo.bitrate) / 1000, mp3FrameInfo.samprate);
//		BK_LOGD(NULL, "Channel: %d, Version: %d, Layer: %d\r\n", mp3FrameInfo.nChans, mp3FrameInfo.version, mp3FrameInfo.layer);
//		BK_LOGD(NULL, "OutputSamps: %d\r\n", mp3FrameInfo.outputSamps);
		*mp3_readptr = readptr;
	}

}

static void audio_mp3_play_main(void)
{
	bk_err_t ret = BK_OK;
	unsigned char *readptr;
	uint32_t size = 0;

	ret = bk_audio_mp3_play_dac_config();
	if (ret != BK_OK) {
		BK_LOGD(NULL, "audio dac init failed!\r\n");
		return;
	}

	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		BK_LOGD(NULL, "dma driver init failed!\r\n");
		return;
	}

	/* allocate free DMA channel */
	mp3_dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((mp3_dac_dma_id < DMA_ID_0) || (mp3_dac_dma_id >= DMA_ID_MAX)) {
		BK_LOGD(NULL, "mp3 dma malloc failed!\r\n");
		return;
	}
//	BK_LOGD(NULL, "mp3_dac_dma_id: %d \r\n", mp3_dac_dma_id);

	/* start mp3 init and decode */
	bk_audio_mp3_play_decode_init();
	readptr = readBuf;
	bk_audio_mp3_play_decode_start(DISK_NUMBER_SDIO_SD, g_mp3_name, &readptr);


	mp3_play_ring_buff = os_malloc(PCM_SIZE_MAX * 5);
	if (mp3_play_ring_buff == NULL) {
		BK_LOGD(NULL, "mp3 play ring buffer malloc failed!\r\n");
		return;
	}

	ret = bk_audio_mp3_play_dma_config(mp3_dac_dma_id, mp3_play_ring_buff, PCM_SIZE_MAX * 5, mp3FrameInfo.outputSamps * 2);
	if (ret != BK_OK) {
		BK_LOGD(NULL, "audio mp3 play dma config failed!\r\n");
		return;
	}

	ring_buffer_init(&mp3_play_rb, (uint8_t*)mp3_play_ring_buff, PCM_SIZE_MAX * 5, mp3_dac_dma_id, RB_DMA_TYPE_READ);

	bk_aud_dac_set_samp_rate(mp3FrameInfo.samprate);

	size = ring_buffer_write(&mp3_play_rb, (uint8_t *)pcmBuf, mp3FrameInfo.outputSamps * 2);
//	BK_LOGD(NULL, "ring buffer write size = %d!\r\n", size);

	bk_audio_mp3_play_decode_start(DISK_NUMBER_SDIO_SD, g_mp3_name, &readptr);
	size = ring_buffer_write(&mp3_play_rb, (uint8_t *)pcmBuf, mp3FrameInfo.outputSamps * 2);
//	BK_LOGD(NULL, "ring buffer write size = %d!\r\n", size);

	bk_aud_dac_start();
	bk_dma_start(mp3_dac_dma_id);
	bk_aud_dac_start();

	while(1) {
		audio_mp3_play_msg_t msg;
		ret = rtos_pop_from_queue(&audio_mp3_play_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUDIO_MP3_PLAY_PREV:
					break;

				case AUDIO_MP3_PLAY_NEXT:
					break;

				case AUDIO_MP3_PLAY_START:
					size = ring_buffer_get_free_size(&mp3_play_rb);
//					BK_LOGD(NULL, "speaker_rb: free_size=%d \r\n", size);
					if (g_mp3_decode_complete_status == 1) {
						if (size > PCM_SIZE_MAX * 5 - mp3FrameInfo.outputSamps * 2) {
							goto audio_mp3_play_exit;
						} else {
							break;
						}
					}
					
					if (size > PCM_SIZE_MAX) {
						bk_audio_mp3_play_decode_start(DISK_NUMBER_SDIO_SD, g_mp3_name, &readptr);
						size = ring_buffer_write(&mp3_play_rb, (uint8_t *)pcmBuf, mp3FrameInfo.outputSamps * 2);
//						BK_LOGD(NULL, "ring buffer write size = %d!\r\n", size);
					}
					break;

				case AUDIO_MP3_PLAY_STOP:
					break;

				case AUDIO_MP3_PLAY_EXIT:
					g_mp3_decode_complete_status = 1;
					break;

				default:
					break;
			}
		}
//		rtos_delay_milliseconds(10);
	}

audio_mp3_play_exit:
	bytesLeft = 0;
	f_close(&mp3file);
	g_mp3_play_is_running = 0;
	g_mp3_decode_status = 0;
	g_mp3_decode_complete_status = 0;

	MP3FreeDecoder(hMP3Decoder);
    os_free(readBuf);
    os_free(pcmBuf);

	bk_aud_dac_stop();
	bk_aud_dac_deinit();

	bk_dma_stop(mp3_dac_dma_id);
	bk_dma_deinit(mp3_dac_dma_id);
	bk_dma_free(DMA_DEV_AUDIO, mp3_dac_dma_id);

	/* free audio ring buffer */
	ring_buffer_clear(&mp3_play_rb);
	os_free(mp3_play_ring_buff);

	/* delete msg queue */
	ret = rtos_deinit_queue(&audio_mp3_play_msg_que);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "delete message queue failed!\r\n");
	}
	audio_mp3_play_msg_que = NULL;
	BK_LOGD(NULL, "delete message queue completed!\r\n");

	audio_mp3_play_thread_hdl = NULL;
	ret = rtos_delete_thread(NULL);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "delete task failed!\r\n");
	}

}

bk_err_t bk_audio_mp3_play_init(void)
{
	bk_err_t ret = BK_OK;

	g_mp3_play_is_running = 1;

	ret = rtos_init_queue(&audio_mp3_play_msg_que,
							"audio_mp3_play_queue",
							sizeof(audio_mp3_play_msg_t),
							TU_QITEM_COUNT);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "create audio mp3 play message queue failed!\r\n");
		return BK_FAIL;
	}

	ret = rtos_create_thread(&audio_mp3_play_thread_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "audio_mp3_play",
							 (beken_thread_function_t)audio_mp3_play_main,
							 4096,
							 NULL);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "create audio mp3 play task fail!\r\n");
		rtos_deinit_queue(&audio_mp3_play_msg_que);
		audio_mp3_play_msg_que = NULL;
		audio_mp3_play_thread_hdl = NULL;
	}

	return ret;
}

bk_err_t bk_audio_mp3_play_deinit(void)
{
	bk_err_t ret = BK_OK;

	audio_mp3_play_msg_t msg;

	msg.op = AUDIO_MP3_PLAY_EXIT;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		BK_LOGD(NULL, "send msg: %d fails \r\n", msg.op);
		return BK_FAIL;
	}

	return ret;
}

void cli_mp3_play_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc != 3) {
		BK_LOGD(NULL, "aud_cp0_audio_mp3_play_test {start|stop} xx.mp3 \r\n");
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		BK_LOGD(NULL, "start audio mp3 play test!\r\n");

		g_mp3_name = argv[2];

		if (!g_mp3_play_is_running) {
			/* init audio mp3 play task */
			ret = bk_audio_mp3_play_init();
			if (ret != BK_OK) {
				BK_LOGD(NULL, "init audio mp3 play task fail!\r\n");
				return;
			}
		} else {
			BK_LOGD(NULL, "mp3 play task is running, please input stop command firstly!\r\n");
			return;
		}

	} else if (os_strcmp(argv[1], "stop") == 0) {
		BK_LOGD(NULL, "stop audio mp3 play test!\r\n");

		ret = bk_audio_mp3_play_deinit();
		if (ret != BK_OK) {
			BK_LOGD(NULL, "init audio mp3 play task fail!\r\n");
			return;
		}
	}
}

static void bk_audio_mp3_decode(void)
{
	bk_err_t ret = BK_OK;

	FRESULT fr;
	uint32 uiTemp = 0;
	uint32 uiTemp2 = 0;
	int offset = 0;
	unsigned char *readptr = readBuf;

	while(1) {
		if (bytesLeft < MAINBUF_SIZE) {
			os_memmove(readBuf, readptr, bytesLeft);
			BK_LOGD(NULL, "mp3file bytesLeft = %d!\r\n", bytesLeft);
			fr = f_read(&mp3file, (void *)(readBuf + bytesLeft), MAINBUF_SIZE - bytesLeft, &uiTemp);
			if (fr != FR_OK) {
				BK_LOGD(NULL, "read %s failed!\r\n", mp3_file_name);
				return;
			}

			if ((uiTemp == 0) && (bytesLeft == 0)) {
				BK_LOGD(NULL, "uiTemp = 0 and bytesLeft = 0\r\n");
				return;
			}

			bytesLeft = bytesLeft + uiTemp;
			readptr = readBuf;
		}

		offset = MP3FindSyncWord(readptr, bytesLeft);
		BK_LOGD(NULL, "MP3FindSyncWord offset = %d!\r\n", offset);

		if (offset < 0) {
			BK_LOGD(NULL, "MP3FindSyncWord not find!\r\n");
			bytesLeft = 0;
		} else {
			readptr += offset;
			bytesLeft -= offset;
			
			ret = MP3Decode(hMP3Decoder, &readptr, &bytesLeft, output, 0);
			if (ret != ERR_MP3_NONE) {
				BK_LOGD(NULL, "MP3Decode failed, code is %d", ret);
				return;
			} 

			fr = f_write(&pcmfile, (void *)output, mp3FrameInfo.outputSamps * 2, &uiTemp2);
			if (fr != FR_OK) {
				BK_LOGD(NULL, "write pcm file failed.\r\n");
				return;
			}

			MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
			BK_LOGD(NULL, "Bitrate: %d kb/s, Samprate: %d\r\n", (mp3FrameInfo.bitrate) / 1000, mp3FrameInfo.samprate);
			BK_LOGD(NULL, "Channel: %d, Version: %d, Layer: %d\r\n", mp3FrameInfo.nChans, mp3FrameInfo.version, mp3FrameInfo.layer);
			BK_LOGD(NULL, "OutputSamps: %d\r\n", mp3FrameInfo.outputSamps);
		}
	}
}

void cli_mp3_decode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	static char pcm_file_name[50];
	uint32 uiTemp = 0;
	char tag_header[10];
	int tag_size = 0;

	readBuf = os_malloc(MAINBUF_SIZE);
	if (readBuf == NULL) {
		BK_LOGD(NULL, "readBuf malloc failed\r\n");
		return;
	}

	output = os_malloc(PCM_SIZE_MAX * 2);
	if (output == NULL) {
		os_free(readBuf);
		BK_LOGD(NULL, "outBuf malloc failed");
		return;
    }

	if (argc != 4) {
		BK_LOGD(NULL, "mp3_decode_test {start|stop} xx.mp3 xx.pcm \r\n");
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		BK_LOGD(NULL, "start audio mp3 decode test!\r\n");

		hMP3Decoder = MP3InitDecoder();
		if (hMP3Decoder == 0) {
			os_free(readBuf);
			os_free(output);
			BK_LOGD(NULL, "hMP3Decoder malloc failed!\r\n");
			return;
		}

		BK_LOGD(NULL, "MP3InitDecoder init successful!\r\n");

		/*open file to read mp3 data */
		sprintf(mp3_file_name, "%d:/%s", DISK_NUMBER_SDIO_SD, argv[2]);
		fr = f_open(&mp3file, mp3_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			MP3FreeDecoder(hMP3Decoder);
			os_free(readBuf);
			os_free(output);
			BK_LOGD(NULL, "open %s failed!\r\n", mp3_file_name);
			return;
		}
		BK_LOGD(NULL, "mp3 file open successfully!\r\n");

		sprintf(pcm_file_name, "%d:/%s", DISK_NUMBER_SDIO_SD, argv[3]);
		fr = f_open(&pcmfile, pcm_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "open %s failed!\r\n", pcm_file_name);
			return;
		}
		BK_LOGD(NULL, "pcm file open successfully!\r\n");

		fr = f_read(&mp3file, (void *)tag_header, 10, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "read %s failed!\r\n", mp3_file_name);
			return;
		}
		BK_LOGD(NULL, "mp3file read successfully!\r\n");

		if (os_memcmp(tag_header, "ID3", 3) == 0) {
			tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
			BK_LOGD(NULL, "tag_size = %d\r\n", tag_size);
			f_lseek(&mp3file, tag_size + 10);
			BK_LOGD(NULL, "tag_header has found!\r\n");
		} else {
			BK_LOGD(NULL, "tag_header not found!\r\n");
			f_lseek(&mp3file, 0);
		}

		bk_audio_mp3_decode();

		fr = f_close(&pcmfile);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "close %s fail!\r\n", pcm_file_name);
			return;
		}

		BK_LOGD(NULL, "start audio mp3 play test successful!\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		MP3FreeDecoder(hMP3Decoder);
	    os_free(readBuf);
	    os_free(output);

		/* close mp3 file */
		fr = f_close(&mp3file);
		if (fr != FR_OK) {
			BK_LOGD(NULL, "close %s fail!\r\n", mp3_file_name);
			return;
		}

		BK_LOGD(NULL, "stop audio mp3 play test successful!\r\n");
	} else {
		BK_LOGD(NULL, "mp3_decode_test {start|stop} xx.mp3 xx.pcm \r\n");
		return;
	}

}


#define MP3_CMD_CNT (sizeof(s_mp3_commands) / sizeof(struct cli_command))
static const struct cli_command s_mp3_commands[] = {
	{"mp3_decode_test", "mp3_decode_test {start|stop} xx.mp3 xx.pcm", cli_mp3_decode_test_cmd},
	{"mp3_play_test", "mp3_play_test {start|stop} xx.mp3", cli_mp3_play_test_cmd},
};

int cli_mp3_init(void)
{
	return cli_register_commands(s_mp3_commands, MP3_CMD_CNT);
}

