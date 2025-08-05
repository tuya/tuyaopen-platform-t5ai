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

typedef void (*decode_callback_t)(uint8_t ret);

typedef struct {
	frame_buffer_t *in_frame;
	frame_buffer_t *out_frame;
	decode_callback_t cb;
} media_software_decode_info_t;

bk_err_t software_decode_task_send_msg(uint32_t type, uint32_t param);
bk_err_t software_decode_single_frame(frame_buffer_t *in_frame,
										frame_buffer_t *out_frame,
										uint8_t scale,
										JD_FORMAT_OUTPUT format,
										media_rotate_t rotate_angle,
										uint8_t *work_buf,
										uint8_t *rotate_buf);
void software_decode_set_rotate(uint8_t rotate_angle);
bool check_software_decode_task_is_open(void);
bk_err_t software_decode_task_open();
bk_err_t software_decode_task_close();

//CONFIG_SOC_SMP
bk_err_t software_decode_minor_task_send_msg(uint32_t type, uint32_t param);
void software_decode_minor_set_rotate(uint8_t rotate_angle);
bool check_software_decode_minor_task_is_open(void);
bk_err_t software_decode_minor_task_open();
bk_err_t software_decode_minor_task_close();

#ifdef __cplusplus
}
#endif

