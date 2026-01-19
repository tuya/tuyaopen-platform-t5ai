// Copyright 2024-2025 Beken
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

#include <driver/aud_dac.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include "ring_buffer.h"
#include "audio_play.h"
#include <modules/pm.h>


#define ONBOARD_SPK_PLAY_TAG "ob_spk"
#define LOGI(...) BK_LOGI(ONBOARD_SPK_PLAY_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(ONBOARD_SPK_PLAY_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(ONBOARD_SPK_PLAY_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(ONBOARD_SPK_PLAY_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(ONBOARD_SPK_PLAY_TAG, ##__VA_ARGS__)


#define DMA_CARRY_SPK_FRAME_NUM                (2)
#define DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL    (8)


//#define ONBOARD_SPK_PLAY_DEBUG

#ifdef ONBOARD_SPK_PLAY_DEBUG

#define AUD_DAC_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32); GPIO_DOWN(32);} while (0)

#define ONBOARD_SPK_PLAY_WRITE_RB_START()       do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define ONBOARD_SPK_PLAY_WRITE_RB_END()         do { GPIO_DOWN(33); } while (0)

#define ONBOARD_SPK_PLAY_READ_RB_START()        do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define ONBOARD_SPK_PLAY_READ_RB_END()          do { GPIO_DOWN(34); } while (0)

#define ONBOARD_SPK_PLAY_WRITE_FIFO_START()     do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define ONBOARD_SPK_PLAY_WRITE_FIFO_END()       do { GPIO_DOWN(35); } while (0)

#else

#define AUD_DAC_DMA_ISR_START()

#define ONBOARD_SPK_PLAY_WRITE_RB_START()
#define ONBOARD_SPK_PLAY_WRITE_RB_END()

#define ONBOARD_SPK_PLAY_READ_RB_START()
#define ONBOARD_SPK_PLAY_READ_RB_END()

#define ONBOARD_SPK_PLAY_WRITE_FIFO_START()
#define ONBOARD_SPK_PLAY_WRITE_FIFO_END()

#endif


#define ONBOARD_SPK_PLAY_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            LOGE("ONBOARD_SPK_PLAY_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

typedef enum
{
    SPK_DATA_READ_IDLE = 0,
    SPK_DATA_READ_START,
    SPK_DATA_READ_EXIT
} spk_data_read_op_t;

typedef struct
{
    spk_data_read_op_t op;
    void *param;
} spk_data_read_msg_t;


typedef struct
{
    uint32_t frame_size;                    /**< the frame size of audio dac ring buffer */
    uint8_t frame_num;                      /**< the frame numbers of audio dac ring buffer */
    int32_t *dma_rb_addr;                   /**< audio dac ring buffer memory address */
    RingBufferContext dma_rb;               /**< audio dac ring buffer context */
    dma_id_t dac_dma_id;                    /**< dma id carry audio dac FIFO data */

    ringbuf_handle_t rb;                    /**< read pool ringbuffer handle */
    uint32_t rb_size;                       /**< read pool size, unit byte */
    beken_semaphore_t can_process;
    uint8_t *read_buff;                     /**< the read pool buffer save speaker data need to play */

    beken_thread_t spk_data_read_task_hdl;
    beken_queue_t spk_data_read_msg_que;
    beken_semaphore_t sem;
    bool running;

    audio_play_sta_t state;                 /**< play state */
    audio_play_cfg_t config;
} onboard_speaker_play_priv_t;


static onboard_speaker_play_priv_t *gl_onboard_speaker_play = NULL;

static void onboard_speaker_state_set(onboard_speaker_play_priv_t *onboard_spk, audio_play_sta_t state)
{
    onboard_spk->state = state;
}

static audio_play_sta_t onboard_speaker_state_get(onboard_speaker_play_priv_t *onboard_spk)
{
    return onboard_spk->state;
}

static bk_err_t spk_data_read_send_msg(beken_queue_t queue, spk_data_read_op_t op, void *param)
{
    bk_err_t ret;
    spk_data_read_msg_t msg;

    if (!queue)
    {
        LOGE("%s, %d, queue: %p \n", __func__, __LINE__, queue);
        return BK_FAIL;
    }

    msg.op = op;
    msg.param = param;
    ret = rtos_push_to_queue(&queue, &msg, BEKEN_NO_WAIT);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, send message: %d fail, ret: %d\n", __func__, __LINE__, op, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

static void spk_data_read_task_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;

    onboard_speaker_play_priv_t *spk_data_read_handle = (onboard_speaker_play_priv_t *)param_data;

    spk_data_read_handle->running = false;
    uint32_t wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&spk_data_read_handle->sem);

    while (1)
    {
        spk_data_read_msg_t msg;
        ret = rtos_pop_from_queue(&spk_data_read_handle->spk_data_read_msg_que, &msg, wait_time);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case SPK_DATA_READ_IDLE:
                    spk_data_read_handle->running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case SPK_DATA_READ_EXIT:
                    goto spk_data_read_exit;
                    break;

                case SPK_DATA_READ_START:
                    spk_data_read_handle->running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        /* read speaker data and write to dac fifo */
        if (spk_data_read_handle->running)
        {
            if (BK_OK != rtos_get_semaphore(&spk_data_read_handle->can_process, 20 / portTICK_RATE_MS)) //portMAX_DELAY, 25 / portTICK_RATE_MS
            {
                //return -1;
                //player_log(LOG_ERR, "%s, %d, rtos_get_semaphore fail\n", __func__, __LINE__);
            }
            else
            {
                /* read speaker data from ringbuffer, and write to dac fifo */
                if (rb_bytes_filled(spk_data_read_handle->rb) >= spk_data_read_handle->frame_size)
                {
                    ONBOARD_SPK_PLAY_READ_RB_START();
                    int r_len = rb_read(spk_data_read_handle->rb, (char *)spk_data_read_handle->read_buff, spk_data_read_handle->frame_size, 0);
                    ONBOARD_SPK_PLAY_READ_RB_END();
                    if (r_len != spk_data_read_handle->frame_size)
                    {
                        os_memset(spk_data_read_handle->read_buff, 0, spk_data_read_handle->frame_size);
                    }
                }
                else
                {
                    /* fill silence data */
                    LOGV("%s, %d, fill silence data\n", __func__, __LINE__);
                    os_memset(spk_data_read_handle->read_buff, 0, spk_data_read_handle->frame_size);
                }
                ONBOARD_SPK_PLAY_WRITE_FIFO_START();
                ring_buffer_write(&spk_data_read_handle->dma_rb, spk_data_read_handle->read_buff, spk_data_read_handle->frame_size);
                ONBOARD_SPK_PLAY_WRITE_FIFO_END();
            }
        }
    }

spk_data_read_exit:

    spk_data_read_handle->running = false;

    if (spk_data_read_handle->can_process)
    {
        rtos_deinit_semaphore(&spk_data_read_handle->can_process);
        spk_data_read_handle->can_process = NULL;
    }

    /* delete msg queue */
    ret = rtos_deinit_queue(&spk_data_read_handle->spk_data_read_msg_que);
    if (ret != kNoErr)
    {
        LOGE("%s, %d, delete message queue fail\n", __func__, __LINE__);
    }
    spk_data_read_handle->spk_data_read_msg_que = NULL;

    /* delete task */
    spk_data_read_handle->spk_data_read_task_hdl = NULL;

    rtos_set_semaphore(&spk_data_read_handle->sem);

    rtos_delete_thread(NULL);
}

static bk_err_t spk_data_read_task_init(onboard_speaker_play_priv_t *spk_data_read_handle)
{
    bk_err_t ret = BK_OK;
    bk_err_t err = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(spk_data_read_handle);

    spk_data_read_handle->read_buff = os_malloc(spk_data_read_handle->frame_size);
    ONBOARD_SPK_PLAY_CHECK_NULL(spk_data_read_handle->read_buff);

    os_memset(spk_data_read_handle->read_buff, 0, sizeof(spk_data_read_handle->frame_size));

    ret = rtos_init_semaphore(&spk_data_read_handle->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_init_semaphore(&spk_data_read_handle->can_process, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_init_queue(&spk_data_read_handle->spk_data_read_msg_que,
                          "spk_data_rd_que",
                          sizeof(spk_data_read_msg_t),
                          5);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate spk data read message queue fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_create_thread(&spk_data_read_handle->spk_data_read_task_hdl,
                             (BEKEN_DEFAULT_WORKER_PRIORITY - 1),
                             "spk_data_rd",
                             (beken_thread_function_t)spk_data_read_task_main,
                             2048,
                             (beken_thread_arg_t)spk_data_read_handle);
    if (ret != BK_OK)
    {
        err = BK_FAIL;
        LOGE("%s, %d, create spk data read task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&spk_data_read_handle->sem, BEKEN_NEVER_TIMEOUT);

    LOGD("init spk data read task complete\n");

    return BK_OK;

fail:

    if (spk_data_read_handle->sem)
    {
        rtos_deinit_semaphore(&spk_data_read_handle->sem);
        spk_data_read_handle->sem = NULL;
    }

    if (spk_data_read_handle->can_process)
    {
        rtos_deinit_semaphore(&spk_data_read_handle->can_process);
        spk_data_read_handle->can_process = NULL;
    }

    if (spk_data_read_handle->spk_data_read_msg_que)
    {
        rtos_deinit_queue(&spk_data_read_handle->spk_data_read_msg_que);
        spk_data_read_handle->spk_data_read_msg_que = NULL;
    }

    if (spk_data_read_handle->read_buff)
    {
        os_free(spk_data_read_handle->read_buff);
        spk_data_read_handle->read_buff = NULL;
    }

    return err;
}

bk_err_t spk_data_read_task_deinit(onboard_speaker_play_priv_t *spk_data_read_handle)
{
    bk_err_t ret = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(spk_data_read_handle);

    LOGD("%s\n", __func__);

    ret = spk_data_read_send_msg(spk_data_read_handle->spk_data_read_msg_que, SPK_DATA_READ_EXIT, NULL);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, send message: SPK_DATA_READ_EXIT fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* abort read speaker data from pipe ringbuffer.
       And handle "SPK_DATA_READ_EXIT" opcode.
     */
    rb_abort(spk_data_read_handle->rb);

    rtos_get_semaphore(&spk_data_read_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&spk_data_read_handle->sem);
    spk_data_read_handle->sem = NULL;

    if (spk_data_read_handle->read_buff)
    {
        os_free(spk_data_read_handle->read_buff);
    }

    LOGD("deinit spk data read complete\n");

    return BK_OK;
}


static bk_err_t aud_dac_dma_deconfig(onboard_speaker_play_priv_t *onboard_spk)
{
    if (onboard_spk == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_spk->dac_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_spk->dac_dma_id);
    //bk_dma_driver_deinit();
    if (onboard_spk->dma_rb_addr)
    {
        ring_buffer_clear(&onboard_spk->dma_rb);
        os_free(onboard_spk->dma_rb_addr);
        onboard_spk->dma_rb_addr = NULL;
    }

    return BK_OK;
}


/* Carry one frame audio dac data(20ms) to DAC FIFO complete */
static void aud_dac_dma_finish_isr(void)
{
    //LOGD("%s\n", __func__);
    AUD_DAC_DMA_ISR_START();

    bk_err_t ret = rtos_set_semaphore(&gl_onboard_speaker_play->can_process);
    if (ret != BK_OK)
    {
        LOGE("%s, rtos_set_semaphore fail \n", __func__);
    }
}

static bk_err_t aud_dac_dma_config(onboard_speaker_play_priv_t *onboard_spk)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dac_port_addr;
    bk_err_t err = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    /* init dma driver */
/*
    ret = bk_dma_driver_init();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dma_driver_init fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto exit;
    }
*/

    //malloc dma channel
    onboard_spk->dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_spk->dac_dma_id < DMA_ID_0) || (onboard_spk->dac_dma_id >= DMA_ID_MAX))
    {
        LOGE("malloc dma fail \n");
        err = BK_FAIL;
        goto exit;
    }

    /* the pause address can not is the same as the end address of dma, so add 8 bytes to protect speaker ring buffer. */
    onboard_spk->dma_rb_addr = (int32_t *)os_malloc(2 * onboard_spk->frame_size + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk->dma_rb_addr);
    ring_buffer_init(&onboard_spk->dma_rb, (uint8_t *)onboard_spk->dma_rb_addr, onboard_spk->frame_size * onboard_spk->frame_num + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL, onboard_spk->dac_dma_id, RB_DMA_TYPE_READ);
    LOGD("%s, %d, dma_id: %d, dma_rb_addr: %p, dma_rb_size: %d \n", __func__, __LINE__, onboard_spk->dac_dma_id, onboard_spk->dma_rb_addr, onboard_spk->frame_size * onboard_spk->frame_num + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
    /* init dma channel */
    os_memset(&dma_config, 0, sizeof(dma_config_t));
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.dst.dev = DMA_DEV_AUDIO;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    switch (onboard_spk->config.nChans)
    {
        case 1:
            dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
            break;
        case 2:
            dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
            break;
        default:
            break;
    }
    /* get dac fifo address */
    ret = bk_aud_dac_get_fifo_addr(&dac_port_addr);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, get dac fifo address fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto exit;
    }
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = dac_port_addr;
    dma_config.dst.end_addr = dac_port_addr + 4;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = (uint32_t)onboard_spk->dma_rb_addr;
    dma_config.src.end_addr = (uint32_t)(onboard_spk->dma_rb_addr) + onboard_spk->frame_size * onboard_spk->frame_num + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL;
    ret = bk_dma_init(onboard_spk->dac_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dma_init fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_spk->dac_dma_id, onboard_spk->frame_size);
#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_spk->dac_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_spk->dac_dma_id, DMA_ATTR_SEC);
#endif
    /* register dma isr */
    bk_dma_register_isr(onboard_spk->dac_dma_id, NULL, (void *)aud_dac_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_spk->dac_dma_id);

    return BK_OK;
exit:
    aud_dac_dma_deconfig(onboard_spk);
    return err;
}

static bk_err_t spk_dac_deconfig(void)
{
    return bk_aud_dac_deinit();
}


static bk_err_t spk_dac_config(onboard_speaker_play_priv_t *onboard_spk)
{
    bk_err_t ret = BK_OK;
    bk_err_t err = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    /* init onboard speaker */
    aud_dac_config_t aud_dac_cfg = DEFAULT_AUD_DAC_CONFIG();
    if (onboard_spk->config.nChans == 1)
    {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_L;
    }
    else if (onboard_spk->config.nChans == 2)
    {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_LR;
    }
    else
    {
        LOGE("dac_chl: %d is not support \n", onboard_spk->config.nChans);
        err = BK_FAIL;
        goto fail;
    }
    aud_dac_cfg.samp_rate = onboard_spk->config.sampRate;
    aud_dac_cfg.dac_gain = onboard_spk->config.volume;
    LOGD("dac_cfg chl_num: %s, dac_gain: 0x%02x, samp_rate: %d, clk_src: %s, dac_mode: %s \n",
            aud_dac_cfg.dac_chl == AUD_DAC_CHL_L ? "AUD_DAC_CHL_L" : "AUD_DAC_CHL_LR",
            aud_dac_cfg.dac_gain,
            aud_dac_cfg.samp_rate,
            aud_dac_cfg.clk_src == 1 ? "APLL" : "XTAL",
            aud_dac_cfg.work_mode == 1 ? "AUD_DAC_WORK_MODE_SIGNAL_END" : "AUD_DAC_WORK_MODE_DIFFEN");
    ret = bk_aud_dac_init(&aud_dac_cfg);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, aud_dac_init fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    return BK_OK;

fail:

    spk_dac_deconfig();

    return err;
}


static bk_err_t onboard_speaker_open(onboard_speaker_play_priv_t *onboard_spk)
{
    bk_err_t ret = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    ret = spk_dac_config(onboard_spk);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, spk_dac_config fail\n", __func__, __LINE__);
        goto onboard_speaker_open_fail;
    }

    ret = aud_dac_dma_config(onboard_spk);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, aud_dac_dma_config fail\n", __func__, __LINE__);
        goto onboard_speaker_open_fail;
    }

    /* init ringbuffer */
    onboard_spk->rb = rb_create(onboard_spk->rb_size);
    if (!onboard_spk->rb)
    {
        LOGE("%s, %d, create ringbuffer fail\n", __func__, __LINE__);
        goto onboard_speaker_open_fail;
    }

    ret = spk_data_read_task_init(onboard_spk);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, spk_data_read_task_init fail\n", __func__, __LINE__);
        goto onboard_speaker_open_fail;
    }

    return BK_OK;

onboard_speaker_open_fail:

    return BK_FAIL;
}

static bk_err_t onboard_speaker_start(onboard_speaker_play_priv_t *onboard_spk)
{
    bk_err_t ret = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    /* fill two frame silence data */
    os_memset(onboard_spk->read_buff, 0, onboard_spk->frame_size);
    ring_buffer_write(&onboard_spk->dma_rb, onboard_spk->read_buff, onboard_spk->frame_size);
    ring_buffer_write(&onboard_spk->dma_rb, onboard_spk->read_buff, onboard_spk->frame_size);

    ret = spk_data_read_send_msg(onboard_spk->spk_data_read_msg_que, SPK_DATA_READ_START, NULL);

    ret = bk_dma_start(onboard_spk->dac_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_dac_start();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac stop fail\n", __func__, __LINE__);
    }

    return ret;
}

static bk_err_t onboard_speaker_stop(onboard_speaker_play_priv_t *onboard_spk)
{
    LOGD("%s \n", __func__);

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    bk_err_t ret = bk_dma_stop(onboard_spk->dac_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_dac_stop();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac stop fail\n", __func__, __LINE__);
    }

    ret = spk_data_read_send_msg(onboard_spk->spk_data_read_msg_que, SPK_DATA_READ_IDLE, NULL);

    return ret;
}

static bk_err_t onboard_speaker_close(onboard_speaker_play_priv_t *onboard_spk)
{
    LOGD("%s \n", __func__);

    ONBOARD_SPK_PLAY_CHECK_NULL(onboard_spk);

    spk_data_read_task_deinit(onboard_spk);

    bk_err_t ret = bk_dma_stop(onboard_spk->dac_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_dac_stop();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dac stop fail\n", __func__, __LINE__);
    }

    spk_dac_deconfig();

    aud_dac_dma_deconfig(onboard_spk);

    /* deinit ringbuffer */
    rb_destroy(onboard_spk->rb);

    return BK_OK;
}

static bk_err_t onboard_speaker_mute(void)
{
    LOGD("%s \n", __func__);

    bk_aud_dac_mute();

    return BK_OK;
}

static bk_err_t onboard_speaker_unmute(void)
{
    LOGD("%s \n", __func__);

    bk_aud_dac_unmute();

    return BK_OK;
}

static bk_err_t onboard_speaker_set_volume(int volume)
{
    LOGD("%s %d\n", __func__, volume);

    return bk_aud_dac_set_gain(volume);
}

static int onboard_speaker_play_open(audio_play_t *play, audio_play_cfg_t *config)
{
    onboard_speaker_play_priv_t *temp_onboard_speaker_play = NULL;
    int ret;

    if (!play || !config)
    {
        LOGE("%s, %d, params error, play: %p, config: %p\n", __func__, __LINE__, play, config);
        return BK_FAIL;
    }

    onboard_speaker_play_priv_t *priv = (onboard_speaker_play_priv_t *)play->play_ctx;
    if (priv != NULL)
    {
        LOGE("%s, %d, onboard_speaker_play: %p already open\n", __func__, __LINE__, priv);
        return BK_FAIL;
    }

    temp_onboard_speaker_play = os_malloc(sizeof(onboard_speaker_play_priv_t));
    if (!temp_onboard_speaker_play)
    {
        LOGE("%s, %d, os_malloc temp_onboard_speaker_play: %d fail\n", __func__, __LINE__, sizeof(onboard_speaker_play_priv_t));
        return BK_FAIL;
    }

    play->play_ctx = temp_onboard_speaker_play;

    gl_onboard_speaker_play = temp_onboard_speaker_play;

    temp_onboard_speaker_play->frame_size = config->frame_size;
    temp_onboard_speaker_play->frame_num = DMA_CARRY_SPK_FRAME_NUM;
    temp_onboard_speaker_play->rb_size = config->pool_size;

    temp_onboard_speaker_play->config.port = config->port;
    temp_onboard_speaker_play->config.nChans = config->nChans;
    temp_onboard_speaker_play->config.sampRate = config->sampRate;
    temp_onboard_speaker_play->config.bitsPerSample = config->bitsPerSample;
    temp_onboard_speaker_play->config.volume = config->volume;
    temp_onboard_speaker_play->config.play_mode = config->play_mode;
    temp_onboard_speaker_play->config.frame_size = config->frame_size;
    temp_onboard_speaker_play->config.pool_size = config->pool_size;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    ret = onboard_speaker_open(temp_onboard_speaker_play);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard spk open fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return ret;
    }

    ret = onboard_speaker_start(temp_onboard_speaker_play);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard spk start fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return ret;
    }

    onboard_speaker_state_set(temp_onboard_speaker_play, AUDIO_PLAY_STA_RUNNING);

    LOGD("onboard spk open complete \n");

    return BK_OK;
}

static int onboard_speaker_play_close(audio_play_t *play)
{
    bk_err_t ret = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(play);
    onboard_speaker_play_priv_t *priv = (onboard_speaker_play_priv_t *)play->play_ctx;
    ONBOARD_SPK_PLAY_CHECK_NULL(priv);

    if (onboard_speaker_state_get(priv) == AUDIO_PLAY_STA_IDLE)
    {
        return BK_OK;
    }

    LOGD("%s \n", __func__);

    ret = onboard_speaker_stop(priv);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard spk stop fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    ret = onboard_speaker_close(priv);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard spk close fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    onboard_speaker_state_set(priv, AUDIO_PLAY_STA_IDLE);

    LOGV("onboard spk close complete\n");
    return BK_OK;
}

static int onboard_speaker_play_write(audio_play_t *play, char *buffer, uint32_t len)
{
    if (!play || !buffer || !len)
    {
        LOGE("%s, %d, params error, play: %p, buffer: %p, len: %d\n", __func__, __LINE__, play, buffer, len);
        return BK_FAIL;
    }

    onboard_speaker_play_priv_t *priv = (onboard_speaker_play_priv_t *)play->play_ctx;
    ONBOARD_SPK_PLAY_CHECK_NULL(priv);

    uint32_t need_w_len = len;

    ONBOARD_SPK_PLAY_WRITE_RB_START();

    while (need_w_len)
    {
        int w_len = rb_write(priv->rb, buffer, need_w_len, BEKEN_WAIT_FOREVER);
        if (w_len > 0)
        {
            need_w_len -= w_len;
        }
        else
        {
            LOGE("rb_write FAIL, w_len: %d \n", w_len);
            return BK_FAIL;
        }
    }

    ONBOARD_SPK_PLAY_WRITE_RB_END();

    return len;
}


static int onboard_speaker_play_control(audio_play_t *play, audio_play_ctl_t ctl)
{
    int ret = BK_OK;

    ONBOARD_SPK_PLAY_CHECK_NULL(play);

    onboard_speaker_play_priv_t *priv = (onboard_speaker_play_priv_t *)play->play_ctx;
    ONBOARD_SPK_PLAY_CHECK_NULL(priv);

    if (ctl == AUDIO_PLAY_PAUSE)
    {
        if (onboard_speaker_state_get(priv) == AUDIO_PLAY_STA_PAUSED)
        {
            return BK_OK;
        }

        /* first mute audio dac, then pause audio pipeline */
        ret = onboard_speaker_stop(priv);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard spk paly pause fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        onboard_speaker_state_set(priv, AUDIO_PLAY_STA_PAUSED);
    }
    else if (ctl == AUDIO_PLAY_RESUME)
    {
        if (onboard_speaker_state_get(priv) == AUDIO_PLAY_STA_RUNNING)
        {
            return BK_OK;
        }

        /* first resume audio play, then unmute audio dac */
        ret = onboard_speaker_start(priv);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard spk paly start fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        onboard_speaker_state_set(priv, AUDIO_PLAY_STA_RUNNING);
    }
    else if (ctl == AUDIO_PLAY_MUTE)
    {
        ret = onboard_speaker_mute();
        if (ret != BK_OK)
        {
            LOGE("%s, onboard spk paly mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (ctl == AUDIO_PLAY_UNMUTE)
    {
        ret = onboard_speaker_unmute();
        if (ret != BK_OK)
        {
            LOGE("%s, onboard spk paly unmute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (ctl == AUDIO_PLAY_SET_VOLUME)
    {
        ret = onboard_speaker_set_volume(play->config.volume);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard spk paly set volume fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else
    {
        //nothing to do
    }

    return ret;
}

audio_play_ops_t onboard_speaker_play_ops =
{
    .open =    onboard_speaker_play_open,
    .write =   onboard_speaker_play_write,
    .control = onboard_speaker_play_control,
    .close =   onboard_speaker_play_close,
};

audio_play_ops_t *get_onboard_speaker_play_ops(void)
{
    return &onboard_speaker_play_ops;
}

