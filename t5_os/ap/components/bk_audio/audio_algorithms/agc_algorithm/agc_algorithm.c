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
#include <components/bk_audio/audio_algorithms/agc_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <modules/audio_agc.h>
#include <modules/audio_agc_types.h>


#define TAG  "AGC_ALGORITHM"

typedef struct agc_algorithm
{
    bk_agc_config_t agc_cfg;
    int32_t minLevel;
    int32_t maxLevel;
    uint32_t fs;
    int out_rb_block;
    void *agcInst;
} agc_algorithm_t;


static bk_err_t _agc_algorithm_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _agc_algorithm_open \n", audio_element_get_tag(self));
    agc_algorithm_t *agc = (agc_algorithm_t *)audio_element_getdata(self);

    if (BK_OK != bk_aud_agc_create(&agc->agcInst))
    {
        BK_LOGE(TAG, "[%s] agc create fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    if (BK_OK != bk_aud_agc_init(agc->agcInst, agc->minLevel, agc->maxLevel, agc->fs))
    {
        BK_LOGE(TAG, "[%s] agc init fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    if (BK_OK != bk_aud_agc_set_config(agc->agcInst, agc->agc_cfg))
    {
        BK_LOGE(TAG, "[%s] agc set config fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    BK_LOGD(TAG, "[%s] _agc_algorithm_open, frame_10ms_size: %d \n", audio_element_get_tag(self), agc->fs * 2 / 100);

    return BK_OK;
}

static bk_err_t _agc_algorithm_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _agc_algorithm_close \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _agc_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _agc_algorithm_process, in_len: %d \n", audio_element_get_tag(self), in_len);
    agc_algorithm_t *agc = (agc_algorithm_t *)audio_element_getdata(self);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        /* check r_size */
        uint32_t frame_10ms_size = agc->fs * 2 / 100;
        if (agc->out_rb_block != r_size / frame_10ms_size)
        {
            BK_LOGW(TAG, "[%s] r_size is not integer multiple of 10ms \n", audio_element_get_tag(self));
        }

        int16_t *agc_temp_ptr = audio_malloc(frame_10ms_size);
        AUDIO_MEM_CHECK(TAG, agc_temp_ptr, return -1);

        for (uint8_t i = 0; i < (r_size / frame_10ms_size); i++)
        {
            int res = bk_aud_agc_process(agc->agcInst, (int16_t *)(in_buffer + frame_10ms_size * i), frame_10ms_size / 2, agc_temp_ptr);
            if (0 != res)
            {
                BK_LOGW(TAG, "[%s] failed in WebRtcAgc_Process, res: %d \n", audio_element_get_tag(self), res);
                break;
            }
            os_memcpy((int16_t *)(in_buffer + frame_10ms_size * i), agc_temp_ptr, frame_10ms_size);
        }

        w_size = audio_element_output(self, (char *)in_buffer, (r_size / frame_10ms_size) * frame_10ms_size);
        audio_free(agc_temp_ptr);
    }
    else
    {
        w_size = r_size;
    }
    return w_size;
}

static bk_err_t _agc_algorithm_destroy(audio_element_handle_t self)
{
    agc_algorithm_t *agc = (agc_algorithm_t *)audio_element_getdata(self);

    if (BK_OK != bk_aud_agc_free(agc->agcInst))
    {
        BK_LOGE(TAG, "[%s] _agc_algorithm_destroy fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    audio_free(agc);
    return BK_OK;
}

audio_element_handle_t agc_algorithm_init(agc_algorithm_cfg_t *config)
{
    audio_element_handle_t el;

    /* check config */
    if (config->fs != 8000 && config->fs != 16000)
    {
        BK_LOGE(TAG, "check config->fs fail \n");
        return NULL;
    }

    agc_algorithm_t *agc_alg = audio_calloc(1, sizeof(agc_algorithm_t));
    AUDIO_MEM_CHECK(TAG, agc_alg, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _agc_algorithm_open;
    cfg.close = _agc_algorithm_close;
    cfg.seek = NULL;
    cfg.process = _agc_algorithm_process;
    cfg.destroy = _agc_algorithm_destroy;
    cfg.read = NULL;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    /* 10ms, 16bit */
    cfg.out_block_size = config->fs / 100 * 2 * config->out_rb_block;
    cfg.buffer_len = cfg.out_block_size;

    cfg.tag = "agc_algorithm";
    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _agc_algorithm_init_exit);

    agc_alg->agc_cfg = config->agc_cfg;
    agc_alg->minLevel = config->minLevel;
    agc_alg->maxLevel = config->maxLevel;
    agc_alg->fs = config->fs;
    agc_alg->out_rb_block = config->out_rb_block;
    agc_alg->agcInst = NULL;
    audio_element_setdata(el, agc_alg);
    return el;
_agc_algorithm_init_exit:
    audio_free(agc_alg);
    return NULL;
}

