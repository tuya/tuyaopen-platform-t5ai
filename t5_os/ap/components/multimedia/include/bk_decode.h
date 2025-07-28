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
#include <driver/lcd_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_LCD_DECODE_ERR                 (BK_FAIL)
#define BK_LCD_DECODE_TIMEOUT             (BK_FAIL - 1)

typedef struct
{
	beken_semaphore_t hw_dec_sem;
	beken_semaphore_t sw_dec_sem;
	frame_buffer_t * decoder_frame;
	uint8_t result;
	uint8_t decode_err : 1;
	uint8_t decode_timeout : 1;
	uint8_t hw_state;
	uint8_t sw_state;
}bk_decode_t;

bk_err_t bk_hw_decode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame);
bk_err_t bk_hw_decode_init(void);
bk_err_t bk_hw_decode_deinit(void);

bk_err_t bk_sw_decode_init(media_decode_mode_t sw_dec_mode);
bk_err_t bk_sw_decode_deinit(media_decode_mode_t sw_dec_mode);

void bk_jpeg_dec_sw(uint32_t param);
void bk_jpeg_dec_sw_open(uint32_t param);

bk_err_t bk_sw_jpegdec_start(frame_buffer_t *frame, frame_buffer_t *dst_frame);
bk_err_t bk_sw_minor_jpegdec_start(frame_buffer_t *frame, frame_buffer_t *dst_frame);


#ifdef __cplusplus
}
#endif



