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

/*
    Copyright 2001, 2002 Georges Menie (www.menie.org)
    stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
    putchar is the only external dependency for this file,
    if you have a working putchar, leave it commented out.
    If not, uncomment the define below and
    replace outbyte(c) by your own function call.

*/
#include "base.h"
#include <stdarg.h>
//#include "irq.h"
#ifdef CONFIG_BK_BOOT
#include "hal_boot_control.h"
#endif

#define TEST_PRINTF 0

struct _output_args {
    char *outstr;
    size_t len;
    size_t pos;
};

/* This file is copied from FreeRTOS project */
extern void hal_uart_putc(uint8_t c);

void (*putchar)(uint8_t c) = hal_uart_putc;

static void printchar(struct _output_args *args, char **str, int c)
{
    //extern int putchar(int c);

    if (str) {
        if (args->pos < args->len) {
            **str = (char)c;
            ++(*str);
            args->pos++;
        }
    }
    else
    {
        if ('\n' == c) {
            (void)putchar('\n');
            (void)putchar('\r');
        } else {
            (void)putchar(c);
        }
    }
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(struct _output_args *args, char **out, const char *string, int width, int pad)
{
    register int pc = 0, padchar = ' ';

    if (width > 0) {
        register int len = 0;
        register const char *ptr;
        for (ptr = string; *ptr; ++ptr) ++len;
        if (len >= width) width = 0;
        else width -= len;
        if (pad & PAD_ZERO) padchar = '0';
    }
    if (!(pad & PAD_RIGHT)) {
        for ( ; width > 0; --width) {
            printchar (args, out, padchar);
            ++pc;
        }
    }
    for ( ; *string ; ++string) {
        printchar (args, out, *string);
        ++pc;
    }
    for ( ; width > 0; --width) {
        printchar (args, out, padchar);
        ++pc;
    }

    return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(struct _output_args *args, char **out, int i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    register char *s;
    register int t, neg = 0, pc = 0;
    register unsigned int u = (unsigned int)i;

    if (i == 0) {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return prints (args, out, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0) {
        neg = 1;
        u = (unsigned int)-i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    *s = '\0';

    while (u) {
        t = (unsigned int)u % b;
        if( t >= 10 )
            t += letbase - '0' - 10;
        *--s = (char)(t + '0');
        u /= b;
    }

    if (neg) {
        if( width && (pad & PAD_ZERO) ) {
            printchar (args, out, '-');
            ++pc;
            --width;
        }
        else {
            *--s = '-';
        }
    }

    return pc + prints (args, out, s, width, pad);
}

static int print( char **out, unsigned int count, const char *format, va_list args )
{
    register int width, pad;
    register int pc = 0;
    char scr[2];
    struct _output_args output_args;

    output_args.len = count - 1;
    output_args.pos = 0;

    for (; *format != 0; ++format) {
        if (0 != count) {
          if (pc >= count -1) {
               break;
           }
        }

        if (*format == '%') {
            ++format;
            width = pad = 0;
            if (*format == '\0') break;
            if (*format == '%') goto out;
            if (*format == '-') {
                ++format;
                pad = PAD_RIGHT;
            }
            while (*format == '0') {
                ++format;
                pad |= PAD_ZERO;
            }
            for ( ; *format >= '0' && *format <= '9'; ++format) {
                width *= 10;
                width += *format - '0';
            }
            if( *format == 's' ) {
                register char *s = (char *)va_arg( args, int );
                pc += prints (&output_args, out, s?s:"(null)", width, pad);
                continue;
            }
            if( *format == 'd' ) {
                pc += printi (&output_args, out, va_arg( args, int ), 10, 1, width, pad, 'a');
                continue;
            }
            if( *format == 'x' ) {
                pc += printi (&output_args, out, va_arg( args, int ), 16, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'X' ) {
                pc += printi (&output_args, out, va_arg( args, int ), 16, 0, width, pad, 'A');
                continue;
            }
            if( *format == 'u' ) {
                pc += printi (&output_args, out, va_arg( args, int ), 10, 0, width, pad, 'a');
                continue;
            }
            if( *format == 'c' ) {
                /* char are converted to int then pushed on the stack */
                scr[0] = (char)va_arg( args, int );
                scr[1] = '\0';
                pc += prints (&output_args, out, scr, width, pad);
                continue;
            }
        }
        else {
        out:
            printchar (&output_args, out, *format);
            ++pc;
        }
    }
    if (out) **out = '\0';
    va_end( args );
    return pc;
}

int printf(const char *format, ...)
{
        va_list args;

#ifdef CONFIG_BK_BOOT
	if (hal_ctrl_is_security_boot_print_disabled()) {
		return 0;
	}
#endif
        //uint32_t pri = irq_lock();
        va_start( args, format );
        int ret = print( 0, 0, format, args );
        //irq_unlock(pri);
        return ret;
}

int vprintf(const char *format, va_list vargs)
{
    return print( 0, 0, format, vargs );
}

int sprintf(char *out, const char *format, ...)
{
    va_list args;

    va_start( args, format );
    return print( &out, 0, format, args );
}

int snprintf( char *buf, unsigned int count, const char *format, ... )
{
    va_list args;

    if ((NULL == buf) || (0 == count)) {
        return 0;
    }

    va_start( args, format );
    return print( &buf, count, format, args );
}

int vsnprintf( char *buf, unsigned int count, const char *format, va_list vargs)
{
    if ((NULL == buf) || (0 == count) || (NULL == format)) {
        return 0;
    }
    return print( &buf, count, format, vargs );
}

#if TEST_PRINTF
static int32_t _printf_test(void)
{
    int32_t ret;
    uint8_t buf[10];

    printf("================== %d\n", __LINE__);
    ret = snprintf(NULL, 1, "snprintf test %d\n", __LINE__);
    if (0 != ret) {
        printf("%d %d\n", __LINE__, ret);
        return -1;
    }

    ret = snprintf(NULL, 0, "snprintf test %d\n", __LINE__);
    if (0 != ret) {
        printf("%d %d\n", __LINE__, ret);
        return -1;
    }


    ret = snprintf((char *)buf, 0, "snprintf test %d\n", __LINE__);
    if (0 != ret) {
        printf("%d %d\n", __LINE__, ret);
        return -1;
    }

    ret = snprintf((char *)buf, 9, "snprintf test %d\n", __LINE__);
    if (8 != ret) {
        printf("%d %d\n", __LINE__, ret);
        return -1;
    }
    printf("buf %s\n", buf);
    printf("================== %d\n", __LINE__);

    return 0;
}

int printf_test_main(void)
{
    char *ptr = "Hello world!";
    char *np = 0;
    int i = 5;
    unsigned int bs = sizeof(int)*8;
    int mi;
    char buf[80];

    mi = (1 << (bs-1)) + 1;
    printf("%s\n", ptr);
    printf("printf test\n");
    printf("%s is null pointer\n", np);
    printf("%d = 5\n", i);
    printf("%d = - max int\n", mi);
    printf("char %c = 'a'\n", 'a');
    printf("hex %x = ff\n", 0xff);
    printf("hex %02x = 00\n", 0);
    printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
    printf("%d %s(s)%", 0, "message");
    printf("\n");
    printf("%d %s(s) with %%\n", 0, "message");
    sprintf(buf, "justif: \"%-10s\"\n", "left"); printf("%s", buf);
    sprintf(buf, "justif: \"%10s\"\n", "right"); printf("%s", buf);
    sprintf(buf, " 3: %04d zero padded\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %-4d left justif.\n", 3); printf("%s", buf);
    sprintf(buf, " 3: %4d right justif.\n", 3); printf("%s", buf);
    sprintf(buf, "-3: %04d zero padded\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %-4d left justif.\n", -3); printf("%s", buf);
    sprintf(buf, "-3: %4d right justif.\n", -3); printf("%s", buf);

    if ( 0 != _printf_test()) {
        return -1;
    }
    return 0;
}

/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_PRINTF -c printf.c
 * you will get a normal warning:
 *   printf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */

#endif
