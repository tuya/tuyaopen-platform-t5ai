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

#include <os/os.h>
#include <os/mem.h>
#include <stdio.h>
#include <components/log.h>
#include "transfer_act.h"
#include "media_utils.h"
#include "frame_buffer.h"

#define TAG "trs_app"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t enable : 1;
    uint16_t img_format;
    frame_list_node_t *stream;
    frame_cb_t transfer_cb;
    beken_semaphore_t sem;
    beken_thread_t thread;
} transfer_info_t;

extern media_debug_t *media_debug;
transfer_info_t *s_transfer_info = NULL;

static void transfer_app_task_entry(beken_thread_arg_t data)
{
    uint32_t before = 0, after = 0;
    uint8_t log_enable = 0;
    transfer_info_t *transfer_info = (transfer_info_t *)data;
    frame_list_node_t *stream = NULL;
    transfer_info->enable = true;
    frame_buffer_t *frame = NULL;
    rtos_set_semaphore(&transfer_info->sem);

    while (transfer_info->enable)
    {
        if (transfer_info->img_format & IMAGE_MJPEG)
        {
            stream = frame_buffer_list_get_main_stream();
        }
        else
        {
            stream = frame_buffer_list_get_by_format(transfer_info->img_format);
        }

        if (stream == NULL)
        {
            if (transfer_info->stream != stream)
            {
                frame_buffer_fb_deregister(transfer_info->stream, MODULE_WIFI);
                transfer_info->stream = NULL;
            }

            if (transfer_info->enable)
            {
                rtos_delay_milliseconds(500);
            }
        }
        else
        {
            if (transfer_info->stream == NULL)
            {
                LOGD("%s, stream:%p\n", __func__, stream);
                transfer_info->stream = stream;
                frame_buffer_fb_register(transfer_info->stream, MODULE_WIFI);
            }

            if (transfer_info->stream != stream)
            {
                frame_buffer_fb_deregister(transfer_info->stream, MODULE_WIFI);
                transfer_info->stream = stream;
                frame_buffer_fb_register(transfer_info->stream, MODULE_WIFI);
            }
        }

        if (transfer_info->stream == NULL)
        {
            if (log_enable >= 10)
            {
                LOGW("%s, can not find fmt:%d stream!\n", __func__, transfer_info->img_format);
                log_enable = 0;
            }
            else
            {
                log_enable++;
            }
            continue;
        }

        frame = frame_buffer_fb_read(transfer_info->stream, MODULE_WIFI, 100);
        if (frame == NULL)
        {
            if (log_enable >= 10)
            {
                LOGE("read frame NULL timeout %p, %d\n", transfer_info->stream, transfer_info->stream->invalid);
                log_enable = 0;
            }
            else
            {
                log_enable++;
            }
            continue;
        }

        log_enable = 0;
        before = get_current_timestamp();
        media_debug->begin_trs = true;
        media_debug->end_trs = false;

        if (transfer_info->transfer_cb)
        {
            transfer_info->transfer_cb(frame);
        }

        media_debug->end_trs = true;
        media_debug->begin_trs = false;

        after = get_current_timestamp();

        media_debug->meantimes += (after - before);
        media_debug->fps_wifi++;
        media_debug->wifi_kbps += frame->length;

        frame_buffer_fb_read_free(transfer_info->stream, frame, MODULE_WIFI);
    };

    LOGD("transfer_app_task exit\n");
    transfer_info->enable = false;
    transfer_info->thread = NULL;
    if (transfer_info->stream)
    {
        frame_buffer_fb_deregister(transfer_info->stream, MODULE_WIFI);
    }
    rtos_set_semaphore(&transfer_info->sem);
    rtos_delete_thread(NULL);
}

bk_err_t transfer_app_task_init(frame_cb_t cb, uint16_t image_format)
{
    int ret = BK_OK;

    LOGD("%s, %d, format:%d\n", __func__, __LINE__, image_format);

    if (s_transfer_info)
    {
        LOGW("%s, already open\n", __func__);
        return ret;
    }

    s_transfer_info = (transfer_info_t *)os_malloc(sizeof(transfer_info_t));
    if (s_transfer_info == NULL)
    {
        LOGE("%s, malloc failed\n", __func__);
        ret = BK_FAIL;
        return ret;
    }

    os_memset(s_transfer_info, 0, sizeof(transfer_info_t));
    s_transfer_info->transfer_cb = cb;
    s_transfer_info->img_format = image_format;

    ret = rtos_init_semaphore(&s_transfer_info->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, sem init failed\n", __func__);
        goto error;
    }

    ret = rtos_create_thread(&s_transfer_info->thread,
                                BEKEN_DEFAULT_WORKER_PRIORITY,
                                "trs_app_task",
                                (beken_thread_function_t)transfer_app_task_entry,
                                CONFIG_TRANS_APP_TASK_SIZE,
                                (beken_thread_arg_t)s_transfer_info);

    if (BK_OK != ret)
    {
        LOGE("%s transfer_app_task init failed\n", __func__);
        ret = BK_ERR_NO_MEM;
        goto error;
    }

    rtos_get_semaphore(&s_transfer_info->sem, BEKEN_NEVER_TIMEOUT);

    return ret;

error:

    if (s_transfer_info)
    {
        if (s_transfer_info->sem)
        {
            rtos_deinit_semaphore(&s_transfer_info->sem);
        }

        os_free(s_transfer_info);
        s_transfer_info = NULL;
    }

    return ret;
}

bk_err_t transfer_app_task_deinit(void)
{
    bk_err_t ret = BK_OK;
    LOGD("%s, %d\n", __func__, __LINE__);

    transfer_info_t *transfer_info = s_transfer_info;

    if (transfer_info == NULL)
    {
        LOGW("%s, already close\n", __func__);
        return ret;
    }

    transfer_info->enable = false;
    rtos_get_semaphore(&transfer_info->sem, BEKEN_NEVER_TIMEOUT);
    rtos_deinit_semaphore(&transfer_info->sem);
    os_free(transfer_info);
    s_transfer_info = NULL;
    return ret;
}

