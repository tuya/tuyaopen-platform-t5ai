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
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include <os/os.h>
#include "bk_posix.h"

#if (CONFIG_ADK_ONBOARD_MIC_STREAM && CONFIG_ADK_AAC_ENCODER && CONFIG_ADK_UART_STREAM)
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#endif

#define TAG  "AAC_DECODER_TEST"

#define TEST_VFS_READER  "/sd0/mono_8K_16bit_32000bitrate.aac"
#define TEST_VFS_WRITER  "/sd0/mono_8K_16bit_32000bitrate.pcm"

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


/* mount sdcard */
static int vfs_mount_sd0_fatfs(void)
{
	int ret = BK_OK;
	static bool is_mounted = false;

	if(!is_mounted) {
		struct bk_fatfs_partition partition;
		char *fs_name = NULL;
		fs_name = "fatfs";
		partition.part_type = FATFS_DEVICE;
		partition.part_dev.device_name = FATFS_DEV_SDCARD;
		partition.mount_path = VFS_SD_0_PATITION_0;
		ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
		is_mounted = true;
        BK_LOGI(TAG, "func %s, mount /sd0 \n", __func__);
	}
	return ret;
}

static bk_err_t vfs_unmount_sd0_fatfs(void)
{
    return umount(VFS_SD_0_PATITION_0);
}


/* The "aac-decoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     vfs      |               |     aac      |               |     vfs      |
   |  stream[IN]  |               |   decoder    |               |  stream[OUT] |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use aac decoder to decode aac file to pcm file in tfcard.

   The "vfs-stream[IN]" element read aac file from tfcard to ringbuffer. The
   "aac-decoder" element read audio data from ringbuffer, decode the data to pcm format
   and write the data to ringbuffer. The "vfs-stream[OUT]" element read pcmd data from
   ringbuffer, and save to tfcard.
*/
bk_err_t adk_aac_decoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t aac_dec, aac_in, pcm_out;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    if (BK_OK != vfs_mount_sd0_fatfs())
    {
        BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    vfs_stream_cfg_t vfs_reader_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_reader_cfg.buf_sz = AAC_DECODER_MAIN_BUFF_SIZE;
    vfs_reader_cfg.out_block_size = AAC_DECODER_MAIN_BUFF_SIZE;
    vfs_reader_cfg.out_block_num = 1;
    vfs_reader_cfg.type = AUDIO_STREAM_READER;
    aac_in = vfs_stream_init(&vfs_reader_cfg);
    TEST_CHECK_NULL(aac_in);

    vfs_stream_cfg_t vfs_writer_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    pcm_out = vfs_stream_init(&vfs_writer_cfg);
    TEST_CHECK_NULL(pcm_out);

    if (BK_OK != audio_element_set_uri(aac_in, TEST_VFS_READER))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_set_uri(pcm_out, TEST_VFS_WRITER))
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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"stream_in", "aac_dec", "stream_out"}, 3))
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

        if (msg.source == (void *) aac_dec && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(aac_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
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
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, pcm_out))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
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

    vfs_unmount_sd0_fatfs();

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

   The "onboard-mic-stream[IN]" element write mic data to ringbuffer. The "aac-encoder" element read mic data from ringbuffer,
    encode the data to aac format and write the data to ringbuffer. The "aac-decoder" element read aac data from ringbuffer,
    decode the data to pcm format and write the data to ringbuffer. The "uart-stream[OUT]" element read pcm data from ringbuffer,
     and write it to uart.
*/
bk_err_t adk_aac_decoder_test_case_1(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t onboard_mic, aac_enc, aac_dec, uart_out;

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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"onboard_mic", "aac_enc", "aac_dec", "uart_out"}, 4))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "pipeline set listener fail, %d \n", __LINE__);
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

        if (msg.source == (void *) aac_dec && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(aac_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
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
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_enc))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aac_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_out))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "pipeline listener destroy fail, %d \n", __LINE__);
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
