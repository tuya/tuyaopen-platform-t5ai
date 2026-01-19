/**
* @file tkl_output.h
* @brief Common process - - adapter the log output api
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_OUTPUT_H__
#define __TKL_OUTPUT_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
* @brief Output log information
*
* @param[in] format: log information
*
* @note This API is used for outputing log information
*
* @return 
*/
VOID_T tkl_log_output(CONST CHAR_T *format, ...);

/**
* @brief Close log port
*
* @param VOID
*
* @note This API is used for closing log port.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_log_close(VOID_T);

/**
* @brief Open log port
*
* @param VOID
*
* @note This API is used for openning log port.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_log_open(VOID_T);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

