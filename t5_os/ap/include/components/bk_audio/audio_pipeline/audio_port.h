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

#ifndef _AUDIO_PORT_H__
#define _AUDIO_PORT_H__

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bsd_queue.h"
#include <stdint.h>
#include "FreeRTOS.h"


#ifdef __cplusplus
extern "C" {
#endif

#define PORT_OK                 (BK_OK)
#define PORT_FAIL               (BK_FAIL)
#define PORT_DONE               (-2)
#define PORT_ABORT              (-3)
#define PORT_TIMEOUT            (-4)
#define PORT_SIZE_OUT_RANGE     (-5)

typedef enum
{
    PORT_TYPE_RB = 1,   /* I/O through ringbuffer */
    PORT_TYPE_FB,       /* I/O through framebuffer */
    PORT_TYPE_CB,       /* I/O through callback */
} port_type_t;

typedef struct audio_port *audio_port_handle_t;

typedef bk_err_t (*port_io_func)(audio_port_handle_t self);
typedef int (*port_stream_func)(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context);

/**
 * @brief Audio Port configurations.
 *        Each Element at startup will be a self-running task.
 *        These tasks will execute the callback open -> [loop: read -> write] -> close.
 *        These callback functions are provided by the user corresponding to this configuration.
 *
 */
typedef struct
{
    port_io_func            open;               /*!< Open callback function */
    port_io_func            close;              /*!< Close callback function */
    port_io_func            destroy;            /*!< Destroy callback function */
    port_io_func            abort;              /*!< Abort callback function */
    port_io_func            reset;              /*!< Reset callback function */
    port_stream_func        read;               /*!< Read callback function */
    port_stream_func        write;              /*!< Write callback function */
    port_io_func            write_done;         /*!< write done callback function */
    port_io_func            get_size;           /*!< Get ringbuffer size or frame buffer node number function */
    port_io_func            get_filled_size;    /*!< Get ringbuffer filled size or ready frame buffer node number function */

    char *                  tag;        /*!< Port tag */

    //int                 multi_in_rb_num;  /*!< The number of multiple input ringbuffer */
    //int                 multi_out_rb_num; /*!< The number of multiple output ringbuffer */
} audio_port_cfg_t;

#if 0
#define DEFAULT_AUDIO_PORT_CONFIG() {                \
    .buffer_len         = DEFAULT_ELEMENT_BUFFER_LENGTH,\
    .task_stack         = DEFAULT_ELEMENT_STACK_SIZE,   \
    .task_prio          = DEFAULT_ELEMENT_TASK_PRIO,    \
    .task_core          = DEFAULT_ELEMENT_TASK_CORE,    \
    .multi_in_rb_num    = 0,                            \
    .multi_out_rb_num   = 0,                            \
}
#endif

audio_port_handle_t audio_port_init(audio_port_cfg_t *config);

bk_err_t audio_port_deinit(audio_port_handle_t port);
bk_err_t audio_port_set_tag(audio_port_handle_t port, const char *tag);
char *audio_port_get_tag(audio_port_handle_t port);
bk_err_t audio_port_open(audio_port_handle_t port);
bk_err_t audio_port_close(audio_port_handle_t port);
bk_err_t audio_port_abort(audio_port_handle_t port);
bk_err_t audio_port_reset(audio_port_handle_t port);
bk_err_t audio_port_read(audio_port_handle_t port, char *buffer, int len, TickType_t ticks_to_wait);
bk_err_t audio_port_write(audio_port_handle_t port, char *buffer, int len, TickType_t ticks_to_wait);
bk_err_t audio_port_write_done(audio_port_handle_t port);
bk_err_t audio_port_get_size(audio_port_handle_t port);
bk_err_t audio_port_get_filled_size(audio_port_handle_t port);
bk_err_t audio_port_set_data(audio_port_handle_t port, void *data);
void *audio_port_get_data(audio_port_handle_t port);
bk_err_t audio_port_set_type(audio_port_handle_t port, port_type_t type);
bk_err_t audio_port_get_type(audio_port_handle_t port);

#ifdef __cplusplus
}
#endif

#endif

