// Copyright 2022-2023 Beken
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

#ifndef _AUDIO_TYPE_DEF_H_
#define _AUDIO_TYPE_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define BK_AUDIO_SAMPLERATE_8K (8000)
#define BK_AUDIO_SAMPLERATE_11K (11025)
#define BK_AUDIO_SAMPLERATE_16K (16000)
#define BK_AUDIO_SAMPLERATE_22K (22050)
#define BK_AUDIO_SAMPLERATE_32K (32000)
#define BK_AUDIO_SAMPLERATE_44K (44100)
#define BK_AUDIO_SAMPLERATE_48K (48000)


#define BK_AUDIO_CHANNEL_MONO (1)
#define BK_AUDIO_CHANNEL_DUAL (2)

typedef enum
{
    BK_CODEC_TYPE_UNKNOW        = 0,
    BK_CODEC_TYPE_RAW           = 1,
    BK_CODEC_TYPE_WAV           = 2,
    BK_CODEC_TYPE_MP3           = 3,
    BK_CODEC_TYPE_AAC           = 4,
    BK_CODEC_TYPE_OPUS          = 5,
    BK_CODEC_TYPE_M4A           = 6,
    BK_CODEC_TYPE_MP4           = 7,
    BK_CODEC_TYPE_FLAC          = 8,
    BK_CODEC_TYPE_OGG           = 9,
    BK_CODEC_TYPE_TSAAC         = 10,
    BK_CODEC_TYPE_AMRNB         = 11,
    BK_CODEC_TYPE_AMRWB         = 12,
    BK_CODEC_TYPE_PCM           = 13,
    BK_AUDIO_TYPE_M3U8          = 14,
    BK_AUDIO_TYPE_PLS           = 15,
    BK_CODEC_TYPE_UNSUPPORT     = 16,
} bk_codec_type_t;

typedef enum
{
    BK_DECODER_WORK_MODE_MANUAL = 0,
    BK_DECODER_WORK_MODE_AUTO   = 1,
} bk_decoder_work_mode_t;

/**
 * @brief the enum value from `BK_CODEC_ERR_OK` to `BK_CODEC_ERR_TIMEOUT` rely on `audio_element_err_t` which in `audio_element.h`.
 */
typedef enum
{
    BK_CODEC_ERR_CONTINUE       = 1,
    BK_CODEC_ERR_OK             = 0,
    BK_CODEC_ERR_FAIL           = -1,
    BK_CODEC_ERR_DONE           = -2,
    BK_CODEC_ERR_ABORT          = -3,
    BK_CODEC_ERR_TIMEOUT        = -4,
    BK_CODEC_ERR_UNSYNC         = -5,
    BK_CODEC_ERR_UNSUPPORT      = -6,
    BK_CODEC_ERR_PARSE          = -7,
    BK_CODEC_ERR_INFO           = -8,
    BK_CODEC_ERR_INPUT          = -9,
    BK_CODEC_ERR_NO_MEM         = -10,
} bk_codec_err_t;

#ifdef __cplusplus
}
#endif

#endif
