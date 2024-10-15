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

#ifndef __HEAP_H__
#define __HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "asm.h"

#define CONFIG_SYSTEM_HEAP_SIZE     (170 * 1024) /* 100K heap */

#define KERNEL_HEAP_DEFINE(name, align, size)		\
	uint8_t __ALIGNED(align) _GENERIC_SECTION(.uninitialized) \
        name[size];

extern void global_heap_init(void);
extern void *kmalloc(size_t size);
extern void *kcalloc(size_t nmemb, size_t size);
extern void kfree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __HEAP_H__ */
