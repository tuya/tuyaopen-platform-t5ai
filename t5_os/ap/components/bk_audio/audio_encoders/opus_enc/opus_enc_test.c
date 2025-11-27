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
#include <components/bk_audio/audio_encoders/opus_enc.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <components/bk_audio/audio_streams/array_stream.h>
#include <os/os.h>
#include "opus_enc_test_input_data.c"

#define TAG  "OPUS_ENC_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

/* The "opus-encoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     array    |               |     opus     |               |     uart     |
   |    stream    |               |   encoder    |               |    stream    |
   |             src - ringbuf - sink           src - framebuf -sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use opus encoder to encode fixed audio data in array.

   The "opus-encoder" element read audio data from array through array stream, encode the data to opus
   format and write the data to uart through uart stream.
*/
bk_err_t adk_opus_enc_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t opus_enc_el, array_stream, uart_stream;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    array_stream_cfg_t array_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
    array_cfg.type = AUDIO_STREAM_READER;
    array_stream = array_stream_init(&array_cfg);
    TEST_CHECK_NULL(array_stream);
    array_stream_set_data(array_stream, (uint8_t *)opus_input_data, sizeof(opus_input_data));

    opus_enc_cfg_t opus_encoder_cfg = DEFAULT_OPUS_ENC_CONFIG();
    opus_enc_el = opus_enc_init(&opus_encoder_cfg);
    TEST_CHECK_NULL(opus_enc_el);

    uart_stream_cfg_t uart_cfg = DEFAULT_UART_STREAM_CONFIG();
    uart_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream = uart_stream_init(&uart_cfg);
    TEST_CHECK_NULL(uart_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, array_stream, "array_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, opus_enc_el, "opus_enc"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uart_stream, "uart_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"array_stream", "opus_enc", "uart_stream"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

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

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && msg.source == uart_stream
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    rtos_dump_stack_memory_usage();
    GLOBAL_INT_RESTORE();

    BK_LOGD(TAG, "--------- step7: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, array_stream))
    {
        BK_LOGE(TAG, "pipeline unregister array_stream element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, opus_enc_el))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_stream))
    {
        BK_LOGE(TAG, "pipeline unregister uart_stream element fail, %d \n", __LINE__);
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


    if (BK_OK != audio_element_deinit(array_stream))
    {
        BK_LOGE(TAG, "array_stream element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(opus_enc_el))
    {
        BK_LOGE(TAG, "opus_enc_el element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_stream))
    {
        BK_LOGE(TAG, "uart_stream element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- opus encoder test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}