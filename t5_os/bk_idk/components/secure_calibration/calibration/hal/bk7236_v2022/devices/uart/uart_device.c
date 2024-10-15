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

#include "base.h"
#include "platform.h"
#include "uart_dev.h"
#include "uart.h"

#define DRV_NAME "uart0"

static uart_hal_t s_uart_hal = {0};
static struct uart_dev_config s_uart0_config  = {
    //.id = UART_ID_0,
    .id = UART_ID_1,
    .hal = &s_uart_hal,
    .config = {
#ifdef CONFIG_UART_1M
        .baud_rate = UART_BAUDRATE_1000000,
#else
        .baud_rate = UART_BAUDRATE_115200,
#endif
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk = UART_SCLK_XTAL_26M, 
    },
};

extern const struct uart_driver_api g_uart_driver_api;

DEVICE_DEFINE(dev_uart0,
              DRV_NAME,
              &uart_init,
              NULL,
              &s_uart0_config,
              DEVICE_LEVEL0,
              0,
              &g_uart_driver_api);

struct device *uart_0_dev = DEVICE_GET(dev_uart0);
