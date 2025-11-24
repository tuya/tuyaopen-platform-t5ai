/*
 * Copyright (c) 2024 Beken Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OPUS_DEC_H_
#define _OPUS_DEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opus decoder handle
 */
typedef void *opus_dec_handle_t;

/**
 * @brief Opus decoder configuration
 */
typedef struct {
    uint32_t sample_rate;      /*!< Sample rate (8000, 12000, 16000, 24000, 48000) */
    uint8_t channels;          /*!< Number of channels (1: mono, 2: stereo) */
} opus_dec_config_t;

/**
 * @brief Opus decoder configuration for audio element
 */
typedef struct {
    uint32_t sample_rate;      /*!< Sample rate (8000, 12000, 16000, 24000, 48000) */
    uint8_t channels;          /*!< Number of channels (1: mono, 2: stereo) */
    int task_prio;             /*!< Task priority */
    int task_stack;            /*!< Task stack size */
    int task_core;             /*!< Task core */
    int out_block_size;        /*!< Output block size */
    int out_block_num;         /*!< Output block number */
    int buf_sz;                /*!< Buffer size */
} opus_dec_cfg_t;

#define OPUS_DEC_TASK_STACK          (6 * 1024)
#define OPUS_DEC_TASK_CORE           (1)
#define OPUS_DEC_TASK_PRIO           (5)
#define OPUS_DEC_SAMPLE_RATE         (16000)
#define OPUS_DEC_MAX_BITRATE         (64000)
#define OPUS_DEC_MAX_FRAME_DURATION  (120)
#define OPUS_DEC_BUFFER_SIZE         (OPUS_DEC_MAX_BITRATE*OPUS_DEC_MAX_FRAME_DURATION/1000/8*2) //double size to handle VBR
#define OPUS_DEC_OUT_BLOCK_SIZE      (OPUS_DEC_SAMPLE_RATE*OPUS_DEC_MAX_FRAME_DURATION/1000*2)
#define OPUS_DEC_OUT_BLOCK_NUM       (2)

#define DEFAULT_OPUS_DEC_CONFIG() {                 \
    .buf_sz             = OPUS_DEC_BUFFER_SIZE,     \
    .out_block_size     = OPUS_DEC_OUT_BLOCK_SIZE,  \
    .out_block_num      = OPUS_DEC_OUT_BLOCK_NUM,   \
    .task_stack         = OPUS_DEC_TASK_STACK,      \
    .task_core          = OPUS_DEC_TASK_CORE,       \
    .task_prio          = OPUS_DEC_TASK_PRIO,       \
    .sample_rate        = OPUS_DEC_SAMPLE_RATE,     \
    .channels           = 1,                        \
}

/**
 * @brief Initialize Opus decoder as audio element
 *
 * @param[in] config Opus decoder configuration
 * @return Audio element handle
 */
audio_element_handle_t opus_dec_init(opus_dec_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif /* _OPUS_DEC_H_ */