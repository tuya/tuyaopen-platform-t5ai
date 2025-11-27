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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/uart.h>
#include "gpio_driver.h"


#define TAG  "UART_STR"

typedef struct uart_stream
{
    uint8_t                 uart_id;        /**< Uart id */
    audio_stream_type_t     type;           /**< Type of stream */
    int                     out_block_size; /**< Size of output block */
    int                     out_block_num;  /**< Number of output block */
    int                     buffer_len;     /**< Size of read every time */
    uart_config_t           config;         /**< uart config */
    bool                    is_open;        /**< uart enable, true: enable, false: disable */
} uart_stream_t;


static int _uart_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _uart_open \n", audio_element_get_tag(self));

    uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(self);

    if (uart_stream->is_open)
    {
        return BK_OK;
    }

    if (BK_OK != bk_uart_init(uart_stream->uart_id, &uart_stream->config))
    {
        BK_LOGE(TAG, "[%s] %s, %d, init uart fail \n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    /* set read data timeout */
    //audio_element_set_input_timeout(self, 10000);   //15 / portTICK_RATE_MS

    uart_stream->is_open = true;

    return BK_OK;
}

static int _uart_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] _uart_read, len: %d \n", audio_element_get_tag(el), len);

    uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(el);

    if (len > 0)
    {
        return bk_uart_read_bytes(uart_stream->uart_id, buffer, len, (uint32_t)BEKEN_WAIT_FOREVER);
    }
    else
    {
        return len;
    }
}

static int _uart_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;

    BK_LOGV(TAG, "[%s] _uart_write, len: %d \n", audio_element_get_tag(el), len);

    uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(el);

    if (len)
    {
        if (BK_OK != bk_uart_write_bytes(uart_stream->uart_id, buffer, len))
        {
            BK_LOGE(TAG, "[%s] %s, %d, uart write fail\n", audio_element_get_tag(el), __func__, __LINE__);
            return 0;
        }
    }

    return len;
}

static int _uart_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _uart_process, in_len: %d \n", audio_element_get_tag(self), in_len);

    //  uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(self);

    /* read input data */
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;
    if (r_size == AEL_IO_TIMEOUT)
    {
        r_size = 0;
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else if (r_size > 0)
    {
        //      audio_element_multi_output(self, in_buffer, r_size, 0);
        w_size = audio_element_output(self, in_buffer, r_size);
        //更新处理数据的指针
        //      audio_element_update_byte_pos(self, w_size);
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}

static int _uart_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _uart_close \n", audio_element_get_tag(self));

    uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(self);

    if (BK_OK != bk_uart_deinit(uart_stream->uart_id))
    {
        BK_LOGE(TAG, "[%s] %s, %d, uart deinit fail\n", audio_element_get_tag(self), __func__, __LINE__);
        return BK_FAIL;
    }

    uart_stream->is_open = false;

    return BK_OK;
}

static int _uart_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _uart_destroy \n", audio_element_get_tag(self));

    uart_stream_t *uart_stream = (uart_stream_t *)audio_element_getdata(self);

    if (uart_stream)
    {
        audio_free(uart_stream);
        uart_stream = NULL;
    }

    return BK_OK;
}

audio_element_handle_t uart_stream_init(uart_stream_cfg_t *config)
{
    audio_element_handle_t el;
    uart_stream_t *uart_stream = audio_calloc(1, sizeof(uart_stream_t));
    AUDIO_MEM_CHECK(TAG, uart_stream, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _uart_open;
    cfg.close = _uart_close;
    cfg.process = _uart_process;
    cfg.destroy = _uart_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->buffer_len;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    if (config->type == AUDIO_STREAM_READER)
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _uart_read;
        cfg.out_type = PORT_TYPE_RB;
        cfg.write = NULL;
    }
    else if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.in_type = PORT_TYPE_RB;
        cfg.read = NULL;
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _uart_write;
    }
    else
    {
        BK_LOGE(TAG, "uart type: %d, is not support, please check\n", config->type);
        goto _uart_init_exit;
    }

    cfg.tag = "uart_stream";
    BK_LOGD(TAG, "buffer_len: %d, out_block_size: %d, out_block_num: %d\n", cfg.buffer_len, cfg.out_block_size, cfg.out_block_num);

    /* config uart */
    uart_stream->uart_id = config->uart_id;
    uart_stream->type = config->type;
    uart_stream->out_block_size = config->out_block_size;
    uart_stream->out_block_num = config->out_block_num;
    uart_stream->buffer_len = config->buffer_len;
    uart_stream->config.baud_rate = config->baud_rate;
    uart_stream->config.data_bits = UART_DATA_8_BITS;
    uart_stream->config.parity = UART_PARITY_NONE;
    uart_stream->config.stop_bits = UART_STOP_BITS_1;
    uart_stream->config.flow_ctrl = UART_FLOWCTRL_DISABLE;
    uart_stream->config.src_clk = UART_SCLK_XTAL_26M;
    BK_LOGD(TAG, "uart_id: %d, baud_rate: %d \n", uart_stream->uart_id, uart_stream->config.baud_rate);

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _uart_init_exit);
    audio_element_setdata(el, uart_stream);

    audio_element_info_t info = {0};
    info.sample_rates = 8000;   //not need set
    info.channels = 2;
    info.bits = 16;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    return el;
_uart_init_exit:

    audio_free(uart_stream);
    uart_stream = NULL;
    return NULL;
}

