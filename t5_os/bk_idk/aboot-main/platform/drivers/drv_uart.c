/*
 * File      : drv_uart.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     Murphy       the first version
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <board.h>
#include "drv_uart.h"
#include "BK_System.h"

#ifndef CONFIG_PRINTF_BUF_SIZE
#define CONFIG_PRINTF_BUF_SIZE    (128)
#endif

int uart_init(void)
{
    uart0_init(115200);
    uart1_init(115200);    	
    return 0;
}
int uart_deinit(void)
{
	uart0_disable();
	uart1_disable();
}

int uart_putc(char c)
{
    char buf[1];
    buf[0] = c;

#if ((PRINT_PORT == DEBUG_PORT_UART0))
    uart0_send((u8 *)&buf[0], 1);

#endif
#if ((PRINT_PORT == DEBUG_PORT_UART1))
    uart1_send((u8 *)&buf[0], 1);
#endif
#if ((PRINT_PORT == DEBUG_PORT_UART2) && (SOC_BK7271 == CFG_SOC_NAME) && (SOC_BK7256 == CFG_SOC_NAME))
	uart2_send((u8 *)&buf[0], 1);
#endif
    return 1;
}

int uart_getc(void)
{
    int ch;
    ch = -1;

    return ch;
}

void uart_console_output(const char *str, int length)
{
    int index;

    for (index = 0; index < length; index ++)
    {
        uart_putc(*str);
        str++;
    }
}

#ifdef DEBUG_PRINT_ENABLE
void uart_send_string(const char *fmt)
{
	#if (DEBUG_PORT_UART0 == PRINT_PORT)
    uart0_send_string(fmt);
	#elif (DEBUG_PORT_UART2 == PRINT_PORT) && ((SOC_BK7271 == CFG_SOC_NAME) || (SOC_BK7256 == CFG_SOC_NAME))
	uart2_send_string(fmt);
    #else
    uart1_send_string(fmt);
    #endif
}

static void exception_mode_printf(const char *fmt, va_list ap)
{
	char string[CONFIG_PRINTF_BUF_SIZE];

	vsnprintf(string, sizeof(string) - 1, fmt, ap);
	string[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
	uart_send_string(string);
}
void bk_printf(const char *fmt, ...)
{
	char string[CONFIG_PRINTF_BUF_SIZE];

	va_list args;
	va_start(args, fmt);
	//exception_mode_printf(fmt, args);

	vsnprintf(string, sizeof(string) - 1, fmt, args);
	string[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
	uart_send_string(string);
	va_end(args);

	return;
}
#else
void bk_printf(const char *fmt, ...)
{
#if (DEBUG_PORT_UART0 == PRINT_PORT)
    uart0_send_string(fmt);
	#elif (DEBUG_PORT_UART2 == PRINT_PORT) && ((SOC_BK7271 == CFG_SOC_NAME) || (SOC_BK7256 == CFG_SOC_NAME))
	uart2_send_string(fmt);
    #else
    uart1_send_string(fmt);
    #endif
}
#endif

char *hex2Str( uint8_t data)
{
  char hex[] = "0123456789ABCDEF";
  static char str[3];
  char *pStr = str;
 
  *pStr++ = hex[data >> 4];
  *pStr++ = hex[data & 0x0F];
  *pStr = 0;

  return str;
}

void bk_print_hex(unsigned int num)
{
	uint8_t i;
	uint8_t m;

    #if (DEBUG_PORT_UART0 == PRINT_PORT)
	uart0_send_string((char *)"0x");
	#elif (DEBUG_PORT_UART2 == PRINT_PORT) && ((SOC_BK7271 == CFG_SOC_NAME) || (SOC_BK7256 == CFG_SOC_NAME))
	uart2_send_string((char *)"0x");
    #else
	uart1_send_string((char *)"0x");
    #endif
    
	for (i = 4;i > 0;i--)
	{
		m = ((num >> (8 * (i - 1)))& 0xff);
        #if (DEBUG_PORT_UART0 == PRINT_PORT)
        uart0_send_string(hex2Str(m));
		#elif (DEBUG_PORT_UART2 == PRINT_PORT) && ((SOC_BK7271 == CFG_SOC_NAME) || (SOC_BK7256 == CFG_SOC_NAME))
		uart2_send_string(hex2Str(m));
        #else
        uart1_send_string(hex2Str(m));
        #endif        
	} 
}

