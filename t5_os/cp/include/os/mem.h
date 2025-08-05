#ifndef _MEM_PUB_H_
#define _MEM_PUB_H_

#include <stdarg.h>
#include <string.h>
#include <common/bk_typedef.h>
#include <common/sys_config.h>
#ifdef __cplusplus
extern"C" {
#endif

#define os_write_word(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define os_read_word(addr,val)                  val = *((volatile uint32_t *)(addr))
#define os_get_word(addr)                       *((volatile uint32_t *)(addr))

/// Memory type for mempool
typedef enum {
    HEAP_MEM_TYPE_DEFAULT, /**< Default heap memory type */
	HEAP_MEM_TYPE_SRAM,    /**< SRAM heap memory type */
	HEAP_MEM_TYPE_PSRAM,   /**< PSRAM heap memory type */
    MEM_TYPE_MAX = 0xf
} beken_mem_type_t;

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
static inline void os_memcpy_word(uint32_t *out, const uint32_t *in, uint32_t n)
{
    if (n == 0)
        return;

    if (((uintptr_t)in & 0x3) != 0)
    {
        memcpy(out, in, n);
        return;
    }

    uint32_t word_cnt = n >> 2;
    uint32_t i = 0;
    const uint32_t *aligned_in = in;

    // Manual unroll improves performance
    for (i = 0; i + 3 < word_cnt; i += 4)
    {
        os_write_word(out + i,     os_get_word(aligned_in + i));
        os_write_word(out + i + 1, os_get_word(aligned_in + i + 1));
        os_write_word(out + i + 2, os_get_word(aligned_in + i + 2));
        os_write_word(out + i + 3, os_get_word(aligned_in + i + 3));
    }

    // Dispose of the remaining word
    for (; i < word_cnt; i++)
    {
        os_write_word(out + i, os_get_word(aligned_in + i));
    }
}

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
__attribute__((section(".itcm_sec_code"))) static inline void os_memset_word(uint32_t *b, int32_t c, uint32_t n)
{
    // Note:
    // the word count == sizeof(buf)/sizeof(uint32_t)
    uint32_t word_cnt = n>>2;

    for(uint32_t i = 0; i < word_cnt; i++)
    {
        os_write_word((b + i), c);
    }
}

/** @brief   comparing the size of two memory blocks by specified length
  *
  * @note    this function is a wrapper for memcmp
  *
  * @param   s1     : the pointer of a memory block
  * @param   s2     : the pointer of other memory block
  * @param   n      : the specified length
  *
  * @return  the comarison results
  */
INT32 os_memcmp(const void *s1, const void *s2, UINT32 n);

/** @brief   move data from one memory location to another memory
  *          location with specified length
  *
  * @note    this function is a wrapper for memmove
  *
  * @param   out    : the pointer of source memory location
  * @param   in     : the pointer of destinated memory location
  * @param   n      : the specified length
  *
  */
void *os_memmove(void *out, const void *in, UINT32 n);

/** @brief   copies data from one memory location to another memory
  *          location with specified length
  *
  * @note    this function is a wrapper for memcpy
  *
  * @param   out    : the pointer of source memory location
  * @param   in     : the pointer of destinated memory location
  * @param   n      : the specified length
  *
  */
void *os_memcpy(void *out, const void *in, UINT32 n);

/** @brief   copies data from one memory location to another memory
  *          location with specified length
  *
  * @note    this function is a wrapper for memcpy
  *
  * @param   a    : the pointer of source memory location
  * @param   b    : the pointer of destinated memory location
  * @param   len  : the specified length
  *
  */
int os_memcmp_const(const void *a, const void *b, size_t len);

/** @brief   fill or copy a given value into the specified len bytes
  *          of the specified memory
  *
  * @note    this function is a wrapper for memset
  *
  * @param   b    : the pointer of the specified memory
  * @param   c    : the given value
  * @param   len  : the specified length
  *
  */
void *os_memset(void *b, int c, UINT32 len);

/** @brief   reallocate memory
  *
  * @note    this API cannot be called in interrupt context
  *
  * @param   ptr    : the pointer of the specified memory
  * @param   size   : the reallocate memory length
  *
  */
void *os_realloc(void *ptr, size_t size);

/** @brief   reallocate memory in psram
  *
  * @note    this API cannot be called in interrupt context
  *
  * @param   ptr    : the pointer of the specified memory
  * @param   size   : the reallocate memory length
  *
  */
void *bk_psram_realloc(void *ptr, size_t size);
void *psram_realloc(void *ptr, size_t size);

#if (CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG)
void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero);
void *os_sram_malloc_debug(const char *func_name, int line, size_t size, int need_zero);
void *psram_malloc_debug(const char *func_name, int line, size_t size, int need_zero);
void *os_free_debug(const char *func_name, int line, void *pv);
void os_dump_memory_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task);

#define os_malloc(size)   os_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 0)
#define os_sram_malloc(size)   os_sram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 0)
#define os_sram_calloc(a, b)  os_sram_malloc_debug((const char*)__FUNCTION__,__LINE__,(a * b), 1)
#define os_sram_zalloc(size)  os_sram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 1)
#define os_free(p)        os_free_debug((const char*)__FUNCTION__,__LINE__, p)
#define os_zalloc(size)   os_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 1)
#define psram_malloc(size)   psram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 0)
#define psram_zalloc(size)   psram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 1)
#else
/** @brief   request memory according to the specified size
  *
  * @note    This API cannot be called in interrupt context.
  *          When macro CONFIG_USE_PSRAM_HEAP_AT_SRAM_OOM is configured,
  *          if sram is insufficient,the system will try to request
  *          memory form psram
  *
  * @param   size   : requested memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *os_malloc(size_t size);

/** @brief   request memory according to the specified size
  *
  * @note    This API cannot be called in interrupt context.
  *          and only request memory in sram when call this API
  *
  * @param   size   : requested memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *os_sram_malloc(size_t size);

/** @brief   release preciously reuqested memory
  *
  * @note    This API cannot be called in interrupt context.
  *
  * @param   ptr   : the pointer of the handle previously requested
  *
  */
void  os_free(void *ptr);

/** @brief   request memory of a specified size and auotmatically initializes
  *          the allocated memory to zero
  *
  * @note    This API cannot be called in interrupt context.
  *          and this function is a wrapper for memset and malloc
  *
  * @param   size   : requested memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *os_zalloc(size_t size);

/** @brief   request memory in psram  according to the specified size
  *
  * @note    This API cannot be called in interrupt context.
  *
  * @param   size   : requested pspram memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *psram_malloc(size_t size);

/** @brief   request psram memory of a specified size and auotmatically initializes
  *          the allocated memory to zero
  *
  * @note    This API cannot be called in interrupt context.
  *          and this function is a wrapper for psram_malloc and memset
  *
  * @param   size   : requested psram memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *psram_zalloc(size_t size);

/** @brief   request memory in sram of a specified size and auotmatically initializes
  *          the allocated memory to zero
  *
  * @note    This API cannot be called in interrupt context.
  *          and this function is a wrapper for os_sram_malloc and memset
  *
  * @param   a   : the number of elements to allocate
  * @param   b   : the size of each element in bytes
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *os_sram_calloc(size_t a, size_t b);

/** @brief   request memory in sram of a specified size and auotmatically initializes
  *          the allocated memory to zero
  *
  * @note    This API cannot be called in interrupt context.
  *          and this function is a wrapper for os_sram_malloc and memset
  *
  * @param   size   : requested memory size in sram
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void *os_sram_zalloc(size_t size);
#endif

#define psram_free        os_free

/** @brief   get the current used psram size
  *
  * @return  psram size has been used
  *
  */
uint32_t bk_psram_heap_get_used_count(void);

/** @brief   get detailed information about the current psram in use
  *
  *
  */
void bk_psram_heap_get_used_state(void);
void bk_psram_heap_dump_data(void);

/** @brief   request memory according to the specified size
  *
  * @param   size   : requested memory size
  *
  * @return  if request success, return the pointer of the memory, otherwise
  *          NULL is returned
  *
  */
void* os_malloc_wifi_buffer(size_t size);

/** @brief   show current system memory information
  *
  *
  */
void os_show_memory_config_info(void);

/** @brief   Given a value, check if there is a memory block larger than that value in the free memory list.
  * 
  * @param   size   : check memory block size
  * 
  * @return  if request success, return 1, otherwise
  *          0 is returned
  * 
  */
uint32_t CheckBlockSizeValid(uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // _MEM_PUB_H_

// EOF
