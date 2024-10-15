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

#ifndef __MUTEX_H__
#define __MUTEX_H__

typedef void *mutex_t;

static inline int32_t mutex_init(mutex_t *m)
{
    return 0;
}

static inline int32_t mutex_acquire(mutex_t *m)
{
    return 0;
}

static inline int32_t mutex_release(mutex_t *m)
{
    return 0;
}

#endif /* __MUTEX_H__ */
