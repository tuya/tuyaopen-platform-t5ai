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

typedef struct {
    uint32_t magic;             // TYDS: 0x74796473
    uint32_t entry_flag;        // 如果是默认0xffffffff，则保存参数，写入0x55aa55aa,然后重启
                                // 如果是0x55aa55aa，则设置唤醒源，进入深度休眠
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg[DS_MAX_CFG_ITEM];
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
VOID_T tkl_cpu_allow_sleep(VOID_T);

/**
 * @brief force wakeup
 *
 * @param[in] none
 *
 * @return none
 */
VOID_T tkl_cpu_force_wakeup(VOID_T);

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



#ifdef __cplusplus
}
#endif

#endif

