/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <common/bk_include.h>
#include <os/mem.h>

#include <stdlib.h>
#include <string.h>
#include <driver/pwr_clk.h>
#include <driver/psram.h>

#include "tuya_mem_heap.h"
static HEAP_HANDLE s_sram_handle = NULL;
HEAP_HANDLE s_psram_handle = NULL;
static void prvHeapInit( void );

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#if CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION spinlock_t s_spinlock_heap = SPIN_LOCK_ACQUIRE_INIT;
//#define HeapEnterCritical() {vTaskSuspendAll(); spin_lock(&s_spinlock_heap);}
//#define HeapExitCritical() {spin_unlock(&s_spinlock_heap); xTaskResumeAll();}

#define HeapEnterCritical() {vPortEnterCritical(&s_spinlock_heap);}
#define HeapExitCritical() {vPortExitCritical(&s_spinlock_heap);}
#endif


#if CONFIG_TZ
#define PSRAM_START_ADDRESS    (void*)(CONFIG_AP_PSRAM_HEAP_ADDR + SOC_ADDR_OFFSET)
#else
#define PSRAM_START_ADDRESS    (void*)(CONFIG_AP_PSRAM_HEAP_ADDR)
#endif
#define PSRAM_HEAP_SIZE        CONFIG_AP_PSRAM_HEAP_SIZE

uint32_t bk_psram_heap_get_used_count(void)
{
	heap_state_t state = {0};
	tuya_mem_heap_state(s_psram_handle, &state);
	return state.used_block;
}

void bk_psram_heap_get_used_state(void)
{
}

size_t xPortGetPsramTotalHeapSize( void )
{
    return PSRAM_HEAP_SIZE;
}

size_t xPortGetPsramFreeHeapSize( void )
{
    return tuya_mem_heap_available(s_psram_handle);
}

size_t xPortGetPsramMinimumFreeHeapSize( void )
{
	heap_state_t state = {0};
	tuya_mem_heap_state(s_psram_handle, &state);
	return state.free_watermark;
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *psram_malloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void *psram_malloc( size_t xWantedSize )
#endif
{
	void *pvReturn = NULL;

    if(!s_psram_handle) {
        prvHeapInit();
    }

    if(xWantedSize == 0) {
        xWantedSize = 4;
    } else {
        // xWantedSize += 4; // for debug use
    }

	pvReturn = tuya_mem_heap_malloc(s_psram_handle, xWantedSize);

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn && need_zero) {
		os_memset(pvReturn, 0, xWantedSize);
	}
#endif

    return pvReturn;
}

void * psram_calloc(size_t num, size_t size)
{
	return tuya_mem_heap_calloc(s_psram_handle, num * size);
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void * bk_wrap_sram_malloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void * bk_wrap_sram_malloc(size_t xWantedSize)
#endif
{
    if(!s_sram_handle) {
        prvHeapInit();
    }

    if(xWantedSize == 0) {
        xWantedSize = 4;
    }

	return tuya_mem_heap_malloc(s_sram_handle, xWantedSize);
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *pvPortMalloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void *pvPortMalloc( size_t xWantedSize )
#endif
{
	void *pvReturn = NULL;

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
    pvReturn = bk_wrap_sram_malloc_cm(call_func_name, line, xWantedSize, need_zero);
#else
    pvReturn = bk_wrap_sram_malloc(xWantedSize);
#endif

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn && need_zero) {
		os_memset(pvReturn, 0, xWantedSize);
	}
#endif

	return pvReturn;
}

/*-----------------------------------------------------------*/
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *vPortFree_cm(const char *call_func_name, int line, void *pv )
#else
void vPortFree( void *pv )
#endif
{
	tuya_mem_heap_free(0, pv);

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	return NULL;
#endif
}

extern unsigned char _heap_start, _heap_end;
#define HEAP_START_ADDRESS    (void*)&_heap_start
#define HEAP_END_ADDRESS      (void*)&_heap_end

void vTaskSuspendAll_pri( void )
{
	HeapEnterCritical();
}

void vTaskResumeAll_pri( void )
{
	HeapExitCritical();
}

static void prvHeapInit( void )
{
	if(FIXED_ADDR_PSRAM_POWER_DOWN == PM_PSRAM_POWER_DOWN_MAGIC)
	{
		bk_psram_heap_init_flag_set(false);
		FIXED_ADDR_PSRAM_POWER_DOWN = 0x0;
	}

	bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_AS_MEM,PM_POWER_MODULE_STATE_ON);
	bk_psram_heap_init_flag_set(true);

    FIXED_ADDR_PSRAM_USDE_COUNT = 100;

    int ret = 0;
    heap_context_t ctx = {0};
    ctx.dbg_output = bk_printf;
    ctx.enter_critical = vTaskSuspendAll_pri;
    ctx.exit_critical = vTaskResumeAll_pri;
    ret = tuya_mem_heap_init(&ctx);
    if(0 != ret) {
        bk_printf("--------->heap init err:%d \n", ret);
    }

	ret = tuya_mem_heap_create((void *)HEAP_START_ADDRESS, (HEAP_END_ADDRESS - HEAP_START_ADDRESS), &s_sram_handle);
	if(0 != ret) {
		bk_printf("--------->sram heap create err:%d \n", ret);
	}

	ret = tuya_mem_heap_create((void *)PSRAM_START_ADDRESS, PSRAM_HEAP_SIZE, &s_psram_handle);
	if(0 != ret) {
		bk_printf("--------->psram heap create err:%d \n", ret);
	}
}

void * pvPortCalloc(size_t num, size_t size)
{
	return tuya_mem_heap_calloc(s_sram_handle, num * size);
}

void *pvPortRealloc( void *pv, size_t size )
{
	if(!pv) {
		return pvPortCalloc(1, size);
	} else if((uint32_t)pv >= (uint32_t)PSRAM_START_ADDRESS) {
		return tuya_mem_heap_realloc(s_psram_handle, pv, size);
	} else {
		return tuya_mem_heap_realloc(s_sram_handle, pv, size);
	}
}

size_t xPortGetFreeHeapSize( void )
{
	return tuya_mem_heap_available(s_sram_handle);
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	heap_state_t state = {0};
	tuya_mem_heap_state(s_sram_handle, &state);
	return state.free_watermark;
}

void xPortDumpMemStats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task)
{
}

void pvShowMemoryConfigInfo(void)
{
}

void bk_psram_heap_dump_data(void)
{
}
