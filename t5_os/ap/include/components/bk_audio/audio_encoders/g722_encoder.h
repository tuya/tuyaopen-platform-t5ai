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

#ifndef _G722_ENCODER_H_
#define _G722_ENCODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Enum of G722 Encoder bit rate
 */
typedef enum
{
    G722_ENC_RATE_48000 = 0,    /* 48kbps */
    G722_ENC_RATE_56000,        /* 56kbps */
    G722_ENC_RATE_64000,        /* 64kbps */
    G722_ENC_RATE_MAX,          /*!< Invalid rate */
} g722_encoder_rate_t;

/**
 * @brief      G722 Encoder configurations
 */
typedef struct
{
    int                     buf_sz;         /*!< Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    g722_encoder_rate_t     enc_rate;       /*!< 0: 48kbps  1: 56kbps  2: 64kbps */
    int                     options;        /*!< G722 options, combination of G722_SAMPLE_RATE_8000 and G722_PACKED */
} g722_encoder_cfg_t;

#define G722_ENCODER_TASK_STACK          (1 * 1024)
#define G722_ENCODER_TASK_CORE           (1)
#define G722_ENCODER_TASK_PRIO           (5)
#define G722_ENCODER_BUFFER_SIZE         (320)
#define G722_ENCODER_OUT_BLOCK_SIZE      (160)
#define G722_ENCODER_OUT_BLOCK_NUM       (1)

#define DEFAULT_G722_ENCODER_CONFIG() {                 \
    .buf_sz             = G722_ENCODER_BUFFER_SIZE,     \
    .out_block_size     = G722_ENCODER_OUT_BLOCK_SIZE,  \
    .out_block_num      = G722_ENCODER_OUT_BLOCK_NUM,   \
    .task_stack         = G722_ENCODER_TASK_STACK,      \
    .task_core          = G722_ENCODER_TASK_CORE,       \
    .task_prio          = G722_ENCODER_TASK_PRIO,       \
    .enc_rate           = G722_ENC_RATE_64000,          \
    .options            = 0,                            \
}

/**
 * @brief      Create a G722 encoder of Audio Element to encode incoming data using G722 format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t g722_encoder_init(g722_encoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif