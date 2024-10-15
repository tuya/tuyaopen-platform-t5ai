// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdio.h"
#include <os/os.h>
#include <modules/pm.h>
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "onboard_speaker_stream.h"
#include "fatfs_stream.h"
#include "mp3_decoder.h"
#include "audio_play.h"


#define TAG  "AUD_PLAY_SDCARD_MP3"

#define TU_QITEM_COUNT      (4)

#define AUDIO_PLAY_SDCARD_MP3_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGE(TAG, "AUDIO_PLAY_SDCARD_MP3_CHECK_NULL fail \n");\
			goto audio_play_exit;\
		}\
	} while(0)


typedef struct {
	audio_pipeline_handle_t pipeline;
	audio_element_handle_t onboard_speaker;
	audio_element_handle_t fatfs_read;
	audio_element_handle_t mp3_decoder;
	char *file_name;
	beken_thread_t audio_play_task_hdl;
	beken_queue_t audio_play_msg_que;
} audio_play_info_t;

static audio_play_info_t *audio_play_info = NULL;

bk_err_t audio_play_send_msg(audio_play_op_t op, void *param)
{
	bk_err_t ret;
	audio_play_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (audio_play_info && audio_play_info->audio_play_msg_que) {
		ret = rtos_push_to_queue(&audio_play_info->audio_play_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			BK_LOGE(TAG, "audio_play_send_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

/* The "onboard mic stream" element is a producer. The element is the first element.
   Usually this element only has src.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+               +-------------------+
   |      fatfs      |               |        mp3        |               |  onboard-speaker  |
   |   stream[in]    |               |      decoder      |               |       stream      |
   |                 |               |                   |               |                   |
   |                src - ringbuf - sink                src - ringbuf - sink                 |
   |                 |               |                   |               |                   |
   |                 |               |                   |               |                   |
   +-----------------+               +-------------------+               +-------------------+

   Function: Use onboard mic stream to record audio data to tfcard.
*/
void audio_play_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	audio_event_iface_handle_t evt = NULL;

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_320M);

	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start");

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	audio_play_info->pipeline = audio_pipeline_init(&pipeline_cfg);
	AUDIO_PLAY_SDCARD_MP3_CHECK_NULL(audio_play_info->pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	/* fatfs read */
	BK_LOGI(TAG, "init fatfs_read element \n");
	fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_reader_cfg.buf_sz = MP3_DECODER_MAIN_BUFF_SIZE;
	fatfs_reader_cfg.out_rb_size = MP3_DECODER_MAIN_BUFF_SIZE;
	fatfs_reader_cfg.type = AUDIO_STREAM_READER;
	audio_play_info->fatfs_read = fatfs_stream_init(&fatfs_reader_cfg);
	AUDIO_PLAY_SDCARD_MP3_CHECK_NULL(audio_play_info->fatfs_read);
	if (BK_OK != audio_element_set_uri(audio_play_info->fatfs_read, audio_play_info->file_name)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		goto audio_play_exit;
	}
	/* mp3 decoder */
	BK_LOGI(TAG, "init mp3_decoder element \n");
	mp3_decoder_cfg_t mp3_decoder_cfg = DEFAULT_MP3_DECODER_CONFIG();
	audio_play_info->mp3_decoder = mp3_decoder_init(&mp3_decoder_cfg);
	AUDIO_PLAY_SDCARD_MP3_CHECK_NULL(audio_play_info->mp3_decoder);
	/* onboard speaker */
	BK_LOGI(TAG, "init onboard_speaker element \n");
	onboard_speaker_stream_cfg_t onboard_speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
	onboard_speaker_cfg.chl_num = 2;
	onboard_speaker_cfg.samp_rate = 48000;
	onboard_speaker_cfg.spk_gain = 0x10;
	audio_play_info->onboard_speaker = onboard_speaker_stream_init(&onboard_speaker_cfg);
	AUDIO_PLAY_SDCARD_MP3_CHECK_NULL(audio_play_info->onboard_speaker);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(audio_play_info->pipeline, audio_play_info->fatfs_read, "fatfs_in")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		goto audio_play_exit;
	}
	if (BK_OK != audio_pipeline_register(audio_play_info->pipeline, audio_play_info->mp3_decoder, "mp3_dec")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		goto audio_play_exit;
	}
	if (BK_OK != audio_pipeline_register(audio_play_info->pipeline, audio_play_info->onboard_speaker, "onboard_spk")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		goto audio_play_exit;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	if (BK_OK != audio_pipeline_link(audio_play_info->pipeline, (const char *[]) {"fatfs_in", "mp3_dec", "onboard_spk"}, 3)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		goto audio_play_exit;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	evt = audio_event_iface_init(&evt_cfg);

	if (BK_OK != audio_pipeline_set_listener(audio_play_info->pipeline, evt)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		goto audio_play_exit;
	}

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
	if (BK_OK != audio_pipeline_run(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		goto audio_play_exit;
	}

	while (1) {
		audio_event_iface_msg_t msg;

		audio_play_msg_t play_msg;
		ret = rtos_pop_from_queue(&audio_play_info->audio_play_msg_que, &play_msg, 0);
		if (kNoErr == ret) {
			switch (play_msg.op) {
				case AUDIO_PLAY_EXIT:
					BK_LOGI(TAG, "goto: audio_play_exit \r\n");
					goto audio_play_exit;
					break;

				default:
					break;
			}
		}

		ret = audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS);//portMAX_DELAY
		if (ret != BK_OK) {
			BK_LOGD(TAG, "not receive event, error : %d \n", ret);
			continue;
		}

		if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) audio_play_info->mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
			audio_element_info_t music_info = {0};
			audio_element_getinfo(audio_play_info->mp3_decoder, &music_info);
			BK_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, ch=%d, bits=%d \n", music_info.sample_rates, music_info.channels, music_info.bits);
			onboard_speaker_stream_set_param(audio_play_info->onboard_speaker, music_info.sample_rates, music_info.channels, music_info.bits);
/*
			audio_element_info_t onboard_speaker_info = {0};
			audio_element_getinfo(audio_play_info->onboard_speaker, &onboard_speaker_info);
			onboard_speaker_info.sample_rates = music_info.sample_rates;
			onboard_speaker_info.channels = music_info.channels;
			onboard_speaker_info.bits = music_info.bits;
			audio_element_setinfo(audio_play_info->onboard_speaker, &onboard_speaker_info);
*/
			continue;
		}

		if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
			&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
			&& (((int)msg.data == AEL_STATUS_STATE_STOPPED)
			|| ((int)msg.data == AEL_STATUS_STATE_FINISHED)
			|| (int)msg.data == AEL_STATUS_ERROR_PROCESS)) {
			BK_LOGW(TAG, "[ * ] Stop event received \n");
			break;
		}

	}


audio_play_exit:

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
	}
	if (BK_OK != audio_pipeline_wait_for_stop(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_terminate(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_unregister(audio_play_info->pipeline, audio_play_info->fatfs_read)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}
	if (BK_OK != audio_pipeline_unregister(audio_play_info->pipeline, audio_play_info->mp3_decoder)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}
	if (BK_OK != audio_pipeline_unregister(audio_play_info->pipeline, audio_play_info->onboard_speaker)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_remove_listener(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (evt && BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_deinit(audio_play_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_element_deinit(audio_play_info->fatfs_read)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_element_deinit(audio_play_info->mp3_decoder)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_element_deinit(audio_play_info->onboard_speaker)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
	}

	BK_LOGI(TAG, "--------- play sdcard mp3 music test complete ----------\n");
	AUDIO_MEM_SHOW("end");


	if (audio_play_info && audio_play_info->file_name) {
		os_free(audio_play_info->file_name);
		audio_play_info->file_name = NULL;
	}

	/* delete msg queue */
	if (audio_play_info && audio_play_info->audio_play_msg_que) {
		rtos_deinit_queue(&audio_play_info->audio_play_msg_que);
		audio_play_info->audio_play_msg_que = NULL;
	}

	/* delete task */
	audio_play_info->audio_play_task_hdl = NULL;

	if (audio_play_info) {
		os_free(audio_play_info);
		audio_play_info = NULL;
	}

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

	rtos_delete_thread(NULL);
}

bk_err_t audio_play_sdcard_mp3_music_start(char *file_name)
{
	bk_err_t ret = BK_OK;

	audio_play_info = (audio_play_info_t *)os_malloc(sizeof(audio_play_info_t));
	if (audio_play_info) {
		audio_play_info->audio_play_task_hdl = NULL;
		audio_play_info->audio_play_msg_que = NULL;
		audio_play_info->fatfs_read = NULL;
		audio_play_info->onboard_speaker = NULL;
		audio_play_info->pipeline = NULL;
		audio_play_info->file_name = NULL;
		audio_play_info->file_name = (char *)os_malloc(50);
		if (audio_play_info->file_name) {
			sprintf(audio_play_info->file_name, "1:/%s", file_name);
			BK_LOGI(TAG, "file_name: %s \n", audio_play_info->file_name);
		} else {
			BK_LOGE(TAG, "malloc file_name fail, %d \n", __LINE__);
			return BK_FAIL;
		}
	} else {
		BK_LOGE(TAG, "malloc audio_play_info fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if ((!audio_play_info->audio_play_task_hdl) && (!audio_play_info->audio_play_msg_que))
	{
		ret = rtos_init_queue(&audio_play_info->audio_play_msg_que,
							  "audio_play_msg_que",
							  sizeof(audio_play_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr)
		{
			BK_LOGE(TAG, "ceate audio record message queue fail\n");
			goto fail;
		}

		ret = rtos_create_thread(&audio_play_info->audio_play_task_hdl,
								 4,
								 "audio_play",
								 (beken_thread_function_t)audio_play_main,
								 1024 * 2,
								 NULL);
		if (ret != kNoErr)
		{
			BK_LOGE(TAG, "Error: Failed to create audio_play task \n");
			goto fail;
		}
	}
	else
	{
		goto fail;
	}

	return ret;

fail:
	if (audio_play_info && audio_play_info->file_name) {
		os_free(audio_play_info->file_name);
		audio_play_info->file_name = NULL;
	}

	if (audio_play_info && audio_play_info->audio_play_msg_que) {
		rtos_deinit_queue(&audio_play_info->audio_play_msg_que);
		audio_play_info->audio_play_msg_que = NULL;
	}

	if (audio_play_info) {
		os_free(audio_play_info);
		audio_play_info = NULL;
	}

	return BK_FAIL;
}

bk_err_t audio_play_sdcard_mp3_music_stop(void)
{
	bk_err_t ret;
	audio_play_msg_t msg;

	msg.op = AUDIO_PLAY_EXIT;
	msg.param = NULL;
	if (audio_play_info->audio_play_msg_que) {
		ret = rtos_push_to_queue_front(&audio_play_info->audio_play_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			BK_LOGE(TAG, "audio send msg: AUDIO_PLAY_EXIT fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

