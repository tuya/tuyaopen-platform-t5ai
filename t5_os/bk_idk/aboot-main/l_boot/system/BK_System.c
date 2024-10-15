/*************************************************************
 * @file        BK_System.c
 * @brief       Project BK7231 system functions
 */
#include "BK_System.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void DelayNops(volatile unsigned long nops)
{
    while (nops --)
    {
    }
}

void DelayUS(volatile unsigned long timesUS)
{
    volatile unsigned long i;
	
    while (timesUS --)
    {
        i = 0;
        while (i < tick_cnt_us)
        {
            i++;
        }
    }
}

void DelayMS(volatile unsigned long timesMS)
{
    volatile unsigned long i;

    while (timesMS --)
    {
        i = 0;
        while (i < tick_cnt_ms)
        {
            i++;
        }
    }
}

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
void dump_hex(const unsigned char *ptr, int buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16)
    {
        bk_printf("%08X: ", i);

        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                bk_printf("%02X ", buf[i + j]);
            else
                bk_printf("   ");
        bk_printf(" ");

        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                bk_printf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
        bk_printf("\r\n");
    }
}
// eof

