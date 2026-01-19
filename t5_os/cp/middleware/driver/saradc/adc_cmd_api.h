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
#if CONFIG_SADC_API_TEST || CONFIG_ADC_API_TEST
#include <components/log.h>
#include <os/os.h>

#include <driver/adc.h>
#include "bk_saradc.h"
#include "adc_hal.h"
#include <driver/adc_types.h>

/* sadc maybe does not be used, when enable smp feature;
 * configure the macro based on system requirement.
 */
#define CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE  CONFIG_FREERTOS_SMP

#if CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
#include "spinlock.h"
#endif // CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE

#define ADC_TAG "adc"
#define ADC_LOGI(...) BK_LOGI(ADC_TAG, ##__VA_ARGS__)
#define ADC_LOGW(...) BK_LOGW(ADC_TAG, ##__VA_ARGS__)
#define ADC_LOGE(...) BK_LOGE(ADC_TAG, ##__VA_ARGS__)
#define ADC_LOGD(...) BK_LOGD(ADC_TAG, ##__VA_ARGS__)

#define DEFAULT_ADC_MODE                  ADC_CONTINUOUS_MODE
#define DEFAULT_ADC_SAMPLE_RATE           0x20
#define DEFAULT_ADC_SCLK                  ADC_SCLK_XTAL
#define DEFAULT_ADC_CLK                   2600000
#define DEFAULT_ADC_STEADY_TIME           7
#define DEFAULT_ADC_SAMPLE_UNIT_BYTES     2
#define DEFAULT_AVERAGE_SAMPLE_SIZE       32
#define DEFAULT_SATURATE_MODE             ADC_SATURATE_MODE_3
#define ADC_SAMPLE_THRESHOLD_DEFAULT      32
#define MAP_INVALID_ITEM                  0xFF

#define BK_ERR_ADC_INSUFFICIENT_MEM      (BK_ERR_ADC_BASE - 11) /**< ADC out of memory */
#define BK_ERR_ADC_DEINIT_MUTEX          (BK_ERR_ADC_BASE - 12) /**< ADC mutex deinit failed */
#define BK_ERR_ADC_DEINIT_READ_SEMA      (BK_ERR_ADC_BASE - 13) /**< ADC read sync deinit failed */

typedef void (*adc_isr_cb)(uint32_t param);
typedef void (*FUNC_2PV_PTR)(void *ctx, void *arg);

typedef enum
{
    SARADC_CALIBRATE_EXT_LOW1,
    SARADC_CALIBRATE_LOW1,
    SARADC_CALIBRATE_HIGH1,
    SARADC_CALIBRATE_TEMP_CODE25_1,
    SARADC_CALIBRATE_TEMP_STEP10_1,
    SARADC_CALIBRATE_MAX1,
} SARADC_MODE1;

struct sadc_context {
    beken_semaphore_t sync;
    beken_mutex_t lock;

    #if CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
    volatile spinlock_t multicore_lock;
    #endif // CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
};

struct sadc_data {
    uint16_t *buffer;
    uint16_t sampling_request;
    uint16_t sampling_index;

    uint16_t sadc_cali_val[SARADC_CALIBRATE_MAX1];
};

struct sadc_calib_ana_context {
    uint32_t ana_reg2_val;
    uint32_t ana_reg4_val;
    uint32_t ana_reg5_val;
    uint32_t ana_reg10_val;
};

struct sadc_statistics {
    uint32_t adc_isr_cnt;
    uint32_t adc_rx_total_cnt;
    uint32_t adc_rx_succ_cnt;
    uint32_t adc_rx_drop_cnt;
};

struct sadc_device {
    adc_hal_t hal;
    adc_config_t *channel_cfg[ADC_MAX];
    #if (CONFIG_SARADC_REVISION)
    uint16_t rf_active_patch;
    uint16_t rf_active_nums;
    adc_chan_t adc_chan;
    #endif

    struct sadc_context ctx;
    struct sadc_data data;

    adc_isr_cb isr_cb;
    void *cb_arg;

    /* data callback, and the first parameter is buffer
     * it is designated for compal module;
     * typedef void ( * IotAdcCallback_t )( uint16_t * pusConvertedData,
                                            void * pvUserContext );
     */
    FUNC_2PV_PTR data_callback;
    void *cb_ctx;

    #if CONFIG_ADC_STATIS
    struct sadc_statistics stats;
    #endif
};

static inline bk_err_t adc_context_init(struct sadc_context *ctx)
{
    bk_err_t ret;
    ret = rtos_init_mutex(&ctx->lock);
    if (kNoErr != ret) {
        return BK_ERR_ADC_INIT_MUTEX;
    }

    ret = rtos_init_semaphore(&ctx->sync, 1);
    if (BK_OK != ret) {
        return BK_ERR_ADC_INIT_READ_SEMA;
    }

    #if CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
    ctx->multicore_lock.owner = SPIN_LOCK_FREE;
    ctx->multicore_lock.count = 0;
    #endif // CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE

    return ret;
}

static inline bk_err_t adc_context_lock(struct sadc_context *ctx)
{
    #if CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
    spin_lock(&ctx->multicore_lock);
    #endif // CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE

    return rtos_lock_mutex(&ctx->lock);
}

static inline bk_err_t adc_context_release(struct sadc_context *ctx)
{
    #if CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE
    spin_unlock(&ctx->multicore_lock);
    #endif // CONFIG_ENABLE_USING_SADC_IN_MULTI_CORE

    return rtos_unlock_mutex(&ctx->lock);
}

static inline bk_err_t adc_context_deinit(struct sadc_context *ctx)
{
    bk_err_t ret;

    ret = rtos_deinit_mutex(&ctx->lock);
    if (kNoErr != ret) {
        return BK_ERR_ADC_DEINIT_MUTEX;
    }
    ctx->lock = NULL;

    ret = rtos_deinit_semaphore(&ctx->sync);
    if (BK_OK != ret) {
        return BK_ERR_ADC_DEINIT_READ_SEMA;
    }
    ctx->sync = NULL;

    return ret;
}

bk_err_t bk_adc_get_config(uint32 adc_ch, adc_config_t **config);
bk_err_t bk_adc_is_valid_ch(uint32_t ch);
bk_err_t bk_adc_register_isr_callback_iot_callback(void *iot_callback, void *p_iot_context);
bk_err_t bk_adc_unregister_isr_iot_callback_test(void);

#if CONFIG_SARADC_V1P2
bk_err_t bk_adc_enter_calib_mode(void);
bk_err_t bk_adc_cont_get_raw(uint8_t chan_id, uint16_t *data, uint32_t count);
bk_err_t bk_adc_cont_start(adc_config_t *config, uint8_t chan_id, uint16_t *data, uint32_t count);
#endif // CONFIG_SARADC_V1P2

/**
 * @brief     Init the ADC driver
 *
 * This API init the resoure common to all ADC  id:
 *   - Init ADC driver control memory
 *
 * @attention 1. This API should be called before any other ADC APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_adc_driver_init_test(void);

/**
 * @brief     Deinit the ADC driver
 *
 * This API free all resource related to ADC and power down all ADC.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_adc_driver_deinit_test(void);

/**
 * @brief     adc set config
 *
 * This API config adc params
 *
 * @param config adc parameter settings

 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM : param is null
 *    - BK_ERR_ADC_INVALID_MODE:adc invalid mode
 *    - BK_ERR_ADC_INVALID_SCLK_MODE: ADC source clock is invalid
 *    - BK_ERR_ADC_INVALID_ID: ADC id is invalid
 *    - others: other errors.
 */
bk_err_t bk_adc_channel_init(adc_config_t *config);

/**
 * @brief     Deinit the ADC id
 *
 * This API deinit the ADC id:
 *   - Stop the ADC
 *   - Disable the ADC id interrupt
 *   - Power down the ADC
 *   - Unmap gpio id
 *
 * @param id ADC id
 *
 * @attention 1. This API should be called before use adc.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_ADC_NOT_INIT: ADC driver not init
 *    - BK_ERR_ADC_INVALID_ID: ADC id is invalid
 *    - others: other errors.
 */
bk_err_t bk_adc_channel_deinit(adc_chan_t id);

/**
 * @brief     Start read the average value of sampling data
 *
 * @param chan_id: adc channel index
 * @param data store the average value of all sample values in ADC software buffers
 * @param timeout adc read semaphore timeout
 *
 * @return

 *    - BK_ERR_ADC_BUSY: ADC is busy
 *    - BK_ERR_ADC_INVALID_MODE: ADC mode is invalid
 *    - others: other errors.
 */
bk_err_t bk_adc_channel_read(adc_chan_t chan_id, uint16_t *data, uint32_t timeout);

/**
 * @brief     Start read the raw ADC data in continuous mode
 *
 * @attention 1. The read_buf is in unit of uint16_t, the application needs to
 *               malloc (sample_cnt * 2) bytes for read_buf.
 * @attention 2. The maximum value of size is CONFIG_ADC_BUF_SIZE, if the size
 *               exceeds CONFIG_ADC_BUF_SIZE, the API returns BK_ERR_ADC_SIZE_TOO_BIG.
 *
 * @param channel_id: adc channel index
 * @param read_buf application malloc buffer which save the current adc value
 * @param size the size of read_buf, the unit of size is uint16_t
 * @param timeout adc read semaphore timeout
 *
 * @return
 *    - BK_ERR_ADC_BUSY: ADC is busy
 *    - BK_ERR_ADC_INVALID_MODE: ADC mode is invalid
 *    - BK_ERR_ADC_SIZE_TOO_BIG: size is too big
 *    - others: other errors.
 */
bk_err_t bk_adc_channel_raw_read(adc_chan_t channel_id, uint16_t* buf, uint32_t sample_cnt, uint32_t timeout);

/**
 * @brief     Register the adc interrupt service routine
 *
 * @param isr ADC intterrupt callback
 * @param param ADC sample data size which depend on user
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_ADC_NOT_INIT: adc not init
 *    - others: other errors.
 */
bk_err_t bk_adc_register_isr_callback(adc_isr_t isr, uint32_t param);

/**
 * @brief     Register the adc interrupt service routine
 *
 * @param NULL
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_ADC_NOT_INIT: adc not init
 *    - others: other errors.
 */
bk_err_t bk_adc_unregister_isr_callback(void);

/**
 * @brief     adc set voltage div
 *
 * @param adc_chan adc channel
 * @param vol_div voltage div
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_ADC_INVALID_ID: ADC id is invalid
 *    - others: other errors.
 */
bk_err_t bk_adc_set_vol_div(adc_chan_t adc_chan, adc_vol_div_t vol_div);

/**
 * @brief     adc calculate voltage
 *
 * @param adc_val adc value
 * @param adc_chan adc channel
 *
 * @return voltage value, the unit is mv
 */
float bk_adc_data_calculate_test(uint16_t adc_val, adc_chan_t adc_chan);
#endif
//eof

