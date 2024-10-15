// Copyright 2022-2023 Beken
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
#include "FreeRTOS.h"
#include "task.h"
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "raw_stream.h"
#include "onboard_mic_stream.h"
#include "onboard_speaker_stream.h"


#define TAG  "RAW_TEST"

#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

#define TEST_NUM 100
static int read_count = 0;


/* The "onboard mic stream" element is a producer. The "onboard speaker stream" element is a consumer.
   The pipeline "onboard_mic --> raw" record mic data. The pipeline "raw --> onboard_speaker" play audio data.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+                               +-----------------+               +-------------------+
   |   onboard-mic   |               |        raw        |                               |      raw        |               |  onboard-speaker  |
   |     stream      |               |   stream[read]    |          +----------+         |  stream[write]  |               |    stream[out]    |
   |                 |               |                   |   read   |          |  write  |                 |               |                   |
   |                src - ringbuf - sink                 | -------> | usr task | ------> |                src - ringbuf - sink                 |
   |                 |               |                   |          |          |         |                 |               |                   |
   |                 |               |                   |          +----------+         |                 |               |                   |
   +-----------------+               +-------------------+                               +-----------------+               +-------------------+

   Function: User task read mic data from "onboard_mic --> raw" pipeline, and write audio data to "raw --> onboard_speaker" pipeline.

   The speaker play audio data from mic.
*/
bk_err_t asdf_raw_test_case_0(void)
{
	audio_pipeline_handle_t record_pipeline, play_pipeline;
	audio_element_handle_t onboard_mic, raw_read;
	audio_element_handle_t raw_write, onboard_speaker;
#if 0
	bk_set_printf_sync(true);
		extern void bk_enable_white_list(int enabled);
		bk_enable_white_list(1);
		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
		bk_disable_mod_printf("AUDIO_EVENT", 0);
//		bk_disable_mod_printf("AUDIO_MEM", 0);
		bk_disable_mod_printf("FATFS_STREAM", 0);
		bk_disable_mod_printf("ONBOARD_MIC", 0);
		bk_disable_mod_printf("ONBOARD_MIC_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	char *buf = audio_calloc(1, 960);
	AUDIO_MEM_CHECK(TAG, buf, return BK_FAIL;);

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
	TEST_CHECK_NULL(record_pipeline);

	audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
	TEST_CHECK_NULL(play_pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
	onboard_mic = onboard_mic_stream_init(&onboard_mic_cfg);
	TEST_CHECK_NULL(onboard_mic);

	raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
	raw_read_cfg.type = AUDIO_STREAM_READER;
	raw_read = raw_stream_init(&raw_read_cfg);
	TEST_CHECK_NULL(raw_read);

	raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
	raw_write_cfg.type = AUDIO_STREAM_WRITER;
	raw_write = raw_stream_init(&raw_write_cfg);
	TEST_CHECK_NULL(raw_write);

	onboard_speaker_stream_cfg_t onboard_speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
	onboard_speaker = onboard_speaker_stream_init(&onboard_speaker_cfg);
	TEST_CHECK_NULL(onboard_speaker);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(record_pipeline, onboard_mic, "onboard_mic")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(record_pipeline, raw_read, "raw_read")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_register(play_pipeline, raw_write, "raw_write")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(play_pipeline, onboard_speaker, "onboard_speaker")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]) {"onboard_mic", "raw_read"}, 2)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_link(play_pipeline, (const char *[]) {"raw_write", "onboard_speaker"}, 2)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t record_evt = audio_event_iface_init(&evt_cfg);
	if (BK_OK != audio_pipeline_set_listener(record_pipeline, record_evt)) {
		BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	audio_event_iface_handle_t play_evt = audio_event_iface_init(&evt_cfg);
	if (BK_OK != audio_pipeline_set_listener(play_pipeline, play_evt)) {
		BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
	audio_event_iface_set_listener(record_evt, evt);
	audio_event_iface_set_listener(play_evt, evt);

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
	if (BK_OK != audio_pipeline_run(record_pipeline)) {
		BK_LOGE(TAG, "record_pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_run(play_pipeline)) {
		BK_LOGE(TAG, "play_pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	while (1) {
#if 1
		audio_event_iface_msg_t msg;
		bk_err_t ret = audio_event_iface_listen(evt, &msg, 0);//portMAX_DELAY
		if (ret == BK_OK) {
			if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
				&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
				&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
				BK_LOGW(TAG, "[ * ] Stop event received \n");
				break;
			}
		}
#endif

		int size = raw_stream_read(raw_read, buf, 960);
		if (size > 0) {
			//BK_LOGI(TAG, "raw_stream_read size: %d \n", size);
			size = raw_stream_write(raw_write, buf, size);
			if (size <= 0) {
				BK_LOGE(TAG, "raw_stream_write size: %d \n", size);
				break;
			} else {
				//BK_LOGI(TAG, "raw_stream_write size: %d \n", size);
				read_count++;
				if (read_count == TEST_NUM)
					//read_count = 0;
					break;
			}
		} else {
			BK_LOGE(TAG, "raw_stream_read size: %d \n", size);
		}

#if 0
		int size = raw_stream_read(raw_read, buf, 960);
		if (size > 0) {
			size = raw_stream_write(raw_write, buf, size);
			if (size <= 0) {
				BK_LOGE(TAG, "raw_stream_write size: %d \n", size);
				break;
			} else {
				read_count++;
				if (read_count == TEST_NUM)
					break;
			}
		} else {
			BK_LOGE(TAG, "raw_stream_read size: %d \n", size);
			break;
		}
#endif
	}

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(play_pipeline)) {
		BK_LOGE(TAG, "play_pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline)) {
		BK_LOGE(TAG, "play_pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_stop(record_pipeline)) {
		BK_LOGE(TAG, "record_pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline)) {
		BK_LOGE(TAG, "record_pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_terminate(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_terminate(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_unregister(record_pipeline, onboard_mic)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(record_pipeline, raw_read)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(play_pipeline, onboard_speaker)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(play_pipeline, raw_write)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_remove_listener(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_remove_listener(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_event_iface_destroy(record_evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_event_iface_destroy(play_evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_deinit(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_deinit(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(onboard_mic)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(raw_read)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(onboard_speaker)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(raw_write)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- raw test complete ----------\n");
	AUDIO_MEM_SHOW("end \n");
	read_count = 0;

	return BK_OK;
}

