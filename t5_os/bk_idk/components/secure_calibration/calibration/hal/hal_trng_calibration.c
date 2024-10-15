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
#include "string.h"
#include "hal_platform.h"
#include "hal_trng_calibration.h"

#define DEFAULT_TRNG_CALIBRATION_AGENT_TIMEOUT_SECOND (120) /* 120 second times out */

HAL_API bool hal_trng_calibration_is_enabled(void)
{
    uint8_t user_input = 0;

    if (hal_uart_rx_ready()) {
        hal_uart_getc(&user_input);
    } else {
        return false;
    }

    if (user_input == 't') {
        return true;
    } else {
        return false;
    }
}

HAL_API void hal_trng_calibration_indicate_on(void)
{
    /* user defined trng calibration indicate on */
    return;
}

HAL_API void hal_trng_calibration_indicate_off(void)
{
    /* user defined trng calibration indicate off */
    return;
}

/**
 *       timeout > 0, unit second;
 *       timeout = -1, no timeout, wait forever
 */
HAL_API void hal_trng_calibration_get_timeout_setting(uint32_t *timeout)
{
    *timeout = DEFAULT_TRNG_CALIBRATION_AGENT_TIMEOUT_SECOND;
    PAL_LOG_INFO("Current TRNG Calibration timeout setting is %d second!\n",
                 *timeout);
}