/* string.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char  *strcpy(char *d, const char *s);
extern char  *strncpy(char *d, const char *s, size_t n);
extern size_t strlcpy(char *dst, char const *src, size_t s);
extern char  *strchr(const char *s, int c);
extern char  *strrchr(const char *s, int c);
extern size_t strlen(const char *s);
extern int   strcmp(const char *s1, const char *s2);
extern int   strncmp(const char *s1, const char *s2, size_t n);
extern char *strcat(char *dest, const char *src);
extern char *strncat(char *d, const char *s, size_t n);
extern char *strstr(const char *s, const char *find);

extern int   safe_memcmp(const void *in_a, const void *in_b, size_t len);
extern int   memcmp(const void *m1, const void *m2, size_t n);
extern void *memmove(void *d, const void *s, size_t n);
extern void *memcpy(void *d, const void *s, size_t n);
extern void *memset(void *buf, int c, size_t n);
extern void *memchr(const void *s, int c, size_t n);
extern void int2str(int n, char *str);

#ifdef __cplusplus
}
#endif

#endif  /* __STRING_H__ */
