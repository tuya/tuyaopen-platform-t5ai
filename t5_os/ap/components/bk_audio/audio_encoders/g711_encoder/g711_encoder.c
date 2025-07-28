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
#include <components/bk_audio/audio_encoders/g711_encoder.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/g711.h>


#define TAG  "G711_ENC"


/* dump g711_encoder stream output g711 data by uart */
//#define G711_ENC_DATA_DUMP_BY_UART

#ifdef G711_ENC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_g711_enc_uart_util = {0};
#define G711_ENC_DATA_DUMP_UART_ID            (1)
#define G711_ENC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define G711_ENC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_g711_enc_uart_util, G711_ENC_DATA_DUMP_UART_ID, G711_ENC_DATA_DUMP_UART_BAUD_RATE)
#define G711_ENC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_g711_enc_uart_util)
#define G711_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_g711_enc_uart_util, data_buf, len)

#else

#define G711_ENC_DATA_DUMP_BY_UART_OPEN()
#define G711_ENC_DATA_DUMP_BY_UART_CLOSE()
#define G711_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //G711_ENC_DATA_DUMP_BY_UART



typedef struct g711_encoder
{
    g711_encoder_mode_t     enc_mode;       /*!< 0: a-law  1: u-law */
} g711_encoder_t;



static bk_err_t _g711_encoder_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _g711_encoder_open \n", audio_element_get_tag(self));

    return BK_OK;
}

static bk_err_t _g711_encoder_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _g711_encoder_close \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _g711_encoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _g711_encoder_process \n", audio_element_get_tag(self));
    g711_encoder_t *g711_enc = (g711_encoder_t *)audio_element_getdata(self);

    //  addAON_GPIO_Reg0x3 = 2;

    int r_size = audio_element_input(self, in_buffer, in_len);
    //  BK_LOGD(TAG, "[%s] r_size: %d \n", audio_element_get_tag(self), r_size);

    int w_size = 0;
    if (r_size > 0)
    {
        unsigned char *g711_out_ptr = audio_malloc(r_size >> 1);
        AUDIO_MEM_CHECK(TAG, g711_out_ptr, return -1);

        int16_t *linear = (int16_t *)in_buffer;
        if (g711_enc->enc_mode == G711_ENC_MODE_U_LOW)
        {
            for (uint32_t i = 0; i < r_size >> 1; i++)
            {
                g711_out_ptr[i] = linear2ulaw(linear[i]);
            }
        }
        else
        {
            for (uint32_t i = 0; i < r_size >> 1; i++)
            {
                g711_out_ptr[i] = linear2alaw(linear[i]);
            }
        }
        //      BK_LOGD(TAG, "[%s] r_size>>1: %d \n", audio_element_get_tag(self), r_size>1);

        G711_ENC_DATA_DUMP_BY_UART_DATA(g711_out_ptr, r_size >> 1);

        w_size = audio_element_output(self, (char *)g711_out_ptr, r_size >> 1);
        audio_free(g711_out_ptr);
    }
    else
    {
        w_size = r_size;
    }
    //  addAON_GPIO_Reg0x3 = 0;
    return w_size;
}

static bk_err_t _g711_encoder_destroy(audio_element_handle_t self)
{
    g711_encoder_t *g711_enc = (g711_encoder_t *)audio_element_getdata(self);
    audio_free(g711_enc);

    G711_ENC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}


audio_element_handle_t g711_encoder_init(g711_encoder_cfg_t *config)
{
    audio_element_handle_t el;
    g711_encoder_t *g711_enc = audio_calloc(1, sizeof(g711_encoder_t));

    AUDIO_MEM_CHECK(TAG, g711_enc, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _g711_encoder_open;
    cfg.close = _g711_encoder_close;
    cfg.seek = NULL;
    cfg.process = _g711_encoder_process;
    cfg.destroy = _g711_encoder_destroy;
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

    cfg.tag = "g711_encoder";
    g711_enc->enc_mode = config->enc_mode;

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _g711_encoder_init_exit);
    audio_element_setdata(el, g711_enc);

    G711_ENC_DATA_DUMP_BY_UART_OPEN();

    return el;
_g711_encoder_init_exit:
    audio_free(g711_enc);
    return NULL;
}

