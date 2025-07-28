#pragma once

#include "sdkconfig.h"


// #define configUSE_QUEUE_SETS                      0	
#define configUSE_TASK_NOTIFICATIONS                 1
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H    1	
#define configSTACK_DEPTH_TYPE                       uint32_t
#define configAPPLICATION_ALLOCATED_HEAP             1
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0

/* ------------------------ Hooks -------------------------- */


#define configRECORD_STACK_HIGH_ADDRESS       1             /* This must be set as the port requires TCB.pxEndOfStack */

/* ------------------- Run-time Stats ---------------------- */

// #ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
//     #define configGENERATE_RUN_TIME_STATS           1       /* Used by vTaskGetRunTimeStats() */
// #endif /* CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS */
// #ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
//     #define configUSE_STATS_FORMATTING_FUNCTIONS    1       /* Used by vTaskList() */
// #endif /* CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS */

// #if !CONFIG_FREERTOS_SMP
//     #if CONFIG_FREERTOS_RUN_TIME_COUNTER_TYPE_U32
//         
//     #elif CONFIG_FREERTOS_RUN_TIME_COUNTER_TYPE_U64
//         #define configRUN_TIME_COUNTER_TYPE    uint64_t
//     #endif /* CONFIG_FREERTOS_RUN_TIME_COUNTER_TYPE_U64 */
// #endif /* !CONFIG_FREERTOS_SMP */
#define configRUN_TIME_COUNTER_TYPE    uint32_t
/* ------------------------ List --------------------------- */

#define configLIST_VOLATILE    volatile                     /* We define List elements as volatile to prevent the compiler from optimizing out essential code */


#define INCLUDE_xTaskGetIdleTaskHandle             1

#define INCLUDE_uxTaskGetStackHighWaterMark        1
#define INCLUDE_xTaskResumeFromISR                 1
#define INCLUDE_xTimerPendFunctionCall             1
#define INCLUDE_xTaskGetSchedulerState             1



// #if CONFIG_FREERTOS_USE_APPLICATION_TASK_TAG
//     #define configUSE_APPLICATION_TASK_TAG    1
// #endif // CONFIG_FREERTOS_USE_APPLICATION_TASK_TAG

/* -------------------- Trace Macros ----------------------- */

/*
 * For trace macros.
 * Note: Include trace macros here and not above as trace macros are dependent on some of the FreeRTOS configs
 */
#ifndef __ASSEMBLER__
    #if CONFIG_FREERTOS_SMP

/* Default values for trace macros added to ESP-IDF implementation of SYSVIEW
 * that is not part of Amazon SMP FreeRTOS. */
        #ifndef traceISR_EXIT
            #define traceISR_EXIT()
        #endif
        #ifndef traceISR_ENTER
            #define traceISR_ENTER( _n_ )
        #endif

        #ifndef traceQUEUE_GIVE_FROM_ISR
            #define traceQUEUE_GIVE_FROM_ISR( pxQueue )
        #endif

        #ifndef traceQUEUE_GIVE_FROM_ISR_FAILED
            #define traceQUEUE_GIVE_FROM_ISR_FAILED( pxQueue )
        #endif

        #ifndef traceQUEUE_SEMAPHORE_RECEIVE
            #define traceQUEUE_SEMAPHORE_RECEIVE( pxQueue )
        #endif
    #endif /* CONFIG_FREERTOS_SMP */
#endif /* def __ASSEMBLER__ */

/* -------------------------------------------------- IDF FreeRTOS -----------------------------------------------------
 * - All IDF FreeRTOS specific configurations
 * ------------------------------------------------------------------------------------------------------------------ */
#define CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID 1

#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    #define configTASKLIST_INCLUDE_COREID                  1
#endif /* CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID */
#ifdef CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS
    #define configTHREAD_LOCAL_STORAGE_DELETE_CALLBACKS    1
#endif /* CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS */
#if CONFIG_FREERTOS_CHECK_MUTEX_GIVEN_BY_OWNER
    #define configCHECK_MUTEX_GIVEN_BY_OWNER               1
#endif /* CONFIG_FREERTOS_CHECK_MUTEX_GIVEN_BY_OWNER */

#define portNUM_PROCESSORS    configNUM_CORES
