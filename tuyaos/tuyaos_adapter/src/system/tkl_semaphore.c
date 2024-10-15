/**
 * @file tkl_semaphore.c
 * @brief the default weak implements of tuya os semaphore, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_semaphore.h"
#include "tkl_system.h"

// #include "portmacro.h"
#include "projdefs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// #include <semaphore.h>
#include <time.h>

typedef struct
{
    xSemaphoreHandle sem;
}TKL_SEM_MANAGE,*P_TKL_SEM_MANAGE;

/**
* @brief Create semaphore
*
* @param[out] handle: semaphore handle
* @param[in] sem_cnt: semaphore init count
* @param[in] sem_max: semaphore max count
*
* @note This API is used for creating semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_create_init(TKL_SEM_HANDLE *handle, const uint32_t sem_cnt, const uint32_t sem_max)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    *handle = (TKL_SEM_HANDLE)xSemaphoreCreateCounting(sem_max, sem_cnt);
    if (NULL == *handle) {
        return OPRT_OS_ADAPTER_SEM_CREAT_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief Wait semaphore
*
* @param[in] handle: semaphore handle
* @param[in] timeout: wait timeout, SEM_WAIT_FOREVER means wait until get semaphore
*
* @note This API is used for waiting semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_wait(const TKL_SEM_HANDLE handle, const uint32_t timeout)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    BaseType_t ret;
    if (timeout == TKL_SEM_WAIT_FOREVER) {
        ret = xSemaphoreTake(handle, portMAX_DELAY);
    } else {
        uint32_t ticks = timeout / portTICK_RATE_MS;

        if (ticks == 0) {
            ticks = 1;
        }
        ret = xSemaphoreTake(handle, ticks);
    }
    if (pdTRUE != ret) {
        return OPRT_OS_ADAPTER_SEM_WAIT_FAILED;
    }

    return OPRT_OK;
}
extern uint32_t platform_is_in_interrupt_context( void );
/**
* @brief Post semaphore
*
* @param[in] handle: semaphore handle
*
* @note This API is used for posting semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_post(const TKL_SEM_HANDLE handle)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    P_TKL_SEM_MANAGE sem_manage;
    sem_manage = (P_TKL_SEM_MANAGE)handle;

    BaseType_t ret;
    if (0 == platform_is_in_interrupt_context()) {
        ret = xSemaphoreGive(sem_manage->sem);
    } else {
        signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        ret = xSemaphoreGiveFromISR(sem_manage->sem,
                                    &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    if (pdTRUE != ret) {
        return OPRT_OS_ADAPTER_SEM_POST_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief Release semaphore
*
* @param[in] handle: semaphore handle
*
* @note This API is used for releasing semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_release(const TKL_SEM_HANDLE handle)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    vSemaphoreDelete(handle);

    return OPRT_OK;
}


