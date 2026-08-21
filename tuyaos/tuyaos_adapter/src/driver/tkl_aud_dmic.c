/**
 * @file tkl_aud_dmic.c
 * @brief TKL audio digital (DMIC) driver implementation
 * @version 1.0
 * @date 2025-03-25
 * @copyright Copyright (c) Tuya Inc.
 */

#include <common/bk_include.h>
#include <os/os.h>

#include <driver/aud_dmic_types.h>
#include <driver/aud_dmic.h>
#include <driver/aud_common.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/gpio.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>

#include "tuya_cloud_types.h"
#include "tkl_aud_dmic.h"
#include "tkl_aud_pm.h"
#include "gpio_driver.h"

extern void bk_printf(const char *fmt, ...);

#define MODULE_LOG_HELPER(fmt, ...) \
    bk_printf(fmt, ##__VA_ARGS__)
#define TKL_AUD_DMIC_LOG(...) MODULE_LOG_HELPER(__VA_ARGS__)

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define AUD_DMIC_FRAME_SIZE(sr, sb, t, ch)  (((sr) * ((sb) / 8) / 1000) * (ch) * (t))

#define DMIC_DMA_BUF_SAFE_INTERVAL    (8)
#define TKL_DMIC_CLK_GPIO             GPIO_8
#define TKL_DMIC_DAT_GPIO             GPIO_9

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */
typedef struct {
    TKL_AUD_DMIC_CFG_T   cfg;
    bool                    is_open;
    bool                    pm_voted;
    uint32_t                frame_size;
    uint32_t                dma_frame_size;
    dma_id_t                dmic_dma_id;
    RingBufferContext       dmic_rb;
    int8_t                 *dmic_dma_buf;
    uint8_t                *read_buf;
} tkl_aud_dmic_ctx_t;

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static tkl_aud_dmic_ctx_t *gl_dmic_ctx = NULL;

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */

/**
 * @brief DMA finish ISR for DMIC
 * @return none
 * @note Reads one DMA frame from ring buffer, extracts single channel
 *       if needed, then calls upper_cb.
 */
static void __tkl_aud_dmic_dma_finish_isr(void)
{
    if (gl_dmic_ctx == NULL || !gl_dmic_ctx->is_open) {
        return;
    }

    if (gl_dmic_ctx->cfg.upper_cb == NULL) {
        return;
    }

    uint32_t dma_frame_size = gl_dmic_ctx->dma_frame_size;

    uint32_t fill_size = ring_buffer_get_fill_size(&gl_dmic_ctx->dmic_rb);
    if (fill_size < dma_frame_size) {
        return;
    }

    uint32_t read_size = ring_buffer_read(&gl_dmic_ctx->dmic_rb,
                                          gl_dmic_ctx->read_buf,
                                          dma_frame_size);
    if (read_size != dma_frame_size) {
        return;
    }

#if 1
    gl_dmic_ctx->cfg.upper_cb(TUYA_AUDIO_FRAME_EVENT_DMIC_RX,
            gl_dmic_ctx->read_buf,
            gl_dmic_ctx->frame_size,
            gl_dmic_ctx->cfg.args);
#endif
}

/**
 * @brief Release DMA channel and ring buffer
 * @return BK_OK on success
 */
static bk_err_t __tkl_aud_dmic_dma_deconfig(void)
{
    if (gl_dmic_ctx == NULL) {
        return BK_OK;
    }

    bk_dma_deinit(gl_dmic_ctx->dmic_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, gl_dmic_ctx->dmic_dma_id);

    if (gl_dmic_ctx->dmic_dma_buf) {
        ring_buffer_clear(&gl_dmic_ctx->dmic_rb);
        audio_dma_mem_free(gl_dmic_ctx->dmic_dma_buf);
        gl_dmic_ctx->dmic_dma_buf = NULL;
    }

    return BK_OK;
}

/**
 * @brief Configure DMA channel for DMIC ring buffer transfer
 * @return BK_OK on success, BK_FAIL on error
 */
static bk_err_t __tkl_aud_dmic_dma_config(void)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dmic_port_addr;
    uint32_t frame_size = gl_dmic_ctx->dma_frame_size;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    gl_dmic_ctx->dmic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((gl_dmic_ctx->dmic_dma_id < DMA_ID_0) || (gl_dmic_ctx->dmic_dma_id >= DMA_ID_MAX)) {
        TKL_AUD_DMIC_LOG("dmic: malloc dma fail\n");
        goto exit;
    }

    gl_dmic_ctx->dmic_dma_buf = (int8_t *)audio_dma_mem_calloc(2, frame_size + DMIC_DMA_BUF_SAFE_INTERVAL / 2);
    if (gl_dmic_ctx->dmic_dma_buf == NULL) {
        TKL_AUD_DMIC_LOG("dmic: malloc dma buffer fail\n");
        goto exit;
    }

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev = DMA_DEV_AUD_DMIC;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

    if (bk_aud_dmic_get_fifo_addr(&dmic_port_addr) != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: get fifo address failed\n");
        goto exit;
    }

    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = dmic_port_addr;
    dma_config.src.end_addr = dmic_port_addr + 4;

    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uint32_t)(uintptr_t)gl_dmic_ctx->dmic_dma_buf;
    dma_config.dst.end_addr = (uint32_t)(uintptr_t)gl_dmic_ctx->dmic_dma_buf
                              + frame_size * 2 + DMIC_DMA_BUF_SAFE_INTERVAL;

    ret = bk_dma_init(gl_dmic_ctx->dmic_dma_id, &dma_config);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: dma_init fail\n");
        goto exit;
    }

    bk_dma_set_transfer_len(gl_dmic_ctx->dmic_dma_id, frame_size);

    bk_dma_register_isr(gl_dmic_ctx->dmic_dma_id, NULL, (void *)__tkl_aud_dmic_dma_finish_isr);
    bk_dma_enable_finish_interrupt(gl_dmic_ctx->dmic_dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(gl_dmic_ctx->dmic_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(gl_dmic_ctx->dmic_dma_id, DMA_ATTR_SEC);
#endif

    ring_buffer_init(&gl_dmic_ctx->dmic_rb,
                     (uint8_t *)gl_dmic_ctx->dmic_dma_buf,
                     frame_size * 2 + DMIC_DMA_BUF_SAFE_INTERVAL,
                     gl_dmic_ctx->dmic_dma_id,
                     RB_DMA_TYPE_WRITE);

    TKL_AUD_DMIC_LOG("dmic: dma_config dma_id=%d transfer_len=%u fifo_addr=0x%08x dma_buf=%p rb_size=%u\n",
                     gl_dmic_ctx->dmic_dma_id, frame_size, dmic_port_addr,
                     (void *)gl_dmic_ctx->dmic_dma_buf, frame_size * 2 + DMIC_DMA_BUF_SAFE_INTERVAL);

    return BK_OK;

exit:
    __tkl_aud_dmic_dma_deconfig();
    return BK_FAIL;
}

/**
 * @brief Initialize audio digital (DMIC) input
 * @param[in] port Digital audio port number
 * @param[in] config DMIC configuration
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_init(TUYA_AUDIO_DMIC_PORT_E port, TKL_AUD_DMIC_CFG_T *config)
{
    if (config == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (config->chan != TUYA_AUDIO_DMIC_CHANNEL_LR) {
        TKL_AUD_DMIC_LOG("dmic: chan %d not support\n", config->chan);
        return OPRT_INVALID_PARM;
    }

    if (config->sample_bits != 16) {
        TKL_AUD_DMIC_LOG("dmic: bits %d not support, only 16bits\n", config->sample_bits);
        return OPRT_INVALID_PARM;
    }

    TKL_AUD_DMIC_LOG("dmic: init request port=%d chan=%d bits=%u rate=%u frame_ms=%u cb_set=%d args=%p\n",
                     port, config->chan, config->sample_bits, config->sample_rate,
                     config->frame_time_ms, config->upper_cb != NULL, config->args);

    if (gl_dmic_ctx != NULL) {
        TKL_AUD_DMIC_LOG("dmic: already initialized\n");
        return OPRT_COM_ERROR;
    }

    (void) port;

    bk_err_t ret = BK_OK;
    aud_dmic_chl_t dmic_chl = AUD_DMIC_CHL_LR;

    // 初始化上下文控制块
    gl_dmic_ctx = (tkl_aud_dmic_ctx_t *)os_malloc(sizeof(tkl_aud_dmic_ctx_t));
    if (gl_dmic_ctx == NULL) {
        TKL_AUD_DMIC_LOG("dmic: malloc context fail\n");
        return OPRT_MALLOC_FAILED;
    }
    os_memset(gl_dmic_ctx, 0, sizeof(tkl_aud_dmic_ctx_t));
    os_memcpy(&gl_dmic_ctx->cfg, config, sizeof(TKL_AUD_DMIC_CFG_T));

    // 设置主频
    tkl_aud_pm_acquire();
    gl_dmic_ctx->pm_voted = true;

    // 配置DMIC
    aud_dmic_config_t tkl_aud_dmic_cfg = DEFAULT_AUD_DMIC_CONFIG();
    tkl_aud_dmic_cfg.dmic_chl = dmic_chl;
    tkl_aud_dmic_cfg.samp_rate = config->sample_rate;
    tkl_aud_dmic_cfg.clk_src = AUD_CLK_APLL;

    TKL_AUD_DMIC_LOG("dmic config, chl %d, samp_rate: %d, clk_src: %s\n",
                      tkl_aud_dmic_cfg.dmic_chl, tkl_aud_dmic_cfg.samp_rate,
                      tkl_aud_dmic_cfg.clk_src == 1 ? "APLL" : "XTAL");

    ret = bk_aud_dmic_init(&tkl_aud_dmic_cfg);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: bk_aud_dmic_init fail\n");
        goto __dmic_init_exit;
    }

    // 计算帧大小
    gl_dmic_ctx->frame_size = AUD_DMIC_FRAME_SIZE(config->sample_rate, config->sample_bits, config->frame_time_ms, 2);
    gl_dmic_ctx->dma_frame_size = AUD_DMIC_FRAME_SIZE(config->sample_rate, config->sample_bits, config->frame_time_ms, 2);
    TKL_AUD_DMIC_LOG("dmic: computed frame_size=%u dma_frame_size=%u\n",
                     gl_dmic_ctx->frame_size, gl_dmic_ctx->dma_frame_size);

    // 配置DMA
    ret = __tkl_aud_dmic_dma_config();
    TKL_AUD_DMIC_LOG("dmic: __tkl_aud_dmic_dma_config ret=%d\n", ret);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: dma config fail\n");
        goto __dmic_init_exit;
    }

    // 分配读缓冲区，临时存储一帧数据，用于上层回调处理
    gl_dmic_ctx->read_buf = (uint8_t *)os_malloc(gl_dmic_ctx->dma_frame_size);
    if (gl_dmic_ctx->read_buf == NULL) {
        TKL_AUD_DMIC_LOG("dmic: malloc read buffer fail\n");
        goto __dmic_init_exit;
    }

    TKL_AUD_DMIC_LOG("dmic: init success, frame_size=%u, dma_frame_size=%u\n",
                      gl_dmic_ctx->frame_size, gl_dmic_ctx->dma_frame_size);
    return OPRT_OK;

__dmic_init_exit:
    __tkl_aud_dmic_dma_deconfig();
    bk_aud_dmic_deinit();
    if (gl_dmic_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_dmic_ctx->pm_voted = false;
    }

    if (gl_dmic_ctx) {
        if (gl_dmic_ctx->read_buf) {
            os_free(gl_dmic_ctx->read_buf);
        }

        os_free(gl_dmic_ctx);
        gl_dmic_ctx = NULL;
    }

    TKL_AUD_DMIC_LOG("dmic: init failed\n");
    return OPRT_COM_ERROR;
}

/**
 * @brief Start audio digital (DMIC) capture
 * @param[in] port Digital audio port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_start(TUYA_AUDIO_DMIC_PORT_E port)
{
    (void) port;
    uint32_t dmic_status = 0;
    bk_err_t status_ret = BK_FAIL;

    if (gl_dmic_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (gl_dmic_ctx->is_open) {
        TKL_AUD_DMIC_LOG("dmic: already started\n");
        return OPRT_OK;
    }

    bk_err_t ret = bk_dma_start(gl_dmic_ctx->dmic_dma_id);
    TKL_AUD_DMIC_LOG("dmic: bk_dma_start ret=%d\n", ret);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: dma start fail\n");
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_dmic_start();
    TKL_AUD_DMIC_LOG("dmic: bk_aud_dmic_start ret=%d\n", ret);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: start fail\n");
        bk_dma_stop(gl_dmic_ctx->dmic_dma_id);
        return OPRT_COM_ERROR;
    }

    gl_dmic_ctx->is_open = true;
    TKL_AUD_DMIC_LOG("dmic: start ok\n");

    return OPRT_OK;
}

/**
 * @brief Set audio digital (DMIC) gain
 * @param[in] port Digital audio port number
 * @param[in] gain Digital gain value
 * @return OPRT_OK on success, error code on failure
 * @note DMIC gain control is hardware dependent. Currently a no-op
 *       placeholder; implement when hardware gain register is available.
 */
OPERATE_RET tkl_aud_dmic_set_vol(TUYA_AUDIO_DMIC_PORT_E port, uint32_t gain)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief Stop audio digital (DMIC) capture
 * @param[in] port Digital audio port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_stop(TUYA_AUDIO_DMIC_PORT_E port)
{
    (void) port;
    uint32_t dmic_status = 0;
    bk_err_t status_ret = BK_FAIL;

    if (gl_dmic_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (!gl_dmic_ctx->is_open) {
        return OPRT_OK;
    }

    bk_err_t ret = bk_dma_stop(gl_dmic_ctx->dmic_dma_id);
    TKL_AUD_DMIC_LOG("dmic: bk_dma_stop ret=%d\n", ret);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: dma stop fail\n");
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_dmic_stop();
    TKL_AUD_DMIC_LOG("dmic: bk_aud_dmic_stop ret=%d\n", ret);
    if (ret != BK_OK) {
        TKL_AUD_DMIC_LOG("dmic: stop fail\n");
        return OPRT_COM_ERROR;
    }

    gl_dmic_ctx->is_open = false;

    return OPRT_OK;
}

/**
 * @brief Deinitialize audio digital (DMIC) input
 * @param[in] port Digital audio port number
 * @return none
 */
void tkl_aud_dmic_deinit(TUYA_AUDIO_DMIC_PORT_E port)
{
    if (gl_dmic_ctx == NULL) {
        return;
    }

    if (gl_dmic_ctx->is_open) {
        tkl_aud_dmic_stop(port);
    }

    __tkl_aud_dmic_dma_deconfig();
    bk_aud_dmic_deinit();

    if (gl_dmic_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_dmic_ctx->pm_voted = false;
    }

    if (gl_dmic_ctx->read_buf) {
        os_free(gl_dmic_ctx->read_buf);
    }

    os_free(gl_dmic_ctx);
    gl_dmic_ctx = NULL;
}
