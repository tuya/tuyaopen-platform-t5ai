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


#ifndef _FATFS_COPROCESS_H_
#define _FATFS_COPROCESS_H_

#include <os/os.h>
#include "fatfs_act.h"
#include "audio_coprocess.h"
#include "audio_mem.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
	fatfs_event_t op;
	void *param;
} fatfs_coprocess_msg_t;



/**
 * @brief      Create a task in cpu0 as fatfs coprocessor to excute file operations, such as open,
 *             write, read and so on.
 *
 * @param[in]      setup_cfg  The configuration
 *
 * @return         The coprocess task context
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_coprocess_ctx_t * fatfs_coprocess_create(audio_element_coprocess_cfg_t *setup_cfg);


#ifdef __cplusplus
}
#endif

#endif
