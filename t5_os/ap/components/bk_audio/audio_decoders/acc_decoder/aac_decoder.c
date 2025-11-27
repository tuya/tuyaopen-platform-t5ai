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
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <modules/aacdec.h>
#include <modules/pm.h>
#include <os/os.h>


#define TAG  "AAC_DEC"

//#define AAC_DECODER_DEBUG   //GPIO debug

#ifdef AAC_DECODER_DEBUG

#define AAC_DECODER_FRAME_START()           do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AAC_DECODER_FRAME_END()             do { GPIO_DOWN(32); } while (0)

#define AAC_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AAC_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define AAC_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AAC_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define AAC_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AAC_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define AAC_DECODER_FRAME_START()
#define AAC_DECODER_FRAME_END()

#define AAC_DECODER_PROCESS_START()
#define AAC_DECODER_PROCESS_END()

#define AAC_DECODER_INPUT_START()
#define AAC_DECODER_INPUT_END()

#define AAC_DECODER_OUTPUT_START()
#define AAC_DECODER_OUTPUT_END()

#endif


typedef struct aac_decoder
{
    HAACDecoder dec_handle;             /**< aac decoder handle */
    AACFrameInfo frame_info;            /**< aac frame infomation */
    uint8_t *main_buff;                 /**< mainbuff save data read */
    uint32_t main_buff_size;            /**< mainbuff size */
    uint32_t main_buff_remain_size;     /**< mainbuff remain size */
    uint32_t main_buff_consume_size;    /**< consume size of decode one frame */
    uint8_t *main_buff_readptr;         /**< read ptr of main buffer */
    int16_t *out_pcm_buff;              /**< out pcm buffer save data decoded */
    uint32_t out_pcm_buff_size;         /**< out pcm buffer size */
} aac_decoder_t;


static bk_err_t _aac_decoder_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    aac_decoder_t *aac_dec = (aac_decoder_t *)audio_element_getdata(self);
    aac_dec->main_buff_readptr = aac_dec->main_buff;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    return BK_OK;
}

static bk_err_t _aac_decoder_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    //nothing todo

    return BK_OK;
}

static bk_err_t music_info_report(audio_element_handle_t self)
{
    aac_decoder_t *aac_dec = (aac_decoder_t *)audio_element_getdata(self);

    audio_element_info_t info = {0};
    bk_err_t ret = audio_element_getinfo(self, &info);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] audio_element_getinfo fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    /* check frame information, report new frame information if frame information change */
    if (aac_dec->frame_info.bitsPerSample != info.bits
        || aac_dec->frame_info.sampRateOut != info.sample_rates
        || aac_dec->frame_info.nChans != info.channels)
    {
        info.bits = aac_dec->frame_info.bitsPerSample;
        info.sample_rates = aac_dec->frame_info.sampRateOut;
        info.channels = aac_dec->frame_info.nChans;
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
    }
    return BK_OK;
}

static int _aac_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    bk_err_t ret = BK_OK;
    int r_size = 0;

    BK_LOGV(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    aac_decoder_t *aac_dec = (aac_decoder_t *)audio_element_getdata(self);

    AAC_DECODER_PROCESS_START();

__retry:
    if (aac_dec->main_buff_remain_size < aac_dec->main_buff_size)
    {
        os_memmove(aac_dec->main_buff, aac_dec->main_buff_readptr, aac_dec->main_buff_remain_size);
        AAC_DECODER_INPUT_START();
        r_size = audio_element_input(self, (char *)(aac_dec->main_buff + aac_dec->main_buff_remain_size), aac_dec->main_buff_size - aac_dec->main_buff_remain_size);
        AAC_DECODER_INPUT_END();
        if (r_size > 0)
        {
            aac_dec->main_buff_remain_size = aac_dec->main_buff_remain_size + r_size;
        }
        aac_dec->main_buff_readptr = aac_dec->main_buff;
    }

    /* check remain data size */
    if (r_size == AEL_IO_DONE && aac_dec->main_buff_remain_size == 0)
    {
        BK_LOGE(TAG, "[%s] %s, %d, r_size: %d, remain_size: %d\n", audio_element_get_tag(self), __func__, __LINE__, r_size, aac_dec->main_buff_remain_size);
        return AEL_IO_DONE;
    }

    aac_dec->main_buff_consume_size = 0;
    int bytesLeft = aac_dec->main_buff_remain_size;
    AAC_DECODER_FRAME_START();
    ret = AACDecode(aac_dec->dec_handle, &aac_dec->main_buff_readptr, &bytesLeft, aac_dec->out_pcm_buff);
    AAC_DECODER_FRAME_END();
    if (ret == 0)
    {
        /* no error */
        AACGetLastFrameInfo(aac_dec->dec_handle, &aac_dec->frame_info);
        BK_LOGV(TAG, "aacFrameInfo.bitRate: %d \n", aac_dec->frame_info.bitRate);
        BK_LOGV(TAG, "aacFrameInfo.nChans: %d \n", aac_dec->frame_info.nChans);
        BK_LOGV(TAG, "aacFrameInfo.sampRateCore: %d \n", aac_dec->frame_info.sampRateCore);
        BK_LOGV(TAG, "aacFrameInfo.sampRateOut: %d \n", aac_dec->frame_info.sampRateOut);
        BK_LOGV(TAG, "aacFrameInfo.bitsPerSample: %d \n", aac_dec->frame_info.bitsPerSample);
        BK_LOGV(TAG, "aacFrameInfo.outputSamps: %d \n", aac_dec->frame_info.outputSamps);
        BK_LOGV(TAG, "aacFrameInfo.profile: %d \n", aac_dec->frame_info.profile);
        BK_LOGV(TAG, "aacFrameInfo.tnsUsed: %d \n", aac_dec->frame_info.tnsUsed);
        BK_LOGV(TAG, "aacFrameInfo.pnsUsed: %d \n", aac_dec->frame_info.pnsUsed);
        aac_dec->main_buff_consume_size += aac_dec->main_buff_remain_size - bytesLeft;
        aac_dec->main_buff_remain_size = bytesLeft;
        BK_LOGV(TAG, "[%s] remain_size: %d, consume_size: %d\n", audio_element_get_tag(self), aac_dec->main_buff_remain_size, aac_dec->main_buff_consume_size);
        r_size = aac_dec->frame_info.outputSamps * aac_dec->frame_info.bitsPerSample / 8;
    }
    else
    {
        /* exception handle */
        switch (ret)
        {
            case ERR_AAC_INDATA_UNDERFLOW:
                /* data is not enough, read more data and decode */
                goto __retry;
                break;

            default:
                BK_LOGE(TAG, "[%s] %s, %d, AACDecode failed, code is %d \n", audio_element_get_tag(self), __func__, __LINE__, ret);
                //goto __retry;
                return -1;
        }
    }

    ret = music_info_report(self);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] %s, %d, music_info_report fail\n", audio_element_get_tag(self), __func__, __LINE__);
        return 0;
    }

    int w_size = 0;
    if (r_size > 0)
    {
        AAC_DECODER_OUTPUT_START();
        w_size = audio_element_output(self, (char *)aac_dec->out_pcm_buff, r_size);
        AAC_DECODER_OUTPUT_END();
    }
    else
    {
        w_size = r_size;
    }

    AAC_DECODER_PROCESS_END();

    return w_size;
}

static bk_err_t _aac_decoder_destroy(audio_element_handle_t self)
{
    aac_decoder_t *aac_dec = (aac_decoder_t *)audio_element_getdata(self);

    if (aac_dec->main_buff)
    {
        audio_free(aac_dec->main_buff);
        aac_dec->main_buff = NULL;
    }
    if (aac_dec->out_pcm_buff)
    {
        audio_free(aac_dec->out_pcm_buff);
        aac_dec->out_pcm_buff = NULL;
    }
    if (aac_dec->dec_handle)
    {
        AACFreeDecoder(aac_dec->dec_handle);
        aac_dec->dec_handle = NULL;
    }
    audio_free(aac_dec);

    return BK_OK;
}

audio_element_handle_t aac_decoder_init(aac_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    aac_decoder_t *aac_dec = audio_calloc(1, sizeof(aac_decoder_t));
    AUDIO_MEM_CHECK(TAG, aac_dec, return NULL);
    os_memset(aac_dec, 0, sizeof(aac_decoder_t));

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _aac_decoder_open;
    cfg.close = _aac_decoder_close;
    cfg.seek = NULL;
    cfg.process = _aac_decoder_process;
    cfg.destroy = _aac_decoder_destroy;
    cfg.read = NULL;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_type = PORT_TYPE_RB;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = cfg.out_block_size;

    cfg.tag = "aac_decoder";

    aac_dec->main_buff_size = config->main_buff_size;
    aac_dec->out_pcm_buff_size = config->out_pcm_buff_size;
    aac_dec->dec_handle = AACInitDecoder();
    AUDIO_MEM_CHECK(TAG, aac_dec->dec_handle, goto _aac_decoder_init_exit);
    aac_dec->main_buff = (uint8_t *)audio_malloc(aac_dec->main_buff_size);
    AUDIO_MEM_CHECK(TAG, aac_dec->main_buff, goto _aac_decoder_init_exit);
    aac_dec->out_pcm_buff = (int16_t *)audio_malloc(aac_dec->out_pcm_buff_size);
    AUDIO_MEM_CHECK(TAG, aac_dec->out_pcm_buff, goto _aac_decoder_init_exit);

    BK_LOGD(TAG, "main_buff_size: %d, out_pcm_buff_size: %d\n", aac_dec->main_buff_size, aac_dec->out_pcm_buff_size);

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _aac_decoder_init_exit);
    audio_element_setdata(el, aac_dec);

    audio_element_info_t info = {0};
    audio_element_getinfo(el, &info);
    info.sample_rates = 8000;
    info.channels = 2;
    info.bits = 16;
    info.codec_fmt = BK_CODEC_TYPE_AAC;
    audio_element_setinfo(el, &info);

    return el;
_aac_decoder_init_exit:
    if (aac_dec->main_buff)
    {
        audio_free(aac_dec->main_buff);
        aac_dec->main_buff = NULL;
    }
    if (aac_dec->out_pcm_buff)
    {
        audio_free(aac_dec->out_pcm_buff);
        aac_dec->out_pcm_buff = NULL;
    }
    if (aac_dec->dec_handle)
    {
        AACFreeDecoder(aac_dec->dec_handle);
        aac_dec->dec_handle = NULL;
    }
    audio_free(aac_dec);
    aac_dec = NULL;
    return NULL;
}

