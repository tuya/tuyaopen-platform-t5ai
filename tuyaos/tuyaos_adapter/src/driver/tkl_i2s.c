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
#include "tal_log.h"
#include "tkl_i2s.h"
#include "i2s_hal.h"
#include "tkl_audio.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"
#include "bk_general_dma.h"

#include "i2s_hw.h"
#include <driver/i2s_types.h>
// --- END: user defines and implements ---

/***********************************************************
************************macro define************************
***********************************************************/
#define DMA_FRAME_SIZE    (16000 * 2 * 32 / 8 / 50)
#define BUFFER_NUM        (2)
#define DMA_MAGIC         (0xf0f0f0f0)
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct i2s_cb_config {
    i2s_data_handle_cb tx_cb;   //发送回调，dma发送完成时调用
    i2s_data_handle_cb rx_cb;   //接收回调，dma接收完成时调用
    dma_id_t dma_id;
    UINT32_T * dma_curr_buf;
}i2s_cb_config_t;

/***********************************************************
********************function declaration********************
***********************************************************/
static int ch0_tx_data_handle_cb(uint32_t size);
static int ch0_rx_data_handle_cb(uint32_t size);
static int ch1_tx_data_handle_cb(uint32_t size);
static int ch1_rx_data_handle_cb(uint32_t size);
static int ch2_tx_data_handle_cb(uint32_t size);
static int ch2_rx_data_handle_cb(uint32_t size);
/***********************************************************
***********************variable define**********************
***********************************************************/
i2s_cb_config_t m_i2s_cfg[TUYA_I2S_NUM_MAX] = {
    {ch0_tx_data_handle_cb,ch0_rx_data_handle_cb,DMA_ID_MAX},
    {ch1_tx_data_handle_cb,ch1_rx_data_handle_cb,DMA_ID_MAX},
    {ch2_tx_data_handle_cb,ch2_rx_data_handle_cb,DMA_ID_MAX}
};
static TUYA_I2S_BASE_CFG_T m_i2s_config[TUYA_I2S_NUM_MAX] = {0};
static RingBufferContext *m_ringbuffer[TUYA_I2S_NUM_MAX] = {NULL};
static TKL_SEM_HANDLE m_semaphore[TUYA_I2S_NUM_MAX] = {0};
/***********************************************************
***********************function define**********************
************************************************************/

/**
 * @brief channel 0 write calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch0_tx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_0]);
    if (ret != BK_OK) {
        bk_printf("dma tx sem fail %d\r\n", ret);
        return ret;
    }

	return size;
}

/**
 * @brief channel 0 read calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch0_rx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_0]);
    if (ret != BK_OK) {
        bk_printf("dma rx sem fail %d\r\n", ret);
        return ret;
    }
	return size;
}

/**
 * @brief channel 1 write calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch1_tx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_1]);
    if (ret != BK_OK) {
        bk_printf("dma tx sem fail %d\r\n", ret);
        return ret;
    }

	return size;
}

/**
 * @brief channel 1 read calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch1_rx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_1]);
    if (ret != BK_OK) {
        bk_printf("dma rx sem fail %d\r\n", ret);
        return ret;
    }
	return size;
}

/**
 * @brief channel 2 write calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch2_tx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_2]);
    if (ret != BK_OK) {
        bk_printf("dma tx sem fail %d\r\n", ret);
        return ret;
    }
	return size;
}

/**
 * @brief channel 2 read calback
 *
 * @param[in] size: size of buffer one frame
 *
 * @return err or size
 */
static int ch2_rx_data_handle_cb(uint32_t size)
{
    int ret = 0;
    ret = tkl_semaphore_post(m_semaphore[TUYA_I2S_NUM_2]);
    if (ret != BK_OK) {
        bk_printf("dma rx sem fail %d\r\n", ret);
        return ret;
    }
	return size;
}

/**
 * @brief i2s init
 *
 * @param[in] i2c_pin: i2s pin number
 * @param[in] cfg: i2s configure
 *
 * @return OPRT_OK on success, others on error
 */
OPERATE_RET tkl_i2s_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *cfg)
{
    // --- BEGIN: user implements ---
    OPERATE_RET ret = 0;
    i2s_config_t i2s_config = DEFAULT_I2S_CONFIG();
    i2s_data_handle_cb i2s_cb = NULL;
    i2s_txrx_type_t type = I2S_TXRX_TYPE_MAX;

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
            i2s_config.pcm_chl_num = 2;
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
            i2s_config.pcm_chl_num = 2;
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

    bk_i2s_driver_init();
    i2s_config.data_length = cfg->bits_per_sample;
    bk_i2s_init((i2s_gpio_group_id_t)i2s_num, &i2s_config);
    if (cfg->i2s_dma_flags) {
        if (cfg->mode & TUYA_I2S_MODE_TX) {
            type = I2S_TXRX_TYPE_TX;
            i2s_cb = m_i2s_cfg[i2s_num].tx_cb;
        } else {
            type = I2S_TXRX_TYPE_RX;
            i2s_cb = m_i2s_cfg[i2s_num].rx_cb;
        }
        ret = tkl_semaphore_create_init(&m_semaphore[i2s_num], 0, 1);
        if (ret != BK_OK) {
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            bk_printf("i2s sem create failed %d\n", ret);
			return OPRT_COM_ERROR;
		}
        ret = bk_i2s_chl_init(I2S_CHANNEL_1, type, DMA_FRAME_SIZE * 2, i2s_cb, &m_ringbuffer[i2s_num]);
		if (ret != BK_OK) {
            bk_i2s_deinit();
            bk_i2s_driver_deinit();
            tkl_semaphore_release(m_semaphore[i2s_num]);
			return OPRT_COM_ERROR;
		}
    }
	bk_i2s_start();
    memcpy(&m_i2s_config[i2s_num], cfg, sizeof(TUYA_I2S_BASE_CFG_T));
    
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
    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }

    if (m_i2s_config[i2s_num].i2s_dma_flags) {
        bk_i2s_chl_deinit(I2S_CHANNEL_1, (m_i2s_config[i2s_num].mode & (TUYA_I2S_MODE_TX | TUYA_I2S_MODE_RX)));
    }
    bk_i2s_enable(I2S_DISABLE);
    bk_i2s_deinit();
    bk_i2s_driver_deinit();

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
OPERATE_RET tkl_i2s_send(TUYA_I2S_NUM_E i2s_num, VOID_T *buff, UINT32_T len)
{
    // --- BEGIN: user implements ---
    INT32_T ret = 0;
    uint32_t write_flag = 0U;
    if ((i2s_num >= TUYA_I2S_NUM_MAX) || ((m_i2s_config[i2s_num].i2s_dma_flags) && (len > DMA_FRAME_SIZE)) || (buff == NULL)) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    if ((m_i2s_config[i2s_num].i2s_dma_flags) && (m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_TX)) {
        ret = ring_buffer_write(m_ringbuffer[i2s_num], buff, DMA_FRAME_SIZE);
        if (ret != OPRT_OK)
        {
            return OPRT_SEND_ERR;
        }
        ret = tkl_semaphore_wait(m_semaphore[i2s_num], TKL_SEM_WAIT_FOREVER);
        if (ret != OPRT_OK)
        {
            bk_printf("tkl_semaphore_wait %d is invalid\n", ret);
            return ret;
        }
    } else {
        bk_i2s_get_write_ready(&write_flag);
        if (write_flag) {
            BK_RETURN_ON_ERR(bk_i2s_write_data(i2s_num, buff, (UINT32_T)len));
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
INT_T tkl_i2s_recv(TUYA_I2S_NUM_E i2s_num, VOID_T *buff, UINT32_T len)
{
    // --- BEGIN: user implements ---
    INT_T ret = 0L;
    uint32_t read_flag = 0U;
    if ((i2s_num >= TUYA_I2S_NUM_MAX) || ((m_i2s_config[i2s_num].i2s_dma_flags) && (len < DMA_FRAME_SIZE)) || (buff == NULL)) {
        bk_printf("i2s port %d is invalid\n", i2s_num);
        return OPRT_INVALID_PARM;
    }
    if ((m_i2s_config[i2s_num].i2s_dma_flags) && (m_i2s_config[i2s_num].mode & TUYA_I2S_MODE_RX)) {
        ret = tkl_semaphore_wait(m_semaphore[i2s_num], TKL_SEM_WAIT_FOREVER);
        if (ret != OPRT_OK)
        {
            bk_printf("tkl_semaphore_wait %d is invalid\n", ret);
            return ret;
        }
        ring_buffer_read(m_ringbuffer[i2s_num], buff, DMA_FRAME_SIZE);
        ret = DMA_FRAME_SIZE;
    } else {
        bk_i2s_get_read_ready(&read_flag);
        if (read_flag) {
            BK_RETURN_ON_ERR(bk_i2s_read_data(buff, (UINT32_T)len));
        }
    }
    return ret;
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
    BK_RETURN_ON_ERR(bk_i2s_enable(I2S_DISABLE));
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
    BK_RETURN_ON_ERR(bk_i2s_enable(I2S_DISABLE));
    return OPRT_OK;
    // --- END: user implements ---
}
