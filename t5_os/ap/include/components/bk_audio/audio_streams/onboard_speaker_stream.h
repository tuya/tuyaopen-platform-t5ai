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

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/aud_dac_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Onboard Speaker Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    uint8_t                 chl_num;            /*!< speaker channel number */
    uint32_t                sample_rate;        /*!< speaker sample rate */
    int32_t                 dig_gain;           /*!< audio dac digital gain: value range: 0x00 ~ 0x3f(-45db ~ 18db, 0x2d: 0db), suggest: 0x2d */
    int32_t                 ana_gain;           /*!< audio dac analog gain: value range: , suggest:  */
    aud_dac_work_mode_t     work_mode;          /*!< audio dac mode: signal_ended/differen */
    uint8_t                 bits;               /*!< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t               clk_src;            /*!< audio clock: XTAL(26MHz)/APLL */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
    uint32_t                frame_size;         /*!< the length of one frame speaker data dma carried (suggest 20ms data) */
    uint32_t                pool_length;        /*!< speaker data pool size, the unit is byte */
    uint32_t                pool_play_thold;    /*!< the play threshold of pool, the unit is byte */
    uint32_t                pool_pause_thold;   /*!< the pause threshold of pool, the unit is byte */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
} onboard_speaker_stream_cfg_t;


#define ONBOARD_SPEAKER_STREAM_TASK_STACK          (1024)
#define ONBOARD_SPEAKER_STREAM_TASK_CORE           (1)
#define ONBOARD_SPEAKER_STREAM_TASK_PRIO           (3)

#define ONBOARD_SPEAKER_STREAM_CFG_DEFAULT() {                 \
        .chl_num = 1,                                          \
        .sample_rate = 8000,                                   \
        .dig_gain = 0x2d,                                      \
        .ana_gain = 0x07,                                      \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                 \
        .bits = 16,                                            \
        .clk_src = AUD_CLK_XTAL,                               \
        .multi_out_port_num = 0,                               \
        .frame_size = 320,                                     \
        .pool_length = 0,                                      \
        .pool_play_thold = 0,                                  \
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
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int bits, int ch);

/**
 * @brief      Updata onboard speaker stream digital gain.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      gain  speaker digital gain, range: 0x00 ~ 0x3f(-45db ~ 18db, 0x2d: 0db)
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_digital_gain(audio_element_handle_t onboard_speaker_stream, uint8_t gain);

/**
 * @brief      Get onboard speaker stream digital gain.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in,out]  gain  speaker digital gain
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_get_digital_gain(audio_element_handle_t onboard_speaker_stream, uint8_t *gain);

/**
 * @brief      Control onboard audio dac mute.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      value  mute value (1: mute, 0: unmute)
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_dac_mute_en(audio_element_handle_t onboard_speaker_stream, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
