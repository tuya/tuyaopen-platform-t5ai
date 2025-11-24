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

#ifndef _AEC_ALGORITHM_H_
#define _AEC_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

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
} aec_mode_t;


typedef struct
{
    aec_mode_t mode;        /*!< aec work mode: hardware mode or software mode */
    uint32_t fs;            /*!< Sample rate (8000 or 16000) */
    uint32_t delay_points;  /*!< Delay sampling points between reference signal and source signal */
    /* aec */
    uint32_t ec_depth;      /*!< recommended value range: 1~50, the greater the echo, the greater the value setting */
    uint32_t TxRxThr;       /*!< the max amplitude of rx audio data */
    uint32_t TxRxFlr;       /*!< the min amplitude of rx audio data */
    uint8_t ref_scale;      /*!< value range:0,1,2, the greater the signal amplitude, the greater the setting */
    /* ns */
    uint8_t ns_level;       /*!< recommended value range: 1~8, the lower the noise, the lower the level */
    uint8_t ns_para;        /*!< value range:0,1,2, the lower the noise, the lower the level, the default valude is recommended */
} aec_cfg_t;

/**
 * @brief      AEC algorithm configurations
 */
typedef struct
{
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    aec_cfg_t               aec_cfg;            /*!< aec config */
    int                     out_block_num;      /*!< Number of output block, the size of block is frame size of 20ms audio data */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
} aec_algorithm_cfg_t;

#define AEC_DELAY_SAMPLE_POINTS_MAX           (1000)

#define AEC_ALGORITHM_TASK_STACK          (1 * 1024)
#define AEC_ALGORITHM_TASK_CORE           (1)
#define AEC_ALGORITHM_TASK_PRIO           (5)
#define AEC_ALGORITHM_OUT_BLOCK_NUM       (2)

#define AEC_ALGORITHM_FS                  (8000)
#define AEC_DELAY_POINTS                  (211)
#define AEC_ALGORITHM_EC_DEPTH            (50)
#define AEC_ALGORITHM_TXRXTHR             (30)
#define AEC_ALGORITHM_TXRXFLR             (6)
#define AEC_ALGORITHM_REF_SCALE           (0)
#define AEC_ALGORITHM_NS_LEVEL            (2)
#define AEC_ALGORITHM_NS_PARA             (1)

#define DEFAULT_AEC_ALGORITHM_CONFIG() {                           \
    .task_stack            = AEC_ALGORITHM_TASK_STACK,             \
    .task_core             = AEC_ALGORITHM_TASK_CORE,              \
    .task_prio             = AEC_ALGORITHM_TASK_PRIO,              \
    .aec_cfg = {                                                   \
        .mode              = AEC_MODE_SOFTWARE,                    \
        .fs                = AEC_ALGORITHM_FS,                     \
        .delay_points      = AEC_DELAY_POINTS,                     \
        .ec_depth          = AEC_ALGORITHM_EC_DEPTH,               \
        .TxRxThr           = AEC_ALGORITHM_TXRXTHR,                \
        .TxRxFlr           = AEC_ALGORITHM_TXRXFLR,                \
        .ref_scale         = AEC_ALGORITHM_REF_SCALE,              \
        .ns_level          = AEC_ALGORITHM_NS_LEVEL,               \
        .ns_para           = AEC_ALGORITHM_NS_PARA,                \
    },                                                             \
    .out_block_num         = AEC_ALGORITHM_OUT_BLOCK_NUM,          \
    .multi_out_port_num    = 0,                                    \
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
audio_element_handle_t aec_algorithm_init(aec_algorithm_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif

