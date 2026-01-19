/*
 * Copyright (c) 2024 Beken Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <string.h>
#include <components/bk_audio/audio_decoders/opus_dec.h>
#include <modules/opus.h>

#define TAG  "OPUS_DEC"

/**
 * @brief Opus decoder structure
 */
typedef struct {
    OpusDecoder *decoder;        /*!< Opus decoder instance */
    opus_dec_config_t config;    /*!< Opus decoder configuration */
    int max_frame_size;          /*!< Opus decoder max frame buffer */
    int16_t *out_buf;            /*!< Opus decoder output buffer */
} opus_dec_t;

static bk_err_t _opus_dec_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _opus_dec_open \n", audio_element_get_tag(self));
    opus_dec_t *opus_dec = (opus_dec_t *)audio_element_getdata(self);

    /* set read data timeout */
    //audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);

    /* Calculate maximum frame size */
    opus_dec->max_frame_size = opus_dec->config.sample_rate * opus_dec->config.channels * 120 / 1000; /* 120ms max frame */
    
    /* Allocate output buffer */
    opus_dec->out_buf = (int16_t *)audio_malloc(opus_dec->max_frame_size * sizeof(int16_t));
    if (!opus_dec->out_buf) {
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t _opus_dec_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _opus_dec_close \n", audio_element_get_tag(self));

    return BK_OK;
}

static int _opus_dec_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _opus_dec_process \n", audio_element_get_tag(self));
    opus_dec_t *opus_dec = (opus_dec_t *)audio_element_getdata(self);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        /* Decode Opus data */
        int dec_output_size = opus_decode(opus_dec->decoder, (uint8_t *)in_buffer, r_size, opus_dec->out_buf, opus_dec->max_frame_size, 0);
        if (dec_output_size < 0) {
            BK_LOGE(TAG, "[%s] _opus_dec_process err:%d \n", audio_element_get_tag(self),dec_output_size);
            return AEL_PROCESS_FAIL;
        }
        
        w_size = audio_element_output(self, (char *)opus_dec->out_buf, dec_output_size<<1);
        BK_LOGV(TAG, "[%s] %s:r_size:%d,w_size:%d,dec_oputput_size:%d\n", audio_element_get_tag(self),__func__,r_size,w_size,dec_output_size<<1);
    }
    else
    {
        w_size = r_size;
        BK_LOGD(TAG, "[%s] %s:r_size:%d,w_size:%d\n", audio_element_get_tag(self),__func__,r_size,w_size);
    }

    return w_size;
}

static bk_err_t _opus_dec_destroy(audio_element_handle_t self)
{
    opus_dec_t *opus_dec = (opus_dec_t *)audio_element_getdata(self);

    
    if (opus_dec->out_buf) {
        audio_free(opus_dec->out_buf);
    }
    
    if (opus_dec->decoder) {
        opus_decoder_destroy(opus_dec->decoder);
    }
    
    audio_free(opus_dec);

    return BK_OK;
}

audio_element_handle_t opus_dec_init(opus_dec_cfg_t *config)
{
    audio_element_handle_t el;
    opus_dec_t *opus_dec = audio_calloc(1, sizeof(opus_dec_t));

    AUDIO_MEM_CHECK(TAG, opus_dec, return NULL);

    int error;
    opus_dec->decoder = opus_decoder_create(config->sample_rate, config->channels, &error);
    if (error != OPUS_OK || !opus_dec->decoder) {
        audio_free(opus_dec);
        return NULL;
    }

    opus_dec->config.sample_rate = config->sample_rate;
    opus_dec->config.channels = config->channels;

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _opus_dec_open;
    cfg.close = _opus_dec_close;
    cfg.process = _opus_dec_process;
    cfg.destroy = _opus_dec_destroy;
    cfg.in_type = PORT_TYPE_FB;
    cfg.out_type = PORT_TYPE_RB;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;
    cfg.tag = "opus_dec";

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _opus_decoder_init_exit);
    audio_element_setdata(el, opus_dec);

    return el;

_opus_decoder_init_exit:
    if (opus_dec->decoder) {
        opus_decoder_destroy(opus_dec->decoder);
    }
    audio_free(opus_dec);
    return NULL;
}
