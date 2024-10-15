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

#ifndef _AUDIO_COPROCESS_H_
#define _AUDIO_COPROCESS_H_

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "media_mailbox_list_util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	AUDIO_ELEMENT_FATFS,
} audio_element_module_t;

typedef struct {
	int task_stack;     /*!< Task stack size */
	int task_prio;      /*!< Task priority (based on freeRTOS priority) */
} audio_element_coprocess_cfg_t;

typedef struct {
	beken_thread_t audio_element_coprocess_thread_hdl;
	beken_queue_t audio_element_coprocess_msg_que;
	beken_semaphore_t sem;
	void *audio_element_data;
} audio_element_coprocess_ctx_t;

typedef struct {
	uint32_t event;
	uint32_t param;
} audio_msg_t;

bk_err_t audio_coprocess_task_init(void);
bk_err_t audio_coprocess_send_msg(audio_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif

