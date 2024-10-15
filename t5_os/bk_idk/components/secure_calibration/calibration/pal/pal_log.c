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

#include "base.h"
#include <stdarg.h>
#include "pal_common.h"
#include "pal_log.h"
#include "sys_hal.h"
#include "device.h"
#include "hal_platform.h"
#include "hal_boot_control.h"

static int32_t _g_pal_log_level = CONFIG_RUNTIME_LOG_LEVEL;
uint32_t g_verify_err = 0;

void pal_log_set_level(int32_t level)
{
    _g_pal_log_level = level;
}

int32_t pal_log_get_level(void)
{
    return _g_pal_log_level;
}

int32_t pal_platform_printf(const char *fmt, ...)
{
    va_list vargs;
    int r;

    if (g_print_inited == false)
        return 0;

    va_start(vargs, fmt);
    r = vprintf(fmt, vargs);
    va_end(vargs);

    return r;
}

void pal_log_dump_buf(const char* __msg__, const uint8_t* __buf__, int __size__)
{
        int32_t __i__;
        uint8_t *__ptr__ = (uint8_t *)__buf__;
        __PAL_LOG(INFO, "%s (%d):\r\n", __msg__, (int)__size__);
        for (__i__ = 0; __i__ < (int)__size__; __i__++) {
            __PAL_LOG_RAW(INFO, "%02X ", __ptr__[__i__]);
            if ((__i__ & 0xF) == 0xF) {
                __PAL_LOG_RAW(INFO, "\r\n");
            }
        }
        __PAL_LOG_RAW(INFO, "\r\n");
}

bool g_boot_print_disable_bypass = false;
void pal_critical_err(uint32_t err)
{
	char buf[32] = {0};
	bool print_inited = g_print_inited;

	if ((err > CRITICAL_ERR_FLASH_BUSY) && hal_ctrl_critical_err_disabled() && hal_ctrl_is_security_boot_print_disabled()) {
		return;
	}

	g_boot_print_disable_bypass = true;
	if (g_print_inited == false) {
		sys_hal_init();
		sys_device_do_config_level(DEVICE_LEVEL0);
		g_print_inited = true;
	}

	sprintf(buf, "E%x\r\n", err);
	hal_uart_puts(buf);
	g_print_inited = print_inited;
	g_boot_print_disable_bypass = false;
}
