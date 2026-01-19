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
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>
#include <os/os.h>
#include <components/bk_audio/audio_streams/raw_stream.h>


#define TAG  "UAC_SPK_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

#define TEST_NUM 300
static int write_count = 0;

/* 8K mono, 960 bytes */
static const uint16_t in_data[] =
{
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,

    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,

    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
    0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
};


/* The "uac speaker stream" element is a client. The element is the last element.
   Usually this element only has sink.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+
   |       raw       |               |      uac-spk      |
   |  stream[write]  |               |    stream[out]    |
   |                 |               |                   |
   |                src - ringbuf - sink                 |
   |                 |               |                   |
   |                 |               |                   |
   +-----------------+               +-------------------+

   Function: Use uac speaker stream to play sin audio data.

   The "uac speaker stream" element read audio data from ringbuffer and play the data.
*/
bk_err_t adk_uac_spk_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t uac_spk, raw_write;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    raw_write = raw_stream_init(&raw_write_cfg);
    TEST_CHECK_NULL(raw_write);

    uac_speaker_stream_cfg_t uac_spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
    uac_spk = uac_speaker_stream_init(&uac_spk_cfg);
    TEST_CHECK_NULL(uac_spk);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, raw_write, "raw_write"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uac_spk, "uac_spk"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"raw_write", "uac_spk"}, 2))
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
        bk_err_t ret = audio_event_iface_listen(evt, &msg, 0);//portMAX_DELAY
        if (ret != BK_OK)
        {
            //BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
        }
        else
        {
            if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
            {
                BK_LOGW(TAG, "[ * ] Stop event received \n");
                break;
            }
        }

        /* write sin data to raw stream */
        int size = raw_stream_write(raw_write, (char *)in_data, 320);
        if (size <= 0)
        {
            BK_LOGE(TAG, "raw_stream_write size: %d \n", size);
            break;
        }
        else
        {
            //BK_LOGD(TAG, "raw_stream_write size: %d \n", size);
            write_count++;
            if (write_count == TEST_NUM)
                //read_count = 0;
            {
                break;
            }
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

    if (BK_OK != audio_pipeline_unregister(pipeline, raw_write))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uac_spk))
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

    if (BK_OK != audio_element_deinit(raw_write))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uac_spk))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- uac speaker test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");
    write_count = 0;

    return BK_OK;
}

