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

#include <components/bk_audio/audio_algorithms/rsp_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>

#define TAG  "RSP_ALGORITHM"

//#define RSP_DEBUG   //GPIO debug

#ifdef RSP_DEBUG

#define RSP_ALGORITHM_START()                       do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define RSP_ALGORITHM_END()                         do { GPIO_DOWN(32); } while (0)

#define RSP_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define RSP_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define RSP_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define RSP_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define RSP_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define RSP_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define RSP_ALGORITHM_START()
#define RSP_ALGORITHM_END()

#define RSP_PROCESS_START()
#define RSP_PROCESS_END()

#define RSP_INPUT_START()
#define RSP_INPUT_END()

#define RSP_OUTPUT_START()
#define RSP_OUTPUT_END()

#endif

#if CONFIG_ADK_UTILS

#define RSP_DATA_DUMP
#ifdef RSP_DATA_DUMP

/* dump rsp data by uart */
#define RSP_DATA_DUMP_BY_UART      (0)

#if RSP_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util g_rsp_uart_util = {0};

#define RSP_DATA_DUMP_UART_ID            (2)
#define RSP_DATA_DUMP_UART_BAUD_RATE     (1000000)

#define RSP_DATA_DUMP_OPEN()                          uart_util_create(&g_rsp_uart_util, RSP_DATA_DUMP_UART_ID, RSP_DATA_DUMP_UART_BAUD_RATE)
#define RSP_DATA_DUMP_CLOSE()                         uart_util_destroy(&g_rsp_uart_util)
#define RSP_DATA_DUMP_BEFORE_DATA(data_buf, len)      //rsp_data_dump_before_data(data_buf, len)
#define RSP_DATA_DUMP_AFTER_DATA(data_buf, len)       uart_util_tx_data(&g_rsp_uart_util, data_buf, len)

#else

#define RSP_DATA_DUMP_OPEN()
#define RSP_DATA_DUMP_CLOSE()
#define RSP_DATA_DUMP_BEFORE_DATA(data_buf, len)
#define RSP_DATA_DUMP_AFTER_DATA(data_buf, len) 

#endif

#if CONFIG_ADK_UTILS
#define AUD_RSP_DATA_COUNT
#endif  //CONFIG_ADK_UTILS

#ifdef AUD_RSP_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t aud_rsp_count_util = {0};
#define AUD_RSP_DATA_COUNT_INTERVAL     (1000 * 4)
#define AUD_RSP_DATA_COUNT_TAG          "AUD_RSP"

#define AUD_RSP_DATA_COUNT_OPEN()               count_util_create(&aud_rsp_count_util, AUD_RSP_DATA_COUNT_INTERVAL, AUD_RSP_DATA_COUNT_TAG)
#define AUD_RSP_DATA_COUNT_CLOSE()              count_util_destroy(&aud_rsp_count_util)
#define AUD_RSP_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&aud_rsp_count_util, size)

#else

#define AUD_RSP_DATA_COUNT_OPEN()
#define AUD_RSP_DATA_COUNT_CLOSE()
#define AUD_RSP_DATA_COUNT_ADD_SIZE(size)

#endif  //AUD_RSP_DATA_COUNT

#endif  //RSP_DATA_DUMP

#endif  //CONFIG_ADK_UTILS



typedef struct rsp_algorithm
{
    aud_rsp_cfg_t rsp_cfg;
    int out_block_num;      /**< Number of output block, the size of block is frame size of 20ms audio data */
    int16_t *before_addr;
    int16_t *after_addr;
    uint32_t frame_size;    /**< 20ms data */
} rsp_algorithm_t;

static bk_err_t _rsp_algorithm_open(audio_element_handle_t self)
{
//	rsp_algorithm_t *rsp = (rsp_algorithm_t *)audio_element_getdata(self);
//	bk_err_t ret = bk_aud_rsp_init(rsp->rsp_cfg);
//	if (ret != BK_OK) {
//		BK_LOGE(TAG, "rsp_init Fail\n");
//	}
	BK_LOGD(TAG, "[%s] %s rsp init ok\n", audio_element_get_tag(self), __func__);

	return BK_OK;
}

static bk_err_t _rsp_algorithm_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    return BK_OK;
}

static int16_t rsp_dbg[1024] = {0};

static int _rsp_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] %s, in_len: %d \n", audio_element_get_tag(self), __func__, in_len);
    rsp_algorithm_t *rsp = (rsp_algorithm_t *)audio_element_getdata(self);

    RSP_PROCESS_START();
    RSP_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    if (r_size != in_len)
    {
        BK_LOGE(TAG, "rsp_data Waring: r_size=%d, in_len=%d \n", r_size, in_len);
    }

	AUD_RSP_DATA_COUNT_ADD_SIZE(r_size);
	rsp->before_addr = (int16_t *)in_buffer;

    RSP_INPUT_END();

    uint32_t  w_size = 0;
    if (r_size > 0)
    {
		uint32_t rr = r_size >> 1;
		uint32_t ww = r_size;

        RSP_ALGORITHM_START();
		bk_aud_rsp_process(rsp->before_addr, (uint32_t*)&rr, rsp_dbg, &ww);
        RSP_ALGORITHM_END();

        RSP_OUTPUT_START();
		w_size = audio_element_output(self, (char *)rsp_dbg, ww*2);
        RSP_OUTPUT_END();
		RSP_DATA_DUMP_AFTER_DATA(rsp_dbg, ww*2);

    }
    else
    {
        w_size = r_size;
    }

    RSP_PROCESS_END();
    return w_size;
}

static bk_err_t _rsp_algorithm_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    bk_aud_rsp_deinit();

    rsp_algorithm_t *rsp = (rsp_algorithm_t *)audio_element_getdata(self);

    audio_free(rsp);

    RSP_DATA_DUMP_CLOSE();
    AUD_RSP_DATA_COUNT_CLOSE();
    return BK_OK;
}

audio_element_handle_t rsp_algorithm_init(rsp_algorithm_cfg_t *config)
{
    audio_element_handle_t el;

    /* check config */
    if (config->rsp_cfg.src_rate == config->rsp_cfg.dest_rate)
    {
        BK_LOGE(TAG, "the src_rate = dest_rate, don't resample. \n");
        return NULL;
    }

    rsp_algorithm_t *rsp_alg = audio_calloc(1, sizeof(rsp_algorithm_t));
    AUDIO_MEM_CHECK(TAG, rsp_alg, return NULL);

    rsp_alg->frame_size = config->rsp_cfg.src_rate / 1000 * 2 * 20;   /* one frame <-> 20ms <-> Bytes */

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open       = _rsp_algorithm_open;
    cfg.close      = _rsp_algorithm_close;
    cfg.seek       = NULL;
    cfg.process    = _rsp_algorithm_process;
    cfg.destroy    = _rsp_algorithm_destroy;
    cfg.in_type    = PORT_TYPE_RB;
    cfg.read       = NULL;
    cfg.out_type   = PORT_TYPE_RB;
    cfg.write      = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio  = config->task_prio;
    cfg.task_core  = config->task_core;

    /* 20ms, 16bit */
    cfg.out_block_size = config->rsp_cfg.src_rate / 1000 * 2 * 20;
    cfg.out_block_num  = config->out_block_num*2;
	os_printf("[+++]%s, out_block_size:%d, block_num:%d\n", __func__, cfg.out_block_size, cfg.out_block_num);

    {
        cfg.buffer_len = rsp_alg->frame_size;
        cfg.multi_in_port_num = 0;
    }

    cfg.tag = "rsp_algorithm";
    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _rsp_algorithm_init_exit);

    rsp_alg->rsp_cfg.complexity  = config->rsp_cfg.complexity;
    rsp_alg->rsp_cfg.src_ch      = config->rsp_cfg.src_ch;
    rsp_alg->rsp_cfg.dest_ch     = config->rsp_cfg.dest_ch;
    rsp_alg->rsp_cfg.src_bits    = config->rsp_cfg.src_bits;
    rsp_alg->rsp_cfg.dest_bits   = config->rsp_cfg.dest_bits;
    rsp_alg->rsp_cfg.src_rate    = config->rsp_cfg.src_rate;
    rsp_alg->rsp_cfg.dest_rate   = config->rsp_cfg.dest_rate;
    rsp_alg->rsp_cfg.down_ch_idx = config->rsp_cfg.down_ch_idx;
    rsp_alg->out_block_num       = config->out_block_num;
    rsp_alg->before_addr         = NULL;
    rsp_alg->after_addr          = rsp_dbg;//NULL;

    audio_element_setdata(el, rsp_alg);

    bk_err_t ret = bk_aud_rsp_init(rsp_alg->rsp_cfg);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "rsp_init Fail\n");
        goto _rsp_algorithm_init_exit;
    }

    RSP_DATA_DUMP_OPEN();
    AUD_RSP_DATA_COUNT_OPEN();

    return el;
_rsp_algorithm_init_exit:
    audio_free(rsp_alg);
    return NULL;
}

