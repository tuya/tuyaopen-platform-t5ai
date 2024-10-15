/**
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and
 * any derivative work shall retain this copyright notice.
 *
 */
#include "flash_operation.h"
#include "flash/flash_dev.h"

#define __MASK(x, align) ((x) & ((align)-1))
#define __IS_ALIGN(x, align) (0 == __MASK(x, align))
#define _MIN(a, b) (((a) < (b)) ? (a) : (b))

#define _ROUND_DOWN(x, align) ((hal_addr_t)(x) & ~((hal_addr_t)(align)-1))

HAL_BUILD_ASSERT(FLASH_SECTOR_SIZE == HAL_FLASH_SECTOR_SIZE);
extern struct device *spi_flash_device;

static hal_ret_t _flash_read_write(uint32_t offset,
                                   uint8_t *data,
                                   uint32_t len,
                                   uint8_t is_write)
{
    hal_ret_t ret        = HAL_OK;
    uint32_t current_len = 0;

    while (len) {
        current_len = _MIN(FLASH_RW_MAX_DATA_SIZE, len);
        if (is_write) {
            /* set write protection */
            ret = flash_write_protection_set(spi_flash_device, false);
            HAL_CHECK_RET("flash_write_protection_set failed: 0x%x\n", ret);

            ret = flash_write(spi_flash_device, offset, data, current_len);
        } else {
            ret = flash_read(spi_flash_device, offset, data, current_len);
        }
        HAL_CHECK_RET("_flash_read_write %s failed: 0x%x \n",
                      (is_write ? ("write") : ("read")),
                      ret);
        len -= current_len;
        offset += current_len;
        data += current_len;
    }
finish:
    return ret;
}

static hal_ret_t __erase_and_write_sector(uint32_t sector_offset,
                                          const uint8_t *data)
{
    hal_ret_t ret = HAL_OK;

    PAL_ASSERT_MSG(__IS_ALIGN(sector_offset, FLASH_SECTOR_SIZE),
                   "Sector offset 0x%x not aligned!\n",
                   sector_offset);
    PAL_ASSERT_MSG(FLASH_SECTOR_SIZE, "flash sector size is 0!\n");

    ret = flash_write_protection_set(spi_flash_device, false);
    HAL_CHECK_RET("flash_write_protection_set failed: 0x%x\n", ret);
    ret = flash_erase(spi_flash_device, sector_offset, FLASH_SECTOR_SIZE);
    HAL_CHECK_RET("flash_erase failed: 0x%x\n", ret);

    ret = _flash_read_write(
        sector_offset, (uint8_t *)(data), FLASH_SECTOR_SIZE, 1);
    HAL_CHECK_RET("_flash_read_write [write] failed!: 0x%x\n", ret);

finish:
    return ret;
}

static hal_ret_t _flash_write(uint32_t offset,
                              const uint8_t *data,
                              uint32_t len)
{
    hal_ret_t ret                  = HAL_OK;
    uint32_t current_len           = 0;
    uint32_t current_sector_offset = 0;
    uint8_t *tmp_p                 = NULL;
    uint32_t page_head_len         = 0;
    uint32_t page_tail_len         = 0;

    HAL_CHECK_CONDITION(offset < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset: 0x%x\n",
                        offset);
    HAL_CHECK_CONDITION(
        len <= FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((offset + len) <= FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset + len: 0x%x + 0x%x\n",
                        offset,
                        len);

    if (!__IS_ALIGN(offset, FLASH_SECTOR_SIZE)) {
        page_head_len = FLASH_SECTOR_SIZE - __MASK(offset, FLASH_SECTOR_SIZE);
    } else {
        page_head_len = 0;
    }

    page_tail_len = __MASK((offset + len), FLASH_SECTOR_SIZE);

    if (page_head_len || page_tail_len) {
        tmp_p = (uint8_t *)pal_malloc(FLASH_SECTOR_SIZE);
        HAL_CHECK_CONDITION(tmp_p,
                            HAL_ERR_OUT_OF_MEMORY,
                            "Malloc 0x%x out of memory!\n",
                            FLASH_SECTOR_SIZE);
    }
    /* process head */
    if (page_head_len && len) {
        current_len           = _MIN(page_head_len, len);
        current_sector_offset = _ROUND_DOWN(offset, FLASH_SECTOR_SIZE);
        /* read whole page */
        ret = _flash_read_write(
            current_sector_offset, tmp_p, FLASH_SECTOR_SIZE, 0);
        HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);
        /* copy data to target */
        pal_memcpy(
            tmp_p + (FLASH_SECTOR_SIZE - page_head_len), data, current_len);
        /* erase and write */
        ret = __erase_and_write_sector(current_sector_offset, tmp_p);
        HAL_CHECK_RET("__erase_and_write_sector offset: %d failed: 0x%x\n",
                      current_sector_offset,
                      ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    while (len >= FLASH_SECTOR_SIZE) {
        PAL_ASSERT_MSG(__IS_ALIGN(offset, FLASH_SECTOR_SIZE),
                       "offset not sector aligned: 0x%x\n",
                       offset);
        current_sector_offset = offset;
        current_len           = FLASH_SECTOR_SIZE;
        /* erase and write one sector */
        ret = __erase_and_write_sector(current_sector_offset, data);
        HAL_CHECK_RET("__erase_and_write_sector offset: %d failed: 0x%x\n",
                      current_sector_offset,
                      ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    if (page_tail_len && len) {
        PAL_ASSERT_MSG(page_tail_len == len,
                       "tail length != len: 0x%x != 0x%x\n",
                       page_tail_len,
                       len);
        current_len           = page_tail_len;
        current_sector_offset = offset;

        ret = _flash_read_write(
            current_sector_offset, tmp_p, FLASH_SECTOR_SIZE, 0);
        HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);
        /* copy tail data */
        pal_memcpy(tmp_p, data, current_len);
        /* erase and write */
        ret = __erase_and_write_sector(current_sector_offset, tmp_p);
        HAL_CHECK_RET("__erase_and_write_sector offset: %d failed: 0x%x\n",
                      current_sector_offset,
                      ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    PAL_ASSERT_MSG(0 == len, "length != 0!: 0x%x\n", len);
finish:
    if (tmp_p) {
        pal_free(tmp_p);
    }
    return ret;
}

static hal_ret_t _flash_raw_write(uint32_t offset,
                                  const uint8_t *data,
                                  uint32_t len)
{
    hal_ret_t ret                  = HAL_OK;
    uint32_t current_len           = 0;
    uint32_t current_sector_offset = 0;
    uint8_t *tmp_p                 = NULL;
    uint32_t page_head_len         = 0;
    uint32_t page_tail_len         = 0;

    HAL_CHECK_CONDITION(offset < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset: 0x%x\n",
                        offset);
    HAL_CHECK_CONDITION(
        len <= FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((offset + len) <= FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset + len: 0x%x + 0x%x\n",
                        offset,
                        len);

    if (!__IS_ALIGN(offset, FLASH_SECTOR_SIZE)) {
        page_head_len = FLASH_SECTOR_SIZE - __MASK(offset, FLASH_SECTOR_SIZE);
    } else {
        page_head_len = 0;
    }

    page_tail_len = __MASK((offset + len), FLASH_SECTOR_SIZE);

    if (page_head_len || page_tail_len) {
        tmp_p = (uint8_t *)pal_malloc(FLASH_SECTOR_SIZE);
        HAL_CHECK_CONDITION(tmp_p,
                            HAL_ERR_OUT_OF_MEMORY,
                            "Malloc 0x%x out of memory!\n",
                            FLASH_SECTOR_SIZE);
    }

    /* process head */
    if (page_head_len && len) {
        current_len           = _MIN(page_head_len, len);
        current_sector_offset = _ROUND_DOWN(offset, FLASH_SECTOR_SIZE);
        /* read whole page */
        ret = _flash_read_write(
            current_sector_offset, tmp_p, FLASH_SECTOR_SIZE, 0);
        HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);
        /* copy data to target */
        pal_memcpy(
            tmp_p + (FLASH_SECTOR_SIZE - page_head_len), data, current_len);
        /* raw write head page */
        ret = _flash_read_write(
            current_sector_offset, (uint8_t *)(tmp_p), FLASH_SECTOR_SIZE, 1);
        HAL_CHECK_RET("_flash_read_write [write] failed!: 0x%x\n", ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    while (len >= FLASH_SECTOR_SIZE) {
        PAL_ASSERT_MSG(__IS_ALIGN(offset, FLASH_SECTOR_SIZE),
                       "offset not sector aligned: 0x%x\n",
                       offset);
        current_sector_offset = offset;
        current_len           = FLASH_SECTOR_SIZE;
        /* raw write one sector */
        ret = _flash_read_write(
            current_sector_offset, (uint8_t *)(data), current_len, 1);
        HAL_CHECK_RET("_flash_read_write [write] failed!: 0x%x\n", ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    if (page_tail_len && len) {
        PAL_ASSERT_MSG(page_tail_len == len,
                       "tail length != len: 0x%x != 0x%x\n",
                       page_tail_len,
                       len);
        current_len           = page_tail_len;
        current_sector_offset = offset;

        ret = _flash_read_write(
            current_sector_offset, tmp_p, FLASH_SECTOR_SIZE, 0);
        HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);
        /* copy tail data */
        pal_memcpy(tmp_p, data, current_len);
        /* raw write tail page */
        ret = _flash_read_write(
            current_sector_offset, tmp_p, FLASH_SECTOR_SIZE, 1);
        HAL_CHECK_RET("_flash_read_write [write] failed!: 0x%x\n", ret);
        len -= current_len;
        data += current_len;
        offset += current_len;
    }

    PAL_ASSERT_MSG(0 == len, "length != 0!: 0x%x\n", len);
finish:
    if (tmp_p) {
        pal_free(tmp_p);
    }
    return ret;
}

static hal_ret_t _flash_erase_aligned(uint32_t offset, uint32_t len)
{
    hal_ret_t ret        = HAL_OK;
    uint32_t current_len = 0;

    HAL_CHECK_CONDITION(offset < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset: 0x%x\n",
                        offset);
    HAL_CHECK_CONDITION(
        len <= FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((offset + len) <= FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset + len: 0x%x + 0x%x\n",
                        offset,
                        len);

    /* offset and len should be sector aligned */
    HAL_CHECK_CONDITION(__IS_ALIGN(offset, FLASH_SECTOR_SIZE),
                        HAL_ERR_BAD_PARAM,
                        "Invalid offset: 0x%x, not sector aligend\n",
                        offset);
    HAL_CHECK_CONDITION(__IS_ALIGN(len, FLASH_SECTOR_SIZE),
                        HAL_ERR_BAD_PARAM,
                        "Invalid len: 0x%x, not sector aligend\n",
                        len);

    while (len) {
        if (offset == 0 && len == FLASH_MAX_SIZE) {
            /* process whole flash */
            current_len = FLASH_MAX_SIZE;
        } else if (__IS_ALIGN(offset, FLASH_ERASE_BLOCK_SIZE) &&
                   (len >= FLASH_ERASE_BLOCK_SIZE)) {
            /* process block */
            current_len = FLASH_ERASE_BLOCK_SIZE;
        } else if (__IS_ALIGN(offset, FLASH_ERASE_BLOCK32_SIZE) &&
                   (len >= FLASH_ERASE_BLOCK32_SIZE)) {
            /* process block32k erase */
            current_len = FLASH_ERASE_BLOCK32_SIZE;
        } else {
            /* process sector erase */
            current_len = FLASH_SECTOR_SIZE;
        }
        ret = flash_write_protection_set(spi_flash_device, false);
        HAL_CHECK_RET("flash_write_protection_set failed: 0x%x\n", ret);

        ret = flash_erase(spi_flash_device, offset, current_len);
        HAL_CHECK_RET("flash_erase failed: 0x%x\n", ret);

        offset += current_len;
        len -= current_len;
    }
finish:
    return ret;
}

hal_ret_t flash_op_read(hal_addr_t addr, uint8_t *buffer_p, size_t len)
{
    hal_ret_t ret = HAL_OK;

    PAL_LOG_INFO("flash read, addr=%x, len=%d\r\n", addr, len);
    HAL_CHECK_CONDITION(
        buffer_p, HAL_ERR_BAD_PARAM, "Parameter buffer_p is NULL\n");

    HAL_CHECK_CONDITION(
        addr < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid addr: 0x%x\n", addr);
    HAL_CHECK_CONDITION(
        len < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((addr + len) < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr + len: 0x%x + 0x%x\n",
                        addr,
                        len);

    ret = _flash_read_write(addr, buffer_p, len, 0);
    HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);

    ret = HAL_OK;
finish:
    return ret;
}

hal_ret_t flash_op_write(hal_addr_t addr, const uint8_t *buffer_p, size_t len)
{
    hal_ret_t ret = HAL_OK;

#ifdef CONFIG_SKIP_FLASH_WRITE
    PAL_LOG_DEBUG("skip flash write, addr=%x len=%d\n", addr, len);
    return ret;
#endif

    HAL_CHECK_CONDITION(
        buffer_p, HAL_ERR_BAD_PARAM, "Parameter buffer_p is NULL\n");

    HAL_CHECK_CONDITION(
        addr < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid addr: 0x%x\n", addr);
    HAL_CHECK_CONDITION(
        len < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((addr + len) < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr + len: 0x%x + 0x%x\n",
                        addr,
                        len);

    ret = _flash_write(addr, buffer_p, len);
    HAL_CHECK_RET("_flash_write failed: 0x%x \n", ret);

    ret = HAL_OK;
finish:
    return ret;
}

hal_ret_t flash_op_raw_write(hal_addr_t addr, const uint8_t *buffer_p, size_t len)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION(
        buffer_p, HAL_ERR_BAD_PARAM, "Parameter buffer_p is NULL\n");

    HAL_CHECK_CONDITION(
        addr < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid addr: 0x%x\n", addr);
    HAL_CHECK_CONDITION(
        len < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((addr + len) < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr + len: 0x%x + 0x%x\n",
                        addr,
                        len);

    ret = _flash_raw_write(addr, buffer_p, len);
    HAL_CHECK_RET("_flash_raw_write failed: 0x%x \n", ret);

    ret = HAL_OK;
finish:
    return ret;
}

hal_ret_t flash_op_erase(hal_addr_t addr, size_t len)
{
    hal_ret_t ret  = HAL_OK;
    uint8_t *tmp_p = NULL;

    HAL_CHECK_CONDITION(
        addr < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid addr: 0x%x\n", addr);
    HAL_CHECK_CONDITION(
        len < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((addr + len) < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr + len: 0x%x + 0x%x\n",
                        addr,
                        len);

    tmp_p = (uint8_t *)pal_malloc(len);
    HAL_CHECK_CONDITION(tmp_p,
                        HAL_ERR_OUT_OF_MEMORY,
                        "Malloc 0x%x out of memory!\n",
                        FLASH_SECTOR_SIZE);

    pal_memset(tmp_p, 0xFF, len);
    ret = _flash_write(addr, tmp_p, len);
    HAL_CHECK_RET("_flash_write failed: 0x%x \n", ret);

    ret = HAL_OK;
finish:
    if (tmp_p) {
        pal_free(tmp_p);
    }
    return ret;
}

hal_ret_t flash_op_sector_write(hal_addr_t addr,
                                const uint8_t *buffer_p,
                                size_t len)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION(
        buffer_p, HAL_ERR_BAD_PARAM, "Parameter buffer_p is NULL\n");

    HAL_CHECK_CONDITION(
        addr < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid addr: 0x%x\n", addr);
    HAL_CHECK_CONDITION(
        len < FLASH_MAX_SIZE, HAL_ERR_BAD_PARAM, "Invalid len: 0x%x\n", len);
    HAL_CHECK_CONDITION((addr + len) < FLASH_MAX_SIZE,
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr + len: 0x%x + 0x%x\n",
                        addr,
                        len);

    /* addr and len should be sector aligned */
    HAL_CHECK_CONDITION(__IS_ALIGN(addr, FLASH_SECTOR_SIZE),
                        HAL_ERR_BAD_PARAM,
                        "Invalid addr: 0x%x, not sector aligend\n",
                        addr);
    HAL_CHECK_CONDITION(__IS_ALIGN(len, FLASH_SECTOR_SIZE),
                        HAL_ERR_BAD_PARAM,
                        "Invalid len: 0x%x, not sector aligend\n",
                        len);

    ret = _flash_erase_aligned(addr, len);
    HAL_CHECK_RET("_flash_erase_aligned failed: 0x%x \n", ret);

    ret = _flash_read_write(addr, (uint8_t *)buffer_p, len, 1);
    HAL_CHECK_RET("_flash_read_write [read] failed: 0x%x \n", ret);

    ret = HAL_OK;
finish:
    return ret;
}
