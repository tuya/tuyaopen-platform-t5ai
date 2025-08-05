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

#ifdef __cplusplus
extern "C" {
#endif
#include <os/os.h>

typedef enum
{
	BK_GetOneSec = 0,
	BK_GetFiveSec,
	BK_GetTenSec,
} BK_CpuLoadTime;

/* In debug version, the corresponding debugging function is enabled by default */
#if CONFIG_DEBUG_VERSION

#define FREERTOS_TASK_RECORDER 1
#define FREERTOS_TASK_RECORDER_CNT 10

#endif

void rtos_dump_task_list(void);
void rtos_dump_stack_memory_usage(void);
void rtos_dump_task_runtime_stats(void);
void rtos_dump_task_backtrace(beken_thread_t *thread);
void rtos_dump_backtrace(void);
/**
 * @brief     	dump cpu percentage of tasks from past few seconds
 * 				if in smp version, this functino will dump tasks percentage of cpu0 and cpu1 respectively.
 * @param eTime CpuLoadTime eTime
 * 				pass period of cpu percentage to the function
 */
void rtos_dump_task_history_runtime_stats(BK_CpuLoadTime eTime);


#ifdef __cplusplus
}
#endif
