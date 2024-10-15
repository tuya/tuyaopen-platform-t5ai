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

#ifndef CONFIG_BK_BOOT

#define _CTRL_CTRL_MAGIC (0x4C725463)
typedef struct _boot_ctrl_data_t {
    uint32_t magic;
    uint32_t boot_flag;
} boot_ctrl_data_t;

hal_ret_t hal_read_preferred_boot_flag(hal_boot_flag_t *flag)
{
    hal_ret_t ret         = HAL_OK;
    boot_ctrl_data_t data = {0};

    HAL_CHECK_CONDITION(flag, HAL_ERR_BAD_PARAM, "Parameter flag is NULL!\n");

    ret = flash_op_read(LAYOUT_CTRL_PARTITION_FLASH_OFFSET,
                        (uint8_t *)(&data),
                        sizeof(boot_ctrl_data_t));
    HAL_CHECK_CONDITION(
        HAL_OK == ret, HAL_ERR_GENERIC, "hal flash read failed!\n");

    if (data.magic == _CTRL_CTRL_MAGIC) {
        if ((data.boot_flag == HAL_BOOT_FLAG_PRIMARY) ||
            (data.boot_flag == HAL_BOOT_FLAG_RECOVERY)) {
            *flag = data.boot_flag;
        } else {
            *flag = HAL_BOOT_FLAG_INVALID;
        }
    } else {
        *flag = HAL_BOOT_FLAG_INVALID;
    }

    /* Over write boot flag if it is invalid */
    /* TODO: should we need this? */
    if (*flag == HAL_BOOT_FLAG_INVALID) {
        data.magic     = _CTRL_CTRL_MAGIC;
        data.boot_flag = HAL_BOOT_FLAG_PRIMARY;
        if (flash_op_write(LAYOUT_CTRL_PARTITION_FLASH_OFFSET,
                           (const uint8_t *)(&data),
                           sizeof(boot_ctrl_data_t)) != HAL_OK) {
            PAL_LOG_ERR("Update boot flag failed!\n");
        }
    }
finish:
    return ret;
}

hal_ret_t hal_write_preferred_boot_flag(const hal_boot_flag_t flag)
{
    hal_ret_t ret         = HAL_OK;
    boot_ctrl_data_t data = {0};

    HAL_CHECK_CONDITION((flag == HAL_BOOT_FLAG_PRIMARY) ||
                            (flag == HAL_BOOT_FLAG_RECOVERY),
                        HAL_ERR_BAD_PARAM,
                        "Parameter flag is invalid: %d!\n",
                        flag);

    data.magic     = _CTRL_CTRL_MAGIC;
    data.boot_flag = flag;

    ret = flash_op_write(LAYOUT_CTRL_PARTITION_FLASH_OFFSET,
                         (const uint8_t *)(&data),
                         sizeof(boot_ctrl_data_t));
    HAL_CHECK_CONDITION(
        HAL_OK == ret, HAL_ERR_GENERIC, "hal flash write failed!\n");

finish:
    return ret;
}

#endif
