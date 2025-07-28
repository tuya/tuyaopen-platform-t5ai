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

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include <components/log.h>
#if CONFIG_MEM_DEBUG
#include "bk_list.h"
#include <os/str.h>
#include <driver/wdt.h>
#include "bk_wdt.h"
#include <wdt_driver.h>
#include "arch_interrupt.h"
#endif
#include <os/mem.h>
#if CONFIG_PSRAM_AS_SYS_MEMORY
#include <driver/psram.h>
#endif
#include <driver/pwr_clk.h>
#include "stack_base.h"

#if CONFIG_MEM_DEBUG_OVERFLOW
// #include "dwt.h"
// #include "driver/aon_rtc.h"
#include "cmsis_gcc.h"
#endif

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#define TAG "os"

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

#define MEM_OVERFLOW_TAG        0xDC
#define MEM_OVERFLOW_WORD_TAG   0xDCDCDCDC

#define MEM_HEAD_WORD_TAG       0xFFFFEEEE

#if CONFIG_MEM_DEBUG
#define MEM_CHECK_TAG_LEN      0x4
#else
#define MEM_CHECK_TAG_LEN      0x0
#endif

/* Allocate the memory for the heap. */
#if configDYNAMIC_HEAP_SIZE
uint8_t *ucHeap;
#elif( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
#if CONFIG_HEAP_IN_CACHE
	static __attribute__((section(".sram_cache"))) uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Due to the dynamic memory allocation operations in the log output mechanism,
 * it is not recommended to add logs during the initialization, allocation, and free processes of dynamic memory management.
 * If logging is necessary during this process for debugg, it is advisable to use MEM_STATIC_LOGI.
 * Using other logging API may lead to unstable situations.
 */
#define MEM_STATIC_LOGI( tag, format, ... ) bk_printf_static_nonblock(BK_LOG_INFO, tag, format, ##__VA_ARGS__)
#define MEM_STATIC_LOGD( tag, format, ... ) bk_printf_static_nonblock(BK_LOG_DEBUG, tag, format, ##__VA_ARGS__)

#if CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION spinlock_t s_spinlock_heap = SPIN_LOCK_ACQUIRE_INIT;
//#define HeapEnterCritical() {vTaskSuspendAll(); spin_lock(&s_spinlock_heap);}
//#define HeapExitCritical() {spin_unlock(&s_spinlock_heap); xTaskResumeAll();}

#define HeapEnterCritical() {vPortEnterCritical(&s_spinlock_heap);}
#define HeapExitCritical() {vPortExitCritical(&s_spinlock_heap);}
#endif

#ifndef CONFIG_MEM_DEBUG_FUNC_NAME_LEN
#define CONFIG_MEM_DEBUG_FUNC_NAME_LEN 24
#endif

#ifndef CONFIG_MEM_DEBUG_TASK_NAME_LEN
#define CONFIG_MEM_DEBUG_TASK_NAME_LEN 8
#endif

typedef void * (*CALLOC_PTR)(size_t num, size_t size);

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
#if CONFIG_MEM_DEBUG_OVERFLOW
	uint32_t head_magic_num;
#endif
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
#if CONFIG_MEM_DEBUG
	struct list_head node;					/*<< linked to xUsed */

#if CONFIG_MEM_DEBUG_FUNC_NAME
	char *funcName;                                     /*<< the function name */
#endif

#if CONFIG_MEM_DEBUG_TASK_NAME
	char *taskName;                                  /*<< the task name */
#endif
	union {
		struct {
			uint16_t allocTime;					        /*<< the leak time (*1sec)*/
			uint16_t line;                              /*<< the function line */
		};
		uint32_t time_line;
	};
	uint32_t wantedSize;						/*<< malloc size */
#endif
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );

#if CONFIG_MEM_DEBUG_OVERFLOW
#define GET_AON_RTC_TICK (REG_READ(SOC_AON_RTC_REG_BASE + (0x3 << 2)))
#define FREE_RECORD_MAX  CONFIG_MEM_OVERFLOW_FREE_RECORD_MAX
static __attribute__((section(".psram.bss"))) volatile uint32_t s_free_record_index = 0;
typedef struct {
	char *name;
	uint32_t line;
	uint32_t lr;
	char *free_ptr;
	uint32_t time;
}free_record_type;
static  __attribute__((section(".psram.bss"))) free_record_type volatile s_free_records[FREE_RECORD_MAX];
#define MALLOC_RECORD_MAX  CONFIG_MEM_OVERFLOW_MALLOC_RECORD_MAX
static __attribute__((section(".psram.bss"))) volatile uint32_t s_malloc_record_index = 0;
static __attribute__((section(".psram.bss"))) volatile uint32_t s_sram_malloc_record_index = 0;
static __attribute__((section(".psram.bss"))) free_record_type volatile s_malloc_records[MALLOC_RECORD_MAX];
static __attribute__((section(".psram.bss"))) free_record_type volatile s_sram_malloc_records[MALLOC_RECORD_MAX];

__attribute__((section(".iram")))void CheckFreeList(void);
#endif

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;
#if CONFIG_MEM_DEBUG
static struct list_head xUsed;
#endif

#if CONFIG_MEM_DEBUG && CONFIG_PSRAM_AS_SYS_MEMORY
static struct list_head xPsramUsed;
#endif


/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
uint8_t *psram_ucHeap;
/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t psram_xStart, *psram_pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t psram_xFreeBytesRemaining = 0U;
static size_t psram_xMinimumEverFreeBytesRemaining = 0U;

static volatile uint32_t s_psram_used_count = 0;

void rtos_regist_plat_dump_hook(uint32_t reg_base_addr, uint32_t reg_size);

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
#if CONFIG_TZ
#define PSRAM_START_ADDRESS    (void*)(CONFIG_AP_PSRAM_HEAP_ADDR + SOC_ADDR_OFFSET)
#else
#define PSRAM_START_ADDRESS    (void*)(CONFIG_AP_PSRAM_HEAP_ADDR)
#endif
#define PSRAM_HEAP_SIZE        CONFIG_AP_PSRAM_HEAP_SIZE
#endif


/*-----------------------------------------------------------*/
#if (CONFIG_LV_ATTRIBUTE_FAST_MEM) && (CONFIG_SOC_BK7258)
void bk_psram_heap_init(void) {
#elif CONFIG_SOC_BK7236XX
__attribute__((section(".iram"))) void bk_psram_heap_init(void) {
#else
__attribute__((section(".itcm_sec_code"))) void bk_psram_heap_init(void) {
#endif

	BlockLink_t *pxFirstFreeBlock;
	uint8_t *pucAlignedHeap;
	size_t uxAddress;
	size_t xTotalHeapSize;

	xTotalHeapSize = PSRAM_HEAP_SIZE;
	psram_ucHeap = PSRAM_START_ADDRESS;

	MEM_STATIC_LOGD(TAG, "psram:0x%x,size:%d\r\n", psram_ucHeap, xTotalHeapSize);

#if CONFIG_PSRAM_HEAP_INIT_SET_ZERO
	os_memset_word((uint32_t *)psram_ucHeap, 0x0, xTotalHeapSize);
#endif

	// rtos_regist_plat_dump_hook((uint32_t)psram_ucHeap, xTotalHeapSize);

	/* Ensure the heap starts on a correctly aligned boundary. */
	uxAddress = ( size_t ) psram_ucHeap;

	if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( portBYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
		xTotalHeapSize -= uxAddress - ( size_t ) psram_ucHeap;
	}

	pucAlignedHeap = ( uint8_t * ) uxAddress;

	/* psram_xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	psram_xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
#if CONFIG_MEM_DEBUG_OVERFLOW
	psram_xStart.head_magic_num = MEM_HEAD_WORD_TAG;
    psram_xStart.pxNextFreeBlock->head_magic_num = MEM_HEAD_WORD_TAG;
#endif
	psram_xStart.xBlockSize = ( size_t ) 0;

	/* psram_pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
	psram_pxEnd = ( void * ) uxAddress;
	psram_pxEnd->xBlockSize = 0;
	psram_pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by psram_pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = psram_pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	psram_xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	psram_xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	#if CONFIG_MEM_DEBUG
	INIT_LIST_HEAD(&xPsramUsed);
	#endif

	s_psram_used_count = 0;
	FIXED_ADDR_PSRAM_USDE_COUNT = 0;
}

static void psram_prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &psram_xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != psram_pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = psram_pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}

/*-----------------------------------------------------------*/

static void *psram_malloc_without_lock( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( psram_pxEnd == NULL || !bk_psram_heap_init_flag_get())
		{
			bk_psram_heap_init();
			bk_psram_heap_init_flag_set(true);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
					configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= psram_xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &psram_xStart;
				pxBlock = psram_xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != psram_pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
						configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
                #if CONFIG_MEM_DEBUG_OVERFLOW
						pxNewBlockLink->head_magic_num = MEM_HEAD_WORD_TAG;
                #endif
						psram_prvInsertBlockIntoFreeList( pxNewBlockLink );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					psram_xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( psram_xFreeBytesRemaining < psram_xMinimumEverFreeBytesRemaining )
					{
						psram_xMinimumEverFreeBytesRemaining = psram_xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;

#if CONFIG_MEM_DEBUG
					list_add_tail(&pxBlock->node, &xPsramUsed);
#endif

				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
	return pvReturn;
}

uint32_t psram_used_area_begin = 0;
uint32_t psram_used_area_end = 0;
uint32_t psram_memory_end = 0;

static uint32_t get_memory_end(BlockLink_t *pxLink) {
	uint32_t mem_end = (uint32_t)pxLink + (pxLink->xBlockSize & ~xBlockAllocatedBit);
	return mem_end;
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *psram_malloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void *psram_malloc( size_t xWantedSize )
#endif
{
	void *pvReturn = NULL;
#if CONFIG_MEM_DEBUG
	uint8_t *mem_end = NULL;
	uint32_t mem_end_len = 0;

    if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGD(NULL, "malloc_risk\r\n");
		BK_ASSERT(false);
	}
#endif

	if (xWantedSize == 0)
		xWantedSize = 4;

	if(( xWantedSize & 0x3 ) != 0x00)
	{
		/* 4 Byte alignment required for psram. */
		xWantedSize += ( 0x4 - ( xWantedSize & 0x3 ) );
	}
	if(FIXED_ADDR_PSRAM_POWER_DOWN == PM_PSRAM_POWER_DOWN_MAGIC)
	{
		bk_psram_heap_init_flag_set(false);
		FIXED_ADDR_PSRAM_POWER_DOWN = 0x0;
	}

	if( psram_pxEnd == NULL || !bk_psram_heap_init_flag_get())
	{
		bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_AS_MEM,PM_POWER_MODULE_STATE_ON);
	}
	//vTaskSuspendAll();
#if CONFIG_FREERTOS_SMP
	HeapEnterCritical();
#endif
	pvReturn = psram_malloc_without_lock(xWantedSize  + MEM_CHECK_TAG_LEN);
	if(pvReturn)
	{
		s_psram_used_count++;
		FIXED_ADDR_PSRAM_USDE_COUNT += 1;
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
		BlockLink_t *pxLink = (BlockLink_t *)((u8*)pvReturn - xHeapStructSize);
		if(pvReturn && call_func_name) {
#if CONFIG_MALLOC_STATIS
			MEM_STATIC_LOGD(TAG, "m:%p,%d|%s,%d\r\n", pxLink, (pxLink->xBlockSize & ~xBlockAllocatedBit), call_func_name, line);
#endif
		}
#if CONFIG_MEM_DEBUG
		pxLink->time_line = ((xTaskGetTickCount()/configTICK_RATE_HZ) & 0xffff) | ((line & 0xffff) << 16);

#if CONFIG_MEM_DEBUG_FUNC_NAME
		pxLink->funcName = (char *)call_func_name;
#endif
#if CONFIG_MEM_DEBUG_TASK_NAME
		//malloc can only be called in Task context!
		if (rtos_is_scheduler_started()) {
			pxLink->taskName = pcTaskGetName(NULL);
		}
		else
			pxLink->taskName = "NA";
#endif

		pxLink->wantedSize = xWantedSize;

 		mem_end = pvReturn + xWantedSize;
 		mem_end_len = (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize - xWantedSize;
 		os_memset_word((uint32_t *)mem_end, MEM_OVERFLOW_WORD_TAG, mem_end_len);

    #if CONFIG_MEM_DEBUG_OVERFLOW
		pxLink->head_magic_num = MEM_HEAD_WORD_TAG;
    #endif

		if(psram_used_area_begin == 0 || (uint32_t)pxLink < psram_used_area_begin) {
			psram_used_area_begin = (uint32_t)pxLink;
		}
		psram_memory_end = get_memory_end(pxLink);
		if(psram_used_area_end == 0 || psram_memory_end > psram_used_area_end) {
			psram_used_area_end = psram_memory_end;
		}
#endif
#endif //#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	}

#if CONFIG_MEM_DEBUG_OVERFLOW
	CheckFreeList();
#endif

	//( void ) xTaskResumeAll();
#if CONFIG_FREERTOS_SMP
	HeapExitCritical();
#endif

	#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn && need_zero)
		os_memset_word(pvReturn, 0, xWantedSize);

	#endif

#if CONFIG_DEBUG_VERSION
	if (pvReturn == NULL)
		BK_ASSERT(0);
#endif

	return pvReturn;
}

void * psram_calloc(size_t num, size_t size)
{
    size_t total = num * size;

    // check for multiplication overflow
    if ((size != 0) && ((total / size) != num))
    {
        return NULL;
    }

    void * ptr = psram_malloc(total);
    if (ptr != NULL)
    {
        #if CONFIG_FIX_PSRAM_PTR_ISSUE
        memset(ptr, 0, total);
        #else
        os_memset_word((uint32_t *)ptr, 0, total);
        #endif
    }

    return ptr;
}

#endif //#if CONFIG_PSRAM_AS_SYS_MEMORY


#if CONFIG_MEM_DEBUG_OVERFLOW
#define FREE_LIST_RECORD_MAX  CONFIG_MEM_OVERFLOW_FREELIST_RECORD_MAX
// static volatile uint32_t s_malloc_freelist_index = 0;
static __attribute__((section(".psram.bss"))) uint32_t volatile s_freelist_records[FREE_LIST_RECORD_MAX];
__attribute__((section(".iram")))void CheckFreeList(void)
{
	BlockLink_t *pxIterator;
	uint32_t i = 0;
	// uint32_t int_level = rtos_enter_critical();
	
	for( pxIterator = &xStart; (pxIterator->pxNextFreeBlock != pxEnd) && (i < FREE_LIST_RECORD_MAX); pxIterator = pxIterator->pxNextFreeBlock )
	{
        i++;
		/* Nothing to do here, just iterate to the right position. */
		if(pxIterator && (pxIterator->pxNextFreeBlock) && ( MEM_HEAD_WORD_TAG != (uint32_t)pxIterator->pxNextFreeBlock->head_magic_num))
			BK_ASSERT(0);
	}
#if (CONFIG_PSRAM_AS_SYS_MEMORY)

	i = 0;
	for( pxIterator = &psram_xStart; (pxIterator->pxNextFreeBlock != psram_pxEnd) && (i < 1024); pxIterator = pxIterator->pxNextFreeBlock )
	{
        i++;
		/* Nothing to do here, just iterate to the right position. */
		if(pxIterator && (pxIterator->pxNextFreeBlock) && ( MEM_HEAD_WORD_TAG != (uint32_t)pxIterator->pxNextFreeBlock->head_magic_num))
			BK_ASSERT(0);
	}
#endif
	// rtos_exit_critical(int_level);
}
#endif

#if CONFIG_MEM_DEBUG
static inline void show_mem_info(BlockLink_t *pxLink)
{
#if CONFIG_MEM_DEBUG_FUNC_NAME && CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("%-8d   0x%-8x   %-4d   %-5d   %-32s   %-16s\r\n",
		pxLink->allocTime, (u8*)pxLink + xHeapStructSize, pxLink->wantedSize,
		pxLink->line, pxLink->funcName, pxLink->taskName);
#elif  CONFIG_MEM_DEBUG_FUNC_NAME
	BK_DUMP_OUT("%-8d   0x%-8x   %-4d   %-5d   %-32s\r\n",
		pxLink->allocTime, (u8*)pxLink + xHeapStructSize, pxLink->wantedSize,
		pxLink->line, pxLink->funcName);
#elif CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("%-8d   0x%-8x   %-4d   %-5d   %-16s\r\n",
		pxLink->allocTime, (u8*)pxLink + xHeapStructSize, pxLink->wantedSize,
		pxLink->line, pxLink->taskName);
#else
	BK_DUMP_OUT("%-8d   0x%-8x   %-4d   %-5d\r\n",
		pxLink->allocTime, (u8*)pxLink + xHeapStructSize, pxLink->wantedSize,
		pxLink->line);
#endif

#if (CONFIG_TASK_WDT)
	bk_task_wdt_feed();
#endif

#if 0	//variables maybe modified abnormal, causes feed watchdog fail,so force to feed them.
#if CONFIG_INT_WDT
	bk_wdt_feed();
#endif
#if (CONFIG_INT_AON_WDT)
	bk_int_aon_wdt_feed();
#endif
#else
#if (CONFIG_INT_WDT || CONFIG_INT_AON_WDT)
	bk_wdt_force_feed();
#endif
#endif

}

static inline void mem_overflow_check(BlockLink_t *pxLink)
{
	uint8_t *mem_end = (uint8_t *)pxLink + xHeapStructSize + pxLink->wantedSize;
	uint32_t mem_end_len = (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize - pxLink->wantedSize;

	for ( int i = 0; i < mem_end_len; i++) {
	#if CONFIG_MEM_DEBUG_OVERFLOW
		if ((MEM_OVERFLOW_TAG != mem_end[i]) ||
			((pxLink->head_magic_num != MEM_HEAD_WORD_TAG) && (i == 0)))
    #else   
        if (MEM_OVERFLOW_TAG != mem_end[i])
    #endif
		{
			BK_DUMP_OUT("Mem Overflow ......mem_end[%p + %d]=[0x%02x].....\r\n", mem_end, i, mem_end[i]);
			show_mem_info(pxLink);
			stack_mem_dump((uint32_t)pxLink - 64, (uint32_t)pxLink + xHeapStructSize + pxLink->wantedSize + 64);
			if (0 == arch_is_enter_exception()) {
				configASSERT( false );
			}
            break;
		}
	}
}
#endif

/*-----------------------------------------------------------*/

static void *malloc_without_lock( size_t xWantedSize )
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;

	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( pxEnd == NULL )
		{
			prvHeapInit();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
					configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
						configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
                    #if CONFIG_MEM_DEBUG_OVERFLOW
						pxNewBlockLink->head_magic_num = MEM_HEAD_WORD_TAG;
                    #endif
						prvInsertBlockIntoFreeList( pxNewBlockLink );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;

#if CONFIG_MEM_DEBUG
					list_add_tail(&pxBlock->node, &xUsed);
#endif

				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
	return pvReturn;
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void * bk_wrap_sram_malloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void * bk_wrap_sram_malloc(size_t xWantedSize)
#endif
{
    void *pvReturn = NULL;
#if CONFIG_MEM_DEBUG
	uint8_t *mem_end = NULL;
	uint32_t mem_end_len = 0;
#endif

#if CONFIG_MEM_DEBUG_OVERFLOW
	uint32_t lr = __get_LR();
#endif

    HeapEnterCritical();

	if (xWantedSize == 0)
		xWantedSize = 4;

	// vTaskSuspendAll();
    

	pvReturn = malloc_without_lock(xWantedSize + MEM_CHECK_TAG_LEN);
	#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn)
	{
		BlockLink_t *pxLink = (BlockLink_t *)((u8*)pvReturn - xHeapStructSize);
		if(pvReturn && call_func_name) {
#if CONFIG_MALLOC_STATIS
			MEM_STATIC_LOGD(TAG, "m:%p,%d|%s,%d\r\n", pxLink, (pxLink->xBlockSize & ~xBlockAllocatedBit), call_func_name, line);
#endif
		}
#if CONFIG_MEM_DEBUG
		pxLink->allocTime = (uint16_t)(xTaskGetTickCount()/configTICK_RATE_HZ);
#if CONFIG_MEM_DEBUG_FUNC_NAME
		pxLink->funcName = (char *)call_func_name;
#endif
#if CONFIG_MEM_DEBUG_TASK_NAME
		//malloc can only be called in Task context!
		if (rtos_is_scheduler_started())
			pxLink->taskName = pcTaskGetName(NULL);
		else
			pxLink->taskName = "NA";
#endif
		pxLink->line = line;
		pxLink->wantedSize = xWantedSize;

 		mem_end = pvReturn + xWantedSize;
 		mem_end_len = (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize - xWantedSize;
 		os_memset(mem_end, MEM_OVERFLOW_TAG, mem_end_len);
    #if CONFIG_MEM_DEBUG_OVERFLOW
		pxLink->head_magic_num = MEM_HEAD_WORD_TAG;
    #endif
#endif
	}
    #endif

#if CONFIG_MEM_DEBUG_OVERFLOW
	{
		s_sram_malloc_records[s_sram_malloc_record_index%MALLOC_RECORD_MAX].free_ptr = pvReturn;
		s_sram_malloc_records[s_sram_malloc_record_index%MALLOC_RECORD_MAX].time = GET_AON_RTC_TICK; 
		s_sram_malloc_records[s_sram_malloc_record_index%MALLOC_RECORD_MAX].lr = lr;
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
		s_sram_malloc_records[s_sram_malloc_record_index%MALLOC_RECORD_MAX].name = (char *)call_func_name;
		s_sram_malloc_records[s_sram_malloc_record_index%MALLOC_RECORD_MAX].line = line;
#endif
		s_sram_malloc_record_index++;
	}

	CheckFreeList();
#endif

    #if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn && need_zero)
		os_memset(pvReturn, 0, xWantedSize);
	#endif

    // ( void ) xTaskResumeAll();
    HeapExitCritical();

    return pvReturn;
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *pvPortMalloc_cm(const char *call_func_name, int line, size_t xWantedSize, int need_zero )
#else
void *pvPortMalloc( size_t xWantedSize )
#endif
{	
    void *pvReturn = NULL;

#if CONFIG_MEM_DEBUG_OVERFLOW
	uint32_t lr = __get_LR();
#endif

    HeapEnterCritical();

#if CONFIG_MEM_DEBUG
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGD(NULL, "malloc_risk\r\n");
		BK_ASSERT(false);
	}
#endif

    #if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
    pvReturn = bk_wrap_sram_malloc_cm(call_func_name, line, xWantedSize, need_zero);
    #else
    pvReturn = bk_wrap_sram_malloc(xWantedSize);
    #endif

 #if CONFIG_MEM_DEBUG_OVERFLOW
	{
		s_malloc_records[s_malloc_record_index%MALLOC_RECORD_MAX].free_ptr = pvReturn;
		s_malloc_records[s_malloc_record_index%MALLOC_RECORD_MAX].time = GET_AON_RTC_TICK;
		s_malloc_records[s_malloc_record_index%MALLOC_RECORD_MAX].lr = lr;
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
		s_malloc_records[s_malloc_record_index%MALLOC_RECORD_MAX].name = (char *)call_func_name;
		s_malloc_records[s_malloc_record_index%MALLOC_RECORD_MAX].line = line;
#endif
		s_malloc_record_index++;
	}

	CheckFreeList();
#endif

	#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	if(pvReturn && need_zero)
		os_memset(pvReturn, 0, xWantedSize);
	#endif

    HeapExitCritical();

	return pvReturn;
}

/*-----------------------------------------------------------*/
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void *vPortFree_cm(const char *call_func_name, int line, void *pv )
#else
void vPortFree( void *pv )
#endif
{
	uint8_t *puc = ( uint8_t * ) pv;
	BlockLink_t *pxLink;

#if CONFIG_MEM_DEBUG_OVERFLOW
	uint32_t lr = __get_LR();
#endif

#if CONFIG_MEM_DEBUG
	if (platform_is_in_interrupt_context() && (arch_is_enter_exception() == 0)) {
		BK_LOGD(NULL, "free_risk\r\n");
		BK_ASSERT(false);
	}
#endif

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );
#if CONFIG_MEM_DEBUG
		mem_overflow_check(pxLink);
#endif

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				//vTaskSuspendAll();
#if CONFIG_FREERTOS_SMP
				HeapEnterCritical();
#endif

#if CONFIG_MEM_DEBUG_OVERFLOW
				{
					s_free_records[s_free_record_index%FREE_RECORD_MAX].free_ptr = pv;
					s_free_records[s_free_record_index%FREE_RECORD_MAX].time = GET_AON_RTC_TICK;
#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
					s_free_records[s_free_record_index%FREE_RECORD_MAX].name = (char *)call_func_name;
					s_free_records[s_free_record_index%FREE_RECORD_MAX].lr = lr;
					s_free_records[s_free_record_index%FREE_RECORD_MAX].line = line;
#endif
					s_free_record_index++;
				}
#endif

#if CONFIG_MALLOC_STATIS
                if (call_func_name)
                {
                    MEM_STATIC_LOGD(TAG, "f:%p,%d|%s,%d\r\n", pxLink, pxLink->xBlockSize, call_func_name, line);
                }
#endif
#if CONFIG_MEM_DEBUG
				list_del(&pxLink->node);
				pxLink->allocTime = 0;
#if CONFIG_MEM_DEBUG_FUNC_NAME
				pxLink->funcName = 0;
#endif
#if CONFIG_MEM_DEBUG_TASK_NAME
				pxLink->taskName = 0;
#endif
				pxLink->line = 0;
#endif
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
				if ((uint32_t)puc >= (uint32_t)PSRAM_START_ADDRESS)
                {
                    /* Add this block to the list of psram free blocks. */
                    psram_xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    psram_prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
					s_psram_used_count--;
					FIXED_ADDR_PSRAM_USDE_COUNT -= 1;
                }
                else
#endif
				{
					/* Add this block to the list of ram free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}

#if CONFIG_MEM_DEBUG_OVERFLOW
				CheckFreeList();
#endif

				//( void ) xTaskResumeAll();
#if CONFIG_FREERTOS_SMP
				HeapExitCritical();
#endif

			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	return NULL;
#endif
}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
void xPortDumpMemStats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task)
{
	BlockLink_t *pxLink;

	BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "tick", "addr", "size", "line");

#if CONFIG_MEM_DEBUG_FUNC_NAME
	BK_DUMP_OUT("   %-32s", "func");
#endif

#if CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("   %-16s", "task");
#endif
	BK_DUMP_OUT("\n");

	BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "--------", "----------", "----", "-----");

#if CONFIG_MEM_DEBUG_FUNC_NAME
	BK_DUMP_OUT("   %-32s", "--------------------------------");
#endif

#if CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("   %-16s", "----------------");
#endif
	BK_DUMP_OUT("\n");

	if (arch_is_enter_exception() == 0) {
		//vTaskSuspendAll();
#if CONFIG_FREERTOS_SMP
		HeapEnterCritical();
#endif
	}

	list_for_each_entry(pxLink, &xUsed, node) {

		if (pxLink->allocTime < start_tick)
			continue;

		if ((pxLink->allocTime - start_tick) < ticks_since_malloc)
			continue;

	#if CONFIG_MEM_DEBUG_TASK_NAME
		if (task && os_strncmp(task, pxLink->taskName, sizeof(pxLink->taskName)))
			continue;
	#endif

		show_mem_info(pxLink);

		mem_overflow_check(pxLink);
	}

	if (arch_is_enter_exception() == 0) {
		//( void ) xTaskResumeAll();
#if CONFIG_FREERTOS_SMP
		HeapExitCritical();
#endif
	}

	bk_psram_heap_get_used_state();
}

void mem_overflow_check_all(void)
{
	BlockLink_t *pxLink;

	if (arch_is_enter_exception() == 0) {
		//vTaskSuspendAll();
#if CONFIG_FREERTOS_SMP
		HeapEnterCritical();
#endif
	}

	list_for_each_entry(pxLink, &xUsed, node) {
		mem_overflow_check(pxLink);
	}

#if CONFIG_PSRAM_AS_SYS_MEMORY
	if(bk_psram_heap_get_used_count() > 0) {
		list_for_each_entry(pxLink, &xPsramUsed, node) {
			mem_overflow_check(pxLink);
		}
	}

    if (arch_is_enter_exception() == 1) {
		stack_mem_dump(psram_used_area_begin, psram_used_area_end);
	}

#endif //#if CONFIG_PSRAM_AS_SYS_MEMORY

	if (arch_is_enter_exception() == 0) {
		//( void ) xTaskResumeAll();
#if CONFIG_FREERTOS_SMP
		HeapExitCritical();
#endif
	}
}

#endif

uint32_t bk_psram_heap_get_used_count(void) {
#if CONFIG_PSRAM_AS_SYS_MEMORY
	return s_psram_used_count;
#else
	return 0;
#endif
}

void bk_psram_heap_get_used_state(void) {

#if CONFIG_PSRAM_AS_SYS_MEMORY
	uint32_t count = bk_psram_heap_get_used_count();
	BK_DUMP_OUT("Psram heap used count is %d.\n", count);
	if (0 == count) {
		return;
	}

#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
	BlockLink_t *pxLink;
	BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "tick", "addr", "size", "line");

#if CONFIG_MEM_DEBUG_FUNC_NAME
	BK_DUMP_OUT("   %-32s", "func");
#endif

#if CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("   %-16s", "task");
#endif
	BK_DUMP_OUT("\n");

	BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "--------", "----------", "----", "-----");

#if CONFIG_MEM_DEBUG_FUNC_NAME
	BK_DUMP_OUT("   %-32s", "--------------------------------");
#endif

#if CONFIG_MEM_DEBUG_TASK_NAME
	BK_DUMP_OUT("   %-16s", "----------------");
#endif
	BK_DUMP_OUT("\n");

	if (arch_is_enter_exception() == 0) {
	
#if CONFIG_FREERTOS_SMP
		HeapEnterCritical();
#endif

	}

	list_for_each_entry(pxLink, &xPsramUsed, node) {
		show_mem_info(pxLink);
		mem_overflow_check(pxLink);
	}

	if (arch_is_enter_exception() == 0) {
	
#if CONFIG_FREERTOS_SMP
		HeapExitCritical();
#endif
	}

#endif //#if CONFIG_MALLOC_STATIS || CONFIG_MEM_DEBUG
#endif //#if CONFIG_PSRAM_AS_SYS_MEMORY
}

extern unsigned char __psram_data_start__;
#define PSRAM_DATA_START_ADDRESS ((uint32_t)&__psram_data_start__)

extern unsigned char __psram_data_end__;
#define PSRAM_DATA_END_ADDRESS ((uint32_t)&__psram_data_end__)

extern unsigned char __psram_bss_start__;
#define PSRAM_BSS_START_ADDRESS ((uint32_t)&__psram_bss_start__)

extern unsigned char __psram_bss_end__;
#define PSRAM_BSS_END_ADDRESS ((uint32_t)&__psram_bss_end__)


void bk_psram_heap_dump_data(void)
{
#if CONFIG_PSRAM_AS_SYS_MEMORY
	stack_mem_dump(psram_used_area_begin, psram_used_area_end);

    if (PSRAM_DATA_END_ADDRESS > PSRAM_DATA_START_ADDRESS)
    {
        stack_mem_dump(PSRAM_DATA_START_ADDRESS, PSRAM_DATA_END_ADDRESS);
    }

    if (PSRAM_BSS_END_ADDRESS > PSRAM_BSS_START_ADDRESS)
    {
        stack_mem_dump(PSRAM_BSS_START_ADDRESS, PSRAM_BSS_END_ADDRESS);
    }
    
#endif
}

/*-----------------------------------------------------------*/
size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}

/*-----------------------------------------------------------*/
size_t xPortGetPsramTotalHeapSize( void )
{
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
    return PSRAM_HEAP_SIZE;
#else
    return 0x0;
#endif
}

size_t xPortGetPsramFreeHeapSize( void )
{
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
    return psram_xFreeBytesRemaining;
#else
    return 0x0;
#endif
}

size_t xPortGetPsramMinimumFreeHeapSize( void )
{
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
    return psram_xMinimumEverFreeBytesRemaining;
#else
    return 0x0;
#endif
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

#if configDYNAMIC_HEAP_SIZE

extern unsigned char _heap_start, _heap_end;
#define HEAP_START_ADDRESS    (void*)&_heap_start
#define HEAP_END_ADDRESS      (void*)&_heap_end

static void *prvHeapGetHeaderPointer(void)
{
	return (void *)HEAP_START_ADDRESS;
}

uint32_t prvHeapGetTotalSize(void)
{
	configASSERT(HEAP_END_ADDRESS > HEAP_START_ADDRESS);
	return (HEAP_END_ADDRESS - HEAP_START_ADDRESS);
}

#else //#if configDYNAMIC_HEAP_SIZE

#define HEAP_START_ADDRESS    (uint32_t)(&ucHeap[0])
#define HEAP_END_ADDRESS      (uint32_t)(HEAP_START_ADDRESS + configTOTAL_HEAP_SIZE)

uint32_t prvHeapGetTotalSize(void)
{
	return configTOTAL_HEAP_SIZE;
}

#endif //#if configDYNAMIC_HEAP_SIZE

void rtos_regist_plat_dump_hook(uint32_t reg_base_addr, uint32_t reg_size);

static void prvHeapInit( void )
{
	BlockLink_t *pxFirstFreeBlock;
	uint8_t *pucAlignedHeap;
	size_t uxAddress;
	size_t xTotalHeapSize;

	#if configDYNAMIC_HEAP_SIZE
	xTotalHeapSize = prvHeapGetTotalSize();
	ucHeap = prvHeapGetHeaderPointer();
	rtos_regist_plat_dump_hook((uint32_t)ucHeap, xTotalHeapSize);
	#else
	xTotalHeapSize = configTOTAL_HEAP_SIZE;
	#endif

	MEM_STATIC_LOGD(TAG, "prvHeapInit-start addr:0x%x, size:%d\r\n", ucHeap, xTotalHeapSize);

	/* Ensure the heap starts on a correctly aligned boundary. */
	uxAddress = ( size_t ) ucHeap;

	if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( portBYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
		xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
	}

	pucAlignedHeap = ( uint8_t * ) uxAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
#if CONFIG_MEM_DEBUG_OVERFLOW
	xStart.head_magic_num = MEM_HEAD_WORD_TAG;
	xStart.pxNextFreeBlock->head_magic_num = MEM_HEAD_WORD_TAG;
#endif
	xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
	pxEnd = ( void * ) uxAddress;
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
	// It's too early init psram maybe failed
	// bk_psram_heap_init();
#endif

#if CONFIG_MEM_DEBUG
	INIT_LIST_HEAD(&xUsed);
#endif

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

#if CONFIG_MEM_DEBUG_OVERFLOW
   
	uint32_t i = 0;
	uint32_t int_level = rtos_enter_critical();

	for( pxIterator = &xStart; (pxIterator->pxNextFreeBlock != pxEnd) && i < FREE_LIST_RECORD_MAX; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* bak all free nodes to list */
		s_freelist_records[i++] = (uint32_t)pxIterator->pxNextFreeBlock;
	}
	rtos_exit_critical(int_level);
#endif
}

/**
 * @brief xPortPointerSize is based on the malloc implementation of heap_4
 * Returns the size of allocated block associated to the pointer
 *
 * @param pv pointer
 * @return size_t block size
 */
size_t xPortPointerSize(void * pv)
{
    uint8_t * puc = (uint8_t *) pv;
    BlockLink_t * pxLink;
    void * voidp;
    size_t sz = 0;

    if (pv != NULL)
    {
		//vTaskSuspendAll();
#if CONFIG_FREERTOS_SMP
		HeapEnterCritical();
#endif

        {
            /* The memory being checked will have an BlockLink_t structure immediately
            before it. */
            puc -= xHeapStructSize;

            /* This casting is to keep the compiler from issuing warnings. */
            voidp  = (void *) puc;
            pxLink = (BlockLink_t *) voidp;

            /* Check if the block is actually allocated. */
            configASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);
            configASSERT(pxLink->pxNextFreeBlock == NULL);

            #if CONFIG_MEM_DEBUG
            sz = pxLink->wantedSize;
            #else
            sz = (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize;
            #endif
        }
		//( void ) xTaskResumeAll();
#if CONFIG_FREERTOS_SMP
		HeapExitCritical();
#endif
    }

    return sz;
}

void * pvPortCalloc(size_t num, size_t size)
{
    size_t total = num * size;

    // check for multiplication overflow
    if ((size != 0) && ((total / size) != num))
    {
        return NULL;
    }

    void * ptr = pvPortMalloc(total);
    if (ptr != NULL)
    {
        memset(ptr, 0, total);
    }

    return ptr;
}

void *pvPortRealloc( void *pv, size_t size )
{
    CALLOC_PTR calloc_ptr;
    void * resized_ptr  = NULL;
    size_t current_size = xPortPointerSize(pv);

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
    size_t is_psram_addr = 0;
    if((uint32_t)pv >= (uint32_t)PSRAM_START_ADDRESS)
    {
        is_psram_addr = 1;
        calloc_ptr = psram_calloc;
    }
    else
#endif /* CONFIG_PSRAM_AS_SYS_MEMORY*/
	{
		calloc_ptr = pvPortCalloc;
	}

    if (current_size > 0) // pv is allocated
    {
        if (size) // New size is not 0
        {
            if (size == current_size) // if existing pointer is the same size
            {
                resized_ptr = pv;
            }
            else // New size is a different from current size
            {
                resized_ptr = (*calloc_ptr)(1, size);
                if (resized_ptr != NULL)
                {
                    size_t smallest_size = size < current_size ? size : current_size;

                    #if (CONFIG_PSRAM_AS_SYS_MEMORY)
                    if(is_psram_addr)
                    {
                        #if (CONFIG_FIX_PSRAM_PTR_ISSUE)
                        memcpy(resized_ptr, pv, smallest_size);
                        #else
                        os_memcpy_word((uint32_t *)resized_ptr, (uint32_t *)pv, smallest_size);
                        #endif
                    }
                    else
                    {
                        memcpy(resized_ptr, pv, smallest_size);
                    }
                    #else
                    memcpy(resized_ptr, pv, smallest_size);
                    #endif

                   vPortFree(pv);
                }
            }
        }
        else // If size if 0, free pointer
        {
            vPortFree(pv);
        }
    }
    else // pv is not allocated, allocate a new pointer
    {
        resized_ptr = (*calloc_ptr)(1, size);
    }

    return resized_ptr;
}

uint32_t CheckBlockSizeValid(uint32_t size)
{
	BlockLink_t *pxIterator;

	HeapEnterCritical()
	
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock )
	{
		if(pxIterator->xBlockSize >= size)
        {
            HeapExitCritical();
            return 1;
        }
			
	}
	HeapExitCritical();
    return 0;
}


// When bk_printf depends on heap initialization since bk_printf needs to init the mutex
// and mutex initialization needs to allocate memory from heap. So when heap initialization
// is called, the bk_printf is not ready, so define a special API for displaying heap
// configuration info!
//
// TODO - after we support bk_eary_printf() API, we can remove this API.
//
#if CONFIG_SOC_BK7236XX || (CONFIG_SOC_BK7239XX)
extern unsigned char _data_ram_begin;
#define RAM_START_ADDRESS  ((uint32_t)&_data_ram_begin)

extern unsigned char __data_start__;
#define DATA_START_ADDRESS ((uint32_t)&__data_start__)

extern unsigned char _data_ram_end;
#define DATA_END_ADDRESS ((uint32_t)&_data_ram_end)

extern unsigned char _bss_start;
#define BSS_START_ADDRESS ((uint32_t)&_bss_start)

extern unsigned char _bss_end;
#define BSS_END_ADDRESS ((uint32_t)&_bss_end)

extern unsigned char __dtcm_cpu0_start__;
#define DTCM_START_ADDRESS ((uint32_t)&__dtcm_cpu0_start__)

extern unsigned char __dtcm_cpu0_end__;
#define DTCM_END_ADDRESS ((uint32_t)&__dtcm_cpu0_end__)

extern unsigned char __itcm_cpu0_start__;
#define ITCM_START_ADDRESS ((uint32_t)&__itcm_cpu0_start__)

extern unsigned char __itcm_cpu0_end__;
#define ITCM_END_ADDRESS ((uint32_t)&__itcm_cpu0_end__)

extern unsigned char __iram_start__;
#define IRAM_START_ADDRESS ((uint32_t)&__iram_start__)

extern unsigned char __iram_end__;
#define IRAM_END_ADDRESS ((uint32_t)&__iram_end__)


void pvShowMemoryConfigInfo(void)
{
#if configDYNAMIC_HEAP_SIZE
	BK_LOGD(TAG, "\n");
	BK_LOGD(TAG, "%-8s %-8s %-8s %-8s\n", "mem_type", "start", "end", "size");
	BK_LOGD(TAG, "%-8s %-8s %-8s %-8s\n", "--------", "--------", "--------", "--------");
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "itcm", ITCM_START_ADDRESS, ITCM_END_ADDRESS, (ITCM_END_ADDRESS - ITCM_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "dtcm", DTCM_START_ADDRESS, DTCM_END_ADDRESS, (DTCM_END_ADDRESS - DTCM_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "ram", RAM_START_ADDRESS, HEAP_END_ADDRESS, (HEAP_END_ADDRESS - RAM_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "non_heap", RAM_START_ADDRESS, HEAP_START_ADDRESS, (HEAP_START_ADDRESS - RAM_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "iram", IRAM_START_ADDRESS, IRAM_END_ADDRESS, (IRAM_END_ADDRESS - IRAM_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "data", DATA_START_ADDRESS, DATA_END_ADDRESS, (DATA_END_ADDRESS - DATA_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "bss", BSS_START_ADDRESS, BSS_END_ADDRESS, (BSS_END_ADDRESS - BSS_START_ADDRESS));
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "heap", HEAP_START_ADDRESS, HEAP_END_ADDRESS, (HEAP_END_ADDRESS - HEAP_START_ADDRESS));
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
	BK_LOGD(TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "psram", PSRAM_START_ADDRESS, (PSRAM_START_ADDRESS + PSRAM_HEAP_SIZE), PSRAM_HEAP_SIZE);
#endif
#endif
}

#else
void pvShowMemoryConfigInfo(void)
{

}
#endif
