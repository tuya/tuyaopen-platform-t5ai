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

#include "os/os.h"
#include "audio_player.h"
#include "audio_codec.h"
#include "fatfs_stream_player.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#include "lv_demo_music_main.h"
#include "lv_vendor.h"

#define TAG  "audio_player"

#define TU_QITEM_COUNT      10

audio_player_song_time_t song_time;
static uint8_t is_first_file_paly = 1;
static beken_thread_t audio_player_thread_hdl = NULL; 
static beken_queue_t audio_player_msg_queue = NULL; 

extern const char *song_list[];
extern uint32_t current_song, song_number;
extern uint8_t audio_player_pause_flag;


static void audio_player_main(beken_thread_arg_t param);


bk_err_t audio_player_start(void)
{
	bk_err_t ret = BK_OK;

	fatfs_stream_player_msg_t msg;
	msg.op = FATFS_STREAM_PLAYER_START;
	ret = fatfs_stream_player_send_msg(msg);
	if (ret != kNoErr) {
		BK_LOGI(TAG, "netstream player send msg: %d fail\r\n", msg.op);
		return ret;
	}
	is_first_file_paly = 0;
	BK_LOGI(TAG, "audio player start complete\r\n");

	return ret;
}

bk_err_t audio_player_stop(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_aud_intf_spk_stop();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio intf spk stop fail, ret:%d \r\n", ret);
		return ret;
	}

	BK_LOGI(TAG, "audio intf spk stop complete\r\n");

	ret = bk_aud_intf_spk_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio intf drvier deinit fail, ret:%d \r\n", ret);
		return ret;
	}

	BK_LOGI(TAG, "audio player stop complete\r\n");

	return ret;
}

bk_err_t audio_player_pause(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_aud_intf_spk_pause();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio player pause fail, ret:%d\r\n", ret);
	}
	audio_player_pause_flag = 1;
	BK_LOGI(TAG, "audio player pause complete\r\n");

	return ret;
}

bk_err_t audio_player_play(void)
{
	bk_err_t ret = BK_OK;
	if (is_first_file_paly == 1)
	{
		audio_player_start();
	}
	else
	{
		ret = bk_aud_intf_spk_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			BK_LOGE(TAG, "audio player play fail, ret:%d \r\n", ret);
			return ret;
		}
	}

	audio_player_pause_flag = 0;
	
	BK_LOGI(TAG, "audio player play complete\r\n");

	return ret;
}

bk_err_t audio_player_set_volume(uint8_t vol_value)
{
	bk_err_t ret = BK_OK;

	ret = bk_aud_intf_set_spk_gain(vol_value);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio player volume set fail, ret:%d \r\n", ret);
	}

	BK_LOGI(TAG, "audio player volume set complete\r\n");

	return ret;
}

bk_err_t audio_player_pre(void)
{
	bk_err_t ret = BK_OK;

	if (file_list_index == 0)
	{
		file_list_index = mp3_file_count - 1;
	}
	else
	{
		file_list_index--;
	}

	if (is_first_file_paly == 1)
	{
		audio_player_start();
	}
	else
	{
		fatfs_stream_player_stop();

		audio_codec_mp3_decoder_stop();

		ret = audio_player_stop();
		if (ret != BK_OK) {
			BK_LOGE(TAG, "audio player stop fail, ret:%d \r\n", ret);
			return ret;
		}

		fatfs_stream_player_msg_t load_msg;
		load_msg.op = FATFS_STREAM_PLAYER_START;
		ret = fatfs_stream_player_send_msg(load_msg);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "fatfs stream player send msg: %d fail\r\n", load_msg.op);
			return ret;
		}
	}

	return ret;
}

bk_err_t audio_player_next(void)
{
	bk_err_t ret = BK_OK;;
	// audio_codec_mp3_decoder_stop();

	if (file_list_index == mp3_file_count - 1)
	{
		file_list_index = 0;
	}
	else
	{
		file_list_index++;
	}

	if (is_first_file_paly == 1)
	{
		audio_player_start();
	}
	else
	{
		fatfs_stream_player_stop();

		audio_codec_mp3_decoder_stop();

		ret = audio_player_stop();
		if (ret != BK_OK) {
			BK_LOGE(TAG, "audio player stop fail, ret:%d \r\n", ret);
			return ret;
		}

		fatfs_stream_player_msg_t load_msg;
		load_msg.op = FATFS_STREAM_PLAYER_START;
		ret = fatfs_stream_player_send_msg(load_msg);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "fatfs stream player send msg: %d fail\r\n", load_msg.op);
			return ret;
		}
	}

	return ret;
}

bk_err_t audio_player_appoint(uint32_t idx)
{
	bk_err_t ret = BK_OK;;

	file_list_index = idx;

	if (is_first_file_paly == 1)
	{
		audio_player_start();
	}
	else
	{
		fatfs_stream_player_stop();
			
		audio_codec_mp3_decoder_stop();

		ret = audio_player_stop();
		if (ret != BK_OK) {
			BK_LOGE(TAG, "audio player stop fail, ret:%d \r\n", ret);
			return ret;
		}

		fatfs_stream_player_msg_t load_msg;
		load_msg.op = FATFS_STREAM_PLAYER_START;
		ret = fatfs_stream_player_send_msg(load_msg);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "fatfs stream player send msg: %d fail\r\n", load_msg.op);
	}
return ret;
	}

	return ret;

}


bk_err_t audio_player_resume(void)
{
	bk_err_t ret = BK_OK;

	fatfs_stream_player_stop();

	fatfs_stream_player_msg_t load_msg;
	load_msg.op = FATFS_STREAM_PLAYER_START;
	ret = fatfs_stream_player_send_msg(load_msg);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "fatfs stream player send msg: %d fail\r\n", load_msg.op);
		return ret;
	}

	return ret;
}

uint32_t audio_player_get_song_length(void)
{
	return song_time.song_time;
}

uint32_t audio_player_get_song_rate(void)
{
	return song_time.song_current_time;
}

uint32_t audio_player_get_song_url_id(void)
{
	return song_time.song_uri_id;
}

void audio_player_song_time_reset(void)
{
	song_time.song_current_time = 0;
	song_time.song_time = 0;
	song_time.song_uri_id = 0;
}

bk_err_t audio_player_init(void)
{
	bk_err_t ret = BK_OK;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

	aud_intf_drv_setup.aud_intf_rx_spk_data = audio_codec_mp3_decoder_handler;
	ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio intf drvier init fail, ret:%d\r\n", ret);
		return ret;
	}

	aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
	ret = bk_aud_intf_set_mode(aud_work_mode);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "audio intf mode set fail, ret:%d \r\n", ret);
		return ret;
	}

	ret = rtos_init_queue(&audio_player_msg_queue,
						  "audio_player_queue",
						  sizeof(audio_player_msg_t),
						  TU_QITEM_COUNT);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "create audio player message queue fail\r\n");
		return BK_FAIL;
	}

	ret = rtos_create_thread(&audio_player_thread_hdl,
							 2,
							 "audio player",
							 (beken_thread_function_t)audio_player_main,
							 1024 * 2,
							 NULL);
	if (ret != kNoErr) {
		rtos_deinit_queue(&audio_player_msg_queue);
		audio_player_msg_queue = NULL;
		BK_LOGE(TAG, "create audio player thread fail\r\n");
		audio_player_thread_hdl = NULL;
	}


	BK_LOGI(TAG, "audio player init complete\r\n");

	return ret;
}

bk_err_t audio_player_deinit(void)
{
	bk_err_t ret = BK_OK;

	ret = fatfs_stream_player_deinit();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "netstream player deinit fail, ret:%d \r\n", ret);
		return ret;
	}

	ret = audio_player_stop();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "audio player stop fail, ret:%d \r\n", ret);
		return ret;
	}

	audio_codec_mp3_decoder_stop();

	ret = bk_aud_intf_drv_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(TAG, "bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		return ret;
	}

	audio_player_msg_t player_msg;
	player_msg.op = AUDIO_PLAYER_EXIT;
	ret = audio_player_send_msg(player_msg);
	if (ret != kNoErr) {
		BK_LOGE(TAG,"audio player send msg: %d fail\r\n", player_msg.op);
		return ret;
	}

	BK_LOGI(TAG, "audio player deinit complete\r\n");

	return ret;
}

static void audio_player_main(beken_thread_arg_t param)
{
	int ret = 0;

	while(1) {
		audio_player_msg_t msg;
		ret = rtos_pop_from_queue(&audio_player_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUDIO_PLAYER_START:
					lv_vendor_disp_lock();
					_lv_demo_music_play(song_time.song_uri_id);
					lv_vendor_disp_unlock();
					break;
				case AUDIO_PLAYER_STOP:
					ret = audio_player_stop();
					if (ret != BK_OK) {
						BK_LOGE(TAG, "audio player stop fail\r\n");
					}
					break;
				case AUDIO_PLAYER_RESUME:
					break;
				case AUDIO_PLAYER_NEXT:
					audio_player_next();
					break;
				case AUDIO_PLAYER_PRE:
					break;
				case AUDIO_PLAYER_EXIT:
					goto __exit;
					break;
				default:
					break;
			}
		}
	}

__exit:

	rtos_deinit_queue(&audio_player_msg_queue);
	audio_player_msg_queue = NULL;

	audio_player_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t audio_player_send_msg(audio_player_msg_t msg)
{
	bk_err_t ret = BK_OK;

	if (audio_player_msg_queue) {
		ret = rtos_push_to_queue(&audio_player_msg_queue, &msg, BEKEN_NO_WAIT);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "audio player send message fail\r\n");
			return kGeneralErr;
		}
		return ret;
	}
	return kNoResourcesErr;
}



