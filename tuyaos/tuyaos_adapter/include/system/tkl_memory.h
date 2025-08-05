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
VOID_T *tkl_system_malloc(SIZE_T size);

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return VOID_T
*/
VOID_T tkl_system_free(VOID_T* ptr);

/**
* @brief set memory
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memset(VOID_T* src, INT_T ch, CONST SIZE_T n);

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memcpy(VOID_T* src, CONST VOID_T* dst, CONST SIZE_T n);

/**
 * @brief Allocate and clear the memory
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 *
 * @return the memory address calloced
 */
VOID_T *tkl_system_calloc(size_t nitems, size_t size);

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return VOID_T
 */
VOID_T *tkl_system_realloc(VOID_T* ptr, size_t size);

/**
* @brief Get system free heap size
*
* @param none
*
* @return heap size
*/
INT_T tkl_system_get_free_heap_size(VOID_T);

INT_T tkl_system_get_minimum_heap_size(VOID_T);

INT_T tkl_system_memcmp(CONST VOID_T *str1, CONST VOID_T *str2, SIZE_T n);


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
VOID_T *tkl_system_psram_malloc(SIZE_T size);

/**
* @brief Free psram memory of system in psram
*
* @param[in] ptr: memory point
*
* @note ENABLE_EXT_RAM neeed define in tuyaos_kernel.config.
*
* @return VOID_T
*/
VOID_T tkl_system_psram_free(VOID_T* ptr);

/**
 * @brief Allocate and clear the memory in psram
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 *
 * @return the memory address calloced
 */
VOID_T *tkl_system_psram_calloc(size_t nitems, size_t size);

/**
 * @brief Re-allocate the memory in psram
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return VOID_T
 */
VOID_T *tkl_system_psram_realloc(VOID_T* ptr, size_t size);

/**
* @brief Get system free heap size in psram
*
* @param none
*
* @return heap size
*/
INT_T tkl_system_get_free_heap_size(VOID_T);

/**
* @brief Get free heap size in psram
*
* @param VOID
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
INT_T tkl_system_psram_get_free_heap_size(VOID_T);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

