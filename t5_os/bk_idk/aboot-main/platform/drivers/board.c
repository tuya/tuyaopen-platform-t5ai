/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2018 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     armink      first implementation
 * 2018-02-06     Murphy      add flash control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "drv_uart.h"
#include "driver_system.h"
#include "driver_flash.h"
#include "driver_sys_ctrl.h"
#include "BK_System.h"
#include "drv_timer.h"
#include "interrupt.h"


void __attribute__((section(".itcm_write_flash"))) bl_hw_board_init(void)
{

    uart_init(); 
    //flash_init(); //flash need init when upgrade OTA 
    flash_set_line_mode(2);
    flash_set_clk(5);  // 78M
    get_flash_ID();
    flash_get_current_flash_config();
    arch_interrupt_ctrl(0x1);
}
void __attribute__((section(".itcm_write_flash"))) bl_hw_board_deinit(void)
{
	uart_deinit();
	arch_interrupt_ctrl(0x0);
}