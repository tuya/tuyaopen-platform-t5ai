// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>
#include <os/mem.h>

typedef enum {
	USB_SIMULATE_UART_ID_0 = 0, /**< USBH UART id 0 */
	USB_SIMULATE_UART_ID_1,     /**< USBH UART id 1 */
	USB_SIMULATE_UART_ID_MAX    /**< USBH UART id max */
} usb_simulate_uart_id_t;

typedef void (*usb_to_uart_cb_t)(usb_simulate_uart_id_t id, void *arg);

typedef struct {
    usb_to_uart_cb_t rx_cb;
    void *rx_arg;
    usb_to_uart_cb_t tx_cb;
    void *tx_arg;
    usb_to_uart_cb_t connect_cb;

    void *connect_arg;
    usb_to_uart_cb_t disconnect_cb;
    void *disconnect_arg;
} usb_simulate_uart_callback;

/**
 * @brief     Registering Callbacks
 *
 * Notification callback functions for registering device connections and
 * disconnections, as well as callback functions for sending and 
 * receiving successful data
 *
 *   - id: Select the simulated serial port ID to register
 *   - cb_p: Fill in callback fucntions and private parameters in the structure
 *
 * This API should be called before any other USB APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_register_callback(usb_simulate_uart_id_t id, usb_simulate_uart_callback *cb_p);

/**
 * @brief     Initialization function
 *
 * Register device recongnition driver
 * Open USB underlying driver 
 * Allocation baud rate, data bit width, flow control, etc
 *
 *   - id: Select the simulated serial port ID to register
 *   - config: Parameter configuration carried out
 *
 * Before sending and receiving data.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_init(usb_simulate_uart_id_t id, const uart_config_t *config);

/**
 * @brief     Uninitialization function
 *
 * Unregister device recongnition driver
 * Close USB underlying driver 
 *
 *   - id: Select the simulated serial port ID to register
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_deinit(usb_simulate_uart_id_t id);

/**
 * @brief     Read data
 *
 * According to maxpacketsize, every time data is received, this interrupt is reported
 * After receiving the callback funciton notification, please read the data out in a timely manner
 *
 *   - id: Select the simulated serial port ID to register
 *   - data: Buffer pointer, copy data to pointer memory
 *   - cnt: Expected length of data to be read
 *   - timeout_ms: Read length exceeds the number of data stored in FIFO, waiting time
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_read(usb_simulate_uart_id_t id, uint8_t *data, uint32_t cnt, uint32_t timeout_ms);

/**
 * @brief     Write data
 *
 * Sending data is divied into two situations
 * The waiting time is 0. Write the data into the software fifo and hand it over to the lower half for transmissiong
 * If the waiting time is greater than 0, the caller will be blocked until all data is sent or timeout occurs.
 *
 *   - id: Select the simulated serial port ID to register
 *   - data: Buffer pointer, copy data to pointer memory
 *   - cnt: Expected length of data to be read
 *   - timeout_ms: Send wait, if exceeded, an exception will be returned
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_write(usb_simulate_uart_id_t id, uint8_t *data, uint32_t cnt, uint32_t timeout_ms);

/**
 * @brief     Set baud rate
 *
 * Configure according to the required baud rate of the serial port
 *
 *   - id: Select the simulated serial port ID to register
 *   - baud_rate: Fill in the baud rate, for example 115200
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_set_baud_rate(usb_simulate_uart_id_t id, uint32_t baud_rate);

/**
 * @brief     Check successful connection
 *
 * Configure according to the required baud rate of the serial port
 *
 *   - id: Select the simulated serial port ID to register
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_to_uart_simulate_check_device(usb_simulate_uart_id_t id);

#ifdef __cplusplus
}
#endif