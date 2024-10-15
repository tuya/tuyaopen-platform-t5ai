/*
 * File      : main.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <stdio.h>
#include <board.h>
#include "BK_System.h"
#include "platform.h"

extern u32 startup;
extern u32 uart_buff_write;
extern u8 bim_uart_rx_buf[4096];
static u16 bim_uart_temp, uart_buff_read;
static int  check_cnt = 0;
extern void reset_timer(void);
extern void system_startup(void);
extern void bk_printf(const char *fmt);
extern void bk_print_hex(unsigned int num);
extern void boot_uart_data_callback( u8 *buff, u16 len);
 
int boot_main(void)
{
    bl_hw_board_init();  
    bk_printf("boot_main enter\r\n");
    while(1)
    {
        bim_uart_temp = uart_buff_write;
        if (uart_buff_read < bim_uart_temp)
        {
            boot_uart_data_callback(bim_uart_rx_buf + uart_buff_read, bim_uart_temp - uart_buff_read);
            uart_buff_read = bim_uart_temp;
            check_cnt = 0;
        }
        else if (uart_buff_read > bim_uart_temp)
        {
            boot_uart_data_callback(bim_uart_rx_buf + uart_buff_read, sizeof(bim_uart_rx_buf) - uart_buff_read);
            boot_uart_data_callback(bim_uart_rx_buf, bim_uart_temp);
            uart_buff_read = bim_uart_temp;
            check_cnt = 0;
        }
        else
        {
            if(uart_download_status == 0 )
            {
               if(check_cnt++ > CHECK_CNT_NUMBER0 )
                {
                    break;
                }
            }
            else
            {      
                if(check_cnt++ > CHECK_CNT_NUMBER1)
                {
                   bk_printf("begin reboot\r\n");
                    *((volatile uint32_t *)(0x44010000 + 0xf*4)) &= ~(0x1<<1);
                    wdt_reboot();
                }
            }
        }
    }
    extern u32 boot_downloading;
    boot_downloading = FALSE;
    startup = TRUE;
    system_startup();		
    return 0;
}

