/**
 * @file tkl_i2s_v2.c
 * @brief Optimized I2S adapter (refactored from tkl_i2s.c)
 * @version 2.0
 * @date 2026-04-09
 * @copyright Copyright (c) Tuya Inc.
 */
#include <os/mem.h>
#include <driver/i2s.h>
#include "tkl_output.h"
#include "tal_log.h"
#include "tkl_i2s.h"
#include "i2s_hal.h"
#include "tkl_system.h"
#include "bk_general_dma.h"
#include "tkl_gpio.h"
#include "i2s_hw.h"
#include <driver/i2s_types.h>

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define DEFAULT_CHANNEL_NUM  (2)
#define BITS_PER_BYTE        (8)
#define MS_20_DIV            (50)
#define BUFFER_NUM           (2)
#define DMA_MAGIC            (0xf0f0f0f0)
#define RATE_TBL_SIZE        (sizeof(s_rate_tbl) / sizeof(s_rate_tbl[0]))

#define I2S_CHECK_PORT(num) do {                            \
    if ((num) >= TUYA_I2S_NUM_MAX) {                        \
        bk_printf("i2s port %d is invalid\n", (int)(num));  \
        return OPRT_INVALID_PARM;                           \
    }                                                       \
} while (0)

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */

typedef struct {
    uint32_t        hz;
    i2s_samp_rate_t bk_rate;
} i2s_rate_entry_t;

typedef struct {
    uint8_t             rx_initialized : 1;
    uint8_t             tx_initialized : 1;
    TUYA_I2S_BASE_CFG_T i2s_config;
    RingBufferContext   *tx_rb;
    RingBufferContext   *rx_rb;
    uint8_t            *rx_buf;
    uint32_t            tx_frame_size;
    uint32_t            rx_frame_size;
} i2s_port_ctx_t;

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static i2s_port_ctx_t sg_i2s_port_ctx[TUYA_I2S_NUM_MAX] = {0};

/* Actual Hz -> BK sample rate enum mapping */
static const i2s_rate_entry_t s_rate_tbl[] = {
    {8000,  I2S_SAMP_RATE_8000},
    {11025, I2S_SAMP_RATE_11025},
    {12000, I2S_SAMP_RATE_12000},
    {16000, I2S_SAMP_RATE_16000},
    {22050, I2S_SAMP_RATE_22050},
    {24000, I2S_SAMP_RATE_24000},
    {32000, I2S_SAMP_RATE_32000},
    {44100, I2S_SAMP_RATE_44100},
    {48000, I2S_SAMP_RATE_48000},
};

/**
 * @brief Unified DMA finish callback for all I2S ports
 * @param[in] id I2S GPIO group, numerically equal to TUYA_I2S_NUM_*
 * @param[in] size half-buffer size in bytes
 * @param[in] type TX or RX direction
 * @return size on success, error code on failure
 * @note For TX: invokes frame_cb to notify TAL output worker (matching
 *       tkl_aud_dac DMA ISR pattern).
 *       For RX: reads one frame from ring buffer and invokes upper_cb
 *       directly from ISR (matching ADC/DMIC callback pattern).
 */
static int __i2s_dma_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    if (id >= TUYA_I2S_NUM_MAX) {
        return -1;
    }

    i2s_port_ctx_t *ctx = &sg_i2s_port_ctx[id];

    if (type == I2S_TXRX_TYPE_TX) {
        typedef VOID (*frame_cb_t)(TUYA_AUDIO_DAC_FRAME_EVT_E, void *);
        frame_cb_t cb = (frame_cb_t)ctx->i2s_config.upper_tx_cb;
        if (cb != NULL) {
            cb(TUYA_AUDIO_DAC_FRAME_EVENT_TX_COMPLETE,
               ctx->i2s_config.tx_args);
        }
        return (int)size;
    }

    /* RX path: read ring buffer and deliver via callback */
    TKL_AUD_INPUT_CB cb = (TKL_AUD_INPUT_CB)ctx->i2s_config.upper_rx_cb;
    if (cb == NULL || ctx->rx_buf == NULL) {
        return (int)size;
    }

    uint32_t frame_size = ctx->rx_frame_size;
    if (frame_size == 0) {
        return (int)size;
    }

    if (ctx->rx_rb != NULL && ring_buffer_get_fill_size(ctx->rx_rb) >= frame_size) {
        ring_buffer_read(ctx->rx_rb, ctx->rx_buf, frame_size);
        cb(TUYA_AUDIO_FRAME_EVENT_I2S_RX, ctx->rx_buf, frame_size,
           ctx->i2s_config.rx_args);
    }

    return (int)size;
}

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */

/**
 * @brief Map actual sample rate (Hz) to BK sample rate enum
 * @param[in] hz sample rate in Hz (e.g. 8000, 16000, 48000)
 * @param[out] out BK sample rate enum value
 * @return OPRT_OK if a matching rate was found, OPRT_INVALID_PARM otherwise
 */
static inline OPERATE_RET __i2s_hz_to_bk_rate(uint32_t hz, i2s_samp_rate_t *out)
{
    for (uint32_t i = 0; i < RATE_TBL_SIZE; i++) {
        if (s_rate_tbl[i].hz == hz) {
            *out = s_rate_tbl[i].bk_rate;
            return OPRT_OK;
        }
    }
    return OPRT_INVALID_PARM;
}

/**
 * @brief Set BK I2S work mode from Tuya communication format
 * @param[in] comm_fmt Tuya communication format enum
 * @param[out] out BK I2S config to populate
 * @return OPRT_OK on success, OPRT_INVALID_PARM on unsupported format
 */
static inline OPERATE_RET __i2s_set_work_mode(TUYA_I2S_COMM_FORMAT_E comm_fmt,
                                          i2s_config_t *out)
{
    switch (comm_fmt) {
        case I2S_COMM_FORMAT_STAND_I2S:
            out->work_mode = I2S_WORK_MODE_I2S;
            break;
        case I2S_COMM_FORMAT_STAND_MSB:
            out->work_mode = I2S_WORK_MODE_LEFTJUST;
            break;
        case I2S_COMM_FORMAT_STAND_PCM_SHORT:
            out->work_mode = I2S_WORK_MODE_SHORTFAMSYNC;
            break;
        case I2S_COMM_FORMAT_STAND_PCM_LONG:
            out->work_mode = I2S_WORK_MODE_LONGFAMSYNC;
            break;
        default:
            return OPRT_INVALID_PARM;
    }
    return OPRT_OK;
}

/**
 * @brief Set BK I2S channel store mode and PCM channel count
 * @param[in] ch_fmt Tuya channel format enum
 * @param[out] out BK I2S config to populate
 * @return OPRT_OK on success, OPRT_INVALID_PARM on unsupported format
 * @note I2S_LRCOM_STORE_16R16L: packs L+R into one 32-bit word;
 */
static inline OPERATE_RET __i2s_set_channel_cfg(TUYA_AUDIO_SAMPLE_BITS_E sample_bits, TUYA_I2S_CHANNEL_FMT_E ch_fmt,
                                            i2s_config_t *out)
{
    switch (ch_fmt) {
        case TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT:
            if (sample_bits == TUYA_AUDIO_SAMPLE_BITS_16) {
                out->store_mode  = I2S_LRCOM_STORE_16R16L;
            } else {
                out->store_mode  = I2S_LRCOM_STORE_LRLR;
            }
            break;
        case TUYA_I2S_CHANNEL_FMT_ALL_RIGHT:
        case TUYA_I2S_CHANNEL_FMT_ALL_LEFT:
            out->store_mode  = I2S_LRCOM_STORE_LRLR;
            break;
        case TUYA_I2S_CHANNEL_FMT_ONLY_RIGHT:
        case TUYA_I2S_CHANNEL_FMT_ONLY_LEFT:
        default:
            bk_printf("format not support %d\n", ch_fmt);
            return OPRT_INVALID_PARM;
    }
    out->pcm_chl_num = 2;
    return OPRT_OK;
}

/**
 * @brief Determine TX/RX type from the stored mode config
 * @param[in] num I2S port number (must be < TUYA_I2S_NUM_MAX)
 * @return I2S_TXRX_TYPE_TX or I2S_TXRX_TYPE_RX
 */
static inline i2s_txrx_type_t __i2s_get_txrx_type(TUYA_I2S_NUM_E num)
{
    return (sg_i2s_port_ctx[num].i2s_config.mode & TUYA_I2S_MODE_TX)
           ? I2S_TXRX_TYPE_TX : I2S_TXRX_TYPE_RX;
}

/**
 * @brief Calculate 20 ms frame size in bytes for a given I2S port
 * @param[in] num I2S port number
 * @return frame size in bytes, 0 if port invalid or not configured
 * @note sample_rate is the actual Hz value (e.g. 8000, 16000).
 *       bits_per_sample is guaranteed to be 8/16/24/32 (validated in init).
 *       RIGHT_LEFT with 16R16L store mode: each sample occupies 4 bytes.
 *       Mono channels: each sample occupies (bits_per_sample / 8) bytes.
 */
static inline uint32_t __i2s_calc_frame_size(TUYA_I2S_NUM_E num)
{
    if (num >= TUYA_I2S_NUM_MAX) {
        return 0;
    }
    const TUYA_I2S_BASE_CFG_T *cfg = &sg_i2s_port_ctx[num].i2s_config;

    if (cfg->channel_format == TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT) {
        return (cfg->sample_rate * 4) / MS_20_DIV;
    }
    return (cfg->sample_rate * (cfg->bits_per_sample / BITS_PER_BYTE))
           / MS_20_DIV;
}

/**
 * @brief Common stop implementation shared by send_stop and recv_stop
 * @param[in] i2s_num I2S port number
 * @return OPRT_OK on success
 */
static OPERATE_RET __i2s_stop(TUYA_I2S_NUM_E i2s_num)
{
    I2S_CHECK_PORT(i2s_num);
    BK_RETURN_ON_ERR(bk_i2s_stop_by_id(i2s_num));
    return OPRT_OK;
}

static void __i2s_prime_tx_dma(TUYA_I2S_NUM_E i2s_num)
{
    i2s_port_ctx_t *ctx;
    uint32_t fill_len;
    uint32_t free_len;
    uint8_t *silence;
    uint32_t written;

    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        return;
    }

    ctx = &sg_i2s_port_ctx[i2s_num];
    if (!ctx->tx_initialized || ctx->tx_rb == NULL) {
        return;
    }

    if (ring_buffer_get_fill_size(ctx->tx_rb) > 0) {
        return;
    }

    fill_len = ctx->tx_frame_size ? ctx->tx_frame_size : __i2s_calc_frame_size(i2s_num);
    free_len = ring_buffer_get_free_size(ctx->tx_rb);
    if (fill_len == 0 || free_len == 0) {
        return;
    }

    if (fill_len > free_len) {
        fill_len = free_len;
    }

    silence = (uint8_t *)tkl_system_malloc(fill_len);
    if (silence == NULL) {
        return;
    }

    os_memset(silence, 0, fill_len);
    written = ring_buffer_write(ctx->tx_rb, silence, fill_len);
    tkl_system_free(silence);

    if (written != fill_len) {
        bk_printf("i2s%d prime write error %u, expect %u\n",
                  (int)i2s_num, (unsigned)written, (unsigned)fill_len);
    }
}

/**
 * @brief Initialize I2S port
 * @param[in] i2s_num I2S port number
 * @param[in] cfg I2S base configuration
 * @return OPRT_OK on success, error code on failure
 * @note Uses goto-based cleanup to avoid duplicated teardown logic.
 *       For RX mode, a pre-allocated rx_buf is used by the DMA ISR to
 *       deliver data through upper_cb, matching the ADC/DMIC pattern.
 *       For TX mode, frame_cb is invoked from DMA ISR to notify the TAL
 *       output worker, matching the DAC pattern.
 */
OPERATE_RET tkl_i2s_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *cfg)
{
    I2S_CHECK_PORT(i2s_num);
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (!cfg->i2s_dma_flags) {
        bk_printf("only support dma mode\n");
        return OPRT_NOT_SUPPORTED;
    }

    i2s_port_ctx_t *ctx = &sg_i2s_port_ctx[i2s_num];

    /* Determine which directions this call needs to initialize */
    BOOL_T need_tx = ((cfg->mode & TUYA_I2S_MODE_TX) && !ctx->tx_initialized) ? TRUE : FALSE;
    BOOL_T need_rx = ((cfg->mode & TUYA_I2S_MODE_RX) && !ctx->rx_initialized) ? TRUE : FALSE;

    if (!need_tx && !need_rx) {
        return OPRT_OK;
    }

    /* If any direction was previously initialized, validate shared params */
    BOOL_T hw_initialized = (ctx->tx_initialized || ctx->rx_initialized) ? TRUE : FALSE;
    if (hw_initialized) {
        const TUYA_I2S_BASE_CFG_T *old = &ctx->i2s_config;
        if (cfg->sample_rate != old->sample_rate ||
            cfg->bits_per_sample != old->bits_per_sample ||
            cfg->channel_format != old->channel_format ||
            cfg->communication_format != old->communication_format) {
            bk_printf("i2s%d tx/rx config mismatch\n", (int)i2s_num);
            return OPRT_INVALID_PARM;
        }
    }

    OPERATE_RET ret = OPRT_OK;

    if (!hw_initialized) {
        i2s_config_t i2s_cfg = DEFAULT_I2S_CONFIG();

        if (cfg->bits_per_sample != TUYA_AUDIO_SAMPLE_BITS_8 &&
            cfg->bits_per_sample != TUYA_AUDIO_SAMPLE_BITS_16 &&
            cfg->bits_per_sample != TUYA_AUDIO_SAMPLE_BITS_24 &&
            cfg->bits_per_sample != TUYA_AUDIO_SAMPLE_BITS_32) {
            bk_printf("i2s unsupported parameter\n");
            return OPRT_INVALID_PARM;
        }

        /* Map Tuya config to BK config */
        i2s_cfg.role = (cfg->mode & TUYA_I2S_MODE_SLAVE) ? I2S_ROLE_SLAVE : I2S_ROLE_MASTER;
        ret = __i2s_set_work_mode(cfg->communication_format, &i2s_cfg);
        ret |= __i2s_set_channel_cfg(cfg->bits_per_sample, cfg->channel_format, &i2s_cfg);
        ret |= __i2s_hz_to_bk_rate(cfg->sample_rate, &i2s_cfg.samp_rate);
        if (ret != OPRT_OK) {
            bk_printf("i2s unsupported parameter\n");
            return OPRT_INVALID_PARM;
        }

        i2s_cfg.data_length = cfg->bits_per_sample;

        /* Initialize BK I2S driver */
        ret = bk_i2s_multi_driver_init();
        if (ret != BK_OK) {
            return OPRT_COM_ERROR;
        }

        ret = bk_i2s_init_by_id((i2s_gpio_group_id_t)i2s_num, &i2s_cfg);
        if (ret != BK_OK) {
            bk_i2s_multi_driver_deinit();
            return OPRT_COM_ERROR;
        }

        memcpy(&ctx->i2s_config, cfg, sizeof(TUYA_I2S_BASE_CFG_T));
    } else {
        /* Merge mode flags and new direction callbacks */
        ctx->i2s_config.mode |= cfg->mode;
        if (need_tx) {
            ctx->i2s_config.upper_tx_cb = cfg->upper_tx_cb;
            ctx->i2s_config.tx_args     = cfg->tx_args;
        }
        if (need_rx) {
            ctx->i2s_config.upper_rx_cb = cfg->upper_rx_cb;
            ctx->i2s_config.rx_args     = cfg->rx_args;
        }
    }

    /* DMA channel setup for each new direction */
    uint32_t frame_size = __i2s_calc_frame_size(i2s_num);

    if (need_tx) {
        ret = bk_i2s_chl_init_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_TX,
                                     frame_size * BUFFER_NUM, __i2s_dma_handle_cb,
                                     &ctx->tx_rb);
        if (ret != BK_OK) {
            goto err_deinit_hw;
        }
        ctx->tx_frame_size = frame_size;
    }

    if (need_rx) {
        ret = bk_i2s_chl_init_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_RX,
                                     frame_size * BUFFER_NUM, __i2s_dma_handle_cb,
                                     &ctx->rx_rb);
        if (ret != BK_OK) {
            goto err_deinit_tx_chl;
        }

        ctx->rx_buf = tkl_system_psram_malloc(frame_size);
        if (ctx->rx_buf == NULL) {
            bk_printf("i2s rx_buf alloc failed\n");
            goto err_deinit_rx_chl;
        }
        ctx->rx_frame_size = frame_size;
    }

    if (need_tx) { ctx->tx_initialized = 1; }
    if (need_rx) { ctx->rx_initialized = 1; }

    char *direction = NULL;
    if (need_rx && need_tx) direction = "txrx";
    else if (need_rx) direction = "rx";
    else if (need_tx) direction = "tx";
    else direction = "unknown";

    char *role = (cfg->mode & TUYA_I2S_MODE_SLAVE) ? "slave" : "master";

    bk_printf("i2s%d init, mode: %s %s, sample rate: %d, bits: %d, channel format: %d, protocol: %d\r\n",
            i2s_num, role, direction, cfg->sample_rate, cfg->bits_per_sample,
            cfg->channel_format, cfg->communication_format);

    return OPRT_OK;

err_deinit_rx_chl:
    bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_RX);
    ctx->rx_rb = NULL;
err_deinit_tx_chl:
    if (need_tx) {
        bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_TX);
        ctx->tx_rb = NULL;
    }
err_deinit_hw:
    if (!hw_initialized) {
        bk_i2s_deinit_by_id(i2s_num);
        bk_i2s_multi_driver_deinit();
    } else {
        /* Revert config merge so the running direction is not affected */
        if (need_tx) {
            ctx->i2s_config.mode &= ~TUYA_I2S_MODE_TX;
            ctx->i2s_config.upper_tx_cb = NULL;
            ctx->i2s_config.tx_args     = NULL;
        }
        if (need_rx) {
            ctx->i2s_config.mode &= ~TUYA_I2S_MODE_RX;
            ctx->i2s_config.upper_rx_cb = NULL;
            ctx->i2s_config.rx_args     = NULL;
        }
    }
    return OPRT_COM_ERROR;
}

/**
 * @brief Deinitialize I2S port and release all resources
 * @param[in] i2s_num I2S port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_i2s_deinit(TUYA_I2S_NUM_E i2s_num)
{
    I2S_CHECK_PORT(i2s_num);

    i2s_port_ctx_t *ctx = &sg_i2s_port_ctx[i2s_num];

    bk_i2s_stop_by_id(i2s_num);

    if (ctx->rx_buf != NULL) {
        tkl_system_psram_free(ctx->rx_buf);
        ctx->rx_buf = NULL;
    }

    if (ctx->tx_initialized) {
        bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_TX);
    }
    if (ctx->rx_initialized) {
        bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, I2S_TXRX_TYPE_RX);
    }

    bk_i2s_deinit_by_id(i2s_num);
    bk_i2s_multi_driver_deinit();
    os_memset(ctx, 0, sizeof(i2s_port_ctx_t));

    return OPRT_OK;
}

/**
 * @brief Write PCM data into the I2S DMA ring buffer (non-blocking)
 * @param[in] i2s_num I2S port number
 * @param[in] buff PCM data buffer
 * @param[in] len data length in bytes
 * @return OPRT_OK on success, OPRT_OS_ADAPTER_DAC_BUSY when ring buffer has
 *         insufficient free space (caller should retry after frame_cb fires)
 * @note Follows the same non-blocking pattern as tkl_aud_dac_write.
 *       The TAL output worker handles frame feeding via frame_cb events.
 */
OPERATE_RET tkl_i2s_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    I2S_CHECK_PORT(i2s_num);
    if (buff == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }
    if (!(sg_i2s_port_ctx[i2s_num].i2s_config.mode & TUYA_I2S_MODE_TX)) {
        return OPRT_COM_ERROR;
    }

    if (sg_i2s_port_ctx[i2s_num].tx_rb == NULL) {
        return OPRT_COM_ERROR;
    }

    uint32_t free_len = ring_buffer_get_free_size(sg_i2s_port_ctx[i2s_num].tx_rb);
    if (free_len < len) {
        return OPRT_OS_ADAPTER_DAC_BUSY;
    }

    uint32_t written = ring_buffer_write(sg_i2s_port_ctx[i2s_num].tx_rb,
                                          (uint8_t *)buff, len);
    if (written != len) {
        bk_printf("i2s ring_buffer_write error %u, expect %u\n",
                   (unsigned)written, (unsigned)len);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief Start I2S hardware (DMA + codec)
 * @param[in] i2s_num I2S port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_i2s_start(TUYA_I2S_NUM_E i2s_num)
{
    I2S_CHECK_PORT(i2s_num);
    __i2s_prime_tx_dma(i2s_num);
    BK_RETURN_ON_ERR(bk_i2s_start_by_id(i2s_num));
    return OPRT_OK;
}

/**
 * @brief Receive data via I2S (deprecated, data now delivered via upper_cb)
 * @param[in] i2s_num I2S port number
 * @param[out] buff receive buffer (unused)
 * @param[in] len receive size in bytes (unused)
 * @return OPRT_NOT_SUPPORTED
 */
int32_t tkl_i2s_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    (void)i2s_num;
    (void)buff;
    (void)len;
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief Stop I2S send
 * @param[in] i2s_num I2S port number
 * @return OPRT_OK on success
 */
OPERATE_RET tkl_i2s_send_stop(TUYA_I2S_NUM_E i2s_num)
{
    return __i2s_stop(i2s_num);
}

/**
 * @brief Stop I2S receive
 * @param[in] i2s_num I2S port number
 * @return OPRT_OK on success
 */
OPERATE_RET tkl_i2s_recv_stop(TUYA_I2S_NUM_E i2s_num)
{
    return __i2s_stop(i2s_num);
}

/**
 * @brief Set I2S volume
 * @param[in] i2s_num I2S port number
 * @param[in] gain volume gain
 * @return OPRT_NOT_SUPPORTED
 */
OPERATE_RET tkl_i2s_set_vol(TUYA_I2S_NUM_E i2s_num, uint32_t gain)
{
    return OPRT_NOT_SUPPORTED;
}
