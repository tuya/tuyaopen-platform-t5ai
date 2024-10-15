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

#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>     /* Needed to get definition of va_list */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char * fmt, ...);
int snprintf(char *s, size_t len, const char *fmt, ...);
int sprintf(char *s, const char *fmt, ...);

int vprintf(const char * fmt, va_list list);
int vsnprintf(char *s, size_t len, const char *fmt, va_list list);
int vsprintf(char *s, const char *fmt, va_list list);
int vsprintf( char *buf, const char *format, va_list vargs);

#ifdef __cplusplus
}
#endif

#endif /* __STDIO_H__ */
