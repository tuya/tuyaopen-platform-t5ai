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

#pragma once

#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio_asr_service_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aud_asr *aud_asr_handle_t;

typedef struct {
    asr_handle_t asr_handle;                                                    /*!< voice handle */
    uint32_t max_read_size;                                                         /*!< the max size of data read from voice handle, used in voice_read_callback */
//int (*asr_read_callback)(unsigned char *data, unsigned int len, void *args);  /*!< call this callback when avlid data has been read */
    void *args;                                                                     /*!< the pravate parameter of callback */
    int task_stack;                                                                 /*!< Task stack size */
    int task_core;                                                                  /*!< Task running in core (0 or 1) */
    int task_prio;                                                                  /*!< Task priority (based on freeRTOS priority) */
    audio_mem_type_t mem_type;                                                      /*!< memory type used, sram or psram */
} aud_asr_cfg_t;

#define AUDIO_ASR_TASK_PRIO    (BEKEN_DEFAULT_WORKER_PRIORITY - 1)

#define AUDIO_ASR_CFG_DEFAULT() {                   \
    .asr_handle = NULL,                             \
    .max_read_size = 960,                           \
    .args = NULL,                                   \
    .task_stack = 2048,                             \
    .task_core = 0,                                 \
    .task_prio = AUDIO_ASR_TASK_PRIO,               \
    .mem_type = AUDIO_MEM_TYPE_PSRAM,               \
}


#ifdef __cplusplus
}
#endif

