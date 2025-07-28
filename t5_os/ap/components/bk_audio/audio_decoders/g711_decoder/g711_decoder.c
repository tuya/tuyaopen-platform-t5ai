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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_decoders/g711_decoder.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/g711.h>
#include <os/os.h>


#define TAG  "G711_DEC"

//#define G711_DECODER_DEBUG   //GPIO debug

#ifdef G711_DECODER_DEBUG

#define G711_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define G711_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define G711_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define G711_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define G711_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define G711_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define G711_DECODER_PROCESS_START()
#define G711_DECODER_PROCESS_END()

#define G711_DECODER_INPUT_START()
#define G711_DECODER_INPUT_END()

#define G711_DECODER_OUTPUT_START()
#define G711_DECODER_OUTPUT_END()

#endif

/* dump g711_decoder stream output pcm data by uart */
//#define G711_DEC_DATA_DUMP_BY_UART

#ifdef G711_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_g711_dec_uart_util = {0};
#define G711_DEC_DATA_DUMP_UART_ID            (1)
#define G711_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define G711_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_g711_dec_uart_util, G711_DEC_DATA_DUMP_UART_ID, G711_DEC_DATA_DUMP_UART_BAUD_RATE)
#define G711_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_g711_dec_uart_util)
#define G711_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_g711_dec_uart_util, data_buf, len)

#else

#define G711_DEC_DATA_DUMP_BY_UART_OPEN()
#define G711_DEC_DATA_DUMP_BY_UART_CLOSE()
#define G711_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //G711_ENC_DATA_DUMP_BY_UART


typedef struct g711_decoder
{
    g711_decoder_mode_t     dec_mode;       /*!< 0: a-law  1: u-law */
} g711_decoder_t;


static bk_err_t _g711_decoder_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _g711_decoder_open \n", audio_element_get_tag(self));
    //  BK_LOGD(TAG, "[%s] 0xD5 ->G711-> 0x%04x \n", audio_element_get_tag(self), alaw2linear(0xD5));

    /* set read data timeout */
    //audio_element_set_input_timeout(self, 1000 / portTICK_RATE_MS);   // 2000, 15 / portTICK_RATE_MS

    return BK_OK;
}

static bk_err_t _g711_decoder_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _g711_decoder_close \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _g711_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _g711_decoder_process \n", audio_element_get_tag(self));
    g711_decoder_t *g711_dec = (g711_decoder_t *)audio_element_getdata(self);

    G711_DECODER_PROCESS_START();

    G711_DECODER_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    G711_DECODER_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        int16_t *g711_out_ptr = audio_malloc(r_size << 1);
        AUDIO_MEM_CHECK(TAG, g711_out_ptr, return -1);

        uint8_t *law = (uint8_t *)in_buffer;
        if (g711_dec->dec_mode == G711_DEC_MODE_U_LOW)
        {
            for (uint32_t i = 0; i < r_size; i++)
            {
                g711_out_ptr[i] = ulaw2linear(law[i]);
            }
        }
        else
        {
            for (uint32_t i = 0; i < r_size; i++)
            {
                g711_out_ptr[i] = alaw2linear(law[i]);
            }
        }

        G711_DEC_DATA_DUMP_BY_UART_DATA(g711_out_ptr, r_size << 1);

        G711_DECODER_OUTPUT_START();
        w_size = audio_element_output(self, (char *)g711_out_ptr, r_size << 1);
        G711_DECODER_OUTPUT_END();

        audio_free(g711_out_ptr);
    }
    else
    {
        w_size = r_size;
    }

    G711_DECODER_PROCESS_END();

    return w_size;
}

static bk_err_t _g711_decoder_destroy(audio_element_handle_t self)
{
    g711_decoder_t *g711_dec = (g711_decoder_t *)audio_element_getdata(self);
    audio_free(g711_dec);

    G711_DEC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t g711_decoder_init(g711_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    g711_decoder_t *g711_dec = audio_calloc(1, sizeof(g711_decoder_t));

    AUDIO_MEM_CHECK(TAG, g711_dec, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _g711_decoder_open;
    cfg.close = _g711_decoder_close;
    cfg.seek = NULL;
    cfg.process = _g711_decoder_process;
    cfg.destroy = _g711_decoder_destroy;
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

    cfg.tag = "g711_decoder";
    g711_dec->dec_mode = config->dec_mode;

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _g711_decoder_init_exit);
    audio_element_setdata(el, g711_dec);

    G711_DEC_DATA_DUMP_BY_UART_OPEN();

    return el;
_g711_decoder_init_exit:
    audio_free(g711_dec);
    return NULL;
}

