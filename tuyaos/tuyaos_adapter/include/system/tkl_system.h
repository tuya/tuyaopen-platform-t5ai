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
uint32_t tkl_system_enter_critical(void);

/**
 * @brief system exit critical
 *
 * @param[in]   irq_mask: irq mask 
 * @return  none
 */
void tkl_system_exit_critical(uint32_t irq_mask);

/**
 * @brief enter critical macro
 */
#define TKL_ENTER_CRITICAL()        \
    uint32_t __irq_mask;              \
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
void tkl_system_reset(void);

/**
* @brief Get system tick count
*
* @param none
*
* @return system tick count
*/
SYS_TICK_T tkl_system_get_tick_count(void);

/**
* @brief Get system millisecond
*
* @param none
*
* @return system millisecond
*/
SYS_TIME_T tkl_system_get_millisecond(void);

/**
* @brief Get system random data
*
* @param[in] range: random from 0  to range
*
* @return random value
*/
int32_t tkl_system_get_random(uint32_t range);

/**
* @brief Get system reset reason
*
* @param[in] describe: point to reset reason describe
*
* @return reset reason
*/
TUYA_RESET_REASON_E tkl_system_get_reset_reason(char** describe);

/**
* @brief  system sleep
*
* @param[in] describe: num ms
*
* @return none
*/
void tkl_system_sleep(uint32_t num_ms);

/**
* @brief  system sleep 
*
* @param[in] describe: num us
*
* @return none
*/
void tkl_system_sleep_us(uint32_t num_us);

/**
* @brief system delay
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return void
*/
void tkl_system_delay(uint32_t num_ms);

/**
* @brief get system cpu info
*
* @param[in] cpu_ary: info of cpus
* @param[in] cpu_cnt: num of cpu
* @note This API is used for system cpu info get.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/

OPERATE_RET tkl_system_get_cpu_info(TUYA_CPU_INFO_T **cpu_ary, int *cpu_cnt);

/**
* @brief Get the device unique id, for use as a deterministic input.
*
* Where the value comes from is platform-defined -- the chip id, the RF
* calibration parameter area, an OTP area, an eFuse area, or any other source
* satisfying the contract. The contract binds the value's properties, not its
* origin, so do not assume it is intrinsic to the silicon.
*
* Distinct in purpose from tkl_system_get_cpu_info()'s chip id, which serves
* production-test identification. See tal_system.h for the full platform
* contract; in short: unique per device, identical across power cycles, resets,
* OTA and factory reset, read-only, readable at KV init time, length in
* 1..32 and never all-0x00 / all-0xFF.
*
* @param[out]   id  caller-provided buffer
* @param[inout] len in: buffer capacity; out: bytes written. Too small -> the
*                   required length is written back and OPRT_INVALID_PARM is
*                   returned; the id is never truncated.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_system_get_hw_unique_id(uint8_t *id, uint8_t *len);

#if (CONFIG_FREERTOS_SMP)
/**
 * @brief system exit critical
 *
 * @return  core ID
 */
uint32_t tkl_system_get_coreid(void);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

