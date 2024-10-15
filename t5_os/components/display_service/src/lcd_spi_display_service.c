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


#include <os/os.h>
#include <driver/spi.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/lcd.h>


#define LCD_SPI_TAG "lcd_spi"

#define LCD_SPI_LOGI(...) BK_LOGI(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGW(...) BK_LOGW(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGE(...) BK_LOGE(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGD(...) BK_LOGD(LCD_SPI_TAG, ##__VA_ARGS__)


#define LCD_SPI_RESET_PIN           GPIO_28
#define LCD_SPI_BACKLIGHT_PIN       GPIO_26
#define LCD_SPI_RS_PIN              GPIO_9
#define LCD_SPI_DEVICE_CASET        0x2A
#define LCD_SPI_DEVICE_RASET        0x2B
#define LCD_SPI_DEVICE_RAMWR        0x2C
#define LCD_SPI_ID                  SPI_ID_0

spi_config_t config = {0};

static void lcd_spi_device_gpio_init(void)
{
    BK_LOG_ON_ERR(bk_gpio_driver_init());

    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RS_PIN));

    gpio_config_t config;
    config.io_mode = GPIO_OUTPUT_ENABLE;
    config.pull_mode = GPIO_PULL_DISABLE;
    config.func_mode = GPIO_SECOND_FUNC_DISABLE;

    bk_gpio_set_config(LCD_SPI_RESET_PIN, &config);
    bk_gpio_set_config(LCD_SPI_BACKLIGHT_PIN, &config);
    bk_gpio_set_config(LCD_SPI_RS_PIN, &config);

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RS_PIN));
}

static void lcd_spi_device_gpio_deinit(void)
{
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RS_PIN));
}

static void lcd_spi_send_cmd(uint8_t cmd)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RS_PIN));
    bk_spi_write_bytes(LCD_SPI_ID, &cmd, 1);
}

static void lcd_spi_send_data(uint8_t *data, uint32_t data_len)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RS_PIN));

#if CONFIG_SPI_DMA
    if (data_len > 32) {
        bk_spi_dma_write_bytes(LCD_SPI_ID, data, data_len);
    } else {
        bk_spi_write_bytes(LCD_SPI_ID, data, data_len);
    }
#else
    bk_spi_write_bytes(LCD_SPI_ID, data, data_len);
#endif
}

void lcd_spi_backlight_open(void)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_BACKLIGHT_PIN));
}

void lcd_spi_backlight_close(void)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_BACKLIGHT_PIN));
}

static void lcd_spi_driver_init(void)
{
    bk_spi_driver_init();

    config.role = SPI_ROLE_MASTER;
    config.bit_width = SPI_BIT_WIDTH_8BITS;
    config.polarity = SPI_POLARITY_HIGH;
    config.phase = SPI_PHASE_2ND_EDGE;
    config.wire_mode = SPI_4WIRE_MODE;
    config.baud_rate = 30000000;
    config.bit_order = SPI_MSB_FIRST;
    #if CONFIG_SPI_DMA
    config.dma_mode = SPI_DMA_MODE_ENABLE;
    config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
    config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
    #else
    config.dma_mode = SPI_DMA_MODE_DISABLE;
    #endif

    BK_LOG_ON_ERR(bk_spi_init(LCD_SPI_ID, &config));
}

void lcd_spi_driver_deinit(void)
{
    BK_LOG_ON_ERR(bk_spi_deinit(LCD_SPI_ID));
    bk_dma_free(DMA_DEV_GSPI0, config.spi_tx_dma_chan);
    bk_dma_free(DMA_DEV_GSPI0_RX, config.spi_rx_dma_chan);
}

void lcd_spi_init(const lcd_device_t *device)
{
    lcd_spi_device_gpio_init();

    lcd_spi_driver_init();

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RESET_PIN));
    rtos_delay_milliseconds(100);
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RESET_PIN));
    rtos_delay_milliseconds(120);

    if (device->spi->init_cmd != NULL) {
        const lcd_qspi_init_cmd_t *init = device->spi->init_cmd;
        for (uint32_t i = 0; i < device->spi->device_init_cmd_len; i++) {
            if (init->data_len == 0xFF) {
                rtos_delay_milliseconds(init->data[0]);
            } else {
                lcd_spi_send_cmd(init->cmd);
                if (init->data_len != 0) {
                    lcd_spi_send_data((uint8_t *)init->data, init->data_len);
                }
            }
            init++;
        }
    } else {
        LCD_SPI_LOGE("lcd spi device init cmd is null\r\n");
    }

    lcd_spi_backlight_open();
}

void lcd_spi_deinit(void)
{
    lcd_spi_backlight_close();

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RESET_PIN));

    lcd_spi_driver_deinit();

    lcd_spi_device_gpio_deinit();
}

void lcd_spi_display_frame(uint8_t *frame_buffer, uint32_t width, uint32_t height)
{
    uint8_t column_value[4] = {0};
    uint8_t row_value[4] = {0};
    column_value[2] = (width >> 8) & 0xFF,
    column_value[3] = (width & 0xFF) - 1,
    row_value[2] = (height >> 8) & 0xFF;
    row_value[3] = (height & 0xFF) - 1;

    lcd_spi_send_cmd(LCD_SPI_DEVICE_CASET);
    lcd_spi_send_data(column_value, 4);
    lcd_spi_send_cmd(LCD_SPI_DEVICE_RASET);
    lcd_spi_send_data(row_value, 4);

    lcd_spi_send_cmd(LCD_SPI_DEVICE_RAMWR);
    lcd_spi_send_data(frame_buffer, width * height * 2);
}

