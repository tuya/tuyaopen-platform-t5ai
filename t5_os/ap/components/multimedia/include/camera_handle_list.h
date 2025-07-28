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

#include <driver/media_types.h>
#include <bk_list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	LIST_HEADER_T list;
	camera_handle_t cam_handle;
	uint16_t id;
	uint16_t format;
} media_camera_node_t;

bk_err_t bk_camera_handle_list_init(void *param);
bk_err_t bk_camera_handle_list_deinit(void);

/*create new camera handle node, and push to list*/
media_camera_node_t *bk_camera_handle_node_init(uint16_t id, uint16_t format);

/*delete handle rlease node from list, and free mem*/
void bk_camera_handle_node_deinit(camera_handle_t handle);

/*get camera handle node from list by id and format, but not delete from list*/
camera_handle_t bk_camera_handle_node_get_by_id_and_fomat(uint16_t id, uint16_t format);

/*get camera handle node from list, but not delete from list*/
camera_handle_t bk_camera_handle_node_pop(void);

#ifdef __cplusplus
}
#endif
