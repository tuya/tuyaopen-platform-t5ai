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
#include <components/ate.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <os/os.h>
#include "bk_pm_model.h"
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "flash_bypass.h"


extern bk_err_t bk_flash_erase_sector(uint32_t address);
extern bk_err_t bk_flash_erase_32k(uint32_t address);
extern bk_err_t bk_flash_erase_block(uint32_t address);

extern bk_err_t flash_partition_addr_check(bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size);
extern bk_err_t flash_partition_write_perm_check(bk_logic_partition_t *partition_info);
#define FLASH_OPERATE_SIZE_AND_OFFSET    (4096)
bk_err_t bk_spec_flash_write_bytes(bk_partition_t partition, const uint8_t *user_buf, uint32_t size,uint32_t offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 *save_flashdata_buff  = NULL;

	bk_ptr = bk_flash_partition_get_info(partition);
	if (bk_ptr == NULL) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
	if (flash_partition_addr_check(bk_ptr, offset, size) != BK_OK )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	if (flash_partition_write_perm_check(bk_ptr) != BK_OK )
	{
		return BK_FAIL;
	}

	if((size + offset) > FLASH_OPERATE_SIZE_AND_OFFSET)
		return BK_FAIL;

	save_flashdata_buff= os_malloc(bk_ptr->partition_length);
	if(save_flashdata_buff == NULL)
	{
		BK_LOGD(NULL,"save_flashdata_buff malloc err\r\n");
		return BK_FAIL;
	}

	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);

	bk_flash_erase_sector(bk_ptr->partition_start_addr);
	os_memcpy((save_flashdata_buff + offset), user_buf, size);
	bk_flash_write_bytes(bk_ptr->partition_start_addr ,(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);

	os_free(save_flashdata_buff);
	save_flashdata_buff = NULL;

	return BK_OK;

}

__attribute__((section(".iram")))
#if CONFIG_ARCH_RISCV
void * __attribute__((no_execit, optimize("-O3"))) bk_memcpy_4w(void *dst, void *src, unsigned int size)
#else
void * __attribute__((optimize("-O3"))) bk_memcpy_4w(void *dst, const void *src, unsigned int size)  /* 4 words copy. */
#endif
{
	unsigned char *dst_ptr = (unsigned char *)dst;
	const unsigned char *src_ptr = (const unsigned char *)src;

	unsigned int temp1, temp2, temp3, temp4;

	if((((unsigned int)src_ptr ^ (unsigned int)dst_ptr) & (sizeof(unsigned int) - 1)) == 0)
	{
		while( (unsigned int)src_ptr & (sizeof(unsigned int) - 1) )
		{
			if(size == 0)
				return dst;

			size--;
			*dst_ptr++ = *src_ptr++;
		}

		const unsigned int *src_wptr = (const unsigned int *)src_ptr;
		unsigned int *dst_wptr = (unsigned int *)dst_ptr;

		while( size >= (sizeof(unsigned int) * 4) )
		{
			temp1 = src_wptr[0];
			temp2 = src_wptr[1];
			temp3 = src_wptr[2];
			temp4 = src_wptr[3];

			dst_wptr[0] = temp1;
			dst_wptr[1] = temp2;
			dst_wptr[2] = temp3;
			dst_wptr[3] = temp4;

			src_wptr += 4;
			dst_wptr += 4;
			size -= (sizeof(unsigned int) * 4);
		}

		while( size >= sizeof(unsigned int) )
		{
			*dst_wptr++ = *src_wptr++;
			size -= sizeof(unsigned int);
		}

		src_ptr = (const unsigned char *)src_wptr;
		dst_ptr = (unsigned char *)dst_wptr;
	}

	while( size > 0 )
	{
		*dst_ptr++ = *src_ptr++;
		size --;
	}

	return dst;
}

static inline bool is_64k_aligned(uint32_t addr)
{
	return ((addr & (KB(64) - 1)) == 0);
}

static inline bool is_32k_aligned(uint32_t addr)
{
	return ((addr & (KB(32) - 1)) == 0);
}

/*make sure wdt is closed!*/
bk_err_t bk_flash_erase_fast(uint32_t erase_off, uint32_t len)
{
	uint32_t erase_size = 0;
	int erase_remain = len;

	while (erase_remain > 0) {
		if ((erase_remain >= KB(64)) && is_64k_aligned(erase_off)) {
			FLASH_LOGV("64k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_block(erase_off);
			erase_size = KB(64);
		} else if ((erase_remain >= KB(32)) && is_32k_aligned(erase_off)) {
			FLASH_LOGV("32k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_32k(erase_off);
			erase_size = KB(32);
		} else {
			FLASH_LOGV("4k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_sector(erase_off);
			erase_size = KB(4);
		}
		erase_off += erase_size;
		erase_remain -= erase_size;
	}

	return BK_OK;
}

#if defined(CONFIG_SECURITY_OTA) && !defined(CONFIG_TFM_FWU)

#include "_ota.h"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include "bk_wdt.h"
#endif

#define CEIL_ALIGN_34(addr)           (((addr) + 34 - 1) / 34 * 34)

extern void     bk_flash_enable_cpu_data_wr(void);
extern void     bk_flash_disable_cpu_data_wr(void);

__attribute__((section(".iram")))
static void *flash_memcpy(void *d, const void *s, size_t n)
{
	return bk_memcpy_4w(d, s, n);
}

__attribute__((section(".iram")))
static void bk_flash_write_cbus_flush(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	volatile uint8_t * temp1;
	volatile uint8_t * temp2;
	uint8_t            temp_data;

	if(size > 64)
	{
		temp1 = (volatile uint8_t *)(0x02000000 + address);
		temp2 = (volatile uint8_t *)(0x02000000 + address + 32);
		temp_data = *temp1;
		temp_data = *temp2;
	}
	else
	{
		if((address & (0x1000 - 1)) > 0x800)
		{
			temp1 = (volatile uint8_t *)(0x02000000 + address - 64);   ///  sure ????
			temp2 = (volatile uint8_t *)(0x02000000 + address - 32);
			temp_data = *temp1;
			temp_data = *temp2;
		}
		else
		{
			temp1 = (volatile uint8_t *)(0x02000000 + address + 64);
			temp2 = (volatile uint8_t *)(0x02000000 + address + 96);
			temp_data = *temp1;
			temp_data = *temp2;
		}
	}

	(void)temp_data;
}

__attribute__((section(".iram")))
static void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	bk_flash_enable_cpu_data_wr();
	flash_memcpy((char*)(0x02000000+address), user_buf,size);

	// flush out the prefecth buffers (2 buffers, buffer len = 32 bytes.).
	bk_flash_write_cbus_flush(address, user_buf,size);
	bk_flash_disable_cpu_data_wr();
}

#if CONFIG_OTA_OVERWRITE
static void bk_flash_overwrite_write_dbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t fa_addr = CONFIG_OTA_PHY_PARTITION_OFFSET;
	uint32_t addr = fa_addr + off;
	bk_flash_write_bytes(addr, src, len);
}

static void bk_flash_overwrite_update(uint32_t off, const void *src, uint32_t len)
{
	bk_flash_overwrite_write_dbus(off, src, len);
}
#endif

__attribute__((section(".iram")))
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	flash_memcpy((char*)user_buf, (const char*)(0x02000000+address),size);
}

#if CONFIG_DIRECT_XIP
__attribute__((section(".iram")))
static void bk_flash_xip_write_cbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET));

	if((fa_off+off) & 0x31)  // MUST aligh with 32-byte.
		return;

	uint32_t int_status =  rtos_disable_int();
#if CONFIG_CACHE_ENABLE
    enable_dcache(0);
#endif
    uint32_t write_addr = (fa_off+off);
    write_addr |= 1 << 24;
    bk_flash_write_cbus(write_addr,src,len);
#if CONFIG_CACHE_ENABLE
    enable_dcache(1);
#endif
	rtos_enable_int(int_status);
}

void bk_flash_xip_write_dbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t update_id = (flash_get_excute_enable() ^ 1);
	uint32_t fa_addr;
	if (update_id  == 0) {
		fa_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET;
	} else {
		fa_addr = CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET;
	}
	uint32_t addr = fa_addr + off;
	bk_flash_write_bytes(addr, src, len);
}

void bk_flash_xip_update(uint32_t off, const void *src, uint32_t len)
{
#if CONFIG_OTA_ENCRYPTED
	bk_flash_xip_write_dbus(off, src, len);
#else
	bk_flash_xip_write_cbus(off, src, len);
#endif
}

static uint32_t boot_xip_magic_off(uint32_t fa_id)
{
	uint32_t phy_offset = 0xFFFFFFFF;
	if(fa_id == 0) {
		phy_offset = CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096);
	} else if (fa_id == 1){
		phy_offset = CEIL_ALIGN_34(CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET + CONFIG_SECONDARY_ALL_PHY_PARTITION_SIZE - 4096);
	}
	return phy_offset;
}

void bk_flash_write_xip_status(uint32_t fa_id, uint32_t type, uint32_t status)
{
	uint32_t offset = boot_xip_magic_off(fa_id) + (type -1 ) * 32;
	const uint8_t * value = (const uint8_t *)&(status);

	bk_flash_write_bytes(offset,value,4);
}
#endif /*CONFIG_DIRECT_XIP*/

#if CONFIG_OTA_CONFIRM_UPDATE
static uint32_t boot_overwrite_magic_off(void)
{
	uint32_t phy_offset = (CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4);
	return phy_offset;
}

void bk_flash_ota_write_confirm(uint32_t status)
{
	uint32_t offset = boot_overwrite_magic_off();
	const uint8_t * value = (const uint8_t *)&(status);

	bk_flash_write_bytes(offset,value,4);
}

void bk_flash_ota_erase_confirm()
{
	bk_flash_erase_fast(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096,4096);
}
#endif
void bk_flash_ota_update(uint32_t off, const void *src, uint32_t len)
{
#if CONFIG_DIRECT_XIP
	bk_flash_xip_update(off, src, len);
#elif CONFIG_OTA_OVERWRITE
	bk_flash_overwrite_update(off, src, len);
#endif
}

void bk_flash_ota_erase(void)
{
#if CONFIG_DIRECT_XIP
	uint32_t update_id = flash_get_excute_enable() ^ 1;
	uint32_t erase_addr;
	if (update_id  == 0) {
		erase_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET;
	} else {
		erase_addr = CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET;
	}
	uint32_t erase_size = CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE;
#elif CONFIG_OTA_OVERWRITE
	bk_flash_erase_fast(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096,4096);
	uint32_t erase_addr = CONFIG_OTA_PHY_PARTITION_OFFSET;
	uint32_t erase_size = CONFIG_OTA_PHY_PARTITION_SIZE;
#endif
#if CONFIG_INT_WDT
	extern bk_err_t bk_wdt_stop(void);
	extern bk_err_t bk_wdt_start(uint32_t timeout_ms);
	bk_wdt_stop();
#endif
	bk_flash_erase_fast(erase_addr,erase_size);
#if CONFIG_INT_WDT
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#endif
}

#if CONFIG_DIRECT_XIP
void bk_flash_ota_write_magic(void)
{
	uint32_t update_id = flash_get_excute_enable() ^ 1;
	bk_flash_write_xip_status(update_id,XIP_MAGIC_TYPE,XIP_SET);
}
#endif

#endif //  CONFIG_SECURITY_OTA && !CONFIG_TFM_FWU

