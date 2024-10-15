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
#include "g711_encoder.h"
#include <os/os.h>


#define TAG  "G711_ENCODER_TEST"


#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

#define INPUT_VAL  0x11
#define OUTPUT_VAL  0x84

#define INPUT_RINGBUF_SIZE  (1024 * 8)
#define OUTPUT_RINGBUF_SIZE  (1024 * 8)
#define TEST_NUM 10

static int16_t *input_rb_temp_data = NULL;
static uint8_t *output_rb_temp_data = NULL;
static bool test_result = true;

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
    BK_LOGD(TAG, "[%s] _el_read \n", audio_element_get_tag(self));
	static int read_count = 0;

	//the data in output_rb need to be read
	if (input_rb_temp_data) {
		os_memcpy(buffer, input_rb_temp_data, len);
	}

	if (read_count++ == TEST_NUM) {
		return 0;
	}

    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
//	vTaskDelay(100/portTICK_RATE_MS);
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

static int _el_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGD(TAG, "[%s] _el_write \n", audio_element_get_tag(self));

	if (output_rb_temp_data) {
		os_memcpy(output_rb_temp_data, buffer, len);
	}

	/* check the audio data in output buffer */
//	char *ptr = output_rb_temp_data;
	for (uint32_t i = 0; i < len; i++) {
		if (output_rb_temp_data[i] != OUTPUT_VAL) {
			test_result = false;
			BK_LOGE(TAG, "check fail, output_rb_temp_data[i]: 0x%02x, OUTPUT_VAL: 0x%02x \n", output_rb_temp_data[i], OUTPUT_VAL);
			return -1;
		}
	}

    return len;
}


/* The "g711-encoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     ...      |               |     g711     |               |     ...      |
   |     ...      |               |   encoder    |               |    .....     |
  ...            src - ringbuf - sink           src - ringbuf - sink           ...
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use g711 encoder to encode fixed audio data in ringbuffer.

   The "g711-encoder" element read audio data from ringbuffer, encode the data to g711
   format and write the data to ringbuffer.
*/
bk_err_t asdf_g711_encoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t g711_enc, test_stream_in, test_stream_out;
	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
#if 1
		bk_set_printf_sync(true);
//		extern void bk_enable_white_list(int enabled);
//		bk_enable_white_list(1);
//		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
//		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
//		bk_disable_mod_printf("AUDIO_EVENT", 0);
//		bk_disable_mod_printf("AUDIO_MEM", 0);
//		bk_disable_mod_printf("FATFS_STREAM", 0);
//		bk_disable_mod_printf("FATFS_STREAM_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");
	input_rb_temp_data = os_malloc(INPUT_RINGBUF_SIZE);
	os_memset(input_rb_temp_data, INPUT_VAL, INPUT_RINGBUF_SIZE);
	output_rb_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
	os_memset(output_rb_temp_data, 0, OUTPUT_RINGBUF_SIZE);

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

    cfg.process = _el_process;
	cfg.read = NULL;
	cfg.write = _el_write;
    test_stream_out = audio_element_init(&cfg);
    TEST_CHECK_NULL(test_stream_out);

    g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
    g711_enc = g711_encoder_init(&g711_encoder_cfg);
    TEST_CHECK_NULL(g711_enc);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, test_stream_in, "stream_in")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
    if (BK_OK != audio_pipeline_register(pipeline, g711_enc, "g711_enc")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
    if (BK_OK != audio_pipeline_register(pipeline, test_stream_out, "stream_out")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"stream_in", "g711_enc", "stream_out"}, 3)) {
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
    if (BK_OK != audio_pipeline_unregister(pipeline, g711_enc)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
    if (BK_OK != audio_pipeline_unregister(pipeline, test_stream_out)) {
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

    if (BK_OK != audio_element_deinit(g711_enc)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_element_deinit(test_stream_out)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	os_free(output_rb_temp_data);
	output_rb_temp_data = NULL;
	os_free(input_rb_temp_data);
	input_rb_temp_data = NULL;

	BK_LOGI(TAG, "--------- g711 encoder test complete: %s ----------\n", test_result? "PASS": "FAIL");

	if (test_result == false) {
		return BK_FAIL;
	}
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

