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

#include <common/bk_include.h>
#include <driver/otp_types.h>
#include <driver/vault.h>
#include <string.h>
#include "otp_driver.h"

#define OTP_BANK_SIZE   (0x800)
static int s_otp_init_count = 0;
bk_err_t bk_otp_init(void)
{
    if(s_otp_init_count > 0){
        s_otp_init_count++;
        OTP_LOGW("otp init: Already initialized, count %d\n", s_otp_init_count);
        return BK_OK;
    }
	bk_vault_driver_init(MODULE_OTP);
    s_otp_init_count++;

    return BK_OK;
}

bk_err_t bk_otp_deinit(void)
{
    if(s_otp_init_count <= 0){
        bk_printf("otp deinit already");
        return BK_OK;
    }

    s_otp_init_count--;
    if(s_otp_init_count > 0){
        OTP_LOGI("otp deinit not excuted, remain count:%d\n", s_otp_init_count);
    }else{
        bk_vault_driver_deinit(MODULE_OTP);
    }
        return BK_OK;
}

bk_err_t bk_otp_read_bytes_nonsecure(uint8_t *buffer, uint32_t addr, uint32_t len)
{
	if(buffer == NULL)
		return BK_ERR_OTP_READ_BUFFER_NULL;
	
	if((addr > OTP_BANK_SIZE) || ((addr + len) > OTP_BANK_SIZE))
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;

    bk_otp_init();
	memcpy((void *)buffer, (void *)(SOC_VAULT_OTP_BASE + addr), len);
    bk_otp_deinit();

	return BK_OK;
}

