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
#include <driver/jpeg_enc.h>
#if CONFIG_YUV_BUF
#include <driver/yuv_buf.h>
#include <driver/h264.h>
#endif
#include <driver/media_types.h>
#include <driver/psram.h>
#include <os/mem.h>
#include <driver/timer.h>
#include <components/video_transfer.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/video_common_driver.h>
#include "bk_drv_model.h"
#include <modules/pm.h>
#include "media_mailbox_list_util.h"
#include "media_evt.h"


#define EJPEG_DROP_COUNT              (0)
#define EJPEG_DELAY_HTIMER_CHANNEL     5
#define EJPEG_I2C_DEFAULT_BAUD_RATE    I2C_BAUD_RATE_100KHZ
#define EJPEG_DELAY_HTIMER_VAL         (2)  // 2ms
#define EJPEG_DROP_FRAME               (0)

#define TAG "dvp_major"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern bk_err_t bk_dvp_camera_deinit(void);
extern const dvp_sensor_config_t *bk_dvp_camera_enumerate(void);
static bk_err_t media_tranfer_task_init(void);

#define MSG_EOF 0
#define MSG_DMA_RESTART 1
typedef struct
{
	uint8_t type;
	uint32_t data;
} media_trs_task_msg_t;

static const dvp_sensor_config_t *current_sensor = NULL;
static media_camera_device_t *dvp_device = NULL;

static uint8_t jpeg_drop_image = 0;
static beken_semaphore_t ejpeg_sema = NULL;
static volatile uint8_t jpeg_eof_flag = 0;
media_mailbox_msg_t *media_msg = NULL;
beken_thread_t media_trans_task = NULL;
beken_queue_t media_trans_msg_que = NULL;
static uint8_t *dvp_encode_buffer = NULL;
#if (EJPEG_DROP_FRAME)
static uint8_t vsync_index = 0;
#endif

static bk_err_t media_trans_major_task_send_msg(uint8_t type, uint32_t data)
{
	bk_err_t ret;
	media_trs_task_msg_t msg;

	if (media_trans_msg_que)
	{
		msg.type = type;
		msg.data = data;

		ret = rtos_push_to_queue(&media_trans_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			LOGE("transfer_major_msg_que failed, type:%d\r\n", type);
			return kNoResourcesErr;
		}

		return ret;
	}
	return kGeneralErr;
}

static void camera_intf_delay_timer_hdl(timer_id_t timer_id)
{
#if CONFIG_GENERAL_DMA
	uint32_t frame_len = 0;
	if (dvp_device->fmt == PIXEL_FMT_JPEG)
	{
		frame_len = bk_jpeg_enc_get_frame_size();
	}
	else if (dvp_device->fmt == PIXEL_FMT_H264)
	{
#if CONFIG_YUV_BUF
		frame_len = bk_h264_get_encode_count() * 4;
#endif
	}

	LOGD("encode frame len %d\r\n", frame_len);

	if (jpeg_drop_image == 0) {
		media_trans_major_task_send_msg(MSG_EOF, 0);
	}
#endif

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

#if CONFIG_YUV_BUF
uint8_t ppi_err = 0;
static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
	if (!ppi_err)
	{
		LOGI("perr\r\n");
		ppi_err = true;
	}
}

static void dvp_camera_reset_hardware_modules_handler(void)
{
	bk_yuv_buf_soft_reset();

	bk_yuv_buf_stop(H264_MODE);
	bk_yuv_buf_start(H264_MODE);
	bk_h264_soft_reset();

	//to do mbox dma restart
	media_trans_major_task_send_msg(MSG_DMA_RESTART, 0);
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

#if CONFIG_YUV_BUF
	if (ppi_err)
	{
		LOGI("reset OK \r\n");
		ppi_err = false;
		dvp_camera_reset_hardware_modules_handler();
	}
#endif

#if (EJPEG_DROP_FRAME)
	// if (vsync_index == 1)
	// {
	// 	vsync_index = 0;
	// 	if (dvp_video_config->frame_drop)
	// 		dvp_video_config->frame_drop(vsync_index);
	// }
	// else
	// {
	// 	vsync_index = 1;
	// 	if (dvp_video_config->frame_drop)
	// 		dvp_video_config->frame_drop(vsync_index);
	// }
	// to do drop frame
#endif

}

bk_err_t bk_dvp_camera_init(media_mailbox_msg_t *msg)
{
	int err = kNoErr;
	jpeg_config_t jpeg_config = {0};
	jpeg_drop_image = EJPEG_DROP_COUNT;

#if (EJPEG_DROP_FRAME)
	vsync_index = 0;
#endif
	dvp_device = (media_camera_device_t *)os_malloc(sizeof(media_camera_device_t));
	os_memcpy(dvp_device, (uint32_t *)msg->param, sizeof(media_camera_device_t));
	media_tranfer_task_init();

	media_ppi_t cam_ppi = (dvp_device->info.resolution.width << 16) | dvp_device->info.resolution.height;

	current_sensor = bk_dvp_camera_enumerate();
	LOGI("cur sensor %x \r\n", current_sensor);
	if (current_sensor == NULL)
	{
		LOGE("NOT find camera\r\n");
		return -1;
	}

	jpeg_config.x_pixel = dvp_device->info.resolution.width / 8;
	jpeg_config.y_pixel = dvp_device->info.resolution.height / 8;
	jpeg_config.vsync = current_sensor->vsync;
	jpeg_config.hsync = current_sensor->hsync;
	jpeg_config.clk = current_sensor->clk;

	LOGI("x_pixel:%d, y_pixel:%d\r\n", dvp_device->info.resolution.width, dvp_device->info.resolution.height);
	if (dvp_device->fmt == PIXEL_FMT_JPEG)
	{
		LOGI("mjpeg\r\n");
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
		dvp_encode_buffer = dvp_camera_yuv_base_addr_init(dvp_device->info.resolution, JPEG_MODE);
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
	else if (dvp_device->fmt == PIXEL_FMT_H264)
	{
		LOGI("h264 \r\n");
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
		dvp_encode_buffer = dvp_camera_yuv_base_addr_init(dvp_device->info.resolution, H264_MODE);
		if (dvp_encode_buffer == NULL) {
			return BK_ERR_NO_MEM;
		}
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#else
		dvp_camera_encode = bk_psram_frame_buffer_malloc(PSRAM_HEAP_ENCODE, 1024 * 80);
		bk_yuv_buf_set_em_base_addr((uint32_t)dvp_encode_buffer);
#endif
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
	bk_video_encode_start(dvp_device->mode);
	current_sensor->init();
	current_sensor->set_ppi(cam_ppi);
	current_sensor->set_fps(dvp_device->info.fps);

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

	if (dvp_encode_buffer) {
		os_free(dvp_encode_buffer);
	}

	bk_video_power_off(9, 1);

#if CONFIG_EXTERN_32K
		pm_clk_32k_source_switch(PM_LPO_SRC_X32K);
#endif

	jpeg_eof_flag = 0;
	jpeg_drop_image = 0;

	rtos_deinit_semaphore(&ejpeg_sema);
	ejpeg_sema = NULL;

	current_sensor = NULL;

	LOGI("camera deinit finish\r\n");
	return ret;
}

static void media_trans_task_entry(beken_thread_arg_t data)
{
	while (1)
	{
		media_trs_task_msg_t msg;
		int ret = rtos_pop_from_queue(&media_trans_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case MSG_EOF:
					media_msg->event = EVENT_DVP_EOF_NOTIFY;
					msg_send_notify_to_media_major_mailbox(media_msg, APP_MODULE);
					break;

				case MSG_DMA_RESTART:
					media_msg->event = EVENT_DMA_RESTART_NOTIFY;
					msg_send_notify_to_media_major_mailbox(media_msg, APP_MODULE);
					break;

				default:
					break;
			}
		}
	}

// exit:

// 	LOGI("media_trans_major_task task exit\n");

// 	rtos_deinit_queue(&media_trans_msg_que);
// 	media_trans_msg_que = NULL;

// 	media_trans_task = NULL;
// 	rtos_delete_thread(NULL);
}

static bk_err_t media_tranfer_task_init(void)
{
	bk_err_t ret = BK_FAIL;

	if (media_msg == NULL)
	{
		media_msg = (media_mailbox_msg_t *)os_malloc(sizeof(media_mailbox_msg_t));
		if (media_msg == NULL)
		{
			LOGE("%s media_msg malloc failed\n", __func__);
			goto error;
		}
	}

	if ((!media_trans_task) && (!media_trans_msg_que))
	{
		ret = rtos_init_queue(&media_trans_msg_que,
								"media_trans_msg_que",
								sizeof(media_trs_task_msg_t),
								10);
		if (kNoErr != ret)
		{
			LOGE("media_trans_msg_que init failed\n");
			ret = kNoMemoryErr;
			goto error;
		}

		ret = rtos_create_thread(&media_trans_task,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"media_tranfer_task",
								(beken_thread_function_t)media_trans_task_entry,
								1024,
								NULL);

		if (BK_OK != ret)
		{
			LOGE("media_tranfer_task init failed\n");
			ret = kNoMemoryErr;
			goto error;
		}
	}

	return ret;

error:

	if (media_msg)
	{
		os_free(media_msg);
		media_msg = NULL;
	}

	if (media_trans_msg_que)
	{
		rtos_deinit_queue(&media_trans_msg_que);
		media_trans_msg_que = NULL;
	}

	if (media_trans_task)
	{
		media_trans_task = NULL;
	}

	return ret;
}
