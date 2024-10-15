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
#include <stdio.h>
#include <string.h>
#include <components/event.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <ff.h>
#include "diskio.h"
#include "fatfs_stream_player.h"
#include "audio_player.h"
#include "audio_codec.h"

#define TAG  "fatfs_stream_player"
#define TU_QITEM_COUNT      (60)

#define GET_FATFS_BUFSZ      1024 * 20
static beken_thread_t fatfs_stream_player_thread_hdl = NULL; 
static beken_queue_t fatfs_stream_player_msg_queue = NULL; 
uint32_t psram_buffer = 0x60000000;
unsigned char *psram_end_addr = NULL;
uint8_t fatfs_stream_load_stop_flag = 0;
static unsigned char *fatfs_stream_buffer = NULL;

static FATFS *pfs = NULL;
int file_list_index = 0;
FIL tf_mp3file;
uint32 tf_br = 0;
char tf_g_mp3_name[50];
FILINFO tf_mp3_file[MP3_FILE_COUNT_MAX];
int mp3_file_count = 0;


bk_err_t audio_tf_card_mount(int number)
{
	bk_err_t fr;
    char cFileName[FF_MAX_LFN];
	if (pfs != NULL)
	{
		os_free(pfs);
	}

	pfs = os_malloc(sizeof(FATFS));
	if(NULL == pfs)
	{
		BK_LOGI(TAG, "f_mount malloc failed!\r\n");
		return BK_FAIL;
	}
    sprintf(cFileName, "%d:", number);
	fr = f_mount(pfs, cFileName, 1);
	if (fr != FR_OK)
	{
		BK_LOGE(TAG, "f_mount failed:%d\r\n", fr);
		return BK_FAIL;
	}
	else
	{
		BK_LOGI(TAG, "f_mount OK!\r\n");
	}

	return BK_OK;
}

int is_mp3_file(char *file_name)
{
	int len = strlen(file_name);

	if (len < 5)
	{
		return 0;
	}
	
	if ((file_name[len - 1] == '3') &&
	  	((file_name[len - 2] == 'p') || (file_name[len - 2] == 'P')) &&
	  	((file_name[len - 3] == 'm') || (file_name[len - 3] == 'M')) &&
	  	(file_name[len - 4] == '.'))
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

bk_err_t audio_tf_card_scan_files
(
    char *path        /* Start node to be scanned (***also used as work area***) */
)
{
    bk_err_t fr;
    DIR dir;
    FILINFO fno;
    fr = f_opendir(&dir, path);                 /* Open the directory */
    if (fr == FR_OK)
    {
        // BK_LOGI(TAG, "path: %s/\r\n", path);
        while (1)
        {
        	
            fr = f_readdir(&dir, &fno);         /* Read a directory item */
            if (fr != FR_OK)
            {
                break;  /* Break on error */
            }
            if (fno.fname[0] == 0)
            {
                break;  /* Break on end of dir */
            }
            if (fno.fattrib & AM_DIR)
            {
                /* It is a directory */
                char *pathTemp = os_malloc(strlen(path)+strlen(fno.fname)+2);
                if(pathTemp == 0)
                {
                    BK_LOGE(TAG, "%s:os_malloc dir fail \r\n", __func__);
                    break;
                }
                sprintf(pathTemp, "%s/%s", path, fno.fname);
                fr = audio_tf_card_scan_files(pathTemp);      /* Enter the directory */
                if (fr != FR_OK)
                {
                    os_free(pathTemp);
                    pathTemp = 0;
                    break;
	  			}
                if(pathTemp)
                {
                    os_free(pathTemp);
                    pathTemp = 0;
	         	}
            }
            else
            {
                /* It is a file. */
				if (is_mp3_file(fno.fname))
				{
					os_memcpy(&tf_mp3_file[mp3_file_count], (void *)&fno, sizeof(FILINFO));
					mp3_file_count++;
 					if (mp3_file_count >= MP3_FILE_COUNT_MAX)
 					{
 						mp3_file_count = MP3_FILE_COUNT_MAX;
						break;
					}
 				}
			}
        }
        f_closedir(&dir);
    }
    else
    {
        BK_LOGI(TAG, "f_opendir failed\r\n");
    }

    return fr;
}

bk_err_t audio_tf_card_scan(int number)
{
    bk_err_t fr;
    char cFileName[FF_MAX_LFN];

    BK_LOGI(TAG, "\r\n----- scan_file_system %d start -----\r\n", number);

    sprintf(cFileName, "%d:", number);
    fr = audio_tf_card_scan_files(cFileName);
    if (fr != FR_OK)
    {
        BK_LOGI(TAG, "audio_fatfs_scan_files failed!\r\n");
		return fr;
    }
    else
    {
	    
        BK_LOGI(TAG, "audio_fatfs_scan_files OK!\r\n");
		BK_LOGI(TAG, "----- scan_file_system %d over  -----\r\n\r\n", number);
		return BK_OK;
    }

}

int audio_tf_get_mp3_files_count(void)
{
	return mp3_file_count - 1;
}

static void audio_tf_psram_word_memcpy(void *dst_t, void *src_t, unsigned int length)
{
	unsigned int *dst = (unsigned int*)dst_t, *src = (unsigned int*)src_t;
	int index = (unsigned int)dst & 0x03;
	int count = length >> 2, tail = length & 0x3;
	unsigned int tmp = 0;
	unsigned char *p, *pp = (unsigned char *)src;

	if (!index)
	{
		while (count--)
		{
			*dst++ = pp[0] | pp[1] << 8 | pp[2] << 16 | pp[3] << 24;
			pp += 4;
		}

		if (tail)
		{
			tmp = *dst;
			p = (unsigned char *)&tmp;

			while(tail--)
			{
				*p++ = *pp++;
			}
			*dst = tmp;
		}
	}
	else
	{
		unsigned int *pre_dst = (unsigned int*)((unsigned int)dst & 0xFFFFFFFC);
		unsigned int pre_count = 4 - index;
		tmp = *pre_dst;
		p = (unsigned char *)&tmp + index;

		if (pre_count > length) {
			pre_count = length;
		}

		while (pre_count--)
		{
			*p++ = *pp++;
			length--;
		}

		*pre_dst = tmp;

		if (length <= 0)
		{
			return;
		}

		dst = pre_dst + 1;
		count = length >> 2;
		tail = length & 0x3;

		while (count--)
		{
			*dst++ = pp[0] | pp[1] << 8 | pp[2] << 16 | pp[3] << 24;
			pp += 4;
		}

		if (tail)
		{
			tmp = *dst;
			p = (unsigned char *)&tmp;

			while(tail--)
			{
				*p++ = *pp++;
			}
			*dst = tmp;
		}
	}
}


static bk_err_t fatfs_stream_player_start(void)
{
	bk_err_t fr = BK_OK;
	int bytes_read;
	int content_length = -1;
	uint32_t *paddr = NULL;
	paddr = (uint32_t *)psram_buffer;
	fatfs_stream_load_stop_flag = 0;


	// print sd mp3 file list
	BK_LOGI(TAG, "current song file_list_index: %d, song name: %s, song length: %d\n\r", file_list_index, tf_mp3_file[file_list_index].fname, tf_mp3_file[file_list_index].fsize);
	for (int i = 0; i< mp3_file_count; i++)
	{
		BK_LOGI(TAG, "[%d] %s\r\n", i, tf_mp3_file[i].fname);
	}

	os_memset(tf_g_mp3_name, 0, sizeof(tf_g_mp3_name)/sizeof(tf_g_mp3_name[0]));
	sprintf(tf_g_mp3_name, "%d:/%s", DISK_NUMBER_SDIO_SD, tf_mp3_file[file_list_index].fname);

	/*open file to read mp3 data to psram*/

	//always open file failed so try open 5 times
	int re_open = 0;
	do
	{
		/* code */
		fr = f_open(&tf_mp3file, tf_g_mp3_name, FA_OPEN_EXISTING | FA_READ);
		re_open++;
	} while (fr != FR_OK && re_open < 5);
	
	if (fr != FR_OK && re_open == 5)
	{
		re_open = 0;
		BK_LOGE(TAG, "try open mp3 file 5 times failed!!!\n\r");
		return fr;
	}
	re_open = 0;

	content_length = tf_mp3_file[file_list_index].fsize;
	BK_LOGI(TAG, "mp3 file name: %s, size: %d\n\r", tf_g_mp3_name, tf_mp3_file[file_list_index].fsize);
	BK_LOGI(TAG, "song len: %d, song time: %ds\n\r", content_length, (content_length / 16000));
	psram_end_addr = (unsigned char *)psram_buffer + content_length;

	// add caculate song time 
	audio_player_song_time_reset();
	song_time.song_time = content_length/16000;
	song_time.song_uri_id = file_list_index;
	
	// add lv_music_play act
	audio_player_msg_t msg;
	msg.op = AUDIO_PLAYER_START;
	fr = audio_player_send_msg(msg);
	if (fr != kNoErr) {
		BK_LOGE(TAG, "audio player send msg: %d fail\r\n", msg.op);
		return fr;
	}

	//read sd file data to psram
	int content_pos = 0;
	do
	{
		if (!fatfs_stream_load_stop_flag)
		{
			fr = f_read(&tf_mp3file, (void *)fatfs_stream_buffer, tf_mp3_file[file_list_index].fsize - content_pos > GET_FATFS_BUFSZ ?
								GET_FATFS_BUFSZ : tf_mp3_file[file_list_index].fsize - content_pos, &tf_br);
			if (fr != FR_OK) {
				BK_LOGE(TAG, "read %s failed!\r\n", tf_g_mp3_name);
				return fr;
			}

			bytes_read = tf_br;
			if(bytes_read != GET_FATFS_BUFSZ) {
				if (bytes_read % 4) {
					bytes_read = (bytes_read / 4 + 1) * 4;
				}
				audio_tf_psram_word_memcpy((uint32_t *)paddr, (uint32_t *)fatfs_stream_buffer, bytes_read);
				BK_LOGI(TAG, "load mp3 file:%s to psram end\n\r", tf_mp3_file[file_list_index].fname);
				fatfs_stream_load_stop_flag = 1;
			} else {
				// os_memcpy_word
				audio_tf_psram_word_memcpy((uint32_t *)paddr, (uint32_t *)fatfs_stream_buffer, GET_FATFS_BUFSZ);
				paddr += (GET_FATFS_BUFSZ / 4);
			}

			if (content_pos == GET_FATFS_BUFSZ * 10) {
				fr = audio_codec_mp3_decoder_start();
				if (fr != BK_OK) {
					BK_LOGE(TAG,"audio_codec_mp3_decoder_start error %d\n\r", fr);
					break;
				}

				fr = audio_player_play();
				// ret = audio_player_pause();
				if (fr != BK_OK) {
					BK_LOGE(TAG,"audio_player_play error %d\n\r", fr);
					break;
				}
			}

			content_pos += bytes_read;
			rtos_delay_milliseconds(2);			
		}
		else
		{
			BK_LOGI(TAG, "fatfs stream download has been stopped\r\n");
			break;
		}
	} while (content_pos < content_length);

	return fr;
}

bk_err_t fatfs_stream_player_scan(void)
{
	bk_err_t err_t = audio_tf_card_mount(1);
	if (err_t != 0)
	{
		BK_LOGE(TAG,"fatfs stream mount sdcard error %d\n\r", err_t);
		return err_t;
	}
	audio_tf_card_scan(1);

	for (int i = 0; i< mp3_file_count; i++)
	{
		BK_LOGI(TAG, "[%d] %s  --%d\r\n", i, tf_mp3_file[i].fname, tf_mp3_file[i].fsize);
	}

	err_t = audio_player_init();
	if (err_t != BK_OK)	{
		BK_LOGE(TAG,"audio player init fail\r\n");
		return err_t;
	}	
	return err_t;
}

static void fatfs_stream_player_main(beken_thread_arg_t param)
{
	int ret = 0;

	fatfs_stream_buffer = (unsigned char *)os_malloc(GET_FATFS_BUFSZ);
	if (fatfs_stream_buffer == NULL) {
		BK_LOGE(TAG, "no memory for receive buffer\r\n");
		goto __exit;
	}

	while(1) {
		fatfs_stream_player_msg_t msg;
		ret = rtos_pop_from_queue(&fatfs_stream_player_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case FATFS_STREAM_PLAYER_SCAN:
					fatfs_stream_player_scan();
					break;
				case FATFS_STREAM_PLAYER_START:
					ret = fatfs_stream_player_start();
					if (ret != BK_OK) {
						BK_LOGE(TAG, "fatfs stream player start fail\r\n");
					}
					break;

				case FATFS_STREAM_PLAYER_STOP:
					fatfs_stream_player_stop();
					break;
				case FATFS_STREAM_PLAYER_RESUME:
					// audio_player_resume();
					break;
				case FATFS_STREAM_PLAYER_NEXT:
					break;
				case FATFS_STREAM_PLAYER_EXIT:
					goto __exit;
					break;
				default:
					break;
			}
		}
	}

__exit:
	rtos_deinit_queue(&fatfs_stream_player_msg_queue);
	fatfs_stream_player_msg_queue = NULL;

	fatfs_stream_player_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t fatfs_stream_player_stop(void)
{
	if (fatfs_stream_load_stop_flag) {

	} else {
		fatfs_stream_load_stop_flag = 1;
	}

	BK_LOGI(TAG, "fatfs stream player stop complete\r\n");

	return BK_OK;
}

bk_err_t fatfs_stream_player_send_msg(fatfs_stream_player_msg_t msg)
{
	bk_err_t ret = BK_OK;

	if (fatfs_stream_player_msg_queue) {
		ret = rtos_push_to_queue(&fatfs_stream_player_msg_queue, &msg, BEKEN_NO_WAIT);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "fatfs stream player send message fail\r\n");
			return kGeneralErr;
		}
		return ret;
	}
	return kNoResourcesErr;
}

bk_err_t fatfs_stream_player_init(void)
{
	bk_err_t ret = BK_OK;

	ret = rtos_init_queue(&fatfs_stream_player_msg_queue,
						  "fatfs_stream_load_queue",
						  sizeof(fatfs_stream_player_msg_t),
						  TU_QITEM_COUNT);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "create fatfs stream player message queue fail\r\n");
		return BK_FAIL;
	}

	ret = rtos_create_thread(&fatfs_stream_player_thread_hdl,
							 5,
							 "fatfs_stream_download",
							 (beken_thread_function_t)fatfs_stream_player_main,
							 1024 * 2,
							 NULL);
	if (ret != kNoErr) {
		rtos_deinit_queue(&fatfs_stream_player_msg_queue);
		fatfs_stream_player_msg_queue = NULL;
		BK_LOGE(TAG, "create fatfs stream player thread fail\r\n");
		fatfs_stream_player_thread_hdl = NULL;
	}

	// scan sd card mp3 files first for lvgl show  file list
	fatfs_stream_player_scan();
	
	return ret;
}

bk_err_t fatfs_stream_player_deinit(void)
{
	bk_err_t ret = BK_OK;

	fatfs_stream_player_stop();

	fatfs_stream_player_msg_t load_msg;
	load_msg.op = FATFS_STREAM_PLAYER_EXIT;
	ret = fatfs_stream_player_send_msg(load_msg);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "fatfs stream player send msg: %d fail\r\n", load_msg.op);
	}

	return ret;
}

