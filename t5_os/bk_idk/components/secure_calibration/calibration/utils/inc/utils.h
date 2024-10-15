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

#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define CONTAINER_OF(ptr, type, field)                          \
	((type *)(((char *)(ptr)) - offsetof(type, field)))

/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
#define ROUND_UP(x, align)                                      \
	(((unsigned long)(x) + ((unsigned long)align - 1)) &        \
	 ~((unsigned long)align - 1))
#define ROUND_DOWN(x, align) ((unsigned long)(x) & ~((unsigned long)align - 1))

/* check if "x" is align with "align"(must be a power of 2).
   0: NOT ALIGN
   1: ALIGN */
#define IS_ALIGNED(x, align)  \
        (!(((unsigned long)(x)) & ((unsigned long)align - 1)))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif /* __UTILS_H__ */

