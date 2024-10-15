// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTP defines
 * @defgroup bk_api_otp_defs macos
 * @ingroup bk_api_otp
 * @{
 */
#define BK_ERR_OTP_READ_BUFFER_NULL   (BK_ERR_OTP_BASE - 1) /**< The pointed buff is NULL */
#define BK_ERR_OTP_ADDR_OUT_OF_RANGE  (BK_ERR_OTP_BASE - 2) /**< OTP address is out of range */

typedef uint8_t otp_unit_t; /**< otp uint id */


typedef enum{
    /*Hardware*/
    OTP_MEMORY_CHECK_MARK = 0,
    OTP_AES_KEY,

    /*ARM*/
    OTP_MODEL_ID,
    OTP_MODEL_KEY,
    OTP_ARM_DEVICE_ID,/*repeated*/
    OTP_DEVICE_ROOT_KEY,
    OTP_BL1_BOOT_PUBLIC_KEY_HASH,
    OTP_BL2_BOOT_PUBLIC_KEY_HASH,
    OTP_ARM_LCS,/*repeated*/
    OTP_LOCK_CONTROL,

    /**/
    OTP_BL1_SECURITY_COUNTER,
    EK1,
    EK2,
    EK3,
    OTP_BL2_SECURITY_COUNTER,
    OTP_HUK,
    OTP_IAK,
    OTP_IAK_LEN,
    OTP_IAK_TYPE,
    OTP_IAK_ID,
    OTP_BOOT_SEED,
    OTP_LCS,
    OTP_IMPLEMENTATION_ID,
    OTP_HW_VERSION,
    OTP_VERIFICATION_SERVICE_URL,
    OTP_PROFILE_DEFINITION,
    OTP_ENTROPY_SEED,
    OTP_SECURE_DEBUG_PK,

    /**/
    OTP_MAC_ADDRESS,
    OTP_VDDDIG_BANDGAP,
    OTP_DIA,
    OTP_GADC_CALIBRATION,
    OTP_SDMADC_CALIBRATION,
    OTP_DEVICE_ID,
    OTP_MEMORY_CHECK_VDDDIG,
    OTP_GADC_TEMPERATURE,
    OTP_APB_TEST,
    OTP_RANDOM_KEY,

} otp_id_t;

typedef enum{
    OTP_AHB_TEST = 0,
}otp2_id_t;

typedef enum{
    OTP_READ_WRITE = 0,
    OTP_READ_ONLY,
    OTP_NO_ACCESS,
} otp_privilege_t;

typedef enum{
    OTP_BUSY = 0x1,
    OTP_ERROR = 0x2,
    OTP_WARNING = 0x4,
    OTP_WRONG = 0x8,
    OTP_FORBID = 0x10,
} otp_status_t;


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

