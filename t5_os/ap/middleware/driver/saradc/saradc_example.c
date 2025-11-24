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

#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include <driver/adc.h>
#include "saradc_client.h"

#define TAG "saradc_example"

#define SARADC_EXAMPLE_BUFFER_SIZE 32
#define SARADC_EXAMPLE_TIMEOUT 1000
/**
 * @brief SARADC basic usage example
 *
 * This example demonstrates how to:
 * 1. Initialize SARADC driver
 * 2. Acquire ADC resource
 * 3. Initialize ADC channel
 * 4. Start ADC conversion
 * 5. Read ADC raw data
 * 6. Stop ADC conversion
 * 7. Deinitialize ADC channel
 * 8. Release ADC resource
 * 9. Deinitialize SARADC driver
 */
uint16_t saradc_example(UINT8 adc_chan)
{
    bk_err_t ret;
    adc_config_t adc_config;
    uint16_t adc_data = 0;
    uint16_t cali_value = 0;

    // Acquire ADC resource
    ret = bk_adc_acquire();
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC acquire failed: %d\r\n", ret);
        bk_adc_release();
        return cali_value;
    }

    // Configure ADC parameters
    adc_config.chan = adc_chan;
    adc_config.adc_mode = ADC_CONTINUOUS_MODE;
    adc_config.src_clk = ADC_SCLK_XTAL_26M;
    adc_config.clk = 0x30A0C5;
    adc_config.saturate_mode = ADC_SATURATE_MODE_3;
    adc_config.steady_ctrl = 7;
    adc_config.adc_filter = 0;
    adc_config.sample_rate = 0;
    adc_config.is_open = 0;
    adc_config.vol_div = ADC_VOL_DIV_NONE;//adc internal voltage division ratio

    // Initialize ADC channel
    ret = bk_adc_init(adc_chan);
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC init failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    ret = bk_adc_set_config(&adc_config);
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC set config failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    bk_adc_enable_bypass_clalibration();
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC bypass cali failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    // Step 4: Start ADC conversion
    ret = bk_adc_start();
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC start failed: %d\r\n", ret);
        bk_adc_stop();
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    // Step 5: Read ADC data
    ret = bk_adc_read(&adc_data, SARADC_EXAMPLE_TIMEOUT);
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC read raw failed: %d\r\n", ret);
        bk_adc_stop();
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    cali_value = bk_adc_data_calculate(adc_data, adc_chan);
    BK_LOGE(TAG, "ADC data = %d, volt = %d(mv)\r\n", adc_data, cali_value);

    ret = bk_adc_stop();
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC stop failed: %d\r\n", ret);
    } else {
        //BK_LOGD(TAG, "ADC conversion stopped\r\n");
    }

    ret = bk_adc_deinit(adc_chan);
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC deinit failed: %d\r\n", ret);
    } else {
        //BK_LOGD(TAG, "ADC channel %d deinitialized\r\n", adc_chan);
    }

    ret = bk_adc_release();
    if (ret != BK_OK) {
        BK_LOGI(TAG, "ADC release failed: %d\r\n", ret);
    } else {
        //BK_LOGD(TAG, "ADC resource released\r\n");
    }

    return cali_value;
}

uint16_t saradc_test(UINT8 adc_chan)
{
    uint16_t cali_value = 0;

     // Before using the ADC functionality, the GPIO pin must be mapped to ADC mode.
     // This mapping needs to be done only once, and the GPIO must remain dedicated to ADC functionality afterward.
    bk_adc_chan_init_gpio(adc_chan);


    cali_value = saradc_example(adc_chan);


    bk_adc_chan_deinit_gpio(adc_chan);

    return cali_value;
}

// eof
