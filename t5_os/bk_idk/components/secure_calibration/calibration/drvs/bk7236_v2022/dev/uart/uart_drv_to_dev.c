// Copyright 2021-2022 Beken
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

#include <stdint.h>
#include <stddef.h>
#include "device.h"
#include "uart_dev.h"
#include "sys_hal.h"
#include "gpio_hal.h"
#include "hal_boot_control.h"

#define DEV_CFG(dev) ((struct uart_dev_config *)(dev)->config->config_info)

bool g_print_inited = false;
static gpio_hal_t s_gpio_hal = {0};

static void uart_init_gpio(gpio_id_t gpio_id)
{
	gpio_ll_set_gpio_perial_mode(s_gpio_hal.hw, gpio_id, 0);

	gpio_hal_output_enable(&s_gpio_hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio_hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio_hal, gpio_id, 0);
	gpio_hal_sencond_function_enable(&s_gpio_hal, gpio_id, 1);

	gpio_hal_pull_enable(&s_gpio_hal, gpio_id, 1);
	gpio_hal_pull_up_enable(&s_gpio_hal, gpio_id, 1);
}

static void uart_init_gpios(uart_id_t uart_id)
{
	gpio_id_t rx_gpio_id = uart_hal_get_rx_pin(uart_id);
	gpio_id_t tx_gpio_id = uart_hal_get_tx_pin(uart_id);
	uart_init_gpio(rx_gpio_id);
	uart_init_gpio(tx_gpio_id);
}

static void uart_clock_enable(uart_id_t id)
{
    switch(id)
    {
        case UART_ID_0:
            sys_hal_clk_pwr_ctrl(CLK_PWR_ID_UART1, CLK_PWR_CTRL_PWR_UP);
            break;
        case UART_ID_1:
            sys_hal_clk_pwr_ctrl(CLK_PWR_ID_UART2, CLK_PWR_CTRL_PWR_UP);
            break;
        case UART_ID_2:
            sys_hal_clk_pwr_ctrl(CLK_PWR_ID_UART3, CLK_PWR_CTRL_PWR_UP);
            break;
        default:
            break;
    }
}

int32_t uart_init(struct device *dev)
{
	struct uart_dev_config * dev_cfg = DEV_CFG(dev);
	uart_id_t id = dev_cfg->id;
	uart_hal_t *hal = dev_cfg->hal;
	uart_config_t *config = &dev_cfg->config;

	if (hal_ctrl_is_security_boot_print_disabled()) {
		return 0;
	}

	gpio_hal_init(&s_gpio_hal);

	hal->id = id;
	uart_hal_init(hal);
	uart_clock_enable(id);
	sys_hal_uart_select_clock(id, config->src_clk);
	uart_init_gpios(id);
	uart_hal_init_uart(hal, id, config);
	uart_hal_start_common(hal, id);

	g_print_inited = true;
	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int32_t _uart_poll_in(struct device *dev, unsigned char *c)
{
    volatile struct uart_dev_config *uart = DEV_CFG(dev);

    if (hal_ctrl_is_security_boot_print_disabled()) {
        return 0;
    }

    if (uart_hal_is_fifo_read_ready(uart->hal, uart->id)) {
        *c = uart_hal_read_byte(uart->hal, uart->id);
        return 0;
    } else {
        return -1;
    }
}

static int32_t _uart_poll_rx_ready(struct device *dev)
{
    volatile struct uart_dev_config *uart = DEV_CFG(dev);

    return uart_hal_is_fifo_read_ready(uart->hal, uart->id);
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char _uart_poll_out(struct device *dev,
                         unsigned char c)
{
    volatile struct uart_dev_config *uart = DEV_CFG(dev);

    if (hal_ctrl_is_security_boot_print_disabled()) {
        return 0;
    }

    while(!uart_hal_is_fifo_write_ready(uart->hal, uart->id));
    uart_hal_write_byte(uart->hal, uart->id, c);
    return 0;
}

const struct uart_driver_api g_uart_driver_api = {
    .poll_in = _uart_poll_in,
    .poll_out = _uart_poll_out,
    .poll_rx_ready = _uart_poll_rx_ready,
};

void uart_putc(struct device *dev, uint8_t c)
{
    const struct uart_driver_api* drv_api;

    if (NULL == dev) {
        while(1);
    }

    drv_api = dev->driver_api;
    if (NULL == drv_api) {
        while(1);
    }

    drv_api->poll_out(dev, c);
}

int32_t uart_getc(struct device *dev, uint8_t *c)
{
    const struct uart_driver_api* drv_api;

    if (NULL == dev || NULL == c) {
        while(1);
    }

    drv_api = dev->driver_api;
    if (NULL == drv_api) {
        while(1);
    }

    return drv_api->poll_in(dev, c);
}

int32_t uart_rx_ready(struct device *dev)
{
    const struct uart_driver_api* drv_api;

    if (NULL == dev) {
        return 0;
    }

    drv_api = dev->driver_api;
    if (NULL == drv_api) {
        return 0;
    }

    return drv_api->poll_rx_ready(dev);
}
