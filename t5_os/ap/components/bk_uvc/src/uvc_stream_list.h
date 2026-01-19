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

#pragma once

#include <common/bk_err.h>
#include <driver/int.h>
#include "bk_uvc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

uvc_node_t *uvc_camera_stream_node_init(uvc_stream_handle_t *handle);
void uvc_camera_stream_node_deinit(uvc_stream_handle_t *handle);
camera_param_t *uvc_camera_stream_node_get_by_port_and_format(uvc_stream_handle_t *handle, uint8_t port, uint16_t format);
camera_param_t *uvc_camera_stream_node_get_by_port_info(uvc_stream_handle_t *handle, bk_usb_hub_port_info *port_info);
bool uvc_camera_stream_check_all_uvc_closed(uvc_stream_handle_t *handle);

#ifdef __cplusplus
}
#endif