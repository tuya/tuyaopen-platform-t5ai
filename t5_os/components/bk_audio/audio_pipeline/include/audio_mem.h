// Copyright 2022-2023 Beken
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
 * @brief   Malloc memory in ADF
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_malloc(uint32_t size);

/**
 * @brief   Free memory in ADF
 *
 * @param[in]  ptr  memory pointer
 *
 * @return
 *     - void
 */
void audio_free(void *ptr);

/**
 * @brief  Malloc memory in ADF, if spi ram is enabled, it will malloc memory in the spi ram
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
 * @brief   Malloc memory in ADF, it will malloc to internal memory
 *
 * @param[in] nmemb   number of block
 * @param[in]  size   block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_calloc_inner(uint32_t nmemb, uint32_t size);

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
 * @brief  Reallocate memory in ADF, if spi ram is enabled, it will allocate memory in the spi ram
 *
 * @param[in]  ptr   memory pointer
 * @param[in]  size  block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *audio_realloc(void *ptr, uint32_t size);

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
 * @brief   SPI ram is enabled or not
 *
 * @return
 *     - true, spi ram is enabled
 *     - false, spi ram is not enabled
 */
bool audio_mem_spiram_is_enabled(void);

/**
 * @brief   Stack on external SPI ram is enabled or not
 *
 * @return
 *     - true, stack on spi ram is enabled
 *     - false, stack on spi ram is not enabled
 */
bool audio_mem_spiram_stack_is_enabled(void);

#define AUDIO_MEM_SHOW(x)  audio_mem_print(x, __LINE__, __func__)

#ifdef __cplusplus
}
#endif

#endif /*_AUDIO_MEM_H_*/
