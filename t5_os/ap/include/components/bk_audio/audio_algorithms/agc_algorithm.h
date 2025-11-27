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

#ifndef _AGC_ALGORITHM_H_
#define _AGC_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/audio_agc_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief      AGC algorithm configurations
 */
typedef struct
{
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    bk_agc_config_t         agc_cfg;        /*!< agc config */
    int32_t                 minLevel;       /*!< Minimum possible mic level (suggested value: 0) */
    int32_t                 maxLevel;       /*!< Maximum possible mic level (suggested value: 255) */
    uint32_t                fs;             /*!< Sample rate (8000 or 16000) */
    int                     out_rb_block;   /*!< Block number of output ringbuffer, the unit is frame size of 10ms audio data */
} agc_algorithm_cfg_t;

#define AGC_ALGORITHM_TASK_STACK          (1 * 1024)
#define AGC_ALGORITHM_TASK_CORE           (1)
#define AGC_ALGORITHM_TASK_PRIO           (5)
#define AGC_ALGORITHM_MINLEVEL            (0)
#define AGC_ALGORITHM_MAXLEVEL            (255)
#define AGC_ALGORITHM_FS                  (8000)
#define AGC_ALGORITHM_OUT_RB_BLOCK        (4)

#define AGC_ALGORITHM_TARGET_LEVEL_DBFS   (3)
#define AGC_ALGORITHM_COMPRESSION_GAIN_DB (9)
#define AGC_ALGORITHM_LIMITER_ENABLE      (1)

#define DEFAULT_AGC_ALGORITHM_CONFIG() {                           \
    .task_stack            = AGC_ALGORITHM_TASK_STACK,             \
    .task_core             = AGC_ALGORITHM_TASK_CORE,              \
    .task_prio             = AGC_ALGORITHM_TASK_PRIO,              \
    .agc_cfg = {                                                   \
        .targetLevelDbfs   = AGC_ALGORITHM_TARGET_LEVEL_DBFS,      \
        .compressionGaindB = AGC_ALGORITHM_COMPRESSION_GAIN_DB,    \
        .limiterEnable     = AGC_ALGORITHM_LIMITER_ENABLE,         \
    },                                                             \
    .minLevel              = AGC_ALGORITHM_MINLEVEL,               \
    .maxLevel              = AGC_ALGORITHM_MAXLEVEL,               \
    .fs                    = AGC_ALGORITHM_FS,                     \
    .out_rb_block          = AGC_ALGORITHM_OUT_RB_BLOCK,           \
    }

/**
 * @brief      Create a Agc algorithm of Audio Element to automatic control gain
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t agc_algorithm_init(agc_algorithm_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif

