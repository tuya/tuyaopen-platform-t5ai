// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <stdio.h>

#include "frame_buffer.h"

#include "media_evt.h"
#include "storage_act.h"

#define TAG "storage_major"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define MAX_NAME_LENGTH (31)

typedef enum
{
    STORAGE_PICTURE_MODE = 0,
    STORAGE_STREAM_MODE,
} storage_mode_t;

typedef enum
{
    STORAGE_STATE_DISABLED,
    STORAGE_STATE_ENABLED,
} storage_state_t;

typedef enum
{
    STORAGE_TASK_CAPTURE,
    STORAGE_TASK_SAVE_START,
    STORAGE_TASK_SAVE_STOP,
    STORAGE_TASK_EXIT,
} storage_task_evt_t;

typedef struct
{
    storage_state_t enable;
    image_format_t img_format;
    storage_mode_t mode;
    frame_list_node_t *stream;
    frame_cb_t cb;
    beken_queue_t queue;
    beken_thread_t thread;
    beken_semaphore_t sem;
    char name[MAX_NAME_LENGTH];
} storage_info_t;

static storage_info_t *s_storage_config = NULL;

static bk_err_t storage_app_task_send_msg(uint32_t event, uint32_t param)
{
    bk_err_t ret = BK_FAIL;
    media_msg_t msg;

    storage_info_t *config = s_storage_config;

    if (config)
    {
        msg.event = event;
        msg.param = param;

        ret = rtos_push_to_queue(&config->queue, &msg, BEKEN_NO_WAIT);
        if (BK_OK != ret)
        {
            LOGE("push storage task_queue failed!\n");
        }
    }

    return ret;
}

frame_list_node_t *storage_app_get_target_stream(storage_info_t *info)
{
    void *stream = NULL;
    uint8_t retry_times = 5;
    do {
        if (info->img_format & IMAGE_H264)
        {
            stream = frame_buffer_list_get_by_format(IMAGE_H264);
        }
        else
        {
            stream = frame_buffer_list_get_main_stream();
        }

        retry_times--;

        if (stream == NULL && info->enable == STORAGE_STATE_ENABLED)
        {
            rtos_delay_milliseconds(1000);
        }
    } while(stream == NULL && retry_times && info->enable == STORAGE_STATE_ENABLED);

    if (stream == NULL)
    {
        LOGE("%s, get target format:%d stream timeout 5s, please check camera or stream have been opend\n", __func__, info->img_format);
    }

    return stream;
}

static void storage_app_task_save_start_handle(uint32_t param)
{
    frame_buffer_t *frame = NULL;
    frame_list_node_t *stream = NULL;
    storage_info_t *info = (storage_info_t *)param;
    char filename[51] = {0};

    while (info->enable == STORAGE_STATE_ENABLED)
    {
        stream = storage_app_get_target_stream(info);
        if (stream == NULL)
        {
            if (info->stream != stream)
            {
                frame_buffer_fb_deregister(info->stream, MODULE_CAPTURE);
                info->stream = NULL;
            }
            continue;
        }
        else
        {
            if (info->stream == NULL)
            {
                LOGD("%s, stream:%p\n", __func__, stream);
                info->stream = stream;
                frame_buffer_fb_register(info->stream, MODULE_CAPTURE);
            }
        }

        if (info->stream == NULL)
        {
            continue;
        }

        frame = frame_buffer_fb_read(info->stream, MODULE_CAPTURE, 100);
        if (frame == NULL)
        {
            LOGE("read frame NULL timeout\n");
        }
        else
        {
            if (info->cb)
            {
                info->cb(frame);
            }
            else
            {
                if (info->mode == STORAGE_PICTURE_MODE)
                {
                    sprintf(filename, "%d_%s", frame->sequence, info->name);
                    bk_mem_save_to_sdcard(filename, frame->frame, frame->length);
                }
                else
                {
                    bk_mem_append_save_to_sdcard(info->name, frame->frame, frame->length);
                }
            }
            frame_buffer_fb_read_free(info->stream, frame, MODULE_CAPTURE);
        }
    }

    if (info->stream)
    {
        frame_buffer_fb_deregister(info->stream, MODULE_CAPTURE);
        info->stream = NULL;
    }

    rtos_set_semaphore(&info->sem);
}

static void storage_app_task_capture_handle(uint32_t param)
{
    frame_buffer_t *frame = NULL;
    storage_info_t *info = (storage_info_t *)param;

    info->stream = storage_app_get_target_stream(info);

    if (info->stream == NULL)
    {
        rtos_set_semaphore(&info->sem);
        LOGE("%s, %d capture fail\n", __func__, __LINE__);
        return;
    }

    frame_buffer_fb_register(info->stream, MODULE_CAPTURE);
    frame = frame_buffer_fb_read(info->stream, MODULE_CAPTURE, 100);

    if (frame == NULL)
    {
        LOGE("read frame NULL timeout\n");
    }
    else
    {
        LOGD("%s, seq:%d, length:%d\n", __func__, frame->sequence, frame->length);
        if (info->cb)
        {
            info->cb(frame);
        }
        else
        {
            bk_mem_save_to_sdcard(info->name, frame->frame, frame->length);
        }
        frame_buffer_fb_read_free(info->stream, frame, MODULE_CAPTURE);
    }

    frame_buffer_fb_deregister(info->stream, MODULE_CAPTURE);
    info->stream = NULL;
    rtos_set_semaphore(&info->sem);
}

static void storage_app_task_entry(beken_thread_arg_t data)
{
    bk_err_t ret = BK_OK;
    media_msg_t msg;

    storage_info_t *config = (storage_info_t *)data;

    rtos_set_semaphore(&config->sem);

    while (1)
    {
        ret = rtos_pop_from_queue(&config->queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case STORAGE_TASK_CAPTURE:
                    storage_app_task_capture_handle(msg.param);
                    break;

                case STORAGE_TASK_SAVE_START:
                    storage_app_task_save_start_handle(msg.param);
                    break;

                case STORAGE_TASK_EXIT:
                    goto exit;

                default:
                    break;
            }
        }
    }

exit:

    LOGD("storage_major_task exit success!\r\n");
    rtos_deinit_queue(&config->queue);
    config->queue = NULL;
    config->thread = NULL;
    rtos_set_semaphore(&config->sem);
    rtos_delete_thread(NULL);
}

bk_err_t storage_app_task_init(frame_cb_t cb)
{
    int ret = BK_OK;
    LOGD("%s, %d\n", __func__, __LINE__);

    if (s_storage_config)
    {
        LOGW("%s, already open\n", __func__);
        return ret;
    }

    s_storage_config = (storage_info_t *)os_malloc(sizeof(storage_info_t));
    if (s_storage_config == NULL)
    {
        LOGE("%s, malloc failed\n", __func__);
        ret = BK_FAIL;
        return ret;
    }

    os_memset(s_storage_config, 0, sizeof(storage_info_t));
    s_storage_config->cb = cb;

    ret = rtos_init_semaphore(&s_storage_config->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, sem init failed\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&s_storage_config->queue,
                            "storage_queue",
                            sizeof(media_msg_t),
                            5);

    if (BK_OK != ret)
    {
        LOGE("%s storage_queue init failed\n", __func__);
        goto error;
    }

    ret = rtos_create_thread(&s_storage_config->thread,
                                BEKEN_DEFAULT_WORKER_PRIORITY,
                                "storage_app_task",
                                (beken_thread_function_t)storage_app_task_entry,
                                2560,
                                (beken_thread_arg_t)s_storage_config);

    if (BK_OK != ret)
    {
        LOGE("%s storage_app_task init failed\n", __func__);
        goto error;
    }

    rtos_get_semaphore(&s_storage_config->sem, BEKEN_NEVER_TIMEOUT);

    return ret;

error:

    if (s_storage_config)
    {
        if (s_storage_config->sem)
        {
            rtos_deinit_semaphore(&s_storage_config->sem);
        }

        os_free(s_storage_config);
        s_storage_config = NULL;
    }

    return ret;
}

bk_err_t storage_app_task_deinit(void)
{
    int ret = BK_OK;
    LOGD("%s, %d\n", __func__, __LINE__);

    storage_info_t *storage_info = s_storage_config;

    if (storage_info == NULL)
    {
        LOGW("%s, already close\n", __func__);
        return ret;
    }

    if (storage_info->enable == STORAGE_STATE_ENABLED)
    {
        LOGW("%s, in capturing/saving state, please check...\n", __func__);
        ret = BK_FAIL;
        return ret;
    }

    ret = storage_app_task_send_msg(STORAGE_TASK_EXIT, (uint32_t)storage_info);
    if (BK_OK == ret)
    {
        rtos_get_semaphore(&storage_info->sem, BEKEN_NEVER_TIMEOUT);
        rtos_deinit_semaphore(&storage_info->sem);
        os_free(storage_info);
        s_storage_config = NULL;
    }

    return ret;
}

bk_err_t storage_app_task_capture(image_format_t format, char *name)
{
    bk_err_t ret = BK_FAIL;
    LOGD("%s, %d\n", __func__, __LINE__);

    storage_info_t *storage_info = s_storage_config;
    if (storage_info == NULL || name == NULL)
    {
        LOGW("%s, not open or name is null:%p\n", __func__, name);
        return ret;
    }

    if (storage_info->enable == STORAGE_STATE_ENABLED)
    {
        LOGW("%s, in capture state, please check...\n", __func__);
        return ret;
    }

    uint32_t length = os_strlen(name);
    os_memset(storage_info->name, 0, MAX_NAME_LENGTH);
    if (length > MAX_NAME_LENGTH)
    {
        os_memcpy(storage_info->name, name, MAX_NAME_LENGTH - 1);
    }
    else
    {
        os_memcpy(storage_info->name, name, length);
    }

    storage_info->name[MAX_NAME_LENGTH - 1] = '\0';
    LOGV("%s, %d, name:%s\n", __func__, __LINE__, storage_info->name);
    storage_info->img_format = format;
    storage_info->enable = STORAGE_STATE_ENABLED;
    storage_info->mode = STORAGE_PICTURE_MODE;
    ret = storage_app_task_send_msg(STORAGE_TASK_CAPTURE, (uint32_t)storage_info);
    if (ret == BK_OK)
    {
        rtos_get_semaphore(&storage_info->sem, BEKEN_NEVER_TIMEOUT);
        storage_info->enable = STORAGE_STATE_DISABLED;
    }

    return ret;
}

bk_err_t storage_app_task_save_start(image_format_t format, char *name)
{
    bk_err_t ret = BK_FAIL;
    LOGD("%s, %d\n", __func__, __LINE__);

    storage_info_t *storage_info = s_storage_config;
    if (storage_info == NULL)
    {
        LOGW("%s, not open\n", __func__);
        return ret;
    }

    if (storage_info->enable == STORAGE_STATE_ENABLED)
    {
        LOGW("%s, in saving state, please check...\n", __func__);
        return ret;
    }

    uint32_t length = os_strlen(name);
    os_memset(storage_info->name, 0, MAX_NAME_LENGTH);
    if (length > MAX_NAME_LENGTH)
    {
        os_memcpy(storage_info->name, name, MAX_NAME_LENGTH - 1);
    }
    else
    {
        os_memcpy(storage_info->name, name, length);
    }

    storage_info->name[MAX_NAME_LENGTH - 1] = '\0';
    LOGV("%s, %d, name:%s\n", __func__, __LINE__, storage_info->name);
    storage_info->img_format = format;
    storage_info->enable = STORAGE_STATE_ENABLED;
    ret = storage_app_task_send_msg(STORAGE_TASK_SAVE_START, (uint32_t)storage_info);
    if (ret != BK_OK)
    {
        storage_info->enable = STORAGE_STATE_DISABLED;
    }

    return ret;
}

bk_err_t storage_app_task_save_stop(void)
{
    bk_err_t ret = BK_FAIL;
    LOGD("%s, %d\n", __func__, __LINE__);

    storage_info_t *storage_info = s_storage_config;
    if (storage_info == NULL)
    {
        LOGW("%s, not open\n", __func__);
        return ret;
    }

    if (storage_info->enable == STORAGE_STATE_DISABLED)
    {
        LOGW("%s, not in saving state, please check...\n", __func__);
        return ret;
    }

    storage_info->enable = STORAGE_STATE_DISABLED;
    rtos_get_semaphore(&storage_info->sem, BEKEN_NEVER_TIMEOUT);
    LOGD("%s, %d complete\n", __func__, __LINE__);

    return ret;
}
