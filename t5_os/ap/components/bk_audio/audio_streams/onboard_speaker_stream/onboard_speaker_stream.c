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
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <timers.h>
#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/aud_dac.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/gpio.h>
#include "gpio_driver.h"


#define TAG  "ONBOARD_SPEAKER"

//#define ONBOARD_SPK_DEBUG   //GPIO debug

#ifdef ONBOARD_SPK_DEBUG

#define AUD_DAC_DMA_ISR_START()                 do { GPIO_DOWN(4); GPIO_UP(4);} while (0)
#define AUD_DAC_DMA_ISR_END()                   do { GPIO_DOWN(4); } while (0)

#define AUD_ONBOARD_SPK_PROCESS_START()         do { GPIO_DOWN(5); GPIO_UP(5);} while (0)
#define AUD_ONBOARD_SPK_PROCESS_END()           do { GPIO_DOWN(5); } while (0)

#define AUD_ONBOARD_SPK_INPUT_START()           do { GPIO_DOWN(8); GPIO_UP(8);} while (0)
#define AUD_ONBOARD_SPK_INPUT_END()             do { GPIO_DOWN(8); } while (0)

#define AUD_ONBOARD_SPK_OUTPUT_START()          do { GPIO_DOWN(9); GPIO_UP(9);} while (0)
#define AUD_ONBOARD_SPK_OUTPUT_END()            do { GPIO_DOWN(9); } while (0)

#else

#define AUD_DAC_DMA_ISR_START()
#define AUD_DAC_DMA_ISR_END()

#define AUD_ONBOARD_SPK_PROCESS_START()
#define AUD_ONBOARD_SPK_PROCESS_END()

#define AUD_ONBOARD_SPK_INPUT_START()
#define AUD_ONBOARD_SPK_INPUT_END()

#define AUD_ONBOARD_SPK_OUTPUT_START()
#define AUD_ONBOARD_SPK_OUTPUT_END()

#endif

/* onboard speaker data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count onboard speaker data. */
#if CONFIG_ADK_UTILS

#define ONBOARD_SPK_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef ONBOARD_SPK_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t onboard_spk_count_util = {0};
#define ONBOARD_SPK_DATA_COUNT_INTERVAL     (1000 * 4)
#define ONBOARD_SPK_DATA_COUNT_TAG          "ONBOARD_SPK"

#define ONBOARD_SPK_DATA_COUNT_OPEN()               count_util_create(&onboard_spk_count_util, ONBOARD_SPK_DATA_COUNT_INTERVAL, ONBOARD_SPK_DATA_COUNT_TAG)
#define ONBOARD_SPK_DATA_COUNT_CLOSE()              count_util_destroy(&onboard_spk_count_util)
#define ONBOARD_SPK_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&onboard_spk_count_util, size)

#else

#define ONBOARD_SPK_DATA_COUNT_OPEN()
#define ONBOARD_SPK_DATA_COUNT_CLOSE()
#define ONBOARD_SPK_DATA_COUNT_ADD_SIZE(size)

#endif  //ONBOARD_SPK_DATA_COUNT

/* dump onboard_spk stream play pcm data by uart */
//#define ONBOARD_SPK_DATA_DUMP_BY_UART

#ifdef ONBOARD_SPK_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_ob_spk_uart_util = {0};
#define ONBOARD_SPK_DATA_DUMP_UART_ID            (1)
#define ONBOARD_SPK_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_ob_spk_uart_util, ONBOARD_SPK_DATA_DUMP_UART_ID, ONBOARD_SPK_DATA_DUMP_UART_BAUD_RATE)
#define ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_ob_spk_uart_util)
#define ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_ob_spk_uart_util, data_buf, len)

#else

#define ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN()
#define ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE()
#define ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //ONBOARD_MIC_DATA_DUMP_BY_UART


#define DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL    (8)


//#define SPK_DATA_DEBUG

#ifdef SPK_DATA_DEBUG
static const uint32_t PCM_8000[] = {
	0x00010000, 0x5A825A81, 0x7FFF7FFF, 0x5A825A83, 0x00000000, 0xA57FA57E, 0x80018002, 0xA57EA57E,
};
#endif

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
typedef struct input_audio_port_info_item
{
    STAILQ_ENTRY(input_audio_port_info_item)    next;
    audio_port_info_t                           port_info;
} input_audio_port_info_item_t;

typedef STAILQ_HEAD(input_audio_port_info_list, input_audio_port_info_item) input_audio_port_info_list_t;
#endif

typedef struct onboard_speaker_stream
{
    uint8_t                  chl_num;                       /**< speaker channel number */
    uint32_t                 sample_rate;                   /**< speaker sample rate */
    int32_t                  dig_gain;                      /**< audio dac digital gain: value range: , suggest: */
    int32_t                  ana_gain;                      /**< audio dac analog gain: value range: , suggest: */
    aud_dac_work_mode_t      work_mode;                     /**< audio dac mode: signal_ended/differen */
    uint8_t                  bits;                          /**< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t                clk_src;                       /**< audio clock: XTAL(26MHz)/APLL */
    bool                     is_open;                       /**< speaker enable, true: enable, false: disable */
    uint32_t                 frame_size;                    /**< size of one frame speaker data, the size
                                                                        when AUD_DAC_CHL_L_ENABLE mode, the size must bean integer multiple of two bytes
                                                                        when AUD_DAC_CHL_LR_ENABLE mode, the size must bean integer multiple of four bytes */
    dma_id_t                 spk_dma_id;                    /**< dma id that dma carry spk data from ring buffer to fifo */
    RingBufferContext        spk_rb;                        /**< speaker rb handle */
    int8_t                  *spk_ring_buff;                 /**< speaker ring buffer addr */
    uint32_t                 pool_length;                   /**< speaker data pool size, the unit is byte */
    uint32_t                 pool_play_thold;               /**< the play threshold of pool, the unit is byte */
    uint32_t                 pool_pause_thold;              /**< the pause threshold of pool, the unit is byte */
    RingBufferContext        pool_rb;                       /**< the pool ringbuffer handle */
    int8_t                  *pool_ring_buff;                /**< pool ring buffer addr */
    bool                     pool_can_read;                 /**< the pool if can read */
    beken_semaphore_t        can_process;                   /**< can process */
    int8_t                  *temp_buff;                     /**< temp buffer addr used to save data written to speaker ring buffer */
    bool                     wr_spk_rb_done;                /**< write one farme data to speaker ring buffer done */
    uint8_t                  valid_frame_count_in_spk_rb;   /**< the count of valid farme data in speaker ring buffer, data playback finish when the count is 0 */

    bool                     pa_ctrl_en;                    /**< control pa enable */
    uint16_t                 pa_ctrl_gpio;                  /**< the gpio id of control pa */
    uint8_t                  pa_on_level;                   /**< the gpio level of turn on pa, 0: low level, 1: high level */
    uint32_t                 pa_on_delay;                   /**< the delay time(ms) of turn on pa after enable audio dac. [dac init -> pa turn on] */
    uint32_t                 pa_off_delay;                  /**< the delay time(ms) of disable audio dac after turn off pa. [mute -> pa turn off -> dac deinit] */
    TimerHandle_t            pa_turn_on_timer;              /**< the timer handle of turn on pa */
    bool                     pa_state;                      /**< the state of pa, true: on, false: off */

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    int                             current_port_id;        /**< the valid audio port of currently reading speaker data, 0: element->in, >=1: element->multi_in */
    SemaphoreHandle_t               lock;                   /**< input audio port info list lock */
    input_audio_port_info_list_t    input_port_list;        /**< the list of input audio port info */
#endif
} onboard_speaker_stream_t;

static onboard_speaker_stream_t *gl_onboard_speaker = NULL;

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
#define input_port_list_release(handle) xSemaphoreGive(handle)
#define input_port_list_block(handle, time) xSemaphoreTake(handle, time)
#endif

//#define AEC_MIC_DELAY_POINTS_DEBUG

static bk_err_t _onboard_speaker_close(audio_element_handle_t self);

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
//#define PORT_LIST_DEBUG
#ifdef PORT_LIST_DEBUG
#define INPUT_PORT_LIST_DEBUG(list_ptr, func, line)  _debug_print_input_port_list(list_ptr, func, line)
#else
#define INPUT_PORT_LIST_DEBUG(list_ptr, func, line)
#endif

static void _debug_print_input_port_list(input_audio_port_info_list_t *input_port_list, const char *func, int line)
{
    if (input_port_list == NULL)
    {
        BK_LOGD(TAG, "input_port_list is NULL\n");
        return;
    }

    input_audio_port_info_item_t *audio_port_info_item = NULL;

    BK_LOGD(TAG, "----------------- [%s] %d, input_port_list -----------------\n", func, line);
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item)
        {
            BK_LOGD(TAG, "port_id: %d, priority: %d, port: %p, chl_num: %d, sample_rate: %d, dig_gain: %d, ana_gain: %d, bits: %d\n",
                    audio_port_info_item->port_info.port_id,
                    audio_port_info_item->port_info.priority,
                    audio_port_info_item->port_info.port,
                    audio_port_info_item->port_info.chl_num,
                    audio_port_info_item->port_info.sample_rate,
                    audio_port_info_item->port_info.dig_gain,
                    audio_port_info_item->port_info.ana_gain,
                    audio_port_info_item->port_info.bits);
        }
    }
    BK_LOGD(TAG, "\n");

}

/* Traverse the input_audio_port_info_list according to port priority to obtain the high-priority port id with valid data */
static int8_t _get_valid_port_id(input_audio_port_info_list_t *input_port_list)
{
    int8_t valid_port_id = -1;
    input_audio_port_info_item_t *audio_port_info_item = NULL;

    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port)
        {
            uint32_t filled_size = audio_port_get_filled_size(audio_port_info_item->port_info.port);
            if (filled_size > 0)
            {
                valid_port_id = audio_port_info_item->port_info.port_id;
                break;
            }
        }
    }

    return valid_port_id;
}

static audio_port_info_t *_get_audio_port_info_by_port_id(input_audio_port_info_list_t *input_port_list, uint8_t input_poit_id)
{
    audio_port_info_t *port_info = NULL;
    input_audio_port_info_item_t *audio_port_info_item = NULL;

    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port_id == input_poit_id)
        {
            port_info = &audio_port_info_item->port_info;
            break;
        }
    }

    return port_info;
}

static bk_err_t _add_audio_port_to_list(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    input_audio_port_info_item_t *prev_audio_port_info_item = NULL;

    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority >= port_info->priority)
        {
            prev_audio_port_info_item = audio_port_info_item;
        }
    }

    input_audio_port_info_item_t *new_audio_port_info_item = audio_calloc(1, sizeof(input_audio_port_info_item_t));
    AUDIO_MEM_CHECK(TAG, new_audio_port_info_item, return BK_FAIL);
    os_memcpy(&new_audio_port_info_item->port_info, port_info, sizeof(audio_port_info_t));

    if (prev_audio_port_info_item)
    {
        STAILQ_INSERT_AFTER(input_port_list, prev_audio_port_info_item, new_audio_port_info_item, next);
    }
    else
    {
        STAILQ_INSERT_HEAD(input_port_list, new_audio_port_info_item, next);
    }

    return BK_OK;
}

static bk_err_t _update_audio_port_to_list(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    input_audio_port_info_item_t *tmp_item = NULL;
    input_audio_port_info_item_t *prev_audio_port_info_item = NULL;
    input_audio_port_info_item_t *audio_port_info_item_bk = NULL;

    STAILQ_FOREACH_SAFE(audio_port_info_item, input_port_list, next, tmp_item)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port_id == port_info->port_id)
        {
            STAILQ_REMOVE(input_port_list, audio_port_info_item, input_audio_port_info_item, next);
            audio_port_info_item_bk = audio_port_info_item;
            break;
        }
    }

    /* if port_info->port is NULL, remove this port from list */
    if (port_info->port == NULL)
    {
        audio_free(audio_port_info_item_bk);
        return BK_OK;
    }

    os_memcpy(&audio_port_info_item_bk->port_info, port_info, sizeof(audio_port_info_t));

    audio_port_info_item = NULL;
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority >= port_info->priority)
        {
            prev_audio_port_info_item = audio_port_info_item;
        }
    }

    if (prev_audio_port_info_item)
    {
        STAILQ_INSERT_AFTER(input_port_list, prev_audio_port_info_item, audio_port_info_item_bk, next);
    }
    else
    {
        STAILQ_INSERT_HEAD(input_port_list, audio_port_info_item_bk, next);
    }

    return BK_OK;
}
#endif

#ifdef AEC_MIC_DELAY_POINTS_DEBUG
static void aec_mic_delay_debug(int16_t *data, uint32_t size)
{
    static uint32_t mic_delay_num = 0;
    mic_delay_num++;
    os_memset(data, 0, size);
    if (mic_delay_num == 50)
    {
        data[0] = 0x2FFF;
        mic_delay_num = 0;
        BK_LOGD(TAG, "AEC_MIC_DELAY_POINTS_DEBUG \n");
    }
}
#endif

#ifdef SPK_DATA_DEBUG
void change_pcm_data_to_8k(uint8_t* buffer, uint32_t size)
{
    for (uint32_t i = 0; i < (size/sizeof(PCM_8000)); i++)
    {
        os_memcpy(&buffer[i * sizeof(PCM_8000)], PCM_8000, sizeof(PCM_8000));
    }
}
#endif

/* PA control gpio */
static void _pa_gpio_ctrl(uint16_t pa_ctrl_gpio, uint8_t pa_on_level, bool en)
{
    if (en)
    {
        BK_LOGD(TAG, "%s, %d, PA turn on \n", __func__, __LINE__);
        /* open pa according to congfig */
        if (pa_on_level)
        {
            bk_gpio_set_output_high(pa_ctrl_gpio);
        }
        else
        {
            bk_gpio_set_output_low(pa_ctrl_gpio);
        }
    }
    else
    {
        BK_LOGD(TAG, "%s, %d, PA turn off \n", __func__, __LINE__);
        if (pa_on_level)
        {
            bk_gpio_set_output_low(pa_ctrl_gpio);
        }
        else
        {
            bk_gpio_set_output_high(pa_ctrl_gpio);
        }
    }
}

/*
 * @brief: pa control api
 * @param: onboard_spk: speaker stream
 * @param: en: true: turn on, false: turn off
 * @param: delay_flag: true: delay turn on, false: no delay
 * @return: none
 */
static void pa_ctrl_en(onboard_speaker_stream_t *onboard_spk, uint8_t en, bool delay_flag)
{
    if (!onboard_spk->pa_ctrl_en)
    {
        return;
    }

    if (en)
    {
        if (onboard_spk->pa_state)
        {
            /* pa already turn on */
            return;
        }
        else
        {
            if (onboard_spk->pa_turn_on_timer && delay_flag)
            {
                if (xTimerIsTimerActive(onboard_spk->pa_turn_on_timer))
                {
                    xTimerReset(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
                }
                else
                {
                    BK_LOGD(TAG, "start pa_turn_on_timer, pa_on_delay: %d\n", onboard_spk->pa_on_delay);
                    xTimerStart(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
                }
            }
            else
            {
                /* not need delay */
                _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, true);
                if (onboard_spk->dig_gain > 0)
                {
                    bk_aud_dac_unmute();
                }
                onboard_spk->pa_state = true;
            }
        }
    }
    else
    {
        if (onboard_spk->pa_turn_on_timer)
        {
            if (xTimerIsTimerActive(onboard_spk->pa_turn_on_timer))
            {
                xTimerStop(onboard_spk->pa_turn_on_timer, portMAX_DELAY);
            }
        }

        if (!onboard_spk->pa_state)
        {
            /* pa already turn off */
            return;
        }

        /* mute -> turn off pa */
        bk_aud_dac_mute();
        if (onboard_spk->pa_off_delay)
        {
            rtos_delay_milliseconds(onboard_spk->pa_off_delay);
        }
        _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, false);
        onboard_spk->pa_state = false;
    }
}

/* PA turn on callback */
static void pa_turn_on_timer_callback(TimerHandle_t xTimer)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)pvTimerGetTimerID(xTimer);

    /* turn on pa according to congfig */
    _pa_gpio_ctrl(onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level, true);

    if (onboard_spk->dig_gain > 0)
    {
        bk_aud_dac_unmute();
    }

    onboard_spk->pa_state = true;
    BK_LOGD(TAG, "turn on pa complete, pa_ctrl_gpio: %d, pa_on_level: %d\n", onboard_spk->pa_ctrl_gpio, onboard_spk->pa_on_level);
}

static bk_err_t aud_dac_dma_deconfig(onboard_speaker_stream_t *onboard_spk)
{
    if (onboard_spk == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_spk->spk_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_spk->spk_dma_id);
    //bk_dma_driver_deinit();
    if (onboard_spk->spk_ring_buff)
    {
        ring_buffer_clear(&onboard_spk->spk_rb);
        audio_dma_mem_free(onboard_spk->spk_ring_buff);
        onboard_spk->spk_ring_buff = NULL;
    }

    return BK_OK;
}


/* Carry one frame audio dac data(20ms) to DAC FIFO complete */
static void aud_dac_dma_finish_isr(void)
{
    AUD_DAC_DMA_ISR_START();
    //BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = rtos_set_semaphore(&gl_onboard_speaker->can_process);
    if (ret != BK_OK)
    {
        BK_LOGV(TAG, "%s, rtos_set_semaphore fail \n", __func__);
#if 0
        /* write data to speaker ring buffer immediately */
        if (onboard_spk->pool_can_read)
        {
            uint32_t read_size = ring_buffer_read(&onboard_spk->pool_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
            if (read_size != onboard_spk->frame_size)
            {
                BK_LOGE(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, onboard_spk->frame_size);
            }
        }
        else
        {
            os_memset(onboard_spk->temp_buff, 0x00, onboard_spk->frame_size);
            BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
        }

        //  addAON_GPIO_Reg0x9 = 2;
        ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
        //  addAON_GPIO_Reg0x9 = 0;
#endif
    }
    gl_onboard_speaker->wr_spk_rb_done = false;
    AUD_DAC_DMA_ISR_END();
}

static bk_err_t aud_dac_dma_config(onboard_speaker_stream_t *onboard_spk)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dac_port_addr;

#if 0
    /* init dma driver */
    ret = bk_dma_driver_init();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_driver_init fail\n", __func__, __LINE__);
        goto exit;
    }
#endif

    //malloc dma channel
    onboard_spk->spk_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_spk->spk_dma_id < DMA_ID_0) || (onboard_spk->spk_dma_id >= DMA_ID_MAX))
    {
        BK_LOGE(TAG, "malloc dma fail \n");
        goto exit;
    }

    /* init two frames ringbuffer */
    /* the pause address can not is the same as the end address of dma, so add 8 bytes to protect speaker ring buffer. */
    onboard_spk->spk_ring_buff = (int8_t *)audio_dma_mem_calloc(2, onboard_spk->frame_size + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL/2);
    AUDIO_MEM_CHECK(TAG, onboard_spk->spk_ring_buff, return BK_FAIL);
    ring_buffer_init(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->spk_ring_buff, onboard_spk->frame_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL, onboard_spk->spk_dma_id, RB_DMA_TYPE_READ);
    BK_LOGD(TAG, "%s, %d, spk_ring_buff: %p, spk_ring_buff size: %d \n", __func__, __LINE__, onboard_spk->spk_ring_buff, onboard_spk->frame_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
    /* init dma channel */
    os_memset(&dma_config, 0, sizeof(dma_config_t));
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.dst.dev = DMA_DEV_AUDIO;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    switch (onboard_spk->chl_num)
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
        BK_LOGE(TAG, "%s, %d, get dac fifo address fail\n", __func__, __LINE__);
        goto exit;
    }
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = dac_port_addr;
    dma_config.dst.end_addr = dac_port_addr + 4;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = (uint32_t)(uintptr_t)onboard_spk->spk_ring_buff;
    dma_config.src.end_addr = (uint32_t)(uintptr_t)(onboard_spk->spk_ring_buff) + onboard_spk->frame_size * 2 + DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL;
    ret = bk_dma_init(onboard_spk->spk_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_spk->spk_dma_id, onboard_spk->frame_size);
#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_spk->spk_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_spk->spk_dma_id, DMA_ATTR_SEC);
#endif
    /* register dma isr */
    bk_dma_register_isr(onboard_spk->spk_dma_id, NULL, (void *)aud_dac_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_spk->spk_dma_id);

    return BK_OK;
exit:
    aud_dac_dma_deconfig(onboard_spk);
    return BK_FAIL;
}

static bk_err_t _onboard_speaker_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_open \n", audio_element_get_tag(self));
    uint32_t free_size = 0;

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);

    if (onboard_spk->is_open)
    {
        return BK_OK;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (audio_element_get_multi_input_max_port_num(self) > 0)
    {
        audio_port_info_t *port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, 0);
        if (port_info == NULL)
        {
            input_audio_port_info_item_t *port_info_item = audio_calloc(1, sizeof(input_audio_port_info_item_t));
            if (port_info_item == NULL)
            {
                BK_LOGE(TAG, "[%s] malloc intput audio port info item fail \n", audio_element_get_tag(self));
                return BK_FAIL;
            }
            port_info_item->port_info.sample_rate = onboard_spk->sample_rate;
            port_info_item->port_info.chl_num = onboard_spk->chl_num;
            port_info_item->port_info.ana_gain = onboard_spk->ana_gain;
            port_info_item->port_info.dig_gain = onboard_spk->dig_gain;
            port_info_item->port_info.bits = onboard_spk->bits;
            port_info_item->port_info.port_id = 0;
            port_info_item->port_info.priority = 0;
            port_info_item->port_info.port = audio_element_get_input_port(self);
            STAILQ_INSERT_TAIL(&gl_onboard_speaker->input_port_list, port_info_item, next);
        }
    }
#endif

    /* set read data timeout */
    audio_element_set_input_timeout(self, 0);   // 2000, 15 / portTICK_RATE_MS

    if (gl_onboard_speaker->pool_ring_buff)
    {
        free_size = ring_buffer_get_free_size(&gl_onboard_speaker->pool_rb);
        if (free_size)
        {
            uint8_t *temp_data = (uint8_t *)audio_malloc(free_size - gl_onboard_speaker->frame_size);
            AUDIO_MEM_CHECK(TAG, temp_data, return BK_FAIL);
            os_memset(temp_data, 0x00, free_size - gl_onboard_speaker->frame_size);
            ring_buffer_write(&gl_onboard_speaker->pool_rb, temp_data, free_size - gl_onboard_speaker->frame_size);
            audio_free(temp_data);
            temp_data = NULL;
        }
    }

    free_size = ring_buffer_get_free_size(&gl_onboard_speaker->spk_rb);
    if (free_size)
    {
        uint8_t *temp_data = (uint8_t *)audio_malloc(free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
        AUDIO_MEM_CHECK(TAG, temp_data, return BK_FAIL);
        os_memset(temp_data, 0x00, free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
        ring_buffer_write(&gl_onboard_speaker->spk_rb, temp_data, free_size - DMA_CARRY_SPK_RINGBUF_SAFE_INTERVAL);
        audio_free(temp_data);
        temp_data = NULL;
    }

    bk_err_t ret = bk_dma_start(onboard_spk->spk_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (gl_onboard_speaker->pa_ctrl_en)
    {
        bk_aud_dac_mute();
    }

	ret = bk_aud_dac_start();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    onboard_spk->is_open = true;
    onboard_spk->pool_can_read = true;
    onboard_spk->valid_frame_count_in_spk_rb = 2;

    /* turn on pa */
    pa_ctrl_en(onboard_spk, true, true);

    return BK_OK;
}

static int _onboard_speaker_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] _onboard_speaker_write, len: %d \n", audio_element_get_tag(el), len);

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(el);
    int ret = BK_OK;
    uint32_t write_size = 0;

    if (len)
    {
        //write some data to speaker pool
        if (onboard_spk->pool_ring_buff)
        {
            if (ring_buffer_get_free_size(&onboard_spk->pool_rb) >= len)
            {
                //BK_LOGV(TAG, "[%s] _onboard_speaker_write, pool_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_spk->pool_rb));
#ifdef SPK_DATA_DEBUG
                change_pcm_data_to_8k((uint8_t *)buffer, len);
#endif
                write_size = ring_buffer_write(&onboard_spk->pool_rb, (uint8_t *)buffer, len);
                if (write_size == len)
                {
                    ret = write_size;
                }
                else
                {
                    BK_LOGE(TAG, "The error is happened in writing data. write_size: %d \n", write_size);
                    ret = -1;
                }
                //BK_LOGV(TAG, "[%s] _onboard_speaker_write, pool_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_spk->pool_rb));
            }
        }
    }
    else
    {
        ret = len;
    }

    if (onboard_spk->pool_ring_buff)
    {
        /* check pool pause threshold */
        if (onboard_spk->pool_can_read)
        {
            if (ring_buffer_get_fill_size(&onboard_spk->pool_rb) <= onboard_spk->pool_pause_thold)
            {
                BK_LOGE(TAG, "pause pool read, pool_fill: %d <= %d \n", ring_buffer_get_fill_size(&onboard_spk->pool_rb), onboard_spk->pool_pause_thold);
                onboard_spk->pool_can_read = false;
            }
        }
        else
        {
            if (ring_buffer_get_fill_size(&onboard_spk->pool_rb) >= onboard_spk->pool_play_thold)
            {
                BK_LOGE(TAG, "start pool read \n");
                onboard_spk->pool_can_read = true;
            }
        }
    }

    //BK_LOGD(TAG, "%s, ret: %d\n", __func__, ret);
    return ret;
}

static bk_err_t audio_dac_reconfig(onboard_speaker_stream_t *onboard_spk, int rate, int ch, int bits)
{
    /* check and set sample rate */
    if (onboard_spk->sample_rate != rate)
    {
        if (BK_OK != bk_aud_dac_set_samp_rate(rate))
        {
            BK_LOGE(TAG, "%s, line: %d, updata onboard speaker sample rate: %d fail \n", __func__, __LINE__, rate);
            return BK_FAIL;
        }
        else
        {
            BK_LOGD(TAG, "%s, line: %d, updata onboard speaker sample rate: %d ok \n", __func__, __LINE__, rate);
        }
    }

    /* check and set channel num */
    if (onboard_spk->chl_num != ch)
    {
        aud_dac_chl_t chl_cfg = AUD_DAC_CHL_L;
        if (ch == 1)
        {
            chl_cfg = AUD_DAC_CHL_L;
        }
        else
        {
            chl_cfg = AUD_DAC_CHL_LR;
        }
        if (BK_OK != bk_aud_dac_set_chl(chl_cfg))
        {
            BK_LOGE(TAG, "%s, line: %d, updata onboard speaker channel: %d fail \n", __func__, __LINE__, ch);
            return BK_FAIL;
        }
        else
        {
            BK_LOGD(TAG, "%s, line: %d, updata onboard speaker channel: %d ok \n", __func__, __LINE__, ch);
        }

        //TODO
        //set dest_data_width 16bit or 32bit
        //lack dma set api
        /*
                aud_dac_dma_deconfig(onboard_spk);
                if (BK_OK != aud_dac_dma_config(onboard_spk)) {
                    BK_LOGE(TAG, "%s, line: %d, audio_dac_dma_reconfig fail \n", __func__, __LINE__);
                    return BK_FAIL;
                }
        */
    }

    return BK_OK;
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
/* Check whether audio dac configuration need to be updated */
static bk_err_t _update_dac_config(audio_element_handle_t onboard_speaker_stream, uint8_t current_port_id, uint8_t new_port_id)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    audio_port_info_t *current_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, current_port_id);
    audio_port_info_t *new_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, new_port_id);

    if (new_port_info)
    {
        if (!current_port_info || current_port_info->sample_rate != new_port_info->sample_rate)
        {
            if (BK_OK != bk_aud_dac_set_samp_rate(new_port_info->sample_rate))
            {
                BK_LOGE(TAG, "%s, line: %d, updata onboard speaker sample rate: %d fail \n", __func__, __LINE__, new_port_info->sample_rate);
            }
            else
            {
                BK_LOGD(TAG, "%s, line: %d, updata onboard speaker sample rate: %d->%d ok \n", __func__, __LINE__, current_port_info ? current_port_info->sample_rate : -1, new_port_info->sample_rate);
            }
        }

        if (!current_port_info || current_port_info->chl_num != new_port_info->chl_num)
        {
            aud_dac_chl_t chl_cfg = AUD_DAC_CHL_L;
            if (new_port_info->chl_num == 1)
            {
                chl_cfg = AUD_DAC_CHL_L;
            }
            else
            {
                chl_cfg = AUD_DAC_CHL_LR;
            }
            if (BK_OK != bk_aud_dac_set_chl(chl_cfg))
            {
                BK_LOGE(TAG, "%s, line: %d, updata onboard speaker channel: %d fail \n", __func__, __LINE__, new_port_info->chl_num);
            }
            else
            {
                BK_LOGD(TAG, "%s, line: %d, updata onboard speaker channel: %d->%d ok \n", __func__, __LINE__, current_port_info ? current_port_info->chl_num : -1, new_port_info->chl_num);
            }
        }

        if (!current_port_info || current_port_info->bits != new_port_info->bits)
        {
            //TODO
            BK_LOGD(TAG, "%s, line: %d, updata onboard speaker bits: %d->%d ok \n", __func__, __LINE__, current_port_info ? current_port_info->bits : -1, new_port_info->bits);
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, new_port_id: %d is not valid \n", __func__, __LINE__, new_port_id);
        return BK_FAIL;
    }

    return BK_OK;
}
#endif

static int _onboard_speaker_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);
    bool read_data_valid_flag = true;

    AUD_ONBOARD_SPK_PROCESS_START();
    if (BK_OK != rtos_get_semaphore(&onboard_spk->can_process, 2000 / portTICK_RATE_MS)) //portMAX_DELAY, 25 / portTICK_RATE_MS
    {
        //return -1;
        BK_LOGE(TAG, "[%s] semaphore get timeout 2000ms\n", audio_element_get_tag(self));
        // TODO
        static int cnt = 0;
        cnt ++;
        if (cnt > 3) {
            return -1;
        }
    }

    BK_LOGV(TAG, "[%s] _onboard_speaker_process \n", audio_element_get_tag(self));

    /* check whether pool enable */
    if (onboard_spk->pool_ring_buff)
    {
        /* write data to speaker ring buffer immediately */
        if (onboard_spk->pool_can_read)
        {
            uint32_t read_size = ring_buffer_read(&onboard_spk->pool_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
            if (read_size != onboard_spk->frame_size)
            {
                BK_LOGV(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, onboard_spk->frame_size);
            }
            else
            {
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
                aec_mic_delay_debug((int16_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
#endif
#ifdef SPK_DATA_DEBUG
                change_pcm_data_to_8k((uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
#endif
                ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(onboard_spk->temp_buff, onboard_spk->frame_size);

                ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
                onboard_spk->wr_spk_rb_done = true;
                /* write data to ref ring buffer */
                audio_element_multi_output(self, (char *)onboard_spk->temp_buff, onboard_spk->frame_size, 0);

                ONBOARD_SPK_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size);
            }
        }
        /*
                else
                {
                    os_memset(onboard_spk->temp_buff, 0x00, onboard_spk->frame_size);
                    BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
                }
        */
    }

    /* read input data */
    AUD_ONBOARD_SPK_INPUT_START();
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    int r_size = 0;
    if (audio_element_get_multi_input_max_port_num(self) > 0)
    {
        int old_port_id = onboard_spk->current_port_id;
        int valid_port_id = _get_valid_port_id(&onboard_spk->input_port_list);
        if (valid_port_id == -1)
        {
            /* Use default audio port, if all audio port is empty */
            //TODO
            BK_LOGV(TAG, "%s, line: %d, valid_port_id: %d \n", __func__, __LINE__, valid_port_id);
            valid_port_id = 0;
        }

        if (valid_port_id != onboard_spk->current_port_id)
        {
            INPUT_PORT_LIST_DEBUG(&onboard_spk->input_port_list, __func__, __LINE__);
            BK_LOGD(TAG, "%s, line: %d, valid_port_id: %d -> %d\n", __func__, __LINE__, onboard_spk->current_port_id, valid_port_id);
            _onboard_speaker_close(self);

            //TODO
            if (BK_OK != _update_dac_config(self, onboard_spk->current_port_id, valid_port_id))
            {
                BK_LOGE(TAG, "%s, line: %d, update dac config fail \n", __func__, __LINE__);
                //TODO
            }
            onboard_spk->current_port_id = valid_port_id;
            _onboard_speaker_open(self);
        }

        /* valid audio port change */
        if (old_port_id != onboard_spk->current_port_id)
        {
            if (old_port_id == 0)
            {
                input_port_list_block(onboard_spk->lock, portMAX_DELAY);
                audio_port_info_t *current_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, onboard_spk->current_port_id);
                if (current_port_info && current_port_info->notify_cb)
                {
                    current_port_info->notify_cb(APT_STATE_RUNNING, (void *)current_port_info, current_port_info->user_data);
                }
                input_port_list_release(onboard_spk->lock);
            }
            else
            {
                if (onboard_spk->current_port_id == 0)
                {
                    input_port_list_block(onboard_spk->lock, portMAX_DELAY);
                    audio_port_info_t *old_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, old_port_id);
                    if (old_port_info && old_port_info->notify_cb)
                    {
                        old_port_info->notify_cb(APT_STATE_FINISHED, (void *)old_port_info, old_port_info->user_data);
                    }
                    input_port_list_release(onboard_spk->lock);
                }
                else
                {
                    input_port_list_block(onboard_spk->lock, portMAX_DELAY);
                    audio_port_info_t *current_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, onboard_spk->current_port_id);
                    audio_port_info_t *old_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, old_port_id);

                    if (current_port_info && current_port_info->notify_cb)
                    {
                        current_port_info->notify_cb(APT_STATE_RUNNING, (void *)current_port_info, current_port_info->user_data);
                    }

                    if (old_port_info != NULL)
                    {
                        if (current_port_info->priority > old_port_info->priority)
                        {
                            if (old_port_info && old_port_info->notify_cb)
                            {
                                old_port_info->notify_cb(APT_STATE_PAUSED, (void *)old_port_info, old_port_info->user_data);
                            }
                        }
                        else
                        {
                            if (old_port_info && old_port_info->notify_cb)
                            {
                                old_port_info->notify_cb(APT_STATE_FINISHED, (void *)old_port_info, old_port_info->user_data);
                            }
                        }
                    }

                    input_port_list_release(onboard_spk->lock);
                }
            }
            BK_LOGD(TAG, "%s, line: %d, old_port_id: %d -> %d\n", __func__, __LINE__, old_port_id, onboard_spk->current_port_id);
        }
        //BK_LOGD(TAG, "%s, audio_element_multi_input r_size: %d\n", __func__, r_size);
    }

    if (onboard_spk->current_port_id == 0)
    {
        r_size = audio_element_input(self, in_buffer, in_len);
    }
    else
    {
        r_size = audio_element_multi_input(self, in_buffer, in_len, onboard_spk->current_port_id - 1, 0);
        //BK_LOGD(TAG, "%s, line: %d, multi_input: %d, r_size: %d \n", __func__, __LINE__, onboard_spk->current_port_id - 1, r_size);
    }
#else
    int r_size = audio_element_input(self, in_buffer, in_len);
#endif
    AUD_ONBOARD_SPK_INPUT_END();

    if (onboard_spk->wr_spk_rb_done == false)
    {
        if (r_size == onboard_spk->frame_size)
        {
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
            aec_mic_delay_debug((int16_t *)in_buffer, onboard_spk->frame_size);
#endif
#ifdef SPK_DATA_DEBUG
            change_pcm_data_to_8k((uint8_t *)in_buffer, onboard_spk->frame_size);
#endif
            ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(in_buffer, onboard_spk->frame_size);

            ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)in_buffer, onboard_spk->frame_size);
            onboard_spk->wr_spk_rb_done = true;
            /* write data to ref ring buffer */
            audio_element_multi_output(self, (char *)in_buffer, onboard_spk->frame_size, 0);
            read_data_valid_flag = false;

            ONBOARD_SPK_DATA_COUNT_ADD_SIZE(onboard_spk->frame_size);
        }
        else
        {
            /* fill silence data */
            os_memset(onboard_spk->temp_buff, 0x00, onboard_spk->frame_size);
#ifdef AEC_MIC_DELAY_POINTS_DEBUG
            aec_mic_delay_debug((int16_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
#endif
            BK_LOGV(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
#ifdef SPK_DATA_DEBUG
            change_pcm_data_to_8k((uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
#endif
            ONBOARD_SPK_DATA_DUMP_BY_UART_DATA(onboard_spk->temp_buff, onboard_spk->frame_size);

            ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
            onboard_spk->wr_spk_rb_done = true;
            /* write data to ref ring buffer */
            audio_element_multi_output(self, (char *)onboard_spk->temp_buff, onboard_spk->frame_size, 0);
        }
    }

    int w_size = 0;
    if (r_size > 0)
    {
        AUD_ONBOARD_SPK_OUTPUT_START();
        /* call _onboard_speaker_write to play or pause if pool ring buffer is exist */
        if (onboard_spk->pool_ring_buff)
        {
            if (read_data_valid_flag == false)
            {
                audio_element_output(self, in_buffer, 0);
                w_size = r_size;
            }
            else
            {
                w_size = audio_element_output(self, in_buffer, r_size);
            }
        }
        else
        {
            w_size = r_size;
        }
        AUD_ONBOARD_SPK_OUTPUT_END();
        /* Update the pointer for processing data */
        //audio_element_update_byte_pos(self, w_size);
    }
    else
    {
        /* check r_size value
           If r_size is AEL_IO_DONE, return AEL_IO_DONE until speaker data of pool_ring_buff is empty.
           If r_size is AEL_IO_TIMEOUT, return in_len.
           If r_size is others, return r_size.
        */
        if (r_size == AEL_IO_TIMEOUT)
        {
            /* call _onboard_speaker_write to play or pause if pool ring buffer is exist */
            if (onboard_spk->pool_ring_buff)
            {
                audio_element_output(self, in_buffer, 0);
            }
            w_size = in_len;
        }
        else if (r_size == AEL_IO_DONE)
        {
            /* two frames in speaker ring buffer playback finish */
            if (onboard_spk->valid_frame_count_in_spk_rb == 0)
            {
                w_size = r_size;
            }
            else
            {
                if (onboard_spk->pool_ring_buff)
                {
                    /* check whether pool_ring_buff is empty */
                    uint32_t fill_size = ring_buffer_get_fill_size(&onboard_spk->pool_rb);
                    if (fill_size < onboard_spk->frame_size)
                    {
                        onboard_spk->valid_frame_count_in_spk_rb -=1;
                        BK_LOGD(TAG, "read data finish, valid_frame_count_in_spk_rb: %d\n", onboard_spk->valid_frame_count_in_spk_rb);
                    }
                }
                else
                {
                    onboard_spk->valid_frame_count_in_spk_rb -=1;
                }
                w_size = in_len;
            }
        }
        else
        {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
            /* When reading data from other sources returns AEL_IO_ABORT, it indicates that the data source has stopped.
               In this case, no error code is returned to prevent the speaker from stopping playback.
             */
            if (onboard_spk->current_port_id > 0)
            {
                w_size = in_len;
            }
            else
            {
                w_size = r_size;
            }
#else
            w_size = r_size;
#endif
        }
    }

    //w_size = onboard_spk->frame_size;

    AUD_ONBOARD_SPK_PROCESS_END();
    //BK_LOGD(TAG, "%s, %d, w_size: %d\n", __func__, __LINE__, w_size);
    return w_size;
}

static bk_err_t _onboard_speaker_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_close \n", audio_element_get_tag(self));

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);

    bk_err_t ret = bk_dma_stop(onboard_spk->spk_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac dma stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    pa_ctrl_en(onboard_spk, false, false);

    ret = bk_aud_dac_stop();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    onboard_spk->is_open = false;
    onboard_spk->pool_can_read = false;
    onboard_spk->wr_spk_rb_done = false;
    onboard_spk->valid_frame_count_in_spk_rb = 0;

    return BK_OK;
}

static bk_err_t _onboard_speaker_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_speaker_destroy \n", audio_element_get_tag(self));

    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(self);
    /* deinit dma */
    aud_dac_dma_deconfig(onboard_spk);
    /* deinit dac */
    bk_aud_dac_deinit();

    /* free spk pool */
    if (onboard_spk && onboard_spk->pool_ring_buff)
    {
        ring_buffer_clear(&onboard_spk->pool_rb);
        audio_free(onboard_spk->pool_ring_buff);
        onboard_spk->pool_ring_buff = NULL;
    }
    if (onboard_spk && onboard_spk->temp_buff)
    {
        audio_free(onboard_spk->temp_buff);
        onboard_spk->temp_buff = NULL;
    }
    if (onboard_spk && onboard_spk->can_process)
    {
        rtos_deinit_semaphore(&onboard_spk->can_process);
        onboard_spk->can_process = NULL;
    }

    if (gl_onboard_speaker->pa_turn_on_timer)
    {
        xTimerDelete(gl_onboard_speaker->pa_turn_on_timer, portMAX_DELAY);
        gl_onboard_speaker->pa_turn_on_timer = NULL;
    }

    if (onboard_spk)
    {
        audio_free(onboard_spk);
        onboard_spk = NULL;
    }

    ONBOARD_SPK_DATA_COUNT_CLOSE();
    ONBOARD_SPK_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t onboard_speaker_stream_init(onboard_speaker_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    gl_onboard_speaker = audio_calloc(1, sizeof(onboard_speaker_stream_t));
    AUDIO_MEM_CHECK(TAG, gl_onboard_speaker, return NULL);
    os_memset(gl_onboard_speaker, 0, sizeof(onboard_speaker_stream_t));

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _onboard_speaker_open;
    cfg.close = _onboard_speaker_close;
    cfg.process = _onboard_speaker_process;
    cfg.destroy = _onboard_speaker_destroy;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write = _onboard_speaker_write;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->frame_size;
    cfg.multi_in_port_num = config->multi_in_port_num;
    cfg.multi_out_port_num = config->multi_out_port_num;
    BK_LOGD(TAG, "cfg.buffer_len: %d\n", cfg.buffer_len);

    cfg.tag = "onboard_speaker";
    gl_onboard_speaker->chl_num = config->chl_num;
    gl_onboard_speaker->sample_rate = config->sample_rate;
    gl_onboard_speaker->dig_gain = config->dig_gain;
    gl_onboard_speaker->ana_gain = config->ana_gain;
    gl_onboard_speaker->work_mode = config->work_mode;
    gl_onboard_speaker->bits = config->bits;
    gl_onboard_speaker->clk_src = config->clk_src;
    gl_onboard_speaker->frame_size = config->frame_size;
    gl_onboard_speaker->pool_length = config->pool_length;
    gl_onboard_speaker->pool_play_thold = config->pool_play_thold;
    gl_onboard_speaker->pool_pause_thold = config->pool_pause_thold;
    gl_onboard_speaker->pa_ctrl_en = config->pa_ctrl_en;
    gl_onboard_speaker->pa_ctrl_gpio = config->pa_ctrl_gpio;
    gl_onboard_speaker->pa_on_level = config->pa_on_level;
    gl_onboard_speaker->pa_on_delay = config->pa_on_delay;
    gl_onboard_speaker->pa_off_delay = config->pa_off_delay;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    gl_onboard_speaker->current_port_id = 0;
#endif

    /* init onboard speaker */
    aud_dac_config_t aud_dac_cfg = DEFAULT_AUD_DAC_CONFIG();
    if (config->chl_num == 1)
    {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_L;
    }
    else if (config->chl_num == 2)
    {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_LR;
    }
    else
    {
        BK_LOGE(TAG, "dac_chl: %d is not support \n", config->chl_num);
        goto _onboard_speaker_init_exit;
    }
	aud_dac_cfg.samp_rate = config->sample_rate;
    aud_dac_cfg.work_mode = config->work_mode;
    aud_dac_cfg.clk_src = config->clk_src;
    aud_dac_cfg.dac_gain = config->dig_gain;
    //aud_dac_cfg.ana_gain = config->ana_gain;
    BK_LOGD(TAG, "dac_cfg chl_num: %s, dig_gain: 0x%02x, sample_rate: 0x%d, clk_src: %s, dac_mode: %s \n",
            aud_dac_cfg.dac_chl == AUD_DAC_CHL_L ? "AUD_DAC_CHL_L" : "AUD_DAC_CHL_LR",
            aud_dac_cfg.dac_gain,
            aud_dac_cfg.samp_rate,
            aud_dac_cfg.clk_src == 1 ? "APLL" : "XTAL",
            aud_dac_cfg.work_mode == 1 ? "AUD_DAC_WORK_MODE_SIGNAL_END" : "AUD_DAC_WORK_MODE_DIFFEN");
    ret = bk_aud_dac_init(&aud_dac_cfg);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, aud_dac_init fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

    if (aud_dac_cfg.dac_gain == 0)
    {
        bk_aud_dac_mute();
    }
    else
    {
        bk_aud_dac_unmute();
    }

    //TODO
    /* set speaker mode */
    /*
        if (config->chl_num == 1) {
            ret = bk_aud_dac_set_mic_mode(AUD_MIC_MIC1, config->adc_cfg.mode);
        } else {
            ret = bk_aud_adc_set_mic_mode(AUD_MIC_BOTH, config->adc_cfg.mode);
        }
    */
    ret = aud_dac_dma_config(gl_onboard_speaker);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac_dma_init fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

    /* init speaker ringbuffer pool */
    if (gl_onboard_speaker->pool_length > 0 && gl_onboard_speaker->pool_length > gl_onboard_speaker->frame_size)
    {
        gl_onboard_speaker->pool_ring_buff = (int8_t *)audio_calloc(1, gl_onboard_speaker->pool_length);
        AUDIO_MEM_CHECK(TAG, gl_onboard_speaker->pool_ring_buff, goto _onboard_speaker_init_exit);
        ring_buffer_init(&gl_onboard_speaker->pool_rb, (uint8_t *)gl_onboard_speaker->pool_ring_buff, gl_onboard_speaker->pool_length, DMA_ID_MAX, RB_DMA_TYPE_NULL);
    }

    gl_onboard_speaker->temp_buff = (int8_t *)audio_calloc(1, gl_onboard_speaker->frame_size);
    os_memset(gl_onboard_speaker->temp_buff, 0x00, gl_onboard_speaker->frame_size);

    ret = rtos_init_semaphore(&gl_onboard_speaker->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _onboard_speaker_init_exit;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (cfg.multi_in_port_num > 0)
    {
        STAILQ_INIT(&gl_onboard_speaker->input_port_list);

        gl_onboard_speaker->lock = xSemaphoreCreateMutex();
        if (gl_onboard_speaker->lock == NULL)
        {
            BK_LOGE(TAG, "[%s] create semaphore fail \n", cfg.tag);
            goto _onboard_speaker_init_exit;
        }
    }
#endif

    if (gl_onboard_speaker->pa_ctrl_en)
    {
        if (gl_onboard_speaker->pa_on_delay > 0)
        {
            gl_onboard_speaker->pa_turn_on_timer = xTimerCreate(
                "pa_turn_on_timer",
                BK_MS_TO_TICKS(gl_onboard_speaker->pa_on_delay),
                pdFALSE,
                (void *)gl_onboard_speaker,
                pa_turn_on_timer_callback);
            if (gl_onboard_speaker->pa_turn_on_timer == NULL)
            {
                BK_LOGE(TAG, "create pa_turn_on_timer fail \n");
                goto _onboard_speaker_init_exit;
            }
        }

        /* config gpio to output */
        gpio_dev_unmap(gl_onboard_speaker->pa_ctrl_gpio);
        bk_gpio_enable_output(gl_onboard_speaker->pa_ctrl_gpio);
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _onboard_speaker_init_exit);
    audio_element_setdata(el, gl_onboard_speaker);

    audio_element_info_t info = {0};
    info.sample_rates = config->sample_rate;
    info.channels = config->chl_num;
    info.bits = config->bits;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    ONBOARD_SPK_DATA_COUNT_OPEN();
    ONBOARD_SPK_DATA_DUMP_BY_UART_OPEN();

    return el;
_onboard_speaker_init_exit:
    /* deinit dma */
    aud_dac_dma_deconfig(gl_onboard_speaker);
    /* deinit dac */
    bk_aud_dac_deinit();
    bk_aud_driver_deinit();
    /* free spk pool */
    if (gl_onboard_speaker->pool_ring_buff)
    {
        ring_buffer_clear(&gl_onboard_speaker->pool_rb);
        audio_free(gl_onboard_speaker->pool_ring_buff);
        gl_onboard_speaker->pool_ring_buff = NULL;
    }
    if (gl_onboard_speaker->temp_buff)
    {
        audio_free(gl_onboard_speaker->temp_buff);
        gl_onboard_speaker->temp_buff = NULL;
    }
    if (gl_onboard_speaker->can_process)
    {
        rtos_deinit_semaphore(&gl_onboard_speaker->can_process);
        gl_onboard_speaker->can_process = NULL;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (gl_onboard_speaker->lock)
    {
        vSemaphoreDelete(gl_onboard_speaker->lock);
        gl_onboard_speaker->lock = NULL;
    }
#endif

    if (gl_onboard_speaker->pa_turn_on_timer)
    {
        xTimerDelete(gl_onboard_speaker->pa_turn_on_timer, portMAX_DELAY);
        gl_onboard_speaker->pa_turn_on_timer = NULL;
    }

    audio_free(gl_onboard_speaker);
    gl_onboard_speaker = NULL;
    return NULL;
}

bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int bits, int ch)
{
    bk_err_t err = BK_OK;
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
    audio_element_state_t state = audio_element_get_state(onboard_speaker_stream);

    BK_LOGD(TAG, "%s \n", __func__);

    /* check param */
    if (rate != 8000 && rate != 110250 && rate != 12000 && rate != 16000 && rate != 22050 && rate != 24000 && rate != 32000 && rate != 44100 && rate != 48000)
    {
        BK_LOGE(TAG, "sample rate: %d is not support \n", rate);
        return BK_FAIL;
    }
    if (ch < 1 || ch > 2)
    {
        BK_LOGE(TAG, "dac_chl: %d is not support \n", ch);
        return BK_FAIL;
    }
    if (bits != 16)
    {
        BK_LOGE(TAG, "bits: %d is not support \n", bits);
        return BK_FAIL;
    }

    if (onboard_spk->sample_rate == rate && onboard_spk->chl_num == ch && onboard_spk->bits == bits)
    {
        BK_LOGD(TAG, "current sample_rate: %d, chl_num: %d, bits: %d \n", onboard_spk->sample_rate, onboard_spk->chl_num, onboard_spk->bits);
        BK_LOGD(TAG, "new samp_rate: %d, chl_num: %d, bits: %d \n", rate, ch, bits);
        BK_LOGD(TAG, "not need update onboard speaker \n");
        return BK_OK;
    }

    if (state == AEL_STATE_RUNNING)
    {
        /* set read data timeout */
        audio_element_set_input_timeout(onboard_speaker_stream, 0);
        if (BK_OK != audio_element_pause(onboard_speaker_stream))
        {
            BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
        }
    }

    if (BK_OK == audio_dac_reconfig(onboard_spk, rate, ch, bits))
    {
        onboard_spk->sample_rate = rate;
        onboard_spk->chl_num = ch;
        onboard_spk->bits = bits;
        audio_element_setdata(onboard_speaker_stream, onboard_spk);
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, updata onboard speaker config fail \n", __func__, __LINE__);
        err = BK_FAIL;
    }

    audio_element_set_music_info(onboard_speaker_stream, rate, ch, bits);

    if (state == AEL_STATE_RUNNING)
    {
        audio_element_resume(onboard_speaker_stream, 0, 0);
        /* set read data timeout */
        audio_element_set_input_timeout(onboard_speaker_stream, 2000);
    }

    return err;
}

bk_err_t onboard_speaker_stream_set_digital_gain(audio_element_handle_t onboard_speaker_stream, uint8_t gain)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (gain < 0 || gain > 0x3f)
    {
        BK_LOGE(TAG, "gain: %d is out of range: 0x00 ~ 0x3f \n", gain);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (onboard_spk->dig_gain == gain)
    {
        BK_LOGD(TAG, "not need updata onboard speaker digital gain \n");
        return BK_OK;
    }

    if (BK_OK == bk_aud_dac_set_gain(gain))
    {
        if (gain == 0)
        {
            pa_ctrl_en(onboard_spk, false, false);
            bk_aud_dac_mute();
        }
        else
        {
            pa_ctrl_en(onboard_spk, true, false);
            bk_aud_dac_unmute();
        }
        onboard_spk->dig_gain = gain;
        audio_element_setdata(onboard_speaker_stream, onboard_spk);
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, updata speaker digital gain fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t onboard_speaker_stream_get_digital_gain(audio_element_handle_t onboard_speaker_stream, uint8_t *gain)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (gain == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    *gain = onboard_spk->dig_gain;

    return BK_OK;
}


bk_err_t onboard_speaker_stream_dac_mute_en(audio_element_handle_t onboard_speaker_stream, uint8_t value)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (value == 0)
    {
        bk_aud_dac_unmute();
    }
    else
    {
        bk_aud_dac_mute();
    }

    return BK_OK;
}

bk_err_t onboard_speaker_stream_set_analog_gain(audio_element_handle_t onboard_speaker_stream, uint8_t gain)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (gain < 0 || gain > 0x3f)
    {
        BK_LOGE(TAG, "gain: %d is out of range: 0x00 ~ 0x3f \n", gain);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (onboard_spk->ana_gain == gain)
    {
        BK_LOGD(TAG, "not need update onboard spk analog gain \n");
        return BK_OK;
    }

	if (BK_OK == bk_aud_set_ana_dac_gain(gain))
	{
		onboard_spk->ana_gain = gain;
		audio_element_setdata(onboard_speaker_stream, onboard_spk);
	} else
	{
		BK_LOGE(TAG, "%s, line: %d, update spk analog gain fail \n", __func__, __LINE__);
		return BK_FAIL;
	}

    return BK_OK;
}

bk_err_t onboard_speaker_stream_get_analog_gain(audio_element_handle_t onboard_speaker_stream, uint8_t *gain)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
    /* check param */
    if (gain == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }
    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    *gain = onboard_spk->ana_gain;
    return BK_OK;
}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
bk_err_t onboard_speaker_stream_get_input_port_info_by_port_id(audio_element_handle_t onboard_speaker_stream, uint8_t port_id, audio_port_info_t **port_info)
{
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    /* check param */
    if (onboard_spk == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (port_info == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, port_info is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (port_id > (audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1))
    {
        BK_LOGE(TAG, "%s, line: %d, audio port id: %d out of range: 0 ~ %d \n", __func__, __LINE__, port_id, audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1);
        return BK_FAIL;
    }

    *port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, port_id);

    return BK_OK;
}

bk_err_t onboard_speaker_stream_set_input_port_info(audio_element_handle_t onboard_speaker_stream, audio_port_info_t *port_info)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);

    INPUT_PORT_LIST_DEBUG(&onboard_spk->input_port_list, __func__, __LINE__);

    /* check param */
    if (onboard_spk == NULL || port_info == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_spk is not init or port_info: %p is NULL \n", __func__, __LINE__, port_info);
        return BK_FAIL;
    }

    if (port_info->port_id > (audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1))
    {
        BK_LOGE(TAG, "%s, line: %d, audio port id: %d out of range: 0 ~ %d \n", __func__, __LINE__, port_info->port_id, audio_element_get_multi_input_max_port_num(onboard_speaker_stream) + 1);
        return BK_FAIL;
    }

    /* Only one input port is allowed under one priority level */
    STAILQ_FOREACH(audio_port_info_item, &onboard_spk->input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority == port_info->priority && audio_port_info_item->port_info.port_id != port_info->port_id)
        {
            BK_LOGE(TAG, "%s, line: %d, audio port: %d, priority: %d is exist, different ports are not allowed to use the same priority\n", __func__, __LINE__, port_info->port_id, port_info->priority);
            return BK_FAIL;
        }
    }

    /* Check all ports in the input port list to see if there is a port with the same port ID as port_info, and update the port information */
    audio_port_info_t *tmp_port_info = _get_audio_port_info_by_port_id(&onboard_spk->input_port_list, port_info->port_id);
    if (tmp_port_info)
    {
        /* check whether port_info is same as tmp_port_info */
        if (memcmp(tmp_port_info, port_info, sizeof(audio_port_info_t)) == 0)
        {
            BK_LOGD(TAG, "%s, line: %d, port_info is same as tmp_port_info, not update\n", __func__, __LINE__);
            return BK_OK;
        }

        /* update audio port info in port list */
        input_port_list_block(onboard_spk->lock, portMAX_DELAY);
        _update_audio_port_to_list(&onboard_spk->input_port_list, port_info);
        input_port_list_release(onboard_spk->lock);
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            if (audio_element_get_input_port(onboard_speaker_stream) != port_info->port)
            {
                audio_element_set_input_port(onboard_speaker_stream, port_info->port);
            }
        }
        else
        {
            if (audio_element_get_multi_input_port(onboard_speaker_stream, port_info->port_id - 1) != port_info->port)
            {
                audio_element_set_multi_input_port(onboard_speaker_stream, port_info->port, (int)(port_info->port_id - 1));
            }
        }
    }
    else
    {
        /* if port_info->port is NULL, not add to list */
        if (port_info->port != NULL)
        {
            /* add new port */
            input_port_list_block(onboard_spk->lock, portMAX_DELAY);
            _add_audio_port_to_list(&onboard_spk->input_port_list, port_info);
            input_port_list_release(onboard_spk->lock);
        }
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            audio_element_set_input_port(onboard_speaker_stream, port_info->port);
        }
        else
        {
            audio_element_set_multi_input_port(onboard_speaker_stream, port_info->port, (int)(port_info->port_id - 1));
        }
    }

    INPUT_PORT_LIST_DEBUG(&onboard_spk->input_port_list, __func__, __LINE__);
    return BK_OK;
}
#endif
