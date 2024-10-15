// Copyright 2023-2024 Beken
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


#ifndef _FATFS_ACT_H_
#define _FATFS_ACT_H_

#include <os/os.h>
#if CONFIG_ASDF_WORK_CPU1
#include "media_evt.h"
#include "media_mailbox_list_util.h"
#include "audio_mailbox.h"
#endif
#if CONFIG_SYS_CPU0
#include "ff.h"
#include "diskio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//#define AUD_EVENT    3
//#define MEDIA_EVT_BIT    16


#if CONFIG_ASDF_WORK_CPU1
typedef enum {
	EVENT_FATFS_INIT = (AUD_EVENT << MEDIA_EVT_BIT),
	EVENT_FATFS_DEINIT,
	EVENT_FATFS_OPEN,
	EVENT_FATFS_CLOSE,
	EVENT_FATFS_READ,
	EVENT_FATFS_WRITE,
	EVENT_FATFS_SIZE,
	EVENT_FATFS_LSEEK,
} fatfs_event_t;


typedef struct {
	int task_stack;     /*!< Task stack size */
	int task_prio;      /*!< Task priority (based on freeRTOS priority) */
} fatfs_init_param_t;

typedef struct {
	void *fp;
	char* path;
	uint8_t mode;
} fatfs_open_param_t;

typedef struct {
	void *fp;
	uint32_t size;
} fatfs_fp_param_t;

typedef struct {
	void *fp;
	void *buff;
	uint32_t len;        /*!< length need to read or write */
	uint32_t result;     /*!< length actually read or write */
} fatfs_rw_param_t;

typedef struct {
	void* fp;
	uint64 ofs; 	/* File pointer from top of file */
	uint32_t result;
} fatfs_lseek_param_t;
#endif

bk_err_t fatfs_init(int task_stack, int task_prio, void **coprocess_hdl, char *tag);
bk_err_t fatfs_deinit(void *coprocess_hdl, char *tag);
bk_err_t fatfs_open(void **fp, char* path, uint8_t mode, void *coprocess_hdl, char *tag);
bk_err_t fatfs_close(void *fp, void *coprocess_hdl, char *tag);
int fatfs_read(void *fp, void* buff, uint64_t len, void *coprocess_hdl, char *tag);
int fatfs_write(void *fp, void* buff, uint64_t len, void *coprocess_hdl, char *tag);
bk_err_t fatfs_size(void *fp, void *coprocess_hdl, char *tag);
bk_err_t fatfs_lseek(void *fp, uint64_t ofs, void *coprocess_hdl, char *tag);

#ifdef __cplusplus
}
#endif

#endif
