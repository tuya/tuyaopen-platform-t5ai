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


#ifndef FRAME_ARRAY_STREAM_H_
#define FRAME_ARRAY_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Frame ARRAY Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    audio_stream_type_t type;                   /*!< Stream type */
    uint32_t            data_size;              /**< Frame Array data total size */
    uint32_t            data_len_array_size;    /**< Frame Array data length total size */
    uint32_t            data_offset;            /**< Frame Array data offset */
    uint32_t            data_len_array_offset;  /**< Frame Array data length array offset */
    int                 buf_sz;                 /*!< Audio Element Buffer size */
    int                 out_block_size;         /*!< Size of output block */
    int                 out_block_num;          /*!< Number of output block */
    int                 task_stack;             /*!< Task stack size */
    int                 task_core;              /*!< Task running in core (0 or 1) */
    int                 task_prio;              /*!< Task priority (based on freeRTOS priority) */
} frame_array_stream_cfg_t;


#define FRAME_ARRAY_STREAM_BUF_SIZE            (512)
#define FRAME_ARRAY_STREAM_DATA_LEN_ARRAY_SIZE (128)
#define FRAME_ARRAY_STREAM_TASK_STACK          (1024)
#define FRAME_ARRAY_STREAM_TASK_CORE           (0)
#define FRAME_ARRAY_STREAM_TASK_PRIO           (4)
#define FRAME_ARRAY_STREAM_OUT_BLOCK_SIZE      (512)
#define FRAME_ARRAY_STREAM_OUT_BLOCK_NUM       (2)


#define DEFAULT_FRAME_ARRAY_STREAM_CONFIG() {                 \
        .type = AUDIO_STREAM_NONE,                            \
        .buf_sz = FRAME_ARRAY_STREAM_BUF_SIZE,                \
        .data_size = FRAME_ARRAY_STREAM_BUF_SIZE,             \
        .data_offset = 0,                                     \
        .data_len_array_size = FRAME_ARRAY_STREAM_BUF_SIZE,   \
        .data_len_array_offset = 0,                           \
        .out_block_size = FRAME_ARRAY_STREAM_OUT_BLOCK_SIZE,  \
        .out_block_num = FRAME_ARRAY_STREAM_OUT_BLOCK_NUM,    \
        .task_stack = FRAME_ARRAY_STREAM_TASK_STACK,          \
        .task_core = FRAME_ARRAY_STREAM_TASK_CORE,            \
        .task_prio = FRAME_ARRAY_STREAM_TASK_PRIO,            \
    }

/**
 * @brief      Create a handle to an Audio Element to stream data from frame array to another Element
 *             or get data from other elements written to array, depending on the configuration
 *             the stream type, either AUDIO_STREAM_READER or AUDIO_STREAM_WRITER.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t frame_array_stream_init(frame_array_stream_cfg_t *config);


/**
 * @brief      Set the array information for the array stream. This interface must be used to set
 *             the array and its length after the component is initialized and before it runs.
 *
 * @param[in]  frame_array_stream  The audio element handle of the array stream.
 * @param[in]  data_buf     Pointer to the data buffer.
 * @param[in]  data_size    Size of the data buffer.
 * @param[in]  data_len_buf Pointer to the data len buffer.
 * @param[in]  data_len_array_size    Size of the data len array buffer.
 *
 * @return     Operation result
 *             - BK_OK: Success
 *             - Other values: Failure
 */
bk_err_t frame_array_stream_set_data(audio_element_handle_t frame_array_stream, uint8_t *data_buf, int data_size, uint16_t *data_len_buf, int data_len_array_size);

#ifdef __cplusplus
}
#endif

#endif
