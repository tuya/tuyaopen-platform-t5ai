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
#include "pal_common.h"
#include "pal_string.h"

int32_t pal_safe_memcmp(const void *m1, const void *m2, size_t n)
{
    return safe_memcmp(m1, m2, n);
}

int32_t pal_memcmp(const void *m1, const void *m2, size_t n)
{
    return safe_memcmp(m1, m2, n);
}

void *pal_memcpy(void *d, const void *s, size_t n)
{
    return memcpy(d, s, n);
}

void *pal_memset(void *buf, int32_t c, size_t n)
{
    return memset(buf, c, n);
}

void *pal_memmove(void *d, const void *s, size_t n)
{
    return memmove(d, s, n);
}