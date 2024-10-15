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

//#include <common/bk_include.h>
#include "hal_otp_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/* @brief Overview about this API header
 *
 */

/**
 * @brief OTP API
 * @defgroup bk_api_otp OTP API group
 * @{
 */
 
bk_err_t bk_otp_init();

bk_err_t bk_otp_deinit();

bk_err_t bk_otp_read_otp(uint32_t location, uint32_t* value);

bk_err_t bk_otp_write_otp(uint32_t location, uint32_t value);

bk_err_t bk_otp_overwrite_otp(uint32_t location, uint32_t value);

bk_err_t bk_otp_init_puf();

bk_err_t bk_otp_read_raw_entropy_output(uint32_t* value);

bk_err_t bk_enable_security_protect();
bk_err_t bk_set_otp_permission(uint32_t location);

bk_err_t bk_set_puf_permission(uint32_t location);
bk_err_t bk_set_otp_mask(uint32_t location);
bk_err_t bk_set_otp2_mask(uint32_t location);
bk_err_t bk_set_puf_mask(uint32_t location);
bk_err_t bk_set_lck_mask(uint32_t location);

bk_err_t bk_read_otp_mask(uint32_t location, uint32_t* value);
bk_err_t bk_zeroize_otp(uint32_t location);

bk_err_t bk_zeroize_puf(uint32_t location);

bk_err_t bk_get_otp_zeroized_flag(uint32_t location, uint32_t* flag);

bk_err_t bk_read_otp_permission(uint32_t location, uint32_t* value);

bk_err_t bk_otp2_read_otp(uint32_t location, uint32_t* value);
bk_err_t bk_otp2_write_otp(uint32_t location, uint32_t value);
bk_err_t bk_otp2_overwrite_otp(uint32_t location, uint32_t value); 
uint32_t bk_otp_apb_write_permission(otp_id_t item, uint32_t permission);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif


