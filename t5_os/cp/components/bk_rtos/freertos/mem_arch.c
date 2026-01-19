#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include <string.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <os/os.h>
#include "bk_uart.h"
#include <os/mem.h>
#include "arch_interrupt.h"

INT32 os_memcmp(const void *s1, const void *s2, UINT32 n)
{
	return memcmp(s1, s2, (unsigned int)n);
}

void *os_memmove(void *out, const void *in, UINT32 n)
{
	configASSERT(NULL != in);
	return memmove(out, in, n);
}

void *os_memcpy(void *out, const void *in, UINT32 n)
{
    if (out == NULL || in == NULL || n == 0)
        return out;

    if ((((uintptr_t)out | (uintptr_t)in | n) & 0x3) == 0)
    {
        os_memcpy_word((uint32_t *)out, (const uint32_t *)in, n);
        return out;
    }
    else
    {
        extern void *memcpy(void *dest, const void *src, size_t n);
        return memcpy(out, in, n);
    }
}

void *os_memset(void *b, int c, UINT32 len)
{
	return (void *)memset(b, c, (unsigned int)len);
}

void *os_realloc(void *ptr, size_t size)
{
#ifdef FIX_REALLOC_ISSUE
	return pvPortRealloc(ptr, size);
#else
	void *tmp;

	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGE(NULL,"realloc_risk\r\n");
		BK_ASSERT(false);
	}

	tmp = (void *)pvPortMalloc(size);
	if (tmp && ptr) {
		os_memcpy(tmp, ptr, size);
		vPortFree(ptr);
	}

	return tmp;
#endif
}


void *psram_realloc(void *ptr, size_t size)
{
	void *tmp;

	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGE(NULL,"psram_realloc_risk\r\n");
		BK_ASSERT(false);
	}

	if (size & 0x3) {
		size = ((size >> 2) + 1) << 2;
	}

	tmp = psram_malloc(size);
	if (tmp && ptr) {
		os_memcpy_word((uint32_t *)tmp, (uint32_t *)ptr, size);
		os_free((void *)ptr);
	}

	return tmp;
}

void *bk_psram_realloc(void *ptr, size_t size)
{
	return psram_realloc(ptr, size);
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
	return memcmp(a, b, len);
}

#if !CONFIG_MALLOC_STATIS && !CONFIG_MEM_DEBUG
void *os_malloc(size_t size)
{
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGE(NULL,"malloc_risk\r\n");
		BK_ASSERT(false);
	}

	return (void *)pvPortMalloc(size);
}

void *os_sram_malloc(size_t size)
{
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
        BK_LOGE(NULL,"malloc_risk\r\n");
        BK_ASSERT(false);
    }		

    return (void *)bk_wrap_sram_malloc(size);
}

void *os_zalloc(size_t size)
{
	void *n = (void *)os_malloc(size);

	if (n)
		os_memset(n, 0, size);
	return n;
}

void *os_sram_zalloc(size_t size)
{
	void *n = (void *)os_sram_malloc(size);

	if (n)
		os_memset(n, 0, size);
	return n;
}

void *os_sram_calloc(size_t a, size_t b)
{
	void *pvReturn;

	pvReturn = os_sram_malloc(a * b);
	if (pvReturn)
    {
        os_memset(pvReturn, 0, a*b);
    }

	return pvReturn;
}

void os_free(void *ptr)
{
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGE(NULL,"free_risk\r\n");
		BK_ASSERT(false);
	}

	if (ptr)
		vPortFree(ptr);
}

void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
	if (need_zero) {
		return (void *)os_zalloc(size);
	}
	return (void *)os_malloc(size);
}

void *os_sram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
	if (need_zero) {
		return (void *)os_sram_zalloc(size);
	}
	return (void *)os_sram_malloc(size);
}

void *os_free_debug(const char *func_name, int line, void *pv)
{
	os_free(pv);
	return NULL;
}

void *psram_zalloc(size_t size)
{
	void *n = (void *)psram_malloc(size);

	if (n)
		os_memset(n, 0, size);
	return n;
}

#else

extern void xPortDumpMemStats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task);
extern void *psram_malloc_cm(const char *func_name, int line, size_t size, int need_zero);

void *os_sram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
   if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_DUMP_OUT("Error: [%s] line(%d). malloc_risk.\r\n", func_name, line);
        BK_ASSERT(false);
	}
	return bk_wrap_sram_malloc_cm(func_name, line, size, need_zero); 
}

void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_DUMP_OUT("Error: [%s] line(%d). malloc_risk.\r\n", func_name, line);
		BK_ASSERT(false);
	}
	return pvPortMalloc_cm(func_name, line, size, need_zero);
}

void *psram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
#if CONFIG_PSRAM_AS_SYS_MEMORY
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_DUMP_OUT("Error: [%s] line(%d). malloc_risk.\r\n", func_name, line);
		BK_ASSERT(false);
	}
	return psram_malloc_cm(func_name, line, size, need_zero);
#else
	return NULL;
#endif
}

void *os_free_debug(const char *func_name, int line, void *pv)
{
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_DUMP_OUT("Error: [%s] line(%d). free_risk.\r\n", func_name, line);
		BK_ASSERT(false);
	}
	return vPortFree_cm(func_name, line, pv);
}

void *os_realloc_debug(const char *func_name, int line, void *ptr, size_t size, int need_zero)
{
#ifdef FIX_REALLOC_ISSUE
    return pvPortRealloc(ptr, size);
#else
    void *tmp;

    if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
        BK_DUMP_OUT("Error: [%s] line(%d). realloc_risk!\r\n", func_name, line);
        BK_ASSERT(false);
    }
        
    tmp = (void *)pvPortMalloc_cm(func_name, line, size, need_zero);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        vPortFree_cm(func_name, line, ptr);
    }

    return tmp;
#endif
}

/****************************************************
*    Build CONFIG_MEM_DEBUG version                 *
*    Adapt third lib build with release SDK begin   *
*****************************************************/
#undef os_malloc
#undef os_free
#undef os_zalloc
#undef os_sram_calloc
#undef os_sram_zalloc

void *os_malloc(size_t size)
{
	return (void *)os_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 0);
}

void *os_zalloc(size_t size)
{
	return (void *)os_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 1);
}

void os_free(void *ptr)
{
        os_free_debug((const char*)__FUNCTION__,__LINE__, ptr);
}

void * os_sram_calloc(size_t a, size_t b)
{
	return os_sram_malloc_debug((const char*)__FUNCTION__,__LINE__, (a * b), 1);
}

void * os_sram_zalloc(size_t size)
{
	return os_sram_malloc_debug((const char*)__FUNCTION__,__LINE__, size, 1);
}

#undef psram_malloc
#undef psram_zalloc

void *psram_malloc(size_t size)
{
    return psram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 0);
}

void *psram_zalloc(size_t size)
{
    return psram_malloc_debug((const char*)__FUNCTION__,__LINE__,size, 1);
}

#undef pvPortMalloc
#undef vPortFree

void *pvPortMalloc(size_t xWantedSize) {
	return (void *)psram_malloc_debug((const char*)__FUNCTION__, __LINE__, xWantedSize, 0);
}

void vPortFree(void *pv) {
	os_free_debug((const char*)__FUNCTION__,__LINE__, pv);
}

/****************************************************
*    Build CONFIG_MEM_DEBUG version                 *
*    Adapt third lib build with release SDK end     *
*****************************************************/

void os_dump_memory_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task)
{
	xPortDumpMemStats(start_tick, ticks_since_malloc, task);
}

#endif

void* os_malloc_wifi_buffer(size_t size)
{
#if (CONFIG_RTT) && (CONFIG_SOC_BK7251 || CONFIG_SOC_BK7271)
        return (void*)dtcm_malloc(size);
#elif (CONFIG_FREERTOS) && (CONFIG_SOC_BK7251)
        return (void*)pvPortMalloc(size);
#else
        return (void*)os_malloc(size);
#endif
}

extern void pvShowMemoryConfigInfo(void);
void os_show_memory_config_info(void)
{
	pvShowMemoryConfigInfo();
}
