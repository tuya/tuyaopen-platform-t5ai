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
#include <components/bk_audio/audio_decoders/wav_decoder.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/g711.h>
#include <os/os.h>


#define TAG  "WAV_DEC"

//#define WAV_DECODER_DEBUG   //GPIO debug

#ifdef WAV_DECODER_DEBUG

#define WAV_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define WAV_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define WAV_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define WAV_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define WAV_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
##define WAV_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define WAV_DECODER_PROCESS_START()
#define WAV_DECODER_PROCESS_END()

#define WAV_DECODER_INPUT_START()
#define WAV_DECODER_INPUT_END()

#define WAV_DECODER_OUTPUT_START()
#define WAV_DECODER_OUTPUT_END()

#endif

#if CONFIG_ADK_UTILS
/* dump wav_decoder stream output pcm data by uart */
//#define WAV_DEC_DATA_DUMP_BY_UART
#endif  //CONFIG_ADK_UTILS

#ifdef WAV_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_wav_dec_uart_util = {0};
#define WAV_DEC_DATA_DUMP_UART_ID            (1)
#define WAV_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define WAV_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_wav_dec_uart_util, WAV_DEC_DATA_DUMP_UART_ID, WAV_DEC_DATA_DUMP_UART_BAUD_RATE)
#define WAV_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_wav_dec_uart_util)
#define WAV_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_wav_dec_uart_util, data_buf, len)

#else

#define WAV_DEC_DATA_DUMP_BY_UART_OPEN()
#define WAV_DEC_DATA_DUMP_BY_UART_CLOSE()
#define WAV_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //WAV_ENC_DATA_DUMP_BY_UART


typedef struct wav_decoder
{
    wav_info_t              info;                   /*!< WAV info */
    bool                    head_parse_cmp;         /*!< WAV head parse complete */
    uint8_t                 wav_head[44];           /*!< WAV head */
    uint8_t                 wav_head_valid_size;    /*!< WAV head valid size */
} wav_decoder_t;


static int check_wav_head(wav_decoder_t *wav_dec, uint8_t *in_data, uint32_t len)
{
    BK_LOGD(TAG, "%s\n", __func__);

    wav_dec->head_parse_cmp = true;

    if (BK_OK != wav_check_type(in_data, len))
    {
        BK_LOGE(TAG, "%s, %d, check wav head fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != wav_head_parse((wav_header_t *)in_data, &wav_dec->info))
    {
        BK_LOGE(TAG, "%s, %d, parse wav head fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "%s, %d, sample_rate: %d, chl_num: %d, bits: %d\n", __func__, __LINE__, wav_dec->info.samplerate, wav_dec->info.channels, wav_dec->info.bits);

    return BK_OK;
}

static bk_err_t _wav_decoder_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _wav_decoder_open \n", audio_element_get_tag(self));

    /* set read data timeout */
    audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);

    return BK_OK;
}

static bk_err_t _wav_decoder_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _wav_decoder_close \n", audio_element_get_tag(self));
    wav_decoder_t *wav_dec = (wav_decoder_t *)audio_element_getdata(self);

    if (AEL_STATE_PAUSED != audio_element_get_state(self))
    {
        wav_dec->head_parse_cmp = false;
        //audio_element_set_byte_pos(self, 0);
    }

    return BK_OK;
}

static bk_err_t music_info_report(audio_element_handle_t self)
{
    wav_decoder_t *wav_dec = (wav_decoder_t *)audio_element_getdata(self);

    audio_element_info_t info = {0};
    bk_err_t ret = audio_element_getinfo(self, &info);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] audio_element_getinfo fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    /* Report music information */
    info.bits = wav_dec->info.bits;
    info.sample_rates = wav_dec->info.samplerate;
    info.channels = wav_dec->info.channels;
    ret = audio_element_setinfo(self, &info);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] audio_element_setinfo fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }
    ret = audio_element_report_info(self);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] audio_element_report_info fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    return BK_OK;
}

static int _wav_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _wav_decoder_process \n", audio_element_get_tag(self));
    wav_decoder_t *wav_dec = (wav_decoder_t *)audio_element_getdata(self);

    WAV_DECODER_PROCESS_START();

    WAV_DECODER_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    WAV_DECODER_INPUT_END();
    int out_size = 0;

    int w_size = 0;
    if (r_size > 0)
    {
        if (!wav_dec->head_parse_cmp)
        {
            if (wav_dec->wav_head_valid_size + r_size >= 44)
            {
                os_memcpy(&wav_dec->wav_head[wav_dec->wav_head_valid_size], in_buffer, 44 - wav_dec->wav_head_valid_size);
                wav_dec->wav_head_valid_size = 44;
                out_size = r_size - (44 - wav_dec->wav_head_valid_size);
            }
            else
            {
                os_memcpy(&wav_dec->wav_head[wav_dec->wav_head_valid_size], in_buffer, r_size);
                wav_dec->wav_head_valid_size += r_size;
                out_size = 0;
            }

            if (wav_dec->wav_head_valid_size == 44)
            {
                if (BK_OK != check_wav_head(wav_dec, wav_dec->wav_head, 44))
                {
                    BK_LOGE(TAG, "%s, %d, check wav head fail \n", __func__, __LINE__);
                    return AEL_PROCESS_FAIL;
                }
            }

            /* report music info */
            if (BK_OK != music_info_report(self))
            {
                BK_LOGE(TAG, "%s, %d, report music_info fail \n", __func__, __LINE__);
                return AEL_PROCESS_FAIL;
            }

            if (out_size > 0)
            {
                WAV_DEC_DATA_DUMP_BY_UART_DATA(&in_buffer[r_size - out_size], out_size);

                WAV_DECODER_OUTPUT_START();
                w_size = audio_element_output(self, &in_buffer[r_size - out_size], out_size);
                WAV_DECODER_OUTPUT_END();
            }
        }
        else
        {
            WAV_DEC_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);

            WAV_DECODER_OUTPUT_START();
            w_size = audio_element_output(self, in_buffer, r_size);
            WAV_DECODER_OUTPUT_END();
        }
    }
    else
    {
        w_size = r_size;
    }

    WAV_DECODER_PROCESS_END();

    return w_size;
}

static bk_err_t _wav_decoder_destroy(audio_element_handle_t self)
{
    wav_decoder_t *wav_dec = (wav_decoder_t *)audio_element_getdata(self);
    audio_free(wav_dec);

    WAV_DEC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t wav_decoder_init(wav_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    wav_decoder_t *wav_dec = audio_calloc(1, sizeof(wav_decoder_t));

    AUDIO_MEM_CHECK(TAG, wav_dec, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _wav_decoder_open;
    cfg.close = _wav_decoder_close;
    cfg.seek = NULL;
    cfg.process = _wav_decoder_process;
    cfg.destroy = _wav_decoder_destroy;
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

    cfg.tag = "wav_decoder";

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _wav_decoder_init_exit);
    audio_element_setdata(el, wav_dec);

    WAV_DEC_DATA_DUMP_BY_UART_OPEN();

    return el;
_wav_decoder_init_exit:
    audio_free(wav_dec);
    return NULL;
}
