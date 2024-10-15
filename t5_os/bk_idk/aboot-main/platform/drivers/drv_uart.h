/*
 * File      : drv_uart.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     Murphy      the first version
 */
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_uart2.h"
#ifndef _DRV_UART_H_
#define _DRV_UART_H_

typedef enum {
	UART_ID_1 = 0, /**< UART id 1 */
	UART_ID_2,     /**< UART id 2 */
	UART_ID_3,     /**< UART id 3 */
	UART_ID_MAX    /**< UART id max */
} uart_id_t;

int uart_init(void);
int uart_deinit(void);
int uart_putc(char c);
int uart_getc(void);

void uart_console_output(const char *str, int length);
void bk_printf(const char *fmt, ...);
void bk_print_hex(unsigned int num);

#endif /* _DRV_UART_H_ */
