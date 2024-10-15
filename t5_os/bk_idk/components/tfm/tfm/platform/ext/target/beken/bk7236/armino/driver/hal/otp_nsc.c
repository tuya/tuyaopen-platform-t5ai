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
//#include <driver/vault.h>
#include <string.h>
#include "otp_hal.h"
#include "otp.h"
#include <driver/otp_types.h>
#include "_otp.c" //how to link _otp.c?
#include "tfm_secure_api.h"


typedef struct {
	otp_hal_t hal;
} otp_driver_t;

static otp_driver_t s_otp = {0};
static bool s_otp_driver_is_init = false;

void psa_otp_nsc_stub(void)
{
	return 0;
}

static void bk_otp_init(otp_hal_t *hal)
{
	if(s_otp_driver_is_init) {
		return BK_OK;
	}
	hal->hw = (otp_hw_t *)OTP_LL_REG_BASE();
	hal->hw2 = (otp2_hw_t *)OTP2_LL_REG_BASE();

	s_otp_driver_is_init = true;

	return BK_OK;
}

void otp_sleep()
{
	otp_hal_sleep(&s_otp.hal);
}

int otp_active()
{
	bk_otp_init(&s_otp.hal);
	return otp_hal_active(&s_otp.hal);
}

static uint32_t _otp_read_otp(uint32_t location)
{
	return otp_hal_read_otp(&s_otp.hal, location);
}
static uint32_t _otp2_read_otp(uint32_t location)
{
	return otp2_hal_read_otp(&s_otp.hal, location);
}

static void _otp_write_otp(uint32_t location, uint32_t value)
{
	otp_hal_write_otp(&s_otp.hal, location, value);
}

static void _otp2_write_otp(uint32_t location, uint32_t value)
{
	otp2_hal_write_otp(&s_otp.hal, location, value);
}

static uint32_t _otp_read_permission(uint32_t location)
{
	return otp_hal_read_permission(&s_otp.hal, location);
}

static uint32_t _otp2_read_permission(uint32_t location)
{
	return otp2_hal_read_permission(&s_otp.hal, location);
}

static void _otp_write_permission(uint32_t location, uint32_t permission)
{
	otp_hal_write_permission(&s_otp.hal, location, permission);
}

static void _otp2_write_permission(uint32_t location, uint32_t permission)
{
	otp2_hal_write_permission(&s_otp.hal, location, permission);
}

static uint32_t _otp_read_mask(uint32_t location)
{
	return otp_hal_read_mask(&s_otp.hal, location);
}

static uint32_t _otp2_read_mask(uint32_t location)
{
	return otp2_hal_read_mask(&s_otp.hal, location);
}

static void _otp_write_mask(uint32_t location, uint32_t mask)
{
	otp_hal_write_mask(&s_otp.hal, location, mask);
}

static void _otp2_write_mask(uint32_t location, uint32_t mask)
{
	otp2_hal_write_mask(&s_otp.hal, location, mask);
}

#define OTP_ACTIVE() do {\
	if(otp_active() == -1){ \
		otp_sleep(); \
		return BK_FAIL; \
	}\
} while(0)

typedef uint32_t (*otp_read_ptr)(uint32_t);
typedef void (*otp_write_ptr)(uint32_t, uint32_t);

static uint32_t otp_read_permission(uint8_t map_id, uint32_t item)
{
	uint32_t location = otp_map[map_id-1][item].offset / 4;
	uint32_t size = otp_map[map_id-1][item].allocated_size;
	otp_read_ptr read_permission_fun[] = {_otp_read_permission,_otp2_read_permission};

	uint32_t permission = 0;
	for (uint32_t index = 0; index < size / 4; ++index){
		permission |= read_permission_fun[map_id-1](location + index);
	}
	// otp_map[map_id-1][item].privilege = permission;
	return permission;
}

static void otp_write_permission(uint8_t map_id, uint32_t item, otp_privilege_t permission)
{
	uint32_t location = otp_map[map_id-1][item].offset / 4;
	uint32_t size = otp_map[map_id-1][item].allocated_size;
	otp_write_ptr write_permission_fun[] = {_otp_write_permission,_otp2_write_permission};

	for (uint32_t index = 0; index < size / 4; ++index){
		write_permission_fun[map_id-1](location + index, permission);
	}
	// otp_map[map_id-1][item].privilege = permission;
}

bk_err_t otp_read(uint8_t map_id, uint32_t item, uint8_t* buf, uint32_t size)
{
	if(otp_map[map_id-1][item].privilege == OTP_NO_ACCESS){
		return BK_ERR_NO_READ_PERMISSION;
	}
	if(size > otp_map[map_id-1][item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}

	uint32_t location = otp_map[map_id-1][item].offset / 4;
	uint32_t start = otp_map[map_id-1][item].offset % 4;
	uint32_t value;
	int byte_index, word_index;
	otp_read_ptr read_fun[] = {_otp_read_otp,_otp2_read_otp};

	/*read first word*/
	if(size / 4 < 1){
		value = read_fun[map_id-1](location);
		if(value == 0xFFFFFFFF){
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < size; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
		return BK_OK;
	} else {
		value = read_fun[map_id-1](location);
		if(value == 0xFFFFFFFF){
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			buf[byte_index] = (value >> (start + byte_index)*8) & 0xFF;
		}
	}

	/*read full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = read_fun[map_id-1](location + word_index);
		if(value == 0xFFFFFFFF){
			return BK_ERR_NO_READ_PERMISSION;
		}
		for(byte_index = 0;byte_index < 4;++byte_index){
			buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF; // low bit in low address
		}
	}

	if(word_index * 4 == size) {
		return BK_OK;
	}
	/*read tail word*/
	value = read_fun[map_id-1](location + word_index);
	if(value == 0xFFFFFFFF){
		return BK_ERR_NO_READ_PERMISSION;
	}
	for(byte_index = 0;byte_index < (size+start) % 4;++byte_index){
		buf[word_index*4+byte_index-start] = (value >> (byte_index*8)) & 0xFF;
	}
	return BK_OK;
}

bk_err_t otp_update(uint8_t map_id, uint32_t item, uint8_t* buf, uint32_t size)
{
	if(otp_map[map_id-1][item].privilege != OTP_READ_WRITE){
		return BK_ERR_NO_WRITE_PERMISSION;
	}
	if(size > otp_map[map_id-1][item].allocated_size){
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_map[map_id-1][item].offset / 4;
	uint32_t start = otp_map[map_id-1][item].offset % 4;
	uint32_t value = 0;
	uint32_t mask = 0;
	uint32_t verify = 0;
	int byte_index, word_index;
	otp_read_ptr read_fun[] = {_otp_read_otp,_otp2_read_otp};
	otp_write_ptr write_fun[] = {_otp_write_otp,_otp2_write_otp};

	uint8_t* checkbuf = (uint8_t*)os_malloc(size*sizeof(uint8_t));
	if(checkbuf == NULL){
		return BK_ERR_NO_MEM;
	}

	otp_read(map_id, item, checkbuf, size);
	for(byte_index = 0;byte_index < size;++byte_index){
		if(buf[byte_index] != (buf[byte_index] | checkbuf[byte_index])){
			os_free(checkbuf);
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	/*update first word*/
	if(size / 4 < 1) {
		for(byte_index = 0;byte_index < size; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		write_fun[map_id-1](location, value);
		verify = read_fun[map_id-1](location);
		os_free(checkbuf);
		return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
	} else {
		for(byte_index = 0;byte_index < 4 - start; ++byte_index){
			value |= (buf[byte_index] << (byte_index*8));
			mask  |= (0xFF << (byte_index*8));
		}
		value = value << (start * 8);
		mask  = mask << (start * 8);
		write_fun[map_id-1](location, value);
		verify = read_fun[map_id-1](location);
		if ((verify & mask) != value) {
			os_free(checkbuf);
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	/*update full word*/
	for(word_index = 1; word_index < size / 4;++word_index){
		value = 0;
		for(byte_index = 0;byte_index < 4;++byte_index){
			value |= (buf[4*word_index-start+byte_index] << (byte_index*8));
		}
		write_fun[map_id-1](location + word_index,value);
		verify = read_fun[map_id-1](location + word_index);
		if (verify != value) {
			os_free(checkbuf);
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
	}

	if(word_index * 4 == size) {
		return BK_OK;
	}
	/*update tail word*/
	value = 0;
	mask  = 0;
	for(byte_index = 0;byte_index < (size+start) % 4; ++byte_index){
		value |= (buf[word_index*4-start+byte_index] << (byte_index*8));
		mask  |= (0xFF << (byte_index*8));
	}
	write_fun[map_id-1](location + word_index,value);
	verify = read_fun[map_id-1](location + word_index);
	os_free(checkbuf);
	return ((verify & mask) != value) ? BK_ERR_OTP_UPDATE_NOT_EQUAL : BK_OK;
}

/**
 * obtain APB OTP value in little endian with item map_id:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 */
__tfm_psa_secure_gateway_no_naked_attributes__ bk_err_t bk_otp_read_nsc(uint32_t item)
{
	OTP_ACTIVE();
	uint32_t size = otp_map[1][item].allocated_size;
	uint8_t* buf = (uint8_t*)malloc(size*(sizeof(uint8_t)));
	if(buf == NULL)
		return BK_ERR_NO_MEM;
	bk_err_t ret = otp_read(2, item, buf, size);
	free(buf);
	buf = NULL;
	otp_sleep();
	return ret;
}

/**
 * update APB OTP value in little endian with item map_id:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 * 3. check overwritable before write, return BK_ERR_OTP_CANNOT_WRTIE if failed
 * 4. verify value after write, return BK_ERR_OTP_UPDATE_NOT_EQUAL if failed
 */
__tfm_psa_secure_gateway_no_naked_attributes__ bk_err_t bk_otp_update_nsc(uint8_t map_id, uint32_t item, uint8_t* buf, uint32_t size)
{
	OTP_ACTIVE();
	bk_err_t ret = otp_update(map_id, item, buf, size);
	otp_sleep();
	return ret;
}

__tfm_psa_secure_gateway_no_naked_attributes__ bk_err_t bk_otp_read_permission_nsc(uint8_t map_id, uint32_t item, uint32_t* permission)
{
	OTP_ACTIVE();
	*permission = otp_read_permission(map_id, item);
	otp_sleep();
	return BK_OK;
}

__tfm_psa_secure_gateway_no_naked_attributes__ bk_err_t bk_otp_write_permission_nsc(uint8_t map_id, uint32_t item, uint32_t permission)
{
	OTP_ACTIVE();
	otp_write_permission(map_id, item, permission);
	otp_sleep();
	return BK_OK;
}