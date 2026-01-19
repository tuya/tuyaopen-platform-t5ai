/**
* @file tkl_init_common.h
* @brief Common process
* @version 1.0
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_INIT_COMMON_H__
#define __TKL_INIT_COMMON_H__

#include "tuya_cloud_types.h"
#ifdef ENABLE_RTC
#include "tkl_rtc.h"
#endif
#ifdef ENABLE_WATCHDOG
#include "tkl_watchdog.h"
#endif
#ifdef ENABLE_FLASH
#include "tkl_flash.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_RTC
/*
 * @brief the description of tuya kernel adapter layer rtc
 */
typedef struct {
    OPERATE_RET     (*init)                 (VOID_T);
    OPERATE_RET     (*deinit)               (VOID_T);
    OPERATE_RET     (*time_get)             (TIME_T *time_sec);
    OPERATE_RET     (*time_set)             (TIME_T  time_sec);
} TKL_RTC_DESC_T;

/**
 * @brief register rtc description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_RTC_DESC_T* tkl_rtc_desc_get(VOID_T);
#endif

#ifdef ENABLE_WATCHDOG
/**
 * @brief the description of tuya kernel adapter layer watchdog
 */
typedef struct {
    UINT_T          (*init)                 (TUYA_WDOG_BASE_CFG_T *cfg);
    OPERATE_RET     (*deinit)               (VOID_T);
    OPERATE_RET     (*refresh)              (VOID_T);
} TKL_WATCHDOG_DESC_T;

/**
 * @brief register watchdog description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_WATCHDOG_DESC_T* tkl_watchdog_desc_get(VOID_T);
#endif

#ifdef ENABLE_FLASH
/**
 * @brief the description of tuya kernel adapter layer flash
 */
typedef struct {
    OPERATE_RET     (*read)                 (CONST UINT_T addr, UCHAR_T *dst, CONST UINT_T size);
    OPERATE_RET     (*write)                (CONST UINT_T addr, CONST UCHAR_T *src, CONST UINT_T size);
    OPERATE_RET     (*erase)                (CONST UINT_T addr, CONST UINT_T size);
    OPERATE_RET     (*lock)                 (CONST UINT_T addr, CONST UINT_T size);
    OPERATE_RET     (*unlock)               (CONST UINT_T addr, CONST UINT_T size);
    OPERATE_RET     (*get_one_type_info)    (TUYA_FLASH_TYPE_E type, TUYA_FLASH_BASE_INFO_T* info);
} TKL_FLASH_DESC_T;

/**
 * @brief register flash description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_FLASH_DESC_T* tkl_flash_desc_get(VOID_T);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_INIT_COMMON_H__



