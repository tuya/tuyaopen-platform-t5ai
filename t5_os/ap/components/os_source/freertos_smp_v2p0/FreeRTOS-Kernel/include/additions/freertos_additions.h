// Copyright 2023 Espressif Systems (Shanghai) PTE LTD
// Copyright 2024 Beken Corporation
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
/*
* Change Logs:
* Date			 Author 	  Notes
* 2024-05-05	 Beken	  adapter to Beken sdk
*/

#pragma once


#include "sdkconfig.h"
#include "FreeRTOS.h"
#include "task.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

#define prvENTER_CRITICAL_OR_MASK_ISR( pxLock, uxStatus ) \
{                                                         \
    taskENTER_CRITICAL_ISR( ( pxLock ) );                 \
    ( void ) ( uxStatus );                                \
}
#define prvEXIT_CRITICAL_OR_UNMASK_ISR( pxLock, uxStatus ) \
{                                                          \
    taskEXIT_CRITICAL_ISR( ( pxLock ) );                   \
    ( void ) ( uxStatus );                                 \
}

/* Macros that enter/exit a critical section only when building for SMP */
#define prvENTER_CRITICAL( pxLock )         taskENTER_CRITICAL( pxLock )
#define prvEXIT_CRITICAL( pxLock )          taskEXIT_CRITICAL( pxLock )
#define prvENTER_CRITICAL_ISR( pxLock )     taskENTER_CRITICAL_ISR( pxLock )
#define prvEXIT_CRITICAL_ISR( pxLock )      taskEXIT_CRITICAL_ISR( pxLock )
#define prvENTER_CRITICAL_SAFE( pxLock )    prvTaskEnterCriticalSafe( pxLock )
#define prvEXIT_CRITICAL_SAFE( pxLock )     prvTaskExitCriticalSafe( pxLock )

static inline __attribute__( ( always_inline ) ) void prvTaskEnterCriticalSafe( portMUX_TYPE * pxLock )
{
    if( portCHECK_IF_IN_ISR() == pdFALSE )
    {
        taskENTER_CRITICAL( pxLock );
    }
    else
    {
        taskENTER_CRITICAL_ISR( pxLock );
    }
}

static inline __attribute__( ( always_inline ) ) void prvTaskExitCriticalSafe( portMUX_TYPE * pxLock )
{
    if( portCHECK_IF_IN_ISR() == pdFALSE )
    {
        taskEXIT_CRITICAL( pxLock );
    }
    else
    {
        taskEXIT_CRITICAL_ISR( pxLock );
    }
}

/* Macros that enable/disable interrupts only when building for SMP */
#define prvDISABLE_INTERRUPTS_ISR_SMP_ONLY()             portSET_INTERRUPT_MASK_FROM_ISR()
#define prvENABLE_INTERRUPTS_ISR_SMP_ONLY( uxStatus )    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxStatus )

void prvTakeKernelLock( void );
void prvReleaseKernelLock( void );
BaseType_t xTaskIncrementTickOtherCores( void );


/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */
