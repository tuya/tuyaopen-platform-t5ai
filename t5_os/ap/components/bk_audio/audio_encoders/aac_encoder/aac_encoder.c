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
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <modules/fdk_aac_enc/aacenc_lib.h>
#include <modules/pm.h>


#define TAG  "AAC_ENC"

//#define AAC_ENCODER_DEBUG   //GPIO debug

#ifdef AAC_ENCODER_DEBUG

#define AAC_ENCODER_FRAME_START()           do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AAC_ENCODER_FRAME_END()             do { GPIO_DOWN(32); } while (0)

#define AAC_ENCODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AAC_ENCODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define AAC_ENCODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AAC_ENCODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define AAC_ENCODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AAC_ENCODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define AAC_ENCODER_FRAME_START()
#define AAC_ENCODER_FRAME_END()

#define AAC_ENCODER_PROCESS_START()
#define AAC_ENCODER_PROCESS_END()

#define AAC_ENCODER_INPUT_START()
#define AAC_ENCODER_INPUT_END()

#define AAC_ENCODER_OUTPUT_START()
#define AAC_ENCODER_OUTPUT_END()

#endif


/* aac encoder output aac data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count aac encoder output aac data. */
#if CONFIG_ADK_UTILS

#define AAC_ENC_OUTPUT_DATA_COUNT

#endif  //CONFIG_ADK_UTILS


#ifdef AAC_ENC_OUTPUT_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t aac_enc_output_count_util = {0};
#define AAC_ENC_OUTPUT_DATA_COUNT_INTERVAL     (1000 * 4)
#define AAC_ENC_OUTPUT_DATA_COUNT_TAG          "AAC_ENC_OUTPUT"

#define AAC_ENC_OUTPUT_DATA_COUNT_OPEN()               count_util_create(&aac_enc_output_count_util, AAC_ENC_OUTPUT_DATA_COUNT_INTERVAL, AAC_ENC_OUTPUT_DATA_COUNT_TAG)
#define AAC_ENC_OUTPUT_DATA_COUNT_CLOSE()              count_util_destroy(&aac_enc_output_count_util)
#define AAC_ENC_OUTPUT_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&aac_enc_output_count_util, size)

#else

#define AAC_ENC_OUTPUT_DATA_COUNT_OPEN()
#define AAC_ENC_OUTPUT_DATA_COUNT_CLOSE()
#define AAC_ENC_OUTPUT_DATA_COUNT_ADD_SIZE(size)

#endif  //AAC_ENC_OUTPUT_DATA_COUNT


typedef struct aac_encoder
{
    HANDLE_AACENCODER       enc_handle;

    uint8_t                 chl_num;            /**< channel number */
    uint32_t                samp_rate;          /**< sample rate */
    uint8_t                 bits;               /**< Bit wide (8, 16, 24, 32 bits) */
    uint32_t                modules;            /**< modules, refer to "encModules" parameter description of "aacEncOpen" spi in aacenc_lib.h */
    uint8_t                 aot;                /**< Audio object type, refer to "AACENC_AOT" parameter description in aacenc_lib.h */
    uint32_t                bitrate;            /**< encoder bitrate, refer to "AACENC_BITRATE" parameter description in aacenc_lib.h */
    uint8_t                 bitrate_mode;       /**< Bitrate mode, refer to "AACENC_BITRATEMODE" parameter description in aacenc_lib.h */
    int8_t                  sbr_mode;           /**< sbr mode, refer to "AACENC_SBR_MODE" parameter description in aacenc_lib.h */
    uint32_t                granule_length;     /**< audio frame length in samples, refer to "AACENC_GRANULE_LENGTH" parameter description in aacenc_lib.h */
    uint8_t                 chl_order;          /**< Input audio data channel ordering scheme, refer to "AACENC_CHANNELORDER" parameter description in aacenc_lib.h */
    uint8_t                 afterburner_en;     /**< afterburner feature enable, refer to "AACENC_AFTERBURNER" parameter description in aacenc_lib.h */
    uint8_t                 transport_type;     /**< Transport type, refer to "AACENC_TRANSMUX" parameter description in aacenc_lib.h */

    int                     buffer_len;         /**< Buffer length use for aac Element */
    uint32_t                out_buffer_len;     /**< one frame data size(byte) of output aac data, less than in_frame_size */
    int8_t                  *out_frame_buff;    /**< out buffer save aac data that has been encoded by aac encoder */
    int                     out_block_size;     /**< Size of output block, at  least larger than the size of one frame of output aac data */
    int                     out_block_num;      /**< Number of output block */
    uint32_t                in_pool_len;        /**< input data pool size(byte), save data if the valid data in pool is less than one frame s*/
    int8_t                  *in_pool_buff;      /**< input pool buffer */
    uint32_t                valid_size;         /**< the valid data size(byte) in input pool buffer */
    uint32_t                frame_size;         /**< one frame data size(byte) of input pcm data, granule_length * bits / 8 * chl_num */

    beken_semaphore_t       aac_enc_init_sem;   /**< only use when aac encoder lib init */
} aac_encoder_t;


static HANDLE_AACENCODER _aac_encoder_lib_init(aac_encoder_t *aac_enc)
{
    CHANNEL_MODE mode;
    HANDLE_AACENCODER handle;

    switch (aac_enc->chl_num)
    {
        case 1:
            mode = MODE_1;
            break;
        case 2:
            mode = MODE_2;
            break;
        case 3:
            mode = MODE_1_2;
            break;
        case 4:
            mode = MODE_1_2_1;
            break;
        case 5:
            mode = MODE_1_2_2;
            break;
        case 6:
            mode = MODE_1_2_2_1;
            break;
        default:
            BK_LOGE(TAG, "%s, %d, not supported channels: %d\n", __func__, __LINE__, aac_enc->chl_num);
            return NULL;
    }

    if (aacEncOpen(&handle, aac_enc->modules, aac_enc->chl_num) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, open encoder fail\n", __func__, __LINE__);
        return NULL;
    }

    if (aacEncoder_SetParam(handle, AACENC_AOT, aac_enc->aot) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the AOT: %d fail\n", __func__, __LINE__, aac_enc->aot);
        goto fail;
    }

    if (aac_enc->aot == 39 && aac_enc->sbr_mode)
    {
        if (aacEncoder_SetParam(handle, AACENC_SBR_MODE, aac_enc->sbr_mode) != AACENC_OK)
        {
            BK_LOGE(TAG, "%s, %d, SBR mode: %d for ELD fail\n", __func__, __LINE__, aac_enc->sbr_mode);
            goto fail;
        }
    }

    if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, aac_enc->samp_rate) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the sample rate: %d fail\n", __func__, __LINE__, aac_enc->samp_rate);
        goto fail;
    }

    if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the channel mode: %d fail, chl_num: %d\n", __func__, __LINE__, mode, aac_enc->chl_num);
        goto fail;
    }

    if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, aac_enc->chl_order) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the channel order: %d fail\n", __func__, __LINE__, aac_enc->chl_order);
        goto fail;
    }

    if (aac_enc->bitrate_mode)
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATEMODE, aac_enc->bitrate_mode) != AACENC_OK)
        {
            BK_LOGE(TAG, "%s, %d, set the bitrate mode: %d VBR fail\n", __func__, __LINE__, aac_enc->bitrate_mode);
            goto fail;
        }
    }
    else
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATE, aac_enc->bitrate) != AACENC_OK)
        {
            BK_LOGE(TAG, "%s, %d, set the bitrate: %d fail\n", __func__, __LINE__, aac_enc->bitrate);
            goto fail;
        }
    }

    if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, aac_enc->transport_type) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the tarnsport type: %d fail\n", __func__, __LINE__, aac_enc->transport_type);
        goto fail;
    }

    if (aacEncoder_SetParam(handle, AACENC_AFTERBURNER, aac_enc->afterburner_en) != AACENC_OK)
    {
        BK_LOGE(TAG, "%s, %d, set the afterburner mode: %d fail\n", __func__, __LINE__, aac_enc->afterburner_en);
        goto fail;
    }

    if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
         BK_LOGE(TAG, "%s, %d, initialize the encoder fail\n", __func__, __LINE__);
        goto fail;
    }

    return handle;

fail:
    if (handle)
    {
        aacEncClose(&handle);
    }

    return NULL;
}

static void _aac_enc_open_task_main(    beken_thread_arg_t param_data)
{
    aac_encoder_t *aac_enc = (aac_encoder_t *)param_data;

    aac_enc->enc_handle = _aac_encoder_lib_init(aac_enc);

    rtos_set_semaphore(&aac_enc->aac_enc_init_sem);

    rtos_delete_thread(NULL);
}

static int _aac_encoder_lib_init_by_task(aac_encoder_t *aac_enc)
{
    beken_thread_t acc_enc_open_thread = NULL;
    bk_err_t ret = BK_OK;
    bk_err_t err  = BK_OK;

    ret = rtos_init_semaphore(&aac_enc->aac_enc_init_sem, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = audio_create_thread(&acc_enc_open_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "aac_enc_init",
                             (beken_thread_function_t)_aac_enc_open_task_main,
                             1024 * 8,
                             (void *)aac_enc,
                             0);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create aac encoder init task fail \n", __func__, __LINE__);
        err = BK_FAIL;
        goto out;
    }

    ret = rtos_get_semaphore(&aac_enc->aac_enc_init_sem, 20000);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, get semaphore fail, ret: %d\n", __func__, __LINE__, ret);
        err = BK_FAIL;
        goto out;
    }

out:
    /* delete semaphore */
    if (aac_enc->aac_enc_init_sem)
    {
        rtos_deinit_semaphore(&aac_enc->aac_enc_init_sem);
        aac_enc->aac_enc_init_sem = NULL;
    }

    return err;
}

static int _aac_encoder_lib_frame_process(HANDLE_AACENCODER aac_handle, int in_bytes, int16_t *data_in, int out_buf_len, int *out_bytes, int8_t *data_out)
{
    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
    AACENC_InArgs in_args = { 0 };
    AACENC_OutArgs out_args = { 0 };
    int in_identifier = IN_AUDIO_DATA;
    int out_identifier = OUT_BITSTREAM_DATA;
    int in_size, in_elem_size;
    int out_buf_size, out_elem_size;
    void *in_ptr, *out_ptr;
    int err;

    in_ptr = data_in;
    in_size = in_bytes;
    in_elem_size = 2;
    in_args.numInSamples = in_size <= 0 ? -1 : in_size / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = data_out;
    out_buf_size = out_buf_len;
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_buf_size;
    out_buf.bufElSizes = &out_elem_size;

    err = aacEncEncode(aac_handle, &in_buf, &out_buf, &in_args, &out_args);

    *out_bytes = out_args.numOutBytes;

    BK_LOGV(TAG, "[out_args] numOutBytes: %d, numInSamples: %d, numAncBytes: %d, bitResState: %d\n", out_args.numOutBytes, out_args.numInSamples, out_args.numAncBytes, out_args.bitResState);

    return err;
}

static bk_err_t _aac_encoder_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
    //noting todo

    return BK_OK;
}

static bk_err_t _aac_encoder_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    //noting todo

    return BK_OK;
}

static int _aac_encoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);
    aac_encoder_t *aac_enc = (aac_encoder_t *)audio_element_getdata(self);

    int enc_size = 0;
    bk_err_t ret = BK_OK;
    int result = 0;

    AAC_ENCODER_PROCESS_START();

    /* Check whether the valid data in input pool buffer is enough for one frame.
     * If true, read and process the data in pool buffer until remaining data is not enough for one frame. And then read data from input ringbuffer.
     * If false, read data from input ringbuffer.
     */
retry:
    if (aac_enc->valid_size >= aac_enc->frame_size)
    {
        AAC_ENCODER_FRAME_START();
        ret = _aac_encoder_lib_frame_process(aac_enc->enc_handle, aac_enc->frame_size, (int16_t *)aac_enc->in_pool_buff, aac_enc->out_buffer_len, &enc_size, aac_enc->out_frame_buff);
        AAC_ENCODER_FRAME_END();
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] %s, %d, aac encode frame fail, ret: %d\n", audio_element_get_tag(self), __func__, __LINE__, ret);
            result = -1;
            goto out;
        }

        aac_enc->valid_size -= aac_enc->frame_size;
        os_memmove(aac_enc->in_pool_buff, aac_enc->in_pool_buff + aac_enc->frame_size, aac_enc->valid_size);

        /* output aac data */
        AAC_ENCODER_OUTPUT_START();
        result = audio_element_output(self, (char *)aac_enc->out_frame_buff, enc_size);
        AAC_ENCODER_OUTPUT_END();

        AAC_ENC_OUTPUT_DATA_COUNT_ADD_SIZE(enc_size);

        goto out;
    }
    else
    {
        AAC_ENCODER_INPUT_START();
        int r_size = audio_element_input(self, in_buffer, in_len);
        AAC_ENCODER_INPUT_END();
        if (r_size > 0)
        {
            if (r_size + aac_enc->valid_size > aac_enc->in_pool_len)
            {
                int remain_size = r_size + aac_enc->valid_size - aac_enc->in_pool_len;

                os_memcpy(aac_enc->in_pool_buff + aac_enc->valid_size, in_buffer, aac_enc->in_pool_len - aac_enc->valid_size);
                aac_enc->valid_size += aac_enc->in_pool_len;

                AAC_ENCODER_FRAME_START();
                ret = _aac_encoder_lib_frame_process(aac_enc->enc_handle, aac_enc->frame_size, (int16_t *)aac_enc->in_pool_buff, aac_enc->out_buffer_len, &enc_size, aac_enc->out_frame_buff);
                AAC_ENCODER_FRAME_END();
                if (ret != BK_OK)
                {
                    BK_LOGE(TAG, "[%s] %s, %d, aac encode frame fail, ret: %d\n", audio_element_get_tag(self), __func__, __LINE__, ret);
                    result = -1;
                    goto out;
                }

                aac_enc->valid_size -= aac_enc->frame_size;
                os_memmove(aac_enc->in_pool_buff, aac_enc->in_pool_buff + aac_enc->frame_size, aac_enc->valid_size);

                os_memcpy(aac_enc->in_pool_buff + aac_enc->valid_size, in_buffer + r_size - remain_size, remain_size);
                aac_enc->valid_size += remain_size;

                /* output aac data */
                AAC_ENCODER_OUTPUT_START();
                result = audio_element_output(self, (char *)aac_enc->out_frame_buff, enc_size);
                AAC_ENCODER_OUTPUT_END();

                AAC_ENC_OUTPUT_DATA_COUNT_ADD_SIZE(enc_size);

                goto out;
            }
            else
            {
                os_memcpy(aac_enc->in_pool_buff + aac_enc->valid_size, in_buffer, r_size);
                aac_enc->valid_size += r_size;
                /* not process one frame, retry  */
                goto retry;
            }
        }
        else
        {
            BK_LOGE(TAG, "[%s] %s, %d, read data from ringbuffer fail, ret: %d\n", audio_element_get_tag(self), __func__, __LINE__, ret);
            result = -1;
            goto out;
        }
    }

    AAC_ENCODER_PROCESS_END();

out:

    return result;
}

static bk_err_t _aac_encoder_destroy(audio_element_handle_t self)
{
    aac_encoder_t *aac_enc = (aac_encoder_t *)audio_element_getdata(self);

    if (aac_enc->in_pool_buff)
    {
        audio_free(aac_enc->in_pool_buff);
        aac_enc->in_pool_buff = NULL;
    }

    if (aac_enc->out_frame_buff)
    {
        audio_free(aac_enc->out_frame_buff);
        aac_enc->out_frame_buff = NULL;
    }

    if (aac_enc->enc_handle)
    {
        aacEncClose(&aac_enc->enc_handle);
        aac_enc->enc_handle = NULL;
    }

    audio_free(aac_enc);

    AAC_ENC_OUTPUT_DATA_COUNT_CLOSE();

    return BK_OK;
}


audio_element_handle_t aac_encoder_init(aac_encoder_cfg_t *config)
{
    audio_element_handle_t el;
    aac_encoder_t *aac_enc = audio_calloc(1, sizeof(aac_encoder_t));

    AUDIO_MEM_CHECK(TAG, aac_enc, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _aac_encoder_open;
    cfg.close = _aac_encoder_close;
    cfg.seek = NULL;
    cfg.process = _aac_encoder_process;
    cfg.destroy = _aac_encoder_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_type = PORT_TYPE_FB;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buffer_len;

    cfg.tag = "aac_encoder";

    aac_enc->chl_num = config->chl_num;
    aac_enc->samp_rate = config->samp_rate;
    aac_enc->bits = config->bits;
    aac_enc->modules = config->modules;
    aac_enc->aot = config->aot;
    aac_enc->bitrate = config->bitrate;
    aac_enc->bitrate_mode = config->bitrate_mode;
    aac_enc->sbr_mode = config->sbr_mode;
    aac_enc->granule_length = config->granule_length;
    aac_enc->chl_order = config->chl_order;
    aac_enc->afterburner_en = config->afterburner_en;
    aac_enc->transport_type = config->transport_type;
    aac_enc->buffer_len = config->buffer_len;
    aac_enc->in_pool_len = config->in_pool_len;
    aac_enc->out_buffer_len = config->out_buffer_len;
    aac_enc->out_block_size = config->out_block_size;
    aac_enc->out_block_num = config->out_block_num;
    /* frame size = one_frame_samples * bit / 8 * channel_number */
    aac_enc->frame_size = aac_enc->granule_length * aac_enc->bits / 8 * aac_enc->chl_num;

    BK_LOGD(TAG, "[aac encoder] frame_size: %d, buffer_len: %d, in_pool_len: %d, out_buffer_len: %d\n", aac_enc->frame_size, aac_enc->buffer_len, aac_enc->in_pool_len, aac_enc->out_buffer_len);
    BK_LOGD(TAG, "[aac encoder] out_block_size: %d, out_block_num: %d\n", aac_enc->out_block_size, aac_enc->out_block_num);

    /* create task to init aac encoder, avoid stack overflow */
    if (BK_OK != _aac_encoder_lib_init_by_task(aac_enc))
    {
        BK_LOGE(TAG, "%s, %d, _aac_encoder_lib_init_by_task fail\n", __func__, __LINE__);
        goto _aac_encoder_init_exit;
    }
    AUDIO_MEM_CHECK(TAG, aac_enc->enc_handle, goto _aac_encoder_init_exit);

    aac_enc->in_pool_buff = (int8_t *)audio_malloc(aac_enc->in_pool_len);
    aac_enc->valid_size = 0;
    AUDIO_MEM_CHECK(TAG, aac_enc->in_pool_buff, goto _aac_encoder_init_exit);
    aac_enc->out_frame_buff = (int8_t *)audio_malloc(aac_enc->out_buffer_len);
    AUDIO_MEM_CHECK(TAG, aac_enc->out_frame_buff, goto _aac_encoder_init_exit);

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _aac_encoder_init_exit);
    audio_element_setdata(el, aac_enc);

    AAC_ENC_OUTPUT_DATA_COUNT_OPEN();

    return el;

_aac_encoder_init_exit:
    if (aac_enc->in_pool_buff)
    {
        audio_free(aac_enc->in_pool_buff);
        aac_enc->in_pool_buff = NULL;
    }

    if (aac_enc->out_frame_buff)
    {
        audio_free(aac_enc->out_frame_buff);
        aac_enc->out_frame_buff = NULL;
    }

    if (aac_enc->enc_handle)
    {
        aacEncClose(&aac_enc->enc_handle);
        aac_enc->enc_handle = NULL;
    }

    audio_free(aac_enc);

    return NULL;
}

