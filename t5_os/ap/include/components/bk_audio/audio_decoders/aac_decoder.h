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

#ifndef _AAC_DECODER_H_
#define _AAC_DECODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/aacdec.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief      AAC Decoder configurations
 */
typedef struct
{
    uint32_t                main_buff_size;            /*!< mainbuff size */
    uint32_t                out_pcm_buff_size;         /*!< out pcm buffer size */
    int                     out_block_size;            /*!< Size of output block */
    int                     out_block_num;             /*!< Number of output block */
    int                     task_stack;                /*!< Task stack size */
    int                     task_core;                 /*!< Task running in core (0 or 1) */
    int                     task_prio;                 /*!< Task priority (based on freeRTOS priority) */
} aac_decoder_cfg_t;

#define AAC_DECODER_MAIN_BUFF_SIZE      (AAC_MAINBUF_SIZE)
#define AAC_DECODER_OUT_PCM_BUFF_SIZE   (AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2)
#define AAC_DECODER_OUT_BLOCK_NUM       (1)

#define AAC_DECODER_TASK_STACK          (4 * 1024)
#define AAC_DECODER_TASK_CORE           (1)
#define AAC_DECODER_TASK_PRIO           (BEKEN_DEFAULT_WORKER_PRIORITY - 1)


#define DEFAULT_AAC_DECODER_CONFIG() {                   \
    .main_buff_size     = AAC_DECODER_MAIN_BUFF_SIZE,    \
    .out_pcm_buff_size  = AAC_DECODER_OUT_PCM_BUFF_SIZE, \
    .out_block_size     = AAC_DECODER_OUT_PCM_BUFF_SIZE, \
    .out_block_num      = AAC_DECODER_OUT_BLOCK_NUM,     \
    .task_stack         = AAC_DECODER_TASK_STACK,        \
    .task_core          = AAC_DECODER_TASK_CORE,         \
    .task_prio          = AAC_DECODER_TASK_PRIO,         \
}

/**
 * @brief      Create a AAC decoder of Audio Element to decode incoming data using AAC format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t aac_decoder_init(aac_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif  /* _AAC_DECODER_H_ */

