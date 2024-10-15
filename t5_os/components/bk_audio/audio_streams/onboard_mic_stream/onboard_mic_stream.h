// Copyright 2022-2023 Beken
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


#ifndef _ONBOARD_MIC_STREAM_H_
#define _ONBOARD_MIC_STREAM_H_

#include "audio_error.h"
#include "audio_element.h"
#include "audio_common.h"
#include <driver/aud_adc_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ADC mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    uint8_t                 chl_num;          /*!< mic channel number */
    uint8_t                 bits;             /*!< Bit wide (8, 16 bits) */
    uint32_t                samp_rate;        /*!< mic sample rate */
    uint8_t                 mic_gain;         /*!< audio adc gain: value range:0x0 ~ 0x3f, suggest:0x2d */
    aud_adc_mode_t          mode;             /*!< mic interface mode: signal_ended/differen */
    aud_clk_t               clk_src;          /*!< audio clock: XTAL(26MHz)/APLL */
} adc_cfg_t;

#if 0
/**
 * @brief   LINE IN mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    uint8_t                 chl_num;          /*!< line in channel number */
    uint8_t                 bits;             /*!< Bit wide (8, 16 bits) */
    uint32_t                samp_rate;        /*!< line in sample rate */
    uint8_t                 mic_gain;         /*!< audio adc gain: value range:0x0 ~ 0x3f, suggest:0x2d */
} line_in_cfg_t;

/**
 * @brief   DTMF mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    uint8_t                 chl_num;          /*!< dmic channel number */
    uint8_t                 bits;             /*!< Bit wide (8, 16 bits) */
    uint32_t                samp_rate;        /*!< dmic sample rate */
    uint8_t                 mic_gain;         /*!< audio adc gain: value range:0x0 ~ 0x3f, suggest:0x2d */
} dtmf_cfg_t;

/**
 * @brief   DMIC mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    uint8_t                 chl_num;          /*!< speaker channel number */
    uint8_t                 bits;             /*!< Bit wide (8, 16 bits) */
    uint32_t                samp_rate;        /*!< speaker sample rate */
    uint8_t                 mic_gain;         /*!< audio dac gain: value range:0x0 ~ 0x3f, suggest:0x2d */
} dmic_cfg_t;


/**
 * @brief   Onboard MIC Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    aud_adc_work_mode_t     adc_mode;         /*!< Work mode (adc, line in, dtmf, dmic) */
    union {
        adc_cfg_t           adc_cfg;          /*!< ADC mode configuration */
        line_in_cfg_t       line_in_cfg;      /*!< LINE IN mode configuration */
        dtmf_cfg_t          dtmf_cfg;         /*!< DTMF mode configuration */
        dmic_cfg_t          dmic_cfg;         /*!< DMIC mode configuration */
    } mic_cfg;
    uint8_t                 out_frame_num;    /*!< Number of output ringbuffer, the unit is frame size(20ms) */
    int                     task_stack;       /*!< Task stack size */
    int                     task_core;        /*!< Task running in core (0 or 1) */
    int                     task_prio;        /*!< Task priority (based on freeRTOS priority) */
} onboard_mic_stream_cfg_t;



#define ONBOARD_MIC_STREAM_TASK_STACK          (3072)
#define ONBOARD_MIC_STREAM_TASK_CORE           (1)
#define ONBOARD_MIC_STREAM_TASK_PRIO           (3)

#define ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT() {                       \
    .adc_mode = AUD_ADC_WORK_MODE_ADC,                               \
    .mic_cfg.adc_cfg = {                                             \
                           .chl_num = 1,                             \
                           .bits = 16,                               \
                           .samp_rate = 8000,                        \
                           .mic_gain = 0x2d,                         \
                           .intf_mode = AUD_DAC_WORK_MODE_DIFFEN,    \
                       },                                            \
    .out_frame_num = 1,                                              \
    .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,                     \
    .task_core = ONBOARD_MIC_STREAM_TASK_CORE,                       \
    .task_prio = ONBOARD_MIC_STREAM_TASK_PRIO,                       \
}
#endif

/**
 * @brief   Onboard MIC Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    adc_cfg_t               adc_cfg;          /*!< ADC mode configuration */
    uint8_t                 out_frame_num;    /*!< Number of output ringbuffer, the unit is frame size(20ms) */
    int                     task_stack;       /*!< Task stack size */
    int                     task_core;        /*!< Task running in core (0 or 1) */
    int                     task_prio;        /*!< Task priority (based on freeRTOS priority) */
} onboard_mic_stream_cfg_t;



#define ONBOARD_MIC_STREAM_TASK_STACK          (3072)
#define ONBOARD_MIC_STREAM_TASK_CORE           (1)
#define ONBOARD_MIC_STREAM_TASK_PRIO           (3)

#define ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT() {               \
    .adc_cfg = {                                             \
                   .chl_num = 1,                             \
                   .bits = 16,                               \
                   .samp_rate = 8000,                        \
                   .mic_gain = 0x2d,                         \
                   .mode = AUD_ADC_MODE_DIFFEN,              \
                   .clk_src = AUD_CLK_XTAL,                  \
               },                                            \
    .out_frame_num = 1,                                      \
    .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,             \
    .task_core = ONBOARD_MIC_STREAM_TASK_CORE,               \
    .task_prio = ONBOARD_MIC_STREAM_TASK_PRIO,               \
}

/**
 * @brief      Create a handle to an Audio Element to stream data to another Element.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t onboard_mic_stream_init(onboard_mic_stream_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
