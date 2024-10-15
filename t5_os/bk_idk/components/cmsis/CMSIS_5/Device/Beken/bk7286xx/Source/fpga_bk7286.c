/*
 * Copyright (c) 2009-2019 Arm Limited.
 * Copyright (c) 2019-2020 ArmChina.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "components/log.h"
#include "components/system.h"
#include "bk7286xx.h"
#include "aon_pmu_driver.h"
#include "sys_driver.h"
#include "driver/uart.h"
#include "wdt_driver.h"
#include "bk_pm_internal_api.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#include "stack_base.h"

#include <driver/wdt.h>
#include <os/os.h>
#include "bk_aon_wdt.h"

#define TAG "arch"

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
static void CacheInit(void)
{
    #define MPU_CTRL    *((volatile unsigned long*)(0xE000ED94UL))
    #define MPU_RNR     *((volatile unsigned long*)(0xE000ED98UL))
    #define MPU_RBAR    *((volatile unsigned long*)(0xE000ED9CUL))
    #define MPU_RLAR    *((volatile unsigned long*)(0xE000EDA0UL))
    #define MPU_RBAR_A  *((volatile unsigned long*)(0xE000EDA4UL))
    #define MPU_RLAR_A  *((volatile unsigned long*)(0xE000EDA8UL))
    #define MPU_RBAR_B  *((volatile unsigned long*)(0xE000EDACUL))
    #define MPU_RLAR_B  *((volatile unsigned long*)(0xE000EDB0UL))
    #define MPU_RBAR_C  *((volatile unsigned long*)(0xE000EDB4UL))
    #define MPU_RLAR_C  *((volatile unsigned long*)(0xE000EDB8UL))
    #define MPU_MAIR0   *((volatile unsigned long*)(0xE000EDC0UL))
    #define MPU_MAIR1   *((volatile unsigned long*)(0xE000EDC4UL))
    #define VTOR_BASE   *((volatile unsigned long*)(0xE000ED08UL))

	MPU_CTRL   = 0x0;
	MPU_MAIR0  = 0xCC4FFF44;
	MPU_MAIR1  = 0xEECCCCCC;
	MPU_RNR    = 0x00;
	MPU_RBAR   = (0x02000000 ) | (0x00 << 3) | (0x01 << 1) | 0x00;// (0x00 << 3) Non-shareable;   (0x01 << 1)  0b01 Read/write by any privilege level.
	MPU_RLAR   = (0x027FFFE0 ) | (0x01 << 1) | 0x01;              // select 0x2; | 0x01 enable region;

    MPU_RNR    = 0x01;
	MPU_RBAR_A = (0x28000000 ) | (0x00 << 3) | (0x01 << 1) | 0x00;// (0x00 << 3) Non-shareable;   (0x01 << 1)  0b01 Read/write by any privilege level.
	MPU_RLAR_A = (0x280FFFE0 ) | (0x00 << 1) | 0x01;              // select 0x0; | 0x01 enable region;

	MPU_RNR    = 0x02;	
    MPU_RBAR_B = (0x60000000 ) | (0x00 << 3) | (0x01 << 1) | 0x00;// (0x00 << 3) Non-shareable;   (0x01 << 1)  0b01 Read/write by any privilege level.
	MPU_RLAR_B = (0x600FFFE0 ) | (0x01 << 1) | 0x01;              // select 0x2; | 0x01 enable region;
    MPU_CTRL   = 0x5;

    __ISB();
    __DSB();

	SCB_DisableICache();
	SCB_DisableDCache();	
	SCB_EnableICache();
	SCB_EnableDCache();
    SCB_CleanInvalidateDCache();
}

static void FPUInit(void)
{
    #define portCPACR               ( ( volatile uint32_t * ) 0xe000ed88 ) /* Coprocessor Access Control Register. */
    #define portCPACR_CP10_VALUE    ( 3UL )
    #define portCPACR_CP11_VALUE    portCPACR_CP10_VALUE
    #define portCPACR_CP10_POS      ( 20UL )
    #define portCPACR_CP11_POS      ( 22UL )

    #define portFPCCR               ( ( volatile uint32_t * ) 0xe000ef34 ) /* Floating Point Context Control Register. */
    #define portFPCCR_ASPEN_POS     ( 31UL )
    #define portFPCCR_ASPEN_MASK    ( 1UL << portFPCCR_ASPEN_POS )
    #define portFPCCR_LSPEN_POS     ( 30UL )
    #define portFPCCR_LSPEN_MASK    ( 1UL << portFPCCR_LSPEN_POS )

    /* CP10 = 11 ==> Full access to FPU i.e. both privileged and
     * unprivileged code should be able to access FPU. CP11 should be
     * programmed to the same value as CP10. */
    *(portCPACR) |= ((portCPACR_CP10_VALUE << portCPACR_CP10_POS) | (portCPACR_CP11_VALUE << portCPACR_CP11_POS));

    /* ASPEN = 1 ==> Hardware should automatically preserve floating point
     * context on exception entry and restore on exception return.
     * LSPEN = 1 ==> Enable lazy context save of FP state. */
    *(portFPCCR) |= (portFPCCR_ASPEN_MASK | portFPCCR_LSPEN_MASK);
}

void pre_system_init(void)
{
    CacheInit();
    FPUInit();

    for(int i = 0; i < 80; i++){
		 __NVIC_EnableIRQ(i);
	}
}



