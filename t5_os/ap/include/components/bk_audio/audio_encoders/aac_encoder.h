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

#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/fdk_aac_enc/aacenc_lib.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief      AAC Encoder configurations
 */
typedef struct
{
    uint8_t                 chl_num;            /*!< channel number */
    uint32_t                samp_rate;          /*!< sample rate */
    uint8_t                 bits;               /*!< Bit wide (8, 16, 24, 32 bits) */
    uint32_t                modules;            /*!< modules, refer to "encModules" parameter description of "aacEncOpen" spi in aacenc_lib.h */
    uint8_t                 aot;                /*!< Audio object type, refer to "AACENC_AOT" parameter description in aacenc_lib.h */
    uint32_t                bitrate;            /*!< encoder bitrate, refer to "AACENC_BITRATE" parameter description in aacenc_lib.h */
    uint8_t                 bitrate_mode;       /*!< Bitrate mode, refer to "AACENC_BITRATEMODE" parameter description in aacenc_lib.h */
    int8_t                  sbr_mode;           /*!< sbr mode, refer to "AACENC_SBR_MODE" parameter description in aacenc_lib.h */
    uint32_t                granule_length;     /*!< audio frame length in samples, refer to "AACENC_GRANULE_LENGTH" parameter description in aacenc_lib.h */
    uint8_t                 chl_order;          /*!< Input audio data channel ordering scheme, refer to "AACENC_CHANNELORDER" parameter description in aacenc_lib.h */
    uint8_t                 afterburner_en;     /*!< afterburner feature enable, refer to "AACENC_AFTERBURNER" parameter description in aacenc_lib.h */
    uint8_t                 transport_type;     /*!< Transport type, refer to "AACENC_TRANSMUX" parameter description in aacenc_lib.h */

    int                     buffer_len;         /*!< Buffer length use for aac Element */
    uint32_t                in_pool_len;        /*!< input data pool size(byte), save data if the valid data in pool is less than one frame */
    uint32_t                out_buffer_len;     /*!< one frame data size(byte) of output aac data, less than one frame size */
    int                     out_block_size;     /*!< Size of output block, at  least larger than the size of one frame of output aac data */
    int                     out_block_num;      /*!< Number of output block */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
} aac_encoder_cfg_t;


#define AAC_ENCODER_CHL_NUM             (1)
#define AAC_ENCODER_SAMP_RATE           (8000)
#define AAC_ENCODER_BITS                (16)
#define AAC_ENCODER_MODULES             (1)
#define AAC_ENCODER_AOT                 (2)
#define AAC_ENCODER_BITRATE             (32000)
#define AAC_ENCODER_BITRATE_MODE        (0)
#define AAC_ENCODER_SBR_MODE            (1)
#define AAC_ENCODER_GRANULE_LENGTH      (1024)
#define AAC_ENCODER_CHL_ORDER           (1)
#define AAC_ENCODER_AFTERBURNER_EN      (0)
#define AAC_ENCODER_TRANSPORT_TYPE      (TT_MP4_ADTS)

#define AAC_ENCODER_BUFFER_LEN          (AAC_ENCODER_GRANULE_LENGTH * AAC_ENCODER_BITS / 8 * AAC_ENCODER_CHL_NUM)
#define AAC_ENCODER_IN_POOL_LEN         (AAC_ENCODER_BUFFER_LEN + AAC_ENCODER_SAMP_RATE * AAC_ENCODER_BITS / 8 * AAC_ENCODER_CHL_NUM / 1000 * 20)
#define AAC_ENCODER_OUT_BUFFER_LEN      (AAC_ENCODER_BUFFER_LEN)
#define AAC_ENCODER_OUT_BLOCK_SIZE      (AAC_ENCODER_BUFFER_LEN)
#define AAC_ENCODER_OUT_BLOCK_NUM       (2)
#define AAC_ENCODER_TASK_STACK          (15 * 1024)
#define AAC_ENCODER_TASK_CORE           (1)
#define AAC_ENCODER_TASK_PRIO           (BEKEN_DEFAULT_WORKER_PRIORITY - 1)


#define DEFAULT_AAC_ENCODER_CONFIG() {                   \
    .chl_num            = AAC_ENCODER_CHL_NUM,           \
    .samp_rate          = AAC_ENCODER_SAMP_RATE,         \
    .bits               = AAC_ENCODER_BITS,              \
    .modules            = AAC_ENCODER_MODULES,           \
    .aot                = AAC_ENCODER_AOT,               \
    .bitrate            = AAC_ENCODER_BITRATE,           \
    .bitrate_mode       = AAC_ENCODER_BITRATE_MODE,      \
    .sbr_mode           = AAC_ENCODER_SBR_MODE,          \
    .granule_length     = AAC_ENCODER_GRANULE_LENGTH,    \
    .chl_order          = AAC_ENCODER_CHL_ORDER,         \
    .afterburner_en     = AAC_ENCODER_AFTERBURNER_EN,    \
    .transport_type     = AAC_ENCODER_TRANSPORT_TYPE,    \
    .buffer_len         = AAC_ENCODER_BUFFER_LEN,        \
    .in_pool_len        = AAC_ENCODER_IN_POOL_LEN,       \
    .out_buffer_len     = AAC_ENCODER_OUT_BUFFER_LEN,    \
    .out_block_size     = AAC_ENCODER_OUT_BLOCK_SIZE,    \
    .out_block_num      = AAC_ENCODER_OUT_BLOCK_NUM,     \
    .task_stack         = AAC_ENCODER_TASK_STACK,        \
    .task_core          = AAC_ENCODER_TASK_CORE,         \
    .task_prio          = AAC_ENCODER_TASK_PRIO,         \
}

/**
 * @brief      Create a AAC encoder of Audio Element to encode incoming data using AAC format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t aac_encoder_init(aac_encoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif  //_AAC_ENCODER_H_

