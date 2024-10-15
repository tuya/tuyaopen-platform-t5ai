/**
 *********************************************************************************
 * @file vnd_flash_new.c
 * @brief This file provides all the headers of Flash operation functions..
 *********************************************************************************
 *
 * The MIT License
 * Copyright (c) 2017 BEKEN Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
 *********************************************************************************
*/
#include <common/bk_include.h>
#include <os/os.h>
#include <driver/flash_partition.h>
#include <common/bk_kernel_err.h>
#include <os/mem.h>
#include "partitions.h"


/* Logic partition on flash devices */
const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX] = {
    [BK_PARTITION_BOOTLOADER] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "Bootloader",
        .partition_start_addr      = 0x00000000,
        .partition_length          = 0x0F000,
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
    [BK_PARTITION_APPLICATION] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "Primary Application",
        .partition_start_addr      = 0x11000,
#if CONFIG_FLASH_SIZE_4M
        .partition_length          = 0x264000,//2448K
#else
        .partition_length          = 0x143000,
#endif
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
    [BK_PARTITION_OTA] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "ota",
#if CONFIG_FLASH_SIZE_4M
        .partition_start_addr      = 0x275000,
        .partition_length          = 0x165000, //1428KB
#else
        .partition_start_addr      = 0x132000,
        .partition_length          = 0xAE000, //696KB
#endif
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
#if CONFIG_SUPPORT_MATTER
    [BK_PARTITION_MATTER_FLASH] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "Matter",
        #if CONFIG_FLASH_SIZE_4M
        .partition_start_addr      = 0x3DA000,
        #else
        partition_start_addr       = 0x1CB000,
        #endif
        .partition_length          = 0x15000, //84KB
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
    [BK_PARTITION_USR_CONFIG] =
    {
        .partition_owner = BK_FLASH_EMBEDDED,
        .partition_description = "usr_config",
        .partition_start_addr = 0x3ef000,
        .partition_length = 0xd000, //52KB
        .partition_options = PAR_OPT_EXECUTE_DIS | PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
#else
    [BK_PARTITION_USR_CONFIG] =
    {
        .partition_owner = BK_FLASH_EMBEDDED,
        .partition_description = "usr_config",
        .partition_start_addr = 0x3da000,
        .partition_length = 0x22000, //136KB
        .partition_options = PAR_OPT_EXECUTE_DIS | PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    }, 
#endif
    [BK_PARTITION_EASYFLASH] =
    {
        .partition_owner = BK_FLASH_EMBEDDED,
        .partition_description = "easyflash",
        .partition_start_addr = 0x3fc000,
        .partition_length = 0x2000, //8KB
        .partition_options = PAR_OPT_EXECUTE_DIS | PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
    [BK_PARTITION_RF_FIRMWARE] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "RF Firmware",
#if (CONFIG_FLASH_SIZE_4M)
        .partition_start_addr      = 0x3FE000,
#else
        .partition_start_addr      = CONFIG_SYS_RF_PHY_PARTITION_OFFSET,// for rf related info
#endif
        .partition_length          = CONFIG_SYS_RF_PHY_PARTITION_SIZE,
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
    [BK_PARTITION_NET_PARAM] =
    {
        .partition_owner           = BK_FLASH_EMBEDDED,
        .partition_description     = "NET info",
#if (CONFIG_FLASH_SIZE_4M)
        .partition_start_addr      = 0x3FF000,
#else
        .partition_start_addr      = CONFIG_SYS_NET_PHY_PARTITION_OFFSET,// for net related info
#endif
        .partition_length          = CONFIG_SYS_NET_PHY_PARTITION_SIZE,
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
    },
};