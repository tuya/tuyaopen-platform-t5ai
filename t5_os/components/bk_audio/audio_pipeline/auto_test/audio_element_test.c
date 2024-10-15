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
#include "audio_element.h"
#include "audio_mem.h"
#include <os/os.h>


#define TAG   "AUD_ELE_TEST"


#define INPUT_RINGBUF_SIZE  (1024 * 4)
#define OUTPUT_RINGBUF_SIZE  (1024 * 4)

//
static char *input_rb_temp_data = NULL;
static char *output_rb_temp_data = NULL;
static ringbuf_handle_t input_rb = NULL;
static ringbuf_handle_t output_rb = NULL;

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGI(TAG, "_el_open \n");
    return BK_OK;
}

static int _el_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGI(TAG, "_el_read, len: %d \n", len);
/*
	//the data in output_rb need to be read
	if (output_rb_temp_data) {
		rb_read(output_rb, output_rb_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
		os_memcpy(buffer, output_rb_temp_data, INPUT_RINGBUF_SIZE);
	}
*/
    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	rtos_delay_milliseconds(100);
    BK_LOGI(TAG, "_el_process, in_len: %d \n", in_len);

    int r_size = audio_element_input(self, in_buffer, in_len);

	//write data to input_rb after read to fill input_rb
	if (input_rb_temp_data) {
		rb_write(input_rb, in_buffer, in_len, portMAX_DELAY);
	}

    int w_size = 0;
    if (r_size > 0) {
        w_size = audio_element_output(self, in_buffer, r_size);
    } else {
        w_size = r_size;
    }

	//read data from output_rb after write to clear output_rb
	if (output_rb_temp_data) {
		rb_read(output_rb, output_rb_temp_data, in_len, portMAX_DELAY);
	}

    return w_size;
}

static int _el_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGI(TAG, "_el_write, len: %d \n", len);
/*
	if (input_rb_temp_data) {
		rb_write(input_rb, input_rb_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
	}
*/
    return len;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGI(TAG, "_el_close \n");
    return BK_OK;
}

/* The test element is both a consumer and producer when test element is both the last
   element and the first element of the pipeline. Usually this element has on sink and
   no src. The data flow model of this element is as follow: 
   +---------+
   | element |
   |         |
   |         |
   +---------+

   This test element read audio data through callback api and process the data.
*/
bk_err_t asdf_element_test_case_0(void)
{
	bk_set_printf_sync(true);
#if 0
	extern void bk_enable_white_list(int enabled);
	bk_enable_white_list(1);
	bk_disable_mod_printf("AUD_ELE", 0);
	bk_disable_mod_printf("AUD_MEM", 0);
	bk_disable_mod_printf("AUD_ELE_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	BK_LOGI(TAG, "--------- step1: element init ----------\n");
    audio_element_handle_t el;
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);

	BK_LOGI(TAG, "--------- step2: element run ----------\n");
    if (BK_OK != audio_element_run(el)) {
		BK_LOGE(TAG, "audio_element_run fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step4: element pause ----------\n");
    if (BK_OK != audio_element_pause(el)) {
		BK_LOGE(TAG, "audio_element_pause fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step5: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step6: element stop ----------\n");
    if (BK_OK != audio_element_stop(el)) {
		BK_LOGE(TAG, "audio_element_stop fail \n");
		return BK_FAIL;
	}
    if (BK_OK != audio_element_wait_for_stop(el)) {
		BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step7: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el)) {
		BK_LOGE(TAG, "audio_element_deinit fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- element test [0] complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

/* The test element is a consumer when test element is the last element of the pipeline.
   Usually this element has only sink and no src. The data flow model of this element is
   as follow: 
   +---------+               +---------+
   |   ...   |               | element |
  ...       src - ringbuf - sink       |
   |         |               |         |
   +---------+               +---------+

   The previous element of pipeline write audio data to ringbuffer. This test element
   read audio data form ringbuffer and process the data.
*/
bk_err_t asdf_element_test_case_1(void)
{
    audio_element_handle_t el;

	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	BK_LOGI(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);

	BK_LOGI(TAG, "--------- step2: input ringbuf init ----------\n");
    input_rb = rb_create(1024, INPUT_RINGBUF_SIZE/1024);
    audio_element_set_input_ringbuf(el, input_rb);

	input_rb_temp_data = audio_malloc(INPUT_RINGBUF_SIZE);
	os_memset(input_rb_temp_data, 0, INPUT_RINGBUF_SIZE);

	//write some data to ringbuf
	rb_write(input_rb, input_rb_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
	//check data in ringbuf
	if (rb_bytes_filled(input_rb) != INPUT_RINGBUF_SIZE) {
		BK_LOGE(TAG, "ringbuf check fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el)) {
		BK_LOGE(TAG, "audio_element_run fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el)) {
		BK_LOGE(TAG, "audio_element_pause fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el)) {
		BK_LOGE(TAG, "audio_element_stop fail \n");
		return BK_FAIL;
	}
    if (BK_OK != audio_element_wait_for_stop(el)) {
		BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el)) {
		BK_LOGE(TAG, "audio_element_deinit fail \n");
		return BK_FAIL;
	}
    rb_destroy(input_rb);
	os_free(input_rb_temp_data);
	input_rb_temp_data =NULL;

	BK_LOGI(TAG, "--------- element test [1] complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

/* The test element is a producer when test element is the first element of the pipeline.
   Usually this element has only src and no sink. The data flow model of this element is
   as follow: 
   +---------+               +---------+
   | element |               |   ...   |
   |        src - ringbuf - sink      ...
   |         |               |         |
   +---------+               +---------+

   This test element produce audio data and write data to ringbuffer. The next element of
   pipeline read data from ringbuffer and process the data.
*/
bk_err_t asdf_element_test_case_2(void)
{
    audio_element_handle_t el;

	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	BK_LOGI(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.read = _el_read;
    cfg.process = _el_process;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);

	BK_LOGI(TAG, "--------- step2: output ringbuf init ----------\n");
    output_rb = rb_create(1024, OUTPUT_RINGBUF_SIZE/1024);
    audio_element_set_output_ringbuf(el, output_rb);

	output_rb_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
	os_memset(output_rb_temp_data, 0, OUTPUT_RINGBUF_SIZE);

	BK_LOGI(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el)) {
		BK_LOGE(TAG, "audio_element_run fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el)) {
		BK_LOGE(TAG, "audio_element_pause fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el)) {
		BK_LOGE(TAG, "audio_element_stop fail \n");
		return BK_FAIL;
	}
    if (BK_OK != audio_element_wait_for_stop(el)) {
		BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el)) {
		BK_LOGE(TAG, "audio_element_deinit fail \n");
		return BK_FAIL;
	}
    rb_destroy(output_rb);
	os_free(output_rb_temp_data);
	output_rb_temp_data = NULL;

	BK_LOGI(TAG, "--------- element test [2] complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

/* The test element is neither a producer nor a consumer when test element is neither
   first element nor last element of the pipeline. Usually this element has both src
   and sink. The data flow model of this element is
   as follow: 
   +---------+               +---------+               +---------+
   |   ...   |               | element |               |   ...   |
  ...       src - ringbuf - sink      src - ringbuf - sink      ...
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   The previous element of pipeline write audio data to ringbuffer. This test element
   read the data from ringbuffer, process the data and write output data to ringbuffer.
   The next element of pipeline read data from ringbuffer and process the data.
*/
bk_err_t asdf_element_test_case_3(void)
{
    audio_element_handle_t el;

	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	BK_LOGI(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);

	BK_LOGI(TAG, "--------- step2: input&output ringbuf init ----------\n");
    output_rb = rb_create(1024, OUTPUT_RINGBUF_SIZE/1024);
    input_rb = rb_create(1024, INPUT_RINGBUF_SIZE/1024);
    audio_element_set_input_ringbuf(el, input_rb);
    audio_element_set_output_ringbuf(el, output_rb);

	input_rb_temp_data = os_malloc(INPUT_RINGBUF_SIZE);
	os_memset(input_rb_temp_data, 0, INPUT_RINGBUF_SIZE);
	output_rb_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
	os_memset(output_rb_temp_data, 0, OUTPUT_RINGBUF_SIZE);

	//write some data to ringbuf
	rb_write(input_rb, input_rb_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
	//check data in ringbuf
	if (rb_bytes_filled(input_rb) != INPUT_RINGBUF_SIZE) {
		BK_LOGE(TAG, "ringbuf check fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el)) {
		BK_LOGE(TAG, "audio_element_run fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el)) {
		BK_LOGE(TAG, "audio_element_pause fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

    rtos_delay_milliseconds(2000);

	BK_LOGI(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el)) {
		BK_LOGE(TAG, "audio_element_stop fail \n");
		return BK_FAIL;
	}
    if (BK_OK != audio_element_wait_for_stop(el)) {
		BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el)) {
		BK_LOGE(TAG, "audio_element_deinit fail \n");
		return BK_FAIL;
	}
    rb_destroy(output_rb);
	os_free(output_rb_temp_data);
	output_rb_temp_data = NULL;
    rb_destroy(input_rb);
	os_free(input_rb_temp_data);
	input_rb_temp_data = NULL;

	BK_LOGI(TAG, "--------- element test [3] complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

