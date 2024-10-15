/*************************************************************
 * @file        BK_System.c
 * @brief       Project BK7231 system functions
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 * 2018-02-03   MurphyZhao        add user memory function, need to improve
 */

#include "BK_System.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * DelayNops
 */
void DelayNops(volatile unsigned long nops)
{
    while (nops --)
    {
    }
}

void DelayUS(volatile unsigned long timesUS)
{
    volatile unsigned long i;

    // unsigned long tick_cnt = 8;
#if (MCU_CLK == MCU_CLK_26MHz)
    // unsigned long tick_cnt_us = 3;
#define tick_cnt_us         3
#elif (MCU_CLK == MCU_CLK_120MHz)
#define tick_cnt_us         28
#endif
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

#if (MCU_CLK == MCU_CLK_26MHz)
#define tick_cnt_ms         2700
#elif (MCU_CLK == MCU_CLK_120MHz)
#define tick_cnt_ms         29500
#endif

    while (timesMS --)
    {
        i = 0;
        while (i < tick_cnt_ms)
        {
            i++;
        }
    }
}
// eof

