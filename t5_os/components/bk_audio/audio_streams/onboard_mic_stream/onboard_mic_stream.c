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
#include "onboard_mic_stream.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include <driver/aud_adc.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>

//#include "BK7256_RegList.h"


#define TAG  "ONBOARD_MIC"

typedef struct onboard_mic_stream {
    adc_cfg_t                adc_cfg;          /**< ADC mode configuration */
    bool                     is_open;          /**< mic enable, true: enable, false: disable */
    uint32_t                 frame_size;       /**< size of one frame mic data, the size
                                                        when AUD_MIC_CHL_MIC1 mode, the size must bean integer multiple of two bytes
                                                        when AUD_MIC_CHL_DUAL mode, the size must bean integer multiple of four bytes */
    dma_id_t                 mic_dma_id;       /**< dma id that dma carry mic data from fifo to ring buffer */
    RingBufferContext        mic_rb;           /**< mic rb handle */
    int8_t *                 mic_ring_buff;    /**< mic ring buffer address */
    uint8_t                  out_frame_num;    /**< Number of output ringbuffer, the unit is frame size(20ms) */
    beken_semaphore_t        can_process;      /**< can process */
} onboard_mic_stream_t;

static onboard_mic_stream_t *gl_onboard_mic = NULL;
static uint8_t *temp_mic_data = NULL;

static bk_err_t aud_adc_dma_deconfig(onboard_mic_stream_t *onboard_mic)
{
	bk_dma_deinit(onboard_mic->mic_dma_id);
	bk_dma_free(DMA_DEV_AUDIO, onboard_mic->mic_dma_id);
	bk_dma_driver_init();
	if (onboard_mic->mic_ring_buff) {
		ring_buffer_clear(&onboard_mic->mic_rb);
		os_free(onboard_mic->mic_ring_buff);
		onboard_mic->mic_ring_buff = NULL;
	}

	return BK_OK;
}

/* Carry one frame audio dac data(20ms) from ADC FIFO complete */
static void aud_adc_dma_finish_isr(void)
{
	bk_err_t ret = rtos_set_semaphore(&gl_onboard_mic->can_process);
	if (ret != BK_OK) {
//		BK_LOGE(TAG, "%s, rtos_set_semaphore fail \n", __func__);
		if (ring_buffer_get_fill_size(&gl_onboard_mic->mic_rb) >= gl_onboard_mic->frame_size * 2) {
			ring_buffer_read(&gl_onboard_mic->mic_rb, temp_mic_data, gl_onboard_mic->frame_size);
//			BK_LOGE(TAG, "aud_adc_dma_finish_isr, mic_fill: %d \n", ring_buffer_get_fill_size(&gl_onboard_mic->mic_rb));
		}
	}
}

static bk_err_t aud_adc_dma_config(onboard_mic_stream_t *onboard_mic)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t adc_port_addr;

	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dma_driver_init fail\n", __func__, __LINE__);
		goto exit;
	}

	//malloc dma channel
	onboard_mic->mic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((onboard_mic->mic_dma_id < DMA_ID_0) || (onboard_mic->mic_dma_id >= DMA_ID_MAX)) {
		BK_LOGE(TAG, "malloc dma fail \n");
		goto exit;
	}
	/* init 2X20ms ringbuffer */
	onboard_mic->mic_ring_buff = (int8_t *)audio_calloc(3, onboard_mic->frame_size);
	AUDIO_MEM_CHECK(TAG, onboard_mic->mic_ring_buff, return BK_FAIL);
	/* init dma channel */
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
	dma_config.src.dev = DMA_DEV_AUDIO_RX;
#endif
#if CONFIG_SOC_BK7256XX
	dma_config.src.dev = DMA_DEV_AUDIO;
#endif
//	dma_config.src.dev = DMA_DEV_AUDIO;
	dma_config.dst.dev = DMA_DEV_DTCM;
	switch (onboard_mic->adc_cfg.chl_num) {
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
		BK_LOGE(TAG, "get adc fifo address failed\r\n");
		goto exit;
	} else {
		dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.src.start_addr = adc_port_addr;
		dma_config.src.end_addr = adc_port_addr + 4;
	}
	dma_config.trans_type = DMA_TRANS_DEFAULT;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)onboard_mic->mic_ring_buff;
	dma_config.dst.end_addr = (uint32_t)onboard_mic->mic_ring_buff + (onboard_mic->frame_size) * 3;
	ret = bk_dma_init(onboard_mic->mic_dma_id, &dma_config);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dma_init fail\n", __func__, __LINE__);
		goto exit;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(onboard_mic->mic_dma_id, onboard_mic->frame_size);
	/* register dma isr */
	bk_dma_register_isr(onboard_mic->mic_dma_id, NULL, (void *)aud_adc_dma_finish_isr);
	bk_dma_enable_finish_interrupt(onboard_mic->mic_dma_id);

#if (CONFIG_SPE)
	bk_dma_set_dest_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
	bk_dma_set_src_sec_attr(onboard_mic->mic_dma_id, DMA_ATTR_SEC);
#endif

	ring_buffer_init(&onboard_mic->mic_rb, (uint8_t *)onboard_mic->mic_ring_buff, (onboard_mic->frame_size) * 3, onboard_mic->mic_dma_id, RB_DMA_TYPE_WRITE);

	BK_LOGI(TAG, "adc_dma_cfg mic_dma_id: %d, transfer_len: %d \n", onboard_mic->mic_dma_id, onboard_mic->frame_size);
	BK_LOGI(TAG, "src_start_addr: 0x%08x, src_end_addr: 0x%08x \n", dma_config.src.start_addr, dma_config.src.end_addr);
	BK_LOGI(TAG, "dst_start_addr: 0x%08x, dst_end_addr: 0x%08x \n", dma_config.dst.start_addr, dma_config.dst.end_addr);

	return BK_OK;
exit:
	aud_adc_dma_deconfig(onboard_mic);
	return BK_FAIL;
}

static bk_err_t _onboard_mic_open(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _onboard_mic_open \n", audio_element_get_tag(self));

	onboard_mic_stream_t *onboard_mic= (onboard_mic_stream_t *)audio_element_getdata(self);

	if (onboard_mic->is_open) {
		return BK_OK;
	}

	/* set read data timeout */
//	audio_element_set_input_timeout(self, 15 / portTICK_RATE_MS);
//	ring_buffer_clear(&onboard_mic->mic_rb);

	bk_err_t ret = bk_dma_start(onboard_mic->mic_dma_id);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
		return BK_FAIL;
	}
	ret = bk_aud_adc_start();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma start fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	onboard_mic->is_open = true;

	return BK_OK;
}

static int _onboard_mic_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
	BK_LOGD(TAG, "[%s] _onboard_mic_read, len: %d \n", audio_element_get_tag(self), len);

	onboard_mic_stream_t *onboard_mic= (onboard_mic_stream_t *)audio_element_getdata(self);
	int ret = BK_OK;
	uint32_t read_size = 0;
	GLOBAL_INT_DECLARATION();

	if (len) {
		BK_LOGD(TAG, "[%s] _onboard_mic_read, mic_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_mic->mic_rb));
		if (ring_buffer_get_fill_size(&onboard_mic->mic_rb) >= len) {
			//BK_LOGD(TAG, "[%s] _onboard_mic_read, mic_fill: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_mic->mic_rb));
			GLOBAL_INT_DISABLE();
			read_size = ring_buffer_read(&onboard_mic->mic_rb, (uint8_t *)buffer, len);
			GLOBAL_INT_RESTORE();
			if (read_size == len) {
				ret = read_size;
			} else {
				BK_LOGE(TAG, "The error is happened in read data. read_size: %d \n", read_size);
				ret = -1;
			}
		} else {
			BK_LOGW(TAG, "[%s] _onboard_mic_read, mic_fill: %d < len: %d \n", audio_element_get_tag(self), ring_buffer_get_fill_size(&onboard_mic->mic_rb), len);
			os_memset(buffer, 0, len);
			ret = len;
		}
	} else {
		ret = len;
	}

	return ret;
}

static int _onboard_mic_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	onboard_mic_stream_t *onboard_mic= (onboard_mic_stream_t *)audio_element_getdata(self);

	if (ring_buffer_get_fill_size(&onboard_mic->mic_rb) < in_len) {
		if (kNoErr != rtos_get_semaphore(&onboard_mic->can_process, 2000)) {//portMAX_DELAY, 25 / portTICK_RATE_MS/2000
			//return -1;
			BK_LOGE(TAG, "[%s] rtos_get_semaphore fail \n", audio_element_get_tag(self));
		} else {
			if (ring_buffer_get_fill_size(&onboard_mic->mic_rb) < in_len) {
				if (kNoErr != rtos_get_semaphore(&onboard_mic->can_process, 2000)) {//portMAX_DELAY, 25 / portTICK_RATE_MS/2000
					//return -1;
					BK_LOGE(TAG, "[%s] rtos_get_semaphore fail \n", audio_element_get_tag(self));
				}
			}
		}
	}

	BK_LOGD(TAG, "[%s] _onboard_mic_process \n", audio_element_get_tag(self));
//	addAON_GPIO_Reg0x2 = 2;

	/* read input data */
	int r_size = audio_element_input(self, in_buffer, in_len);
	int w_size = 0;
	if (r_size == AEL_IO_TIMEOUT) {
		r_size = 0;
		w_size = audio_element_output(self, in_buffer, r_size);
	} else if (r_size > 0) {
//		audio_element_multi_output(self, in_buffer, r_size, 0);
		w_size = audio_element_output(self, in_buffer, r_size);
		//更新处理数据的指针
//		audio_element_update_byte_pos(self, w_size);
	} else {
		w_size = r_size;
	}
//	addAON_GPIO_Reg0x2 = 0;

	return w_size;
}

static bk_err_t _onboard_mic_close(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _onboard_mic_close \n", audio_element_get_tag(self));

	onboard_mic_stream_t *onboard_mic= (onboard_mic_stream_t *)audio_element_getdata(self);

	bk_err_t ret = bk_dma_stop(onboard_mic->mic_dma_id);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac dma stop fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	ret = bk_aud_adc_stop();
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, dac stop fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	onboard_mic->is_open = false;

	return BK_OK;
}

static bk_err_t _onboard_mic_destroy(audio_element_handle_t self)
{
	BK_LOGI(TAG, "[%s] _onboard_mic_destroy \n", audio_element_get_tag(self));

	onboard_mic_stream_t *onboard_mic= (onboard_mic_stream_t *)audio_element_getdata(self);
	/* deinit dma */
	aud_adc_dma_deconfig(onboard_mic);
	/* deinit dac */
	bk_aud_adc_deinit();

	if (onboard_mic && onboard_mic->can_process) {
		rtos_deinit_semaphore(&onboard_mic->can_process);
		onboard_mic->can_process = NULL;
	}

	audio_free(onboard_mic);
	onboard_mic = NULL;

	if (temp_mic_data) {
		audio_free(temp_mic_data);
		temp_mic_data = NULL;
	}

	return BK_OK;
}

audio_element_handle_t onboard_mic_stream_init(onboard_mic_stream_cfg_t *config)
{
	audio_element_handle_t el;
	bk_err_t ret = BK_OK;
	gl_onboard_mic = audio_calloc(1, sizeof(onboard_mic_stream_t));
	AUDIO_MEM_CHECK(TAG, gl_onboard_mic, return NULL);

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	cfg.open = _onboard_mic_open;
	cfg.close = _onboard_mic_close;
	cfg.process = _onboard_mic_process;
	cfg.destroy = _onboard_mic_destroy;
	cfg.read = _onboard_mic_read;
	cfg.write = NULL;
	cfg.task_stack = config->task_stack;
	cfg.task_prio = config->task_prio;
	cfg.task_core = config->task_core;
	cfg.buffer_len = config->adc_cfg.chl_num * config->adc_cfg.samp_rate * config->adc_cfg.bits / 1000 / 8 * 20;
	cfg.out_rb_size = cfg.buffer_len * config->out_frame_num;

	cfg.tag = "onboard_mic";
	gl_onboard_mic->frame_size = config->adc_cfg.chl_num * config->adc_cfg.samp_rate * config->adc_cfg.bits / 1000 / 8 * 20;
	os_memcpy(&gl_onboard_mic->adc_cfg, &config->adc_cfg, sizeof(adc_cfg_t));
	gl_onboard_mic->out_frame_num= config->out_frame_num;
	BK_LOGI(TAG, "frame_size: %d, out_buffer_size: %d \n", gl_onboard_mic->frame_size, cfg.buffer_len);

	/* init audio adc */
	aud_adc_config_t aud_adc_cfg = DEFAULT_AUD_ADC_CONFIG();
	if (config->adc_cfg.chl_num == 1) {
		aud_adc_cfg.adc_chl = AUD_ADC_CHL_L;
	} else if (config->adc_cfg.chl_num == 2) {
		aud_adc_cfg.adc_chl = AUD_ADC_CHL_LR;
	} else {
		BK_LOGE(TAG, "adc_chl: %d is not support \n", config->adc_cfg.chl_num);
		goto _onboard_mic_init_exit;
	}
	//must be 16bit
	aud_adc_cfg.adc_gain = config->adc_cfg.mic_gain;
	aud_adc_cfg.samp_rate = config->adc_cfg.samp_rate;
	aud_adc_cfg.clk_src = config->adc_cfg.clk_src;
	aud_adc_cfg.adc_mode = config->adc_cfg.mode;
	BK_LOGI(TAG, "adc_cfg chl_num: %d, adc_gain: 0x%02x, samp_rate: %d, clk_src: %s, adc_mode: %s \n",
					aud_adc_cfg.adc_chl, aud_adc_cfg.adc_gain, aud_adc_cfg.samp_rate, aud_adc_cfg.clk_src == 1? "APLL":"XTAL", aud_adc_cfg.adc_mode == 1? "AUD_ADC_MODE_SIGNAL_END":"AUD_ADC_MODE_DIFFEN");
	ret = bk_aud_adc_init(&aud_adc_cfg);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, aud_adc_init fail\n", __func__, __LINE__);
		goto _onboard_mic_init_exit;
	}
	if (config->adc_cfg.chl_num == 1) {
		ret = bk_aud_adc_set_mic_mode(AUD_MIC_MIC1, config->adc_cfg.mode);
	} else {
		ret = bk_aud_adc_set_mic_mode(AUD_MIC_BOTH, config->adc_cfg.mode);
	}
	if (ret != BK_OK) {
		BK_LOGE(TAG, "set audio adc mode:%d fail, ret: %d \r\n", config->adc_cfg.mode, ret);
		goto _onboard_mic_init_exit;
	}

	ret = aud_adc_dma_config(gl_onboard_mic);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, adc_dma_init fail\n", __func__, __LINE__);
		goto _onboard_mic_init_exit;
	}

	ret = rtos_init_semaphore(&gl_onboard_mic->can_process, 1);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
		goto _onboard_mic_init_exit;
	}

	el = audio_element_init(&cfg);
	AUDIO_MEM_CHECK(TAG, el, goto _onboard_mic_init_exit);
	audio_element_setdata(el, gl_onboard_mic);

	audio_element_info_t info = {0};
	info.sample_rates = config->adc_cfg.samp_rate;
	info.channels = config->adc_cfg.chl_num;
	info.bits = config->adc_cfg.bits;
	info.codec_fmt = BK_CODEC_TYPE_PCM;
	audio_element_setinfo(el, &info);

	temp_mic_data = (uint8_t *)audio_calloc(1, gl_onboard_mic->frame_size);

	return el;
_onboard_mic_init_exit:
	/* deinit dma */
	aud_adc_dma_deconfig(gl_onboard_mic);
	/* deinit adc */
	bk_aud_adc_deinit();
	bk_aud_driver_deinit();
	if (gl_onboard_mic->can_process) {
		rtos_deinit_semaphore(&gl_onboard_mic->can_process);
		gl_onboard_mic->can_process = NULL;
	}

	audio_free(gl_onboard_mic);
	gl_onboard_mic = NULL;
	return NULL;
}

