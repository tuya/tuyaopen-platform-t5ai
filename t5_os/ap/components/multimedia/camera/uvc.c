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
#include <components/log.h>

#include "camera_act.h"
#include "media_evt.h"
#include "media_app.h"

#include <os/mem.h>

#include <driver/uvc_camera.h>

#include "frame_buffer.h"

#define TAG "uvc"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_USBH_UVC

bk_err_t bk_uvc_frame_buffer_list_node_deinit(void *node)
{
    frame_buffer_list_node_deinit((frame_list_node_t *)node);
    return BK_OK;
}

bk_err_t bk_uvc_frame_buffer_list_node_clear(void *node)
{
    frame_buffer_list_node_clear((frame_list_node_t *)node);
    return BK_OK;
}

frame_buffer_t *bk_uvc_frame_buffer_fb_malloc(uint16_t image_format, void *node, uint32_t size)
{
    frame_buffer_t *frame = NULL;

    if (image_format & IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        frame = frame_buffer_fb_malloc((frame_list_node_t *)node, size);
    }

    return frame;
}

bk_err_t bk_uvc_frame_buffer_fb_free(uint16_t image_format, void *node, frame_buffer_t *frame)
{
    if (image_format & IMAGE_YUV)
    {
        frame_buffer_display_free(frame);
    }
    else
    {
        frame_buffer_fb_free((frame_list_node_t *)node, frame);
    }

    return BK_OK;
}

bk_err_t bk_uvc_frame_buffer_fb_complete(uint16_t image_format, void *node, frame_buffer_t *frame)
{
    if (image_format & IMAGE_YUV)
    {
        extern bk_err_t lcd_display_task_send_msg(uint8_t type, uint32_t param);
        if (lcd_display_task_send_msg(/*DISPLAY_FRAME_REQUEST*/0, (uint32_t)frame) != BK_OK)
        {
            frame_buffer_display_free(frame);
        }
    }
    else
    {
        frame_buffer_fb_push((frame_list_node_t *)node, frame);
    }

    return BK_OK;
}

bk_uvc_callback_t uvc_frame_cb = {
    .frame_init = frame_buffer_list_node_init,
    .frame_deinit = bk_uvc_frame_buffer_list_node_deinit,
    .frame_clear = bk_uvc_frame_buffer_list_node_clear,
    .frame_malloc = bk_uvc_frame_buffer_fb_malloc,
    .frame_complete = bk_uvc_frame_buffer_fb_complete,
    .frame_free = bk_uvc_frame_buffer_fb_free,
};

bk_err_t bk_uvc_camera_open(camera_handle_t *handle, media_camera_device_t *device)
{
    int ret = BK_FAIL;
    uvc_config_t config = {0};
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUXS, 0, 0);
    // step 1: wait uvc connect ok
    ret = bk_uvc_power_on(device->format, 4000);
    if (ret != BK_OK)
    {
        LOGW("%s, uvc connect failed\n", __func__);
        return ret;
    }

    // step 2 : open
    config.port = device->port;
    config.width = device->width;
    config.height = device->height;
    config.fps = device->fps;
    config.img_format = device->format;

    ret = bk_uvc_init(handle, &config, &uvc_frame_cb);
    if (ret != BK_OK)
    {
        bk_uvc_power_off();
    }

    LOGD("%s, complete, %d\r\n", __func__, ret);

    return ret;
}

//! tuya-linch 
bk_err_t bk_uvc_camera_start(camera_handle_t *handle, media_camera_device_t *device)
{
    int ret = BK_FAIL;
    uvc_config_t config = {0};
    
    config.port = device->port;
    config.width = device->width;
    config.height = device->height;
    config.fps = device->fps;
    config.img_format = device->format;

    ret = bk_uvc_set_start(handle, &config);
    
    return ret;
}

bk_err_t bk_uvc_camera_stop(camera_handle_t *handle)
{
    int ret = BK_FAIL;
    ret = bk_uvc_set_stop(handle);
    return ret;
}

bk_err_t bk_uvc_camera_close(camera_handle_t *handle)
{
    int ret = BK_OK;

    ret = bk_uvc_deinit(handle);

    bk_uvc_power_off();
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUXS, 1, 0);
    return ret;
}

#endif
