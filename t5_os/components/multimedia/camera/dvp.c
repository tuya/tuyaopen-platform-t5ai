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

#include "media_core.h"
#include "camera_act.h"
#include "lcd_act.h"
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

bk_err_t bk_dvp_camera_open(media_camera_device_t *device)
{
	dvp_camera_config_t config = {0};

	config.fb_init = frame_buffer_fb_init;
	config.fb_deinit = frame_buffer_fb_deinit;
	config.fb_clear = frame_buffer_fb_clear;
	config.fb_malloc = frame_buffer_fb_malloc;
	config.fb_complete = frame_buffer_fb_push;
	config.fb_free = frame_buffer_fb_direct_free;

	config.device = device;

	return bk_dvp_camera_driver_init(&config);
}

bk_err_t bk_dvp_camera_close(void)
{
	bk_dvp_camera_driver_deinit();
	return 0;
}
