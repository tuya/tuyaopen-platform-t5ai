/**
* @file tkl_system.h
* @brief Common process - adpater some api which provide system
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_SYSTEM_H__
#define __TKL_SYSTEM_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief system enter critical
 *
 * @param[in]   none
 * @return  irq mask
 */
UINT_T tkl_system_enter_critical(VOID_T);

/**
 * @brief system exit critical
 *
 * @param[in]   irq_mask: irq mask 
 * @return  none
 */
VOID_T tkl_system_exit_critical(UINT_T irq_mask);

/**
 * @brief enter critical macro
 */
#define TKL_ENTER_CRITICAL()        \
    UINT_T __irq_mask;              \
    __irq_mask = tkl_system_enter_critical()

/**
 * @brief exit critical macro
 */
#define TKL_EXIT_CRITICAL()         \
    tkl_system_exit_critical(__irq_mask)



/**
* @brief system reset
*
* @param none
*
* @return none
*/
VOID_T tkl_system_reset(VOID_T);

/**
* @brief Get system tick count
*
* @param none
*
* @return system tick count
*/
SYS_TICK_T tkl_system_get_tick_count(VOID_T);

/**
* @brief Get system millisecond
*
* @param none
*
* @return system millisecond
*/
SYS_TIME_T tkl_system_get_millisecond(VOID_T);

/**
* @brief Get system random data
*
* @param[in] range: random from 0  to range
*
* @return random value
*/
INT_T tkl_system_get_random(UINT_T range);

/**
* @brief Get system reset reason
*
* @param[in] describe: point to reset reason describe
*
* @return reset reason
*/
TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** describe);

/**
* @brief  system sleep
*
* @param[in] describe: num ms
*
* @return none
*/
VOID_T tkl_system_sleep(UINT_T num_ms);

/**
* @brief  system sleep 
*
* @param[in] describe: num us
*
* @return none
*/
VOID_T tkl_system_sleep_us(UINT_T num_us);

/**
* @brief system delay
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return VOID
*/
VOID_T tkl_system_delay(UINT_T num_ms);

/**
* @brief get system cpu info
*
* @param[in] cpu_ary: info of cpus
* @param[in] cpu_cnt: num of cpu
* @note This API is used for system cpu info get.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/

OPERATE_RET tkl_system_get_cpu_info(TUYA_CPU_INFO_T **cpu_ary, INT_T *cpu_cnt);

/**
 * @brief system exit critical
 *
 * @return  core ID
 */
UINT_T tkl_system_get_coreid(VOID_T);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

