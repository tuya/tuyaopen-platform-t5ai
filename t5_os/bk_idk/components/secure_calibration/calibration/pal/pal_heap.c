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
#include "pal_heap.h"

void *pal_calloc(size_t nmemb, size_t size)
{
    void *p = NULL;

    p = kcalloc(nmemb, size);

    return p;
}
void *pal_malloc(size_t size)
{
    void *p = NULL;

    p = kmalloc(size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}

void pal_free(void *ptr)
{
    kfree(ptr);

    return;
}
