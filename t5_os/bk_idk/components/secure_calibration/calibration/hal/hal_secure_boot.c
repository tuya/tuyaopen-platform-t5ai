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
#include "mem_layout.h"
#include "hal_src_internal.h"

hal_ret_t hal_get_manifest_partition_info(hal_manifest_type_t type,
                                          hal_addr_t *addr,
                                          size_t *size)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION((type == HAL_PRIMARY_MANIFEST) ||
                            (type == HAL_RECOVERY_MANIFEST),
                        HAL_ERR_BAD_PARAM,
                        "Parameter type is invalid: %d\n",
                        type);
    HAL_CHECK_CONDITION(
        addr && size, HAL_ERR_BAD_PARAM, "Parameter addr/size is NULL!\n");

#if CONFIG_BK_BOOT
    hal_manifest_addr_t man = {0};

    hal_read_manifest_address(&man);
    if (type == HAL_PRIMARY_MANIFEST) {
        *addr = man.primary;
        *size = LAYOUT_PRIM_MANIFEST_FLASH_SIZE;
    } else {
        *addr = man.recovery;
        *size = LAYOUT_RECV_MANIFEST_FLASH_SIZE;
    }

#else
    if (type == HAL_PRIMARY_MANIFEST) {
        *addr = LAYOUT_SPI_FLASH_START + LAYOUT_PRIM_MANIFEST_FLASH_OFFSET;
        *size = LAYOUT_PRIM_MANIFEST_FLASH_SIZE;
    } else {
        *addr = LAYOUT_SPI_FLASH_START + LAYOUT_RECV_MANIFEST_FLASH_OFFSET;
        *size = LAYOUT_RECV_MANIFEST_FLASH_SIZE;
    }
#endif
    ret = HAL_OK;
finish:
    return ret;
}
