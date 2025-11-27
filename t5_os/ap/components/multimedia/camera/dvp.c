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
#include "img_service.h"
#include "storage_act.h"
#include "media_evt.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "frame_buffer.h"

#define TAG "dvp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

bk_err_t bk_dvp_frame_buffer_list_node_deinit(void *node)
{
    frame_buffer_list_node_deinit((frame_list_node_t *)node);
    return BK_OK;
}

bk_err_t bk_dvp_frame_buffer_list_node_clear(void *node)
{
    frame_buffer_list_node_clear((frame_list_node_t *)node);
    return BK_OK;
}

frame_buffer_t *bk_dvp_frame_buffer_fb_malloc(uint16_t image_format, void *node, uint32_t size)
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

bk_err_t bk_dvp_frame_buffer_fb_free(uint16_t image_format, void *node, frame_buffer_t *frame)
{
    if (image_format & IMAGE_YUV)
    {
        LOGD("%s, %d\n", __func__, __LINE__);
        frame_buffer_display_free(frame);
    }
    else
    {
        frame_buffer_fb_free((frame_list_node_t *)node, frame);
    }

    return BK_OK;
}

bk_err_t bk_dvp_frame_buffer_fb_complete(uint16_t image_format, void *node, frame_buffer_t *frame)
{
    bk_err_t ret = BK_FAIL;
    if (image_format & IMAGE_YUV)
    {
        img_msg_t msg =
        {
            .event = IMG_DISPLAY_REQUEST,
            .ptr = frame,
        };
        ret = bk_img_msg_send(&msg);
        if (ret != BK_OK)
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

bk_dvp_callback_t dvp_frame_cb = {
    .frame_init = frame_buffer_list_node_init,
    .frame_deinit = bk_dvp_frame_buffer_list_node_deinit,
    .frame_clear = bk_dvp_frame_buffer_list_node_clear,
    .frame_malloc = bk_dvp_frame_buffer_fb_malloc,
    .frame_complete = bk_dvp_frame_buffer_fb_complete,
    .frame_free = bk_dvp_frame_buffer_fb_free,
};

bk_err_t bk_dvp_camera_open(camera_handle_t *handle, media_camera_device_t *device)
{
    dvp_config_t config = {0};

    config.width = device->width;
    config.height = device->height;
    config.fps = device->fps;
    config.rotate = device->rotate;
    config.img_format = device->format;
    config.drop_num = 0;

    return bk_dvp_init(handle, &config, &dvp_frame_cb);
}

bk_err_t bk_dvp_camera_close(camera_handle_t *handle)
{
	bk_dvp_deinit(handle);

	return BK_OK;
}
