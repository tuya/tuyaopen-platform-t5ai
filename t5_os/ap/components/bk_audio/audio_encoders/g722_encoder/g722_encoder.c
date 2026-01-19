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
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_encoders/g722_encoder.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/bk_g722.h>


#define TAG  "G722_ENC"

/* dump g722_encoder stream output g722 data by uart */
//#define G722_ENC_DATA_DUMP_BY_UART

#ifdef G722_ENC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_g722_enc_uart_util = {0};
#define G722_ENC_DATA_DUMP_UART_ID            (1)
#define G722_ENC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define G722_ENC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_g722_enc_uart_util, G722_ENC_DATA_DUMP_UART_ID, G722_ENC_DATA_DUMP_UART_BAUD_RATE)
#define G722_ENC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_g722_enc_uart_util)
#define G722_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_g722_enc_uart_util, data_buf, len)

#else

#define G722_ENC_DATA_DUMP_BY_UART_OPEN()
#define G722_ENC_DATA_DUMP_BY_UART_CLOSE()
#define G722_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //G722_ENC_DATA_DUMP_BY_UART


typedef struct g722_encoder
{
    g722_encode_state_t     enc_state;      /*!< G722 encoder state */
    int                     rate;           /*!< Encoding rate */
} g722_encoder_t;



static bk_err_t _g722_encoder_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _g722_encoder_open \n", audio_element_get_tag(self));
    g722_encoder_t *g722_enc = (g722_encoder_t *)audio_element_getdata(self);

    // Initialize G722 encoder
    bk_g722_encode_init(&g722_enc->enc_state, g722_enc->rate, 0);

    return BK_OK;
}

static bk_err_t _g722_encoder_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _g722_encoder_close \n", audio_element_get_tag(self));
    g722_encoder_t *g722_enc = (g722_encoder_t *)audio_element_getdata(self);

    // Release G722 encoder
    bk_g722_encode_release(&g722_enc->enc_state);

    return BK_OK;
}

static int _g722_encoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _g722_encoder_process \n", audio_element_get_tag(self));
    g722_encoder_t *g722_enc = (g722_encoder_t *)audio_element_getdata(self);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        // Calculate output size based on bit rate
        int out_size = (r_size * 8) / g722_enc->rate;
        if (g722_enc->rate == 48000)
            out_size = r_size * 2 / 3;
        else if (g722_enc->rate == 56000)
            out_size = r_size * 7 / 8;
        else // 64000
            out_size = r_size / 2;

        unsigned char *g722_out_ptr = audio_malloc(out_size);
        AUDIO_MEM_CHECK(TAG, g722_out_ptr, return -1);

        int16_t *linear = (int16_t *)in_buffer;

        // Encode using G722
        int encoded_len = bk_g722_encode(&g722_enc->enc_state, g722_out_ptr, linear, r_size / 2);

        G722_ENC_DATA_DUMP_BY_UART_DATA(g722_out_ptr, encoded_len);

        w_size = audio_element_output(self, (char *)g722_out_ptr, encoded_len);
        audio_free(g722_out_ptr);
    }
    else
    {
        w_size = r_size;
    }
    return w_size;
}

static bk_err_t _g722_encoder_destroy(audio_element_handle_t self)
{
    g722_encoder_t *g722_enc = (g722_encoder_t *)audio_element_getdata(self);
    audio_free(g722_enc);

    G722_ENC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}


audio_element_handle_t g722_encoder_init(g722_encoder_cfg_t *config)
{
    audio_element_handle_t el;
    g722_encoder_t *g722_enc = audio_calloc(1, sizeof(g722_encoder_t));

    AUDIO_MEM_CHECK(TAG, g722_enc, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _g722_encoder_open;
    cfg.close = _g722_encoder_close;
    cfg.seek = NULL;
    cfg.process = _g722_encoder_process;
    cfg.destroy = _g722_encoder_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "g722_encoder";

    // Set encoding rate
    switch (config->enc_rate)
    {
    case G722_ENC_RATE_48000:
        g722_enc->rate = 48000;
        break;
    case G722_ENC_RATE_56000:
        g722_enc->rate = 56000;
        break;
    case G722_ENC_RATE_64000:
    default:
        g722_enc->rate = 64000;
        break;
    }

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _g722_encoder_init_exit);
    audio_element_setdata(el, g722_enc);

    G722_ENC_DATA_DUMP_BY_UART_OPEN();

    return el;
_g722_encoder_init_exit:
    audio_free(g722_enc);
    return NULL;
}