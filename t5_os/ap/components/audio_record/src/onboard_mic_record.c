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

#include <driver/aud_adc.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include "ring_buffer.h"
#include "audio_record.h"
#include <modules/pm.h>


#define ONBOARD_MIC_RECORD_TAG "ob_mic"
#define LOGI(...) BK_LOGI(ONBOARD_MIC_RECORD_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(ONBOARD_MIC_RECORD_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(ONBOARD_MIC_RECORD_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(ONBOARD_MIC_RECORD_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(ONBOARD_MIC_RECORD_TAG, ##__VA_ARGS__)


#define DMA_CARRY_MIC_FRAME_NUM                (2)
#define DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL    (8)


//#define AUD_MIC_DATA_DUMP_BY_UART

#ifdef AUD_MIC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util g_aud_mic_uart_util = {0};
#define AUD_MIC_DATA_DUMP_UART_ID            (1)
#define AUD_MIC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define AUD_MIC_DATA_DUMP_OPEN()                        uart_util_create(&g_aud_mic_uart_util, AUD_MIC_DATA_DUMP_UART_ID, AUD_MIC_DATA_DUMP_UART_BAUD_RATE)
#define AUD_MIC_DATA_DUMP_CLOSE()                       uart_util_destroy(&g_aud_mic_uart_util)
#define AUD_MIC_DATA_DUMP_DATA(data_buf, len)           uart_util_tx_data(&g_aud_mic_uart_util, data_buf, len)
#else
#define AUD_MIC_DATA_DUMP_OPEN()
#define AUD_MIC_DATA_DUMP_CLOSE()
#define AUD_MIC_DATA_DUMP_DATA(data_buf, len)
#endif  //AUD_MIC_DATA_DUMP_BY_UART


//#define ONBOARD_MIC_RECORD_DEBUG

#ifdef ONBOARD_MIC_RECORD_DEBUG

#define AUD_ADC_DMA_ISR_START()                     do { GPIO_DOWN(32); GPIO_UP(32); GPIO_DOWN(32);} while (0)

#define ONBOARD_MIC_RECORD_READ_RB_START()          do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define ONBOARD_MIC_RECORD_READ_RB_END()            do { GPIO_DOWN(33); } while (0)

#define ONBOARD_MIC_RECORD_WRITE_RB_START()         do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define ONBOARD_MIC_RECORD_WRITE_RB_END()           do { GPIO_DOWN(34); } while (0)

#define ONBOARD_MIC_RECORD_READ_FIFO_START()        do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define ONBOARD_MIC_RECORD_READ_FIFO_END()          do { GPIO_DOWN(35); } while (0)

#else

#define AUD_ADC_DMA_ISR_START()

#define ONBOARD_MIC_RECORD_READ_RB_START()
#define ONBOARD_MIC_RECORD_READ_RB_END()

#define ONBOARD_MIC_RECORD_WRITE_RB_START()
#define ONBOARD_MIC_RECORD_WRITE_RB_END()

#define ONBOARD_MIC_RECORD_READ_FIFO_START()
#define ONBOARD_MIC_RECORD_READ_FIFO_END()

#endif


#define ONBOARD_MIC_RECORD_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            LOGE("ONBOARD_MIC_RECORD_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)

typedef enum
{
    MIC_DATA_WRITE_IDLE = 0,
    MIC_DATA_WRITE_START,
    MIC_DATA_WRITE_EXIT
} mic_data_write_op_t;

typedef struct
{
    mic_data_write_op_t op;
    void *param;
} mic_data_write_msg_t;


typedef struct
{
    uint32_t frame_size;                //the max frame number of mic ring buffer
    uint8_t frame_num;
    int32_t *dma_rb_addr;               //save adc data of mic
    RingBufferContext dma_rb;           //mic ring buffer context
    dma_id_t adc_dma_id;                //audio transfer ADC DMA id

    ringbuf_handle_t rb;                //pool for save mic data
    uint32_t rb_size;
    beken_semaphore_t can_process;      /**< can process */

    uint8_t *write_buff;

    beken_thread_t mic_data_write_task_hdl;
    beken_queue_t mic_data_write_msg_que;
    beken_semaphore_t sem;
    bool running;

    audio_record_sta_t state;                 /**< record state */
    audio_record_cfg_t config;
} onboard_mic_record_priv_t;


static onboard_mic_record_priv_t *gl_onboard_mic_record = NULL;

static void onboard_mic_state_set(onboard_mic_record_priv_t *onboard_mic, audio_record_sta_t state)
{
    onboard_mic->state = state;
}

static audio_record_sta_t onboard_mic_state_get(onboard_mic_record_priv_t *onboard_mic)
{
    return onboard_mic->state;
}

static bk_err_t mic_data_write_send_msg(beken_queue_t queue, mic_data_write_op_t op, void *param)
{
    bk_err_t ret;
    mic_data_write_msg_t msg;

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

static void mic_data_write_task_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;

    onboard_mic_record_priv_t *mic_data_write_handle = (onboard_mic_record_priv_t *)param_data;

    mic_data_write_handle->running = false;
    uint32_t wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&mic_data_write_handle->sem);

    while (1)
    {
        mic_data_write_msg_t msg;
        ret = rtos_pop_from_queue(&mic_data_write_handle->mic_data_write_msg_que, &msg, wait_time);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case MIC_DATA_WRITE_IDLE:
                    mic_data_write_handle->running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case MIC_DATA_WRITE_EXIT:
                    goto mic_data_write_exit;
                    break;

                case MIC_DATA_WRITE_START:
                    mic_data_write_handle->running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        /* read mic data and write to ringbuffer */
        if (mic_data_write_handle->running)
        {
            if (BK_OK != rtos_get_semaphore(&mic_data_write_handle->can_process, 20 / portTICK_RATE_MS)) //portMAX_DELAY, 25 / portTICK_RATE_MS
            {
                //return -1;
                //player_log(LOG_ERR, "%s, %d, rtos_get_semaphore fail\n", __func__, __LINE__);
            }
            else
            {
                /* read mic data from dac fifo, and write to ringbuffer */
                ONBOARD_MIC_RECORD_READ_FIFO_START();
                ring_buffer_read(&mic_data_write_handle->dma_rb, mic_data_write_handle->write_buff, mic_data_write_handle->frame_size);
                ONBOARD_MIC_RECORD_READ_FIFO_END();

                if (rb_bytes_available(mic_data_write_handle->rb) >= mic_data_write_handle->frame_size)
                {
                    ONBOARD_MIC_RECORD_WRITE_RB_START();
                    int w_len = rb_write(mic_data_write_handle->rb, (char *)mic_data_write_handle->write_buff, mic_data_write_handle->frame_size, 0);
                    ONBOARD_MIC_RECORD_WRITE_RB_END();
                    if (w_len != mic_data_write_handle->frame_size)
                    {
                        LOGW("%s, %d, write mic data fail\n", __func__, __LINE__);
                        //os_memset(spk_data_read_handle->read_buff, 0, spk_data_read_handle->frame_size);
                    }
                }
                else
                {
                    /* not write mic data */
                    LOGV("%s, %d, mic data full and not write\n", __func__, __LINE__);
                    //os_memset(spk_data_read_handle->read_buff, 0, spk_data_read_handle->frame_size);
                }
            }
        }
    }

mic_data_write_exit:

    mic_data_write_handle->running = false;

    if (mic_data_write_handle->can_process)
    {
        rtos_deinit_semaphore(&mic_data_write_handle->can_process);
        mic_data_write_handle->can_process = NULL;
    }

    /* delete msg queue */
    ret = rtos_deinit_queue(&mic_data_write_handle->mic_data_write_msg_que);
    if (ret != kNoErr)
    {
        LOGE("%s, %d, delete message queue fail\n", __func__, __LINE__);
    }
    mic_data_write_handle->mic_data_write_msg_que = NULL;

    /* delete task */
    mic_data_write_handle->mic_data_write_task_hdl = NULL;

    rtos_set_semaphore(&mic_data_write_handle->sem);

    rtos_delete_thread(NULL);
}

static bk_err_t mic_data_write_task_init(onboard_mic_record_priv_t *mic_data_write_handle)
{
    bk_err_t ret = BK_OK;
    bk_err_t err = BK_OK;

    ONBOARD_MIC_RECORD_CHECK_NULL(mic_data_write_handle);

    mic_data_write_handle->write_buff = os_malloc(mic_data_write_handle->frame_size);
    ONBOARD_MIC_RECORD_CHECK_NULL(mic_data_write_handle->write_buff);

    os_memset(mic_data_write_handle->write_buff, 0, sizeof(mic_data_write_handle->frame_size));

    ret = rtos_init_semaphore(&mic_data_write_handle->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_init_semaphore(&mic_data_write_handle->can_process, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_init_queue(&mic_data_write_handle->mic_data_write_msg_que,
                          "mic_data_wt_que",
                          sizeof(mic_data_write_msg_t),
                          5);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ceate mic data write message queue fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    ret = rtos_create_thread(&mic_data_write_handle->mic_data_write_task_hdl,
                             (BEKEN_DEFAULT_WORKER_PRIORITY - 1),
                             "mic_data_wt",
                             (beken_thread_function_t)mic_data_write_task_main,
                             2048,
                             (beken_thread_arg_t)mic_data_write_handle);
    if (ret != BK_OK)
    {
        err = BK_FAIL;
        LOGE("%s, %d, create mic data write task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&mic_data_write_handle->sem, BEKEN_NEVER_TIMEOUT);

    LOGD("init mic data write task complete\n");

    return BK_OK;

fail:

    if (mic_data_write_handle->sem)
    {
        rtos_deinit_semaphore(&mic_data_write_handle->sem);
        mic_data_write_handle->sem = NULL;
    }

    if (mic_data_write_handle->can_process)
    {
        rtos_deinit_semaphore(&mic_data_write_handle->can_process);
        mic_data_write_handle->can_process = NULL;
    }

    if (mic_data_write_handle->mic_data_write_msg_que)
    {
        rtos_deinit_queue(&mic_data_write_handle->mic_data_write_msg_que);
        mic_data_write_handle->mic_data_write_msg_que = NULL;
    }

    if (mic_data_write_handle->write_buff)
    {
        os_free(mic_data_write_handle->write_buff);
        mic_data_write_handle->write_buff = NULL;
    }

    return err;
}

bk_err_t mic_data_write_task_deinit(onboard_mic_record_priv_t *mic_data_write_handle)
{
    bk_err_t ret = BK_OK;

    ONBOARD_MIC_RECORD_CHECK_NULL(mic_data_write_handle);

    LOGD("%s\n", __func__);

    ret = mic_data_write_send_msg(mic_data_write_handle->mic_data_write_msg_que, MIC_DATA_WRITE_EXIT, NULL);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, send message: MIC_DATA_WRITE_EXIT fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* abort write mic data to pipe ringbuffer.
       And handle "MIC_DATA_WRITE_EXIT" opcode.
     */
    rb_abort(mic_data_write_handle->rb);

    rtos_get_semaphore(&mic_data_write_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&mic_data_write_handle->sem);
    mic_data_write_handle->sem = NULL;

    if (mic_data_write_handle->write_buff)
    {
        os_free(mic_data_write_handle->write_buff);
    }

    LOGD("deinit mic data write complete\n");

    return BK_OK;
}


static bk_err_t aud_adc_dma_deconfig(onboard_mic_record_priv_t *onboard_mic)
{
    if (onboard_mic == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_mic->adc_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_mic->adc_dma_id);
    //bk_dma_driver_deinit();
    if (onboard_mic->dma_rb_addr)
    {
        ring_buffer_clear(&onboard_mic->dma_rb);
        os_free(onboard_mic->dma_rb_addr);
        onboard_mic->dma_rb_addr = NULL;
    }

    return BK_OK;
}


/* Carry one frame audio adc data(20ms) to ADC FIFO complete */
static void aud_adc_dma_finish_isr(void)
{
    //LOGE("%s\n", __func__);
    AUD_ADC_DMA_ISR_START();

    bk_err_t ret = rtos_set_semaphore(&gl_onboard_mic_record->can_process);
    if (ret != BK_OK)
    {
        LOGE("%s, rtos_set_semaphore fail \n", __func__);
    }
}

static bk_err_t aud_adc_dma_config(onboard_mic_record_priv_t *onboard_mic)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t adc_port_addr;
    bk_err_t err = BK_OK;

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
    onboard_mic->adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_mic->adc_dma_id < DMA_ID_0) || (onboard_mic->adc_dma_id >= DMA_ID_MAX))
    {
        LOGE("malloc dma fail \n");
        err = BK_FAIL;
        goto exit;
    }

    /* the pause address can not is the same as the end address of dma, so add 8 bytes to protect mic ring buffer. */
    onboard_mic->dma_rb_addr = (int32_t *)os_malloc(2 * onboard_mic->frame_size + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL);
    ONBOARD_MIC_RECORD_CHECK_NULL(onboard_mic->dma_rb_addr);
    ring_buffer_init(&onboard_mic->dma_rb, (uint8_t *)onboard_mic->dma_rb_addr, onboard_mic->frame_size * onboard_mic->frame_num + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL, onboard_mic->adc_dma_id, RB_DMA_TYPE_WRITE);
    LOGD("%s, %d, dma_id: %d, dma_rb_addr: %p, dma_rb_size: %d \n", __func__, __LINE__, onboard_mic->adc_dma_id, onboard_mic->dma_rb_addr, onboard_mic->frame_size * onboard_mic->frame_num + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL);
    /* init dma channel */
    os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_AUDIO_RX;
	dma_config.dst.dev = DMA_DEV_DTCM;
	switch (onboard_mic->config.nChans) {
		case 1:
			dma_config.src.width = DMA_DATA_WIDTH_16BITS;
			break;

		case 2:
			dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			break;

		default:
			break;
	}
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	/* get adc fifo address */
	if (bk_aud_adc_get_fifo_addr(&adc_port_addr) != BK_OK) {
		LOGE("%s, %d, get adc fifo address fail \n", __func__, __LINE__);
        err = BK_FAIL;
        goto exit;
	}

	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = adc_port_addr;
	dma_config.src.end_addr = adc_port_addr + 4;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)onboard_mic->dma_rb_addr;
	dma_config.dst.end_addr = (uint32_t)(onboard_mic->dma_rb_addr) + onboard_mic->frame_size * onboard_mic->frame_num + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL;
     ret = bk_dma_init(onboard_mic->adc_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, dma_init fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_mic->adc_dma_id, onboard_mic->frame_size);
#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_mic->adc_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_mic->adc_dma_id, DMA_ATTR_SEC);
#endif
    /* register dma isr */
    bk_dma_register_isr(onboard_mic->adc_dma_id, NULL, (void *)aud_adc_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_mic->adc_dma_id);

    return BK_OK;
exit:
    aud_adc_dma_deconfig(onboard_mic);
    return err;
}

static bk_err_t mic_adc_deconfig(void)
{
    return bk_aud_adc_deinit();
}


static bk_err_t mic_adc_config(onboard_mic_record_priv_t *onboard_mic)
{
    bk_err_t ret = BK_OK;
    bk_err_t err = BK_OK;

    /* init onboard mic */
    aud_adc_config_t aud_adc_cfg = DEFAULT_AUD_ADC_CONFIG();
    if (onboard_mic->config.nChans == 1)
    {
        aud_adc_cfg.adc_chl = AUD_ADC_CHL_L;
    }
    else if (onboard_mic->config.nChans == 2)
    {
        aud_adc_cfg.adc_chl = AUD_ADC_CHL_LR;
    }
    else
    {
        LOGE("adc_chl: %d is not support \n", onboard_mic->config.nChans);
        err = BK_FAIL;
        goto fail;
    }
    aud_adc_cfg.samp_rate = onboard_mic->config.sampRate;
    aud_adc_cfg.adc_gain = onboard_mic->config.adc_gain;
    aud_adc_cfg.clk_src = AUD_CLK_XTAL;
    LOGD("adc_cfg chl_num: %s, adc_gain: 0x%02x, samp_rate: %d, clk_src: %s, adc_mode: %s \n",
            aud_adc_cfg.adc_chl == AUD_ADC_CHL_L ? "AUD_ADC_CHL_L" : "AUD_ADC_CHL_LR",
            aud_adc_cfg.adc_gain,
            aud_adc_cfg.samp_rate,
            aud_adc_cfg.clk_src == 1 ? "APLL" : "XTAL",
            aud_adc_cfg.adc_mode == 1 ? "AUD_MIC_MODE_SIGNAL_END" : "AUD_MIC_MODE_DIFFEN");
    ret = bk_aud_adc_init(&aud_adc_cfg);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, aud_adc_init fail\n", __func__, __LINE__);
        err = BK_FAIL;
        goto fail;
    }

    return BK_OK;

fail:

    mic_adc_deconfig();

    return err;
}


static bk_err_t onboard_mic_open(onboard_mic_record_priv_t *onboard_mic)
{
    bk_err_t ret = BK_OK;

    ONBOARD_MIC_RECORD_CHECK_NULL(onboard_mic);

    ret = mic_adc_config(onboard_mic);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, mic_adc_config fail\n", __func__, __LINE__);
        goto onboard_mic_open_fail;
    }

    ret = aud_adc_dma_config(onboard_mic);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, aud_adc_dma_config fail\n", __func__, __LINE__);
        goto onboard_mic_open_fail;
    }

    /* init ringbuffer */
    onboard_mic->rb = rb_create(onboard_mic->rb_size);
    if (!onboard_mic->rb)
    {
        LOGE("%s, %d, create ringbuffer fail\n", __func__, __LINE__);
        goto onboard_mic_open_fail;
    }

    ret = mic_data_write_task_init(onboard_mic);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, mic_data_write_task_init fail\n", __func__, __LINE__);
        goto onboard_mic_open_fail;
    }

    return BK_OK;

onboard_mic_open_fail:

    return BK_FAIL;
}

static bk_err_t onboard_mic_start(onboard_mic_record_priv_t *onboard_mic)
{
    bk_err_t ret = BK_OK;

    ONBOARD_MIC_RECORD_CHECK_NULL(onboard_mic);

    ret = mic_data_write_send_msg(onboard_mic->mic_data_write_msg_que, MIC_DATA_WRITE_START, NULL);

    ret = bk_dma_start(onboard_mic->adc_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_adc_start();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc stop fail\n", __func__, __LINE__);
    }

    return ret;
}

static bk_err_t onboard_mic_stop(onboard_mic_record_priv_t *onboard_mic)
{
    LOGD("%s \n", __func__);

    bk_err_t ret = bk_dma_stop(onboard_mic->adc_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_adc_stop();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc stop fail\n", __func__, __LINE__);
    }

    ret = mic_data_write_send_msg(onboard_mic->mic_data_write_msg_que, MIC_DATA_WRITE_IDLE, NULL);

    return ret;
}

static bk_err_t onboard_mic_close(onboard_mic_record_priv_t *onboard_mic)
{
    LOGD("%s \n", __func__);

    ONBOARD_MIC_RECORD_CHECK_NULL(onboard_mic);

    mic_data_write_task_deinit(onboard_mic);

    bk_err_t ret = bk_dma_stop(onboard_mic->adc_dma_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc dma stop fail\n", __func__, __LINE__);
    }

    ret = bk_aud_adc_stop();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, adc stop fail\n", __func__, __LINE__);
    }

    mic_adc_deconfig();

    aud_adc_dma_deconfig(onboard_mic);

    /* deinit ringbuffer */
    rb_destroy(onboard_mic->rb);

    return BK_OK;
}

static bk_err_t onboard_mic_set_adc_gain(int value)
{
    LOGD("%s \n", __func__);

    return bk_aud_adc_set_gain(value);
}

static int onboard_mic_record_open(audio_record_t *record, audio_record_cfg_t *config)
{
    onboard_mic_record_priv_t *temp_onboard_mic_record = NULL;
    int ret;

    if (!record || !config)
    {
        LOGE("%s, %d, params error, play: %p, param: %p\n", __func__, __LINE__, record, config);
        return BK_FAIL;
    }

    onboard_mic_record_priv_t *priv = (onboard_mic_record_priv_t *)record->record_ctx;
    if (priv != NULL)
    {
        LOGE("%s, %d, onboard_mic_record: %p already open\n", __func__, __LINE__, priv);
        return BK_FAIL;
    }

    temp_onboard_mic_record = os_malloc(sizeof(onboard_mic_record_priv_t));
    if (!temp_onboard_mic_record)
    {
        LOGE("%s, %d, os_malloc temp_onboard_mic_record: %d fail\n", __func__, __LINE__, sizeof(onboard_mic_record_priv_t));
        return BK_FAIL;
    }

    record->record_ctx = temp_onboard_mic_record;

    gl_onboard_mic_record = temp_onboard_mic_record;

    temp_onboard_mic_record->frame_size = config->frame_size;
    temp_onboard_mic_record->frame_num = DMA_CARRY_MIC_FRAME_NUM;
    temp_onboard_mic_record->rb_size = config->pool_size;

    temp_onboard_mic_record->config.port = config->port;
    temp_onboard_mic_record->config.nChans = config->nChans;
    temp_onboard_mic_record->config.sampRate = config->sampRate;
    temp_onboard_mic_record->config.bitsPerSample = config->bitsPerSample;
    temp_onboard_mic_record->config.adc_gain = config->adc_gain;
    temp_onboard_mic_record->config.mic_mode = config->mic_mode;
    temp_onboard_mic_record->config.frame_size = config->frame_size;
    temp_onboard_mic_record->config.pool_size = config->pool_size;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    ret = onboard_mic_open(temp_onboard_mic_record);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard mic open fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return ret;
    }

    ret = onboard_mic_start(temp_onboard_mic_record);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard mic start fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return ret;
    }

    onboard_mic_state_set(temp_onboard_mic_record, AUDIO_RECORD_STA_RUNNING);

    AUD_MIC_DATA_DUMP_OPEN();

    LOGD("onboard mic open complete \n");

    return BK_OK;
}

static int onboard_mic_record_close(audio_record_t *record)
{
    bk_err_t ret = BK_OK;

    ONBOARD_MIC_RECORD_CHECK_NULL(record);
    onboard_mic_record_priv_t *priv = (onboard_mic_record_priv_t *)record->record_ctx;
    ONBOARD_MIC_RECORD_CHECK_NULL(priv);

    if (onboard_mic_state_get(priv) == AUDIO_RECORD_STA_IDLE)
    {
        return BK_OK;
    }

    LOGD("%s \n", __func__);

    ret = onboard_mic_stop(priv);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard mic stop fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    ret = onboard_mic_close(priv);
    if (ret != BK_OK)
    {
        LOGE("%s, onboard mic close fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    onboard_mic_state_set(priv, AUDIO_RECORD_STA_IDLE);

    AUD_MIC_DATA_DUMP_CLOSE();

    LOGV("onboard mic close complete\n");
    return BK_OK;
}

static int onboard_mic_record_read(audio_record_t *record, char *buffer, uint32_t len)
{
    if (!record || !buffer || !len)
    {
        LOGE("%s, %d, params error, record: %p, buffer: %p, len: %d\n", __func__, __LINE__, record, buffer, len);
        return BK_FAIL;
    }

    onboard_mic_record_priv_t *priv = (onboard_mic_record_priv_t *)record->record_ctx;
    ONBOARD_MIC_RECORD_CHECK_NULL(priv);

    int need_r_len = len;

    ONBOARD_MIC_RECORD_READ_RB_START();

    int r_len = rb_read(priv->rb, buffer, need_r_len, BEKEN_WAIT_FOREVER);
    if (r_len < 0)
    {
        LOGE("rb_read FAIL, r_len: %d \n", r_len);
        return BK_FAIL;
    }

    AUD_MIC_DATA_DUMP_DATA(buffer, r_len);

    ONBOARD_MIC_RECORD_READ_RB_END();

    return r_len;
}


static int onboard_mic_record_control(audio_record_t *record, audio_record_ctl_t ctl)
{
    int ret = BK_OK;

    onboard_mic_record_priv_t *priv = (onboard_mic_record_priv_t *)record->record_ctx;
    ONBOARD_MIC_RECORD_CHECK_NULL(priv);

    if (ctl == AUDIO_RECORD_PAUSE)
    {
        if (onboard_mic_state_get(priv) == AUDIO_RECORD_STA_PAUSED)
        {
            return BK_OK;
        }

        ret = onboard_mic_stop(priv);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard mic record pause fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        onboard_mic_state_set(priv, AUDIO_RECORD_STA_PAUSED);
    }
    else if (ctl == AUDIO_RECORD_RESUME)
    {
        if (onboard_mic_state_get(priv) == AUDIO_RECORD_STA_RUNNING)
        {
            return BK_OK;
        }

        ret = onboard_mic_start(priv);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard mic record start fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        onboard_mic_state_set(priv, AUDIO_RECORD_STA_RUNNING);
    }
    else if (ctl == AUDIO_RECORD_SET_ADC_GAIN)
    {
        ret = onboard_mic_set_adc_gain(record->config.adc_gain);
        if (ret != BK_OK)
        {
            LOGE("%s, onboard mic set adc gain fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else
    {
        //nothing to do
    }

    return ret;
}

audio_record_ops_t onboard_mic_record_ops =
{
    .open =    onboard_mic_record_open,
    .read =    onboard_mic_record_read,
    .control = onboard_mic_record_control,
    .close =   onboard_mic_record_close,
};

audio_record_ops_t *get_onboard_mic_record_ops(void)
{
    return &onboard_mic_record_ops;
}

