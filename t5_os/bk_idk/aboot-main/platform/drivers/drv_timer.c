/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "drv_timer.h"
#include "drv_pwm.h"
#include "bk_timer.h"


static volatile u32 current_clock = 0;
static volatile u32 current_seconds = 0;
static u32 second_countdown = FCLK_SECOND;
extern unsigned long get_timer_masked(void);
extern void system_timeout_startup(void);


void fclk_hdl(u8 param)
{
    current_clock ++;

    system_timeout_startup();

    if (--second_countdown == 0)
    {
        current_seconds++;
        second_countdown = FCLK_SECOND;
    }
}

u32 fclk_get_tick(void)
{
    return current_clock;
}

u32 fclk_get_time_ms(void)
{
    return current_clock * FCLK_DURATION_MS;
}

void fclk_reset_tick(void)
{
    current_clock = 0;
}

u32 fclk_get_second(void)
{
    return current_seconds;
}

void fclk_reset_count(void)
{
    current_clock = 0;
    current_seconds = 0;
}

u32 fclk_cal_endvalue(u32 mode)
{
    u32 value = 1;

    if(PWM_CLK_32K == mode)
    {
        /*32k clock*/
        value = FCLK_DURATION_MS * 32;
    }
    else if(PWM_CLK_26M == mode)
    {
        /*26m clock*/
        value = FCLK_DURATION_MS * 26000;
    }

    return value;
}

/* nothing really to do with interrupts, just starts up a counter. */
int timer_init(void)
{
    #if (( 1== CFG_TICK_USE_TIMER) && CFG_SOC_NAME != SOC_BK7231)
    timer_param_t param;
    param.channel = FCLK_TIMER_ID;
    param.div = 1;
    param.period = FCLK_DURATION_MS;
    param.t_Int_Handler= fclk_hdl;

    bk_timer_ctrl( CMD_TIMER_INIT_PARAM, &param);
    UINT32 timer_channel;
    timer_channel = param.channel;
    bk_timer_ctrl( CMD_TIMER_UNIT_ENABLE, &timer_channel);

    #else

    pwm_param_t param;
    /*init pwm*/
    param.channel         = FCLK_PWM_ID;
    param.cfg.bits.en     = PWM_ENABLE;
    param.cfg.bits.int_en = PWM_INT_EN;
    param.cfg.bits.mode   = PMODE_TIMER;

#if (0)  // FPGA:PWM0-2-32kCLK, pwm3-5-24CLK
    param.cfg.bits.clk    = PWM_CLK_32K;
#else
    param.cfg.bits.clk    = PWM_CLK_26M;
#endif

    param.p_Int_Handler   = fclk_hdl;
    param.duty_cycle      = 0;
    param.end_value       = fclk_cal_endvalue((u32)param.cfg.bits.clk);

    pwm_ctrl(CMD_PWM_INIT_PARAM, &param);
    #endif
    return 0;
}

/*
 * timer without interrupts
 */

void reset_timer_masked(void)
{
    fclk_reset_tick();
}


unsigned long get_timer(unsigned long base)
{
    return fclk_get_tick() - base;
}

void udelay_masked(unsigned long usec)
{
    unsigned long tmp;

    tmp = get_timer(0);
    while (get_timer(tmp) < usec)	/* our timer works in usecs */
        ; /* NOP */
}

void reset_timer(void)
{
    reset_timer_masked();
}


void udelay(unsigned long usec)
{
    if(usec < 2000)
        usec = 1;
    else
    {
        usec += 1999;
        usec /= 2000;
    }

    udelay_masked(usec);
}

// eof

