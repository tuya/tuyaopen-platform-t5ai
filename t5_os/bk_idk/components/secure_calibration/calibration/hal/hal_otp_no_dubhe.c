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
#ifndef ARM_CE_DUBHE_OTP

#include "hal.h"
#include "pal.h"
#include "mem_layout.h"
#include "hal_src_internal.h"
#include "flash_operation.h"

/**
 * In our simulated OTP, the OTP bits changing rule is:
 * Only can be changed from 0 --> 1
 * For customers has the opposite OTP logic, please also change the hal_otp
 * implementation.
 */

#if CONFIG_FPGA
 #define _HAL_OTP_BASE_ADDR (LAYOUT_SIMU_OTP_FLASH_OFFSET + 256)
#else
#define _HAL_OTP_BASE_ADDR (LAYOUT_SIMU_OTP_FLASH_OFFSET)
#endif

static int _g_otp_state = 0;

/**
 * @ingroup	hal_otp
 * @brief	hal_otp_init function
 */

hal_ret_t hal_otp_init(void)
{
    _g_otp_state++;
    return HAL_OK;
}

/**
 * @ingroup	hal_otp
 * @brief	hal_otp_cleanup function
 */

void hal_otp_cleanup(void)
{
    if (_g_otp_state > 0) {
        _g_otp_state--;
    }
    return;
}

/* Convert otp data to opposite */
static inline void _convert_otp_data(uint8_t *old_data,
                                     uint8_t *new_data,
                                     size_t size)
{
#if CONFIG_REVERT_OTP
    for (int i = 0; i < size; i++)
        new_data[i] = ~old_data[i];
#endif
}

hal_ret_t hal_otp_read(hal_addr_t offset, uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION(_g_otp_state > 0,
                        HAL_ERR_GENERIC,
                        "Invalid hal_otp calling sequence, not init!\n");

    HAL_CHECK_CONDITION(
        data && size, HAL_ERR_BAD_PARAM, "Parameter data/size is 0!\n");

    ret = flash_op_read((uint32_t)_HAL_OTP_BASE_ADDR + offset, data, size);
    HAL_CHECK_CONDITION(
        HAL_OK == ret, HAL_ERR_GENERIC, "flash wrapper read failed!\n");

    PAL_LOG_DEBUG("otp read, offset=%d size=%d\r\n", offset, size);
    PAL_LOG_DUMP_BUF("otp read:", data, size);
    _convert_otp_data(data, data, size);
finish:
    return ret;
}

/* Check that we are writing from 0 to 1 */
static inline bool _check_otp_write_rule(uint8_t *old_data,
                                         uint8_t *new_data,
                                         size_t size)
{
    int i, j;

    for (i = 0; i < size; i++) {
        for (j = 0; j < 8; j++) {
            /* are we writing from 1 to 0 ? */
            if ((old_data[i] & (1 << j)) && (!(new_data[i] & (1 << j)))) {
                PAL_LOG_ERR("Invalid OTP write: 0x%x --> 0x%x\n",
                            old_data[i],
                            new_data[i]);
                return false;
            }
        }
    }
    return true;
}

hal_ret_t hal_otp_write(hal_addr_t offset, const uint8_t *data, size_t size)
{
    hal_ret_t ret        = HAL_OK;
    uint8_t *otp_rw_data = NULL;

    HAL_CHECK_CONDITION(_g_otp_state > 0,
                        HAL_ERR_GENERIC,
                        "Invalid hal_otp calling sequence, not init!\n");
    HAL_CHECK_CONDITION(
        data && size, HAL_ERR_BAD_PARAM, "Parameter data/size is 0!\n");

    otp_rw_data = pal_malloc(size);
    HAL_CHECK_CONDITION(otp_rw_data,
                        HAL_ERR_OUT_OF_MEMORY,
                        "Malloc %zd out of memory!\n",
                        size);

    ret = flash_op_read(
        (uint32_t)_HAL_OTP_BASE_ADDR + offset, otp_rw_data, size);
    HAL_CHECK_CONDITION(
        HAL_OK == ret, HAL_ERR_GENERIC, "flash wrapper read failed!\n");
    _convert_otp_data(otp_rw_data, otp_rw_data, size);

    HAL_CHECK_CONDITION(
        _check_otp_write_rule(otp_rw_data, (uint8_t *)data, size),
        HAL_ERR_BAD_PARAM,
        "Invalid OTP write data write: write from 1 to 0!\n");

    _convert_otp_data((uint8_t *)data, otp_rw_data, size);
    ret = flash_op_raw_write((uint32_t)_HAL_OTP_BASE_ADDR + offset,
                             (const uint8_t *)otp_rw_data,
                             size);
    HAL_CHECK_CONDITION(
        HAL_OK == ret, HAL_ERR_GENERIC, "flash wrapper write failed!\n");

    ret = HAL_OK;
finish:
    if (otp_rw_data) {
        pal_free(otp_rw_data);
    }
    return ret;
}

hal_ret_t hal_gen_psk(uint8_t *psk, size_t psk_size)
{
    if ((!psk) || (!psk_size)) {
        PAL_LOG_ERR("Parameter is NULL!\n");
        return HAL_ERR_BAD_PARAM;
    }
    return HAL_OK;
}
hal_ret_t hal_otp_write_user_data(const uint8_t *user_data,
                                  size_t user_data_size)
{
    if ((!user_data) || (!user_data_size)) {
        PAL_LOG_ERR("Parameter is NULL!\n");
        return HAL_ERR_BAD_PARAM;
    }
    return HAL_OK;
}

#endif /* !ARM_CE_DUBHE_OTP */
