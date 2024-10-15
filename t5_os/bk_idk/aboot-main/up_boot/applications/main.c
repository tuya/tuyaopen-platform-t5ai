/*
 * File      : main.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <board.h>

#include "BK_System.h"
#include "board.h"

#include "driver_flash.h"
#include "drv_uart.h"
#include "interrupt.h"

extern void __attribute__((section(".itcm_write_flash"))) bl_hw_board_init(void);
int __attribute__((section(".itcm_write_flash")))  boot_main(void)
{
        bl_hw_board_init();	
        bk_printf("rt_hw_board_init finish  \r\n");
        system_timeout_startup();

        return 0;
}



