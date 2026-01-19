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
#include <components/bk_audio/audio_decoders/g722_decoder.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/bk_g722.h>
#include <os/os.h>


#define TAG  "G722_DEC"

//#define G722_DECODER_DEBUG   //GPIO debug

#ifdef G722_DECODER_DEBUG

#define G722_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define G722_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define G722_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define G722_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define G722_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define G722_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define G722_DECODER_PROCESS_START()
#define G722_DECODER_PROCESS_END()

#define G722_DECODER_INPUT_START()
#define G722_DECODER_INPUT_END()

#define G722_DECODER_OUTPUT_START()
#define G722_DECODER_OUTPUT_END()

#endif

/* dump g722_decoder stream output pcm data by uart */
//#define G722_DEC_DATA_DUMP_BY_UART

#ifdef G722_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_g722_dec_uart_util = {0};
#define G722_DEC_DATA_DUMP_UART_ID            (1)
#define G722_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define G722_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_g722_dec_uart_util, G722_DEC_DATA_DUMP_UART_ID, G722_DEC_DATA_DUMP_UART_BAUD_RATE)
#define G722_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_g722_dec_uart_util)
#define G722_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_g722_dec_uart_util, data_buf, len)

#else

#define G722_DEC_DATA_DUMP_BY_UART_OPEN()
#define G722_DEC_DATA_DUMP_BY_UART_CLOSE()
#define G722_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //G722_ENC_DATA_DUMP_BY_UART


typedef struct g722_decoder
{
    g722_decode_state_t     *state;
    int                     rate;
    int                     options;
} g722_decoder_t;


static bk_err_t _g722_decoder_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _g722_decoder_open \n", audio_element_get_tag(self));
    g722_decoder_t *g722_dec = (g722_decoder_t *)audio_element_getdata(self);

    g722_dec->state = audio_malloc(sizeof(g722_decode_state_t));
    AUDIO_MEM_CHECK(TAG, g722_dec->state, return BK_ERR_ADK_FAIL);

    int ret = bk_g722_decode_init(g722_dec->state, g722_dec->rate, g722_dec->options);
    if (ret != 0)
    {
        BK_LOGE(TAG, "bk_g722_decode_init failed, ret: %d\n", ret);
        audio_free(g722_dec->state);
        g722_dec->state = NULL;
        return BK_ERR_ADK_FAIL;
    }

    return BK_OK;
}

static bk_err_t _g722_decoder_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _g722_decoder_close \n", audio_element_get_tag(self));
    g722_decoder_t *g722_dec = (g722_decoder_t *)audio_element_getdata(self);

    if (g722_dec->state)
    {
        bk_g722_decode_release(g722_dec->state);
        audio_free(g722_dec->state);
        g722_dec->state = NULL;
    }

    return BK_OK;
}

static int _g722_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _g722_decoder_process \n", audio_element_get_tag(self));
    g722_decoder_t *g722_dec = (g722_decoder_t *)audio_element_getdata(self);

    G722_DECODER_PROCESS_START();

    G722_DECODER_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    G722_DECODER_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        // Calculate output size: G722 decodes to 16-bit PCM
        // For 8kHz mode, output is same length as input
        // For 16kHz mode, output is twice the length of input
        int out_size = r_size * 2;
        if (!(g722_dec->options & G722_SAMPLE_RATE_8000))
        {
            out_size *= 2;
        }

        int16_t *g722_out_ptr = audio_malloc(out_size);
        AUDIO_MEM_CHECK(TAG, g722_out_ptr, return -1);

        // Decode G722 data
        int decoded_len = bk_g722_decode(g722_dec->state, g722_out_ptr, (uint8_t *)in_buffer, r_size);
        if (decoded_len < 0)
        {
            BK_LOGE(TAG, "g722_decode failed, ret: %d\n", decoded_len);
            audio_free(g722_out_ptr);
            return -1;
        }

        G722_DEC_DATA_DUMP_BY_UART_DATA(g722_out_ptr, decoded_len * sizeof(int16_t));

        G722_DECODER_OUTPUT_START();
        w_size = audio_element_output(self, (char *)g722_out_ptr, decoded_len * sizeof(int16_t));
        G722_DECODER_OUTPUT_END();

        audio_free(g722_out_ptr);
    }
    else
    {
        w_size = r_size;
    }

    G722_DECODER_PROCESS_END();

    return w_size;
}

static bk_err_t _g722_decoder_destroy(audio_element_handle_t self)
{
    g722_decoder_t *g722_dec = (g722_decoder_t *)audio_element_getdata(self);

    if (g722_dec->state)
    {
        bk_g722_decode_release(g722_dec->state);
        audio_free(g722_dec->state);
    }

    audio_free(g722_dec);

    G722_DEC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t g722_decoder_init(g722_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    g722_decoder_t *g722_dec = audio_calloc(1, sizeof(g722_decoder_t));

    AUDIO_MEM_CHECK(TAG, g722_dec, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _g722_decoder_open;
    cfg.close = _g722_decoder_close;
    cfg.seek = NULL;
    cfg.process = _g722_decoder_process;
    cfg.destroy = _g722_decoder_destroy;
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

    cfg.tag = "g722_decoder";
    g722_dec->rate = config->rate;
    g722_dec->options = config->options;

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _g722_decoder_init_exit);
    audio_element_setdata(el, g722_dec);

    G722_DEC_DATA_DUMP_BY_UART_OPEN();

    return el;
_g722_decoder_init_exit:
    audio_free(g722_dec);
    return NULL;
}