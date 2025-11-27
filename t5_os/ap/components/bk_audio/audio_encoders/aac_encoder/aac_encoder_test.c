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
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <os/os.h>


#define TAG  "AAC_ENCODER_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


#define TEST_NUM 30
static int time_count = 0;

/* The "aac-encoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |  onboard-mic |               |     aac      |               |     uart     |
   |  stream[in]  |               |   encoder    |               |  stream[out] |
   |              |               |              |               |              |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use onboard mic stream to record 30s audio mic data. And then use aac encoder
   to encode mic data, output aac data by uart.

   The "aac-encoder" element read audio mic data from ringbuffer, encode the data to aac
   format and write the data to ringbuffer.
*/
bk_err_t adk_aac_encoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t onboard_mic, aac_enc, uart_out;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    onboard_mic = onboard_mic_stream_init(&onboard_mic_cfg);
    TEST_CHECK_NULL(onboard_mic);

    aac_encoder_cfg_t aac_encoder_cfg = DEFAULT_AAC_ENCODER_CONFIG();
    aac_enc = aac_encoder_init(&aac_encoder_cfg);
    TEST_CHECK_NULL(aac_enc);

    uart_stream_cfg_t uart_stream_cfg = UART_STREAM_CFG_DEFAULT();
    uart_stream_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream_cfg.buffer_len = 2048;
    uart_out = uart_stream_init(&uart_stream_cfg);
    TEST_CHECK_NULL(uart_out);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, onboard_mic, "onboard_mic"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, aac_enc, "aac_enc"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uart_out, "uart_out"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"onboard_mic", "aac_enc", "uart_out"}, 3))
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
        bk_err_t ret = audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS);//portMAX_DELAY
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            time_count++;
            if (time_count == TEST_NUM)
            {
                break;
            }
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
    if (BK_OK != audio_pipeline_unregister(pipeline, onboard_mic))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_enc))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_out))
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


    if (BK_OK != audio_element_deinit(onboard_mic))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(aac_enc))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_out))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- aac encoder test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

