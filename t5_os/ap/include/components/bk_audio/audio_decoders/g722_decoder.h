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

#ifndef _G722_DECODER_H_
#define _G722_DECODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/bk_g722.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Enum of G722 Decoder rate
 */
typedef enum
{
    G722_DEC_RATE_48000 = 48000,    /* 48000 kbps */
    G722_DEC_RATE_56000 = 56000,    /* 56000 kbps */
    G722_DEC_RATE_64000 = 64000,    /* 64000 kbps */
} g722_decoder_rate_t;

/**
 * @brief      Enum of G722 Decoder options
 */
typedef enum
{
    G722_DEC_OPTION_NONE = 0,       /* No options */
    G722_DEC_OPTION_8K_SAMPLE = G722_SAMPLE_RATE_8000,  /* Decode to 8k samples/second */
    G722_DEC_OPTION_PACKED = G722_PACKED,  /* G.722 data is packed */
} g722_decoder_option_t;

/**
 * @brief      G722 Decoder configurations
 */
typedef struct
{
    int                     buf_sz;         /*!< Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    g722_decoder_rate_t     rate;           /*!< Bit rate */
    int                     options;        /*!< Decoder options */
} g722_decoder_cfg_t;

#define G722_DECODER_TASK_STACK          (1 * 1024)
#define G722_DECODER_TASK_CORE           (1)
#define G722_DECODER_TASK_PRIO           (5)
#define G722_DECODER_BUFFER_SIZE         (160)
#define G722_DECODER_OUT_BLOCK_SIZE      (320)
#define G722_DECODER_OUT_BLOCK_NUM       (1)

#define DEFAULT_G722_DECODER_CONFIG() {                 \
    .buf_sz             = G722_DECODER_BUFFER_SIZE,     \
    .out_block_size     = G722_DECODER_OUT_BLOCK_SIZE,  \
    .out_block_num      = G722_DECODER_OUT_BLOCK_NUM,   \
    .task_stack         = G722_DECODER_TASK_STACK,      \
    .task_core          = G722_DECODER_TASK_CORE,       \
    .task_prio          = G722_DECODER_TASK_PRIO,       \
    .rate               = G722_DEC_RATE_64000,          \
    .options            = G722_DEC_OPTION_NONE,         \
}

/**
 * @brief      Create a G722 decoder of Audio Element to decode incoming data using G722 format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t g722_decoder_init(g722_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif