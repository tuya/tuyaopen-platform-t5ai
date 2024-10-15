// Copyright 2020-2021 Beken
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

#include "cli.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/dma.h>
#include "sys_driver.h"

#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <driver/uart.h>
#include "gpio_driver.h"

//#define UART_DUMP_DEBUG

#define ADC_FRAME_SIZE    (1280*4)
static dma_id_t adc_dma_id = 0;
static dma_id_t dac_dma_id = 0;
static uint8_t *adc_ringbuff_addr = NULL;
static uint8_t *dac_ringbuff_addr = NULL;
static RingBufferContext adc_rb;
static RingBufferContext dac_rb;
static uint16_t *adc_temp = NULL;

/*
aud_ate_test X1 X2 X3 X4

X1: test control
00: stop test
01: start test

X2: test mode
00: loop
01: adc (IIS OUT,master)
02: dac (IIS IN,master)
03: dac & adc (IIS IN,master)

X3:sample rate setting
1-9: 8K-48K

X4：adc and dac mode
00: differential
01: single-ended
*/

static void cli_aud_ate_help(void)
{
	os_printf("aud_ate_test {X1 X2 X3 X4} \r\n");
	os_printf("X1: test control \n\r00: stop test\n\r01: start test \r\n");
	os_printf("X2: test mode \n\r00: loop\n\r01: adc (IIS OUT,master)\n\r02: dac (IIS IN,master)\n\r03: dac & adc (IIS IN,master)\r\n");
	os_printf("X3:sample rate setting \n\r1-9: 8K-48K \r\n");
	os_printf("X4：adc and dac mode \n\r00: differential\n\r01: single-ended \r\n");
}


/**
 * @brief     audio loop test
 *
 * @param mic_id test mic number
 *
 * @return none
 */
void audio_loop_test(uint32_t state)
{
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	if (state == 1) {

		adc_config.adc_chl = AUD_ADC_CHL_LR;
		adc_config.samp_rate = 16000;
		adc_config.clk_src = AUD_CLK_XTAL;

		dac_config.dac_chl = AUD_DAC_CHL_LR;
		dac_config.samp_rate = 16000;
		dac_config.clk_src = AUD_CLK_XTAL;

		bk_aud_adc_init(&adc_config);
		bk_aud_dac_init(&dac_config);
		/* audio process test */
		sys_drv_aud_dacg_set(0);
		sys_drv_aud_mic1_gain_set(0);
		sys_drv_aud_mic2_gain_set(0);

		//disable audio interrupt when loop test
		sys_drv_aud_int_en(0);
		bk_aud_adc_disable_int();

		//start adc
		bk_aud_adc_start();
		//start adc
		bk_aud_dac_start();

		//enable adc to dac loop test
		bk_aud_adc_start_loop_test();
	}else {
		//disable adc and dac
		bk_aud_adc_stop();
		bk_aud_dac_stop();

		//stop loop test
		bk_aud_adc_stop_loop_test();
		bk_aud_adc_deinit();
		bk_aud_dac_deinit();
	}
}

#ifdef UART_DUMP_DEBUG
static void uart_dump_mic_data(uart_id_t id, uint32_t baud_rate)
{
	uart_config_t config = {0};
	os_memset(&config, 0, sizeof(uart_config_t));
	if (id == 0) {
		gpio_dev_unmap(GPIO_10);
		gpio_dev_map(GPIO_10, GPIO_DEV_UART1_RXD);
		gpio_dev_unmap(GPIO_11);
		gpio_dev_map(GPIO_11, GPIO_DEV_UART1_TXD);
	} else if (id == 2) {
		gpio_dev_unmap(GPIO_40);
		gpio_dev_map(GPIO_40, GPIO_DEV_UART3_RXD);
		gpio_dev_unmap(GPIO_41);
		gpio_dev_map(GPIO_41, GPIO_DEV_UART3_TXD);
	} else {
		gpio_dev_unmap(GPIO_0);
		gpio_dev_map(GPIO_0, GPIO_DEV_UART2_TXD);
		gpio_dev_unmap(GPIO_1);
		gpio_dev_map(GPIO_1, GPIO_DEV_UART2_RXD);
	}

	config.baud_rate = baud_rate;
	config.data_bits = UART_DATA_8_BITS;
	config.parity = UART_PARITY_NONE;
	config.stop_bits = UART_STOP_BITS_1;
	config.flow_ctrl = UART_FLOWCTRL_DISABLE;
	config.src_clk = UART_SCLK_XTAL_26M;

	if (bk_uart_init(id, &config) != BK_OK) {
		os_printf("init uart fail \r\n");
	} else {
		os_printf("init uart ok \r\n");
	}
}
#endif


/* 搬运audio adc 采集到的一帧mic和ref信号后，触发中断通知AEC处理数据 */
static void audio_adc_dma_finish_isr(void)
{
//	GPIO_UP(2);

	/* read adc data from adc ringbuffer */
	//os_printf("adc_rb fill size, size:%d \r\n", ring_buffer_get_fill_size(&adc_rb));
	uint32_t size = ring_buffer_read(&adc_rb, (uint8_t*)adc_temp, ADC_FRAME_SIZE);
	if (size != ADC_FRAME_SIZE) {
		os_printf("read mic_ring_buff fail, size: %d, need: %d \r\n", size, ADC_FRAME_SIZE);
		//return BK_FAIL;
	} else {
		//os_printf("read ok size: %d, need: %d \r\n", size, ADC_FRAME_SIZE);
	}

	/* select r channel data */
	for (uint32_t i = 0; i < ADC_FRAME_SIZE/2; i++) {
		adc_temp[i] = adc_temp[2*i+1];
	}

#ifdef UART_DUMP_DEBUG
		bk_uart_write_bytes(UART_ID_1, adc_temp, ADC_FRAME_SIZE/2);
#endif

	size = ring_buffer_get_free_size(&dac_rb);
	if (size >= ADC_FRAME_SIZE/2) {
		ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);
	} else {
		os_printf("dac_rb free size, size:%d \r\n", size);
	}

//	GPIO_DOWN(2);
}

void audio_adc_mic2_to_dac_test_start(void)
{
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
	adc_config.adc_chl = AUD_ADC_CHL_LR;
	adc_config.clk_src = AUD_CLK_XTAL;
	adc_config.adc_gain = 0x15;
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
	dac_config.clk_src = AUD_CLK_XTAL;
	dac_config.dac_gain = 0x3f;
	dma_config_t dma_config = {0};
	dma_config_t dac_dma_config = {0};
	uint32_t aud_adc_data_addr;
	uint32_t aud_dac_data_addr;
	bk_err_t ret = BK_OK;

#ifdef UART_DUMP_DEBUG
	uart_dump_mic_data(1, 2000000);
#endif

	bk_aud_adc_init(&adc_config);

	//disable audio interrupt when loop test
	sys_drv_aud_int_en(0);
	bk_aud_adc_disable_int();
	
	//start adc
//	bk_aud_adc_start();

	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		os_printf("[%s]bk_dma_driver_init fail, line: %d \n", __func__, __LINE__);
		return;
	}

	/* allocate free DMA channel */
	adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((adc_dma_id < DMA_ID_0) || (adc_dma_id >= DMA_ID_MAX)) {
		os_printf("[%s]bk_dma_alloc fail, line: %d \n", __func__, __LINE__);
		return;
	}

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.trans_type = DMA_TRANS_DEFAULT;

	/* get audio adc and dac fifo address */
	bk_aud_adc_get_fifo_addr(&aud_adc_data_addr);

	/* init ringbuff */
	adc_ringbuff_addr = (uint8_t *)os_malloc(ADC_FRAME_SIZE*2);
	ring_buffer_init(&adc_rb, adc_ringbuff_addr, ADC_FRAME_SIZE*2, adc_dma_id, RB_DMA_TYPE_WRITE);

	adc_temp = (uint16_t *)os_malloc(ADC_FRAME_SIZE);

	/* audio adc to dtcm by dma */
	dma_config.src.dev = DMA_DEV_AUDIO_RX;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.start_addr = (uint32_t)adc_ringbuff_addr;
	dma_config.dst.end_addr = (uint32_t)adc_ringbuff_addr + ADC_FRAME_SIZE*2;
	dma_config.src.start_addr = aud_adc_data_addr;
	dma_config.src.end_addr = aud_adc_data_addr + 4;

	/* init dma channel */
	ret = bk_dma_init(adc_dma_id, &dma_config);
	if (ret != BK_OK) {
		os_printf("[%s]bk_dma_init fail, line: %d \n", __func__, __LINE__);
		return;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(adc_dma_id, ADC_FRAME_SIZE);

	os_printf("adc ring_buff size: %d, dma transfer len: %d \n", ADC_FRAME_SIZE*2, ADC_FRAME_SIZE);

#if (CONFIG_SPE)
	bk_dma_set_dest_sec_attr(adc_dma_id, DMA_ATTR_SEC);
	bk_dma_set_src_sec_attr(adc_dma_id, DMA_ATTR_SEC);
#endif

	//register isr
	bk_dma_register_isr(adc_dma_id, NULL, (void *)audio_adc_dma_finish_isr);
	bk_dma_enable_finish_interrupt(adc_dma_id);

	/* dac config */
	bk_aud_dac_init(&dac_config);

	//disable audio interrupt when loop test
//	bk_aud_dac_disable_int();

	//start dac
//	bk_aud_dac_start();

	/* allocate free DMA channel */
	dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((dac_dma_id < DMA_ID_0) || (dac_dma_id >= DMA_ID_MAX)) {
		os_printf("[%s]bk_dma_alloc fail, line: %d \n", __func__, __LINE__);
		return;
	}

	dac_dma_config.mode = DMA_WORK_MODE_REPEAT;
	dac_dma_config.chan_prio = 1;
	dac_dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dac_dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	dac_dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dac_dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dac_dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dac_dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dac_dma_config.trans_type = DMA_TRANS_DEFAULT;

	/* get audio adc and dac fifo address */
	bk_aud_dac_get_fifo_addr(&aud_dac_data_addr);

	/* init ringbuff */
	dac_ringbuff_addr = (uint8_t *)os_malloc(ADC_FRAME_SIZE);
	ring_buffer_init(&dac_rb, dac_ringbuff_addr, ADC_FRAME_SIZE, dac_dma_id, RB_DMA_TYPE_READ);

	/* audio adc to dtcm by dma */
	dac_dma_config.src.dev = DMA_DEV_DTCM;
	dac_dma_config.dst.dev = DMA_DEV_AUDIO;
	dac_dma_config.dst.start_addr = aud_dac_data_addr;
	dac_dma_config.dst.end_addr = aud_dac_data_addr + 2;
	dac_dma_config.src.start_addr = (uint32_t)dac_ringbuff_addr;
	dac_dma_config.src.end_addr = (uint32_t)dac_ringbuff_addr + ADC_FRAME_SIZE;

	/* init dma channel */
	ret = bk_dma_init(dac_dma_id, &dac_dma_config);
	if (ret != BK_OK) {
		os_printf("[%s]bk_dma_init fail, line: %d \n", __func__, __LINE__);
		return;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dac_dma_id, ADC_FRAME_SIZE/2);

	os_memset(adc_temp, 0, ADC_FRAME_SIZE/2);
	ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);
	os_printf("dac ring_buff size: %d, dma transfer len: %d \n", ADC_FRAME_SIZE, ADC_FRAME_SIZE/2);

#if (CONFIG_SPE)
	bk_dma_set_dest_sec_attr(dac_dma_id, DMA_ATTR_SEC);
	bk_dma_set_src_sec_attr(dac_dma_id, DMA_ATTR_SEC);
#endif

	/* start dac and adc */
	bk_aud_dac_start();
	bk_aud_dac_start();
	bk_aud_adc_start();

	/* start dma */
	bk_dma_start(dac_dma_id);
	bk_dma_start(adc_dma_id);

}

void audio_adc_mic2_to_dac_test_stop(void)
{

	for (uint8_t i = 0; i < DMA_ID_MAX; i++) {
		if (bk_dma_user(i) == DMA_DEV_AUDIO) {
			bk_dma_stop(i);
			bk_dma_deinit(i);
			bk_dma_free(DMA_DEV_AUDIO, i);
		}
	}

	bk_dma_driver_deinit();

	if (adc_ringbuff_addr) {
		ring_buffer_clear(&adc_rb);
		os_free(adc_ringbuff_addr);
		adc_ringbuff_addr = NULL;
	}

	if (dac_ringbuff_addr) {
		ring_buffer_clear(&dac_rb);
		os_free(dac_ringbuff_addr);
		dac_ringbuff_addr = NULL;
	}

	if (adc_temp) {
		os_free(adc_temp);
		adc_temp =NULL;
	}

	bk_aud_adc_stop();
	bk_aud_adc_deinit();

	bk_aud_dac_stop();
	bk_aud_dac_deinit();

}


/* audio auto test */
static void audio_auto_loop_test(uint32_t state, uint32_t sample_rate, uint32_t dac_mode)
{
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	if (state == 1) {
		adc_config.adc_chl = AUD_ADC_CHL_LR;
		adc_config.clk_src = AUD_CLK_XTAL;
		dac_config.dac_chl = AUD_DAC_CHL_LR;
		dac_config.clk_src = AUD_CLK_XTAL;
		switch (sample_rate) {
			case 1:
				adc_config.samp_rate = 8000;
				dac_config.samp_rate = 8000;
				break;

			case 2:
				adc_config.samp_rate = 11025;
				dac_config.samp_rate = 11025;
				break;

			case 3:
				adc_config.samp_rate = 12000;
				dac_config.samp_rate = 12000;
				break;

			case 4:
				adc_config.samp_rate = 16000;
				dac_config.samp_rate = 16000;
				break;

			case 5:
				adc_config.samp_rate = 22050;
				dac_config.samp_rate = 22050;
				break;
			
			case 6:
				adc_config.samp_rate = 24000;
				dac_config.samp_rate = 24000;
				break;
			
			case 7:
				adc_config.samp_rate = 32000;
				dac_config.samp_rate = 32000;
				break;

			case 8:
				adc_config.samp_rate = 44100;
				dac_config.samp_rate = 44100;
				break;

			case 9:
				adc_config.samp_rate = 48000;
				dac_config.samp_rate = 48000;
				break;

			default:
				break;
		}

		if (dac_mode == 0) {
			dac_config.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
		} else if (dac_mode == 1) {
			dac_config.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		} else {
			dac_config.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
		}

		/* audio process test */
		sys_drv_aud_dacg_set(0);
		sys_drv_aud_mic1_gain_set(0);
		sys_drv_aud_mic2_gain_set(0);

		bk_aud_adc_init(&adc_config);
		bk_aud_dac_init(&dac_config);

		//disable audio interrupt when loop test
		sys_drv_aud_int_en(0);
		bk_aud_adc_disable_int();

		//start adc
		bk_aud_adc_start();
		//start adc
		bk_aud_dac_start();

		//enable adc to dac loop test
		bk_aud_adc_start_loop_test();
	}else {
		//disable adc and dac
		bk_aud_adc_stop();
		bk_aud_dac_stop();

		//stop loop test
		bk_aud_adc_stop_loop_test();
		bk_aud_adc_deinit();
		bk_aud_dac_deinit();
	}
}

static void audio_adc_config(uint32_t sample_rate, uint32_t adc_mode)
{
	aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();

	adc_config.adc_chl = AUD_ADC_CHL_LR;
	adc_config.clk_src = AUD_CLK_XTAL;
	adc_config.adc_mode = adc_mode;
	switch (sample_rate) {
		case 1:
			adc_config.samp_rate = 8000;
			break;
	
		case 2:
			adc_config.samp_rate = 11025;
			break;
	
		case 3:
			adc_config.samp_rate = 12000;
			break;
	
		case 4:
			adc_config.samp_rate = 16000;
			break;
	
		case 5:
			adc_config.samp_rate = 22050;
			break;
		
		case 6:
			adc_config.samp_rate = 24000;
			break;
		
		case 7:
			adc_config.samp_rate = 32000;
			break;
	
		case 8:
			adc_config.samp_rate = 44100;
			break;
	
		case 9:
			adc_config.samp_rate = 48000;
			break;
	
		default:
			break;
	}

	/* audio process test */
	sys_drv_aud_mic1_gain_set(0);
	sys_drv_aud_mic2_gain_set(0);

	bk_aud_adc_init(&adc_config);

	//disable audio interrupt when loop test
//	sys_drv_aud_int_en(0);
//	bk_aud_adc_disable_int();

	//start adc
	bk_aud_adc_start();
}

static void audio_adc_deconfig(void)
{
	//disable adc
	bk_aud_adc_stop();
	bk_aud_adc_deinit();
	//bk_aud_driver_deinit();
}

static void audio_dac_config(uint32_t sample_rate, uint32_t dac_mode)
{
	aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();

	dac_config.dac_chl = AUD_DAC_CHL_LR;
	dac_config.clk_src = AUD_CLK_XTAL;
	dac_config.work_mode = dac_mode;
	switch (sample_rate) {
		case 1:
			dac_config.samp_rate = 8000;
			break;
	
		case 2:
			dac_config.samp_rate = 11025;
			break;
	
		case 3:
			dac_config.samp_rate = 12000;
			break;
	
		case 4:
			dac_config.samp_rate = 16000;
			break;
	
		case 5:
			dac_config.samp_rate = 22050;
			break;
		
		case 6:
			dac_config.samp_rate = 24000;
			break;
		
		case 7:
			dac_config.samp_rate = 32000;
			break;
	
		case 8:
			dac_config.samp_rate = 44100;
			break;
	
		case 9:
			dac_config.samp_rate = 48000;
			break;
	
		default:
			break;
	}

	/* audio process test */
	sys_drv_aud_dacg_set(0);

	bk_aud_dac_init(&dac_config);

	//disable audio interrupt when loop test
	sys_drv_aud_int_en(0);
	bk_aud_adc_disable_int();

	//start adc
	bk_aud_dac_start();
}

static void audio_dac_deconfig(void)
{
	//disable dac
	bk_aud_dac_stop();
	bk_aud_dac_deinit();
	//bk_aud_driver_deinit();
}

static void i2s_config(uint32_t sample_rate)
{
	i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
	i2s_samp_rate_t samp_rate = I2S_SAMP_RATE_8000;

	switch (sample_rate) {
		case 1:		//8k
			samp_rate = I2S_SAMP_RATE_8000;
			break;

		case 2:		//11.025k
			samp_rate = I2S_SAMP_RATE_11025;
			break;

		case 3:		//12k
			samp_rate = I2S_SAMP_RATE_12000;
			break;

		case 4:		//16k
			samp_rate = I2S_SAMP_RATE_16000;
			break;

		case 5:		//22.05k
			samp_rate = I2S_SAMP_RATE_22050;
			break;

		case 6:		//24k
			samp_rate = I2S_SAMP_RATE_24000;
			break;

		case 7:		//32k
			samp_rate = I2S_SAMP_RATE_32000;
			break;

		case 8:		//44.1k
			samp_rate = I2S_SAMP_RATE_44100;
			break;

		case 9:		//48k
			samp_rate = I2S_SAMP_RATE_48000;
			break;

		default:
			break;
	}

	i2s_config.samp_rate = I2S_SAMP_RATE_8000;
	i2s_config.samp_rate = samp_rate;
	i2s_config.store_mode = I2S_LRCOM_STORE_16R16L;

	/* init i2s */
	bk_i2s_driver_init();

	/* config i2s */
#if CONFIG_SOC_BK7256XX
	bk_i2s_init(I2S_GPIO_GROUP_0, &i2s_config);
#endif
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
	bk_i2s_init(I2S_GPIO_GROUP_2, &i2s_config);
#endif

	bk_i2s_enable(I2S_ENABLE);
}

static void i2s_deconfig(void)
{
	bk_i2s_enable(I2S_DISABLE);

	bk_i2s_deinit();

	bk_i2s_driver_deinit();
}


static dma_id_t aud_i2s_dma_config(uint32_t state)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t i2s_data_addr;
	uint32_t aud_adc_data_addr;
	uint32_t aud_dac_data_addr;

	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		return DMA_ID_MAX;
	}

	/* allocate free DMA channel */
	dma_id_t dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((dma_id < DMA_ID_0) || (dma_id >= DMA_ID_MAX)) {
		return DMA_ID_MAX;
	}

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

	/* get audio adc and dac fifo address */
	bk_aud_adc_get_fifo_addr(&aud_adc_data_addr);
	bk_aud_dac_get_fifo_addr(&aud_dac_data_addr);

	/* get i2s address */
	bk_i2s_get_data_addr(I2S_CHANNEL_1, &i2s_data_addr);

	if (state) {
		/* audio adc to i2s by dma */
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
		dma_config.src.dev = DMA_DEV_AUDIO_RX;
#else
		dma_config.src.dev = DMA_DEV_AUDIO;
#endif
#if CONFIG_SOC_BK7256XX
		dma_config.dst.dev = DMA_DEV_I2S;
#endif
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
		dma_config.dst.dev = DMA_DEV_I2S2;
#endif
		dma_config.dst.start_addr = i2s_data_addr;
		dma_config.dst.end_addr = i2s_data_addr + 4;
		dma_config.src.start_addr = aud_adc_data_addr;
		dma_config.src.end_addr = aud_adc_data_addr + 4;
	} else {
		/* audio i2s to dac by dma */
#if CONFIG_SOC_BK7256XX
		dma_config.src.dev = DMA_DEV_I2S_RX;
#endif
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
		dma_config.src.dev = DMA_DEV_I2S2_RX;
#endif
		dma_config.dst.dev = DMA_DEV_AUDIO;
		dma_config.src.start_addr = i2s_data_addr;
		dma_config.src.end_addr = i2s_data_addr + 4;
		dma_config.dst.start_addr = aud_dac_data_addr;
		dma_config.dst.end_addr = aud_dac_data_addr + 4;
	}

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		return DMA_ID_MAX;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, 4);

#if (CONFIG_SPE)
	bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC);
	bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC);
#endif

	return dma_id;
}

static void aud_i2s_dma_deconfig(void)
{
	for (uint8_t i = 0; i < DMA_ID_MAX; i++) {
		if (bk_dma_user(i) == DMA_DEV_AUDIO) {
			bk_dma_stop(i);
			bk_dma_deinit(i);
			bk_dma_free(DMA_DEV_AUDIO, i);
		}
	}

	bk_dma_driver_deinit();
}

static void aud_i2s_dma_ctrl(uint32_t state, dma_id_t adc_dma_id, dma_id_t dac_dma_id)
{
	uint32_t temp_data = 0xF0F0F0F0;

	if (state) {
		/* start dma */
		if (adc_dma_id != DMA_ID_MAX) {
			bk_dma_start(adc_dma_id);
		}

		if (dac_dma_id != DMA_ID_MAX) {
			bk_dma_start(dac_dma_id);
		}

//		bk_i2s_write_data(0, &temp_data, 1);
		bk_i2s_write_data(1, &temp_data, 1);
		bk_i2s_write_data(2, &temp_data, 1);
	} else {
		/* stop dma */
		if (adc_dma_id != DMA_ID_MAX) {
			bk_dma_stop(adc_dma_id);
		}

		if (dac_dma_id != DMA_ID_MAX) {
			bk_dma_stop(dac_dma_id);
		}
	}
}



/*
01 e0 fc 05 37 X1 X2 X3 X4 (hex)

X1: test control
00: stop test
01: start test

X2: test mode
00: loop
01: adc (IIS OUT,master)
02: dac (IIS IN,master)
03: dac & adc (IIS IN,master)

X3:sample rate setting
1-9: 8K-48K

X4：adc and dac mode
00: differential
01: single-ended
*/
//extern void dma_struct_dump(dma_id_t id);

void audio_ap_test_for_ate(UINT8 enable, UINT8 test_mode, UINT8 sample_rate, UINT8 adc_dac_mode)
{
	dma_id_t adc_dma_id = DMA_ID_MAX;
	dma_id_t dac_dma_id = DMA_ID_MAX;
	UINT8 ret = 0;

	/* parameter check */
	if ((enable < 0 || enable > 1) ||
		(test_mode < 0 || test_mode > 4) ||
		(sample_rate < 1 || sample_rate > 9) ||
		(adc_dac_mode < 0 || adc_dac_mode > 1))
	{
		ret = 1;
		goto audio_exit;
	}

	if (enable == 1) {
		/* start test */
		switch (test_mode)
		{
			/* loop mode */
			case 0:
				audio_auto_loop_test(1, sample_rate, adc_dac_mode);
				break;

			/* audio adc */
			case 1:
				audio_adc_config(sample_rate, adc_dac_mode);
				i2s_config(sample_rate);
				adc_dma_id = aud_i2s_dma_config(1);
				//dma_struct_dump(adc_dma_id);
				aud_i2s_dma_ctrl(1, adc_dma_id, dac_dma_id);
				//dma_struct_dump(adc_dma_id);
				break;

			/* audio dac */
			case 2:
				audio_dac_config(sample_rate, adc_dac_mode);
				i2s_config(sample_rate);
				dac_dma_id = aud_i2s_dma_config(0);
				aud_i2s_dma_ctrl(1, adc_dma_id, dac_dma_id);
				break;

			/* audio adc and dac */
			case 3:
				audio_adc_config(sample_rate, adc_dac_mode);
				audio_dac_config(sample_rate, adc_dac_mode);
				i2s_config(sample_rate);
				dac_dma_id = aud_i2s_dma_config(0);
				adc_dma_id = aud_i2s_dma_config(1);
				aud_i2s_dma_ctrl(1, adc_dma_id, dac_dma_id);
				break;

			/* mic2 to dacl */
			case 4:
				audio_adc_mic2_to_dac_test_start();
				break;

			default:
				break;
		}
	} else if (enable == 0) {
		/* stop test */
		switch (test_mode)
		{
			/* loop mode */
			case 0:
				audio_auto_loop_test(0, sample_rate, adc_dac_mode);
				break;

			/* audio adc */
			case 1:
				aud_i2s_dma_ctrl(0, adc_dma_id, dac_dma_id);
				audio_adc_deconfig();
				bk_aud_driver_deinit();
				i2s_deconfig();
				aud_i2s_dma_deconfig();
				break;

			/* audio dac */
			case 2:
				aud_i2s_dma_ctrl(0, adc_dma_id, dac_dma_id);
				audio_dac_deconfig();
				bk_aud_driver_deinit();
				i2s_deconfig();
				aud_i2s_dma_deconfig();
				break;

			/* audio dac */
			case 3:
				aud_i2s_dma_ctrl(0, adc_dma_id, dac_dma_id);
				audio_adc_deconfig();
				audio_dac_deconfig();
				bk_aud_driver_deinit();
				i2s_deconfig();
				aud_i2s_dma_deconfig();
				break;

			/* mic2 to dacl */
			case 4:
				audio_adc_mic2_to_dac_test_stop();
				break;

			default:
				ret = 2;
				goto audio_exit;
				break;
		}
	} else {
		ret = 3;
		goto audio_exit;
	}

audio_exit:
	//uart_send_bytes_for_ate((UINT8 *)&ret, 1);
	os_printf("ret: %d \n", ret);
}

static void cli_aud_ate_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 5) {
		cli_aud_ate_help();
		return;
	}

	audio_ap_test_for_ate(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
}


#define AUD_ATE_CMD_CNT (sizeof(s_aud_ate_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_ate_commands[] = {
	{"aud_ate_test", "aud_ate_test {X1 X2 X3 X4}", cli_aud_ate_test_cmd},
};

int cli_aud_ate_init(void)
{
	return cli_register_commands(s_aud_ate_commands, AUD_ATE_CMD_CNT);
}

