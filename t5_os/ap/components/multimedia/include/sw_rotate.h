// Copyright 2020-2023 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rotate_callback_t)(uint8_t ret);

typedef struct
{
	frame_buffer_t *src_yuv;
	frame_buffer_t *dst_yuv;
	media_rotate_t rot_angle;
	uint32_t start_line;
	uint32_t end_line;
	rotate_callback_t cb;
} media_software_rotate_info_t;


bk_err_t software_rotate_task_send_msg(uint32_t type, uint32_t param);
bool check_software_rotate_task_is_open(void);
bk_err_t software_rotate_task_open(void);
bk_err_t software_rotate_task_close(void);

#ifdef __cplusplus
}
#endif

