
#include <common/bk_include.h>
#include <os/os.h>
#include <components/audio_param_ctrl.h>

#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/aud_common.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/flash.h>
#include <driver/flash_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>

#include "tuya_cloud_types.h"
#include "tkl_aud_adc.h"
#include "tkl_aud_pm.h"

extern void bk_printf(const char *fmt, ...);

#define MODULE_LOG_HELPER(fmt, ...) \
    bk_printf(fmt, ##__VA_ARGS__)
#define TKL_AUD_ADC_LOG(...) MODULE_LOG_HELPER(__VA_ARGS__)

// sr: sample rate, sb: sample bits, ch: channel num, t: frame time in ms
#define AUD_ADC_FRAME_SIZE(sr, sb, t, ch)  (((sr) * ((sb) / 8) / 1000) * (ch) * (t))

/* ADC 数字增益默认值(原始寄存器值, 范围 0x00~0x3F), 对齐旧 onboard_mic 默认增益。
 * init 阶段固定使用该值(不读 config->vol), 运行期如需调整走 tkl_aud_adc_set_vol。 */
#define TKL_AUD_ADC_DEFAULT_GAIN  0x2d

/*
 * DMA_BUF_SAFE_INTERVAL works with ring_buffer_init / ring_buffer_read /
 * ring_buffer_clear to manage dma_set_dst_pause_addr for DMA flow control:
 *   - ring_buffer_init:  pause_addr = buf + capacity - 8
 *   - ring_buffer_clear: pause_addr = buf + capacity - 4
 *   - ring_buffer_read:  pause_addr = rp (or rp-4 boundary case)
 * This prevents DMA from overwriting unread data.
 */
#define DMA_BUF_SAFE_INTERVAL    (8)

typedef struct {
    TKL_AUD_ADC_CFG_T   cfg;
    bool                is_open;
    bool                pm_voted;
    uint32_t            dma_frame_size;
    uint8_t             chl_num;
    dma_id_t            mic_dma_id;
    RingBufferContext   mic_rb;
    int8_t             *mic_dma_buf;
    uint8_t            *read_buf;          /* dma_frame_size: ring_buffer_read target */
    uint8_t            *output_buf;        /* frame_size: single channel extraction result */
    uint32_t            output_buf_size;
} tkl_aud_adc_ctx_t;

static tkl_aud_adc_ctx_t *gl_adc_ctx = NULL;
static aud_adc_config_t aud_adc_cfg = DEFAULT_AUD_ADC_CONFIG();

/* ---------------------- flash operation notify ------------------------------ */

static void __tkl_aud_adc_flash_op_notify_handler(uint32_t param, void *args)
{
    if (gl_adc_ctx == NULL || !gl_adc_ctx->is_open)
    {
        return;
    }

    if (param)
    {
        bk_dma_stop(gl_adc_ctx->mic_dma_id);
        bk_aud_adc_stop();
        ring_buffer_clear(&gl_adc_ctx->mic_rb);
    }
    else
    {
        bk_dma_start(gl_adc_ctx->mic_dma_id);
        bk_aud_adc_start();
    }
}

/* ---------------------- DMA ISR -------------------------------------------- */

/**
 * DMA finish ISR: one dma_frame_size chunk has been written into the ring buffer.
 *
 * ring_buffer_read does three things atomically:
 *   1. Gets DMA write position from hardware register (wp)
 *   2. Copies data from ring buffer to read_buf
 *   3. Updates dma_set_dst_pause_addr so DMA can continue writing
 *
 * For single channel: extract L from L+R interleaved read_buf into output_buf
 * For dual   channel: read_buf already contains the final PCM data
 *
 * Then call upper_aud_adc_cb(pcm_data_ptr) to push data to TAL layer.
 */
static void __tkl_aud_adc_dma_finish_isr(void)
{
    if (gl_adc_ctx == NULL || !gl_adc_ctx->is_open)
    {
        return;
    }

    if (gl_adc_ctx->cfg.upper_cb == NULL)
    {
        return;
    }

    uint32_t dma_frame_size = gl_adc_ctx->dma_frame_size;

    uint32_t fill_size = ring_buffer_get_fill_size(&gl_adc_ctx->mic_rb);
    if (fill_size < dma_frame_size)
    {
        return;
    }

    uint32_t read_size = ring_buffer_read(&gl_adc_ctx->mic_rb,
                                          gl_adc_ctx->read_buf,
                                          dma_frame_size);
    if (read_size != dma_frame_size)
    {
        return;
    }

    if (gl_adc_ctx->chl_num == 1)
    {
        // PCM DATA: LRLRLRLR.....
        int16_t *src = (int16_t *)gl_adc_ctx->read_buf;
        int16_t *dst = (int16_t *)gl_adc_ctx->output_buf;
        
        // T5仅支持16位深采样
        uint32_t samples = gl_adc_ctx->output_buf_size / sizeof(int16_t);
        if (gl_adc_ctx->cfg.chan == TUYA_AUDIO_ADC_CHANNEL_L)
        {
            // 提取左声道数据
            for (uint32_t i = 0; i < samples; i++)
            {
                dst[i] = src[2 * i];
            }
        }
        else if (gl_adc_ctx->cfg.chan == TUYA_AUDIO_ADC_CHANNEL_R)
        {
            // 提取右声道数据
            for (uint32_t i = 0; i < samples; i++)
            {
                dst[i] = src[2 * i + 1];
            }
        }
        else
        {
            // 默认左声道数据
            for (uint32_t i = 0; i < samples; i++)
            {
                dst[i] = src[2 * i];
            }
        }
        gl_adc_ctx->cfg.upper_cb(TUYA_AUDIO_FRAME_EVENT_ADC_RX, gl_adc_ctx->output_buf, gl_adc_ctx->output_buf_size, gl_adc_ctx->cfg.args);
    }
    else
    {
        gl_adc_ctx->cfg.upper_cb(TUYA_AUDIO_FRAME_EVENT_ADC_RX, gl_adc_ctx->read_buf, read_size, gl_adc_ctx->cfg.args);
    }
}

/* ---------------------- DMA config / deconfig ------------------------------ */

static bk_err_t __tkl_aud_adc_dma_deconfig(void)
{
    if (gl_adc_ctx == NULL)
    {
        return BK_OK;
    }

    bk_dma_deinit(gl_adc_ctx->mic_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, gl_adc_ctx->mic_dma_id);

    if (gl_adc_ctx->mic_dma_buf)
    {
        ring_buffer_clear(&gl_adc_ctx->mic_rb);
        audio_dma_mem_free(gl_adc_ctx->mic_dma_buf);
        gl_adc_ctx->mic_dma_buf = NULL;
    }

    return BK_OK;
}

static bk_err_t __tkl_aud_adc_dma_config(void)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t adc_port_addr;
    uint32_t frame_size = gl_adc_ctx->dma_frame_size;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    gl_adc_ctx->mic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((gl_adc_ctx->mic_dma_id < DMA_ID_0) || (gl_adc_ctx->mic_dma_id >= DMA_ID_MAX))
    {
        TKL_AUD_ADC_LOG("malloc dma fail\n");
        goto exit;
    }

    /* Two frames + safe interval for dma_set_dst_pause_addr flow control */
    gl_adc_ctx->mic_dma_buf = (int8_t *)audio_dma_mem_calloc(2, frame_size + DMA_BUF_SAFE_INTERVAL / 2);
    if (gl_adc_ctx->mic_dma_buf == NULL)
    {
        TKL_AUD_ADC_LOG("malloc dma buffer fail\n");
        goto exit;
    }

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev = DMA_DEV_AUDIO_RX;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

    if (bk_aud_adc_get_fifo_addr(&adc_port_addr) != BK_OK)
    {
        TKL_AUD_ADC_LOG("get adc fifo address failed\n");
        goto exit;
    }

    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = adc_port_addr;
    dma_config.src.end_addr = adc_port_addr + 4;

    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uint32_t)(uintptr_t)gl_adc_ctx->mic_dma_buf;
    dma_config.dst.end_addr = (uint32_t)(uintptr_t)gl_adc_ctx->mic_dma_buf
                              + frame_size * 2 + DMA_BUF_SAFE_INTERVAL;

    ret = bk_dma_init(gl_adc_ctx->mic_dma_id, &dma_config);
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("dma_init fail\n");
        goto exit;
    }

    bk_dma_set_transfer_len(gl_adc_ctx->mic_dma_id, frame_size);

    bk_dma_register_isr(gl_adc_ctx->mic_dma_id, NULL, (void *)__tkl_aud_adc_dma_finish_isr);
    bk_dma_enable_finish_interrupt(gl_adc_ctx->mic_dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(gl_adc_ctx->mic_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(gl_adc_ctx->mic_dma_id, DMA_ATTR_SEC);
#endif

    /* ring_buffer_init sets dma_set_dst_pause_addr(dma_id, buf + capacity - 8) */
    ring_buffer_init(&gl_adc_ctx->mic_rb,
                     (uint8_t *)gl_adc_ctx->mic_dma_buf,
                     frame_size * 2 + DMA_BUF_SAFE_INTERVAL,
                     gl_adc_ctx->mic_dma_id,
                     RB_DMA_TYPE_WRITE);

    TKL_AUD_ADC_LOG("dma_config dma_id: %d, transfer_len: %d\n",
                     gl_adc_ctx->mic_dma_id, frame_size);

    return BK_OK;

exit:
    __tkl_aud_adc_dma_deconfig();
    return BK_FAIL;
}

/* ---------------------- Public API ----------------------------------------- */

/**
* @brief Initialize audio ADC
*
* @param[in] config: ADC configuration including chan, sample_rate, sample_bits,
*                    frame_size, vol, upper_aud_adc_cb
*
* @note This API allocates context, configures ADC hardware, DMA ring buffer,
*       and registers flash operation notify handler. ADC always configures LR channel
*       internally; mic_mode selects which mic is active. For single channel, DMA
*       frame_size is doubled to capture interleaved L+R data, then L channel is
*       extracted in ISR. Must be called before tkl_aud_adc_start.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_aud_adc_init(TUYA_AUDIO_ADC_PORT_E port, TKL_AUD_ADC_CFG_T *config)
{
    if (config == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    if (gl_adc_ctx != NULL)
    {
        TKL_AUD_ADC_LOG("adc already initialized\n");
        return OPRT_COM_ERROR;
    }

    (void) port;

    bk_err_t ret = BK_OK;
    aud_mic_id_t mic_id = AUD_MIC_BOTH;
    aud_adc_mode_t adc_mode = AUD_ADC_MODE_DIFFEN;
    uint8_t chl_num = 2;

    gl_adc_ctx = (tkl_aud_adc_ctx_t *)os_malloc(sizeof(tkl_aud_adc_ctx_t));
    if (gl_adc_ctx == NULL)
    {
        TKL_AUD_ADC_LOG("malloc adc context fail\n");
        return OPRT_MALLOC_FAILED;
    }
    os_memset(gl_adc_ctx, 0, sizeof(tkl_aud_adc_ctx_t));
    os_memcpy(&gl_adc_ctx->cfg, config, sizeof(TKL_AUD_ADC_CFG_T));

    tkl_aud_pm_acquire();
    gl_adc_ctx->pm_voted = true;

    switch (config->chan)
    {
#if 0
        case TUYA_AUDIO_ADC_CHANNEL_L:
            mic_id = AUD_MIC_MIC1;
            adc_mode = AUD_ADC_MODE_SIGNAL_END;
            chl_num = 1;
            break;
        case TUYA_AUDIO_ADC_CHANNEL_R:
            mic_id = AUD_MIC_MIC2;
            adc_mode = AUD_ADC_MODE_SIGNAL_END;
            chl_num = 1;
            break;
#endif
        case TUYA_AUDIO_ADC_CHANNEL_L:
        case TUYA_AUDIO_ADC_CHANNEL_R:
            chl_num = 1;                // 强制双声道采集数据，此处设置通道数量1用于后续frame大小计算与上报数据时判断
            break;
        case TUYA_AUDIO_ADC_CHANNEL_LR:
            mic_id = AUD_MIC_BOTH;
            adc_mode = AUD_ADC_MODE_DIFFEN;
            chl_num = 2;
            break;
        default:
            TKL_AUD_ADC_LOG("chan %d not support\n", config->chan);
            goto __adc_init_exit;
    }

    if (config->sample_bits != 16)
    {
        TKL_AUD_ADC_LOG("bits: %d not support, only 16bits\n", config->sample_bits);
        goto __adc_init_exit;
    }

    gl_adc_ctx->chl_num = chl_num;

    // 强制两路mic数据，中断里面根据通道数量上报
    gl_adc_ctx->dma_frame_size = AUD_ADC_FRAME_SIZE(config->sample_rate, config->sample_bits, config->frame_time_ms, 2);

    /* ADC always configures LR channel, mic_mode selects which mic is active */
    aud_adc_cfg.adc_chl = AUD_ADC_CHL_LR;
    aud_adc_cfg.samp_rate = config->sample_rate;
    aud_adc_cfg.adc_gain = TKL_AUD_ADC_DEFAULT_GAIN;
    aud_adc_cfg.clk_src = AUD_CLK_APLL;
    aud_adc_cfg.adc_mode = adc_mode;

    TKL_AUD_ADC_LOG("adc config, chan %d, vol 0x%02x, samp_rate: %d, clk_src: %s, adc_mode: %s\n",
                     aud_adc_cfg.adc_chl, aud_adc_cfg.adc_gain, aud_adc_cfg.samp_rate,
                     aud_adc_cfg.clk_src == 1 ? "APLL" : "XTAL",
                     aud_adc_cfg.adc_mode == 1 ? "AUD_ADC_MODE_SIGNAL_END" : "AUD_ADC_MODE_DIFFEN");

    ret = bk_aud_adc_init(&aud_adc_cfg);
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("bk_aud_adc_init fail\n");
        goto __adc_init_exit;
    }

    ret = bk_aud_adc_set_mic_mode(mic_id, adc_mode);
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("bk_aud_adc_set_mic_mode fail, ret %d mic_id %d mode %d\n",
                         ret, mic_id, adc_mode);
        goto __adc_init_exit;
    }

    ret = __tkl_aud_adc_dma_config();
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("dma config fail\n");
        goto __adc_init_exit;
    }

    /* read_buf: ring_buffer_read destination, always dma_frame_size */
    gl_adc_ctx->read_buf = (uint8_t *)os_malloc(gl_adc_ctx->dma_frame_size);
    if (gl_adc_ctx->read_buf == NULL)
    {
        TKL_AUD_ADC_LOG("malloc read buffer fail\n");
        goto __adc_init_exit;
    }

    /* output_buf: single channel extraction result, frame_size */
    if (chl_num == 1)
    {
        gl_adc_ctx->output_buf_size = AUD_ADC_FRAME_SIZE(gl_adc_ctx->cfg.sample_rate, gl_adc_ctx->cfg.sample_bits, gl_adc_ctx->cfg.frame_time_ms, chl_num);
        gl_adc_ctx->output_buf = (uint8_t *)os_malloc(gl_adc_ctx->output_buf_size);
        if (gl_adc_ctx->output_buf == NULL)
        {
            TKL_AUD_ADC_LOG("malloc output buffer fail\n");
            goto __adc_init_exit;
        }
    }

    bk_aud_set_ana_mic0_gain(0x08);
    // bk_aud_set_ana_mic1_gain(0x08);

    mb_flash_register_op_onboard_mic_stream_notify(__tkl_aud_adc_flash_op_notify_handler, NULL);

    TKL_AUD_ADC_LOG("audio analog mic init success\n");
    return OPRT_OK;

__adc_init_exit:
    __tkl_aud_adc_dma_deconfig();
    bk_aud_adc_deinit();
    if (gl_adc_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_adc_ctx->pm_voted = false;
    }

    if (gl_adc_ctx)
    {
        if (gl_adc_ctx->read_buf)
        {
            os_free(gl_adc_ctx->read_buf);
        }
        if (gl_adc_ctx->output_buf)
        {
            os_free(gl_adc_ctx->output_buf);
        }
        os_free(gl_adc_ctx);
        gl_adc_ctx = NULL;
    }

    TKL_AUD_ADC_LOG("audio analog mic init failed\n");
    return OPRT_COM_ERROR;
}

/**
* @brief Start audio ADC capture
*
* @note Starts DMA transfer and ADC hardware. After starting, DMA finish ISR
*       will be triggered periodically, calling upper_aud_adc_cb with PCM data.
*       Safe to call multiple times (idempotent).
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_aud_adc_start(TUYA_AUDIO_ADC_PORT_E port)
{
    (void) port;
    if (gl_adc_ctx->is_open)
    {
        TKL_AUD_ADC_LOG("aud adc had been open\n");
        return OPRT_OK;
    }

    bk_err_t ret = bk_dma_start(gl_adc_ctx->mic_dma_id);
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("dma start fail\n");
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_adc_start();
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("adc start fail\n");
        bk_dma_stop(gl_adc_ctx->mic_dma_id);
        return OPRT_COM_ERROR;
    }

    gl_adc_ctx->is_open = true;
    TKL_AUD_ADC_LOG("adc start ok\n");

    return OPRT_OK;
}

/**
* @brief Set audio ADC digital gain
*
* @param[in] gain: digital gain value, range 0x00~0x3F
*
* @note This API sets the ADC digital gain register.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_aud_adc_set_vol(TUYA_AUDIO_ADC_PORT_E port, uint32_t gain)
{
    (void) port;

    uint32_t volume =(uint32_t)(gain * 0x3F / 100);
    if (bk_aud_adc_set_gain(volume) != BK_OK)
    {
        TKL_AUD_ADC_LOG("set digital gain fail\n");
        return OPRT_COM_ERROR;
    }

    gl_adc_ctx->cfg.vol = (uint8_t)gain;

    return OPRT_OK;
}

/**
* @brief Stop audio ADC capture
*
* @note Stops DMA transfer and ADC hardware. After stopping, DMA finish ISR
*       will no longer fire. Safe to call multiple times (idempotent).
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_aud_adc_stop(TUYA_AUDIO_ADC_PORT_E port)
{
    (void) port;
    if (!gl_adc_ctx->is_open)
    {
        return OPRT_OK;
    }

    bk_err_t ret = bk_dma_stop(gl_adc_ctx->mic_dma_id);
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("dma stop fail\n");
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_adc_stop();
    if (ret != BK_OK)
    {
        TKL_AUD_ADC_LOG("adc stop fail\n");
        return OPRT_COM_ERROR;
    }

    gl_adc_ctx->is_open = false;

    return OPRT_OK;
}

/**
* @brief Deinitialize audio ADC
*
* @note Stops capture if running, releases DMA channel and ring buffer,
*       unregisters flash operation notify, frees context memory,
*       and restores CPU frequency.
*/
void tkl_aud_adc_deinit(TUYA_AUDIO_ADC_PORT_E port)
{
    (void) port;
    if (gl_adc_ctx->is_open)
    {
        tkl_aud_adc_stop(port);
    }

    __tkl_aud_adc_dma_deconfig();
    bk_aud_adc_deinit();

    mb_flash_unregister_op_onboard_mic_stream_notify();

    if (gl_adc_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_adc_ctx->pm_voted = false;
    }

    if (gl_adc_ctx->read_buf)
    {
        os_free(gl_adc_ctx->read_buf);
    }
    if (gl_adc_ctx->output_buf)
    {
        os_free(gl_adc_ctx->output_buf);
    }
    os_free(gl_adc_ctx);
    gl_adc_ctx = NULL;
}

