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

#include <common/bk_include.h>
#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#if CONFIG_YUV_BUF
#include <driver/yuv_buf.h>
#include <driver/h264.h>
#endif
#include <driver/media_types.h>
#include <driver/psram.h>
#include <os/mem.h>
#include <driver/timer.h>
#include "bk_arm_arch.h"
#include <components/video_transfer.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/video_common_driver.h>
#include "bk_drv_model.h"
#include <gpio_map.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <modules/pm.h>


#define EJPEG_DROP_COUNT              (0)
#define EJPEG_DELAY_HTIMER_CHANNEL     5
#define EJPEG_I2C_DEFAULT_BAUD_RATE    I2C_BAUD_RATE_100KHZ
#define EJPEG_DELAY_HTIMER_VAL         (2)  // 2ms
#define EJPEG_DROP_FRAME               (0)

#define TAG "dvp_camera"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void yuv_buf_struct_dump();
extern void jpeg_struct_dump();
extern bk_err_t bk_dvp_camera_deinit(void);
extern const dvp_sensor_config_t *bk_dvp_camera_enumerate(void);

static uint8_t dvp_video_dma = DMA_ID_MAX;
static const dvp_sensor_config_t *current_sensor = NULL;
static video_config_t *dvp_video_config = NULL;
static uint32_t frame_total_len = 0;

static uint8_t jpeg_drop_image = 0;
static beken_semaphore_t ejpeg_sema = NULL;
static volatile uint8_t jpeg_eof_flag = 0;
static uint8_t *dvp_encode_buffer = NULL;
#if (EJPEG_DROP_FRAME)
static uint8_t vsync_index = 0;
#endif

static void camera_intf_delay_timer_hdl(timer_id_t timer_id)
{
#if CONFIG_GENERAL_DMA
	uint16_t already_len = dvp_video_config->rx_read_len;
	GLOBAL_INT_DECLARATION();
	uint32_t left_len = bk_dma_get_remain_len(dvp_video_dma);
	uint32_t rec_len = dvp_video_config->node_len - left_len;
	uint32_t frame_len = 0;
	if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG)
	{
		frame_len = bk_jpeg_enc_get_frame_size();
		//os_printf("%d %d %d\r\n", left_len, rec_len, frame_len);
		frame_total_len += rec_len - 5;
	}
	else if (dvp_video_config->device->fmt == PIXEL_FMT_H264)
	{
#if CONFIG_YUV_BUF
		frame_len = bk_h264_get_encode_count() * 4;
		frame_total_len += rec_len;
#endif
	}

	LOGD("encode frame len crc error:%d-%d\r\n", frame_len, frame_total_len);
	LOGD("dma_state:%d\r\n", bk_dma_get_enable_status(dvp_video_dma));

	if (jpeg_drop_image == 0)
	{
		if (dvp_video_config && dvp_video_config->node_full_handler && rec_len > 0)
			dvp_video_config->node_full_handler(dvp_video_config->rxbuf + already_len, rec_len, 1, frame_total_len);
	}

	frame_total_len = 0;

	already_len += rec_len;
	if (already_len >= dvp_video_config->rxbuf_len)
		already_len -= dvp_video_config->rxbuf_len;
	GLOBAL_INT_DISABLE();
	dvp_video_config->rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
	// turn off dma, so dma can start from first configure. for easy handler
	bk_dma_stop(dvp_video_dma);
	dvp_video_config->rx_read_len = 0;
	bk_dma_start(dvp_video_dma);
#endif

	if (jpeg_drop_image == 0)
	{
		if (dvp_video_config && dvp_video_config->data_end_handler)
			dvp_video_config->data_end_handler();
	}

	if (jpeg_drop_image != 0)
		jpeg_drop_image--;

#if (!CONFIG_SYSTEM_CTRL)
	bk_timer_disable(EJPEG_DELAY_HTIMER_CHANNEL);
#endif

}

static void camera_intf_start_delay_timer(void)
{
#if (!CONFIG_SYSTEM_CTRL)
	bk_timer_start(EJPEG_DELAY_HTIMER_CHANNEL, EJPEG_DELAY_HTIMER_VAL, camera_intf_delay_timer_hdl);
#else
	camera_intf_delay_timer_hdl(EJPEG_DELAY_HTIMER_CHANNEL);
#endif
}

static void camera_intf_ejpeg_rx_handler(dma_id_t dma_id)
{
	uint16_t already_len = dvp_video_config->rx_read_len;
	uint16_t copy_len = dvp_video_config->node_len;
	frame_total_len += copy_len;
	GLOBAL_INT_DECLARATION();
	if (jpeg_drop_image == 0)
	{
		if (dvp_video_config && dvp_video_config->node_full_handler != NULL)
			dvp_video_config->node_full_handler(dvp_video_config->rxbuf + already_len, copy_len, 0, 0);
	}

	already_len += copy_len;
	if (already_len >= dvp_video_config->rxbuf_len)
		already_len = 0;
	GLOBAL_INT_DISABLE();
	dvp_video_config->rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
}

#if CONFIG_YUV_BUF
static uint8_t ppi_err = 0;
static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
	if (!ppi_err)
	{
		os_printf("perr\r\n");
		ppi_err = true;
	}
}

static void dvp_camera_reset_hardware_modules_handler(void)
{
	bk_yuv_buf_soft_reset();

	bk_dma_stop(dvp_video_dma);

	bk_yuv_buf_stop(H264_MODE);
	bk_yuv_buf_start(H264_MODE);
	bk_h264_soft_reset();

	bk_dma_start(dvp_video_dma);
}

static void yuv_buf_vhandler(yuv_buf_unit_t id, void *param)
{
	if (ppi_err)
	{
		LOGI("reset OK \r\n");
		ppi_err = false;
		dvp_camera_reset_hardware_modules_handler();
	}
}
#endif
bk_err_t camera_intf_dma_config(video_config_t *dvp_trans_config)
{
	dma_config_t dma_config = {0};
	uint32_t encode_fifo_addr;
	int err = 0;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)dvp_video_config->rxbuf;
	dma_config.dst.end_addr = (uint32_t)(dvp_video_config->rxbuf + dvp_video_config->rxbuf_len);

	// dma config
	if (dvp_video_dma >= DMA_ID_MAX)
	{
		if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG)
		{
			bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
			dma_config.src.start_addr = encode_fifo_addr;
			dvp_video_dma = bk_dma_alloc(DMA_DEV_JPEG);
			dma_config.src.dev = DMA_DEV_JPEG;
		}
		else if (dvp_video_config->device->fmt == PIXEL_FMT_H264)
		{
#if CONFIG_YUV_BUF
			bk_h264_get_fifo_addr(&encode_fifo_addr);
			dma_config.src.start_addr = encode_fifo_addr;
			dvp_video_dma = bk_dma_alloc(DMA_DEV_H264);
			dma_config.src.dev = DMA_DEV_H264;
#endif
		}
		if ((dvp_video_dma < DMA_ID_0) || (dvp_video_dma >= DMA_ID_MAX))
		{
			return err;
		}
	}
	LOGI("dvp dma:%d\r\n", dvp_video_dma);
	os_printf("rxbuf addr  %x \r\n", dma_config.dst.start_addr);
	os_printf("rxbuf len %x \r\n", dvp_video_config->rxbuf_len);

	BK_LOG_ON_ERR(bk_dma_init(dvp_video_dma, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_video_dma, dvp_video_config->node_len));
	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_video_dma, NULL, camera_intf_ejpeg_rx_handler));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_video_dma));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dvp_video_dma, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dvp_video_dma, DMA_ATTR_SEC));
#endif
	BK_LOG_ON_ERR(bk_dma_start(dvp_video_dma));

	return BK_OK;
}

static void camera_intf_ejpeg_end_handler(jpeg_unit_t id, void *param)
{
	camera_intf_start_delay_timer();
}

static void camera_intf_vsync_negedge_handler(jpeg_unit_t id, void *param)
{
	if (jpeg_eof_flag)
	{
#if (CONFIG_SIM_I2C)
		current_sensor->power_down();
#endif

#if (CONFIG_YUV_BUF)
		bk_yuv_buf_stop(YUV_MODE);
		bk_yuv_buf_stop(JPEG_MODE);
		bk_yuv_buf_stop(H264_MODE);
#endif

		bk_jpeg_enc_stop(JPEG_MODE);
		jpeg_eof_flag = 0;
		if (ejpeg_sema != NULL)
		{
			rtos_set_semaphore(&ejpeg_sema);
		}
	}

#if (CONFIG_YUV_BUF)
	if (ppi_err)
	{
		LOGI("reset OK \r\n");
		ppi_err = false;
		dvp_camera_reset_hardware_modules_handler();
	}
#endif

#if (EJPEG_DROP_FRAME)
	if (vsync_index == 1)
	{
		vsync_index = 0;
		if (dvp_video_config->frame_drop)
			dvp_video_config->frame_drop(vsync_index);
	}
	else
	{
		vsync_index = 1;
		if (dvp_video_config->frame_drop)
			dvp_video_config->frame_drop(vsync_index);
	}
#endif

}

bk_err_t bk_dvp_camera_init(void *data)
{
	int err = kNoErr;
	jpeg_config_t jpeg_config = {0};
	jpeg_drop_image = EJPEG_DROP_COUNT;

#if (EJPEG_DROP_FRAME)
	vsync_index = 0;
#endif

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_PSRAM, PM_CPU_FRQ_320M);
	//bk_psram_init();
	dvp_video_config = (video_config_t *)data;
	media_ppi_t cam_ppi = (dvp_video_config->device->info.resolution.width << 16) | dvp_video_config->device->info.resolution.height;

	current_sensor = bk_dvp_camera_enumerate();
	os_printf("cur sensor %x \r\n", current_sensor);
	if (current_sensor == NULL)
	{
		LOGE("NOT find camera\r\n");
		return -1;
	}

	err = camera_intf_dma_config(dvp_video_config);
	if (err)
	{
		LOGE("dma alloc failed\r\n");
		return err;
	}

	jpeg_config.x_pixel = dvp_video_config->device->info.resolution.width / 8;
	jpeg_config.y_pixel = dvp_video_config->device->info.resolution.height / 8;
	jpeg_config.vsync = current_sensor->vsync;
	jpeg_config.hsync = current_sensor->hsync;
	jpeg_config.clk = current_sensor->clk;

	LOGI("x_pixel:%d, y_pixel:%d\r\n", dvp_video_config->device->info.resolution.width, dvp_video_config->device->info.resolution.height);
	if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG)
	{
		os_printf("mjpeg\r\n");
		switch (current_sensor->fmt)
		{
			case PIXEL_FMT_YUYV:
				jpeg_config.sensor_fmt = YUV_FORMAT_YUYV;
				break;

			case PIXEL_FMT_UYVY:
				jpeg_config.sensor_fmt = YUV_FORMAT_UYVY;
				break;

			case PIXEL_FMT_YYUV:
				jpeg_config.sensor_fmt = YUV_FORMAT_YYUV;
				break;

			case PIXEL_FMT_UVYY:
				jpeg_config.sensor_fmt = YUV_FORMAT_UVYY;
				break;

			default:
				LOGE("JPEG MODULE not support this sensor input format\r\n");
				err = kParamErr;
				return err;
		}
#ifdef CONFIG_YUV_BUF
		jpeg_config.mode = JPEG_MODE;
		yuv_buf_config_t yuv_mode_config = {0};
		yuv_mode_config.work_mode = JPEG_MODE;
		yuv_mode_config.x_pixel = jpeg_config.x_pixel;
		yuv_mode_config.y_pixel = jpeg_config.y_pixel;
		yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;

		err = bk_yuv_buf_init(&yuv_mode_config);
		if (err != BK_OK) {
			LOGE("yuv_buf jpeg mode init error\n");
		}
#if CONFIG_ENCODE_BUF_DYNAMIC
		dvp_encode_buffer = dvp_camera_yuv_base_addr_init(dvp_video_config->device->info.resolution, JPEG_MODE);
		if (dvp_encode_buffer == NULL) {
			return BK_ERR_NO_MEM;
		}
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#else
		dvp_encode_buffer = bk_psram_frame_buffer_malloc(PSRAM_HEAP_ENCODE, 1024 * 80);
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#endif
#endif
		// config jpeg module
		err = bk_jpeg_enc_init(&jpeg_config);
		if (err != BK_OK)
		{
			LOGE("jpeg init error\n");
			return err;
		}

		bk_jpeg_enc_register_isr(JPEG_EOF, camera_intf_ejpeg_end_handler, NULL);
		bk_jpeg_enc_register_isr(JPEG_VSYNC_NEGEDGE, camera_intf_vsync_negedge_handler, NULL);
	}
	else if (dvp_video_config->device->fmt == PIXEL_FMT_H264)
	{
		os_printf("h264 \r\n");
#ifdef CONFIG_YUV_BUF
		bk_h264_driver_init();
		yuv_buf_config_t yuv_mode_config = {0};
		yuv_mode_config.work_mode = H264_MODE;
		yuv_mode_config.x_pixel = jpeg_config.x_pixel;
		yuv_mode_config.y_pixel = jpeg_config.y_pixel;
		yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;
		err = bk_yuv_buf_init(&yuv_mode_config);
		if (err != BK_OK) {
			LOGE("yuv_buf mode init error\n");
		}
#if CONFIG_ENCODE_BUF_DYNAMIC
		dvp_encode_buffer = dvp_camera_yuv_base_addr_init(dvp_video_config->device->info.resolution, H264_MODE);
		if (dvp_encode_buffer == NULL) {
			return BK_ERR_NO_MEM;
		}
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#else
		dvp_encode_buffer = bk_psram_frame_buffer_malloc(PSRAM_HEAP_ENCODE, 1024 * 80);
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#endif
		//bk_yuv_buf_set_video_module_clk(JPEG_120M_MCLK_20M);
		if (err != BK_OK)
		{
			LOGE("yuv_buf yuv mode init error\n");
			return err;
		}

		bk_h264_init(cam_ppi);

		bk_h264_register_isr(H264_FINAL_OUT, (h264_isr_t)camera_intf_ejpeg_end_handler, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, dvp_camera_sensor_ppi_err_handler, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_FULL, dvp_camera_sensor_ppi_err_handler, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, dvp_camera_sensor_ppi_err_handler, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, dvp_camera_sensor_ppi_err_handler, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, camera_intf_vsync_negedge_handler, NULL);
#endif
	}
	bk_video_set_mclk(MCLK_24M);
	bk_video_encode_start(dvp_video_config->device->mode);
	current_sensor->init();
	current_sensor->set_ppi(cam_ppi);
	current_sensor->set_fps(dvp_video_config->device->info.fps);

	LOGI("%s finish\r\n", __func__);

	return err;
}

bk_err_t bk_dvp_camera_deinit(void)
{
	int ret = kNoErr;

	jpeg_eof_flag = 1;

	ret = rtos_get_semaphore(&ejpeg_sema, 200);
	if (ret != kNoErr)
	{
		LOGE("Not wait jpeg eof!\r\n");
	}

	bk_jpeg_enc_deinit();
#if CONFIG_YUV_BUF
	bk_yuv_buf_deinit();
	bk_h264_driver_deinit();
#endif

	bk_dma_stop(dvp_video_dma);
	bk_dma_deinit(dvp_video_dma);
	if (dvp_video_config->device->fmt == PIXEL_FMT_H264) {
		bk_dma_free(DMA_DEV_H264, dvp_video_dma);
	} else if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG) {
		bk_dma_free(DMA_DEV_JPEG, dvp_video_dma);
	}

	bk_i2c_deinit(CONFIG_DVP_CAMERA_I2C_ID);
	
	if (dvp_encode_buffer) {
		os_free(dvp_encode_buffer);
	}

	bk_video_power_off(GPIO_9, 1);

#if CONFIG_EXTERN_32K
		pm_clk_32k_source_switch(PM_LPO_SRC_X32K);
#endif

	jpeg_eof_flag = 0;
	jpeg_drop_image = 0;
	frame_total_len = 0;
	dvp_video_dma = DMA_ID_MAX;

	rtos_deinit_semaphore(&ejpeg_sema);
	ejpeg_sema = NULL;

	current_sensor = NULL;
	dvp_video_config = NULL;

	LOGI("camera deinit finish\r\n");
	return ret;
}


