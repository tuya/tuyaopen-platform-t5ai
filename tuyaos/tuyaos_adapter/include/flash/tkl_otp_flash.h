/**
 * @file tkl_otp_flash.h
 * @brief Common process - adapter the otp_flash api
 * @version 0.1
 * @date 2021-08-06
 *
 * @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TKL_OTP_FLASH_H__
#define __TKL_OTP_FLASH_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_FLASH_TYPE_RF      0
#define OTP_FLASH_TYPE_OPENSDK 1

/**
 * @brief read otp flash
 *
 * @param[in] type: data type
 * @param[out] dst: pointer of buffer
 * @param[in] size: size of buffer
 *
 * @note This API is used for reading flash.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_otp_flash_read(uint8_t type, uint8_t *dst, uint32_t size);

/**
 * @brief write otp flash
 *
 * @param[in] type: data type
 * @param[in] src: pointer of buffer
 * @param[in] size: size of buffer
 *
 * @note This API is used for writing flash.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_otp_flash_write(uint8_t type, const uint8_t *src, uint32_t size);

/**
 * @brief erase otp flash
 *
 * @param[in] type: data type
 * @param[in] size: size of flash block
 *
 * @note This API is used for erasing flash.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_otp_flash_erase(uint8_t type, uint32_t size);
/**
 * @brief lock otp flash
 *
 * @param[in] none
 *
 * @note This API is used for erasing flash.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_otp_flash_lock(void);

/*
 * @brief check if otp flash is locked
 *
 * @param[in] none
 *
 * @note This API is used for checking the lock status of otp flash.
 *
 * @return 1 if locked, 0 if not locked
 */
uint8_t tkl_otp_flash_is_locked(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
