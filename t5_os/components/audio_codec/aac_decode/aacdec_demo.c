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
#include <os/mem.h>
#include <os/str.h>
#include "stdio.h"
#include <modules/aacdec.h>
#if CONFIG_FATFS
#include "ff.h"
#endif
#include <driver/uart.h>
#include "gpio_driver.h"
#include <modules/pm.h>

//#include "BK7256_RegList.h"


#define TF_dump

static unsigned char *readPtr = NULL;
static uint32_t count = 0;
static uint32_t read_size_count = 0;
static bool file_empty = false;

static void uart_dump_init(void)
{
	/* init uart */
	uart_config_t config = {0};
	os_memset(&config, 0, sizeof(uart_config_t));
	gpio_dev_unmap(GPIO_0);
	gpio_dev_map(GPIO_0, GPIO_DEV_UART2_TXD);
	gpio_dev_unmap(GPIO_1);
	gpio_dev_map(GPIO_1, GPIO_DEV_UART2_RXD);
	config.baud_rate = 2000000;
	config.data_bits = UART_DATA_8_BITS;
	config.parity = UART_PARITY_NONE;
	config.stop_bits = UART_STOP_BITS_1;
	config.flow_ctrl = UART_FLOWCTRL_DISABLE;
	config.src_clk = UART_SCLK_XTAL_26M;
	if (bk_uart_init(1, &config) != BK_OK) {
		os_printf("init uart fail \r\n");
		return;
	}
}


void cli_m4a_decoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
//	char aac_file_name[50] = "";
//	char pcm_file_name[50] = "";

	char aac_file_name[50] = "1:/aac_test.aac";
	char pcm_file_name[50] = "1:/aac_test.pcm";
	FIL aac_file;
	FIL pcm_file;
	FRESULT fr;
	uint32 uiTemp = 0;
//	FSIZE_t test_data_size = 0;

//	unsigned long samplerate;
//	unsigned char channels;
//	void *sample_buffer;
//	NeAACDecHandle hDecoder;
//	NeAACDecFrameInfo frameInfo;
	HAACDecoder hAACDecoder = NULL;
	AACFrameInfo aacFrameInfo = {0};

	int fileread = 0, bytesconsumed = 0, k = 0;
	int buffer_index = 0;
	unsigned char *buffer = NULL;
	unsigned int bytes_in_buffer = 0;

	int bytesLeft = 0;
	short *sample_buffer = NULL;
	int err = 0;


	os_printf("-----------start test----------- \n");

//	os_memset(aac_file_name, 0, sizeof(aac_file_name)/sizeof(aac_file_name[0]));
//	sprintf(aac_file_name, "1:/%s", argv[1]);
//	os_memset(pcm_file_name, 0, sizeof(pcm_file_name)/sizeof(pcm_file_name[0]));
//	sprintf(pcm_file_name, "1:/%s", argv[2]);

	os_printf("aac_file_name: %s \n", aac_file_name);
	os_printf("pcm_file_name: %s \n", pcm_file_name);

	fr = f_open(&aac_file, aac_file_name, FA_READ);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", aac_file_name);
		return;
	}

	fr = f_open(&pcm_file, pcm_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", pcm_file_name);
		return;
	}

	os_printf("-----------open file ok----------- \n");

	fileread = f_size(&aac_file);
	os_printf("fileread: %d \n", fileread);
	buffer = (unsigned char*)os_malloc(AAC_MAINBUF_SIZE);
	os_memset(buffer, 0, AAC_MAINBUF_SIZE);

	//bytes_in_buffer = fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, file);
	fr = f_read(&aac_file, buffer, AAC_MAINBUF_SIZE, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read aac file fail. line: %d\r\n", __LINE__);
		return;
	} else {
		bytes_in_buffer += uiTemp;
//		readPtr = buffer;
		os_printf("read aac file ok. bytes_in_buffer: %d\r\n", bytes_in_buffer);
		read_size_count += uiTemp;
		os_printf("read_size_count: %d \n", read_size_count);
	}

	sample_buffer = os_malloc(AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2);
	if (!sample_buffer) {
		os_printf("malloc sample_buffer: %d fail.\r\n", AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2);
		return;
	}

	os_printf("-----------open file ok----------- \n");
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO,PM_CPU_FRQ_320M);

#ifndef TF_dump
	uart_dump_init();
#endif
	/* init  */
	hAACDecoder = AACInitDecoder();
	if (!hAACDecoder) {
		os_printf("AACInitDecoder fail\n");
		goto fail;
	}
	os_printf("-----------AACInitDecoder ok----------- \n");

	/* Set the default object type and samplerate */
	/* This is useful for RAW AAC files */
	os_memset(&aacFrameInfo, 0, sizeof(AACFrameInfo));
	aacFrameInfo.nChans = 2;
	aacFrameInfo.sampRateCore = 44100;
	aacFrameInfo.profile = AAC_PROFILE_LC;
	aacFrameInfo.bitsPerSample = 16;
	if (AACSetRawBlockParams(hAACDecoder, 0, &aacFrameInfo) != 0) {
		os_printf("AACSetRawBlockParams fail\n");
		goto fail;
	}
	os_printf("-----------AACSetRawBlockParams ok----------- \n");

	do
	{
		os_printf("-----------NeAACDecDecode--%d--------- \n", count);
		rtos_delay_milliseconds(50);
		/* update buffer */
		if (bytesconsumed > 0) {
			for (k = 0; k < (AAC_MAINBUF_SIZE - bytesconsumed); k++) {
				buffer[k] = buffer[k + bytesconsumed];
				//os_printf("buffer[%d]: %02x, buffer[%d]: %02x \n", k, buffer[k], k + bytesconsumed, buffer[k + bytesconsumed]);
			}
			if (!file_empty) {
				fr = f_read(&aac_file, buffer + (AAC_MAINBUF_SIZE) - bytesconsumed, bytesconsumed, &uiTemp);
				if (fr != FR_OK) {
					os_printf("read aac file fail. line: %d\r\n", __LINE__);
					goto fail;
				} else {
					bytes_in_buffer += uiTemp;
//					readPtr = buffer;
					os_printf("read aac file size: %d, need size: %d \n", uiTemp, bytesconsumed);
					read_size_count += uiTemp;
					os_printf("read_size_count: %d \n", read_size_count);
					if (read_size_count == fileread) {
						file_empty = true;
					}
				}
			}
			bytesconsumed = 0;
		}

		readPtr = buffer;
		bytesLeft = bytes_in_buffer;
//		addAON_GPIO_Reg0x8 = 2;
		err = AACDecode(hAACDecoder, &readPtr, &bytesLeft, sample_buffer);
//		addAON_GPIO_Reg0x8 = 0;
		if (err == 0) {
			/* no error */
			AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);
/*
			os_printf("aacFrameInfo.bitRate: %d \n", aacFrameInfo.bitRate);
			os_printf("aacFrameInfo.nChans: %d \n", aacFrameInfo.nChans);
			os_printf("aacFrameInfo.sampRateCore: %d \n", aacFrameInfo.sampRateCore);
			os_printf("aacFrameInfo.sampRateOut: %d \n", aacFrameInfo.sampRateOut);
			os_printf("aacFrameInfo.bitsPerSample: %d \n", aacFrameInfo.bitsPerSample);
			os_printf("aacFrameInfo.outputSamps: %d \n", aacFrameInfo.outputSamps);
			os_printf("aacFrameInfo.profile: %d \n", aacFrameInfo.profile);
			os_printf("aacFrameInfo.tnsUsed: %d \n", aacFrameInfo.tnsUsed);
			os_printf("aacFrameInfo.pnsUsed: %d \n", aacFrameInfo.pnsUsed);
*/
			bytesconsumed += bytes_in_buffer - bytesLeft;
			buffer_index += bytes_in_buffer - bytesLeft;
			bytes_in_buffer -= bytesconsumed;

			os_printf("bytesconsumed: %d \n", bytesconsumed);
//			os_printf(" bytesconsumed: %d \n buffer_index: %d \n bytes_in_buffer: %d\n", bytesconsumed, buffer_index, bytes_in_buffer);

			if (aacFrameInfo.outputSamps > 0)
			{
#ifdef TF_dump
				fr = f_write(&pcm_file, (void *)sample_buffer, aacFrameInfo.outputSamps * 2, &uiTemp);
				if (fr != FR_OK) {
					os_printf("write output data %s fail.\r\n", pcm_file_name);
					break;
				}
#else
				/* write data to uart */
				bk_uart_write_bytes(1, (void *)sample_buffer, aacFrameInfo.outputSamps * 2);
#endif
			}
		} else {
			/* error */
			os_printf("AACDecode Error: %d \n", err);
#if 0
			switch (err) {
			case ERR_AAC_INDATA_UNDERFLOW:
				puts("ERR_AAC_INDATA_UNDERFLOW");
				//outOfData = 1;
				// try to read more data
				// seek backwards to reread partial frame at end of current buffer
				// TODO: find out why it doesn't work if the following line is uncommented
				//aacfile->FilePtr -= bytesLeftBefore;
				f_read(aacfile, (BYTE *)aacbuf, aacbuf_size, &bytes_read);
				fr = f_read(&aac_file, buffer + (AAC_MAINBUF_SIZE) - bytesconsumed, bytesconsumed, &uiTemp);

				if (bytes_read == aacbuf_size) {
					// TODO: reuse writable_buffer
					readPtr = aacbuf;
					offset = 0;
					bytesLeft = aacbuf_size;
					puts("indata underflow, reading more data");
				} else {
					puts("can't read more data");
					return -1;
				}
				break;

				default:
					iprintf("unknown error: %i\n", err);
					// skip this frame
					if (bytesLeft > 0) {
						bytesLeft --;
						readPtr ++;
					} else {
						// TODO
						assert(0);
					}
					break;
				}
#endif
		}

		count++;

		if (buffer_index >= fileread)
			sample_buffer = NULL; /* to make sure it stops now */
	} while (sample_buffer != NULL);

fail:

	fr = f_close(&aac_file);
	if (fr != FR_OK) {
		os_printf("close mic file %s fail!\r\n", aac_file_name);
		return;
	}

	fr = f_close(&pcm_file);
	if (fr != FR_OK) {
		os_printf("close out file %s fail!\r\n", pcm_file_name);
		return;
	}

	AACFreeDecoder(hAACDecoder);

	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}

	if (sample_buffer) {
		os_free(sample_buffer);
		sample_buffer = NULL;
	}

	readPtr = NULL;

	os_printf("test finish \r\n");

}

