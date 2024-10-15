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


#ifndef _ONBOARD_SPEAKER_STREAM_H_
#define _ONBOARD_SPEAKER_STREAM_H_

#include "audio_error.h"
#include "audio_element.h"
#include "audio_common.h"
#include <driver/aud_dac_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Onboard Speaker Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    uint8_t                 chl_num;          /*!< speaker channel number */
    uint32_t                samp_rate;        /*!< speaker sample rate */
    uint8_t                 spk_gain;         /*!< audio dac gain: value range:0x0 ~ 0x3f, suggest:0x2d */
    aud_dac_work_mode_t     work_mode;        /*!< audio dac mode: signal_ended/differen */
    uint8_t                 bits;             /*!< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t               clk_src;          /*!< audio clock: XTAL(26MHz)/APLL */
    int                     multi_out_rb_num; /*!< The number of multiple output ringbuffer */
    uint8_t                 pool_frame_num;   /*!< speaker data pool size, the unit is frame size(20ms) */
    uint8_t                 pool_play_thold;  /*!< the play threshold of pool, the unit is frame size(20ms) */
    uint8_t                 pool_pause_thold; /*!< the pause threshold of pool, the unit is frame size(20ms) */
    int                     task_stack;       /*!< Task stack size */
    int                     task_core;        /*!< Task running in core (0 or 1) */
    int                     task_prio;        /*!< Task priority (based on freeRTOS priority) */
} onboard_speaker_stream_cfg_t;


#define ONBOARD_SPEAKER_STREAM_TASK_STACK          (3072)
#define ONBOARD_SPEAKER_STREAM_TASK_CORE           (0)
#define ONBOARD_SPEAKER_STREAM_TASK_PRIO           (4)
//#define ONBOARD_SPEAKER_STREAM_RINGBUFFER_SIZE     (4 * 1024)

#define ONBOARD_SPEAKER_STREAM_CFG_DEFAULT() {             \
    .chl_num = 1,                                          \
    .samp_rate = 8000,                                     \
    .spk_gain = 0x16,                                      \
    .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                 \
    .bits = 16,                                            \
    .clk_src = AUD_CLK_XTAL,                               \
    .multi_out_rb_num = 0,                                 \
    .pool_frame_num = 5,                                   \
    .pool_play_thold = 5,                                  \
    .pool_pause_thold = 0,                                 \
    .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,       \
    .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,         \
    .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,         \
}

/**
 * @brief      Create a handle to an Audio Element to stream data from another Element to play.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t onboard_speaker_stream_init(onboard_speaker_stream_cfg_t *config);

/**
 * @brief      Updata onboard speaker stream include sample rate, bits and channel number on running.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      rate  sample rate
 * @param[in]      bits  sample bit
 * @param[in]      ch  channel number
 *
 * @return         Result
 *                 - Not NULL: success
 *                 - NULL: failed
 */
bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int bits, int ch);

#ifdef __cplusplus
}
#endif

#endif
