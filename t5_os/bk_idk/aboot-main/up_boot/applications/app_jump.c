/*
 * File      : app_jump.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <board.h>
#include <stdio.h>
#include <stdint.h>
#include "BK_System.h"
#include "board.h"
#include "drv_uart.h"
#include "platform.h"

#define APP_ENTRY_ADDR      USER_APP_ENTRY
#define BOOT_END_ADDR       BK_BOOT_END

void on_jump(uint32_t app_stack_addr, void (*app_entry)(void))
{
        app_entry();
}

int jump_to_app(void)
{
        void (*jump2app)(void);

        jump2app = (void ( *)(void))APP_ENTRY_ADDR;
        bk_printf("jump_to_app to  ");
        bk_print_hex(APP_ENTRY_ADDR);
        bk_printf("\r\n ");
        /* do something then jump to application */
        uart1_wait_tx_finish();
        /*restore configration*/
        bl_hw_board_deinit();
        on_jump(0, jump2app);
        return 0;
}
