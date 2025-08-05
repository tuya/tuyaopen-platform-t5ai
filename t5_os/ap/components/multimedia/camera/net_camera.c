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
#include <os/mem.h>
#include <components/log.h>


#include "camera_act.h"
#include "frame_buffer.h"
#include <driver/net_camera_types.h>

#define TAG "net_camera"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

bk_err_t bk_net_camera_open(camera_handle_t *handle, media_camera_device_t *device)
{
	bk_err_t ret = BK_OK;
	camera_config_t *output_handle = NULL;
	void *stream = NULL;

	if (*handle != NULL)
	{
		LOGW("%s, already open...\n", __func__);
		return ret;
	}

	stream = frame_buffer_list_node_init(device->port, device->type, device->format);
	if (stream == NULL)
	{
		LOGE("%s, init stream failed\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	output_handle = (camera_config_t *)os_malloc(sizeof(camera_config_t));
	if (output_handle == NULL)
	{
		LOGW("%s, output_handle malloc fail\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	output_handle->type = NET_CAMERA;
	output_handle->fps = device->fps;
	output_handle->id = 0;
	output_handle->image_format = device->format;
	output_handle->width = device->width;
	output_handle->height = device->height;
	output_handle->arg = (void *)stream;
	*handle = (camera_handle_t)output_handle;

	return ret;

error:

	if (stream)
	{
		frame_buffer_list_node_deinit(stream);
		stream = NULL;
	}

	return ret;
}

bk_err_t bk_net_camera_close(camera_handle_t *handle)
{
	if (*handle == NULL)
	{
		LOGW("%s, already closed...\n", __func__);
		return BK_OK;
	}

	camera_config_t *config = (camera_config_t *)*handle;

	frame_buffer_list_node_deinit(config->arg);

	os_free(config);
	*handle = NULL;

	return BK_OK;
}

frame_buffer_t *bk_net_camera_frame_buffer_malloc(camera_handle_t *handle)
{
	frame_buffer_t *frame = NULL;
	uint32_t size = 0;

	if (*handle == NULL)
	{
		LOGW("%s, already closed...\n", __func__);
		return frame;
	}

	camera_config_t *config = (camera_config_t *)*handle;

	if (config->image_format == IMAGE_MJPEG)
	{
		size = CONFIG_JPEG_FRAME_SIZE;
	}
	else if (config->image_format == IMAGE_H264 || config->image_format == IMAGE_H265)
	{
		size = CONFIG_H264_FRAME_SIZE;
	}
	else if (config->image_format == IMAGE_YUV)
	{
		size = config->width * config->height * 2;
	}
	else
	{
		LOGE("%s, %d, fmt:%d\n", __func__, __LINE__, config->image_format);
	}

	frame = frame_buffer_fb_malloc(config->arg, size);

	LOGV("%s, %d, %p\n", __func__, __LINE__, frame);

	return frame;
}

bk_err_t bk_net_camera_frame_buffer_push(camera_handle_t *handle, frame_buffer_t *frame)
{
	if (*handle == NULL)
	{
		LOGW("%s, net camera not open...\n", __func__);
		return BK_FAIL;
	}

	camera_config_t *config = (camera_config_t *)*handle;

	if (config->arg == NULL)
	{
		LOGW("%s, net camera stream not init, please check...\n", __func__);
		return BK_FAIL;
	}

	frame_buffer_fb_push(config->arg, frame);

	return BK_OK;
}

bk_err_t bk_net_camera_frame_buffer_free(camera_handle_t *handle, frame_buffer_t *frame)
{
	if (*handle == NULL)
	{
		LOGW("%s, net camera not open...\n", __func__);
		return BK_FAIL;
	}

	camera_config_t *config = (camera_config_t *)*handle;

	if (config->arg == NULL)
	{
		LOGW("%s, net camera stream not init, please check...\n", __func__);
		return BK_FAIL;
	}

	frame_buffer_fb_free(config->arg, frame);

	return BK_OK;
}


