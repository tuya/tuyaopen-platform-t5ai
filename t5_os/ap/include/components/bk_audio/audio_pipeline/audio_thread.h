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


#ifndef _AUDIO_THREAD_H_
#define _AUDIO_THREAD_H_

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Create audio thread to the specified core.
 *
 *          Create audio thread to the core 0 when cpu work in smp system, if the specified core is not support.
 *          Create audio thread to the running core when cpu work in amp system, not depends on the specified core.
 *
 * @param[out] thread       Pointer to variable that will receive the thread handle (can be null)
 * @param[in] priority      A priority number.
 * @param[in] name          a text name for the thread (can be null)
 * @param[in] function      the main thread function
 * @param[in] stack_size    stack size for this thread
 * @param[in] arg           argument which will be passed to thread function
 * @param[in] core_id       the core of the task running
 *
 * @return
 *     - kNoErr success.
 *     - kGeneralErr fail
 */
bk_err_t audio_create_thread(beken_thread_t *thread, uint8_t priority, const char *name, beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg, int core_id);


#ifdef __cplusplus
}
#endif

#endif /*_AUDIO_THREAD_H_*/
