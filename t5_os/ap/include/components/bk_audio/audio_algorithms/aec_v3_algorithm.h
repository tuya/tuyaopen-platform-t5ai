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

#ifndef _AEC_V3_ALGORITHM_H_
#define _AEC_V3_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/audio_param_ctrl.h>
#ifdef __cplusplus
extern "C" {
#endif

/*
//  AEC: Acoustic Echo Cancellation
//  AGC: Automatic Gain Control
//  NS:  Noise Suppression

                                                           +-----------------+
                                                           |                 |
                                                           |  Hardware Mode  |
                                                           |                 |
                         +---------------------------------+-----------------+----------------------------------------------------------+
                         |                                                                                                              |
                         |                               +------------------+                                                           |
                         |                               |                  |                                                           |
                         |                            +->| reference signal |---+                                                       |
                         |                            |  |                  |   |                                                       |
                         |                            |  +------------------+   |                                                       |
+----------------+       |  +---------------------+   |                         |  +-----------+    +-----------+    +-----------+      |
|                |       |  |                     |   |                         +->|           |    |           |    |           |      |
| Audio ADC Fifo |-------|->|  L&R chl separation |---+                            |    AEC    |--->|    NS     |--->|    AGC    |      |
|                |       |  |                     |   |                         +->|           |    |           |    |           |      |
+----------------+       |  +---------------------+   |                         |  +-----------+    +-----------+    +-----------+      |
                         |                            |  +------------------+   |                                                       |
                         |                            |  |                  |   |                                                       |
                         |                            +->|  source signal   |---+                                                       |
                         |                               |                  |                                                           |
                         |                               +------------------+                                                           |
                         |                                                                                                              |
                         +--------------------------------------------------------------------------------------------------------------+

                                                           +-----------------+
                                                           |                 |
                                                           |  Software Mode  |
                                                           |                 |
                         +---------------------------------+-----------------+-----------------------------------------------------+
                         |                                                                                                         |
+----------------+       |  +-------------+      +------------------+                                                              |
|                |       |  |             |      |                  |                                                              |
| Audio ADC Fifo |-------|->| L chl data  |----->|  source signal   |---+                                                          |
|                |       |  |             |      |                  |   |                                                          |
+----------------+       |  +-------------+      +------------------+   |                                                          |
                         |                                              |    +-----------+     +-----------+    +-----------+      |
                         |                                              +--->|           |     |           |    |           |      |
                         |                                                   |    AEC    |---> |    NS     |--->|    AGC    |      |
                         |                                              +--->|           |     |           |    |           |      |
                         |                                              |    +-----------+     +-----------+    +-----------+      |
+----------------+       |                       +------------------+   |                                                          |
|                |       |                       |                  |   |                                                          |
| input ringbuff |-------|---------------------->| reference signal |---+                                                          |
|                |       |                       |                  |                                                              |
+----------------+       |                       +------------------+                                                              |
                         |                                                                                                         |
                         +---------------------------------------------------------------------------------------------------------+

*/
typedef enum
{
    AEC_MODE_HARDWARE,      /*!< hardware mode: Hardware mode get source and reference signal through audio adc L and R channel. Audio adc L channel
                                 connect to mic, and collect source signal. Audio adc R channel connect to speaker, and collect reference signal. */
    AEC_MODE_SOFTWARE       /*!< software mode: Software mode get source and reference signal through audio adc L and software writting. Audio adc L
                                 channel connect to mic, and collect source signal. Software write speaker data to input ringbuffer to support reference signal. */
} aec_v3_mode_t;

typedef enum
{
    NS_CLOSE = 0,
    NS_AI    = 1,
    NS_TRADITION = 2,
    NS_MODE_MAX,
}ns_type_t;

typedef enum 
{
    VAD_NONE              = (0x00),
    VAD_SPEECH_START      = (0x01),
    VAD_SPEECH_END        = (0x02),
    VAD_SILENCE           = (0x03),
}vad_state_t;

// Modified by TUYA Start
typedef enum 
{
    DUAL_CH_0_DEGREE   = (0x00),
    DUAL_CH_90_DEGREE  = (0x01),
}dual_ch_dir_t;
// Modified by TUYA End

typedef struct 
{
    aec_v3_mode_t mode;        /*!< aec work mode: hardware mode or software mode */
    uint32_t fs;            /*!< Sample rate (8000 or 16000) */
    /* default value */
    uint16_t init_flags;
    /* aec */
    uint32_t delay_points;  /*!< set delay points of ref data according to dump data */
    uint32_t ec_depth;      /*!< recommended value range: 1~50, the greater the echo, the greater the value setting */
    uint8_t ref_scale;      /*!< value range:0,1,2, the greater the signal amplitude, the greater the setting */
    uint8_t voice_vol;      /*!< the voice volume level */
    uint32_t TxRxThr;       /*!< the max amplitude of rx audio data */
    uint32_t TxRxFlr;       /*!< the min amplitude of rx audio data */
    /* ns */
    uint8_t ns_type;        /*!< see ns_type_t */
    uint8_t ns_filter;      /*!< recommended value range: 1~8, the lower the noise, the lower the level */
    uint8_t ns_level;       /*!< recommended value range: 1~8, the lower the noise, the lower the level */
    uint8_t ns_para;        /*!< value range:0,1,2, the lower the noise, the lower the level, the default valude is recommended */
    /* drc */
    uint8_t drc;            /*!< recommended value range:0x10~0x1f, the greater the value, the greater the volume */
    
// Modified by TUYA Start
    uint8_t ec_filter;      /*!< 0x01/0x03/0x07,output echo cancellation data |= 1<<5 */
    uint8_t interweave;     /*!< 0/1 */
    int16_t dist;           /*!< 0:DUAL_CH_90_DEGREE,1:DUAL_CH_0_DEGREE dual mic distance is within 3cm;2: DUAL_CH_0_DEGREE dual mic distance is in the range of 3~4cm*/
    uint8_t mic_swap;       /*!< 0:default/1:swap dual channel data */
    uint8_t ec_only_output; /*!< 0:disable,1:enable */
    uint8_t dual_perp;      /*!< dual channel direction,0:0 degree,1:90 degree*/
// Modified by TUYA End
} aec_v3_cfg_t;

typedef struct {
    int16_t vad_enable;
    int16_t vad_start_threshold;
    int16_t vad_stop_threshold;
    int16_t vad_silence_threshold;
    int16_t vad_eng_threshold;
    int     vad_bad_frame;
    uint32_t vad_buf_size;
    uint32_t vad_frame_size;
} vad_cfg_t;

// Modified by TUYA Start
typedef int (*ec_out_callback)(int32_t *buffer, uint16_t len);
typedef int (*vad_state_callback)(int32_t state);
// Modified by TUYA End

/**
 * @brief      AEC algorithm configurations
 */
typedef struct
{
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    aec_v3_cfg_t            aec_cfg;            /*!< aec config */
    vad_cfg_t               vad_cfg;            /*!< vad config */
    int                     out_block_size;     /*!< the size of block*/
    int                     out_block_num;      /*!< Number of output block*/
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
    int                     dual_ch;            /*!< Enable dual channel input(1)/Disable dual channel input(0)*/
// Modified by TUYA Start
    ec_out_callback         ec_out_cb;          /*!< echo cancellation output callback function */
    vad_state_callback      vad_state_cb;       /*!< VAD state callback function */
// Modified by TUYA End
} aec_v3_algorithm_cfg_t;

#define AEC_V3_DELAY_SAMPLE_POINTS_MAX           (1000)

// Modified by TUYA Start
#define AEC_V3_ALGORITHM_TASK_STACK          (4 * 1024)
#define AEC_V3_ALGORITHM_TASK_CORE           (1)
#define AEC_V3_ALGORITHM_TASK_PRIO           (4)
#define AEC_V3_ALGORITHM_OUT_BLOCK_NUM       (2)

#define AEC_V3_ALGORITHM_FS                  (16000)
#define AEC_V3_DELAY_POINTS                  (16)
#define AEC_V3_ALGORITHM_EC_DEPTH            (0xa)
#define AEC_V3_ALGORITHM_REF_SCALE           (0)
#define AEC_V3_ALGORITHM_NS_LEVEL            (7)
#define AEC_V3_ALGORITHM_NS_PARA             (2)
#define AEC_V3_ALGORITHM_INIT_FLAG           (0x1f)
#define AEC_V3_ALGORITHM_NS_FILTER           (0x7)
#define AEC_V3_ALGORITHM_VOL                 (0xd)
#define AEC_V3_ALGORITHM_EC_FILTER           (0x7)
#define AEC_V3_ALGORITHM_DRC                 (0x0)
#define AEC_V3_ALGORITHM_INTERWEAVE          (0x1)
#define AEC_V3_ALGORITHM_MIC_SWAP            (0x0)
#define AEC_V3_ALGORITHM_MIC_DIST            (0x2)
#define AEC_V3_ALGORITHM_EC_ONLY_OUTPUT      (0x1)
#define AEC_V3_VAD_BAD_FRAME_NUM             (16)
// Modified by TUYA End


#define DEFAULT_AEC_V3_ALGORITHM_CONFIG() {                 \
    .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
    .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
    .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
    .aec_cfg = {                                            \
        .mode = AEC_MODE_SOFTWARE,                          \
        .fs = AEC_V3_ALGORITHM_FS,                          \
        .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
        .delay_points = AEC_V3_DELAY_POINTS,                \
        .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
        .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
        .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
        .ns_type = NS_TRADITION,                                   \
        .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
        .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
        .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
        .drc = AEC_V3_ALGORITHM_DRC,                        \
        .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
        .interweave = AEC_V3_ALGORITHM_INTERWEAVE,          \
        .dist = AEC_V3_ALGORITHM_MIC_DIST,                  \
        .mic_swap = AEC_V3_ALGORITHM_MIC_SWAP,              \
        .ec_only_output = AEC_V3_ALGORITHM_EC_ONLY_OUTPUT,  \
        .dual_perp = DUAL_CH_0_DEGREE,                      \
    },                                                      \
    .vad_cfg = {                                            \
        .vad_enable = 1,                                    \
        .vad_start_threshold = 480,                         \
        .vad_stop_threshold = 960,                          \
        .vad_silence_threshold = 320,                       \
        .vad_eng_threshold =2000,                           \
        .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,          \
        .vad_buf_size = 15360,                              \
        .vad_frame_size = 640,                              \
    },                                                      \
    .out_block_size = 640,                                  \
    .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,        \
    .multi_out_port_num = 0,                                \
    .dual_ch = 0,                                           \
}

/**
 * @brief      Create a Aec algorithm of Audio Element to echo cancellation
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t aec_v3_algorithm_init(aec_v3_algorithm_cfg_t *config);

/**
 * @brief      Set AEC V3 algorithm configuration parameters
 *
 * @param[in]      aec_algorithm  The aec algorithm handle
 * @param[in]      aec_config     The aec configuration
 *
 * @return     The status of the operation
 *                 - BK_OK: success
 *                 - BK_FAIL: failed
 */
bk_err_t aec_v3_algorithm_set_config(audio_element_handle_t aec_algorithm, void *aec_config);

/**
 * @brief      Get AEC V3 algorithm configuration parameters
 *
 * @param[in]      aec_algorithm  The aec algorithm handle
 * @param[out]     aec_config     The aec configuration to store the retrieved parameters
 *
 * @return     The status of the operation
 *                 - BK_OK: success
 *                 - BK_FAIL: failed
 */
bk_err_t aec_v3_algorithm_get_config(audio_element_handle_t aec_algorithm, void *aec_config);

// Modified by TUYA Start
/**
 * @brief      Get current VAD state of AEC V3 algorithm
 *
 * @param[in]  aec_algorithm  The aec algorithm handle
 *
 * @return     The current VAD state
 *                 - VAD_NONE: no valid VAD state
 *                 - VAD_SPEECH_START: speech start detected
 *                 - VAD_SPEECH_END: speech end detected
 *                 - VAD_SILENCE: silence detected
 */
int aec_v3_algorithm_get_vad_state(audio_element_handle_t aec_algorithm);


// Modified by TUYA End
typedef int (*aec_vad_process_fun)(short *mic_data, short *ref_data, short *out_data); 
bk_err_t aec_v3_algorithm_set_user_process(aec_vad_process_fun user_process_fun);

#ifdef __cplusplus
}
#endif

#endif

