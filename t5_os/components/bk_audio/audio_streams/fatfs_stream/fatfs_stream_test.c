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
#include "fatfs_stream.h"
#if CONFIG_SYS_CPU0
#include "ff.h"
#include "diskio.h"
#endif

#define TAG  "FTFS_STR_TEST"

#define TEST_FATFS_READER  "1:/mic.pcm"
#define TEST_FATFS_WRITER  "1:/mic_fatfs_stream.pcm"

#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

#if CONFIG_SYS_CPU0
static bk_err_t tf_mount(FATFS *pfs)
{
	FRESULT fr;

	if (pfs != NULL)
	{
		os_free(pfs);
	}

	pfs = os_malloc(sizeof(FATFS));
	if(NULL == pfs)
	{
		BK_LOGI(TAG, "f_mount malloc failed!\r\n");
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
		BK_LOGI(TAG, "f_mount OK!\r\n");
	}

	return BK_OK;
}

static bk_err_t tf_unmount(FATFS *pfs)
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
        BK_LOGI(TAG, "f_unmount OK!\r\n");
    }

	if (pfs)
	{
		os_free(pfs);
		pfs = NULL;
	}

    return BK_OK;
}

static uint64_t get_file_size(const char *name)
{
	FRESULT fr;
    FIL f;
    uint64_t size = 0;

	fr = f_open(&f, name, FA_READ);
	if (fr != FR_OK) {
		BK_LOGE(TAG, "Failed to open. File name: %s, error: %d, line: %d \n", name, fr, __LINE__);
		return BK_FAIL;
	}

    size = f_size(&f);
    f_close(&f);
    return size;
}
static void file_size_comparison(const char *file1, const char *file2)
{
    uint64_t size1 = get_file_size(file1);
    uint64_t size2 = get_file_size(file2);
    BK_LOGI(TAG, "%s size is 0x%x%x, %s size is 0x%x%x \n", file1, (uint32_t)(size1>>32), (uint32_t)size1, file2, (uint32_t)(size2>>32), (uint32_t)size2);
    if (size1 == size2) {
        BK_LOGI(TAG, "The two files are the same size \n");
    } else {
        BK_LOGI(TAG, "The two files are not the same size \n");
    }
}
#endif

/* The case check fatfs stream memory leaks. */
bk_err_t asdf_fatfs_stream_test_case_0(void)
{
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
#if 0
			bk_set_printf_sync(true);
			extern void bk_enable_white_list(int enabled);
			bk_enable_white_list(1);
			bk_disable_mod_printf("AUD_PIPE", 0);
			bk_disable_mod_printf("AUD_ELE", 0);
			bk_disable_mod_printf("AUD_EVT", 0);
			bk_disable_mod_printf("AUD_MEM", 0);
			bk_disable_mod_printf("FTFS_STR", 0);
			bk_disable_mod_printf("FTFS_STR_TEST", 0);
#endif
    audio_element_handle_t fatfs_stream_reader;
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    int cnt = 1;
    AUDIO_MEM_SHOW("BEFORE FATFS_STREAM_INIT MEMORY TEST \n");
    while (cnt--) {
		rtos_delay_milliseconds(1000);
		BK_LOGI(TAG, "--------- step1: element init ----------\n");
        fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
#if 0
		rtos_delay_milliseconds(1000);
		if (BK_OK != audio_element_set_uri(fatfs_stream_reader, TEST_FATFS_READER)) {
			BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
			return BK_FAIL;
		}
		rtos_delay_milliseconds(1000);
		BK_LOGI(TAG, "--------- step222: element run ----------\n");
		if (BK_OK != audio_element_run(fatfs_stream_reader)) {
			BK_LOGE(TAG, "audio_element_run fail \n");
			return BK_FAIL;
		}
		rtos_delay_milliseconds(1000);
		BK_LOGI(TAG, "--------- step333: element resume ----------\n");
		if (BK_OK != audio_element_resume(fatfs_stream_reader, 0, 4000 / portTICK_RATE_MS)) {
			BK_LOGE(TAG, "audio_element_resume fail \n");
			return BK_FAIL;
		}
#endif
		rtos_delay_milliseconds(1000);
		BK_LOGI(TAG, "--------- step2: element deinit ----------\n");
        audio_element_deinit(fatfs_stream_reader);
    }
    AUDIO_MEM_SHOW("AFTER FATFS_STREAM_INIT MEMORY TEST \n");

	BK_LOGI(TAG, "--------- fatfs stream test complete ----------\n");

	return BK_OK;
}

/* The "fatfs-stream[IN]" element is producer that has only one src and no sink
   and this element is the first element of the pipeline. The "fatfs-stream[OUT]"
   element is consumer that has only one sink and no src and this element is the
   last element of the pipeline.
   The data flow model of this element is as follow: 
   +--------------+               +--------------+
   |    fatfs     |               |    fatfs     |
   |  stream[IN]  |               |  stream[out] |
   |             src - ringbuf - sink            |
   |              |               |              |
   +--------------+               +--------------+

   Function: Copy file in tfcard by adf

   The "fatfs-stream[IN]" element read audio data through callback api from tfcard
   and write the data to ringbuffer. The "fatfs-stream[OUT]" element read audio data
   from ringbuffer and write the data to tfcard through callback api.
*/
bk_err_t asdf_fatfs_stream_test_case_1(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t fatfs_stream_reader, fatfs_stream_writer;

#if 0
		bk_set_printf_sync(true);
		extern void bk_enable_white_list(int enabled);
		bk_enable_white_list(1);
//		bk_disable_mod_printf("AUD_PIPE", 0);
		bk_disable_mod_printf("AUD_ELE", 0);
//		bk_disable_mod_printf("AUD_EVT", 0);
//		bk_disable_mod_printf("AUD_MEM", 0);
		bk_disable_mod_printf("FTFS_STR", 0);
		bk_disable_mod_printf("FTFS_STR_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
	TEST_CHECK_NULL(pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);
    TEST_CHECK_NULL(fatfs_stream_reader);

    fatfs_stream_cfg_t fatfs_writer_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    fatfs_stream_writer = fatfs_stream_init(&fatfs_writer_cfg);
    TEST_CHECK_NULL(fatfs_stream_writer);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, fatfs_stream_reader, "file_reader")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_register(pipeline, fatfs_stream_writer, "file_writer")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"file_reader", "file_writer"}, 2)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step5: set element uri ----------\n");
    if (BK_OK != audio_element_set_uri(fatfs_stream_reader, TEST_FATFS_READER)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_element_set_uri(fatfs_stream_writer, TEST_FATFS_WRITER)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step6: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#if 1
	BK_LOGI(TAG, "--------- step7: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    while (1) {
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) fatfs_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

	BK_LOGI(TAG, "--------- step8: stop pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step9: check test result ----------\n");
#if CONFIG_SYS_CPU0
	file_size_comparison(TEST_FATFS_READER, TEST_FATFS_WRITER);
#endif

	BK_LOGI(TAG, "--------- step10: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_terminate(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
    if (BK_OK != audio_pipeline_unregister(pipeline, fatfs_stream_reader)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
    if (BK_OK != audio_pipeline_unregister(pipeline, fatfs_stream_writer)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_remove_listener(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_pipeline_deinit(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_element_deinit(fatfs_stream_reader)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

    if (BK_OK != audio_element_deinit(fatfs_stream_writer)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- audio event test complete ----------\n");

	return BK_OK;
}

