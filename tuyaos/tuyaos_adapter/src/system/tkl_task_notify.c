/**
 * @file tkl_task_notify.c
 * @brief the default weak implements of tuya os thread notify
 * @version 0.1
 * @date 2025-07-24
 *
 * @copyright Copyright 2025-2026 Tuya Inc. All Rights Reserved.
 *
 */
#include "tuya_cloud_types.h"
#include "tkl_task_notify.h"
#include "tkl_thread.h"
#include "FreeRTOS.h"
#include "task.h"

/**
* @brief Task Notify
*
* @param[in] clearcount : true: clear count when exit, false: keep count
* @param[in] timeout: notify max wait time
*
* @note This API is used for task notify.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_task_notify(const TKL_THREAD_HANDLE thread, uint32_t ulValue, TUYA_TASK_NOTIFY_ACTION_E action)
{
    int ret = OPRT_OK;
    BaseType_t res = pdPASS;
    if (!thread) {
        return OPRT_INVALID_PARM;
    }
    if (platform_is_in_interrupt_context() != 0) /* clear event bit in interruption context */
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        res = xTaskNotifyFromISR(thread, ulValue, action, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    } else {
        res = xTaskNotify(thread, ulValue, action);
    }
    if (res != pdPASS)
    {
        ret = OPRT_COM_ERROR;
    }
    
    return ret;
}

/**
* @brief Task Notify Take
*
* @param[in] clearcount : true: clear count when exit, false: keep count
* @param[in] timeout: notify max wait time
*
* @note This API is used for task notify.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
uint32_t tkl_task_notify_take(BOOL_T clearcount,
                                   uint32_t timeout)
{
    uint32_t ret = 0;
    TKL_THREAD_HANDLE thread = xTaskGetCurrentTaskHandle();
    if (!thread) {
        return OPRT_INVALID_PARM;
    }
    if (timeout == TKL_NOTIFY_WAIT_FROEVER) {
        ret = ulTaskNotifyTake(clearcount, portMAX_DELAY);
    } else {
        uint32_t ticks = timeout / portTICK_RATE_MS;

        if (ticks == 0) {
            ticks = 1;
        }
        ret = ulTaskNotifyTake(clearcount, ticks);
    }
    return ret;
}

/**
* @brief  Task Notify Give
*
* @param[in] thread: thread handle
*
* @note This API is used to give thread notify.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_task_notify_give(const TKL_THREAD_HANDLE thread)
{
    OPERATE_RET ret = OPRT_OK;
    if (!thread) {
        return OPRT_INVALID_PARM;
    }
    if (platform_is_in_interrupt_context() != 0) /* clear event bit in interruption context */
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        vTaskNotifyGiveFromISR(thread, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    } else {
        xTaskNotifyGive(thread);
    }
    return ret;
}

/**
* @brief  Task Notify Clear
*
* @param[in] thread: thread handle
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_task_notify_state_clear(const TKL_THREAD_HANDLE thread)
{
    int ret = pdPASS;
    if (!thread) {
        return OPRT_INVALID_PARM;
    }
    ret = xTaskNotifyStateClear( thread );
    return ret;
}
