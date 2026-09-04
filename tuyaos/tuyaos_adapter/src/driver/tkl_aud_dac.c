
#include <common/bk_include.h>
#include <os/os.h>
#include <driver/aud_dac.h>
#include <driver/aud_common.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include <driver/gpio.h>
#include <driver/flash.h>
#include <driver/flash_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include "gpio_driver.h"

#include "tuya_cloud_types.h"
#include "tkl_aud_dac.h"
#include "tkl_aud_pm.h"

extern void bk_printf(const char *fmt, ...);

#define TKL_DAC_LOG(fmt, ...) \
    bk_printf("%s %d, " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define DMA_BUF_SAFE_INTERVAL    (8)

// x: sample rate, y: sample bits, z: frame time in ms, channel only 1
#define TKL_T5_DAC_FRAME_SIZE_MS(x, y, z)  ((x * (y / 8) / 1000) * z)

/* DAC 数字增益默认值(原始寄存器值, 范围 0x00~0x3F, 非 tkl_aud_dac_set_volume 的 0~100 百分比)。
 * 上层传入 volume==0 时用该缺省值, 保证 cfg.volume>0 使 tkl_aud_dac_start 正常 unmute
 * (避免首次播放后被 stop 静音、后续再不解除)。 */
#define TKL_AUD_DAC_DEFAULT_GAIN  0x26

typedef enum {
    TKL_AUD_DAC_STAT_IDLE = 0,
    TKL_AUD_DAC_STAT_INIT,
    TKL_AUD_DAC_STAT_START,
    TKL_AUD_DAC_STAT_STOP,
    TKL_AUD_DAC_STAT_DEINIT,
} TKL_AUD_DAC_STAT_E;

typedef struct {
    TKL_AUD_DAC_CFG_T       cfg;
    TUYA_AUDIO_DAC_PORT_E   port;
    TKL_AUD_DAC_STAT_E      stat;
    bool                    pm_voted;
    dma_id_t                spk_dma_id;
    RingBufferContext       spk_rb;
    int8_t                 *spk_dma_buf;
    uint32_t                frame_size;
    TKL_AUD_DAC_FRAME_CB    frame_cb;
    void                 *frame_cb_arg;
} tkl_aud_dac_ctx_t;

static tkl_aud_dac_ctx_t *gl_dac_ctx = NULL;
static aud_dac_config_t aud_dac_cfg = DEFAULT_AUD_DAC_CONFIG();

dma_id_t test_dma_id = DMA_ID_MAX;
/* ---------------------- DMA ISR -------------------------------------------- */

static void __tkl_aud_dac_dma_finish_isr(void)
{
    if ((gl_dac_ctx != NULL) && (gl_dac_ctx->frame_cb != NULL)) {
        gl_dac_ctx->frame_cb(TUYA_AUDIO_DAC_FRAME_EVENT_TX_COMPLETE,
                gl_dac_ctx->frame_cb_arg);
    }
}

/* ---------------------- DMA config / deconfig ------------------------------ */

static bk_err_t __tkl_aud_dac_dma_deconfig(void)
{
    if (gl_dac_ctx == NULL) {
        return BK_OK;
    }

    bk_dma_deinit(gl_dac_ctx->spk_dma_id);
    bk_dma_free(DMA_DEV_AUDIO, gl_dac_ctx->spk_dma_id);

    if (gl_dac_ctx->spk_dma_buf) {
        ring_buffer_clear(&gl_dac_ctx->spk_rb);
        audio_dma_mem_free(gl_dac_ctx->spk_dma_buf);
        gl_dac_ctx->spk_dma_buf = NULL;
    }

    return BK_OK;
}

static bk_err_t __tkl_aud_dac_dma_config(void)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t dac_port_addr;
    uint32_t frame_size = gl_dac_ctx->frame_size;

    gl_dac_ctx->spk_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
    if ((gl_dac_ctx->spk_dma_id < DMA_ID_0) || (gl_dac_ctx->spk_dma_id >= DMA_ID_MAX)) {
        TKL_DAC_LOG("malloc dma fail\n");
        goto exit;
    }

    test_dma_id = gl_dac_ctx->spk_dma_id;

    /* Two frames + safe interval for dma_set_src_pause_addr flow control */
    gl_dac_ctx->spk_dma_buf = (int8_t *)audio_dma_mem_calloc_on_sram(2, frame_size + DMA_BUF_SAFE_INTERVAL / 2);
    if (gl_dac_ctx->spk_dma_buf == NULL) {
        TKL_DAC_LOG("malloc dma buffer fail\n");
        goto exit;
    }

    /*
     * RB_DMA_TYPE_READ: DMA reads FROM ring buffer TO DAC FIFO.
     * ring_buffer_init sets dma_set_src_pause_addr(dma_id, addr) — DMA paused at start.
     * ring_buffer_write updates dma_set_src_pause_addr to allow DMA to read new data.
     */
    ring_buffer_init(&gl_dac_ctx->spk_rb,
                     (uint8_t *)gl_dac_ctx->spk_dma_buf,
                     frame_size * 2 + DMA_BUF_SAFE_INTERVAL,
                     gl_dac_ctx->spk_dma_id,
                     RB_DMA_TYPE_READ);

    os_memset(&dma_config, 0, sizeof(dma_config_t));
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;

    /* src: ring buffer (DTCM) */
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = (uint32_t)(uintptr_t)gl_dac_ctx->spk_dma_buf;
    dma_config.src.end_addr = (uint32_t)(uintptr_t)gl_dac_ctx->spk_dma_buf
                              + frame_size * 2 + DMA_BUF_SAFE_INTERVAL;

    /* dst: DAC FIFO */
    dma_config.dst.dev = DMA_DEV_AUDIO;
    switch (gl_dac_ctx->cfg.chan_num) {
        case 1:
            dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
            break;
        case 2:
            dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
            break;
        default:
            break;
    }

    ret = bk_aud_dac_get_fifo_addr(&dac_port_addr);
    if (ret != BK_OK) {
        TKL_DAC_LOG("get dac fifo address fail\n");
        goto exit;
    }
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = dac_port_addr;
    dma_config.dst.end_addr = dac_port_addr + 4;

    ret = bk_dma_init(gl_dac_ctx->spk_dma_id, &dma_config);
    if (ret != BK_OK) {
        TKL_DAC_LOG("dma_init fail\n");
        goto exit;
    }

    bk_dma_set_transfer_len(gl_dac_ctx->spk_dma_id, frame_size);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(gl_dac_ctx->spk_dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(gl_dac_ctx->spk_dma_id, DMA_ATTR_SEC);
#endif

    bk_dma_register_isr(gl_dac_ctx->spk_dma_id, NULL, (void *)__tkl_aud_dac_dma_finish_isr);
    bk_dma_enable_finish_interrupt(gl_dac_ctx->spk_dma_id);

    TKL_DAC_LOG("dma_config dma_id: %d, transfer_len: %d\n",
                 gl_dac_ctx->spk_dma_id, frame_size);

    return BK_OK;

exit:
    __tkl_aud_dac_dma_deconfig();
    return BK_FAIL;
}

/* ---------------------- Public API ----------------------------------------- */

/**
 * @brief Initialize the DAC playback context and hardware resources.
 * @param[in] config DAC configuration used to set sample rate, frame size,
 *                   initial volume, and frame callback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The current platform only supports 16-bit PCM output and forces the
 *       playback channel count to one onboard speaker.
 * @note This function must be called before `tkl_aud_dac_start()`.
 */
OPERATE_RET tkl_aud_dac_init(TUYA_AUDIO_DAC_PORT_E port, TKL_AUD_DAC_CFG_T *config)
{
    (void)port;

    if (config == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (config->sample_bits != TUYA_AUDIO_SAMPLE_BITS_16) {
        TKL_DAC_LOG("dac only support 16bits sample");
        return OPRT_INVALID_PARM;
    }

    if (config->frame_time_ms == 0) {
        TKL_DAC_LOG("dac frame time is invalid\n");
        return OPRT_INVALID_PARM;
    }

    if (gl_dac_ctx != NULL) {
        TKL_DAC_LOG("dac already initialized\n");
        return OPRT_COM_ERROR;
    }

    bk_err_t ret = BK_OK;

    gl_dac_ctx = (tkl_aud_dac_ctx_t *)tkl_system_malloc(sizeof(tkl_aud_dac_ctx_t));
    if (gl_dac_ctx == NULL) {
        TKL_DAC_LOG("malloc dac context fail\n");
        return OPRT_MALLOC_FAILED;
    }
    os_memset(gl_dac_ctx, 0, sizeof(tkl_aud_dac_ctx_t));
    os_memcpy(&gl_dac_ctx->cfg, config, sizeof(TKL_AUD_DAC_CFG_T));
    gl_dac_ctx->frame_cb = config->frame_cb;
    gl_dac_ctx->frame_cb_arg = config->args;

    /* T5 only has one onboard speaker, force set */
    gl_dac_ctx->cfg.chan_num = 1;
    gl_dac_ctx->cfg.volume = config->volume == 0 ? TKL_AUD_DAC_DEFAULT_GAIN : config->volume;

    tkl_aud_pm_acquire();
    gl_dac_ctx->pm_voted = true;

    /* DAC init */
    if (gl_dac_ctx->cfg.chan_num == 1) {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_L;
    } else {
        aud_dac_cfg.dac_chl = AUD_DAC_CHL_LR;
    }
    aud_dac_cfg.samp_rate = config->sample_rate;
    aud_dac_cfg.work_mode = AUD_DAC_WORK_MODE_DIFFEN;
    // aud_dac_cfg.clk_src = AUD_CLK_XTAL;
    aud_dac_cfg.clk_src = AUD_CLK_APLL;
    aud_dac_cfg.dac_gain = gl_dac_ctx->cfg.volume;

    TKL_DAC_LOG("dac config, chl %d, vol 0x%02x, samp_rate: %d\n",
                 aud_dac_cfg.dac_chl, aud_dac_cfg.dac_gain, aud_dac_cfg.samp_rate);

    ret = bk_aud_dac_init(&aud_dac_cfg);
    if (ret != BK_OK) {
        TKL_DAC_LOG("bk_aud_dac_init fail\n");
        goto __dac_init_exit;
    }

    if (aud_dac_cfg.dac_gain == 0) {
        bk_aud_dac_mute();
    } else {
        bk_aud_dac_unmute();
    }

    gl_dac_ctx->frame_size = TKL_T5_DAC_FRAME_SIZE_MS(config->sample_rate, config->sample_bits,
                                                      config->frame_time_ms);
    if (gl_dac_ctx->frame_size == 0) {
        TKL_DAC_LOG("dac frame size calc fail\n");
        ret = BK_FAIL;
        goto __dac_init_exit;
    }

    /* DMA config */
    ret = __tkl_aud_dac_dma_config();
    if (ret != BK_OK) {
        TKL_DAC_LOG("dma config fail\n");
        goto __dac_init_exit;
    }

    gl_dac_ctx->port = port;
    gl_dac_ctx->stat = TKL_AUD_DAC_STAT_INIT;

    return OPRT_OK;

__dac_init_exit:

    gl_dac_ctx->stat = TKL_AUD_DAC_STAT_IDLE;

    __tkl_aud_dac_dma_deconfig();
    bk_aud_dac_deinit();

    if (gl_dac_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_dac_ctx->pm_voted = false;
    }

    if (gl_dac_ctx) {
        tkl_system_free(gl_dac_ctx);
        gl_dac_ctx = NULL;
    }
    return OPRT_COM_ERROR;
}

/**
 * @brief Start DAC playback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The driver pre-fills the DMA ring buffer with silence before enabling
 *       DMA and DAC output to reduce pop noise on startup.
 * @note Calling this function after playback has already started returns
 *       success without reinitializing the hardware.
 */
OPERATE_RET tkl_aud_dac_start(TUYA_AUDIO_DAC_PORT_E port)
{
    (void)port;

    if (gl_dac_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_START) {
        return OPRT_OK;
    } else if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_IDLE ||
               gl_dac_ctx->stat == TKL_AUD_DAC_STAT_DEINIT) {
        return OPRT_COM_ERROR;
    }

    /*
     * Pre-fill ring buffer with silence before starting DMA+DAC.
     * This prevents initial pop noise and ensures DMA has data to read.
     */
    uint32_t free_size = ring_buffer_get_free_size(&gl_dac_ctx->spk_rb);
    if (free_size > DMA_BUF_SAFE_INTERVAL) {
        uint32_t fill_len = free_size - DMA_BUF_SAFE_INTERVAL;
        uint8_t *silence = (uint8_t *)tkl_system_malloc(fill_len);
        if (silence) {
            os_memset(silence, 0, fill_len);
            ring_buffer_write(&gl_dac_ctx->spk_rb, silence, fill_len);
            tkl_system_free(silence);
        }
    }

    bk_err_t ret = bk_dma_start(gl_dac_ctx->spk_dma_id);
    if (ret != BK_OK) {
        TKL_DAC_LOG("dma start fail\n");
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_dac_start();
    if (ret != BK_OK) {
        TKL_DAC_LOG("dac start fail\n");
        bk_dma_stop(gl_dac_ctx->spk_dma_id);
        return OPRT_COM_ERROR;
    }

    if (gl_dac_ctx->cfg.volume > 0) {
        bk_aud_dac_unmute();
    }

    gl_dac_ctx->stat = TKL_AUD_DAC_STAT_START;
    TKL_DAC_LOG("dac start ok\n");

    return OPRT_OK;
}

/**
 * @brief Stop DAC playback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note This function stops DMA transfer first, mutes the DAC output, and
 *       then stops the DAC hardware.
 * @note Calling this function after playback has already stopped returns
 *       success without additional hardware operations.
 */
OPERATE_RET tkl_aud_dac_stop(TUYA_AUDIO_DAC_PORT_E port)
{
    (void)port;

    if (gl_dac_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_STOP) {
        return OPRT_OK;
    } else if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_IDLE ||
               gl_dac_ctx->stat == TKL_AUD_DAC_STAT_DEINIT) {
        return OPRT_COM_ERROR;
    }

    bk_err_t ret = bk_dma_stop(gl_dac_ctx->spk_dma_id);
    if (ret != BK_OK) {
        TKL_DAC_LOG("dma stop fail\n");
        return OPRT_COM_ERROR;
    }

    bk_aud_dac_mute();

    ret = bk_aud_dac_stop();
    if (ret != BK_OK) {
        TKL_DAC_LOG("dac stop fail\n");
        return OPRT_COM_ERROR;
    }

    gl_dac_ctx->stat = TKL_AUD_DAC_STAT_STOP;

    return OPRT_OK;
}

/**
 * @brief Update the DAC output volume.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @param[in] volume Logical volume value in the range `0x00` to `0x3F`.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The input volume is remapped to a non-linear hardware gain curve
 *       before calling the underlying DAC gain API.
 * @note A volume value of `0` mutes the DAC output. Non-zero values unmute it.
 */
OPERATE_RET tkl_aud_dac_set_volume(TUYA_AUDIO_DAC_PORT_E port, uint32_t volume)
{
    (void)port;

    if (gl_dac_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_IDLE ||
        gl_dac_ctx->stat == TKL_AUD_DAC_STAT_DEINIT) {
        return OPRT_COM_ERROR;
    }

    // if (volume > 0x3f) {
    //     TKL_DAC_LOG("volume 0x%02x out of range: 0x00 ~ 0x3f\n", volume);
    //     return OPRT_INVALID_PARM;
    // }
    TKL_DAC_LOG("volume set %d\n", volume);

    // 重新映射音量范围
    if (volume <= 30) {
        // 0-30% 映射到 0-50%
        volume = volume * 50 / 30;
    } else {
        // 30-100% 映射到 50-70%
        volume = 50 + (volume - 30) * 20 / 70;
    }
    volume =(uint32_t)(volume * 0x3F / 100) ;

    if (volume == 0) {
        bk_aud_dac_mute();
    } else {
        bk_aud_dac_unmute();
    }

    if (bk_aud_dac_set_gain(volume) != BK_OK) {
        TKL_DAC_LOG("set dac gain fail\n");
        return OPRT_COM_ERROR;
    }

    gl_dac_ctx->cfg.volume = (uint8_t)volume;

    return OPRT_OK;
}

/**
 * @brief Write PCM data into the DAC DMA ring buffer.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @param[in] buffer Pointer to the PCM data buffer.
 * @param[in] len PCM data length in bytes.
 * @return OPRT_OK on success, `OPRT_OS_ADAPTER_DAC_BUSY` when the ring buffer
 *         does not have enough free space, retry after delay, or other error
 *         codes on failure.
 * @note The DAC must already be in the started state before this function is
 *       called.
 * @note Data is copied into the internal DMA ring buffer, so the caller must
 *       ensure that enough free space is available for the requested length.
 */
OPERATE_RET tkl_aud_dac_write(TUYA_AUDIO_DAC_PORT_E port, uint8_t *buffer, uint32_t len)
{
    uint32_t write_len;
    uint32_t free_len;

    (void)port;

    if (gl_dac_ctx == NULL || buffer == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }

    if (gl_dac_ctx->stat != TKL_AUD_DAC_STAT_START) {
        return OPRT_COM_ERROR;
    }

    free_len = ring_buffer_get_free_size(&gl_dac_ctx->spk_rb);
    if (free_len < len) {
        return OPRT_OS_ADAPTER_DAC_BUSY;
    }

    write_len = ring_buffer_write(&gl_dac_ctx->spk_rb, buffer, len);
    if (write_len != len) {
        TKL_DAC_LOG("ring_buffer_write error %d, expect %d\n", write_len, len);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief Deinitialize the DAC driver instance.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note If playback is still running, this function stops it first and then
 *       releases DMA, DAC, callback, and power-management resources.
 */
OPERATE_RET tkl_aud_dac_deinit(TUYA_AUDIO_DAC_PORT_E port)
{
    (void)port;

    if (gl_dac_ctx == NULL) {
        return OPRT_COM_ERROR;
    }

    if (gl_dac_ctx->stat == TKL_AUD_DAC_STAT_START) {
        tkl_aud_dac_stop(port);
    }

    gl_dac_ctx->stat = TKL_AUD_DAC_STAT_DEINIT;
    gl_dac_ctx->frame_cb = NULL;
    gl_dac_ctx->frame_cb_arg = NULL;

    __tkl_aud_dac_dma_deconfig();
    bk_aud_dac_deinit();

    if (gl_dac_ctx->pm_voted) {
        tkl_aud_pm_release();
        gl_dac_ctx->pm_voted = false;
    }

    tkl_system_free(gl_dac_ctx);
    gl_dac_ctx = NULL;

    return OPRT_OK;
}

/**
 * @brief Get the configured DAC frame size in bytes.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return Frame size in bytes, or `0` if the DAC driver is not initialized.
 */
uint32_t tkl_aud_dac_get_frame_size(TUYA_AUDIO_DAC_PORT_E port)
{
    (void)port;

    if (gl_dac_ctx == NULL) {
        return 0;
    }

    return gl_dac_ctx->frame_size;
}
