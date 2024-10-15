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
#include <os/os.h>


#define TAG   "AUD_PIPE_TEST"

#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGI(TAG, "[%s] _el_open \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _el_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGI(TAG, "[%s] _el_read \n", audio_element_get_tag(self));
    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGI(TAG, "[%s] _el_process, in_len=%d \n", audio_element_get_tag(self), in_len);
    rtos_delay_milliseconds(300);
    return in_len;
}

static int _el_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGI(TAG, "[%s] _el_write \n", audio_element_get_tag(self));
    return len;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGI(TAG, "[%s] _el_close \n", audio_element_get_tag(self));
    return BK_OK;
}

bk_err_t asdf_pipeline_test_case_0(void)
{
#if 0
	extern void bk_enable_white_list(int enabled);
	bk_enable_white_list(1);
	bk_disable_mod_printf("AUD_PIPE", 0);
	bk_disable_mod_printf("AUD_ELE", 0);
	bk_disable_mod_printf("AUD_EVT", 0);
	bk_disable_mod_printf("AUD_MEM", 0);
	bk_disable_mod_printf("AUD_PIPE_TEST", 0);
#endif

    BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	bk_err_t ret = BK_OK;
    audio_element_handle_t first_el, mid_el, last_el;
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

	BK_LOGI(TAG, "--------- step1: init elements ----------\n");
    el_cfg.open = _el_open;
    el_cfg.read = _el_read;
    el_cfg.process = _el_process;
    el_cfg.close = _el_close;
    first_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(first_el);

    el_cfg.read = NULL;
    el_cfg.write = NULL;
    mid_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(mid_el);

    el_cfg.write = _el_write;
    last_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(last_el);

	BK_LOGI(TAG, "--------- step2: pipeline register ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    if (BK_OK != audio_pipeline_register(pipeline, first_el, "first")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_register(pipeline, mid_el, "mid")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_register(pipeline, last_el, "last")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"first", "mid", "last"}, 3)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    rtos_delay_milliseconds(8000);

	BK_LOGI(TAG, "--------- step5: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	ret = audio_pipeline_wait_for_stop(pipeline);
    if (BK_OK != ret) {
		BK_LOGE(TAG, "pipeline wait stop fail, ret=%d, %d \n", ret, __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step6: pipeline unlink ----------\n");
    if (BK_OK != audio_pipeline_unlink(pipeline)) {
		BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline)) {
		BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    rtos_delay_milliseconds(5000);

	BK_LOGI(TAG, "--------- step8: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_wait_for_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step9: pipeline deinit ----------\n");
    if (BK_OK != audio_pipeline_deinit(pipeline)) {
		BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- audio pipeline test complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}

