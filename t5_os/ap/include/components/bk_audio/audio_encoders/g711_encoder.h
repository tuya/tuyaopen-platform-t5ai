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

#ifndef _G711_ENCODER_H_
#define _G711_ENCODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Enum of G711 Encoder enc_mode
 */
typedef enum
{
    G711_ENC_MODE_A_LOW = 0,    /* a-law */
    G711_ENC_MODE_U_LOW,        /* u-law */
    G711_ENC_MODE_MAX,          /*!< Invalid mode */
} g711_encoder_mode_t;

/**
 * @brief      G711 Encoder configurations
 */
typedef struct
{
    int                     buf_sz;         /*!< Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    g711_encoder_mode_t     enc_mode;       /*!< 0: a-law  1: u-law */
} g711_encoder_cfg_t;

#define G711_ENCODER_TASK_STACK          (1 * 1024)
#define G711_ENCODER_TASK_CORE           (1)
#define G711_ENCODER_TASK_PRIO           (5)
#define G711_ENCODER_BUFFER_SIZE         (320)
#define G711_ENCODER_OUT_BLOCK_SIZE      (160)
#define G711_ENCODER_OUT_BLOCK_NUM       (1)

#define DEFAULT_G711_ENCODER_CONFIG() {                 \
    .buf_sz             = G711_ENCODER_BUFFER_SIZE,     \
    .out_block_size     = G711_ENCODER_OUT_BLOCK_SIZE,  \
    .out_block_num      = G711_ENCODER_OUT_BLOCK_NUM,   \
    .task_stack         = G711_ENCODER_TASK_STACK,      \
    .task_core          = G711_ENCODER_TASK_CORE,       \
    .task_prio          = G711_ENCODER_TASK_PRIO,       \
    .enc_mode           = G711_ENC_MODE_A_LOW,          \
}

/**
 * @brief      Create a G711 encoder of Audio Element to encode incoming data using G711 format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t g711_encoder_init(g711_encoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif

