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

#include <components/bk_audio/audio_encoders/opus_enc.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <string.h>
#include <modules/opus.h>

#define OPUS_ENC_VBR_SCALE 2

#define TAG "OPUS_ENC"

/*
 * Opus Encoder element
 *
 * General considerations:
 *
 * Opus encoder element is used in a pipeline in order to encode incoming PCM data to Opus format.
 *
 * The element has one input and one output. It takes PCM data from the input and produces
 * Opus encoded data on the output.
 *
 * The element is implemented as a task that reads data from the input, encodes it and writes
 * the result to the output.
 *
 * The element supports different encoding modes, sample rates, number of channels and bitrates.
 *
 * The element uses the libopus library for encoding.
 */

typedef struct opus_enc {
    audio_element_info_t info;
    opus_enc_cfg_t   opus_encoder_cfg;
    OpusEncoder         *encoder;
    int                  frame_samples_per_channel;
    unsigned char       *encoded_data;
    int                  max_data_bytes;
} opus_enc_t;

static bk_err_t _opus_enc_destroy(audio_element_handle_t self)
{
    opus_enc_t *opus_enc = (opus_enc_t *)audio_element_getdata(self);
    
    if (opus_enc->encoder) {
        opus_encoder_destroy(opus_enc->encoder);
        opus_enc->encoder = NULL;
    }
    
    if (opus_enc->encoded_data) {
        audio_free(opus_enc->encoded_data);
        opus_enc->encoded_data = NULL;
    }
    
    audio_free(opus_enc);
    
    return BK_OK;
}

static bk_err_t _opus_enc_open(audio_element_handle_t self)
{
    opus_enc_t *opus_enc = (opus_enc_t *)audio_element_getdata(self);
    opus_enc_cfg_t *opus_encoder_cfg = &opus_enc->opus_encoder_cfg;
    int error = OPUS_OK;
    
    int application;
    switch (opus_encoder_cfg->enc_mode) {
        case OPUS_ENC_MODE_VOIP:
            application = OPUS_APPLICATION_VOIP;
            break;
        case OPUS_ENC_MODE_AUDIO:
            application = OPUS_APPLICATION_AUDIO;
            break;
        case OPUS_ENC_MODE_RESTRICTED_LOWDELAY:
            application = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
            break;
        default:
            application = OPUS_APPLICATION_AUDIO;
            break;
    }
    
    opus_enc->encoder = opus_encoder_create(opus_encoder_cfg->sample_rate, 
                                               opus_encoder_cfg->channels, 
                                               application, 
                                               &error);
    
    if (error != OPUS_OK || opus_enc->encoder == NULL) {
        BK_LOGE(TAG, "opus_encoder_create failed: %d\n", error);
        return BK_FAIL;
    }
    
    // Set bitrate
    error = opus_encoder_ctl(opus_enc->encoder, OPUS_SET_BITRATE(opus_encoder_cfg->bitrate));
    if (error != OPUS_OK) {
        BK_LOGE(TAG, "opus_encoder_ctl OPUS_SET_BITRATE failed: %d\n", error);
        return BK_FAIL;
    }
    
    // Set default encoder parameters
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_COMPLEXITY(1));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_MEDIUMBAND));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_PREDICTION_DISABLED(1));
    opus_encoder_ctl(opus_enc->encoder, OPUS_SET_DTX(0));
    
    // Set frame size (samples per channel) from config
    opus_enc->frame_samples_per_channel = opus_encoder_cfg->frame_samples_per_channel;
    
    // Calculate max_data_bytes for encoded data buffer
    audio_element_info_t *info = &opus_enc->info;
    opus_enc->max_data_bytes = info->bits / 8 * opus_enc->frame_samples_per_channel * opus_enc->opus_encoder_cfg.channels;
    
    // Allocate encoded data buffer
    opus_enc->encoded_data = (unsigned char *)audio_malloc(opus_enc->max_data_bytes);
    if (opus_enc->encoded_data == NULL) {
        BK_LOGE(TAG, "audio_malloc for encoded_data failed\n");
        return BK_FAIL;
    }
    
    return BK_OK;
}

static bk_err_t _opus_enc_close(audio_element_handle_t self)
{
    opus_enc_t *opus_enc = (opus_enc_t *)audio_element_getdata(self);
    
    if (opus_enc->encoder) {
        opus_encoder_destroy(opus_enc->encoder);
        opus_enc->encoder = NULL;
    }
    
    return BK_OK;
}

static int _opus_enc_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    opus_enc_t *opus_enc = (opus_enc_t *)audio_element_getdata(self);
    int frame_size = opus_enc->frame_samples_per_channel;
    int max_data_bytes = opus_enc->max_data_bytes;
    unsigned char *encoded_data = opus_enc->encoded_data;
    static uint32_t opus_enc_frame_cnt = 0;
    
    // Calculate required bytes for one frame
    int required_bytes = frame_size * opus_enc->opus_encoder_cfg.channels * sizeof(opus_int16);
    
    // Check if encoded_data buffer is allocated
    if (encoded_data == NULL) {
        BK_LOGE(TAG, "encoded_data buffer is not allocated\n");
        return AEL_IO_FAIL;
    }
    
    // Read input data for one frame using audio_element_input
    int r_size = audio_element_input(self, in_buffer, required_bytes);
    if (r_size < required_bytes) {
        // Not enough data for a complete frame
        if (r_size > 0) {
            // Partial frame data, but we need a complete frame for Opus encoding
            BK_LOGD(TAG, "Insufficient input data for a complete frame: %d < %d\n", r_size, required_bytes);
        }
        return r_size; // Return the actual number of bytes read, even if it's less than required
    }
    
    opus_int16 *pcm_data = (opus_int16 *)in_buffer;
    
    // Encode the frame
    int encoded_bytes = opus_encode(opus_enc->encoder, pcm_data, frame_size, encoded_data, max_data_bytes);
    
    if (encoded_bytes < 0) {
        BK_LOGE(TAG, "opus_encode failed: %d\n", encoded_bytes);
        return AEL_IO_FAIL;
    }
    
    // Write encoded data to output
    int written = audio_element_output(self, (char *)encoded_data, encoded_bytes);
    
    if (written != encoded_bytes) {
        BK_LOGE(TAG, "audio_element_write failed: %d != %d\n", written, encoded_bytes);
        return AEL_IO_FAIL;
    }

    BK_LOGV(TAG, "frame%d write size:%d\n", opus_enc_frame_cnt++,written);
    
    // Return the number of bytes write to output
    return written;
}

audio_element_handle_t opus_enc_init(opus_enc_cfg_t *config)
{
    if (config == NULL) {
        BK_LOGE(TAG, "opus_enc_init config is NULL\n");
        return NULL;
    }
    
    // Calculate minimum output block size
    // frame_samples_per_channel is the number of samples per channel in a frame
    // Convert to duration in seconds: duration = frame_samples_per_channel / sample_rate
    // Then calculate min_out_block_size = bitrate * duration / 8 * VBR_SCALE
    int min_out_block_size = config->bitrate * config->frame_samples_per_channel / config->sample_rate / 8 * OPUS_ENC_VBR_SCALE;
    
    // Calculate minimum buffer size
    // Buffer needs to hold at least one frame of PCM data
    // frame_size * channels * sizeof(opus_int16)
    int min_buffer_size = config->frame_samples_per_channel * config->channels * sizeof(opus_int16);
    
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_element_handle_t el;
    opus_enc_t *opus_enc;
    
    if ((opus_enc = audio_malloc(sizeof(opus_enc_t))) == NULL) {
        BK_LOGE(TAG, "audio_malloc failed");
        return NULL;
    }
    
    memset(opus_enc, 0, sizeof(opus_enc_t));
    
    // Initialize the base element
    cfg.task_stack = config->task_stack;
    cfg.task_core = config->task_core;
    cfg.task_prio = config->task_prio;
    cfg.out_block_size = config->out_block_size < min_out_block_size ? min_out_block_size : config->out_block_size;
    if (config->out_block_size < min_out_block_size) {
        BK_LOGW(TAG, "out_block_size %d is less than minimum %d, using minimum value\n", config->out_block_size, min_out_block_size);
    }
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz < min_buffer_size ? min_buffer_size : config->buf_sz;
    if (config->buf_sz < min_buffer_size) {
        BK_LOGW(TAG, "buf_sz %d is less than minimum %d, using minimum value\n", config->buf_sz, min_buffer_size);
    }
    cfg.tag = "opus_enc";
    cfg.destroy = _opus_enc_destroy;
    cfg.open = _opus_enc_open;
    cfg.close = _opus_enc_close;
    cfg.process = _opus_enc_process;
    
    // Configure input and output types
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_FB;
    cfg.write = NULL;
    
    el = audio_element_init(&cfg);
    if (el == NULL) {
        BK_LOGE(TAG, "audio_element_init failed\n");
        audio_free(opus_enc);
        return NULL;
    }
    
    // Set the element data
    memcpy(&opus_enc->opus_encoder_cfg, config, sizeof(opus_enc_cfg_t));
    audio_element_setdata(el, opus_enc);
    
    // Set the element info
    audio_element_info_t *info = &opus_enc->info;
    info->sample_rates = config->sample_rate;
    info->channels = config->channels;
    info->bits = 16; // Opus input is always 16-bit
    info->codec_fmt = BK_CODEC_TYPE_OPUS;
    
    return el;
}