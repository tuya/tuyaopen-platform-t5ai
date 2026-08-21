/*
 * test_audio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"

#include "sys_driver.h"
#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <driver/aud_common.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <driver/dma.h>
#include "tkl_mftest.h"

static dma_id_t adc_dma_id = 0;
static dma_id_t dac_dma_id = 0;
static uint8_t *adc_ringbuff_addr = NULL;
static uint8_t *dac_ringbuff_addr = NULL;
static RingBufferContext adc_rb;
static RingBufferContext dac_rb;
static int16_t *adc_temp = NULL;
static uint8_t test_mic_id = 0;
/* AEC tone buffer: fed to DAC in ISR so RUNNING never falls back to mic-loopback */
static volatile int16_t *s_aec_tone_buf = NULL;
static volatile uint32_t s_aec_tone_bytes = 0;
volatile int16_t g_aec_peak_l = 0;
volatile int16_t g_aec_peak_r = 0;
volatile uint32_t g_aec_dac_wr_ok = 0;
volatile uint32_t g_aec_dac_wr_fail = 0;

extern int16_t *aec_temp;
extern volatile uint32_t __aec_test_flag;
extern void tkl_mf_aec_notify(void);

/**
 * @brief Register AEC playback tone buffer (mono int16 PCM, one frame)
 * @param[in] buf tone samples
 * @param[in] bytes buffer size in bytes
 * @return none
 */
void tkl_aec_set_tone_buf(int16_t *buf, uint32_t bytes)
{
    s_aec_tone_buf = buf;
    s_aec_tone_bytes = bytes;
}

/**
 * @brief Enable MIC L/R analog front-end and set gain for AEC loopback test
 * @return none
 * @note BK7258 sys_hal_aud_mic2_en was previously a no-op; MIC R (ana_reg27) stayed off.
 *       audio_test also calls mic2_gain_set(0) which mutes MIC R — restore gain here.
 */
void tkl_aec_enable_dual_mic(void)
{
    sys_drv_aud_mic1_en(1);
    sys_drv_aud_mic2_en(1);
    bk_aud_set_ana_mic0_gain(0x08);
    bk_aud_set_ana_mic1_gain(0x08);
    bk_printf("aec dual mic enabled, gain=0x08\r\n");
}

static void tuya_audio_adc_dma_finish_isr(void)
{
    /* read adc data from adc ringbuffer */
    uint32_t size = ring_buffer_read(&adc_rb, (uint8_t*)adc_temp, ADC_FRAME_SIZE);
    if (size != ADC_FRAME_SIZE) {
        return;
    }

    /*
     * AEC modes must NOT fall through to mic->spk loopback.
     * After START/RUNNING were given distinct values, RUNNING used to hit the
     * loopback path and overwrite the 2kHz tone with mic L, so peak collapsed.
     */
    if (__aec_test_flag == AEC_TEST_INIT ||
        __aec_test_flag == AEC_TEST_START ||
        __aec_test_flag == AEC_TEST_RUNNING) {
        if (s_aec_tone_buf != NULL && s_aec_tone_bytes > 0) {
            uint32_t free_size = ring_buffer_get_free_size(&dac_rb);
            if (free_size >= s_aec_tone_bytes) {
                uint32_t wr = ring_buffer_write(&dac_rb, (uint8_t *)s_aec_tone_buf, s_aec_tone_bytes);
                if (wr == s_aec_tone_bytes) {
                    g_aec_dac_wr_ok++;
                } else {
                    g_aec_dac_wr_fail++;
                }
            } else {
                g_aec_dac_wr_fail++;
            }
        }
        if (__aec_test_flag == AEC_TEST_START && aec_temp != NULL) {
            int16_t peak_l = 0;
            int16_t peak_r = 0;
            /* ADC_FRAME_SIZE/4 = mono samples per frame (MIC R = SPK loopback) */

            /* 16位交错格式：L0,R0,L1,R1... */
            for (uint32_t i = 0; i < ADC_FRAME_SIZE / 4; i++) {
                int16_t l = (int16_t)adc_temp[2 * i];      // 偶数索引 = L (外部MIC)
                int16_t r = (int16_t)adc_temp[2 * i + 1];  // 奇数索引 = R (SPK回采)

                int16_t al = (l < 0) ? (int16_t)(-l) : l;
                int16_t ar = (r < 0) ? (int16_t)(-r) : r;
                if (al > peak_l) {
                    peak_l = al;
                }
                if (ar > peak_r) {
                    peak_r = ar;
                }
                aec_temp[i] = r;  // 复制R声道数据（SPK回采）
            }
            g_aec_peak_l = peak_l;
            g_aec_peak_r = peak_r;
        }
        /* 通知计算线程：确保所有AEC数据处理完成后再通知，避免竞态条件 */
        if (__aec_test_flag == AEC_TEST_START) {
            tkl_mf_aec_notify();
        }
        return;
    }

    /* select r channel data */
    for (uint32_t i = 0; i < ADC_FRAME_SIZE/4; i++) {
        if (test_mic_id == 1)
            adc_temp[i] = adc_temp[2*i];
        else if (test_mic_id == 2)
            adc_temp[i] = adc_temp[2*i+1];
    }

    size = ring_buffer_get_free_size(&dac_rb);
    if (size >= ADC_FRAME_SIZE/2) {
        ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);
    }
}

static void tuya_audio_adc_mic_to_dac_test(uint32_t state, uint32_t sample_rate)
{
    aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
    adc_config.adc_chl = AUD_ADC_CHL_LR;
    adc_config.clk_src = AUD_CLK_XTAL;
    adc_config.samp_rate = sample_rate;
    aud_dac_config_t dac_config = DEFAULT_AUD_DAC_CONFIG();
    dac_config.clk_src = AUD_CLK_XTAL;
    dac_config.samp_rate = sample_rate;
    dma_config_t dma_config = {0};
    dma_config_t dac_dma_config = {0};
    uint32_t aud_adc_data_addr;
    uint32_t aud_dac_data_addr;
    bk_err_t ret = BK_OK;

    if (state == 1 || state == 2) {

        if (test_mic_id == 0)
        {
            bk_aud_adc_init(&adc_config);

            /* Do NOT mute mic2 here: AEC needs MIC R (SPK loopback).
             * Legacy mic1-only loopback muted mic2; that breaks AEC on T5. */

            //disable audio interrupt when loop test
            sys_drv_aud_int_en(0);
            bk_aud_adc_disable_int();

            //start adc
            //	bk_aud_adc_start();

            /* init dma driver */
            ret = bk_dma_driver_init();
            if (ret != BK_OK) {
                return;
            }

            /* allocate free DMA channel */
            adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
            if ((adc_dma_id < DMA_ID_0) || (adc_dma_id >= DMA_ID_MAX)) {
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
                return;
            }

            /* set dma transfer length */
            bk_dma_set_transfer_len(adc_dma_id, ADC_FRAME_SIZE);

    #if (CONFIG_SPE)
            bk_dma_set_dest_sec_attr(adc_dma_id, DMA_ATTR_SEC);
            bk_dma_set_src_sec_attr(adc_dma_id, DMA_ATTR_SEC);
    #endif

            //register isr
            bk_dma_register_isr(adc_dma_id, NULL, (void *)tuya_audio_adc_dma_finish_isr);
            bk_dma_enable_finish_interrupt(adc_dma_id);

            /* dac config */
            bk_aud_dac_init(&dac_config);

            /* 增加DAC增益以获得足够的输出音量用于MIC R回采 */
            /* 尝试不同增益值以找到最佳值 */
            sys_drv_aud_dacg_set(10);  /* 设置为最大增益 (假设0-31范围) */

            /* allocate free DMA channel */
            dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
            if ((dac_dma_id < DMA_ID_0) || (dac_dma_id >= DMA_ID_MAX)) {
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
                return;
            }

            /* set dma transfer length */
            bk_dma_set_transfer_len(dac_dma_id, ADC_FRAME_SIZE/2);

            os_memset(adc_temp, 0, ADC_FRAME_SIZE/2);
            ring_buffer_write(&dac_rb, (uint8_t *)adc_temp, ADC_FRAME_SIZE/2);

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
        test_mic_id = state;
    } else if (state == 0) {
        for (uint8_t i = 0; i < DMA_ID_MAX; i++) {
            if (bk_dma_user(i) == DMA_DEV_AUDIO) {
                bk_dma_stop(i);
                bk_dma_deinit(i);
                bk_dma_free(DMA_DEV_AUDIO, i);
            }
        }

        // bk_dma_driver_deinit();

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

        test_mic_id = state;
    }else {
        //not need todo
        return;
    }
}

void tkl_aec_fill_data(int16_t *buffer, uint32_t len)
{
    int size = ring_buffer_get_free_size(&dac_rb);
    if (size >= ADC_FRAME_SIZE/2) {
        ring_buffer_write(&dac_rb, (uint8_t *)buffer, len);
    }
}

// test cli commond:
// audio_test: [test_mode: 0-close, 1-mic1_to_spk, 2-tmic2_to_spk] [sample_rate: 8000, 16000, 44100, 48000] [dac_mode: 0-single, 1-different]
// eg: audio_test 2 16000 1 ------ test mic2 to spk, samplerate 16000, dac mode different
void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("%s %s %s %s\n\r", argv[1],  argv[2],  argv[3],  argv[4]);
    uint32_t state = (uint8_t)os_strtoul(argv[1], NULL, 10);
    uint32_t sample_rate = os_strtoul(argv[2], NULL, 10);
    uint32_t dac_mode = (uint8_t)os_strtoul(argv[3], NULL, 10);

    uint32_t mic1_config = 0;
    bk_printf("state: %d, sample_rate: %d, dac_mode: %d\n\r", state, sample_rate, dac_mode);

    tuya_audio_adc_mic_to_dac_test(state, sample_rate);
}



