/**
 * @file tkl_memory.c
 * @brief the default weak implements of tuya hal memory, this implement only used when OS=linux
 * @version 0.1
 * @date 2020-05-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "sdkconfig.h"
#include "tkl_memory.h"
#include <os/mem.h>

#define CONFIG_HAVE_PSRAM 1
extern VOID_T *tkl_system_calloc(size_t nitems, size_t size);
extern VOID_T *tkl_system_realloc(VOID_T* ptr, size_t size);
extern VOID_T *tkl_system_psram_malloc(CONST SIZE_T size);
extern VOID_T tkl_system_psram_free(VOID_T* ptr);

extern void bk_printf(const char *fmt, ...);

STATIC BOOL_T s_psram_malloc_force = 0;

VOID_T tkl_system_psram_malloc_force_set(BOOL_T enable)
{
    s_psram_malloc_force = enable;
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
VOID_T* tkl_system_malloc(CONST SIZE_T size)
{
    // if (size > 4096) {
    //     //bk_printf("tkl_system_malloc big memory, size(%d), caller %p\r\n", size, __builtin_return_address(0));
    // }

    VOID_T* ptr = NULL;
    if (size >= 2*1024) {
        ptr = psram_malloc(size);
    } else {
        ptr = os_malloc(size);
    }
    if(NULL == ptr) {
        bk_printf("tkl_system_malloc failed, size(%d)!\r\n", size);
    }

    return ptr;
}

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
VOID_T tkl_system_free(VOID_T* ptr)
{
    os_free(ptr);
}

/**
* @brief set memory
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memset(VOID_T* src, INT_T ch, CONST SIZE_T n)
{
    return os_memset(src, ch, n);
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memcpy(VOID_T* src, CONST VOID_T* dst, CONST SIZE_T n)
{
    return os_memcpy(src, dst, n);
}

/**
 * @brief Allocate and clear the memory
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 */
VOID_T *tkl_system_calloc(size_t nitems, size_t size)
{
    if (size && nitems > (~(size_t) 0) / size)
        return NULL;

    void *ptr =  os_zalloc(nitems * size);
    if (ptr == NULL) {
        bk_printf("tkl_system_calloc failed, total_size(%d)! nitems = %d size = %d\r\n", nitems * size,nitems,size);
    }
    return ptr;
}

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 */
VOID_T *tkl_system_realloc(VOID_T* ptr, size_t size)
{
    return os_realloc(ptr, size);
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
VOID_T* tkl_system_psram_malloc(CONST SIZE_T size)
{
#if CONFIG_HAVE_PSRAM
    VOID_T* ptr = psram_malloc(size);
    if(NULL == ptr) {
        bk_printf("tkl_psram_malloc failed, size(%d)!\r\n", size);
    }
    return ptr;
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
VOID_T tkl_system_psram_free(VOID_T* ptr)
{
#if CONFIG_HAVE_PSRAM
    psram_free(ptr);
#else
    bk_printf("not support %s\r\n", __func__);
#endif // CONFIG_HAVE_PSRAM
}

/**
 * @brief Allocate and clear the memory in psram
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 */
VOID_T *tkl_system_psram_calloc(size_t nitems, size_t size)
{
#if CONFIG_HAVE_PSRAM
    if (size && nitems > (~(size_t) 0) / size)
        return NULL;

    void *ptr = psram_zalloc(nitems * size);
    if (ptr == NULL) {
        bk_printf("tkl_system_calloc failed, total_size(%d)! nitems = %d size = %d\r\n", nitems * size,nitems,size);
    }
    return ptr;
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
 * @brief Re-allocate the memory in psram
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 */
VOID_T *tkl_system_psram_realloc(VOID_T* ptr, size_t size)
{
#if CONFIG_HAVE_PSRAM
    return bk_psram_realloc(ptr, size);
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
* @brief Get free heap size in psram
*
* @param VOID
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
INT_T tkl_system_psram_get_free_heap_size(VOID_T)
{
#if CONFIG_HAVE_PSRAM
    return (INT_T)xPortGetPsramFreeHeapSize();
#else
    bk_printf("not support %s\r\n", __func__);
    return 0;
#endif // CONFIG_HAVE_PSRAM
}

