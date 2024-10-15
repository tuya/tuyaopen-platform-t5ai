/* string.c - common string routines */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "string.h"

/* this file and related header files are copied from Zephyr project */

/**
 *
 * @brief Copy a string
 *
 * @return pointer to destination buffer <d>
 */

char *strcpy(char *d, const char *s)
{
	char *dest = d;

	while (*s != '\0') {
		*d = *s;
		d++;
		s++;
	}

	*d = '\0';

	return dest;
}

/**
 *
 * @brief Copy part of a string
 *
 * @return pointer to destination buffer <d>
 */

char *strncpy(char *d, const char *s, size_t n)
{
	char *dest = d;

	while ((n > 0) && *s != '\0') {
		*d = *s;
		s++;
		d++;
		n--;
	}

	while (n > 0) {
		*d = '\0';
		d++;
		n--;
	}

	return dest;
}

size_t
strlcpy(char *dst, char const *src, size_t s)
{
	size_t i= 0;

	if (!s) {
		return strlen(src);
	}

	for (i= 0; ((i< s-1) && src[i]); i++) {
		dst[i]= src[i];
	}

	dst[i]= 0;

	return i + strlen(src+i);
}
/**
 *
 * @brief String scanning operation
 *
 * @return pointer to 1st instance of found byte, or NULL if not found
 */

char *strchr(const char *s, int c)
{
	char tmp = (char) c;

	while ((*s != tmp) && (*s != '\0'))
		s++;

	return (*s == tmp) ? (char *) s : NULL;
}

/**
 *
 * @brief String scanning operation
 *
 * @return pointer to last instance of found byte, or NULL if not found
 */

char *strrchr(const char *s, int c)
{
	char *match = NULL;

	do {
		if (*s == (char)c) {
			match = (char *)s;
		}
	} while (*s++);

	return match;
}

/**
 *
 * @brief Get string length
 *
 * @return number of bytes in string <s>
 */

size_t strlen(const char *s)
{
	size_t n = 0;

	while (*s != '\0') {
		s++;
		n++;
	}

	return n;
}

/**
 *
 * @brief Compare two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strcmp(const char *s1, const char *s2)
{
	while ((*s1 == *s2) && (*s1 != '\0')) {
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

/**
 *
 * @brief Compare part of two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strncmp(const char *s1, const char *s2, size_t n)
{
	while ((n > 0) && (*s1 == *s2) && (*s1 != '\0')) {
		s1++;
		s2++;
		n--;
	}

	return (n == 0) ? 0 : (*s1 - *s2);
}

char *strcat(char *dest, const char *src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}

char *strncat(char *dest, const char *src,
		  size_t n)
{
	char *orig_dest = dest;
	size_t len = strlen(dest);

	dest += len;
	while ((n-- > 0) && (*src != '\0')) {
		*dest++ = *src++;
	}
	*dest = '\0';

	return orig_dest;
}

char *strstr(char const *s, const char *find)
{
	int len1, len2;

	len2 = strlen(find);
	if (!len2) {
		return (char *)s;
	}

	len1 = strlen(s);
	while (len1 >= len2) {
		len1--;
		if (!memcmp(s,find,len2)) {
			return (char *)s;
		}
		s++;
	}

	return NULL;
}

/**
 * @ingroup	sec_test
 * @brief	safe_memcmp The normal memcmp function has side-channel attack
 * venture,
 *			 better implement safe_memcmp function.
 * Details see:
 * https://security.stackexchange.com/questions/160808/why-should-memcmp-not-be-used
 * -to-compare-security-critical-data?utm_medium=organic&utm_source=google_rich_qa&
 * utm_campaign=google_rich_qa
 *
 * @return	return compare result, 0 means the same others not
 */
int safe_memcmp(const void *in_a, const void *in_b, size_t len)
{
	size_t i;
	const volatile unsigned char *a = in_a;
	const volatile unsigned char *b = in_b;
	unsigned char x					= 0;

	for (i = 0; i < len; i++) {
		x |= a[i] ^ b[i];
	}

	return x;
}

/**
 *
 * @brief Compare two memory areas
 *
 * @return negative # if <m1> < <m2>, 0 if <m1> == <m2>, else positive #
 */
int memcmp(const void *m1, const void *m2, size_t n)
{
	const char *c1 = m1;
	const char *c2 = m2;

	if (!n)
		return 0;

	while ((--n > 0) && (*c1 == *c2)) {
		c1++;
		c2++;
	}

	return *c1 - *c2;
}

/**
 *
 * @brief Copy bytes in memory with overlapping areas
 *
 * @return pointer to destination buffer <d>
 */

void *memmove(void *d, const void *s, size_t n)
{
	char *dest = d;
	const char *src  = s;

	if ((size_t) (d - s) < n) {
		/*
		 * The <src> buffer overlaps with the start of the <dest> buffer.
		 * Copy backwards to prevent the premature corruption of <src>.
		 */

		while (n > 0) {
			n--;
			dest[n] = src[n];
		}
	} else {
		/* It is safe to perform a forward-copy */
		while (n > 0) {
			*dest = *src;
			dest++;
			src++;
			n--;
		}
	}

	return d;
}

/**
 *
 * @brief Copy bytes in memory
 *
 * @return pointer to start of destination buffer
 */

void *memcpy(void *d, const void *s, size_t n)
{
	/* attempt word-sized copying only if buffers have identical alignment */

	unsigned char *d_byte = (unsigned char *)d;
	const unsigned char *s_byte = (const unsigned char *)s;

	if ((((unsigned int)d ^ (unsigned int)s_byte) & 0x3) == 0) {

		/* do byte-sized copying until word-aligned or finished */

		while (((unsigned int)d_byte) & 0x3) {
			if (n == 0) {
				return d;
			}
			*(d_byte++) = *(s_byte++);
			n--;
		};

		/* do word-sized copying as long as possible */

		unsigned int *d_word = (unsigned int *)d_byte;
		const unsigned int *s_word = (const unsigned int *)s_byte;

		while (n >= sizeof(unsigned int)) {
			*(d_word++) = *(s_word++);
			n -= sizeof(unsigned int);
		}

		d_byte = (unsigned char *)d_word;
		s_byte = (unsigned char *)s_word;
	}

	/* do byte-sized copying until finished */

	while (n > 0) {
		*(d_byte++) = *(s_byte++);
		n--;
	}

	return d;
}

/**
 *
 * @brief Set bytes in memory
 *
 * @return pointer to start of buffer
 */

void *memset(void *buf, int c, size_t n)
{
	/* do byte-sized initialization until word-aligned or finished */

	volatile unsigned char *d_byte = (unsigned char *)buf;
	unsigned char c_byte = (unsigned char)c;

	while (((unsigned int)d_byte) & 0x3) {
		if (n == 0) {
			return buf;
		}
		*(d_byte++) = c_byte;
		n--;
	};

	/* do word-sized initialization as long as possible */

	volatile unsigned int *d_word = (unsigned int *)d_byte;
	unsigned int c_word = (unsigned int)(unsigned char)c;

	c_word |= c_word << 8;
	c_word |= c_word << 16;

	while (n >= sizeof(unsigned int)) {
		*(d_word++) = c_word;
		n -= sizeof(unsigned int);
	}

	/* do byte-sized initialization until finished */

	d_byte = (unsigned char *)d_word;

	while (n > 0) {
		*(d_byte++) = c_byte;
		n--;
	}

	return buf;
}

/**
 *
 * @brief Scan byte in memory
 *
 * @return pointer to start of found byte
 */

void *memchr(const void *s, int c, size_t n)
{
	if (n != 0) {
		const unsigned char *p = s;

		do {
			if (*p++ == ((unsigned char)c)) {
				return ((void *)(p - 1));
			}

		} while (--n != 0);
	}

	return NULL;
}

void int2str(int n, char *str)
{
	char buf[10] = "";
	int i = 0;
	int len = 0;
	int temp = n < 0 ? -n: n;  // temp为n的绝对值

	if (NULL == str) {
		return;
	}

	while (temp) {
		buf[i++] = (temp % 10) + '0';  //把temp的每一位上的数存入buf
		temp = temp / 10;
	}

	len = n < 0 ? ++i: i;  //如果n是负数，则多需要一位来存储负号
	str[i] = 0;			   //末尾是结束符0
	while (1) {
		i--;
		if (buf[len-i-1] ==0) {
			break;
		}
		str[i] = buf[len-i-1];	//把buf数组里的字符拷到字符串
	}

	if (i == 0) {
		str[i] = '-';		   //如果是负数，添加一个负号
	}
}
