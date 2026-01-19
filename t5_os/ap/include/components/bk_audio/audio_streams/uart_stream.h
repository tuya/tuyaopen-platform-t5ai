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


#ifndef _UART_STREAM_H_
#define _UART_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <driver/uart.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Uart Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    uint8_t                 uart_id;        /*!< Uart id */
    uint32_t                baud_rate;      /*!< uart baud rate */
    audio_stream_type_t     type;           /*!< Type of stream */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     buffer_len;     /*!< Size of read every time */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} uart_stream_cfg_t;


#define UART_STREAM_UART_ID             (1)
#define UART_STREAM_BAUD_RATE           (2000000)
#define UART_STREAM_BLOCK_SIZE          (2 * 1024)
#define UART_STREAM_BLOCK_NUM           (1)
#define UART_STREAM_BUFFER_LEN          (320)

#define UART_STREAM_TASK_STACK          (1024)
#define UART_STREAM_TASK_CORE           (1)
#define UART_STREAM_TASK_PRIO           (BEKEN_DEFAULT_WORKER_PRIORITY - 1)

#define UART_STREAM_CFG_DEFAULT() DEFAULT_UART_STREAM_CONFIG()

#define DEFAULT_UART_STREAM_CONFIG() {                  \
        .uart_id = UART_STREAM_UART_ID,                 \
        .baud_rate = UART_STREAM_BAUD_RATE,             \
        .type = AUDIO_STREAM_NONE,                      \
        .out_block_size = UART_STREAM_BLOCK_SIZE,       \
        .out_block_num = UART_STREAM_BLOCK_NUM,         \
        .buffer_len = UART_STREAM_BUFFER_LEN,           \
        .task_stack = UART_STREAM_TASK_STACK,           \
        .task_core = UART_STREAM_TASK_CORE,             \
        .task_prio = UART_STREAM_TASK_PRIO,             \
    }

/**
 * @brief      Create a handle to an Audio Element to stream data to another Element.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t uart_stream_init(uart_stream_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
