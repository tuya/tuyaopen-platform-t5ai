/**
* @file tkl_sleep.h
* @brief Common process - adapter the sleep manage api
* @version 0.1
* @date 2021-08-18
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_SLEEP_H__
#define __TKL_SLEEP_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS_INVALID_VALUE                0xffffffff
#define DEEPSLEEP_MAGIC                 0x74796473
#define DS_ENTRY_FLAG                   0x55aa55aa

#define DEEPSLEEP_PARAMETER_ADDRESS     0x7dc000
#define DS_MAX_CFG_ITEM                 6

typedef void (*ds_user_cb)(void);

typedef struct {
    uint32_t magic;             // TYDS: 0x74796473
    uint32_t entry_flag;        // 如果是默认0xffffffff，则保存参数，写入0x55aa55aa,然后重启
                                // 如果是0x55aa55aa，则设置唤醒源，进入深度休眠
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg[DS_MAX_CFG_ITEM];
    ds_user_cb cb;
    uint32_t sum;
}TKL_DS_PARAM_T;

/**
 * @brief sleep callback register
 *
 * @param[in] sleep_cb:  sleep callback
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cpu_sleep_callback_register(TUYA_SLEEP_CB_T *sleep_cb);

/**
 * @brief allow to sleep
 *
 * @param[in] none
 *
 * @return none
 */
void tkl_cpu_allow_sleep(void);

/**
 * @brief force wakeup
 *
 * @param[in] none
 *
 * @return none
 */
void tkl_cpu_force_wakeup(void);

/**
* @brief Set the low power mode of CPU
*
* @param[in] enable: enable switch
* @param[in] mode:   cpu sleep mode
*
* @note This API is used for setting the low power mode of CPU.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode);

// 注册进入deepsleep前用户回调接口，执行deepsleep流程时候，保存注册的回调，
// 进入deepsleep前执行该回调函数
OPERATE_RET tkl_sleep_register_ds_user_cb(ds_user_cb fn);


OPERATE_RET tkl_cpu_sleep_time_set(const uint32_t sleep_ms);


#ifdef __cplusplus
}
#endif

#endif

