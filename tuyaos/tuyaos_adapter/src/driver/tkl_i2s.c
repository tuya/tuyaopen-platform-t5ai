/**
 * @file tkl_i2s.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

// --- BEGIN: user defines and implements ---
#include <os/mem.h>
#include <driver/i2s.h>
#include "tkl_output.h"
// #include "tal_log.h"
#include "tkl_i2s.h"
#include "i2s_hal.h"
#include "tkl_semaphore.h"
#include "tkl_queue.h"
#include "tkl_system.h"
#include "bk_general_dma.h"
#include "tkl_gpio.h"
#include "i2s_hw.h"
#include "tkl_thread.h"
#include <driver/i2s_types.h>
// --- END: user defines and implements ---

/***********************************************************
************************macro define************************
***********************************************************/
#define DEFAULT_CHANNEL_NUM  (2)
#define BITS_PER_BYTE        (8)
#define MS_20_DIV            (50)
#define BUFFER_NUM           (2)
#define DEFAULT_SAMPLE       (16000)
#define DMA_MAGIC         (0xf0f0f0f0)
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct i2s_cb_config {
    i2s_data_handle_cb tx_cb;   //发送回调，dma发送完成时调用
    i2s_data_handle_cb rx_cb;   //接收回调，dma接收完成时调用
}i2s_cb_config_t;

/***********************************************************
********************function declaration********************
***********************************************************/
static int ch0_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
static int ch0_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
static int ch1_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
static int ch1_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
static int ch2_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
static int ch2_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type);
/***********************************************************
***********************variable define**********************
***********************************************************/
i2s_cb_config_t m_i2s_cfg[TUYA_I2S_NUM_MAX] = {
    {ch0_tx_data_handle_cb,ch0_rx_data_handle_cb},
    {ch1_tx_data_handle_cb,ch1_rx_data_handle_cb},
    {ch2_tx_data_handle_cb,ch2_rx_data_handle_cb},
};
static TUYA_I2S_BASE_CFG_T m_i2s_config[TUYA_I2S_NUM_MAX] = {0};
static RingBufferContext *m_ringbuffer[TUYA_I2S_NUM_MAX] = {NULL};
static TKL_SEM_HANDLE m_semaphore[TUYA_I2S_NUM_MAX] = {0};
static TKL_QUEUE_HANDLE m_i2s_msg_queue[TUYA_I2S_NUM_MAX] = {0};

static TKL_THREAD_HANDLE i2s_handle[TUYA_I2S_NUM_MAX] = {NULL};
/***********************************************************
***********************function define**********************
************************************************************/

static inline uint32_t i2s_calc_frame_size(TUYA_I2S_NUM_E num) {
    const TUYA_I2S_BASE_CFG_T *cfg = &m_i2s_config[num];
    if (num >= TUYA_I2S_NUM_MAX || cfg->bits_per_sample == 0) {
        return 0;
    }
    uint32_t real_sample = DEFAULT_SAMPLE;
    switch (cfg->sample_rate) { // 与原枚举的映射保持一致
        case 1: real_sample = 8000; break;
        case 2: real_sample = 11025; break;
        case 3: real_sample = 12000; break;
        case 4: real_sample = 16000; break;
        case 5: real_sample = 22050; break;
        case 6: real_sample = 24000; break;
        case 7: real_sample = 32000; break;
        case 8: real_sample = 44100; break;
        case 9: real_sample = 48000; break;
        default: break;
    }
    /*
     * 20ms frame_size 必须与 BK I2S DMA 实际写入/读取 ringbuffer 的“每采样点占用字节”一致。
     *
     * BK 驱动在 I2S/LEFTJUST/RIGHTJUST 下使用：smp_ratio = data_len - 1
     * 在 PCM short/long 下使用：smp_ratio = data_len * pcm_chl_num - 1
     * 即 PCM 下“每个采样点”的时隙数取决于 pcm_chl_num。
     *
     * 同时，store_mode 会改变内存中的打包方式：
     * - I2S_LRCOM_STORE_16R16L：左右声道拼成 1 个 32bit word（每采样点 4 字节）
     * - I2S_LRCOM_STORE_LRLR：按 L/R 时序交替写入（通常每时隙占用 bits_per_sample 对应字节数）
     */
    uint32_t bytes_per_slot = (cfg->bits_per_sample + 7) / 8;
    uint32_t slots_per_sample = 1;

    if (cfg->communication_format == I2S_COMM_FORMAT_STAND_PCM_SHORT ||
        cfg->communication_format == I2S_COMM_FORMAT_STAND_PCM_LONG) {
        /*
         * TUYA 侧配置在 init 时：
         * - RIGHT_LEFT -> pcm_chl_num = 2
         * - ONLY/ALL   -> pcm_chl_num = 1
         */
        slots_per_sample = (cfg->channel_format == TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT) ? 2 : 1;
    } else {
        slots_per_sample = (cfg->channel_format == TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT) ? 2 : 1;
    }

    if (cfg->channel_format == TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT) {
        // 对齐本文件 init() 的 store_mode 选择：RIGHT_LEFT 使用 16R16L，内存每采样点固定 4 字节
        return (real_sample * 4) / MS_20_DIV;
    }

    return (real_sample * slots_per_sample * bytes_per_slot) / MS_20_DIV;
}

static int ch0_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_0]);
    if (ret != BK_OK) {
        return ret;
    }

	return size;
}

static int32_t ch0_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_0]);
    if (ret != BK_OK) {
        return ret;
    }
	return size;
}
static int ch1_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_1]);
    if (ret != BK_OK) {
        return ret;
    }

	return size;
}

static int32_t ch1_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_1]);
    if (ret != BK_OK) {
        return ret;
    }
	return size;
}
static int ch2_tx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_2]);
    if (ret != BK_OK) {
        return ret;
    }

	return size;
}

static int32_t ch2_rx_data_handle_cb(i2s_gpio_group_id_t id, uint32_t size, i2s_txrx_type_t type)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_2]);
    if (ret != BK_OK) {
        return ret;
    }
	return size;
}
// 统一“队列满时丢弃最早的数据再入队”的策略，避免保留旧数据
static inline OPERATE_RET i2s_queue_push_latest(TKL_QUEUE_HANDLE q, void **pdata) {
    OPERATE_RET ret = tkl_queue_post(q, pdata, 0);
    if (ret == OPRT_OK) {
        return ret;
    }
    // 队列可能已满，丢弃最早数据，再尝试入队一次
    void *old = NULL;
    if (tkl_queue_fetch(q, &old, 0) == OPRT_OK && old) {
        // 释放旧帧内存
        tkl_system_psram_free(old);
        old = NULL;
    }
    return tkl_queue_post(q, pdata, 0);
}

static void i2s_handle_task(void)
{
    int32_t ret = 0L;
    uint32_t val = 0U;
    void *data = NULL;
    uint32_t frame_size = 0U;
    while (1) {
        for (size_t i = 0; i < TUYA_I2S_NUM_MAX; i++) {
            uint32_t frame_size = i2s_calc_frame_size(i);
            if (frame_size == 0) {
                continue;
            }
            if (m_i2s_config[i].i2s_dma_flags && (m_i2s_config[i].mode & TUYA_I2S_MODE_RX)) {
                if (tkl_semaphore_wait(m_semaphore[i], 20) != BK_OK) {
                    continue;
                }
                size_t fill = ring_buffer_get_fill_size(m_ringbuffer[i]);
                int loop_guard = (fill / frame_size);
                if (loop_guard > 8) loop_guard = 8;
                if (loop_guard < 1) loop_guard = 1;
                while (loop_guard-- > 0 && ring_buffer_get_fill_size(m_ringbuffer[i]) >= frame_size) {
                    void *data = tkl_system_psram_malloc(frame_size);
                    if (!data) {
                        break;
                    }
                    ring_buffer_read(m_ringbuffer[i], data, frame_size);
                    if (i2s_queue_push_latest(m_i2s_msg_queue[i], &data) != OPRT_OK) {
                        tkl_system_psram_free(data);
                        data = NULL;
                        break;
                    }
                }
            }
        }
        // 合理让出 CPU，节省功耗
        tkl_system_sleep(2);
    }
}

OPERATE_RET tkl_i2s_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *cfg)
{
    // --- BEGIN: user implements ---
    OPERATE_RET ret = 0;
    uint32_t val = 0U;
    i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
    i2s_data_handle_cb i2s_cb = NULL;
    i2s_txrx_type_t type = I2S_TXRX_TYPE_MAX;
    uint32_t frame_size = 0U;

    if ((i2s_num >= TUYA_I2S_NUM_MAX) || (cfg == NULL)) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }

    if (cfg->mode & TUYA_I2S_MODE_SLAVE) {
        i2s_config.role = I2S_ROLE_SLAVE;
    }
    else {
        i2s_config.role = I2S_ROLE_MASTER;
    }
    switch (cfg->communication_format) {
        case I2S_COMM_FORMAT_STAND_I2S:
            i2s_config.work_mode = I2S_WORK_MODE_I2S;
            break;
        case I2S_COMM_FORMAT_STAND_MSB:
            i2s_config.work_mode = I2S_WORK_MODE_LEFTJUST;
            break;
        case I2S_COMM_FORMAT_STAND_PCM_SHORT:
            i2s_config.work_mode = I2S_WORK_MODE_SHORTFAMSYNC;
            break;
        case I2S_COMM_FORMAT_STAND_PCM_LONG:
            i2s_config.work_mode = I2S_WORK_MODE_LONGFAMSYNC;
            break;
        default:
            i2s_config.work_mode = I2S_WORK_MODE_I2S;
            break;
    }
    /*用于配置通道内数据的存储模式，I2S_LRCOM_STORE_16R16L：左右声道数据拼成一个32bit同时写入PCM_DAT中，低16bit对应左声道，高16bit对应右声道，即{R,L}->{R,L}->……（仅当数据长度小于16时才能配置此模式）。I2S_LRCOM_STORE_LRLR：左右声道数据按时间顺序交替写入PCM_DAT中，即L->R->L->R->…… 。*/
    switch (cfg->channel_format) {
        case TUYA_I2S_CHANNEL_FMT_RIGHT_LEFT:
            i2s_config.store_mode = I2S_LRCOM_STORE_16R16L;
            i2s_config.pcm_chl_num = DEFAULT_CHANNEL_NUM;
            break;
        case TUYA_I2S_CHANNEL_FMT_ALL_RIGHT:
        case TUYA_I2S_CHANNEL_FMT_ALL_LEFT:
        case TUYA_I2S_CHANNEL_FMT_ONLY_RIGHT:
        case TUYA_I2S_CHANNEL_FMT_ONLY_LEFT:
            i2s_config.store_mode = I2S_LRCOM_STORE_LRLR;
            i2s_config.pcm_chl_num = 1;
            break;
        default:
            i2s_config.store_mode = I2S_LRCOM_STORE_16R16L;
            i2s_config.pcm_chl_num = DEFAULT_CHANNEL_NUM;
            break;
    }
	switch (cfg->sample_rate) {
		case 1:		//8k
            i2s_config.samp_rate = I2S_SAMP_RATE_8000;
			break;

		case 2:		//11.025k
            i2s_config.samp_rate = I2S_SAMP_RATE_11025;
			break;

		case 3:		//12k
            i2s_config.samp_rate = I2S_SAMP_RATE_12000;
			break;

		case 4:		//16k
            i2s_config.samp_rate = I2S_SAMP_RATE_16000;
			break;

		case 5:		//22.05k
            i2s_config.samp_rate = I2S_SAMP_RATE_22050;
			break;

		case 6:		//24k
            i2s_config.samp_rate = I2S_SAMP_RATE_24000;
			break;

		case 7:		//32k
            i2s_config.samp_rate = I2S_SAMP_RATE_32000;
			break;

		case 8:		//44.1k
            i2s_config.samp_rate = I2S_SAMP_RATE_44100;
			break;

		case 9:		//48k
            i2s_config.samp_rate = I2S_SAMP_RATE_48000;
			break;

		default:
			break;
	}

    ret = bk_i2s_multi_driver_init();
    if (ret != BK_OK) {
        return OPRT_COM_ERROR;
    }
    
    i2s_config.data_length = cfg->bits_per_sample;
    ret = bk_i2s_init_by_id((i2s_gpio_group_id_t)i2s_num, &i2s_config);
    if (ret != BK_OK) {
        bk_i2s_multi_driver_deinit();
        return OPRT_COM_ERROR;
    }
    //配置信息前置，避免任务运行时参数不一致导致任务无法进入阻塞状态
    memcpy(&m_i2s_config[i2s_num], cfg, sizeof(TUYA_I2S_BASE_CFG_T));
    if (cfg->i2s_dma_flags) {
        if (cfg->mode & TUYA_I2S_MODE_TX) {
            type = I2S_TXRX_TYPE_TX;
            i2s_cb = m_i2s_cfg[i2s_num].tx_cb;
        } else {
            type = I2S_TXRX_TYPE_RX;
            i2s_cb = m_i2s_cfg[i2s_num].rx_cb;
        }
        ret = tkl_semaphore_create_init(&m_semaphore[i2s_num], 0, 2);
        if (ret != BK_OK) {
            bk_i2s_deinit_by_id(i2s_num);
            bk_i2s_multi_driver_deinit();
            bk_printf("i2s sem create failed %d\n", ret);
			return OPRT_COM_ERROR;
		}
        frame_size = i2s_calc_frame_size(i2s_num);
        ret = bk_i2s_chl_init_by_id(i2s_num, I2S_CHANNEL_1, type, frame_size * BUFFER_NUM, i2s_cb, &m_ringbuffer[i2s_num]);
		if (ret != BK_OK) {
            bk_i2s_deinit_by_id(i2s_num);
            bk_i2s_multi_driver_deinit();
            tkl_semaphore_release(m_semaphore[i2s_num]);
            m_semaphore[i2s_num] = NULL;
			return OPRT_COM_ERROR;
		}
        ret = tkl_queue_create_init(&m_i2s_msg_queue[i2s_num], sizeof(void *), 10);
		if (ret != BK_OK) {
            bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, type);
            tkl_semaphore_release(m_semaphore[i2s_num]);
            m_semaphore[i2s_num] = NULL;
            bk_i2s_deinit_by_id(i2s_num);
            bk_i2s_multi_driver_deinit();
			return OPRT_COM_ERROR;
		}

        if (type == I2S_TXRX_TYPE_RX) {
            ret = tkl_thread_create_in_psram(&i2s_handle[i2s_num], "i2s_handle", 1024 * 2, 7, i2s_handle_task, NULL);
	        if (ret != BK_OK) {
                tkl_queue_free(m_i2s_msg_queue[i2s_num]);
                m_i2s_msg_queue[i2s_num] = NULL;
                bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, type);
                tkl_semaphore_release(m_semaphore[i2s_num]);
                m_semaphore[i2s_num] = NULL;
                bk_i2s_deinit_by_id(i2s_num);
                bk_i2s_multi_driver_deinit();
	        	return OPRT_COM_ERROR;
	        }
        }
    }
    
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief tuya i2s deinit
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_deinit(TUYA_I2S_NUM_E i2s_num)
{
    // --- BEGIN: user implements ---
    void *data = NULL;
    uint32_t val = 0U;
    i2s_txrx_type_t type = I2S_TXRX_TYPE_MAX;
    
    if (m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_TX) {
        type = I2S_TXRX_TYPE_TX;
    } else {
        type = I2S_TXRX_TYPE_RX;
    }
    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    bk_i2s_stop_by_id(i2s_num);

    if (m_i2s_config[i2s_num].i2s_dma_flags) {
        tkl_thread_release(i2s_handle[i2s_num]);
        i2s_handle[i2s_num] = NULL;
        //取出所有数据后释放，不阻塞
        while (tkl_queue_fetch(m_i2s_msg_queue[i2s_num], &data, 0) == OPRT_OK) {
            tkl_system_psram_free(data);
            data = NULL;
        }
        tkl_queue_free(m_i2s_msg_queue[i2s_num]);
        m_i2s_msg_queue[i2s_num] = NULL;
        bk_i2s_chl_deinit_by_id(i2s_num, I2S_CHANNEL_1, type);
        tkl_semaphore_release(m_semaphore[i2s_num]);
        m_semaphore[i2s_num] = NULL;
    }
    bk_i2s_deinit_by_id(i2s_num);
    bk_i2s_multi_driver_deinit();

    os_memset(&m_i2s_config[i2s_num], 0, sizeof(TUYA_I2S_BASE_CFG_T));
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief tuya i2s send
 * 
 * @param[in] i2s_num: i2s port number
 * @param[in] buff: i2s send buffer
 * @param[in] len: i2s send size
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    // --- BEGIN: user implements ---
    void *data = NULL;
    int32_t ret = 0;
    uint32_t write_flag = 0U;
    uint32_t size = 0;
    int really_size = 0U;
    uint32_t frame_size = i2s_calc_frame_size(i2s_num);
    if ((i2s_num >= TUYA_I2S_NUM_MAX) || (len == 0) || ((m_i2s_config[i2s_num].i2s_dma_flags) && (len % frame_size != 0)) || (buff == NULL)) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    if (!(m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_TX)) {
        return OPRT_COM_ERROR;
    }

    if ((m_i2s_config[i2s_num].i2s_dma_flags) && (m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_TX)) {
        // 确保 DMA 已经启动，否则 ringbuffer 永远不会被消费，导致永久阻塞
	    bk_i2s_start_by_id(i2s_num);
        while (1) {
            if (ring_buffer_get_free_size(m_ringbuffer[i2s_num]) >= frame_size) {
                ring_buffer_write(m_ringbuffer[i2s_num], ((uint8_t*)buff) + really_size, frame_size);
                really_size += frame_size;
            } else {
                // 不使用永久等待：如果 DMA 未运行/回调未触发，会导致线程死等
                ret = tkl_semaphore_wait(m_semaphore[i2s_num], TKL_SEM_WAIT_FOREVER);
                if (ret != BK_OK) {
                    // 尝试重新启动一次，避免异常情况下 TX 无法恢复
		            bk_i2s_start_by_id(i2s_num);
                }
            }
            if (really_size >= len) {
                break;
            }
        }
        return really_size;
    } else {
        bk_i2s_get_write_ready_by_id(I2S_CHANNEL_1, &write_flag);
        if (write_flag) {
            BK_RETURN_ON_ERR(bk_i2s_write_data_by_id(i2s_num, buff, (uint32_t)len));
            return len;
        } else {
            return OPRT_COM_ERROR;
        }
    }
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief tuya i2s async recv
 * 
 * @param[in] i2s_num: i2s port number
 * @param[in] buff: i2s receive buffer
 * @param[in] len: i2s receive size, large than DMA_FRAME_SIZE
 *
 * @return return >= 0: number of data read; return < 0: read errror
 */
int32_t tkl_i2s_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    // --- BEGIN: user implements ---
    int32_t ret = 0L;
    int32_t really_size = 0U;
    uint32_t val = 0U;
    void *data = NULL;
    uint32_t read_flag = 0U;
    uint32_t frame_size = i2s_calc_frame_size(i2s_num);
    if ((i2s_num >= TUYA_I2S_NUM_MAX) || (len == 0) || ((m_i2s_config[i2s_num].i2s_dma_flags) && (len % frame_size != 0)) || (buff == NULL)) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    if (!(m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_RX)) {
        return OPRT_COM_ERROR;
    }
    if (m_i2s_config[i2s_num].i2s_dma_flags) {
        // 先非阻塞读取所有可读的数据
        while (really_size < len) {
            ret = tkl_queue_fetch(m_i2s_msg_queue[i2s_num], &data, 0);
            if (ret != OPRT_OK) {
                break;
            }
            if (data) {
                uint32_t copy_size = (len - really_size) < frame_size ? (len - really_size) : frame_size;
                memcpy((uint8_t*)buff + really_size, data, copy_size);
                tkl_system_psram_free(data);
                really_size += copy_size;
            }
        }
        // 如果非阻塞读取后，一个数据都没有读到，则阻塞等待
        if (really_size == 0) {
            // 阻塞读取剩余可读的数据
            if (really_size < len) {
	            bk_i2s_start_by_id(i2s_num);
                ret = tkl_queue_fetch(m_i2s_msg_queue[i2s_num], &data, TKL_SEM_WAIT_FOREVER);
                if (ret != OPRT_OK) {
                    return OPRT_COM_ERROR;
                }
                if (data) {
                    uint32_t copy_size = (len - really_size) < frame_size ? (len - really_size) : frame_size;
                    memcpy((uint8_t*)buff + really_size, data, copy_size);
                    tkl_system_psram_free(data);
                    really_size += copy_size;
                }
            }
        }
        return really_size;
    } else {
        bk_i2s_get_read_ready_by_id(I2S_CHANNEL_1, &read_flag);
        if (read_flag) {
            BK_RETURN_ON_ERR(bk_i2s_read_data_by_id((i2s_gpio_group_id_t)i2s_num, buff, (uint32_t)len));
            return len;
        } else {
            return OPRT_COM_ERROR;
        }
        
    }
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief tuya i2s send stop
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_send_stop(TUYA_I2S_NUM_E i2s_num)
{
    // --- BEGIN: user implements ---
    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    BK_RETURN_ON_ERR(bk_i2s_stop_by_id(i2s_num));
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief tuya i2s recv stop
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_recv_stop(TUYA_I2S_NUM_E i2s_num)
{
    // --- BEGIN: user implements ---
    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    BK_RETURN_ON_ERR(bk_i2s_stop_by_id(i2s_num));
    return OPRT_OK;
    // --- END: user implements ---
}

OPERATE_RET tkl_i2s_set_vol(TUYA_I2S_NUM_E i2s_num, UINT32_T gain)
{
    return OPRT_NOT_SUPPORTED;
}

