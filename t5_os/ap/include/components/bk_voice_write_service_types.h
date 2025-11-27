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

#include <components/bk_voice_service_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct voice_write *voice_write_handle_t;

typedef struct {
    voice_handle_t voice_handle;                                                    /*!< voice handle */
    uint32_t start_threshold;
    uint32_t pause_threshold;
    int task_stack;                                                                 /*!< Task stack size */
    int task_core;                                                                  /*!< Task running in core (0 or 1) */
    int task_prio;                                                                  /*!< Task priority (based on freeRTOS priority) */
    audio_mem_type_t mem_type;                                                      /*!< memory type used, sram, psram or audio heap */
    audio_buf_type_t write_buf_type;                                                /*!< write buffer type used, frame buffer or ring buffer */
    int node_size;                                                                  /*!< frame buffer node size or ring buffer pool size*/
    int node_num;                                                                   /*!< frame buffer node number or 1 for ring buffer */
} voice_write_cfg_t;

#define VOICE_WRITE_TASK_PRIO           (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define VOICE_WRITE_POOL_SIZE           (3200)
#define VOICE_WRITE_START_THRESHOLD     (1280)
#define VOICE_WRITE_PAUSE_THRESHOLD     (0)

#define VOICE_WRITE_CFG_DEFAULT() {                 \
    .voice_handle = NULL,                           \
    .start_threshold = VOICE_WRITE_START_THRESHOLD, \
    .pause_threshold = VOICE_WRITE_PAUSE_THRESHOLD, \
    .task_stack = 2048,                             \
    .task_core = 0,                                 \
    .task_prio = VOICE_READ_TASK_PRIO,              \
    .mem_type = AUDIO_MEM_TYPE_PSRAM,               \
    .write_buf_type = AUDIO_BUF_TYPE_RB,            \
    .node_size = VOICE_WRITE_POOL_SIZE,             \
    .node_num = 1,                                  \
}


#ifdef __cplusplus
}
#endif

