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

#ifndef _OPUS_ENC_H_
#define _OPUS_ENC_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Enum of Opus Encoder enc_mode
 */
typedef enum
{
    OPUS_ENC_MODE_VOIP = 0,         /* Optimized for voice over IP */
    OPUS_ENC_MODE_AUDIO,            /* Optimized for audio */
    OPUS_ENC_MODE_RESTRICTED_LOWDELAY, /* Optimized for low delay */
    OPUS_ENC_MODE_MAX,              /*!< Invalid mode */
} opus_enc_mode_t;

/**
 * @brief      Opus Encoder configurations
 */
typedef struct
{
    int                     buf_sz;         /*!< Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
    opus_enc_mode_t         enc_mode;       /*!< Encoding mode */
    int                     sample_rate;    /*!< Sample rate (8000, 12000, 16000, 24000, or 48000) */
    int                     channels;       /*!< Number of channels (1 or 2) */
    int                     bitrate;        /*!< Bitrate in bits per second */
    int                     frame_samples_per_channel; /*!< Frame length in samples per channel */
} opus_enc_cfg_t;

#define OPUS_ENC_TASK_STACK          (18 * 1024)
#define OPUS_ENC_TASK_CORE           (1)
#define OPUS_ENC_TASK_PRIO           (5)
#define OPUS_ENC_SAMPLE_RATE         (16000)
#define OPUS_ENC_BITRATE             (16000)
#define OPUA_ENC_DEFAULT_FRAME_DURITON (20) //20ms
#define OPUS_ENC_BUFFER_SIZE         (OPUS_ENC_SAMPLE_RATE*OPUA_ENC_DEFAULT_FRAME_DURITON/1000*2)
#define OPUS_ENC_OUT_BLOCK_SIZE      (OPUS_ENC_BITRATE*OPUA_ENC_DEFAULT_FRAME_DURITON/1000/8*2) //double size to handle VBR
#define OPUS_ENC_OUT_BLOCK_NUM       (2)

#define DEFAULT_OPUS_ENC_CONFIG() {                     \
    .buf_sz             = OPUS_ENC_BUFFER_SIZE,         \
    .out_block_size     = OPUS_ENC_OUT_BLOCK_SIZE,      \
    .out_block_num      = OPUS_ENC_OUT_BLOCK_NUM,       \
    .task_stack         = OPUS_ENC_TASK_STACK,          \
    .task_core          = OPUS_ENC_TASK_CORE,           \
    .task_prio          = OPUS_ENC_TASK_PRIO,           \
    .enc_mode           = OPUS_ENC_MODE_AUDIO,          \
    .sample_rate        = OPUS_ENC_SAMPLE_RATE,         \
    .channels           = 1,                            \
    .bitrate            = OPUS_ENC_BITRATE,             \
    .frame_samples_per_channel = 320,                   \
}

/**
 * @brief      Create an Opus encoder of Audio Element to encode incoming data using Opus format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t opus_enc_init(opus_enc_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif