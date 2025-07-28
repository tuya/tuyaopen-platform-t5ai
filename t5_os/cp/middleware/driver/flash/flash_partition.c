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

#include <stdlib.h>
#include <string.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_driver.h"
#include "flash_hal.h"

#include <ctype.h>
//#if (CONFIG_PSA_MBEDTLS) || (CONFIG_MBEDTLS_ACCELERATOR) || (CONFIG_MBEDTLS)
//#include "mbedtls/aes.h"
//#endif

#define TAG    "partition"

#define NVS_KEY_SIZE        32 // AES-256
#define DATAUNIT_SIZE       32

#define PARTITION_AMOUNT    50

#define FLASH_PHYSICAL_ADDR_UNIT_SIZE     34
#define FLASH_LOGICAL_ADDR_UNIT_SIZE      32

#define FLASH_PHY_ADDR_VALID(addr)    (((addr) % FLASH_PHYSICAL_ADDR_UNIT_SIZE) < FLASH_LOGICAL_ADDR_UNIT_SIZE)
#define FLASH_PHY_2_LOGICAL(addr)     ((((addr) / FLASH_PHYSICAL_ADDR_UNIT_SIZE) * FLASH_LOGICAL_ADDR_UNIT_SIZE) + ((addr) % FLASH_PHYSICAL_ADDR_UNIT_SIZE))
#define FLASH_LOGICAL_2_PHY(addr)     ((((addr) / FLASH_LOGICAL_ADDR_UNIT_SIZE) * FLASH_PHYSICAL_ADDR_UNIT_SIZE) + ((addr) % FLASH_LOGICAL_ADDR_UNIT_SIZE))

#define SOC_FLASH_BASE_ADDR           0x02000000
#define FLASH_LOGICAL_BASE_ADDR       SOC_FLASH_BASE_ADDR

#define PARTITION_IRAM         __attribute__((section(".iram")))

/* Logic partition on flash devices */
static const bk_logic_partition_t bk_flash_partitions[] = BK_FLASH_PARTITIONS_MAP;

static bool flash_partition_is_valid(bk_partition_t partition)
{
	if ((partition >= BK_PARTITION_BOOTLOADER)
		&& (partition < ARRAY_SIZE(bk_flash_partitions))) {
		return true;
	} else {
		return false;
	}
}

static int is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static uint32_t piece_address(uint8_t *array,uint32_t index)
{
	return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

static uint16_t short_address(uint8_t *array,uint16_t index)
{
	return ((uint16_t)(array[index]) << 8 | (uint16_t)(array[index+1]));
}

int toHex(char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	} else {
		return 0;
	}
}

uint8_t toHexByte(const char* c) {
	return 16 * toHex(c[0]) + toHex(c[1]);
}

void toHexStream(const char* src, uint8_t* dest, uint32_t* dest_len) {
	uint32_t cnt = 0;
	const char* p = src;
	while (*p != '\0' && *(p + 1) != '\0') {
		dest[cnt++] = toHexByte(p);
		p += 2;
	}
	*dest_len = cnt;
}

void generate_iv(uint8_t *iv, size_t unit_num) {
    memset(iv, 0, 16);
    for (int k = 0; k < 16 && k < DATAUNIT_SIZE; k++) {
        if (8 * k >= 32) continue;
        iv[15 - k] = (unit_num >> (8 * k)) & 0xFF;
    }
    for (int j = 0; j < 8; j++) {
        u8 temp = iv[j];
        iv[j] = iv[15 - j];
        iv[15 - j] = temp;
    }
}

static bk_logic_partition_t * flash_partition_get_info_by_addr(uint32_t addr)
{
	const bk_logic_partition_t *pt;

	for(int i = 0; i < ARRAY_SIZE(bk_flash_partitions); i++)
	{
		pt = &bk_flash_partitions[i];

		if(addr < pt->partition_start_addr)
			continue;
		if(addr >= (pt->partition_start_addr + pt->partition_length))
			continue;

		return (bk_logic_partition_t *)pt;
	}

	return NULL;
}

bk_logic_partition_t *bk_flash_partition_get_info(bk_partition_t partition)
{
	bk_logic_partition_t *pt = NULL;

	BK_ASSERT(BK_PARTITION_BOOTLOADER < BK_PARTITIONS_TABLE_SIZE);

	if (flash_partition_is_valid(partition)) {
	    pt = (bk_logic_partition_t *)&bk_flash_partitions[partition];
	}
    else
    {
        FLASH_LOGW("partition:0x%d is not valid.\r\n",partition);
    }
	return pt;
}

bk_err_t flash_partition_addr_check(bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
#if (CONFIG_FLASH_PARTITION_CHECK_VALID)
	if ( (offset >= partition_info->partition_length)
		|| (size > partition_info->partition_length)
		|| (offset + size > partition_info->partition_length) )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
#endif

	return BK_OK;
}

/*********************************************
 *  this function MUST reside in the flash.
 *  it will use itself address to check partition write permission.
 */
bk_err_t flash_partition_write_perm_check(bk_logic_partition_t *partition_info)
{
#if (CONFIG_FLASH_PARTITION_CHECK_VALID)
	if((partition_info->partition_options & PAR_OPT_WRITE_EN) == 0)
		return BK_FAIL;

	// flash ctrl only can read/write 16MB.
	uint32_t   fun_flash_logical_addr = ((uint32_t)flash_partition_write_perm_check) & (FLASH_MAX_SIZE - 1) ;
	uint32_t   fun_flash_phy_addr = FLASH_LOGICAL_2_PHY(fun_flash_logical_addr);

	if(fun_flash_phy_addr < partition_info->partition_start_addr)
	{
		return BK_OK;  // not write current running partition.
	}

	if(fun_flash_phy_addr > (partition_info->partition_start_addr + partition_info->partition_length))
	{
		return BK_OK;  // not write current running partition.
	}

	return BK_FAIL;  // not permit to write current running partition.
#else
	return BK_OK;
#endif

}

static bk_err_t bk_flash_partition_erase_internal(bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
    if (size == 0)
		return BK_OK;

    uint32_t erase_addr = 0;
    uint32_t start_sector, end_sector = 0;

    start_sector = offset >> FLASH_SECTOR_SIZE_OFFSET; /* offset / FLASH_SECTOR_SIZE */
    end_sector = (offset + size - 1) >> FLASH_SECTOR_SIZE_OFFSET;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        erase_addr = partition_info->partition_start_addr + (i << FLASH_SECTOR_SIZE_OFFSET);

        bk_flash_erase_sector(erase_addr);
    }

    return BK_OK;
}

bk_err_t bk_flash_partition_erase(bk_partition_t partition, uint32_t offset, uint32_t size)
{
    bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);

    if (partition_info == NULL) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
	if (flash_partition_addr_check(partition_info, offset, size) != BK_OK )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	if (flash_partition_write_perm_check(partition_info) != BK_OK )
	{
		return BK_FAIL;
	}


    return bk_flash_partition_erase_internal(partition_info, offset, size);
}

static bk_err_t bk_flash_partition_write_internal(bk_logic_partition_t *partition_info, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(buffer);

    uint32_t start_addr;

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }
    if ((offset + buffer_len) <= partition_info->partition_length) {
        bk_flash_write_bytes(start_addr, buffer, buffer_len);
    }
    return BK_OK;
}

bk_err_t bk_flash_partition_write(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);
    if (NULL == partition_info) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
	if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	if (flash_partition_write_perm_check(partition_info) != BK_OK )
	{
		return BK_FAIL;
	}

    return bk_flash_partition_write_internal(partition_info, buffer, offset, buffer_len);
}

static bk_err_t bk_flash_partition_read_internal(bk_logic_partition_t *partition_info, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(out_buffer);

    uint32_t start_addr;

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    bk_flash_read_bytes(start_addr, out_buffer, buffer_len);

    return BK_OK;
}

bk_err_t bk_flash_partition_read(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);
    if (NULL == partition_info) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
	if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

    uint32_t aligned_offset = offset & ~0x1F;
    uint32_t aligned_end = (offset + buffer_len + 31) & ~0x1F;

    uint32_t aligned_length = aligned_end - aligned_offset;
    uint8_t *aligned_buffer = (uint8_t *)malloc(aligned_length);
    if (aligned_buffer == NULL) {
        return BK_ERR_NO_MEM;
    }

    bk_err_t read_status = bk_flash_partition_read_internal(partition_info, aligned_buffer, aligned_offset, aligned_length);

    if (read_status != BK_OK) {
        free(aligned_buffer);
        return read_status;
    }
    memcpy(out_buffer, aligned_buffer + (offset - aligned_offset), buffer_len);
    free(aligned_buffer);

    return BK_OK;
}

extern void * bk_memcpy_4w(void *dst, const void *src, unsigned int size);

#if CONFIG_FLASH_CBUS_RW
bk_err_t bk_flash_partition_read_cbus(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(out_buffer);

	uint32_t in_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;

	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partiion not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

	flash_base = FLASH_LOGICAL_BASE_ADDR;
	start_addr = partition_info->partition_start_addr;
	if(!FLASH_PHY_ADDR_VALID(start_addr))
		return BK_FAIL;

	int ret = flash_partition_addr_check(partition_info, FLASH_LOGICAL_2_PHY(offset), FLASH_LOGICAL_2_PHY(buffer_len));

	if(ret != BK_OK)
		return ret;

	in_ptr = flash_base + FLASH_PHY_2_LOGICAL(start_addr) + offset;

	memcpy(out_buffer, (void *)in_ptr, buffer_len);

	return BK_OK;
}

bk_err_t bk_flash_partition_write_cbus(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(buffer);

	uint32_t wr_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;

	GLOBAL_INT_DECLARATION();
	FLASH_LOGI("bk_flash_partition_write_enhanced:0x%x\r\n", buffer_len);
	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partition not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

	flash_base = SOC_FLASH_DATA_BASE;
	start_addr = partition_info->partition_start_addr;
	wr_ptr = flash_base + FLASH_PHY_2_LOGICAL(start_addr) + offset;
	if((offset + buffer_len) > partition_info->partition_length) {
		FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	GLOBAL_INT_DISABLE();

	flash_protect_type_t  partition_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	if((offset + buffer_len) <= partition_info->partition_length) {
		bk_memcpy_4w((void *)wr_ptr, buffer, buffer_len);
	}
	bk_flash_set_protect_type(partition_type);

	GLOBAL_INT_RESTORE();

	return BK_OK;
}
#endif

bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code)
{
	if(magic_code != FLASH_API_MAGIC_CODE)
		return BK_FAIL;

	bk_logic_partition_t *partition_info = flash_partition_get_info_by_addr(addr);

	if (NULL == partition_info) {
		return BK_FAIL;
	}

	uint32_t   offset = addr - partition_info->partition_start_addr;

	bk_err_t   ret_val = flash_partition_addr_check(partition_info, offset, size);

	if(ret_val != BK_OK)
		return ret_val;

	return flash_partition_write_perm_check(partition_info);
}

