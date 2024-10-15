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
#include "aec_algorithm.h"
#include <os/os.h>
#include "ff.h"
#include "diskio.h"
#include "fatfs_stream.h"


#define TAG  "AEC_ALGORITHM_TEST"


#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

#define TEST_FATFS_READER_CASE_0  "1:/aec_hardware_test.pcm"
#define TEST_FATFS_WRITER_CASE_0  "1:/aec_hardware_test_out.pcm"

#define TEST_FATFS_READER_SRC_CASE_1  "1:/mic.pcm"
#define TEST_FATFS_READER_REF_CASE_1  "1:/ref.pcm"
#define TEST_FATFS_WRITER_CASE_1  "1:/aec_software_test_out.pcm"

static FATFS *pfs = NULL;

static bk_err_t tf_mount(void)
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
		BK_LOGI(TAG, "f_unmount OK!\r\n");
	}

	if (pfs)
	{
		os_free(pfs);
		pfs = NULL;
	}

	return BK_OK;
}

/* The "aec-algorithm" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. When aec algorithm work
   in Hardware mode, this element has one src and one sink. The data flow model of this
   element is as follow:
                                   Hardware mode
   +--------------+               +--------------+               +--------------+
   |    fatfs     |               |     aec      |               |    fatfs     |
   |  stream[IN]  |               |  algorithm   |               | stream[OUT]  |
   |            src - ringbuf - sink           src - ringbuf - sink           ...
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use aec algorithm to process fixed audio data in file.

   The "agc-algorithm" element read audio data from ringbuffer, decode the data to pcm
   format and write the data to ringbuffer.
*/
bk_err_t asdf_aec_algorithm_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t aec_alg, fatfs_stream_reader, fatfs_stream_writer;
#if 1
		bk_set_printf_sync(true);
//		extern void bk_enable_white_list(int enabled);
//		bk_enable_white_list(1);
//		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
//		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
//		bk_disable_mod_printf("AUDIO_EVENT", 0);
//		bk_disable_mod_printf("AUDIO_MEM", 0);
//		bk_disable_mod_printf("AEC_ALGORITHM", 0);
//		bk_disable_mod_printf("AEC_ALGORITHM_TEST", 0);
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	if (BK_OK != tf_mount()) {
		BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	pipeline = audio_pipeline_init(&pipeline_cfg);
	TEST_CHECK_NULL(pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
	aec_alg_cfg.aec_cfg.fs = 16000;
	aec_alg = aec_algorithm_init(&aec_alg_cfg);
	TEST_CHECK_NULL(aec_alg);

	fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_reader_cfg.type = AUDIO_STREAM_READER;
	fatfs_reader_cfg.buf_sz = 1280;
	fatfs_reader_cfg.out_rb_size = 1280;
	fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);
	TEST_CHECK_NULL(fatfs_stream_reader);
	if (BK_OK != audio_element_set_uri(fatfs_stream_reader, TEST_FATFS_READER_CASE_0)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	fatfs_stream_cfg_t fatfs_writer_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_writer_cfg.type = AUDIO_STREAM_WRITER;
	fatfs_stream_writer = fatfs_stream_init(&fatfs_writer_cfg);
	TEST_CHECK_NULL(fatfs_stream_writer);
	if (BK_OK != audio_element_set_uri(fatfs_stream_writer, TEST_FATFS_WRITER_CASE_0)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(pipeline, fatfs_stream_reader, "file_reader")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(pipeline, aec_alg, "aec_alg")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(pipeline, fatfs_stream_writer, "file_writer")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"file_reader", "aec_alg", "file_writer"}, 3)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

	if (BK_OK != audio_pipeline_set_listener(pipeline, evt)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
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

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_terminate(pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_unregister(pipeline, fatfs_stream_reader)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(pipeline, aec_alg)) {
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
	if (BK_OK != audio_element_deinit(aec_alg)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_element_deinit(fatfs_stream_writer)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	tf_unmount();

	BK_LOGI(TAG, "--------- audio aec algorithm test complete ----------\n");
	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}



/* The "aec-algorithm" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. When aec algorithm work
   in Software mode, this element has one src and two sinks. The pipeline 1 is record
   pipeline, this pipeline read source signal data from file, process the data through aec
   algorithm, and write output data to file. The element 1 is play element, this element
   read reference signal from file and write data to ringbuffer.
   The data flow model of this function is as follow:
                                   Software mode
+--------------------------------------------------------------------------------------------+
|                                                                                            |
|  +--------------+                                                                          |
|  |    fatfs     |                                                                          |
|  |  stream[IN]  |                                                                          |
|  |             src ---+                                                                    |
|  |              |     |                                                                    |
|  +--------------+     |                                                                    |
|                       |                +--------------+               +--------------+     |    pipeline 1 (record)
|                       |                |     aec      |               |     fatfs    |     |
|                       |                |  algorithm   |               |  stream[out] |     |
|                       +---> ringbuf - sink            |               |              |     |
|                                        |             src - ringbuf - sink            |     |
|                       +---> ringbuf - sink            |               |              |     |
|                       |                |              |               |              |     |
|                       |                +--------------+               +--------------+     |
|                       |                                                                    |
+-----------------------|--------------------------------------------------------------------+
                        |
+-----------------------|-----+
|                       |     |
|  +--------------+     |     |
|  |    fatfs     |     |     |
|  |  stream[IN]  |     |     |
|  |             src ---+     |    element 1 (play)
|  |              |           |
|  +--------------+           |
|                             |
+-----------------------------+

   Function: Use aec algorithm to process fixed audio data in file.

   The "agc-algorithm" element read audio data from two ringbuffers, process the data to pcm and write the data
   to ringbuffer.
*/

bk_err_t asdf_aec_algorithm_test_case_1(void)
{
	audio_pipeline_handle_t record_pipeline;
	audio_element_handle_t aec_alg, src_stream_in, stream_out, ref_stream_in;
#if 1
		bk_set_printf_sync(true);
#if 0
		extern void bk_enable_white_list(int enabled);
		bk_enable_white_list(1);
		bk_disable_mod_printf("AUDIO_PIPELINE", 0);
		bk_disable_mod_printf("AUDIO_ELEMENT", 0);
		bk_disable_mod_printf("AUDIO_EVENT", 0);
		bk_disable_mod_printf("AUDIO_MEM", 0);
		bk_disable_mod_printf("AEC_ALGORITHM", 0);
		bk_disable_mod_printf("AEC_ALGORITHM_TEST", 0);
#endif
#endif
	BK_LOGI(TAG, "--------- %s ----------\n", __func__);
	AUDIO_MEM_SHOW("start \n");

	if (BK_OK != tf_mount()) {
		BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step1: pipeline init ----------\n");
	/* pipeline 1 record */
	audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
	TEST_CHECK_NULL(record_pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	/* pipeline 1 record */
	fatfs_stream_cfg_t src_stream_in_cfg = FATFS_STREAM_CFG_DEFAULT();
	src_stream_in_cfg.type = AUDIO_STREAM_READER;
	src_stream_in_cfg.buf_sz = 640;
	src_stream_in_cfg.out_rb_size = 640;
	src_stream_in = fatfs_stream_init(&src_stream_in_cfg);
	TEST_CHECK_NULL(src_stream_in);
	if (BK_OK != audio_element_set_uri(src_stream_in, TEST_FATFS_READER_SRC_CASE_1)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
	aec_alg_cfg.aec_cfg.fs = 16000;
	aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
	aec_alg = aec_algorithm_init(&aec_alg_cfg);
	TEST_CHECK_NULL(aec_alg);
	fatfs_stream_cfg_t fatfs_writer_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_writer_cfg.type = AUDIO_STREAM_WRITER;
	stream_out = fatfs_stream_init(&fatfs_writer_cfg);
	TEST_CHECK_NULL(stream_out);
	if (BK_OK != audio_element_set_uri(stream_out, TEST_FATFS_WRITER_CASE_1)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	fatfs_stream_cfg_t ref_stream_in_cfg = FATFS_STREAM_CFG_DEFAULT();
	ref_stream_in_cfg.type = AUDIO_STREAM_READER;
	ref_stream_in_cfg.buf_sz = 640;
	ref_stream_in_cfg.out_rb_size = 640;
	ref_stream_in = fatfs_stream_init(&ref_stream_in_cfg);
	TEST_CHECK_NULL(ref_stream_in);
	if (BK_OK != audio_element_set_uri(ref_stream_in, TEST_FATFS_READER_REF_CASE_1)) {
		BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	/* pipeline 1 record */
	if (BK_OK != audio_pipeline_register(record_pipeline, src_stream_in, "src_stream_in")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(record_pipeline, aec_alg, "aec_alg")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(record_pipeline, stream_out, "stream_out")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	/* pipeline 1 record */
	if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]) {"src_stream_in", "aec_alg", "stream_out"}, 3)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	ringbuf_handle_t ref_rb = rb_create(640, 1);
	if (BK_OK != audio_element_set_output_ringbuf(ref_stream_in, ref_rb)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK !=  audio_element_set_multi_input_ringbuf(aec_alg, ref_rb, 0)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
	if (BK_OK != audio_pipeline_set_listener(record_pipeline, evt)) {
		BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step6: pipeline run ----------\n");
	if (BK_OK != audio_pipeline_run(record_pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_element_run(ref_stream_in)) {
		BK_LOGE(TAG, "audio_element_run fail \n");
		return BK_FAIL;
	}
	if (BK_OK != audio_element_resume(ref_stream_in, 0, 0)) {
		BK_LOGE(TAG, "audio_element_resume fail \n");
		return BK_FAIL;
	}

	while (1) {
		audio_event_iface_msg_t msg;
		bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
			continue;
		}

		if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
			&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
			&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
			BK_LOGW(TAG, "[ * ] Stop event received \n");
			break;
		}
	}

	BK_LOGI(TAG, "--------- step7: deinit pipeline ----------\n");
	if (BK_OK != audio_pipeline_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_stop(ref_stream_in)) {
		BK_LOGE(TAG, "audio_element_stop fail \n");
		return BK_FAIL;
	}
	if (BK_OK != audio_element_wait_for_stop(ref_stream_in)) {
		BK_LOGE(TAG, "element wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_terminate(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_unregister(record_pipeline, src_stream_in)) {
		BK_LOGE(TAG, "pipeline unregister fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(record_pipeline, aec_alg)) {
		BK_LOGE(TAG, "pipeline unregister fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(record_pipeline, stream_out)) {
		BK_LOGE(TAG, "pipeline unregister fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_remove_listener(record_pipeline)) {
		BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "event destroy fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_deinit(record_pipeline)) {
		BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_element_deinit(src_stream_in)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_element_deinit(aec_alg)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_element_deinit(stream_out)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	rb_destroy(ref_rb);
	ref_rb = NULL;

	if (BK_OK != audio_element_deinit(ref_stream_in)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	tf_unmount();

	BK_LOGI(TAG, "--------- agc algorithm test complete ----------\n");

	AUDIO_MEM_SHOW("end \n");

	return BK_OK;
}


/* combine mic.pcm and ref.pcm to aec_hardware_test.pcm */
void aec_hardware_test_file_combine(void)
{
	//uint8_t buffer[sizeof(AECContext)];
	char mic_file_name[] = "1:/mic.pcm";
	char ref_file_name[] = "1:/ref.pcm";
	char out_file_name[] = "1:/aec_hardware_test.pcm";
	FIL file_mic;
	FIL file_ref;
	FIL file_out;
	FRESULT fr;
	uint32 uiTemp = 0;
	FSIZE_t test_data_size = 0;

	int16_t* ref_addr = NULL;
	int16_t* mic_addr = NULL;
	int16_t* out_addr = NULL;

	tf_mount();

	fr = f_open(&file_mic, mic_file_name, FA_READ);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", mic_file_name);
		return;
	}
	fr = f_open(&file_ref, ref_file_name, FA_READ);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", ref_file_name);
		return;
	}
	fr = f_open(&file_out, out_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", out_file_name);
		return;
	}

	mic_addr = os_malloc(320 * 2);
	os_memset(mic_addr, 0x00, 320 * 2);
	ref_addr = os_malloc(320 * 2);
	os_memset(ref_addr, 0x00, 320 * 2);
	out_addr = os_malloc(320 * 4);
	os_memset(out_addr, 0x00, 320 * 4);

	test_data_size = f_size(&file_mic);
	os_printf("frame_samples * 2 = %d \r\n", (320 * 2));
	while(test_data_size >= (320 * 2))
	{
		fr = f_read(&file_ref, ref_addr, 320 * 2, &uiTemp);
		if (fr != FR_OK) {
			os_printf("read ref file fail.\r\n");
			break;
		}

		fr = f_read(&file_mic, mic_addr, 320 * 2, &uiTemp);
		if (fr != FR_OK) {
			os_printf("read mic file fail.\r\n");
			break;
		}

		for (uint32_t i = 0; i < 320; i++) {
			out_addr[2*i] = mic_addr[i];
			out_addr[2*i+1] = ref_addr[i];
		}

		fr = f_write(&file_out, (void *)out_addr, 320 * 4, &uiTemp);
		if (fr != FR_OK) {
			os_printf("write output data %s fail.\r\n", out_file_name);
			break;
		}

		test_data_size -= 320 * 2;
	}

	fr = f_close(&file_mic);
	if (fr != FR_OK) {
		os_printf("close mic file %s fail!\r\n", mic_file_name);
		return;
	}

	fr = f_close(&file_ref);
	if (fr != FR_OK) {
		os_printf("close ref file %s fail!\r\n", ref_file_name);
		return;
	}

	fr = f_close(&file_out);
	if (fr != FR_OK) {
		os_printf("close out file %s fail!\r\n", out_file_name);
		return;
	}

	os_free(mic_addr);
	os_free(ref_addr);
	os_free(out_addr);
	tf_unmount();

	os_printf("test finish \r\n");
}

