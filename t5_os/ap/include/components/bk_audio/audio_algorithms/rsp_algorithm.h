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

#ifndef _RSP_ALGORITHM_H_
#define _RSP_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#include <modules/audio_rsp.h>
#include <modules/audio_rsp_types.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief      RSP algorithm configurations
 */
typedef struct
{
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    aud_rsp_cfg_t           rsp_cfg;            /*!< rsp config */
    int                     out_block_num;      /*!< Number of output block, the size of block is frame size of 20ms audio data */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
} rsp_algorithm_cfg_t;


#define RSP_ALGORITHM_TASK_STACK          (2 * 1024)
#define RSP_ALGORITHM_TASK_CORE           (1)
#define RSP_ALGORITHM_TASK_PRIO           (3)
#define RSP_ALGORITHM_OUT_BLOCK_NUM       (2)


#define RSP_ALGORITHM_COMPLEXITY       (6)
#define RSP_ALGORITHM_SRC_CH           (1)
#define RSP_ALGORITHM_DEST_CH          (1)
#define RSP_ALGORITHM_SRC_BITS         (16)
#define RSP_ALGORITHM_DEST_BITS        (16)
#define RSP_ALGORITHM_SRC_RATE         (16000)//(8000)
#define RSP_ALGORITHM_DEST_RATE        (16000)
#define RSP_ALGORITHM_DOWN_CH_IDX      (0)


#define DEFAULT_RSP_ALGORITHM_CONFIG() {                       \
    .task_stack           = RSP_ALGORITHM_TASK_STACK,          \
    .task_core            = RSP_ALGORITHM_TASK_CORE,           \
    .task_prio            = RSP_ALGORITHM_TASK_PRIO,           \
    .rsp_cfg = {                                               \
        .complexity       = RSP_ALGORITHM_COMPLEXITY,          \
        .src_ch           = RSP_ALGORITHM_SRC_CH,              \
        .dest_ch          = RSP_ALGORITHM_DEST_CH,             \
        .src_bits         = RSP_ALGORITHM_SRC_BITS,            \
        .dest_bits        = RSP_ALGORITHM_DEST_BITS,           \
        .src_rate         = RSP_ALGORITHM_SRC_RATE,            \
        .dest_rate        = RSP_ALGORITHM_DEST_RATE,           \
        .down_ch_idx      = RSP_ALGORITHM_DOWN_CH_IDX,         \
    },                                                         \
    .out_block_num        = RSP_ALGORITHM_OUT_BLOCK_NUM,       \
    .multi_out_port_num   = 0,                                 \
}

/**
 * @brief      Create a rsp algorithm of Audio Element to resample
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t rsp_algorithm_init(rsp_algorithm_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif

