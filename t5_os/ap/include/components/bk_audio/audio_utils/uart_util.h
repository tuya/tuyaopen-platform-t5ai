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

/**
 * @brief      Create a uart util
 *
 * @param[in]      uart_util  The uart util handle
 * @param[in]      id         The uart id
 * @param[in]      baud_rate  The baud rate
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t uart_util_create(uart_util_handle_t uart_util, uart_id_t id, uint32_t baud_rate);

/**
 * @brief      Destroy a uart util
 *
 * @param[in]      uart_util  The uart util handle
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t uart_util_destroy(uart_util_handle_t uart_util);

/**
 * @brief      Transmit data via uart util
 *
 * @param[in]      uart_util  The uart util handle
 * @param[in]      data_buf   The data buffer
 * @param[in]      len        The data length
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t uart_util_tx_data(uart_util_handle_t uart_util, void *data_buf, uint32_t len);

#ifdef __cplusplus
}
#endif

