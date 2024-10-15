/**
 * Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#include "hal.h"
#include "pal.h"
#include "hal_src_internal.h"
#include "mem_layout.h"
#include "flash_operation.h"

#define __CHECK_PARAMETERS(__state__, __buffer__, __size__, __addr__)          \
    HAL_CHECK_CONDITION((__state__) > 0,                                       \
                        HAL_ERR_GENERIC,                                       \
                        "Invalid hal_mem calling sequence, not init!\n");      \
    HAL_CHECK_CONDITION(                                                       \
        (__buffer__), HAL_ERR_BAD_PARAM, "Parameter buffer is NULL!\n");       \
    HAL_CHECK_CONDITION(_HAL_INTERNAL_MAX_UINT32_VAL - (__size__) >=           \
                            (__addr__),                                        \
                        HAL_ERR_BAD_PARAM,                                     \
                        "Invalid parameters, addr: 0x%x, len: 0x%x\n",         \
                        (__addr__),                                            \
                        (__size__));                                           \
    HAL_CHECK_CONDITION(                                                       \
        ((__addr__) + (__size__)) <=                                           \
            (hal_addr_t)(LAYOUT_SPI_FLASH_START + FLASH_MAX_SIZE),             \
        HAL_ERR_BAD_PARAM,                                                     \
        "Invalid parameters, addr: 0x%x, len: 0x%x\n",                         \
        (__addr__),                                                            \
        (__size__));                                                           \
    HAL_CHECK_CONDITION((__addr__) >= LAYOUT_SPI_FLASH_START,                  \
                        HAL_ERR_BAD_PARAM,                                     \
                        "Invalid address: 0x%x\n",                             \
                        (__addr__));

static int _g_flash_state = 0;

hal_ret_t hal_flash_init(void)
{
    _g_flash_state++;
    return HAL_OK;
}

void hal_flash_cleanup(void)
{
    if (_g_flash_state > 0) {
        _g_flash_state--;
    }
}

hal_ret_t hal_flash_read(hal_addr_t addr, uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    __CHECK_PARAMETERS(_g_flash_state, data, size, addr)

    /* convert the flash address to flash offset */
    addr -= LAYOUT_SPI_FLASH_START;

    ret = flash_op_read(addr, data, size);
    HAL_CHECK_CONDITION(HAL_OK == ret,
                        HAL_ERR_GENERIC,
                        "Flash wrapper read failed: 0x%x\n",
                        addr);
finish:
    return ret;
}

static inline bool is_flash_addr_valid(hal_addr_t addr)
{
    if ( (addr >= LAYOUT_SPI_FLASH_START) && (addr <= (LAYOUT_SPI_FLASH_START + SZ_16M)))
        return true;

    return false;
}

hal_ret_t hal_flash_read_via_cbus(hal_addr_t addr, uint8_t *data, size_t size)
{
    if (!data)
        return HAL_ERR_GENERIC;

    if (size < 0)
        return HAL_ERR_GENERIC;

    if ((!is_flash_addr_valid(addr)) || (!is_flash_addr_valid(addr + size - 1)))
        return HAL_ERR_GENERIC;

    pal_memcpy(data, (uint8_t*)addr, size);

    return HAL_OK;
}

hal_ret_t hal_flash_write(hal_addr_t addr, const uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    __CHECK_PARAMETERS(_g_flash_state, data, size, addr)

    /* convert the flash address to flash offset */
    addr -= LAYOUT_SPI_FLASH_START;

    ret = flash_op_write(addr, data, size);
    HAL_CHECK_CONDITION(HAL_OK == ret,
                        HAL_ERR_GENERIC,
                        "Flash wrapper write failed: 0x%x\n",
                        addr);
finish:
    return ret;
}

hal_ret_t hal_flash_raw_write(hal_addr_t addr, const uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    __CHECK_PARAMETERS(_g_flash_state, data, size, addr)

    /* convert the flash address to flash offset */
    addr -= LAYOUT_SPI_FLASH_START;

    ret = flash_op_raw_write(addr, data, size);
    HAL_CHECK_CONDITION(HAL_OK == ret,
                        HAL_ERR_GENERIC,
                        "Flash wrapper raw write failed: 0x%x\n",
                        addr);
finish:
    return ret;
}

hal_ret_t hal_flash_sector_write(hal_addr_t sector_addr,
                                 const uint8_t *data,
                                 size_t size)
{
    hal_ret_t ret = HAL_OK;

    __CHECK_PARAMETERS(_g_flash_state, data, size, sector_addr)

    /* convert the flash address to flash offset */
    sector_addr -= LAYOUT_SPI_FLASH_START;

    ret = flash_op_sector_write(sector_addr, data, size);
    HAL_CHECK_CONDITION(HAL_OK == ret,
                        HAL_ERR_GENERIC,
                        "Flash wrapper sector write failed: 0x%x\n",
                        sector_addr);
finish:
    return ret;
}
