// Copyright 2025-2026 Beken
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
#include <components/bk_audio/audio_algorithms/agc_algorithm.h>
#include <os/os.h>


#define TAG  "AGC_ALGORITHM_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

#define INPUT_VAL  0x11
#define OUTPUT_VAL  0x2f3f

#define INPUT_RINGBUF_SIZE  (1024 * 2)
#define OUTPUT_RINGBUF_SIZE  (1024 * 2)
#define TEST_NUM 20
static int read_count = 0;

static int16_t *agc_input_temp_data = NULL;
static uint8_t *agc_output_temp_data = NULL;
static bool test_result = true;
static bool chek_flag = false;
static bool check_complete = false;

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_open \n", audio_element_get_tag(self));
    return BK_OK;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_close \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _el_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGV(TAG, "[%s] _el_read, len: %d \n", audio_element_get_tag(self), len);

    //the data in output_rb need to be read
    if (agc_input_temp_data)
    {
        os_memcpy(buffer, agc_input_temp_data, len);
    }

    if (read_count++ == TEST_NUM)
    {
        return 0;
    }

    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _el_process, in_len: %d \n", audio_element_get_tag(self), in_len);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}

static int _el_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGV(TAG, "[%s] _el_write, len: %d \n", audio_element_get_tag(self), len);

    if (agc_output_temp_data)
    {
        if (len > OUTPUT_RINGBUF_SIZE)
        {
            os_memcpy(agc_output_temp_data, buffer, OUTPUT_RINGBUF_SIZE);
        }
        else
        {
            os_memcpy(agc_output_temp_data, buffer, len);
        }
    }

    /* check the audio data in output buffer */
    int16_t *ptr = (int16_t *)agc_output_temp_data;
    if (chek_flag && !check_complete)
    {
        for (uint32_t i = 0; i < len / 2; i++)
        {
            if (ptr[i] != OUTPUT_VAL)
            {
                test_result = false;
                BK_LOGE(TAG, "check fail, ptr[i]: 0x%04x, OUTPUT_VAL: 0x%04x \n", ptr[i], OUTPUT_VAL);
                return -1;
            }
            if (i == len / 2 - 1)
            {
                /* check pass, stop check */
                check_complete = true;
            }
        }
    }
    chek_flag = true;

    return len;
}


/* The "agc-algorithm" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     ...      |               |     agc      |               |     ...      |
   |     ...      |               |  algorithm   |               |    .....     |
  ...            src - ringbuf - sink           src - ringbuf - sink           ...
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use agc algorithm to process fixed audio data in ringbuffer.

   The "agc-algorithm" element read audio data from ringbuffer, decode the data to pcm
   format and write the data to ringbuffer.
*/
bk_err_t adk_agc_algorithm_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t agc_alg, test_stream_in, test_stream_out;
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");
    agc_input_temp_data = os_malloc(INPUT_RINGBUF_SIZE);
    os_memset(agc_input_temp_data, INPUT_VAL, INPUT_RINGBUF_SIZE);
    agc_output_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
    os_memset(agc_output_temp_data, 0xFF, OUTPUT_RINGBUF_SIZE);

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
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

    agc_algorithm_cfg_t agc_alg_cfg = DEFAULT_AGC_ALGORITHM_CONFIG();
    agc_alg = agc_algorithm_init(&agc_alg_cfg);
    TEST_CHECK_NULL(agc_alg);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, test_stream_in, "stream_in"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, agc_alg, "agc_alg"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, test_stream_out, "stream_out"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"stream_in", "agc_alg", "stream_out"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    TEST_CHECK_NULL(evt);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    while (1)
    {
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    BK_LOGD(TAG, "--------- step7: deinit pipeline ----------\n");
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

    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, test_stream_in))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, agc_alg))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, test_stream_out))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(test_stream_in))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(agc_alg))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(test_stream_out))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    os_free(agc_output_temp_data);
    agc_output_temp_data = NULL;
    os_free(agc_input_temp_data);
    agc_input_temp_data = NULL;

    BK_LOGD(TAG, "--------- agc algorithm test complete: %s ----------\n", test_result ? "PASS" : "FAIL");

    if (test_result == false)
    {
        return BK_FAIL;
    }
    AUDIO_MEM_SHOW("end \n");
    test_result = true;
    read_count = 0;
    chek_flag = false;
    check_complete = false;

    return BK_OK;
}

