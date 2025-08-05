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
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <os/os.h>


#define TAG   "AUD_PIPE_TEST"


#define OUTPUT_RINGBUF_SIZE     (1024 * 4)

#define OUTPUT_FRAMEBUF_SIZE    (1024)
#define OUTPUT_FRAMEBUF_NUM     (4)

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_open \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _el_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGD(TAG, "[%s] _el_read \n", audio_element_get_tag(el));
    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGD(TAG, "[%s] _el_process, in_len=%d \n", audio_element_get_tag(self), in_len);
    rtos_delay_milliseconds(300);
    return in_len;
}

static int _el_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGD(TAG, "[%s] _el_write \n", audio_element_get_tag(el));
    return len;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_close \n", audio_element_get_tag(self));
    return BK_OK;
}

/* 
   as follow:
   +---------+               +---------+               +---------+
   |  first  |               |   mid   |               |   last  |
  sink      src - ringbuf - sink      src - ringbuf - sink      src
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   All elements are linked through ringbuffer.
*/
bk_err_t adk_pipeline_test_case_0(void)
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

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    bk_err_t ret = BK_OK;
    audio_element_handle_t first_el, mid_el, last_el;
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

    BK_LOGD(TAG, "--------- step1: init elements ----------\n");
    el_cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.open = _el_open;
    el_cfg.in_type = PORT_TYPE_CB;
    el_cfg.read = _el_read;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_RB;
    el_cfg.write = NULL;
    el_cfg.close = _el_close;
    el_cfg.out_block_size = OUTPUT_RINGBUF_SIZE;
    el_cfg.out_block_num = 1;
    first_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(first_el);

    el_cfg.in_type = PORT_TYPE_RB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_RB;
    el_cfg.write = NULL;
    mid_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(mid_el);

    el_cfg.in_type = PORT_TYPE_RB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_CB;
    el_cfg.write = _el_write;
    last_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(last_el);

    BK_LOGD(TAG, "--------- step2: pipeline register ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    if (BK_OK != audio_pipeline_register(pipeline, first_el, "first"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, mid_el, "mid"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, last_el, "last"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"first", "mid", "last"
}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_pause(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_resume(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_resume fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(8000);

    BK_LOGD(TAG, "--------- step5: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    ret = audio_pipeline_wait_for_stop(pipeline);
    if (BK_OK != ret)
    {
        BK_LOGE(TAG, "pipeline wait stop fail, ret=%d, %d \n", ret, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline unlink ----------\n");
    if (BK_OK != audio_pipeline_unlink(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    BK_LOGD(TAG, "--------- step8: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step9: pipeline deinit ----------\n");
    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- audio pipeline test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* 
   as follow:
   +---------+                +---------+                +---------+
   |  first  |                |   mid   |                |   last  |
  sink      src - framebuf - sink      src - framebuf - sink      src
   |         |                |         |                |         |
   +---------+                +---------+                +---------+

   All elements are linked through framebuffer.
*/
bk_err_t adk_pipeline_test_case_1(void)
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

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    bk_err_t ret = BK_OK;
    audio_element_handle_t first_el, mid_el, last_el;
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

    BK_LOGD(TAG, "--------- step1: init elements ----------\n");
    el_cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.open = _el_open;
    el_cfg.in_type = PORT_TYPE_CB;
    el_cfg.read = _el_read;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_FB;
    el_cfg.write = NULL;
    el_cfg.close = _el_close;
    el_cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    first_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(first_el);

    el_cfg.in_type = PORT_TYPE_FB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_FB;
    el_cfg.write = NULL;
    mid_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(mid_el);

    el_cfg.in_type = PORT_TYPE_FB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_CB;
    el_cfg.write = _el_write;
    last_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(last_el);

    BK_LOGD(TAG, "--------- step2: pipeline register ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    if (BK_OK != audio_pipeline_register(pipeline, first_el, "first"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, mid_el, "mid"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, last_el, "last"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"first", "mid", "last"
}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_pause(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_resume(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_resume fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(8000);

    BK_LOGD(TAG, "--------- step5: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    ret = audio_pipeline_wait_for_stop(pipeline);
    if (BK_OK != ret)
    {
        BK_LOGE(TAG, "pipeline wait stop fail, ret=%d, %d \n", ret, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline unlink ----------\n");
    if (BK_OK != audio_pipeline_unlink(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    BK_LOGD(TAG, "--------- step8: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step9: pipeline deinit ----------\n");
    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- audio pipeline test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* 
   as follow:
   +---------+                +---------+               +---------+
   |  first  |                |   mid   |               |   last  |
  sink      src - framebuf - sink      src - ringbuf - sink      src
   |         |                |         |               |         |
   +---------+                +---------+               +---------+

   All elements are linked through framebuffer and ringbuffer.
*/
bk_err_t adk_pipeline_test_case_2(void)
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

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    bk_err_t ret = BK_OK;
    audio_element_handle_t first_el, mid_el, last_el;
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

    BK_LOGD(TAG, "--------- step1: init elements ----------\n");
    el_cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.open = _el_open;
    el_cfg.in_type = PORT_TYPE_CB;
    el_cfg.read = _el_read;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_FB;
    el_cfg.write = NULL;
    el_cfg.close = _el_close;
    el_cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    first_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(first_el);

    el_cfg.in_type = PORT_TYPE_FB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_RB;
    el_cfg.write = NULL;
    el_cfg.out_block_size = OUTPUT_RINGBUF_SIZE;
    el_cfg.out_block_num = 1;
    mid_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(mid_el);

    el_cfg.in_type = PORT_TYPE_RB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_CB;
    el_cfg.write = _el_write;
    last_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(last_el);

    BK_LOGD(TAG, "--------- step2: pipeline register ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    if (BK_OK != audio_pipeline_register(pipeline, first_el, "first"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, mid_el, "mid"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, last_el, "last"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"first", "mid", "last"
}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_pause(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_resume(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_resume fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(8000);

    BK_LOGD(TAG, "--------- step5: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    ret = audio_pipeline_wait_for_stop(pipeline);
    if (BK_OK != ret)
    {
        BK_LOGE(TAG, "pipeline wait stop fail, ret=%d, %d \n", ret, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline unlink ----------\n");
    if (BK_OK != audio_pipeline_unlink(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    BK_LOGD(TAG, "--------- step8: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step9: pipeline deinit ----------\n");
    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- audio pipeline test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* 
   as follow:
   +---------+               +---------+                +---------+
   |  first  |               |   mid   |                |   last  |
  sink      src - ringbuf - sink      src - framebuf - sink      src
   |         |               |         |                |         |
   +---------+               +---------+                +---------+

   All elements are linked through framebuffer and ringbuffer.
*/
bk_err_t adk_pipeline_test_case_3(void)
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

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    bk_err_t ret = BK_OK;
    audio_element_handle_t first_el, mid_el, last_el;
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

    BK_LOGD(TAG, "--------- step1: init elements ----------\n");
    el_cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.open = _el_open;
    el_cfg.in_type = PORT_TYPE_CB;
    el_cfg.read = _el_read;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_RB;
    el_cfg.write = NULL;
    el_cfg.close = _el_close;
    el_cfg.out_block_size = OUTPUT_RINGBUF_SIZE;
    el_cfg.out_block_num = 1;
    first_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(first_el);

    el_cfg.in_type = PORT_TYPE_RB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_FB;
    el_cfg.write = NULL;
    el_cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    el_cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    mid_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(mid_el);

    el_cfg.in_type = PORT_TYPE_FB;
    el_cfg.read = NULL;
    el_cfg.process = _el_process;
    el_cfg.out_type = PORT_TYPE_CB;
    el_cfg.write = _el_write;
    last_el = audio_element_init(&el_cfg);
    TEST_CHECK_NULL(last_el);

    BK_LOGD(TAG, "--------- step2: pipeline register ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    if (BK_OK != audio_pipeline_register(pipeline, first_el, "first"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, mid_el, "mid"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, last_el, "last"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"first", "mid", "last"
}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_pause(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(5000);

    if (BK_OK != audio_pipeline_resume(pipeline))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_resume fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(8000);

    BK_LOGD(TAG, "--------- step5: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    ret = audio_pipeline_wait_for_stop(pipeline);
    if (BK_OK != ret)
    {
        BK_LOGE(TAG, "pipeline wait stop fail, ret=%d, %d \n", ret, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline unlink ----------\n");
    if (BK_OK != audio_pipeline_unlink(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline unlink fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(5000);

    BK_LOGD(TAG, "--------- step8: pipeline stop ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step9: pipeline deinit ----------\n");
    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- audio pipeline test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}


