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


#ifndef _RAW_STREAM_H_
#define _RAW_STREAM_H_

#include "audio_error.h"
#include "audio_element.h"
#include "audio_common.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ADC mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    audio_stream_type_t     type;               /*!< Type of stream */
    int                     out_rb_size;        /*!< Size of output ringbuffer */
} raw_stream_cfg_t;

#define RAW_STREAM_RINGBUFFER_SIZE     (8 * 1024)

#define RAW_STREAM_CFG_DEFAULT() {\
	.type = AUDIO_STREAM_NONE, \
	.out_rb_size = RAW_STREAM_RINGBUFFER_SIZE, \
}


/**
 * @brief      Initialize RAW stream
 *
 * @param      cfg   The RAW Stream configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t raw_stream_init(raw_stream_cfg_t *cfg);

/**
 * @brief      Read data from Stream
 *
 * @param      pipeline     The audio pipeline handle
 * @param      buffer       The buffer
 * @param      buf_size     Maximum number of bytes to be read.
 *
 * @return     Number of bytes actually read.
 */
int raw_stream_read(audio_element_handle_t pipeline, char *buffer, int buf_size);

/**
 * @brief      Write data to Stream
 *
 * @param      pipeline     The audio pipeline handle
 * @param      buffer       The buffer
 * @param      buf_size     Number of bytes to write
 *
 * @return     Number of bytes written
 */
int raw_stream_write(audio_element_handle_t pipeline, char *buffer, int buf_size);

#ifdef __cplusplus
}
#endif

#endif
