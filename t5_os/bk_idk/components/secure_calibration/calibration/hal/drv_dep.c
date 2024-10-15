/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include <stdarg.h>
#include "hal_platform.h"
#include "stdio.h"
#include "string.h"

void drv_udelay(uint32_t usecs)
{
    hal_udelay(usecs);
}

int drv_printf(const char *format, ...)
{
    va_list va_args;
    int ret;

    va_start(va_args, format);
    ret = vprintf(format, va_args);
    va_end(va_args);
    return ret;
}

int drv_memcmp(const void *m1, const void *m2, size_t n)
{
    return memcmp(m1, m2, n);
}

void *drv_memcpy(void *d, const void *s, size_t n)
{
    return memcpy(d, s, n);
}

void *drv_memset(void *buf, int c, size_t n)
{
    return memset(buf, c, n);
}
