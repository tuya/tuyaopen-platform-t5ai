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


#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "audio_type_def.h"

#define ELEMENT_SUB_TYPE_OFFSET 16

typedef enum {
    AUDIO_ELEMENT_TYPE_UNKNOW = 0x01<<ELEMENT_SUB_TYPE_OFFSET,
    AUDIO_ELEMENT_TYPE_ELEMENT= 0x01<<(ELEMENT_SUB_TYPE_OFFSET+1),
    AUDIO_ELEMENT_TYPE_PLAYER = 0x01<<(ELEMENT_SUB_TYPE_OFFSET+2),
    AUDIO_ELEMENT_TYPE_SERVICE = 0x01<<(ELEMENT_SUB_TYPE_OFFSET+3),
    AUDIO_ELEMENT_TYPE_PERIPH = 0x01<<(ELEMENT_SUB_TYPE_OFFSET+4),
} audio_element_type_t;

typedef enum {
    AUDIO_STREAM_NONE = 0,
    AUDIO_STREAM_READER,
    AUDIO_STREAM_WRITER
} audio_stream_type_t;

typedef enum {
    AUDIO_CODEC_TYPE_NONE = 0,
    AUDIO_CODEC_TYPE_DECODER,
    AUDIO_CODEC_TYPE_ENCODER
} audio_codec_type_t;

#define mem_assert(x)

#ifdef __cplusplus
}
#endif

#endif
