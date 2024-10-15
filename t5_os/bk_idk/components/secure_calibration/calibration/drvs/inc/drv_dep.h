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

#ifndef __DRV_DEP_H__
#define __DRV_DEP_H__

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* "MUST" implement apis begin */
void drv_udelay(uint32_t usecs);

int drv_printf(const char *format, ...);

int drv_memcmp(const void *m1, const void *m2, size_t n);
void *drv_memcpy(void *d, const void *s, size_t n);
void *drv_memset(void *buf, int c, size_t n);
/* "MUST" implement apis end */

#define ENABLE_DEBUG 1

#if ENABLE_DEBUG

#ifndef DRV_PROMPT
#define DRV_PROMPT "DRV"
#endif
#define _DRV_PROMPT DRV_PROMPT ": "

#define DRV_DBG(fmt, ...)                                                      \
    do {                                                                       \
        drv_printf(_DRV_PROMPT fmt, ##__VA_ARGS__);                                \
    } while (0)
#else
#define DRV_DBG(fmt, ...)
#endif

#define DRV_INFO(fmt, ...) drv_printf(_DRV_PROMPT fmt, ##__VA_ARGS__)
#define DRV_ERR(fmt, ...) drv_printf(_DRV_PROMPT fmt, ##__VA_ARGS__)

#define DRV_ASSERT_MSG(_x, fmt, ...)                                           \
    do {                                                                       \
        if (!(_x)) {                                                           \
            DRV_ERR("ASSERT FAILURE:\n");                                      \
            DRV_ERR("%s %s (%d): %s\n", __FILE__, __func__, __LINE__,          \
                    __FUNCTION__);                                             \
            DRV_ERR(fmt, ##__VA_ARGS__);                                       \
            while (1) /* loop */                                               \
                ;                                                              \
        }                                                                      \
    } while (0)

#define DRV_ASSERT(_x)                                                         \
    do {                                                                       \
        if (!(_x)) {                                                           \
            DRV_ERR("ASSERT FAILURE:\n");                                      \
            DRV_ERR("%s %s (%d): %s\n", __FILE__, __func__, __LINE__,          \
                    __FUNCTION__);                                             \
            back_trace;                                                        \
            while (1) /* loop */                                               \
                ;                                                              \
        }                                                                      \
    } while (0)

#define DRV_CHECK_RET(format, ...)                                             \
    do {                                                                       \
        if ((0) != (ret)) {                                                    \
            DRV_ERR("%s line:%d. Error number is 0x%x \n", __func__, __LINE__, \
                    ret);                                                      \
            DRV_ERR(format, ##__VA_ARGS__);                                    \
            goto finish;                                                       \
        }                                                                      \
    } while (0)

#define DRV_CHECK_CONDITION(true_condition, ret_code, format, ...)             \
    do {                                                                       \
        if (!(true_condition)) {                                               \
            ret = ret_code;                                                    \
            DRV_ERR("%s line:%d. Error number is 0x%x \n", __func__, __LINE__, \
                    ret);                                                      \
            DRV_ERR(format, ##__VA_ARGS__);                                    \
            goto finish;                                                       \
        }                                                                      \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* __DRV_DEP_H__ */
