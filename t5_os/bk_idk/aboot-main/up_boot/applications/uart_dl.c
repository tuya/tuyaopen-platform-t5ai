#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
//#include "driver_system.h"
//#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"

#if (UART_DOWNLOAD_PORT  ==  DEBUG_PORT_UART1)
#define DL_UART_SEND    uart1_send
#define DL_UART_INIT    uart1_init
#else
#define DL_UART_SEND    uart0_send
#define DL_UART_INIT    uart0_init
#endif

u32 uart_buff_write = 0;
u8 bim_uart_rx_buf[4096];

void  uart_download_rx(UINT8 val)
{
        bim_uart_rx_buf[uart_buff_write++] = (u8)val;
        if(uart_buff_write == 4096)
                uart_buff_write = 0;
}

