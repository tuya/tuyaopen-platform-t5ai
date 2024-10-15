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

#include "media_mailbox_list_util.h"
#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>

#ifdef __cplusplus
extern "C" {
#endif


bk_err_t lcd_rotate_deinit(void);
bk_err_t lcd_rotate_init(media_rotate_mode_t 	    rotate_mode);
bk_err_t lcd_hw_rotate_yuv2rgb565(frame_buffer_t *src, frame_buffer_t *dst, media_rotate_t rotate);

void lcd_calc_init(void);
void lcd_act_rotate_degree90(uint32_t param);
void lcd_act_vuyy_resize(uint32_t param);
bk_err_t lcd_sw_rotate(frame_buffer_t *src, frame_buffer_t *dst, uint8_t rotate);



#ifdef __cplusplus
}
#endif
