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

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_dvp_camera_open(camera_handle_t *handle, media_camera_device_t *device);
bk_err_t bk_dvp_camera_close(camera_handle_t *handle);
bk_err_t bk_uvc_camera_open(camera_handle_t *handle, media_camera_device_t *device);
bk_err_t bk_uvc_camera_close(camera_handle_t *handle);
bk_err_t bk_net_camera_open(camera_handle_t *handle, media_camera_device_t *device);
bk_err_t bk_net_camera_close(camera_handle_t *handle);
frame_buffer_t *bk_net_camera_frame_buffer_malloc(camera_handle_t *handle);
bk_err_t bk_net_camera_frame_buffer_push(camera_handle_t *handle, frame_buffer_t *frame);
bk_err_t bk_net_camera_frame_buffer_free(camera_handle_t *handle, frame_buffer_t *frame);

#ifdef __cplusplus
}
#endif
