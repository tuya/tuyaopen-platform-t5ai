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
#include "onboard_speaker_stream.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include <driver/aud_dac.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/uart.h>
#include "gpio_driver.h"

//#include "BK7256_RegList.h"

#define TAG  "ONBOARD_SPEAKER"

typedef struct onboard_speaker_stream {
    uint8_t                  chl_num;          /**< speaker channel number */
    uint32_t                 samp_rate;        /**< speaker sample rate */
    uint8_t                  spk_gain;         /**< audio dac gain: value range:0x0 ~ 0x3f, suggest:0x2d */
    aud_dac_work_mode_t      work_mode;        /**< audio dac mode: signal_ended/differen */
    uint8_t                  bits;             /**< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t                clk_src;          /**< audio clock: XTAL(26MHz)/APLL */
    bool                     is_open;          /**< speaker enable, true: enable, false: disable */
    uint32_t                 frame_size;       /**< size of one frame speaker data, the size
                                                        when AUD_DAC_CHL_L_ENABLE mode, the size must bean integer multiple of two bytes
                                                        when AUD_DAC_CHL_LR_ENABLE mode, the size must bean integer multiple of four bytes */
    dma_id_t                 spk_dma_id;       /**< dma id that dma carry spk data from ring buffer to fifo */
    RingBufferContext        spk_rb;           /**< speaker rb handle */
    int8_t *                 spk_ring_buff;    /**< speaker ring buffer addr */
    uint8_t                  pool_frame_num;   /**< speaker data pool size, the unit is frame size(20ms) */
    uint8_t                  pool_play_thold;  /**< the play threshold of pool, the unit is frame size(20ms) */
    uint8_t                  pool_pause_thold; /**< the pause threshold of pool, the unit is frame size(20ms) */
    RingBufferContext        pool_rb;          /**< the pool ringbuffer handle */
    int8_t *                 pool_ring_buff;   /**< pool ring buffer addr */
    bool                     pool_can_read;    /**< the pool if can read */
    beken_semaphore_t        can_process;      /**< can process */
    int8_t *                 temp_buff;        /**< temp buffer addr used to save data written to speaker ring buffer */
} onboard_speaker_stream_t;

static onboard_speaker_stream_t *onboard_speaker = NULL;


static bk_err_t aud_dac_dma_deconfig(onboard_speaker_stream_t *onboard_spk)
{
	bk_dma_deinit(onboard_spk->spk_dma_id);
	bk_dma_free(DMA_DEV_AUDIO, onboard_spk->spk_dma_id);
	bk_dma_driver_init();
	if (onboard_spk->spk_ring_buff) {
		ring_buffer_clear(&onboard_spk->spk_rb);
		os_free(onboard_spk->spk_ring_buff);
		onboard_spk->spk_ring_buff = NULL;
	}

	return BK_OK;
}


/* Carry one frame audio dac data(20ms) to DAC FIFO complete */
static void aud_dac_dma_finish_isr(void)
{
	//xSemaphoreGiveFromISR(onboard_speaker->can_process, &xHigherPriorityTaskWoken);
	bk_err_t ret = rtos_set_semaphore(&onboard_speaker->can_process);
	if (ret != BK_OK) {
		BK_LOGD(TAG, "%s, rtos_set_semaphore fail \n", __func__);
#if 0
		/* write data to speaker ring buffer immediately */
		if (onboard_spk->pool_can_read) {
			uint32_t read_size = ring_buffer_read(&onboard_spk->pool_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
			if (read_size != onboard_spk->frame_size) {
				BK_LOGE(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, onboard_spk->frame_size);
			}
		} else {
			os_memset(onboard_spk->temp_buff, 0x00, onboard_spk->frame_size);
			BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
		}

	//	addAON_GPIO_Reg0x9 = 2;
		ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
	//	addAON_GPIO_Reg0x9 = 0;
#endif
	}
}

static bk_err_t aud_dac_dma_config(onboard_speaker_stream_t *onboard_spk)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t dac_port_addr;

	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dma_driver_init fail\n", __func__, __LINE__);
		goto exit;
	}

	//malloc dma channel
	onboard_spk->spk_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((onboard_spk->spk_dma_id < DMA_ID_0) || (onboard_spk->spk_dma_id >= DMA_ID_MAX)) {
		BK_LOGE(TAG, "malloc dma fail \n");
		goto exit;
	}
	/* init 2X20ms ringbuffer */
	onboard_spk->spk_ring_buff = (int8_t *)audio_calloc(2, onboard_spk->frame_size);
	AUDIO_MEM_CHECK(TAG, onboard_spk->spk_ring_buff, return BK_FAIL);
	ring_buffer_init(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->spk_ring_buff, onboard_spk->frame_size * 2, onboard_spk->spk_dma_id, RB_DMA_TYPE_READ);
	BK_LOGI(TAG, "%s, %d, spk_ring_buff: %p, spk_ring_buff size: %d \n", __func__, __LINE__, onboard_spk->spk_ring_buff, onboard_spk->frame_size * 2);
	/* init dma channel */
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.dst.dev = DMA_DEV_AUDIO;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	switch (onboard_spk->chl_num) {
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
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, get dac fifo address fail\n", __func__, __LINE__);
		goto exit;
	}
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = dac_port_addr;
	dma_config.dst.end_addr = dac_port_addr + 4;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = (uint32_t)onboard_spk->spk_ring_buff;
	dma_config.src.end_addr = (uint32_t)(onboard_spk->spk_ring_buff) + onboard_spk->frame_size * 2;
	ret = bk_dma_init(onboard_spk->spk_dma_id, &dma_config);
	if (ret != BK_OK) {
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
	BK_LOGI(TAG, "[%s] _onboard_speaker_open \n", audio_element_get_tag(self));

	onboard_speaker_stream_t *onboard_spk= (onboard_speaker_stream_t *)audio_element_getdata(self);

	if (onboard_spk->is_open) {
		return BK_OK;
	}

	/* set read data timeout */
	audio_element_set_input_timeout(self, 2000);	//15 / portTICK_RATE_MS

	uint32_t free_size = ring_buffer_get_free_size(&onboard_speaker->pool_rb);
	if (free_size) {
		uint8_t *temp_data = (uint8_t *)audio_malloc(free_size - onboard_speaker->frame_size);
		AUDIO_MEM_CHECK(TAG, temp_data, return BK_FAIL);
		os_memset(temp_data, 0x00, free_size - onboard_speaker->frame_size);
		ring_buffer_write(&onboard_speaker->pool_rb, temp_data, free_size - onboard_speaker->frame_size);
		audio_free(temp_data);
		temp_data = NULL;
	}

	free_size = ring_buffer_get_free_size(&onboard_speaker->spk_rb);
	if (free_size) {
		uint8_t *temp_data = (uint8_t *)audio_malloc(free_size);
		AUDIO_MEM_CHECK(TAG, temp_data, return BK_FAIL);
		os_memset(temp_data, 0x00, free_size);
		ring_buffer_write(&onboard_speaker->spk_rb, temp_data, free_size);
		audio_free(temp_data);
		temp_data = NULL;
	}

	bk_err_t ret = bk_dma_start(onboard_spk->spk_dma_id);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	ret = bk_aud_dac_start();
	ret = bk_aud_dac_start();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	onboard_spk->is_open = true;
	onboard_spk->pool_can_read = true;

	return BK_OK;
}

static int _onboard_speaker_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
	BK_LOGD(TAG, "[%s] _onboard_speaker_write, len: %d \n", audio_element_get_tag(self), len);

	onboard_speaker_stream_t *onboard_spk= (onboard_speaker_stream_t *)audio_element_getdata(self);
	int ret = BK_OK;
	uint32_t write_size = 0;

	if (len) {
		//write some data to speaker pool
		if (ring_buffer_get_free_size(&onboard_spk->pool_rb) >= len) {
			//BK_LOGD(TAG, "[%s] _onboard_speaker_write, pool_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_spk->pool_rb));
			write_size = ring_buffer_write(&onboard_spk->pool_rb, (uint8_t *)buffer, len);
			if (write_size == len) {
				ret = write_size;
			} else {
				BK_LOGE(TAG, "The error is happened in writing data. write_size: %d \n", write_size);
				ret = -1;
			}
			//BK_LOGD(TAG, "[%s] _onboard_speaker_write, pool_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_spk->pool_rb));
		}
	} else {
		ret = len;
	}

	/* check pool pause threshold */
	if (onboard_spk->pool_can_read) {
		if (ring_buffer_get_fill_size(&onboard_spk->pool_rb) <= onboard_spk->frame_size * onboard_spk->pool_pause_thold) {
			BK_LOGE(TAG, "pause pool read, pool_fill: %d <= %d \n", ring_buffer_get_fill_size(&onboard_spk->pool_rb), onboard_spk->frame_size * onboard_spk->pool_pause_thold);
			onboard_spk->pool_can_read = false;
		}
	} else {
		if (ring_buffer_get_fill_size(&onboard_spk->pool_rb) >= onboard_spk->frame_size * onboard_spk->pool_play_thold) {
			BK_LOGE(TAG, "start pool read \n");
			onboard_spk->pool_can_read = true;
		}
	}

	return ret;
}

static int _onboard_speaker_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	onboard_speaker_stream_t *onboard_spk= (onboard_speaker_stream_t *)audio_element_getdata(self);

	if (pdTRUE != rtos_get_semaphore(&onboard_spk->can_process, 2000)) {//portMAX_DELAY, 25 / portTICK_RATE_MS
		//return -1;
	}

	BK_LOGD(TAG, "[%s] _onboard_speaker_process \n", audio_element_get_tag(self));

	/* write data to speaker ring buffer immediately */
	if (onboard_spk->pool_can_read) {
		uint32_t read_size = ring_buffer_read(&onboard_spk->pool_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
		if (read_size != onboard_spk->frame_size) {
			BK_LOGE(TAG, "read size: %d, need_size: %d is incorrect \n", read_size, onboard_spk->frame_size);
		}
	} else {
		os_memset(onboard_spk->temp_buff, 0x00, onboard_spk->frame_size);
		BK_LOGW(TAG, "[%s] fill silence data \n", audio_element_get_tag(self));
	}

//	addAON_GPIO_Reg0x9 = 2;
	ring_buffer_write(&onboard_spk->spk_rb, (uint8_t *)onboard_spk->temp_buff, onboard_spk->frame_size);
//	addAON_GPIO_Reg0x9 = 0;

	/* write data to ref ring buffer */
	audio_element_multi_output(self, (char *)onboard_spk->temp_buff, onboard_spk->frame_size, 0);

	/* read input data */
	int r_size = audio_element_input(self, in_buffer, in_len);
	int w_size = 0;
	if (r_size == AEL_IO_TIMEOUT) {
		r_size = 0;
		//w_size = audio_element_output(self, in_buffer, r_size);
	} else if (r_size > 0) {
//		audio_element_multi_output(self, in_buffer, r_size, 0);
		w_size = audio_element_output(self, in_buffer, r_size);
		//更新处理数据的指针
//		audio_element_update_byte_pos(self, w_size);
	} else {
		w_size = r_size;
	}

	return w_size;
}

static bk_err_t _onboard_speaker_close(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _onboard_speaker_close \n", audio_element_get_tag(self));

	onboard_speaker_stream_t *onboard_spk= (onboard_speaker_stream_t *)audio_element_getdata(self);

	bk_err_t ret = bk_dma_stop(onboard_spk->spk_dma_id);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma stop fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	ret = bk_aud_dac_stop();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac stop fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	onboard_spk->is_open = false;
	onboard_spk->pool_can_read = false;

	return BK_OK;
}

static bk_err_t _onboard_speaker_destroy(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _onboard_speaker_destroy \n", audio_element_get_tag(self));

	onboard_speaker_stream_t *onboard_spk= (onboard_speaker_stream_t *)audio_element_getdata(self);
	/* deinit dma */
	aud_dac_dma_deconfig(onboard_spk);
	/* deinit dac */
	bk_aud_dac_deinit();

	/* free spk pool */
	if (onboard_spk && onboard_spk->pool_ring_buff) {
		ring_buffer_clear(&onboard_spk->pool_rb);
		audio_free(onboard_spk->pool_ring_buff);
		onboard_spk->pool_ring_buff = NULL;
	}
	if (onboard_spk && onboard_spk->temp_buff) {
		audio_free(onboard_spk->temp_buff);
		onboard_spk->temp_buff = NULL;
	}
	if (onboard_spk && onboard_spk->can_process) {
		rtos_deinit_semaphore(&onboard_spk->can_process);
		onboard_spk->can_process = NULL;
	}

	if (onboard_spk) {
		audio_free(onboard_spk);
		onboard_spk = NULL;
	}

	return BK_OK;
}

audio_element_handle_t onboard_speaker_stream_init(onboard_speaker_stream_cfg_t *config)
{
	audio_element_handle_t el;
	bk_err_t ret = BK_OK;
	onboard_speaker = audio_calloc(1, sizeof(onboard_speaker_stream_t));
	AUDIO_MEM_CHECK(TAG, onboard_speaker, return NULL);

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	cfg.open = _onboard_speaker_open;
	cfg.close = _onboard_speaker_close;
	cfg.process = _onboard_speaker_process;
	cfg.destroy = _onboard_speaker_destroy;
	cfg.write = _onboard_speaker_write;
	cfg.read = NULL;
	cfg.task_stack = config->task_stack;
	cfg.task_prio = config->task_prio;
	cfg.task_core = config->task_core;
	cfg.buffer_len = config->chl_num * config->samp_rate * config->bits / 1000 / 8 * 20;
	cfg.multi_out_rb_num = config->multi_out_rb_num;
	BK_LOGD(TAG, "cfg.buffer_len: %d\n", cfg.buffer_len);

	cfg.tag = "onboard_speaker";
	onboard_speaker->chl_num = config->chl_num;
	onboard_speaker->samp_rate = config->samp_rate;
	onboard_speaker->spk_gain = config->spk_gain;
	onboard_speaker->work_mode = config->work_mode;
	onboard_speaker->bits = config->bits;
	onboard_speaker->clk_src = config->clk_src;
	onboard_speaker->frame_size = config->chl_num * config->samp_rate * config->bits / 1000 / 8 * 20;
	onboard_speaker->pool_frame_num = config->pool_frame_num;
	onboard_speaker->pool_play_thold = config->pool_play_thold;
	onboard_speaker->pool_pause_thold = config->pool_pause_thold;

	/* init onboard speaker */
	aud_dac_config_t aud_dac_cfg = DEFAULT_AUD_DAC_CONFIG();
	aud_dac_cfg.work_mode = config->work_mode;
	if (config->chl_num == 1) {
		aud_dac_cfg.dac_chl = AUD_DAC_CHL_L;
	} else if (config->chl_num == 2) {
		aud_dac_cfg.dac_chl = AUD_DAC_CHL_LR;
	} else {
		BK_LOGE(TAG, "dac_chl: %d is not support \n", config->chl_num);
		goto _onboard_speaker_init_exit;
	}
	aud_dac_cfg.samp_rate = config->samp_rate;
	aud_dac_cfg.clk_src = config->clk_src;
	aud_dac_cfg.dac_gain = config->spk_gain;
	BK_LOGI(TAG, "dac_cfg chl_num: %d, dac_gain: 0x%02x, samp_rate: %d, clk_src: %s, dac_mode: %s \n",
					aud_dac_cfg.dac_chl == AUD_DAC_CHL_L? "AUD_DAC_CHL_L":"AUD_DAC_CHL_LR",
					aud_dac_cfg.dac_gain,
					aud_dac_cfg.samp_rate,
					aud_dac_cfg.clk_src == 1? "APLL":"XTAL",
					aud_dac_cfg.work_mode == 1? "AUD_DAC_WORK_MODE_SIGNAL_END":"AUD_DAC_WORK_MODE_DIFFEN");
	ret = bk_aud_dac_init(&aud_dac_cfg);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, aud_dac_init fail\n", __func__, __LINE__);
		goto _onboard_speaker_init_exit;
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
	ret = aud_dac_dma_config(onboard_speaker);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac_dma_init fail\n", __func__, __LINE__);
		goto _onboard_speaker_init_exit;
	}

	/* init nX20ms speaker ringbuffer pool */
	onboard_speaker->pool_ring_buff = (int8_t *)audio_calloc(onboard_speaker->pool_frame_num + 1, onboard_speaker->frame_size);
	AUDIO_MEM_CHECK(TAG, onboard_speaker->pool_ring_buff, goto _onboard_speaker_init_exit);
	ring_buffer_init(&onboard_speaker->pool_rb, (uint8_t *)onboard_speaker->pool_ring_buff, (onboard_speaker->pool_frame_num + 1) * onboard_speaker->frame_size, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	onboard_speaker->temp_buff = (int8_t *)audio_calloc(1, onboard_speaker->frame_size);
	os_memset(onboard_speaker->temp_buff, 0x00, onboard_speaker->frame_size);

	//onboard_speaker->can_process  = xSemaphoreCreateBinary();
	ret = rtos_init_semaphore(&onboard_speaker->can_process, 1);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
		goto _onboard_speaker_init_exit;
	}

	el = audio_element_init(&cfg);
	AUDIO_MEM_CHECK(TAG, el, goto _onboard_speaker_init_exit);
	audio_element_setdata(el, onboard_speaker);

	audio_element_info_t info = {0};
	info.sample_rates = config->samp_rate;
	info.channels = config->chl_num;
	info.bits = config->bits;
	info.codec_fmt = BK_CODEC_TYPE_PCM;
	audio_element_setinfo(el, &info);

	return el;
_onboard_speaker_init_exit:
	/* deinit dma */
	aud_dac_dma_deconfig(onboard_speaker);
	/* deinit dac */
	bk_aud_dac_deinit();
	bk_aud_driver_deinit();
	/* free spk pool */
	if (onboard_speaker->pool_ring_buff) {
		ring_buffer_clear(&onboard_speaker->pool_rb);
		audio_free(onboard_speaker->pool_ring_buff);
		onboard_speaker->pool_ring_buff = NULL;
	}
	if (onboard_speaker->temp_buff) {
		audio_free(onboard_speaker->temp_buff);
		onboard_speaker->temp_buff = NULL;
	}
	if (onboard_speaker->can_process) {
		rtos_deinit_semaphore(&onboard_speaker->can_process);
		onboard_speaker->can_process = NULL;
	}

	audio_free(onboard_speaker);
	onboard_speaker = NULL;
	return NULL;
}

static bk_err_t audio_dac_reconfig(onboard_speaker_stream_t *onboard_spk, int rate, int ch, int bits)
{
	/* check and set sample rate */
	if (onboard_spk->samp_rate != rate) {
		if (BK_OK != bk_aud_dac_set_samp_rate(rate)) {
			BK_LOGE(TAG, "%s, line: %d, updata onboard speaker sample rate: %d fail \n", __func__, __LINE__, rate);
			return BK_FAIL;
		} else {
			BK_LOGI(TAG, "%s, line: %d, updata onboard speaker sample rate: %d ok \n", __func__, __LINE__, rate);
		}
	}

	/* check and set channel num */
	if (onboard_spk->chl_num != ch) {
		aud_dac_chl_t chl_cfg = AUD_DAC_CHL_L;
		if (ch == 1) {
			chl_cfg = AUD_DAC_CHL_L;
		} else {
			chl_cfg = AUD_DAC_CHL_LR;
		}
		if (BK_OK != bk_aud_dac_set_chl(chl_cfg)) {
			BK_LOGE(TAG, "%s, line: %d, updata onboard speaker channel: %d fail \n", __func__, __LINE__, ch);
			return BK_FAIL;
		} else {
			BK_LOGI(TAG, "%s, line: %d, updata onboard speaker channel: %d fail \n", __func__, __LINE__, ch);
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

bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int ch, int bits)
{
	bk_err_t err = BK_OK;
	onboard_speaker_stream_t *onboard_spk = (onboard_speaker_stream_t *)audio_element_getdata(onboard_speaker_stream);
	audio_element_state_t state = audio_element_get_state(onboard_speaker_stream);

	/* check param */
	if (rate != 8000 && rate != 110250 && rate != 12000 && rate != 16000 && rate != 22050 && rate != 24000 && rate != 32000 && rate != 44100 && rate != 48000) {
		BK_LOGE(TAG, "sample rate: %d is not support \n", rate);
		return BK_FAIL;
	}
	if (ch < 1 || ch > 2) {
		BK_LOGE(TAG, "dac_chl: %d is not support \n", ch);
		return BK_FAIL;
	}
	if (bits != 16) {
		BK_LOGE(TAG, "bits: %d is not support \n", bits);
		return BK_FAIL;
	}

	if (onboard_spk->samp_rate == rate && onboard_spk->chl_num == ch && onboard_spk->bits == bits) {
		BK_LOGI(TAG, "not need updata onboard speaker \n");
		return BK_OK;
	}

	if (state == AEL_STATE_RUNNING) {
		audio_element_pause(onboard_speaker_stream);
	}

	if (BK_OK == audio_dac_reconfig(onboard_spk, rate, ch, bits)) {
		onboard_spk->samp_rate = rate;
		onboard_spk->chl_num = ch;
		onboard_spk->bits = bits;
		audio_element_setdata(onboard_speaker_stream, onboard_spk);
	} else {
		BK_LOGE(TAG, "%s, line: %d, updata onboard speaker config fail \n", __func__, __LINE__);
		err = BK_FAIL;
	}

	audio_element_set_music_info(onboard_speaker_stream, rate, ch, bits);

	if (state == AEL_STATE_RUNNING) {
		audio_element_resume(onboard_speaker_stream, 0, 0);
	}

	return err;
}


