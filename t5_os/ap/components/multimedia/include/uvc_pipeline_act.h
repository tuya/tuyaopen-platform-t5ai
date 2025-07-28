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


bk_err_t uvc_pipeline_init(void);

bk_err_t h264_jdec_pipeline_open(void);
bk_err_t h264_jdec_pipeline_close(void);
bk_err_t h264_jdec_pipeline_regenerate_idr_frame(void);

bk_err_t lcd_set_fmt(uint32_t fmt);
bk_err_t pipeline_set_rotate(media_rotate_t rotate);

bk_err_t lcd_jdec_pipeline_open(void);
bk_err_t lcd_jdec_pipeline_close(void);

void pipeline_mem_show(void);
void pipeline_mem_leak(void);

uint8_t *get_mux_sram_buffer(void);

#ifdef __cplusplus
}
#endif


