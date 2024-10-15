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

//#include <common/bk_include.h>
//#include <driver/otp_types.h>
//#include <driver/vault.h>
#include <string.h>
#include "otp_hal.h"
#include "otp.h"
#include "hal_otp_types.h"

typedef struct {
	otp_hal_t hal;
} otp_driver_t;

typedef struct
{
    const otp_id_t name;
    const uint32_t allocated_size;
    const uint32_t offset;
    otp_privilege_t privilege;
    //bool security;/*TODO*/
} otp_item_t;


otp_item_t otp_apb_map[] = {
	/* otp_id_t name,          allocated_size, offset,      privilege*/

	{OTP_MEMORY_CHECK_MARK,          96,        0x0,       OTP_READ_WRITE},
	{OTP_AES_KEY,                    32,        0x60,      OTP_READ_WRITE},
	/*reserved 128,0x80*/
	{OTP_MODEL_ID,                   4,         0x100,     OTP_READ_WRITE},
	{OTP_MODEL_KEY,                  16,        0x104,     OTP_READ_WRITE},
	{OTP_ARM_DEVICE_ID,              4,         0x114,     OTP_READ_WRITE},
	{OTP_DEVICE_ROOT_KEY,            16,        0x118,     OTP_READ_WRITE},
	{OTP_BL1_BOOT_PUBLIC_KEY_HASH,   32,        0x128,     OTP_READ_WRITE},
	{OTP_BL2_BOOT_PUBLIC_KEY_HASH,   32,        0x148,     OTP_READ_WRITE},
	{OTP_ARM_LCS,                    4,         0x168,     OTP_READ_WRITE},
	{OTP_LOCK_CONTROL,               4,         0x16C,     OTP_READ_WRITE},
	/*reserved 16,0x180*/
	{OTP_BL1_SECURITY_COUNTER,       4,         0x188,     OTP_READ_WRITE},
	{EK1,                            32,        0x18C,     OTP_READ_WRITE},
	{EK2,                            16,        0x1AC,     OTP_READ_WRITE},
	{EK3,                            16,        0x1CC,     OTP_READ_WRITE},
	/*reserved 36,0x1DC*/
	{OTP_BL2_SECURITY_COUNTER,       64,        0x200,     OTP_READ_WRITE},
	{OTP_HUK,                        32,        0x240,     OTP_READ_WRITE},
	{OTP_IAK,                        32,        0x260,     OTP_READ_WRITE},
	{OTP_IAK_LEN,                    4,         0x280,     OTP_READ_WRITE},
	{OTP_IAK_TYPE,                   4,         0x284,     OTP_READ_WRITE},
	{OTP_IAK_ID,                     32,        0x288,     OTP_READ_WRITE},
	{OTP_BOOT_SEED,                  32,        0x2A8,     OTP_READ_WRITE},
	{OTP_LCS,                        4,         0x2C8,     OTP_READ_WRITE},
	{OTP_IMPLEMENTATION_ID,          32,        0x2CC,     OTP_READ_WRITE},
	{OTP_HW_VERSION,                 32,        0x2EC,     OTP_READ_WRITE},
	{OTP_VERIFICATION_SERVICE_URL,   32,        0x30C,     OTP_READ_WRITE},
	{OTP_PROFILE_DEFINITION,         32,        0x32C,     OTP_READ_WRITE},
	{OTP_ENTROPY_SEED,               64,        0x34C,     OTP_READ_WRITE},
	{OTP_SECURE_DEBUG_PK,            2,         0x38C,     OTP_READ_WRITE},
	{OTP_MAC_ADDRESS,                8,         0x3AC,     OTP_READ_WRITE},
	{OTP_VDDDIG_BANDGAP,             2,         0x3B4,     OTP_READ_WRITE},
	{OTP_DIA,                        2,         0x3B6,     OTP_READ_WRITE},
	{OTP_GADC_CALIBRATION,           4,         0x3B8,     OTP_READ_WRITE},
	{OTP_SDMADC_CALIBRATION,         4,         0x3BC,     OTP_READ_WRITE},
	{OTP_DEVICE_ID,                  8,         0x3C0,     OTP_READ_WRITE},
	{OTP_MEMORY_CHECK_VDDDIG,        4,         0x3C8,     OTP_READ_WRITE},
	{OTP_GADC_TEMPERATURE,           2,         0x3CC,     OTP_READ_WRITE},
	{OTP_RANDOM_KEY,                 32,        0x3C8,     OTP_READ_WRITE},
	/*reserved 24,0x3E8*/
};

static otp_driver_t s_otp = {0};

bk_err_t bk_otp_init()
{
	otp_hal_init(&s_otp.hal);

	return BK_OK;
}

bk_err_t bk_otp_deinit()
{
	otp_hal_sleep(&s_otp.hal);

	return BK_OK;
}

bk_err_t bk_otp_read_otp(uint32_t location, uint32_t* value)
{
	*value = otp_hal_read_otp(&s_otp.hal,location);

	return BK_OK;
}

bk_err_t bk_otp2_read_otp(uint32_t location, uint32_t* value)
{
	*value = otp2_hal_read_otp(&s_otp.hal,location);

	return BK_OK;
}

bk_err_t bk_otp_write_otp(uint32_t location, uint32_t value)
{
	uint32_t check_value;

	if(0 != otp_hal_read_otp(&s_otp.hal,location)){
		printf("This location has already been written! Please use overwrite.\r\n");
		return BK_FAIL;
	}
	
	otp_hal_write_otp(&s_otp.hal,location,value);
	check_value = otp_hal_read_otp(&s_otp.hal,location);
	if(value != check_value){
		printf("After write,value = 0x%x.It should be 0x%x!\r\n",check_value,value);
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t bk_otp2_write_otp(uint32_t location, uint32_t value)
{
	uint32_t check_value;

	if(0 != otp2_hal_read_otp(&s_otp.hal,location)){
		printf("This location has already been written! Please use overwrite.\r\n");
		return BK_FAIL;
	}

	otp2_hal_write_otp(&s_otp.hal,location,value);
	check_value = otp2_hal_read_otp(&s_otp.hal,location);
	if(value != check_value){
		printf("After write,value = 0x%x.It should be 0x%x!\r\n",check_value,value);
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t bk_otp_overwrite_otp(uint32_t location, uint32_t value)
{
	uint32_t old_value,new_value;
	old_value = otp_hal_read_otp(&s_otp.hal,location);
	otp_hal_write_otp(&s_otp.hal,location,value);
	new_value = otp_hal_read_otp(&s_otp.hal,location);
	printf("Before write,value is 0x%x,after is 0x%x,should be 0x%x.\r\n",old_value,new_value,value);
	return BK_OK;
}


bk_err_t bk_otp2_overwrite_otp(uint32_t location, uint32_t value)
{
	uint32_t old_value,new_value;

	old_value = otp2_hal_read_otp(&s_otp.hal,location);
	otp2_hal_write_otp(&s_otp.hal,location,value);
	new_value = otp2_hal_read_otp(&s_otp.hal,location);
	printf("Before write,value is 0x%x,after is 0x%x,should be 0x%x.\r\n",old_value,new_value,value);
	return BK_OK;
}
bk_err_t bk_otp_init_puf()
{
	if(otp_hal_read_enroll(&s_otp.hal) == 0x0){
		otp_hal_do_puf_enroll(&s_otp.hal);
	} else {
		printf("Puf has inited\r\n");
		return BK_FAIL;
	}

	if(otp_hal_read_enroll(&s_otp.hal) == 0xF){
		return BK_OK;
	} else {
		printf("Error code:0x%x\r\n",otp_hal_read_status(&s_otp.hal));
		return BK_FAIL;
	}

	return BK_FAIL;
}

bk_err_t bk_otp_read_raw_entropy_output(uint32_t* value)
{
	otp_hal_enable_pdstb(&s_otp.hal);
	otp_hal_enable_clkosc_en(&s_otp.hal);
	otp_hal_enable_fre_cont(&s_otp.hal);
	*value = otp_hal_read_random_number(&s_otp.hal);
	return BK_OK;
}

bk_err_t bk_enable_security_protect()
{
	otp_hal_enable_security_protection(&s_otp.hal);
	return BK_OK;
}


bk_err_t bk_set_puf_permission(uint32_t location)
{
	otp_hal_write_puf_permission(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_set_otp_mask(uint32_t location)
{
	otp_hal_write_otp_mask(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_set_otp2_mask(uint32_t location)
{
	otp_hal_write_otp2_mask(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_set_puf_mask(uint32_t location)
{
	otp_hal_write_puf_mask(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_set_lck_mask(uint32_t location)
{
	otp_hal_write_puf_mask(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_read_otp_mask(uint32_t location, uint32_t* value)
{
	*value = otp_hal_read_otp_mask(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_zeroize_otp(uint32_t location)
{
	otp_hal_zeroize_otp(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_zeroize_puf(uint32_t location)
{
	otp_hal_zeroize_puf(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_get_otp_zeroized_flag(uint32_t location, uint32_t* flag)
{
	*flag = otp_hal_read_otp_zeroized_flag(&s_otp.hal,location);
	return BK_OK;
}

bk_err_t bk_read_otp_permission(uint32_t location, uint32_t* value)
{
	*value = otp_hal_read_otp_permission(&s_otp.hal,location);
	return BK_OK;
}

uint32_t bk_otp_apb_read_permission(otp_id_t item)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t permission = otp_hal_read_otp_permission(&s_otp.hal,location) & otp_hal_read_otp_mask(&s_otp.hal,location);
	printf("item permission: 0x%x, mask: 0x%x\r\n",otp_hal_read_otp_permission(&s_otp.hal,location),otp_hal_read_otp_mask(&s_otp.hal,location));
	if(permission == 0xF) {
		return OTP_NO_ACCESS;
	} else if(permission == 0x3) {
		return OTP_READ_ONLY;
	} else if(permission == 0x0) {
		return OTP_READ_WRITE;
	} else {
		return BK_FAIL;
	}
}

uint32_t bk_otp_apb_write_permission(otp_id_t item, uint32_t permission)
{
	uint32_t location = otp_apb_map[item].offset / 4;
	uint32_t value;
	if(permission == OTP_READ_ONLY){
		value = 0x3;
	} else if(permission == OTP_NO_ACCESS){
		value = 0xF;
	} else{
		return BK_FAIL;
	}
	if(value < otp_hal_read_otp_permission(&s_otp.hal,location)){
		return BK_FAIL;
	}
	for(uint32_t location = otp_apb_map[item].offset / 4; location <= (otp_apb_map[item].offset+otp_apb_map[item].allocated_size) / 4;++location){
		otp_hal_write_otp_permission(&s_otp.hal,location,value);
	}
	otp_apb_map[item].privilege = bk_otp_apb_read_permission(item);
	return otp_apb_map[item].privilege;
}

