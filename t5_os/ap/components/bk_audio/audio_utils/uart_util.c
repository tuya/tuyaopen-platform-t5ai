// Copyright 2025-2026 Beken
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

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */

#include <os/os.h>
#include <os/mem.h>
#include <driver/uart.h>
#include "gpio_driver.h"
#include <components/bk_audio/audio_utils/uart_util.h>


#define UART_UTIL_TAG "uart_util"

#define LOGI(...) BK_LOGI(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(UART_UTIL_TAG, ##__VA_ARGS__)


bk_err_t uart_util_create(uart_util_handle_t uart_util, uart_id_t id, uint32_t baud_rate)
{

    if (!uart_util)
    {
        LOGE("%s, %d, uart_util: %p\n", __func__, __LINE__, uart_util);
        return BK_FAIL;
    }

    uart_config_t config = {0};
    os_memset(&config, 0, sizeof(uart_config_t));
    if (id == 0)
    {
        gpio_dev_unmap(GPIO_10);
        gpio_dev_map(GPIO_10, GPIO_DEV_UART0_RXD);
        gpio_dev_unmap(GPIO_11);
        gpio_dev_map(GPIO_11, GPIO_DEV_UART0_TXD);
    }
    else if (id == 2)
    {
        gpio_dev_unmap(GPIO_40);
        gpio_dev_map(GPIO_40, GPIO_DEV_UART2_RXD);
        gpio_dev_unmap(GPIO_41);
        gpio_dev_map(GPIO_41, GPIO_DEV_UART2_TXD);
    }
    else
    {
        gpio_dev_unmap(GPIO_0);
        gpio_dev_map(GPIO_0, GPIO_DEV_UART1_TXD);
        gpio_dev_unmap(GPIO_1);
        gpio_dev_map(GPIO_1, GPIO_DEV_UART1_RXD);
    }

    config.baud_rate = baud_rate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_NONE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_FLOWCTRL_DISABLE;
    config.src_clk = UART_SCLK_XTAL_26M;

    uart_util->id = id;
    uart_util->baud_rate = baud_rate;

    if (bk_uart_init(id, &config) != BK_OK)
    {
        LOGE("%s, %d, init uart fail \n", __func__, __LINE__);
        return BK_FAIL;
    }
    LOGD("init uart: %d ok \n", uart_util->id);

    return BK_OK;
}

bk_err_t uart_util_destroy(uart_util_handle_t uart_util)
{
    if (!uart_util)
    {
        return BK_OK;
    }

    if (uart_util->id == 0)
    {
        gpio_dev_unmap(GPIO_10);
        gpio_dev_unmap(GPIO_11);
    }
    else if (uart_util->id == 2)
    {
        gpio_dev_unmap(GPIO_40);
        gpio_dev_unmap(GPIO_41);
    }
    else
    {
        gpio_dev_unmap(GPIO_0);
        gpio_dev_unmap(GPIO_1);
    }

    if (bk_uart_deinit(uart_util->id) != BK_OK)
    {
        LOGE("%s, %d, deinit uart: %d fail \n", __func__, __LINE__, uart_util->id);
    }
    LOGD("deinit uart: %d ok \n", uart_util->id);

    return BK_OK;
}

bk_err_t uart_util_tx_data(uart_util_handle_t uart_util, void *data_buf, uint32_t len)
{
    if (!uart_util || !data_buf || len == 0)
    {
        LOGE("%s, %d, uart_util: %p, data_buf: %p, len: %d \n", __func__, __LINE__, uart_util, data_buf, len);
        return BK_FAIL;
    }

    if (BK_OK != bk_uart_write_bytes(uart_util->id, data_buf, len))
    {
        LOGE("%s, %d, uart write data fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

