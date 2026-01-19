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

#pragma once

#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/uvc_camera_types.h>
#include "frame_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t param1;
    uint32_t param2;
} media_device_t;

bk_err_t camera_get_main_stream_handle(frame_list_node_t *node);
bk_err_t camera_open_handle(media_device_t *dev);
bk_err_t camera_close_handle(camera_handle_t *handle);
bk_err_t camera_uvc_register_device_info_cb_handle(camera_state_cb_t cb);
bk_err_t camera_dvp_h264_reset_handle(void);
bk_err_t camera_set_stream_state_handle(uint32_t state);

#ifdef __cplusplus
}
#endif
