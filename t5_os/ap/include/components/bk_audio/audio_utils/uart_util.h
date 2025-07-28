#pragma once

#include <os/os.h>
#include <driver/uart.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uart_util
{
    uart_id_t id;
    uint32_t baud_rate;
};

typedef struct uart_util *uart_util_handle_t;

bk_err_t uart_util_create(uart_util_handle_t uart_util, uart_id_t id, uint32_t baud_rate);
bk_err_t uart_util_destroy(uart_util_handle_t uart_util);
bk_err_t uart_util_tx_data(uart_util_handle_t uart_util, void *data_buf, uint32_t len);

#ifdef __cplusplus
}
#endif

