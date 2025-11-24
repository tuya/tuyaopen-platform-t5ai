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
#include <components/bk_audio/audio_streams/frame_array_stream.h>
#include "frame_array_stream_input_data.c"

#define TAG  "FRAME_ARRAY_STR_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

static int array_size_comparison(const char *array1, const char *array2, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        //BK_LOGD(TAG, "array1[%d]: %02x, array2[%d]: %02x \n", i, array1[i], i, array2[i]);
        if (array1[i] != array2[i])
        {
            BK_LOGD(TAG, "The two arrays are not the same at index %d \n", i);
            BK_LOGD(TAG, "array1[%d]: %02x, array2[%d]: %02x \n", i, array1[i], i, array2[i]);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "============ The two arrays are same ================\n");

    return BK_OK;
}

/* The "frame_array-stream[IN]" element is producer that has only one src and no sink
   and this element is the first element of the pipeline. The "frame_array-stream[OUT]"
   element is consumer that has only one sink and no src and this element is the
   last element of the pipeline.
   The data flow model of this element is as follow:
   +--------------+               +--------------+
   | frame array  |               | frame array  |
   |  stream[IN]  |               |  stream[out] |
   |             src - framebuf - sink           |
   |              |               |              |
   +--------------+               +--------------+

   The "frame_array-stream[IN]" element read audio data through callback api from array
   and write the data to framebuffer. The "frame_array-stream[OUT]" element read audio data
   from framebuffer and write the data to array through callback api.
*/
bk_err_t adk_frame_array_stream_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t frame_array_stream_reader, frame_array_stream_writer;
    uint32_t total_data_size = 0;

    for(int i = 0; i < sizeof(frame_array_data_len)/sizeof(uint16_t); i++)
    {
        total_data_size += frame_array_data_len[i];
    }

    uint8_t *frame_array_out = (uint8_t *)audio_malloc(total_data_size);
    TEST_CHECK_NULL(frame_array_out);

    BK_LOGD(TAG, "array_out: %p, TEST_ARRAY_SIZE: %d\n", frame_array_out, total_data_size);

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    frame_array_stream_cfg_t frame_array_reader_cfg = DEFAULT_FRAME_ARRAY_STREAM_CONFIG();
    frame_array_reader_cfg.type = AUDIO_STREAM_READER;
    frame_array_stream_reader = frame_array_stream_init(&frame_array_reader_cfg);
    TEST_CHECK_NULL(frame_array_stream_reader);

    frame_array_stream_cfg_t frame_array_writer_cfg = DEFAULT_FRAME_ARRAY_STREAM_CONFIG();
    frame_array_writer_cfg.type = AUDIO_STREAM_WRITER;
    frame_array_stream_writer = frame_array_stream_init(&frame_array_writer_cfg);
    TEST_CHECK_NULL(frame_array_stream_writer);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, frame_array_stream_reader, "frame_array_reader"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_register(pipeline, frame_array_stream_writer, "frame_array_writer"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"frame_array_reader", "frame_array_writer"}, 2))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: set element data ----------\n");
    if (BK_OK != frame_array_stream_set_data(frame_array_stream_reader, (uint8_t *)frame_array_data, total_data_size,(uint16_t *)frame_array_data_len,sizeof(frame_array_data_len)/sizeof(uint16)))
    {
        BK_LOGE(TAG, "set data fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != frame_array_stream_set_data(frame_array_stream_writer, frame_array_out, total_data_size,(uint16_t *)frame_array_data_len,sizeof(frame_array_data_len)/sizeof(uint16)))
    {
        BK_LOGE(TAG, "set data fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: pipeline run ----------\n");
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

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) frame_array_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received, event: %d \n", (int)msg.data);
            break;
        }
    }

    BK_LOGD(TAG, "--------- step8: stop pipeline ----------\n");
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

    BK_LOGD(TAG, "--------- step9: check test result ----------\n");
    array_size_comparison((const char *)frame_array_data, (const char *)frame_array_out, total_data_size);

    BK_LOGD(TAG, "--------- step10: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_unregister(pipeline, frame_array_stream_reader))
    {
        BK_LOGE(TAG, "pipeline unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, frame_array_stream_writer))
    {
        BK_LOGE(TAG, "pipeline unregister fail, %d \n", __LINE__);
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

    if (BK_OK != audio_element_deinit(frame_array_stream_reader))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(frame_array_stream_writer))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- frame array stream test complete ----------\n");

    return BK_OK;
}
