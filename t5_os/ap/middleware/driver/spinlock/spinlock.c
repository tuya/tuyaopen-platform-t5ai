// Copyright 2020-2022 Beken
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

#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "spinlock.h"

#if CONFIG_SOC_SMP
#include "cmsis_gcc.h"
#include "FreeRTOS.h"

#endif

#if CONFIG_SPINLOCK_DEBUG
#include "task.h"
#endif

#if (CONFIG_SOC_SMP)
#else
#define CPU_ID     1
#define portGET_CORE_ID()   CPU_ID
#endif

extern void		arch_fence(void);
extern void		arch_atomic_set(volatile u32 * lock_addr);
extern void		arch_atomic_clear(volatile u32 * lock_addr);

#define arch_int_disable	rtos_disable_int
#define arch_int_restore	rtos_enable_int

void spinlock_init(spinlock_t *slock)
{
#if CONFIG_SPINLOCK_DEBUG
    beken_thread_t  Current_TCB = NULL;
    Current_TCB = (beken_thread_t)xTaskGetCurrentTaskHandle();
    slock->taskTCBPointer = (uint32_t )Current_TCB;
#endif
	slock->owner = 0;
	slock->count = 0;
	slock->core_id = SPINLOCK_CORE_ID_UNINITILIZE;
}

#if CONFIG_SPINLOCK_DEBUG
static void spinlock_release_debug_res(spinlock_t *slock)
{
    slock->taskTCBPointer = 0;
}
#endif

uint32_t spinlock_acquire(volatile spinlock_t *slock, int32_t timeout)
{
	uint32_t flag = arch_int_disable();

#if (CONFIG_CPU_CNT > 1)
	// Note: The core IDs are the full 32 bit (CORE_ID_REGVAL_PRO/CORE_ID_REGVAL_APP) values
	const volatile uint32_t core_id = (uint32_t)portGET_CORE_ID();
	const volatile uint32_t lock_core_id = slock->core_id;

	if(core_id == lock_core_id)
	{
		slock->count++;
		arch_int_restore(flag);
		return flag;
	}

	if(lock_core_id != SPINLOCK_CORE_ID_UNINITILIZE 
		&& lock_core_id != 0
		&& lock_core_id != 1
	) {
		BK_ASSERT(0);
		arch_int_restore(flag);
		return flag;
	}

	arch_atomic_set(( volatile u32 *)&slock->owner);
	arch_fence();

	slock->core_id = core_id;
	slock->count = 1;

#endif
	arch_int_restore(flag);

	return flag;
}

void spinlock_release(volatile spinlock_t *slock, uint32_t flag2)
{
#if (CONFIG_CPU_CNT > 1)
	uint32_t core_id;

	uint32_t flag = arch_int_disable();


	// Note: The core IDs are the full 32 bit (CORE_ID_REGVAL_PRO/CORE_ID_REGVAL_APP) values
	core_id = (uint32_t)portGET_CORE_ID();

	if(core_id != slock->core_id)
	{
		BK_ASSERT(0);
		return;
	}

	if(slock->count == 0)
	{
		BK_ASSERT(0);
		return;
	}

	slock->count--;

	if(slock->count == 0)
	{
		slock->core_id = SPINLOCK_CORE_ID_UNINITILIZE;

		arch_fence();

		arch_atomic_clear(( volatile u32 *)&slock->owner);
	}
#endif

	arch_int_restore(flag);
}

#if (CONFIG_SOC_SMP)
#if CONFIG_CPU_CNT > 2
/* bk7236/58 three cores, just two exclusive access monitors
 * verification code:http://192.168.0.6/wangzhilei/bk7236_verification/-/tree/multicore_spinlock
 *
 * the exclusive operation is the pair of ldrex following by strex: ldrex performs a load of memory
 * but also tags the physical address to be monitored for exclusive access by the that core.
 * strex performs a conditional store to the memory, succeeding only if the target location is tagged
 * as being monitored for exclusive access by that core. this instruction returns non-zero in the
 * general-purpose register if the store does not succeed, and a value of 0 if the store is successful
 *
 * The issue: two cores occupy the exclusive signal, and the other core maybe cannot unlock/strex successfully
 */
 #error number of cpu cores greater than 2, does not support SMP for this configuration.
#endif

/*TODO: the driver layer shall be independent of the architecture or arm instruction*/
static inline int __spin_lock(volatile spinlock_t *lock)
{
	uint32_t core_id;
	int status = 0;

	// Note: The core IDs are the full 32 bit (CORE_ID_REGVAL_PRO/CORE_ID_REGVAL_APP) values
	core_id = portGET_CORE_ID();

	// The caller is already the owner of the lock. Simply increment the nesting count
	if (lock->owner == core_id)
	{
		BK_ASSERT(lock->count > 0 && lock->count < 0xFF);	  // Bad count value implies memory corruption
		lock->count ++;
		return 1;
	}

	do
	{
		// Note: __LDAEX and __STREXW are CMSIS functions

		while (__LDAEX(&lock->owner) != SPIN_LOCK_FREE)
		{
			__WFE();
		}

		BK_ASSERT((core_id == 0) || (core_id  == 1));

		// lock is free
		status = __STREXW(core_id, &lock->owner); // Try to set

	} while (status != 0); // retry until lock successfully

	lock->count ++;

	return 1;
}

static inline int __spin_lock_try(volatile spinlock_t *lock)
{
	uint32_t core_id;
	int status = 0;

	// Note: The core IDs are the full 32 bit (CORE_ID_REGVAL_PRO/CORE_ID_REGVAL_APP) values
	core_id = portGET_CORE_ID();

	// The caller is already the owner of the lock. Simply increment the nesting count
	if (lock->owner == core_id)
	{
		BK_ASSERT(lock->count > 0 && lock->count < 0xFF);	  // Bad count value implies memory corruption
		lock->count ++;
		return 1;
	}

	// Note: __LDAEX and __STREXW are CMSIS functions

	if (__LDAEX(&lock->owner) != SPIN_LOCK_FREE)
	{
		return 0;
	}

	BK_ASSERT((core_id == 0) || (core_id  == 1));
	// lock is free
	status = __STREXW(core_id, &lock->owner); // Try to set

	if(status != 0)
	{
		return 0;
	}

	lock->count ++;

	return 1;
}

void spin_lock(volatile spinlock_t *lock)
{
	__spin_lock(lock);
}

int spin_trylock(volatile spinlock_t *lock)
{
	return __spin_lock_try(lock);
}

void spin_unlock(volatile spinlock_t *lock)
{
    uint32_t core_id;

    core_id = portGET_CORE_ID();
    BK_ASSERT(core_id == lock->owner); // This is a lock that we didn't acquire, or the lock is corrupt
    BK_ASSERT((lock->count > 0) && (lock->count < 0x100));

    lock->count  --;

    if (!lock->count)
	{
		// If this is the last recursive release of the lock, mark the lock as free
		// Note: __STL, __DSB, __SEV are CMSIS functions.

		__STL(SPIN_LOCK_FREE, &lock->owner);
		__DSB();
 		__SEV();
    }
}

uint32_t _spin_lock_irqsave(spinlock_t *lock)
{
	unsigned long flags = rtos_disable_int();
	spin_lock(lock);

	return flags;
}

void _spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags)
{
	spin_unlock(lock);
	rtos_enable_int(flags);
}

/*
 * SPINLOCK:
 * NOTES:
 * There are two exclusive mointors in SRAM PORT, if two CPU-COREs exclusive access SRAM at the same time,
 * the third device(i.e:DMA, Audio, ...) writes data to SRAM will be failed without any indications.
 * So SPINLOCK memory are allocated in special section which will not conflicts with other devices.
 * If static allocates spinlock, please sets the spinlock in section of "sram_spinlock_section"
 */
#if CONFIG_SPINLOCK_SECTION
SPINLOCK_SECTION spinlock_t s_spinlock_memlock = SPIN_LOCK_INIT;
#ifndef CONFIG_SPINLOCK_DYNAMIC_CNT
#define CONFIG_SPINLOCK_DYNAMIC_CNT (128)
#endif
SPINLOCK_SECTION spinlock_t s_spinlock_mem[CONFIG_SPINLOCK_DYNAMIC_CNT] = {SPINLOCK_ACQUIRE_INITIALIZER};
#define SPINLOCK_FULL_GROUPS_CNT ((CONFIG_SPINLOCK_DYNAMIC_CNT)/32)
#define SPINLOCK_GROUPS_CNT ((CONFIG_SPINLOCK_DYNAMIC_CNT+31)/32)
#define SPINLOCK_GROUP_REMAINDER (CONFIG_SPINLOCK_DYNAMIC_CNT - (((CONFIG_SPINLOCK_DYNAMIC_CNT)/32)*32))
static uint32_t s_mem_manage_bits[SPINLOCK_GROUPS_CNT];

spinlock_t *spinlock_mem_dynamic_alloc()
{
	uint32_t i, j;
	bool find_out = 0;
	spinlock_t *lock_p = NULL;
	uint32_t int_level = rtos_disable_int();
	spin_lock(&s_spinlock_memlock);

#if 0
	static bool inited = false;
	if(inited == false)
	{
		for(i = 0; i < CONFIG_SPINLOCK_DYNAMIC_CNT; i++)
		{
			s_spinlock_mem[i].owner = SPIN_LOCK_FREE;
			s_spinlock_mem[i].count = 0;
		}

		inited = true;
	}
#endif

	for(i = 0; i < SPINLOCK_FULL_GROUPS_CNT; i++)
	{
		for(j=0; j <32; j++)
		{
			if(s_mem_manage_bits[i] & (0x1<<j))
				continue;
			else
			{
				find_out = 1;
				s_mem_manage_bits[i] |= 0x1<<j;
				goto exit;
			}
		}
	}

	for(j = 0; j < SPINLOCK_GROUP_REMAINDER; j++)
	{
		if(s_mem_manage_bits[SPINLOCK_GROUPS_CNT - 1] & (0x1<<j))
			continue;
		else
		{
			find_out = 1;
			s_mem_manage_bits[i] |= 0x1<<j;
			goto exit;
		}
	}

exit:
	if(find_out)
	{
	    spinlock_init(&s_spinlock_mem[(i * 32) + j]);		
        lock_p = &s_spinlock_mem[(i * 32) + j];
	}
	else
	{
		BK_LOGD(NULL, "%s fail:spinlock doesn't free? or increase CONFIG_SPINLOCK_DYNAMIC_CNT\r\n", __func__);
		BK_ASSERT(0);	//please check whether some spinlock doesn't free, or increases CONFIG_SPINLOCK_DYNAMIC_CNT value
	}

	spin_unlock(&s_spinlock_memlock);
	rtos_enable_int(int_level);

	return lock_p;
}

bk_err_t spinlock_mem_dynamic_free(spinlock_t *slock)
{
	uint32_t int_level;
	uint32_t i, j, k;
	if((slock) &&
		((uint32_t)slock >= (uint32_t)&s_spinlock_mem[0]) &&
		((uint32_t)slock <= (uint32_t)&s_spinlock_mem[CONFIG_SPINLOCK_DYNAMIC_CNT]))
	{
		k = slock - &s_spinlock_mem[0];
		i = k/32;
		j = k - (i*32);
	}
	else
	{
		BK_LOGD(NULL, "%s:free ptr=0x%x fail\r\n", __func__, slock);
		return BK_FAIL;
	}

	int_level = rtos_disable_int();
	spin_lock(&s_spinlock_memlock);	
#if CONFIG_SPINLOCK_DEBUG
    spinlock_release_debug_res(slock);
#endif
    s_mem_manage_bits[i] &= ~(0x1<<j);
	spin_unlock(&s_spinlock_memlock);
	rtos_enable_int(int_level);


	return BK_OK;
}

#endif


#endif // CONFIG_SOC_SMP || CONFIG_SOC_BK7239_SMP_TEMP
// eof

