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
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "onboard_mic_stream.h"
#include "fatfs_stream.h"
#include "audio_record.h"


#define TAG  "AUD_RECORD_SDCARD"

#define TU_QITEM_COUNT      (4)

#define AUDIO_RECORD_TO_SDCARD_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGE(TAG, "AUDIO_RECORD_TO_SDCARD_CHECK_NULL fail \n");\
			goto audio_record_exit;\
		}\
	} while(0)


typedef struct {
	audio_pipeline_handle_t pipeline;
	audio_element_handle_t onboard_mic;
	audio_element_handle_t fatfs_writer;
	audio_record_setup_t setup;
	beken_thread_t audio_record_task_hdl;
	beken_queue_t audio_record_msg_que;
} audio_record_info_t;

static audio_record_info_t *audio_record_info = NULL;

bk_err_t audio_record_send_msg(audio_record_op_t op, void *param)
{
	bk_err_t ret;
	audio_record_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (audio_record_info && audio_record_info->audio_record_msg_que) {
		ret = rtos_push_to_queue(&audio_record_info->audio_record_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			BK_LOGE(TAG, "aud_tras_send_int_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

/* The "onboard mic stream" element is a producer. The element is the first element.
   Usually this element only has src.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+
   |   onboard-mic   |               |       fatfs       |
   |     stream      |               |    stream[out]    |
   |                 |               |                   |
   |                src - ringbuf - sink                 |
   |                 |               |                   |
   |                 |               |                   |
   +-----------------+               +-------------------+

   Function: Use onboard mic stream to record audio data to tfcard.
*/
void audio_record_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	audio_event_iface_handle_t evt = NULL;

	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start");

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	audio_record_info->pipeline = audio_pipeline_init(&pipeline_cfg);
	AUDIO_RECORD_TO_SDCARD_CHECK_NULL(audio_record_info->pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
	onboard_mic_cfg.adc_cfg.samp_rate = audio_record_info->setup.samp_rate;
	audio_record_info->onboard_mic = onboard_mic_stream_init(&onboard_mic_cfg);
	AUDIO_RECORD_TO_SDCARD_CHECK_NULL(audio_record_info->onboard_mic);
	fatfs_stream_cfg_t fatfs_writer_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_writer_cfg.type = AUDIO_STREAM_WRITER;
	fatfs_writer_cfg.buf_sz = onboard_mic_cfg.adc_cfg.chl_num * onboard_mic_cfg.adc_cfg.samp_rate * onboard_mic_cfg.adc_cfg.bits / 1000 / 8 * 20;
	audio_record_info->fatfs_writer = fatfs_stream_init(&fatfs_writer_cfg);
	AUDIO_RECORD_TO_SDCARD_CHECK_NULL(audio_record_info->fatfs_writer);

	if (BK_OK != audio_element_set_uri(audio_record_info->fatfs_writer, audio_record_info->setup.file_name)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		goto audio_record_exit;
	}

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");

	if (BK_OK != audio_pipeline_register(audio_record_info->pipeline, audio_record_info->onboard_mic, "onboard_mic")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		goto audio_record_exit;
	}
	if (BK_OK != audio_pipeline_register(audio_record_info->pipeline, audio_record_info->fatfs_writer, "fatfs_out")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		goto audio_record_exit;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	if (BK_OK != audio_pipeline_link(audio_record_info->pipeline, (const char *[]) {"onboard_mic", "fatfs_out"}, 2)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		goto audio_record_exit;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	evt = audio_event_iface_init(&evt_cfg);

	if (BK_OK != audio_pipeline_set_listener(audio_record_info->pipeline, evt)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		goto audio_record_exit;
	}

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
	if (BK_OK != audio_pipeline_run(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		goto audio_record_exit;
	}

	while (1) {
		audio_event_iface_msg_t msg;

		audio_record_msg_t record_msg;
		ret = rtos_pop_from_queue(&audio_record_info->audio_record_msg_que, &record_msg, 0);
		if (kNoErr == ret) {
			switch (record_msg.op) {
				case AUDIO_RECORD_EXIT:
					BK_LOGI(TAG, "goto: AUDIO_RECORD_EXIT \r\n");
					goto audio_record_exit;
					break;

				default:
					break;
			}
		}

		ret = audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS);//portMAX_DELAY
		if (ret != BK_OK) {
			BK_LOGI(TAG, "not receive event, error : %d \n", ret);
			continue;
		}

		if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
			&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
			&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
			BK_LOGW(TAG, "Stop event received \n");
			break;
		}
	}


audio_record_exit:

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
	}
	if (BK_OK != audio_pipeline_wait_for_stop(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_terminate(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_unregister(audio_record_info->pipeline, audio_record_info->onboard_mic)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}
	if (BK_OK != audio_pipeline_unregister(audio_record_info->pipeline, audio_record_info->fatfs_writer)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_remove_listener(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (evt && BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_pipeline_deinit(audio_record_info->pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_element_deinit(audio_record_info->onboard_mic)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
	}

	if (BK_OK != audio_element_deinit(audio_record_info->fatfs_writer)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
	}

	BK_LOGI(TAG, "--------- onboard mic test complete ----------\n");
	AUDIO_MEM_SHOW("end");


	if (audio_record_info && audio_record_info->setup.file_name) {
		os_free(audio_record_info->setup.file_name);
		audio_record_info->setup.file_name = NULL;
	}

	/* delete msg queue */
	if (audio_record_info && audio_record_info->audio_record_msg_que) {
		rtos_deinit_queue(&audio_record_info->audio_record_msg_que);
		audio_record_info->audio_record_msg_que = NULL;
	}

	/* delete task */
	audio_record_info->audio_record_task_hdl = NULL;

	if (audio_record_info) {
		os_free(audio_record_info);
		audio_record_info = NULL;
	}

	rtos_delete_thread(NULL);
}

bk_err_t audio_record_to_sdcard_start(char *file_name, uint32_t samp_rate)
{
	bk_err_t ret = BK_OK;

	audio_record_info = (audio_record_info_t *)os_malloc(sizeof(audio_record_info_t));
	if (audio_record_info) {
		audio_record_info->audio_record_task_hdl = NULL;
		audio_record_info->audio_record_msg_que = NULL;
		audio_record_info->fatfs_writer = NULL;
		audio_record_info->onboard_mic = NULL;
		audio_record_info->pipeline = NULL;
		audio_record_info->setup.file_name = NULL;
		audio_record_info->setup.file_name = (char *)os_malloc(50);
		if (audio_record_info->setup.file_name) {
			sprintf(audio_record_info->setup.file_name, "1:/%s", file_name);
			BK_LOGI(TAG, "file_name: %s \n", audio_record_info->setup.file_name);
		} else {
			BK_LOGE(TAG, "malloc file_name fail, %d \n", __LINE__);
			return BK_FAIL;
		}
		audio_record_info->setup.samp_rate = samp_rate;
	} else {
		BK_LOGE(TAG, "malloc audio_record_info fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if ((!audio_record_info->audio_record_task_hdl) && (!audio_record_info->audio_record_msg_que))
	{
		ret = rtos_init_queue(&audio_record_info->audio_record_msg_que,
							  "audio_record_msg_que",
							  sizeof(audio_record_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr)
		{
			BK_LOGE(TAG, "ceate audio record message queue fail\n");
			goto fail;
		}

		ret = rtos_create_thread(&audio_record_info->audio_record_task_hdl,
								 4,
								 "audio_record",
								 (beken_thread_function_t)audio_record_main,
								 1024 * 2,
								 NULL);
		if (ret != kNoErr)
		{
			BK_LOGE(TAG, "Error: Failed to create audio_record task \n");
			goto fail;
		}
	}
	else
	{
		goto fail;
	}

	return ret;

fail:
	if (audio_record_info && audio_record_info->setup.file_name) {
		os_free(audio_record_info->setup.file_name);
		audio_record_info->setup.file_name = NULL;
	}

	if (audio_record_info && audio_record_info->audio_record_msg_que) {
		rtos_deinit_queue(&audio_record_info->audio_record_msg_que);
		audio_record_info->audio_record_msg_que = NULL;
	}

	if (audio_record_info) {
		os_free(audio_record_info);
		audio_record_info = NULL;
	}

	return BK_FAIL;
}

bk_err_t audio_record_to_sdcard_stop(void)
{
	bk_err_t ret;
	audio_record_msg_t msg;

	msg.op = AUDIO_RECORD_EXIT;
	msg.param = NULL;
	if (audio_record_info->audio_record_msg_que) {
		ret = rtos_push_to_queue_front(&audio_record_info->audio_record_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			BK_LOGE(TAG, "audio send msg: AUDIO_RECORD_EXIT fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

