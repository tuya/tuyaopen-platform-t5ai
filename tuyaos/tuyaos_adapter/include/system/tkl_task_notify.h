#ifndef __TKL_TASK_NOTIFY_H__
#define __TKL_TASK_NOTIFY_H__

#include "tuya_cloud_types.h"
#include "tkl_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TKL_NOTIFY_WAIT_FROEVER 0xFFFFFFFF

typedef enum {
    TUYA_NOTIFY_NO_ACTION = 0,				/* Notify the task without updating its notify value. */
    TUYA_NOTIFY_SETBITS,                    /* Set bits in the task's notification value. */
    TUYA_NOTIFY_INCREMENT,                  /* Increment the task's notification value. */
    TUYA_NOTIFY_SETVALUEWITHOVERWRITE,      /* Set the task's notification value to a specific value even if the previous value has not yet been read by the task. */
    TUYA_NOTIFY_SETVALUEWITHOUTOVERWRITE    /* Set the task's notification value if the previous value has been read by the task. */
}TUYA_TASK_NOTIFY_ACTION_E;

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

OPERATE_RET tkl_task_notify(CONST TKL_THREAD_HANDLE thread, UINT32_T ulValue, TUYA_TASK_NOTIFY_ACTION_E action);

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
UINT32_T tkl_task_notify_take(BOOL_T clearcount, UINT_T timeout);

/**
* @brief  Task Notify Give
*
* @param[in] thread: thread handle
*
* @note This API is used to give thread notify.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_task_notify_give(CONST TKL_THREAD_HANDLE thread);

/**
* @brief  Task Notify Clear
*
* @param[in] thread: thread handle
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_task_notify_state_clear(CONST TKL_THREAD_HANDLE thread);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_TASK_NOTIFY_H__

