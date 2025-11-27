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
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include "bk_posix.h"

#define TAG  "VFS_STR_TEST"

#define TEST_VFS_READER  "/sd0/aec_mic.pcm"
#define TEST_VFS_WRITER  "/sd0/aec_mic_bk.pcm"


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

static uint64_t get_file_size(const char *name)
{
    uint64_t size = 0;
    int fd = 0;
    int ret = 0;
	struct stat statbuf;

	fd = open(name, O_RDONLY);
	if (fd < 0) {
		BK_LOGE(TAG, "Failed to open. File name: %s, fd: %d, line: %d \n", name, fd, __LINE__);
		return BK_FAIL;
	}

	ret = stat(name, &statbuf);
	if (ret < 0) {
		BK_LOGE(TAG, "Failed to stat. File name: %s, ret: %d, line: %d \n", name, ret, __LINE__);
		return BK_FAIL;
	}
    size = statbuf.st_size;

    close(fd);

    return size;
}

static void file_size_comparison(const char *file1, const char *file2)
{
    uint64_t size1 = get_file_size(file1);
    uint64_t size2 = get_file_size(file2);
    BK_LOGD(TAG, "%s size is 0x%x%x, %s size is 0x%x%x \n", file1, (uint32_t)(size1 >> 32), (uint32_t)size1, file2, (uint32_t)(size2 >> 32), (uint32_t)size2);
    if (size1 == size2)
    {
        BK_LOGD(TAG, "The two files are the same size \n");
    }
    else
    {
        BK_LOGD(TAG, "The two files are not the same size \n");
    }
}


/* The case check vfs stream memory leaks. */
bk_err_t adk_vfs_stream_test_case_0(void)
{
    BK_LOGD(TAG, "--------- %s ----------\n", __func__);

    audio_element_handle_t vfs_stream_reader;
    vfs_stream_cfg_t vfs_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_cfg.type = AUDIO_STREAM_READER;
    int cnt = 1;
    AUDIO_MEM_SHOW("BEFORE VFS_STREAM_INIT MEMORY TEST \n");
    while (cnt--)
    {
        rtos_delay_milliseconds(1000);
        BK_LOGD(TAG, "--------- step1: element init ----------\n");
        vfs_stream_reader = vfs_stream_init(&vfs_cfg);
        rtos_delay_milliseconds(1000);
        BK_LOGD(TAG, "--------- step2: element deinit ----------\n");
        audio_element_deinit(vfs_stream_reader);
    }
    AUDIO_MEM_SHOW("AFTER VFS_STREAM_INIT MEMORY TEST \n");

    BK_LOGD(TAG, "--------- vfs stream test complete ----------\n");

    return BK_OK;
}

/* The "vfs-stream[IN]" element is producer that has only one src and no sink
   and this element is the first element of the pipeline. The "vfs-stream[OUT]"
   element is consumer that has only one sink and no src and this element is the
   last element of the pipeline.
   The data flow model of this element is as follow:
   +--------------+               +--------------+
   |      vfs     |               |      vfs     |
   |  stream[IN]  |               |  stream[out] |
   |             src - ringbuf - sink            |
   |              |               |              |
   +--------------+               +--------------+

   Function: Copy file in sdcard

   The "vfs-stream[IN]" element read audio data through callback api from tfcard
   and write the data to ringbuffer. The "vfs-stream[OUT]" element read audio data
   from ringbuffer and write the data to sdcard through callback api.
*/
bk_err_t adk_vfs_stream_test_case_1(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t vfs_stream_reader, vfs_stream_writer;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);

    vfs_mount_sd0_fatfs();

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    vfs_stream_cfg_t vfs_reader_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_reader_cfg.type = AUDIO_STREAM_READER;
    vfs_stream_reader = vfs_stream_init(&vfs_reader_cfg);
    TEST_CHECK_NULL(vfs_stream_reader);

    vfs_stream_cfg_t vfs_writer_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_writer_cfg.type = AUDIO_STREAM_WRITER;
    vfs_stream_writer = vfs_stream_init(&vfs_writer_cfg);
    TEST_CHECK_NULL(vfs_stream_writer);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, vfs_stream_reader, "file_reader"))
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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"file_reader", "file_writer"}, 2))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: set element uri ----------\n");
    if (BK_OK != audio_element_set_uri(vfs_stream_reader, TEST_VFS_READER))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_set_uri(vfs_stream_writer, TEST_VFS_WRITER))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
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

        if (msg.source == (void *) vfs_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
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
    file_size_comparison(TEST_VFS_READER, TEST_VFS_WRITER);

    BK_LOGD(TAG, "--------- step10: deinit pipeline ----------\n");
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

    if (BK_OK != audio_element_deinit(vfs_stream_writer))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    vfs_unmount_sd0_fatfs();

    BK_LOGD(TAG, "--------- vfs stream test complete ----------\n");

    return BK_OK;
}

