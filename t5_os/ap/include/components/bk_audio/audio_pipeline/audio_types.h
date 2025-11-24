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

#ifndef _AUDIO_TYPES_H_
#define _AUDIO_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    AUDIO_STREAM_NONE = 0,
    AUDIO_STREAM_READER,
    AUDIO_STREAM_WRITER
} audio_stream_type_t;

typedef enum
{
    BK_CODEC_TYPE_UNKNOW        = 0,
    BK_CODEC_TYPE_PCM           = 1,
    BK_CODEC_TYPE_WAV           = 2,
    BK_CODEC_TYPE_MP3           = 3,
    BK_CODEC_TYPE_AAC           = 4,
    BK_CODEC_TYPE_OPUS          = 5,
    BK_CODEC_TYPE_FLAC          = 6,
    BK_CODEC_TYPE_ADPCM         = 7,
    BK_CODEC_TYPE_G722          = 8,
    BK_CODEC_TYPE_UNSUPPORT     = 9,
} bk_codec_type_t;

/* audio interface mic type */
typedef enum
{
    MIC_TYPE_INVALID = 0,
    MIC_TYPE_ONBOARD = 1,
    MIC_TYPE_UAC,
    MIC_TYPE_ONBOARD_DUAL_DMIC_MIC,
} mic_type_t;

/**************** audio interface speaker ****************/

/* audio stream type */
typedef enum
{
    AUDIO_STRM_TYPE_INVALID = 0,
    AUDIO_STRM_TYPE_ONBOARD_MIC = 1,
    AUDIO_STRM_TYPE_UAC_MIC,
    AUDIO_STRM_TYPE_ONBOARD_SPEAKER,
    AUDIO_STRM_TYPE_UAC_SPEAKER,
    AUDIO_STRM_TYPE_ARRAY,
    AUDIO_STRM_TYPE_VFS,
    AUDIO_STRM_TYPE_RAW,
    AUDIO_STRM_TYPE_UART,
    AUDIO_STRM_TYPE_MAX,
} audio_strm_type_t;

typedef enum
{
    SPK_TYPE_INVALID = 0,
    SPK_TYPE_ONBOARD = 1,
    SPK_TYPE_UAC,
} spk_type_t;

typedef enum
{
    AUDIO_ENC_TYPE_INVALID = 0,
    AUDIO_ENC_TYPE_PCM = 1,
    AUDIO_ENC_TYPE_G711A,
    AUDIO_ENC_TYPE_G711U,
    AUDIO_ENC_TYPE_AAC,
    AUDIO_ENC_TYPE_ADPCM,
    AUDIO_ENC_TYPE_LC3,
    AUDIO_ENC_TYPE_OPUS,
    AUDIO_ENC_TYPE_G722
} audio_enc_type_t;

typedef enum
{
    AUDIO_DEC_TYPE_INVALID = 0,
    AUDIO_DEC_TYPE_PCM = 1,
    AUDIO_DEC_TYPE_G711A,
    AUDIO_DEC_TYPE_G711U,
    AUDIO_DEC_TYPE_AAC,
    AUDIO_DEC_TYPE_ADPCM,
    AUDIO_DEC_TYPE_LC3,
    AUDIO_DEC_TYPE_MP3,
    AUDIO_DEC_TYPE_OPUS,
    AUDIO_DEC_TYPE_G722,
    AUDIO_DEC_TYPE_WAV,
    AUDIO_DEC_TYPE_AMR
} audio_dec_type_t;

typedef enum
{
    AUDIO_MEM_TYPE_SRAM = 1,
    AUDIO_MEM_TYPE_PSRAM,
    AUDIO_MEM_TYPE_AUDIO_HEAP,
} audio_mem_type_t;

typedef enum
{
    AUDIO_BUF_TYPE_RB = 1,
    AUDIO_BUF_TYPE_FB,
} audio_buf_type_t;

#ifdef __cplusplus
}
#endif

#endif
