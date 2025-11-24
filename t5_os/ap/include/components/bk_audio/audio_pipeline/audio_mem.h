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


#ifndef _AUDIO_MEM_H_
#define _AUDIO_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Malloc memory
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_malloc(uint32_t size);

/**
 * @brief   Free memory
 *
 * @param[in]  ptr  memory pointer
 *
 * @return
 *     - void
 */
void audio_free(void *ptr);

/**
 * @brief  Malloc memory
 *
 * @param[in]  nmemb   number of block
 * @param[in]  size    block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_calloc(uint32_t nmemb, uint32_t size);

/**
 * @brief   Print heap memory status
 *
 * @param[in]  tag    tag of log
 * @param[in]  line   line of log
 * @param[in]  func   function name of log
 *
 * @return
 *     - void
 */
void audio_mem_print(char *tag, int line, const char *func);

/**
 * @brief   Duplicate given string.
 *
 *          Allocate new memory, copy contents of given string into it and return the pointer
 *
 * @param[in]  str   String to be duplicated
 *
 * @return
 *     - Pointer to new malloc'ed string
 *     - NULL otherwise
 */
char *audio_strdup(const char *str);

/**
 * @brief   Free memory used by dma
 *
 * @param[in]  ptr  memory pointer
 *
 * @return
 *     - void
 */
void audio_dma_mem_free(void *ptr);

/**
 * @brief  Malloc memory used by dma
 *
 * @param[in]  nmemb   number of block
 * @param[in]  size    block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_dma_mem_calloc(uint32_t nmemb, uint32_t size);

#define AUDIO_MEM_SHOW(x)  audio_mem_print(x, __LINE__, __func__)

#ifdef __cplusplus
}
#endif

#endif /*_AUDIO_MEM_H_*/
