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

typedef struct
{
	uint32_t flash_image_addr;
	uint32_t flasg_img_length;
} storage_flash_t;

bk_err_t storage_app_set_frame_name(char *name);
bk_err_t bk_sdcard_read_to_mem(char *filename, uint32_t *paddr, uint32_t *total_len);
bk_err_t bk_mem_save_to_sdcard(char *filename, uint8_t *paddr, uint32_t total_len);
bk_err_t bk_mem_save_to_flash(char *filename, uint8_t *paddr, uint32_t total_len, storage_flash_t **info);
bk_err_t bk_mem_append_save_to_sdcard(char *filename, uint8_t *paddr, uint32_t total_len);
bk_err_t bk_read_sdcard_file_length(char *filename);

bk_err_t storage_app_task_init(frame_cb_t cb);
bk_err_t storage_app_task_deinit(void);
bk_err_t storage_app_task_capture(image_format_t format, char *name);
bk_err_t storage_app_task_save_start(image_format_t format, char *name);
bk_err_t storage_app_task_save_stop(void);

#ifdef __cplusplus
}
#endif
