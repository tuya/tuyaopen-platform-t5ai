/**
* @file tkl_memory.h
* @brief Common process - adapter the semaphore api provide by OS
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_MEMORY_H__
#define __TKL_MEMORY_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
void *tkl_system_malloc(size_t size);

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return void
*/
void tkl_system_free(void* ptr);

/**
* @brief set memory
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
void *tkl_system_memset(void* src, int32_t ch, const size_t n);

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
void *tkl_system_memcpy(void* src, const void* dst, const size_t n);

/**
 * @brief Allocate and clear the memory
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 *
 * @return the memory address calloced
 */
void *tkl_system_calloc(size_t nitems, size_t size);

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return void
 */
void *tkl_system_realloc(void* ptr, size_t size);

/**
* @brief Get system free heap size
*
* @param none
*
* @return heap size
*/
int32_t tkl_system_get_free_heap_size(void);

int32_t tkl_system_get_minimum_heap_size(void);

int32_t tkl_system_memcmp(const void *str1, const void *str2, size_t n);


#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM==1)
/**
* @brief Alloc psram memory of system in psram
*
* @param[in] size: memory size
*
* @note ENABLE_EXT_RAM neeed define in tuyaos_kernel.config.
*
* @return the memory address malloced
*/
void *tkl_system_psram_malloc(size_t size);

/**
* @brief Free psram memory of system in psram
*
* @param[in] ptr: memory point
*
* @note ENABLE_EXT_RAM neeed define in tuyaos_kernel.config.
*
* @return void
*/
void tkl_system_psram_free(void* ptr);

/**
 * @brief Allocate and clear the memory in psram
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 *
 * @return the memory address calloced
 */
void *tkl_system_psram_calloc(size_t nitems, size_t size);

/**
 * @brief Re-allocate the memory in psram
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return void
 */
void *tkl_system_psram_realloc(void* ptr, size_t size);

/**
* @brief Get system free heap size in psram
*
* @param none
*
* @return heap size
*/
int32_t tkl_system_get_free_heap_size(void);

/**
* @brief Get free heap size in psram
*
* @param void
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
int32_t tkl_system_psram_get_free_heap_size(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

