// Copyright 2025-2026 Beken
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


#include <os/os.h>

#define TAG "AUD_TRD"


bk_err_t audio_create_thread(beken_thread_t *thread, uint8_t priority, const char *name, beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg, int core_id)
{
#if (CONFIG_ADK_BIND_TASK_TO_CORE)

    switch (core_id)
    {
        case 0:
            return rtos_core0_create_thread(thread, priority, name, function, stack_size, arg);

        case 1:
#if (CONFIG_CPU_CNT > 1)
            return rtos_core1_create_thread(thread, priority, name, function, stack_size, arg);
#else
            BK_LOGW(TAG, "%s, %d, not support core 1, CONFIG_CPU_CNT: %d, create task: %s to core 0\n", __func__, __LINE__, CONFIG_CPU_CNT, name);
            return rtos_core0_create_thread(thread, priority, name, function, stack_size, arg);
#endif

        default:
            BK_LOGW(TAG, "%s, %d, not support core: %d, CONFIG_CPU_CNT: %d, create task: %s to core 0\n", __func__, __LINE__, core_id, CONFIG_CPU_CNT, name);
            return rtos_core0_create_thread(thread, priority, name, function, stack_size, arg);
    }

#else
    return rtos_create_thread(thread, priority, name, function, stack_size, arg);
#endif
}

