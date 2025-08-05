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
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/onboard_dual_dmic_mic_stream.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/aud_adc.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/aud_dmic.h>


#define TAG  "OB_MIC"

#define DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL    (8)

//#define ONBOARD_MIC_DEBUG   //GPIO debug

#ifdef ONBOARD_MIC_DEBUG

#define AUD_ADC_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AUD_ADC_DMA_ISR_END()                   do { GPIO_DOWN(32); } while (0)

#define AUD_ONBOARD_MIC_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AUD_ONBOARD_MIC_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define AUD_ONBOARD_MIC_SEM_WAIT_START()        do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AUD_ONBOARD_MIC_SEM_WAIT_END()          do { GPIO_DOWN(34); } while (0)

#define AUD_ONBOARD_MIC_INPUT_START()           do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AUD_ONBOARD_MIC_INPUT_END()             do { GPIO_DOWN(35); } while (0)

#define AUD_ONBOARD_MIC_OUTPUT_START()          do { GPIO_DOWN(36); GPIO_UP(36);} while (0)
#define AUD_ONBOARD_MIC_OUTPUT_END()            do { GPIO_DOWN(36); } while (0)

#else

#define AUD_ADC_DMA_ISR_START()
#define AUD_ADC_DMA_ISR_END()

#define AUD_ONBOARD_MIC_PROCESS_START()
#define AUD_ONBOARD_MIC_PROCESS_END()

#define AUD_ONBOARD_MIC_SEM_WAIT_START()
#define AUD_ONBOARD_MIC_SEM_WAIT_END()

#define AUD_ONBOARD_MIC_INPUT_START()
#define AUD_ONBOARD_MIC_INPUT_END()

#define AUD_ONBOARD_MIC_OUTPUT_START()
#define AUD_ONBOARD_MIC_OUTPUT_END()

#endif

/* onboard mic data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count onboard mic data. */
#if CONFIG_ADK_UTILS

#define ONBOARD_MIC_DATA_COUNT

#endif  //CONFIG_ADK_UTILS


#ifdef ONBOARD_MIC_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t onboard_mic_count_util = {0};
#define ONBOARD_MIC_DATA_COUNT_INTERVAL     (1000 * 4)
#define ONBOARD_MIC_DATA_COUNT_TAG          "ONBOARD_MIC"

#define ONBOARD_MIC_DATA_COUNT_OPEN()               count_util_create(&onboard_mic_count_util, ONBOARD_MIC_DATA_COUNT_INTERVAL, ONBOARD_MIC_DATA_COUNT_TAG)
#define ONBOARD_MIC_DATA_COUNT_CLOSE()              count_util_destroy(&onboard_mic_count_util)
#define ONBOARD_MIC_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&onboard_mic_count_util, size)

#else

#define ONBOARD_MIC_DATA_COUNT_OPEN()
#define ONBOARD_MIC_DATA_COUNT_CLOSE()
#define ONBOARD_MIC_DATA_COUNT_ADD_SIZE(size)

#endif  //ONBOARD_MIC_DATA_COUNT

/* dump onboard_mic stream output pcm data by uart */
//#define ONBOARD_MIC_DATA_DUMP_BY_UART

#ifdef ONBOARD_MIC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_ob_mic_uart_util = {0};
#define ONBOARD_MIC_DATA_DUMP_UART_ID            (1)
#define ONBOARD_MIC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_ob_mic_uart_util, ONBOARD_MIC_DATA_DUMP_UART_ID, ONBOARD_MIC_DATA_DUMP_UART_BAUD_RATE)
#define ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_ob_mic_uart_util)
#define ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_ob_mic_uart_util, data_buf, len)

#else

#define ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN()
#define ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE()
#define ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //ONBOARD_MIC_DATA_DUMP_BY_UART


typedef struct onboard_dual_dmic_mic_stream
{
    dual_dmic_adc_cfg_t      adc_cfg;          /**< ADC mode configuration */
    bool                     is_open;          /**< mic enable, true: enable, false: disable */
    uint32_t                 frame_size;       /**< size of one frame mic data, the size
                                                        when AUD_MIC_CHL_MIC1 mode, the size must bean integer multiple of two bytes
                                                        when AUD_MIC_CHL_DUAL mode, the size must bean integer multiple of four bytes */
    dma_id_t                 mic_dma_id;       /**< dma id that dma carry mic data from fifo to ring buffer */
    RingBufferContext        mic_rb;           /**< mic rb handle */
    int8_t                  *mic_ring_buff;    /**< mic ring buffer address */
    int                      out_block_size;   /**< Size of output block */
    int                      out_block_num;    /**< Number of output block */
    beken_semaphore_t        can_process;      /**< can process */

    int                      ref_mode;         /**< use SW(0)/HW(1) to capture reference signal for AEC*/
    int                      dual_dmic;        /**< Enable dual dmic(1)/Disable dual dmic(0)*/
    int                      dual_dmic_sgl_out;/*!< Enable dual dmic single output*/
    dma_id_t                 dmic_dma_id;      /**< dma id that dma carry dmic data from fifo to ring buffer */
    RingBufferContext        dmic_rb;          /**< dmic rb handle */
    int8_t                  *dmic_ring_buff;   /**< dmic ring buffer address */
    beken_event_t            mic_evt;          /**< mic evt for ADC/DMIC DMA */
    uint16_t                 *hw_ref_buf;      /**< save HW reference data */
} onboard_dual_dmic_mic_stream_t;

static onboard_dual_dmic_mic_stream_t *gl_onboard_mic = NULL;
static uint16_t *p_mic_data_temp_save = NULL;

static bk_err_t aud_adc_dma_deconfig(onboard_dual_dmic_mic_stream_t *onboard_mic)
{
    if (onboard_mic == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_mic->mic_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_mic->mic_dma_id);
    //bk_dma_driver_deinit();
    if (onboard_mic->mic_ring_buff)
    {
        ring_buffer_clear(&onboard_mic->mic_rb);
        audio_dma_mem_free(onboard_mic->mic_ring_buff);
        onboard_mic->mic_ring_buff = NULL;
    }

    return BK_OK;
}

/* Carry one frame audio dac data(20ms) from ADC FIFO complete */
static void aud_adc_dma_finish_isr(void)
{
    AUD_ADC_DMA_ISR_START();

    if(gl_onboard_mic->dual_dmic && gl_onboard_mic->ref_mode)
    {
        rtos_set_event_flags(&gl_onboard_mic->mic_evt, EVENT_MIC_DMA_DONE_BIT);
    }
    
    BK_LOGV(TAG, "adc dma done!\n");
    AUD_ADC_DMA_ISR_END();
}

static bk_err_t aud_adc_dma_config(onboard_dual_dmic_mic_stream_t *onboard_mic)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t adc_port_addr;
    uint32_t frame_size = 0;

    os_memset(&dma_config, 0, sizeof(dma_config_t));
#if 0
    /* init dma driver */
    ret = bk_dma_driver_init();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_driver_init fail\n", __func__, __LINE__);
        goto exit;
    }
#endif
    /* malloc dma channel */
    onboard_mic->mic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_mic->mic_dma_id < DMA_ID_0) || (onboard_mic->mic_dma_id >= DMA_ID_MAX))
    {
        BK_LOGE(TAG, "malloc dma fail \n");
        goto exit;
    }

    /* DMA must carry adcl and adcr data together. frame_size is one channel data size.
     * If channel number is one, need double frame_size.
     */
    if (onboard_mic->adc_cfg.chl_num == 1)
    {
        frame_size = onboard_mic->frame_size * 2;
    }
    else
    {
        frame_size = onboard_mic->frame_size;
    }

    /* init ringbuffer to save two frame data. */
    onboard_mic->mic_ring_buff = (int8_t *)audio_dma_mem_calloc(2, frame_size + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL / 2);
    AUDIO_MEM_CHECK(TAG, onboard_mic->mic_ring_buff, return BK_FAIL);
    /* init dma channel */
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev = DMA_DEV_AUDIO_RX;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    /* get adc fifo address */
    if (bk_aud_adc_get_fifo_addr(&adc_port_addr) != BK_OK)
    {
        BK_LOGE(TAG, "get adc fifo address failed\r\n");
        goto exit;
    }
    else
    {
        dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr = adc_port_addr;
        dma_config.src.end_addr = adc_port_addr + 4;
    }
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uint32_t)(uintptr_t)onboard_mic->mic_ring_buff;
    dma_config.dst.end_addr = (uint32_t)(uintptr_t)onboard_mic->mic_ring_buff + frame_size * 2 + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL;
    ret = bk_dma_init(onboard_mic->mic_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_mic->mic_dma_id, frame_size);
    /* register dma isr */
    bk_dma_register_isr(onboard_mic->mic_dma_id, NULL, (void *)aud_adc_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_mic->mic_dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
#endif

    ring_buffer_init(&onboard_mic->mic_rb, (uint8_t *)onboard_mic->mic_ring_buff, frame_size * 2 + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL, onboard_mic->mic_dma_id, RB_DMA_TYPE_WRITE);

    BK_LOGD(TAG, "adc_dma_cfg mic_dma_id: %d, transfer_len: %d \n", onboard_mic->mic_dma_id, frame_size);
    BK_LOGD(TAG, "src_start_addr: 0x%08x, src_end_addr: 0x%08x \n", dma_config.src.start_addr, dma_config.src.end_addr);
    BK_LOGD(TAG, "dst_start_addr: 0x%08x, dst_end_addr: 0x%08x \n", dma_config.dst.start_addr, dma_config.dst.end_addr);

    return BK_OK;
exit:
    aud_adc_dma_deconfig(onboard_mic);
    return BK_FAIL;
}

static bk_err_t aud_dmic_dma_deconfig(onboard_dual_dmic_mic_stream_t *onboard_mic)
{
    if (onboard_mic == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(onboard_mic->dmic_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, onboard_mic->dmic_dma_id);
    if (onboard_mic->dmic_ring_buff)
    {
        ring_buffer_clear(&onboard_mic->dmic_rb);
        audio_dma_mem_free(onboard_mic->dmic_ring_buff);
        onboard_mic->dmic_ring_buff = NULL;
    }

    return BK_OK;
}

static void aud_dmic_dma_finish_isr(void)
{
    rtos_set_event_flags(&gl_onboard_mic->mic_evt,EVENT_DUAL_DMIC_DMA_DONE_BIT);
    BK_LOGV(TAG, "dmic dma done!\n");
}

static bk_err_t aud_dmic_dma_config(onboard_dual_dmic_mic_stream_t *onboard_mic)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dmic_port_addr;
    uint32_t frame_size = 0;

    os_memset(&dma_config, 0, sizeof(dma_config_t));
    /* malloc dma channel */
    onboard_mic->dmic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((onboard_mic->dmic_dma_id < DMA_ID_0) || (onboard_mic->dmic_dma_id >= DMA_ID_MAX))
    {
        BK_LOGE(TAG, "malloc dma fail \n");
        goto exit;
    }

    /* DMA must carry adcl and adcr data together. frame_size is one channel data size.
     * If channel number is one, need double frame_size.
     */
    frame_size = onboard_mic->frame_size * 2;

    /* init ringbuffer to save two frame data. */
    onboard_mic->dmic_ring_buff = (int8_t *)audio_dma_mem_calloc(2, frame_size + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL / 2);
    AUDIO_MEM_CHECK(TAG, onboard_mic->dmic_ring_buff, return BK_FAIL);
    /* init dma channel */
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev = DMA_DEV_AUD_DMIC;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    /* get adc fifo address */
    if (bk_aud_dmic_get_fifo_addr(&dmic_port_addr) != BK_OK)
    {
        BK_LOGE(TAG, "get adc fifo address failed\r\n");
        goto exit;
    }
    else
    {
        dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
        dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
        dma_config.src.start_addr = dmic_port_addr;
        dma_config.src.end_addr = dmic_port_addr + 4;
    }
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uint32_t)(uintptr_t)onboard_mic->dmic_ring_buff;
    dma_config.dst.end_addr = (uint32_t)(uintptr_t)onboard_mic->dmic_ring_buff + frame_size * 2 + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL;
    ret = bk_dma_init(onboard_mic->dmic_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
        goto exit;
    }

    /* set dma transfer length */
    bk_dma_set_transfer_len(onboard_mic->dmic_dma_id, frame_size);
    /* register dma isr */
    bk_dma_register_isr(onboard_mic->dmic_dma_id, NULL, (void *)aud_dmic_dma_finish_isr);
    bk_dma_enable_finish_interrupt(onboard_mic->dmic_dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(onboard_mic->dmic_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(onboard_mic->dmic_dma_id, DMA_ATTR_SEC);
#endif

    ring_buffer_init(&onboard_mic->dmic_rb, (uint8_t *)onboard_mic->dmic_ring_buff, frame_size * 2 + DMA_CARRY_MIC_RINGBUF_SAFE_INTERVAL, onboard_mic->dmic_dma_id, RB_DMA_TYPE_WRITE);

    BK_LOGD(TAG, "dmic_dma_cfg mic_dma_id: %d, transfer_len: %d \n", onboard_mic->dmic_dma_id, frame_size);
    BK_LOGD(TAG, "src_start_addr: 0x%08x, src_end_addr: 0x%08x \n", dma_config.src.start_addr, dma_config.src.end_addr);
    BK_LOGD(TAG, "dst_start_addr: 0x%08x, dst_end_addr: 0x%08x \n", dma_config.dst.start_addr, dma_config.dst.end_addr);

    return BK_OK;
exit:
    aud_dmic_dma_deconfig(onboard_mic);
    return BK_FAIL;

}

static int _onboard_dual_dmic_mic_read_hw_ref(audio_element_handle_t self, char *buffer, int len)
{
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(self), __func__, len);

    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(self);
    int ret = BK_OK;
    uint32_t read_size = 0;
    uint32_t fill_size;
    uint16_t *addr = (uint16_t *)buffer;
    BK_LOGV(TAG, "[%s] %s, dual_dmic:%d,ref_m:%d,f_s:%d,len:%d\n", audio_element_get_tag(self), __func__,onboard_mic->dual_dmic,onboard_mic->ref_mode,onboard_mic->frame_size,len);

    if (len)
    {
        //Read reference data from MIC ring buffer when HW ref
        fill_size = ring_buffer_get_fill_size(&onboard_mic->mic_rb);
        BK_LOGV(TAG, "[%s] %s, fill_size: %d \n", audio_element_get_tag(self), __func__, fill_size);
        if (fill_size >= len)
        {
            read_size = ring_buffer_read(&onboard_mic->mic_rb, (uint8_t *)p_mic_data_temp_save, len);
            if (read_size == onboard_mic->frame_size*2)//mic+ref
            {
                for (uint16_t i = 0; i < onboard_mic->frame_size/2; i++)
                {
                    addr[i] = p_mic_data_temp_save[2 * i + 1];//get hw ref data,only half data needed
                }
                ret = read_size/2;
                BK_LOGV(TAG, "[%s] %s, read data ok, read_size: %d, fill_size: %d\n", audio_element_get_tag(self), __func__, read_size, fill_size);
            }
            else
            {
                BK_LOGE(TAG, "[%s] %s, read data error, read_size: %d, fill_size: %d\n", audio_element_get_tag(self), __func__, read_size, fill_size);
                os_memset(buffer, 0, len/2);
                ret = len/2;
            }
        }
        else
        {
            BK_LOGE(TAG, "[%s] %s, mic_fill: %d < len: %d \n", audio_element_get_tag(self), __func__, fill_size, len);
            /* dma carry data length is not right */
            //TODO
            /* ============== workaround handle================== */
            os_memset(buffer, 0, len/2);
            ret = len/2;
        }        
    }
    else
    {
        os_memset(buffer, 0, len/2);
        ret = len/2;
    }

    return ret;
}

static bk_err_t _onboard_dual_dmic_mic_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(self);

    if (onboard_mic->is_open)
    {
        return BK_OK;
    }

    /* set read data timeout */
    //audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);
    //ring_buffer_clear(&onboard_mic->mic_rb);
    bk_err_t ret;
    if(onboard_mic->dual_dmic)
    {
        if(onboard_mic->ref_mode)
        {
            ret = bk_dma_start(onboard_mic->mic_dma_id);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, adc dma start fail\n", __func__, __LINE__);
                return BK_FAIL;
            }
            ret = bk_aud_adc_start();
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "%s, %d, adc start fail\n", __func__, __LINE__);
                return BK_FAIL;
            }
        }
        
        ret = bk_dma_start(onboard_mic->dmic_dma_id);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, dmic dma start fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        ret = bk_aud_dmic_start();
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, dmic start fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }
    else
    {
        ret = bk_dma_start(onboard_mic->mic_dma_id);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, adc dma start fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        ret = bk_aud_adc_start();
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, adc start fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    onboard_mic->is_open = true;

    return BK_OK;
}

static int _onboard_dual_dmic_mic_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(el);
    int ret = BK_OK;
    uint32_t read_size = 0;
    uint32_t fill_size;
    RingBufferContext* rb;

    if (len)
    {
        if(onboard_mic->dual_dmic)
        {
            rb = &onboard_mic->dmic_rb;
        }
        else
        {
            rb = &onboard_mic->mic_rb;
        }
        
        fill_size = ring_buffer_get_fill_size(rb);
        BK_LOGV(TAG, "[%s] %s, fill_size: %d \n", audio_element_get_tag(el), __func__, fill_size);
        if (fill_size >= len)
        {
            read_size = ring_buffer_read(rb, (uint8_t *)buffer, len);
            if (read_size == len)
            {
                ret = read_size;
                BK_LOGV(TAG, "[%s] %s, read data ok, read_size: %d, fill_size: %d\n", audio_element_get_tag(el), __func__, read_size, fill_size);
            }
            else
            {
                BK_LOGE(TAG, "[%s] %s, read data error, read_size: %d, fill_size: %d\n", audio_element_get_tag(el), __func__, read_size, fill_size);
                ret = -1;
            }
        }
        else
        {
            BK_LOGW(TAG, "[%s] %s, mic_fill: %d < len: %d \n", audio_element_get_tag(el), __func__, fill_size, len);
            /* dma carry data length is not right */
            //TODO
            /* ============== workaround handle================== */
            os_memset(buffer, 0, len);
            ret = read_size;
            //ret = 0;
        }
    }
    else
    {
        ret = len;
    }

    if(!onboard_mic->dual_dmic)
    {
        /* remove adcr data and remain adcl data. */
        if (ret > 0 && onboard_mic->adc_cfg.chl_num == 1)
        {
            int16_t *ptr = (int16_t *)buffer;
            for (uint32_t i = 0; i < ret / 4; i++)
            {
                ptr[i] = ptr[2 * i];
            }
            ret = ret / 2;
        }
    }

    return ret;
}

static int _onboard_dual_dmic_mic_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(self);

    AUD_ONBOARD_MIC_PROCESS_START();

    AUD_ONBOARD_MIC_SEM_WAIT_START();

    uint32_t wait_event;
    static uint32_t event_suc_cnt = 0;
   
    if(onboard_mic->dual_dmic)
    {
        if(onboard_mic->ref_mode)/*reference signal sampling HARDWARE mode*/
        {
            wait_event = rtos_wait_for_event_flags (&onboard_mic->mic_evt, 
                                                    EVENT_MIC_DMA_DONE_BIT | EVENT_DUAL_DMIC_DMA_DONE_BIT, 
                                                    true, 
                                                    WAIT_FOR_ALL_EVENTS, 
                                                    BEKEN_WAIT_FOREVER);
            if(( wait_event & ( EVENT_MIC_DMA_DONE_BIT | EVENT_DUAL_DMIC_DMA_DONE_BIT )) == (EVENT_MIC_DMA_DONE_BIT | EVENT_DUAL_DMIC_DMA_DONE_BIT)) 
            {
                BK_LOGV(TAG, "mic thread wait event EVENT_MIC_DMA_DONE_BIT & EVENT_DUAL_DMIC_DMA_DONE_BIT ok!cnt:%d\r\n",event_suc_cnt++);
            }
        }
        else/*reference signal sampling SOFTWARE mode*/
        {
            wait_event = rtos_wait_for_event_flags (&onboard_mic->mic_evt, 
                                                    EVENT_DUAL_DMIC_DMA_DONE_BIT, 
                                                    true, 
                                                    WAIT_FOR_ALL_EVENTS, 
                                                    BEKEN_WAIT_FOREVER);
            if(( wait_event & (EVENT_DUAL_DMIC_DMA_DONE_BIT )) == (EVENT_DUAL_DMIC_DMA_DONE_BIT)) 
            {
                BK_LOGV(TAG, "mic thread wait event EVENT_DUAL_DMIC_DMA_DONE_BIT ok!cnt:%d\r\n",event_suc_cnt++);
            }
        }
    }
    
    AUD_ONBOARD_MIC_SEM_WAIT_END();

    BK_LOGV(TAG, "[%s] _onboard_mic_process \n", audio_element_get_tag(self));

    /* read input data */
    AUD_ONBOARD_MIC_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);

    if(onboard_mic->dual_dmic && onboard_mic->ref_mode)
    {
        _onboard_dual_dmic_mic_read_hw_ref(self, (char *)onboard_mic->hw_ref_buf, in_len);
    }

    AUD_ONBOARD_MIC_INPUT_END();

    /* used to test dma pause function */
#if 0
    static uint32_t count = 0;
    if (count > 500)
    {
        //BK_LOGD(TAG, "%s, count: %d\n", __func__, count);
        rtos_delay_milliseconds(300);
    }
    else
    {
        count++;
    }
#endif

    int w_size = 0;
    if (r_size == AEL_IO_TIMEOUT)
    {
        r_size = 0;
    }
    else if (r_size > 0)
    {
        if(onboard_mic->dual_dmic)
        {
            ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);
            AUD_ONBOARD_MIC_OUTPUT_START();
            if(onboard_mic->ref_mode)
            {
                //write HW ref data to aec ref ring buffer
                audio_element_multi_output(self, (char *)onboard_mic->hw_ref_buf, r_size/2, 0);
            }
            
            if(onboard_mic->dual_dmic_sgl_out)
            {
                /* remove dmic r data and remain dmic l data. */
                int16_t *ptr = (int16_t *)in_buffer;
                for (uint32_t i = 0; i < r_size / 4; i++)
                {
                    ptr[i] = ptr[2 * i];
                }
                r_size = r_size / 2;
            }
            
            w_size = audio_element_output(self, in_buffer, r_size);
            AUD_ONBOARD_MIC_OUTPUT_END();
            ONBOARD_MIC_DATA_COUNT_ADD_SIZE(r_size);

            /* write data to multiple audio port */
            /* unblock write, and not check write result */
            //TODO
            audio_element_multi_output(self, in_buffer, r_size, 0);
        }
        else
        {
            ONBOARD_MIC_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);
            AUD_ONBOARD_MIC_OUTPUT_START();
            w_size = audio_element_output(self, in_buffer, r_size);
            if(onboard_mic->ref_mode)
            {
                //write HW ref data to aec ref ring buffer
                audio_element_multi_output(self, (char *)onboard_mic->hw_ref_buf, r_size, 0);
            }
            AUD_ONBOARD_MIC_OUTPUT_END();

            ONBOARD_MIC_DATA_COUNT_ADD_SIZE(r_size);

            /* write data to multiple audio port */
            /* unblock write, and not check write result */
            //TODO
            audio_element_multi_output(self, in_buffer, r_size, 0);
        }
    }
    else
    {
        w_size = r_size;
    }

    AUD_ONBOARD_MIC_PROCESS_END();

    return w_size;
}

static bk_err_t _onboard_dual_dmic_mic_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s\n", audio_element_get_tag(self), __func__);

    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(self);

    bk_err_t ret = bk_dma_stop(onboard_mic->mic_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac dma stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_aud_adc_stop();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dac stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_dma_stop(onboard_mic->dmic_dma_id);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dmic dac dma stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_aud_dmic_stop();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, dmic stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    onboard_mic->is_open = false;

    return BK_OK;
}

static bk_err_t _onboard_dual_dmic_mic_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _onboard_mic_destroy \n", audio_element_get_tag(self));

    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(self);
    /* deinit dma */
    aud_adc_dma_deconfig(onboard_mic);
    /* deinit dac */
    bk_aud_adc_deinit();

    if(onboard_mic->dual_dmic)
    {
        aud_dmic_dma_deconfig(onboard_mic);
        bk_aud_dmic_deinit();
    }
    
    if (onboard_mic->mic_evt)
    {
        rtos_deinit_event_flags(&onboard_mic->mic_evt);
    }
    
    if(p_mic_data_temp_save)
    {
        audio_free(p_mic_data_temp_save);
        p_mic_data_temp_save = NULL;
    }
    
    if(gl_onboard_mic->hw_ref_buf)
    {
        audio_free(gl_onboard_mic->hw_ref_buf);
        gl_onboard_mic->hw_ref_buf = NULL;
    }

    audio_free(onboard_mic);
    onboard_mic = NULL;

    ONBOARD_MIC_DATA_COUNT_CLOSE();
    ONBOARD_MIC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t onboard_dual_dmic_mic_stream_init(onboard_dual_dmic_mic_stream_cfg_t *config)
{
    audio_element_handle_t el;
    bk_err_t ret = BK_OK;
    gl_onboard_mic = audio_calloc(1, sizeof(onboard_dual_dmic_mic_stream_t));
    AUDIO_MEM_CHECK(TAG, gl_onboard_mic, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _onboard_dual_dmic_mic_open;
    cfg.close = _onboard_dual_dmic_mic_close;
    cfg.process = _onboard_dual_dmic_mic_process;
    cfg.destroy = _onboard_dual_dmic_mic_destroy;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read = _onboard_dual_dmic_mic_read;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;

    /* the buffer_len is the parameter of _onboard_mic_process api */
    if ((config->adc_cfg.chl_num == 2) || (config->ref_mode))
    {
        cfg.buffer_len = config->frame_size * 2;
    }
    else if (config->adc_cfg.chl_num == 1)
    {
        cfg.buffer_len = config->frame_size;
    }
    else
    {
        BK_LOGE(TAG, "chl_num: %d is not support\n", config->adc_cfg.chl_num);
        goto _onboard_mic_init_exit;
    }

    gl_onboard_mic->ref_mode = config->ref_mode;
    gl_onboard_mic->dual_dmic = config->dual_dmic;
    gl_onboard_mic->dual_dmic_sgl_out = config->dual_dmic_sgl_out;
    if(gl_onboard_mic->dual_dmic)
    {
        if(gl_onboard_mic->ref_mode)
        {
            p_mic_data_temp_save = audio_calloc(1, config->frame_size * 2);
            AUDIO_MEM_CHECK(TAG, p_mic_data_temp_save, return NULL);

            gl_onboard_mic->hw_ref_buf = audio_calloc(1, config->frame_size);
            AUDIO_MEM_CHECK(TAG, gl_onboard_mic->hw_ref_buf, return NULL);

            cfg.multi_out_port_num = 1;//for ref oputput
            cfg.buffer_len = config->frame_size*2;//DMIC:R/L;
        }
        else
        {
            cfg.buffer_len = config->frame_size*2;//DMIC:R/L;
        }
        
        BK_LOGD(TAG, "dmic:f_s:%d,buf_i_l:%d,buf_o_l:%d\n", config->frame_size, cfg.buffer_len, cfg.out_block_size);
    }

    cfg.tag = "onboard_mic";
    gl_onboard_mic->frame_size = config->frame_size;
    os_memcpy(&gl_onboard_mic->adc_cfg, &config->adc_cfg, sizeof(dual_dmic_adc_cfg_t));
    gl_onboard_mic->out_block_size = config->out_block_size;
    gl_onboard_mic->out_block_num = config->out_block_num;
    BK_LOGD(TAG, "buffer_len: %d, out_block_size: %d, out_block_num: %d\n", cfg.buffer_len, cfg.out_block_size, cfg.out_block_num);

    /* init audio adc */
    aud_adc_config_t aud_adc_cfg = DEFAULT_AUD_ADC_CONFIG();
    if (config->adc_cfg.chl_num == 1 || config->adc_cfg.chl_num == 2)
    {
        aud_adc_cfg.adc_chl = AUD_ADC_CHL_LR;
    }
    else
    {
        BK_LOGE(TAG, "adc_chl: %d is not support \n", config->adc_cfg.chl_num);
        goto _onboard_mic_init_exit;
    }
    /* check bits, must be 16bit */
    if (config->adc_cfg.bits != 16)
    {
        BK_LOGE(TAG, "bits: %d is not support, only support 16bits\n", config->adc_cfg.bits);
        goto _onboard_mic_init_exit;
    }
    aud_adc_cfg.samp_rate = config->adc_cfg.sample_rate;
    aud_adc_cfg.adc_gain = config->adc_cfg.dig_gain;
    aud_adc_cfg.clk_src = config->adc_cfg.clk_src;
    aud_adc_cfg.adc_mode = config->adc_cfg.mode;
    BK_LOGD(TAG, "adc_cfg chl_num: %d, adc_gain: 0x%02x, samp_rate: %d, clk_src: %s, adc_mode: %s \n",
            aud_adc_cfg.adc_chl, aud_adc_cfg.adc_gain, aud_adc_cfg.samp_rate, aud_adc_cfg.clk_src == 1 ? "APLL" : "XTAL", aud_adc_cfg.adc_mode == 1 ? "AUD_ADC_MODE_SIGNAL_END" : "AUD_ADC_MODE_DIFFEN");
    ret = bk_aud_adc_init(&aud_adc_cfg);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, aud_adc_init fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }
    if (config->adc_cfg.chl_num == 1)
    {
        ret = bk_aud_adc_set_mic_mode(AUD_MIC_MIC1, config->adc_cfg.mode);
    }
    else
    {
        ret = bk_aud_adc_set_mic_mode(AUD_MIC_BOTH, config->adc_cfg.mode);
    }
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "set audio adc mode:%d fail, ret: %d \r\n", config->adc_cfg.mode, ret);
        goto _onboard_mic_init_exit;
    }

    ret = aud_adc_dma_config(gl_onboard_mic);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, adc_dma_init fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }
    
    if(gl_onboard_mic->dual_dmic)
    {
        aud_dmic_config_t dmic_config = DEFAULT_AUD_DMIC_CONFIG();
        dmic_config.samp_rate = config->adc_cfg.sample_rate;
        dmic_config.dmic_chl = AUD_DMIC_CHL_LR;
        BK_LOGD(TAG, "dmic_cfg samp_rate: %d, clk_src: %s\n",aud_adc_cfg.samp_rate, dmic_config.clk_src == 1 ? "APLL" : "XTAL");
        /* init audio driver and config dmic */
        ret = bk_aud_dmic_init(&dmic_config);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "%s, %d, init audio dmic fail \n", __func__, __LINE__);
            goto _onboard_mic_init_exit;
        }

        ret = aud_dmic_dma_config(gl_onboard_mic);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, adc_dma_init fail\n", __func__, __LINE__);
            goto _onboard_mic_init_exit;
        }
    }

    rtos_init_event_flags(&gl_onboard_mic->mic_evt);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_event_flags fail\n", __func__, __LINE__);
        goto _onboard_mic_init_exit;
    }


    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _onboard_mic_init_exit);
    audio_element_setdata(el, gl_onboard_mic);

    audio_element_info_t info = {0};
    info.sample_rates = config->adc_cfg.sample_rate;
    info.channels = config->adc_cfg.chl_num;
    info.bits = config->adc_cfg.bits;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    ONBOARD_MIC_DATA_COUNT_OPEN();
    ONBOARD_MIC_DATA_DUMP_BY_UART_OPEN();

    return el;
_onboard_mic_init_exit:
    /* deinit dma */
    aud_adc_dma_deconfig(gl_onboard_mic);
    /* deinit adc */
    bk_aud_adc_deinit();
    bk_aud_driver_deinit();

    if(gl_onboard_mic->mic_evt)
    {
        rtos_deinit_event_flags(&gl_onboard_mic->mic_evt);
    }

    audio_free(gl_onboard_mic);
    gl_onboard_mic = NULL;
    return NULL;
}

bk_err_t onboard_dual_dmic_mic_stream_set_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t gain)
{
    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(onboard_mic_stream);

    /* check param */
    if (gain < 0 || gain > 0x3f)
    {
        BK_LOGE(TAG, "gain: %d is out of range: 0x00 ~ 0x3f \n", gain);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (onboard_mic->adc_cfg.dig_gain == gain)
    {
        BK_LOGD(TAG, "not need updata onboard mic digital gain \n");
        return BK_OK;
    }

    if (BK_OK == bk_aud_adc_set_gain(gain))
    {
        onboard_mic->adc_cfg.dig_gain = gain;
        audio_element_setdata(onboard_mic_stream, onboard_mic);
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, updata mic digital gain fail \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t onboard_dual_dmic_mic_stream_get_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t *gain)
{
    onboard_dual_dmic_mic_stream_t *onboard_mic = (onboard_dual_dmic_mic_stream_t *)audio_element_getdata(onboard_mic_stream);

    /* check param */
    if (gain == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, gain is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* check param */
    if (onboard_mic == NULL)
    {
        BK_LOGE(TAG, "%s, line: %d, onboard_mic is not init \n", __func__, __LINE__);
        return BK_FAIL;
    }

    *gain = onboard_mic->adc_cfg.dig_gain;

    return BK_OK;
}

