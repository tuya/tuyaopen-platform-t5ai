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


#ifndef _ARRAY_STREAM_H_
#define _ARRAY_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ARRAY Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    audio_stream_type_t     type;           /*!< Stream type */
    uint32_t                array_size;     /*!< Array total size */
    uint32_t                array_offset;   /*!< Array offset */
    int                     buf_sz;         /*!< Audio Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} array_stream_cfg_t;


#define ARRAY_STREAM_BUF_SIZE            (512)
#define ARRAY_STREAM_TASK_STACK          (1024)
#define ARRAY_STREAM_TASK_CORE           (0)
#define ARRAY_STREAM_TASK_PRIO           (4)
#define ARRAY_STREAM_OUT_BLOCK_SIZE      (512)
#define ARRAY_STREAM_OUT_BLOCK_NUM       (2)


#define DEFAULT_ARRAY_STREAM_CONFIG() {                 \
        .type = AUDIO_STREAM_NONE,                      \
        .buf_sz = ARRAY_STREAM_BUF_SIZE,                \
        .array_size = ARRAY_STREAM_BUF_SIZE,            \
        .array_offset = 0,                              \
        .out_block_size = ARRAY_STREAM_OUT_BLOCK_SIZE,  \
        .out_block_num = ARRAY_STREAM_OUT_BLOCK_NUM,    \
        .task_stack = ARRAY_STREAM_TASK_STACK,          \
        .task_core = ARRAY_STREAM_TASK_CORE,            \
        .task_prio = ARRAY_STREAM_TASK_PRIO,            \
    }

/**
 * @brief      Create a handle to an Audio Element to stream data from array to another Element
 *             or get data from other elements written to array, depending on the configuration
 *             the stream type, either AUDIO_STREAM_READER or AUDIO_STREAM_WRITER.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t array_stream_init(array_stream_cfg_t *config);


/**
 * @brief      Set the array information for the array stream. This interface must be used to set
 *             the array and its length after the component is initialized and before it runs.
 *
 * @param[in]  array_stream  The audio element handle of the array stream.
 * @param[in]  array_buf     Pointer to the array buffer.
 * @param[in]  array_size    Size of the array.
 *
 * @return     Operation result
 *             - BK_OK: Success
 *             - Other values: Failure
 */
bk_err_t array_stream_set_data(audio_element_handle_t array_stream, uint8_t *array_buf, int array_size);

#ifdef __cplusplus
}
#endif

#endif
