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


#include "FreeRTOS.h"
#include "task.h"
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "onboard_speaker_stream.h"
#include <os/os.h>


#define TAG  "ONBOARD_SPEAKER_TEST"


#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

static const unsigned long PCM_8000[] = {
	0x00010000, 0x5A825A81, 0x7FFF7FFF, 0x5A825A83, 0x00000000, 0xA57FA57E, 0x80018002, 0xA57EA57E,
};

#define TEST_NUM 200
static int read_count = 0;

static bk_err_t _el_open(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _el_open \n", audio_element_get_tag(self));
	return BK_OK;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _el_close \n", audio_element_get_tag(self));
	return BK_OK;
}

static int _el_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
	BK_LOGD(TAG, "[%s] _el_read, len: %d \n", audio_element_get_tag(self), len);

	//the data in output_rb need to be read
	for (uint8_t i = 0; i < (len/32); i++) {
		os_memcpy(&buffer[i*32], PCM_8000, 32);
	}

	if (read_count++ == TEST_NUM) {
		return 0;
	}

	return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	BK_LOGD(TAG, "[%s] _el_process, in_len: %d \n", audio_element_get_tag(self), in_len);

	int r_size = audio_element_input(self, in_buffer, in_len);

	int w_size = 0;
	if (r_size > 0) {
		w_size = audio_element_output(self, in_buffer, r_size);
	} else {
		w_size = r_size;
	}

	return w_size;
}


/* The "onboard speaker stream" element is a consumer. The element is the last element.
   Usually this element only has sink.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+
   |       ...       |               |  onboard-speaker  |
   |       ...       |               |      stream       |
   |                 |               |                   |
   |                src - ringbuf - sink                 |
   |                 |               |                   |
   |                 |               |                   |
   +-----------------+               +-------------------+

   Function: Use onboard speaker stream to play sine audio data in ringbuffer.

   The "onboard speaker stream" element read audio data from ringbuffer and play the data.
*/
bk_err_t asdf_onboard_speaker_test_case_0(void)
{
	audio_pipeline_handle_t pipeline;
	audio_element_handle_t onboard_spk, test_stream_in;
	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
#if 1
		bk_set_printf_sync(true);
//		extern void bk_enable_white_list(int enabled);
//		bk_enable_white_list(1);
//		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
//		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
//		bk_disable_mod_printf("AUDIO_EVENT", 0);
//		bk_disable_mod_printf("AUDIO_MEM", 0);
//		bk_disable_mod_printf("AGC_ALGORITHM", 0);
//		bk_disable_mod_printf("AGC_ALGORITHM_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	pipeline = audio_pipeline_init(&pipeline_cfg);
	TEST_CHECK_NULL(pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	cfg.open = _el_open;
	cfg.close = _el_close;
	cfg.process = _el_process;
	cfg.read = _el_read;
	cfg.write = NULL;
	test_stream_in = audio_element_init(&cfg);
	TEST_CHECK_NULL(test_stream_in);

	onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
	onboard_spk = onboard_speaker_stream_init(&onboard_spk_cfg);
	TEST_CHECK_NULL(onboard_spk);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(pipeline, test_stream_in, "stream_in")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(pipeline, onboard_spk, "onboard_spk")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"stream_in", "onboard_spk"}, 2)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

	if (BK_OK != audio_pipeline_set_listener(pipeline, evt)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
	if (BK_OK != audio_pipeline_run(pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	while (1) {
		audio_event_iface_msg_t msg;
		bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
			continue;
		}

		if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
			&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
			&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
			BK_LOGW(TAG, "[ * ] Stop event received \n");
			break;
		}
	}

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_terminate(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(pipeline, test_stream_in)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(pipeline, onboard_spk)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_remove_listener(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_deinit(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(test_stream_in)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(onboard_spk)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- onboard speaker test complete ----------\n");

	AUDIO_MEM_SHOW("end \n");
	read_count = 0;

	return BK_OK;
}

