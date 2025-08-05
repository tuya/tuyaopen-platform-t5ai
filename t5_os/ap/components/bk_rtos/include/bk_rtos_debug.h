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

typedef enum {
    AP_ENTER_CPU0_RESET_HANDLER         = 1,
    AP_ENTER_ENTRY_MAIN                 = 2,
    AP_ENTER_COMPONTENT_EARLY_INIT      = 3,
    AP_ENTER_DRIVER_EARLY_INIT          = 4,
    AP_EXIT_DRIVER_EARLY_INIT           = 5,
    AP_EXIT_COMPONTENT_EARLY_INIT       = 6,
    AP_ENTER_APP_MAIN_THREAD            = 7,
    AP_EXIT_APP_MAIN_THREAD             = 8,
    AP_ENTER_RTOS_START_SCHEDULER       = 9,
    AP_ENTER_CPU1_RESET_HANDLER         = 10,
    AP_ENTER_CPU1_OTHERCORE_START       = 11,
    AP_ENTER_BK_INIT                    = 12,
    AP_ENTER_AT_SERVER_INIT             = 13,
    AP_EXIT_AT_SERVER                   = 14,
    AP_ENTER_APP_WIFI_INIT              = 15,
    AP_ENTER_APP_BLE_INIT               = 16,
    AP_EXIT_BK_INIT                     = 17,

}ap_startup_type_t;

#if CONFIG_DEBUG_AP_STARTUP
extern volatile uint32_t  g_ap_startup_flag;
#endif

static inline void set_ap_startup_index(ap_startup_type_t index) {
#if CONFIG_DEBUG_AP_STARTUP
    g_ap_startup_flag = (uint32_t)index;
#endif
}


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
