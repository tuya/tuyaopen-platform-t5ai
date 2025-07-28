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
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <components/bk_audio/audio_streams/fatfs_stream.h>
#include <os/os.h>
#include "ff.h"
#include "diskio.h"

#if (CONFIG_ADK_ONBOARD_MIC_STREAM && CONFIG_ADK_AAC_ENCODER && CONFIG_ADK_UART_STREAM)
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#endif

#define TAG  "AAC_DECODER_TEST"

#define TEST_FATFS_READER  "1:/mono_8K_16bit_32000bitrate.aac"
#define TEST_FATFS_WRITER  "1:/mono_8K_16bit_32000bitrate.pcm"

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


static FATFS *pfs = NULL;

static bk_err_t tf_mount(void)
{
    FRESULT fr;

    if (pfs != NULL)
    {
        os_free(pfs);
    }

    pfs = os_malloc(sizeof(FATFS));
    if (NULL == pfs)
    {
        BK_LOGD(TAG, "f_mount malloc failed!\r\n");
        return BK_FAIL;
    }

    fr = f_mount(pfs, "1:", 1);
    if (fr != FR_OK)
    {
        BK_LOGE(TAG, "f_mount failed:%d\r\n", fr);
        return BK_FAIL;
    }
    else
    {
        BK_LOGD(TAG, "f_mount OK!\r\n");
    }

    return BK_OK;
}

static bk_err_t tf_unmount(void)
{
    FRESULT fr;
    fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);
    if (fr != FR_OK)
    {
        BK_LOGE(TAG, "f_unmount failed:%d\r\n", fr);
        return BK_FAIL;
    }
    else
    {
        BK_LOGD(TAG, "f_unmount OK!\r\n");
    }

    if (pfs)
    {
        os_free(pfs);
        pfs = NULL;
    }

    return BK_OK;
}


/* The "aac-decoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |    fatfs     |               |     aac      |               |     fatfs    |
   |  stream[IN]  |               |   decoder    |               |  stream[OUT] |
  ...            src - ringbuf - sink           src - ringbuf - sink           ...
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use aac decoder to decode aac file to pcm file in tfcard.

   The "fatfs-stream[IN]" element read aac file from tfcard to ringbuffer. The
   "aac-decoder" element read audio data from ringbuffer, decode the data to pcm format
   and write the data to ringbuffer. The "fatfs-stream[OUT]" element read pcmd data from
   ringbuffer, and save to tfcard.
*/
bk_err_t adk_aac_decoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t aac_dec, aac_in, pcm_out;

#if 0
    bk_set_printf_sync(true);
    extern void bk_enable_white_list(int enabled);
    bk_enable_white_list(1);
    bk_disable_mod_printf("AUDIO_PIPELINE", 0);
    bk_disable_mod_printf("AUDIO_ELEMENT", 0);
    bk_disable_mod_printf("AUDIO_EVENT", 0);
    //bk_disable_mod_printf("AUDIO_MEM", 0);
    //bk_disable_mod_printf("AAC_DECODER", 0);
    bk_disable_mod_printf("FATFS_STREAM", 0);
    bk_disable_mod_printf("AAC_DECODER_TEST", 0);
#endif
    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    if (BK_OK != tf_mount())
    {
        BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.buf_sz = AAC_DECODER_MAIN_BUFF_SIZE;
    fatfs_reader_cfg.out_block_size = AAC_DECODER_MAIN_BUFF_SIZE;
    fatfs_reader_cfg.out_block_num = 1;
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    aac_in = fatfs_stream_init(&fatfs_reader_cfg);
    TEST_CHECK_NULL(aac_in);

    fatfs_stream_cfg_t fatfs_writer_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    pcm_out = fatfs_stream_init(&fatfs_writer_cfg);
    TEST_CHECK_NULL(pcm_out);

    if (BK_OK != audio_element_set_uri(aac_in, TEST_FATFS_READER))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_set_uri(pcm_out, TEST_FATFS_WRITER))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    aac_decoder_cfg_t aac_decoder_cfg = DEFAULT_AAC_DECODER_CONFIG();
    aac_dec = aac_decoder_init(&aac_decoder_cfg);
    TEST_CHECK_NULL(aac_dec);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, aac_in, "stream_in"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, aac_dec, "aac_dec"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, pcm_out, "stream_out"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"stream_in", "aac_dec", "stream_out"
}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
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

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) aac_dec
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(aac_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED)
                || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                || (int)msg.data == AEL_STATUS_ERROR_PROCESS))
        {
            /* read aac file finish, wait decode complete */
            if (msg.source == (void *)aac_in && (int)msg.data == AEL_STATUS_STATE_FINISHED)
            {
                //not stop
            }
            else
            {
                BK_LOGW(TAG, "[ * ] Stop event received \n");
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
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_in))
    {
        BK_LOGE(TAG, "element unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_dec))
    {
        BK_LOGE(TAG, "element unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, pcm_out))
    {
        BK_LOGE(TAG, "element unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "listener remove fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "listener destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(aac_in))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(aac_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(pcm_out))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    tf_unmount();

    BK_LOGD(TAG, "--------- aac decoder test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}


#if (CONFIG_ADK_ONBOARD_MIC_STREAM && CONFIG_ADK_AAC_ENCODER && CONFIG_ADK_UART_STREAM)
/* The "aac-decoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+                +--------------+               +--------------+               +--------------+
   | onboard-mic  |                |     aac      |               |     aac      |               |     uart     |
   |  stream[IN]  |                |    encoder   |               |   decoder    |               |  stream[OUT] |
   |             src - ringbuf - sink            src - ringbuf - sink           src - ringbuf - sink            |
   |              |                |              |               |              |               |              |
   +--------------+                +--------------+               +--------------+               +--------------+

   Function: Use aac decoder to decode aac file to pcm file in tfcard.

   The "fatfs-stream[IN]" element read aac file from tfcard to ringbuffer. The
   "aac-decoder" element read audio data from ringbuffer, decode the data to pcm format
   and write the data to ringbuffer. The "fatfs-stream[OUT]" element read pcmd data from
   ringbuffer, and save to tfcard.
*/
bk_err_t adk_aac_decoder_test_case_1(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t onboard_mic, aac_enc, aac_dec, uart_out;

#if 0
    bk_set_printf_sync(true);
    extern void bk_enable_white_list(int enabled);
    bk_enable_white_list(1);
    bk_disable_mod_printf("AUDIO_PIPELINE", 0);
    bk_disable_mod_printf("AUDIO_ELEMENT", 0);
    bk_disable_mod_printf("AUDIO_EVENT", 0);
    //bk_disable_mod_printf("AUDIO_MEM", 0);
    //bk_disable_mod_printf("AAC_DECODER", 0);
    bk_disable_mod_printf("FATFS_STREAM", 0);
    bk_disable_mod_printf("AAC_DECODER_TEST", 0);
#endif
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

    aac_decoder_cfg_t aac_decoder_cfg = DEFAULT_AAC_DECODER_CONFIG();
    aac_dec = aac_decoder_init(&aac_decoder_cfg);
    TEST_CHECK_NULL(aac_dec);

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
    if (BK_OK != audio_pipeline_register(pipeline, aac_dec, "aac_dec"))
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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[])
{"onboard_mic", "aac_enc", "aac_dec", "uart_out"
}, 4))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
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

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) aac_dec
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(aac_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED)
                || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                || (int)msg.data == AEL_STATUS_ERROR_PROCESS))
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
        BK_LOGE(TAG, "unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_enc))
    {
        BK_LOGE(TAG, "unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_dec))
    {
        BK_LOGE(TAG, "unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_out))
    {
        BK_LOGE(TAG, "unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "listener destroy fail, %d \n", __LINE__);
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

    if (BK_OK != audio_element_deinit(aac_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_out))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- aac decoder test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
#endif

