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
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_algorithms/aec_algorithm.h>
#include <os/os.h>
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include "bk_posix.h"

#define TAG  "AEC_ALGORITHM_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

#define TEST_VFS_READER_CASE_0  "/sd0/aec_hardware_test.pcm"
#define TEST_VFS_WRITER_CASE_0  "/sd0/aec_hardware_test_out.pcm"

#define TEST_VFS_READER_SRC_CASE_1  "/sd0/mic.pcm"
#define TEST_VFS_READER_REF_CASE_1  "/sd0/ref.pcm"
#define TEST_VFS_WRITER_CASE_1      "/sd0/aec_software_test_out.pcm"

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

/* The "aec-algorithm" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. When aec algorithm work
   in Hardware mode, this element has one src and one sink. The data flow model of this
   element is as follow:
                                   Hardware mode
   +--------------+               +--------------+               +--------------+
   |     vfs      |               |     aec      |               |     vfs      |
   |  stream[IN]  |               |  algorithm   |               | stream[OUT]  |
   |            src - ringbuf - sink           src - ringbuf - sink           ...
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use aec algorithm to process fixed audio data in file.

   The "agc-algorithm" element read audio data from ringbuffer, decode the data to pcm
   format and write the data to ringbuffer.
*/
bk_err_t adk_aec_algorithm_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t aec_alg, vfs_stream_reader, vfs_stream_writer;

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
    aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
    aec_alg_cfg.aec_cfg.fs = 16000;
    aec_alg = aec_algorithm_init(&aec_alg_cfg);
    TEST_CHECK_NULL(aec_alg);

    vfs_stream_cfg_t vfs_reader_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_reader_cfg.type = AUDIO_STREAM_READER;
    vfs_reader_cfg.buf_sz = 1280;
    vfs_reader_cfg.out_block_size = 1280;
    vfs_reader_cfg.out_block_num = 1;
    vfs_stream_reader = vfs_stream_init(&vfs_reader_cfg);
    TEST_CHECK_NULL(vfs_stream_reader);
    if (BK_OK != audio_element_set_uri(vfs_stream_reader, TEST_VFS_READER_CASE_0))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    vfs_stream_cfg_t vfs_writer_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    vfs_stream_writer = vfs_stream_init(&vfs_writer_cfg);
    TEST_CHECK_NULL(vfs_stream_writer);
    if (BK_OK != audio_element_set_uri(vfs_stream_writer, TEST_VFS_WRITER_CASE_0))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, vfs_stream_reader, "file_reader"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, aec_alg, "aec_alg"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, vfs_stream_writer, "file_writer"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"file_reader", "aec_alg", "file_writer"}, 3))
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

        if (msg.source == (void *) vfs_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
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

    if (BK_OK != audio_pipeline_unregister(pipeline, vfs_stream_reader))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, aec_alg))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, vfs_stream_writer))
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

    if (BK_OK != audio_element_deinit(vfs_stream_reader))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(aec_alg))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(vfs_stream_writer))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    vfs_unmount_sd0_fatfs();

    BK_LOGD(TAG, "--------- audio aec algorithm Hardware mode test complete ----------\n");
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

bk_err_t adk_aec_algorithm_test_case_1(void)
{
    audio_pipeline_handle_t record_pipeline;
    audio_element_handle_t aec_alg, src_stream_in, stream_out, ref_stream_in;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    if (BK_OK != vfs_mount_sd0_fatfs())
    {
        BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    /* pipeline 1 record */
    audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
    TEST_CHECK_NULL(record_pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    /* pipeline 1 record */
    vfs_stream_cfg_t src_stream_in_cfg = DEFAULT_VFS_STREAM_CONFIG();
    src_stream_in_cfg.type = AUDIO_STREAM_READER;
    src_stream_in_cfg.buf_sz = 640;
    src_stream_in_cfg.out_block_size = 640;
    src_stream_in_cfg.out_block_num = 1;
    src_stream_in = vfs_stream_init(&src_stream_in_cfg);
    TEST_CHECK_NULL(src_stream_in);
    if (BK_OK != audio_element_set_uri(src_stream_in, TEST_VFS_READER_SRC_CASE_1))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
    aec_alg_cfg.aec_cfg.fs = 16000;
    aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
    aec_alg = aec_algorithm_init(&aec_alg_cfg);
    TEST_CHECK_NULL(aec_alg);
    vfs_stream_cfg_t vfs_writer_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    stream_out = vfs_stream_init(&vfs_writer_cfg);
    TEST_CHECK_NULL(stream_out);
    if (BK_OK != audio_element_set_uri(stream_out, TEST_VFS_WRITER_CASE_1))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    vfs_stream_cfg_t ref_stream_in_cfg = DEFAULT_VFS_STREAM_CONFIG();
    ref_stream_in_cfg.type = AUDIO_STREAM_READER;
    ref_stream_in_cfg.buf_sz = 640;
    ref_stream_in_cfg.out_block_size = 640;
    ref_stream_in_cfg.out_block_num = 1;
    ref_stream_in = vfs_stream_init(&ref_stream_in_cfg);
    TEST_CHECK_NULL(ref_stream_in);
    if (BK_OK != audio_element_set_uri(ref_stream_in, TEST_VFS_READER_REF_CASE_1))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    /* pipeline 1 record */
    if (BK_OK != audio_pipeline_register(record_pipeline, src_stream_in, "src_stream_in"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(record_pipeline, aec_alg, "aec_alg"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(record_pipeline, stream_out, "stream_out"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    /* pipeline 1 record */
    if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]){"src_stream_in", "aec_alg", "stream_out"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    ringbuf_port_cfg_t ref_rb_cfg = {640};
    audio_port_handle_t ref_port = ringbuf_port_init(&ref_rb_cfg);

    if (BK_OK != audio_element_set_output_port(ref_stream_in, ref_port))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK !=  audio_element_set_multi_input_port(aec_alg, ref_port, 0))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    if (BK_OK != audio_pipeline_set_listener(record_pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_run(ref_stream_in))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_resume(ref_stream_in, 0, 0))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
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
    if (BK_OK != audio_pipeline_stop(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_stop(ref_stream_in))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(ref_stream_in))
    {
        BK_LOGE(TAG, "element wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_terminate(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_unregister(record_pipeline, src_stream_in))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(record_pipeline, aec_alg))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(record_pipeline, stream_out))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "event destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(record_pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(src_stream_in))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(aec_alg))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(stream_out))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    audio_port_deinit(ref_port);
    ref_port = NULL;

    if (BK_OK != audio_element_deinit(ref_stream_in))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    vfs_unmount_sd0_fatfs();

    BK_LOGD(TAG, "--------- agc algorithm Software mode test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
