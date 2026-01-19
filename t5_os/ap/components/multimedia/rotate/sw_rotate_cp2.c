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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <modules/image_scale.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "sw_rotate.h"

#define TAG "rot_cp2"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define ROTATE_DIAG_DEBUG
#ifdef ROTATE_DIAG_DEBUG
#define ROTATE_FRAME_START() do { } while (0)
#define ROTATE_FRAME_END() do { } while (0)
#else
#define ROTATE_FRAME_START()
#define ROTATE_FRAME_END()
#endif

#define YUV_ROTATE_IN_SRAM_ENABLE (1)
#define MAX_BLOCK_WIDTH     (80)
#define MAX_BLOCK_HEIGHT    (40)
#define DVP_YUV_ROTATE_BLOCK_SIZE (MAX_BLOCK_WIDTH * MAX_BLOCK_HEIGHT * 2)
extern uint8_t *media_dtcm_share_buff;

typedef struct {
	uint8_t task_state : 1;
	beken_semaphore_t sw_rotate_sem;
	beken_queue_t sw_rotate_queue;
	beken_thread_t sw_rotate_thread;
} sw_rotate_config_t;

static sw_rotate_config_t *sw_rotate_config = NULL;

static media_mailbox_msg_t sw_rotate_to_media_major_msg = {0};

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#include "FreeRTOS.h"
#include "task.h"

extern StaticTask_t xSWDecTaskTCB;
extern StackType_t uxSWDecTaskStack[];
#endif

typedef struct
{
	uint8_t *rx_buf;
	uint8_t *tx_buf;
} rot_buf_t;

static rot_buf_t *s_rot_buf = NULL;

bk_err_t software_rotate_task_send_msg(uint32_t type, uint32_t param)
{
	int ret = BK_OK;
	media_msg_t msg;

	if (sw_rotate_config && sw_rotate_config->sw_rotate_queue)
	{
		msg.event = type;
		msg.param = param;

		ret = rtos_push_to_queue(&sw_rotate_config->sw_rotate_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}
	return ret;
}

static void software_rotate_task_deinit(void)
{
	LOGD("%s\r\n", __func__);
	if (sw_rotate_config)
	{
		if (sw_rotate_config->sw_rotate_queue)
		{
			rtos_deinit_queue(&sw_rotate_config->sw_rotate_queue);
			sw_rotate_config->sw_rotate_queue = NULL;
		}
		if(sw_rotate_config->sw_rotate_sem)
		{
			rtos_deinit_semaphore(&sw_rotate_config->sw_rotate_sem);
		}
		sw_rotate_config->sw_rotate_thread = NULL;

		os_free(sw_rotate_config);
		sw_rotate_config = NULL;
	}

	LOGD("%s complete\r\n", __func__);
}

void memcpy_word(uint32_t *dst, uint32_t *src, uint32_t size)
{
	uint32_t i = 0;

	for (i = 0; i < size; i++)
	{
		dst[i] = src[i];
	}
}

bk_err_t yuv_frame_rotate_handle(media_software_rotate_info_t *rot_notify)
{
	int ret = BK_OK;

	frame_buffer_t *src_yuv = rot_notify->src_yuv;
	frame_buffer_t *rotate_yuv = rot_notify->dst_yuv;
	uint32_t start_line = rot_notify->start_line;
	uint32_t end_line = rot_notify->end_line;

	// start rotate
	int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);

	func = NULL;
	switch (rot_notify->rot_angle)
	{
		case ROTATE_90:
			func = yuyv_rotate_degree90_to_yuyv;
			rotate_yuv->width = src_yuv->height;
			rotate_yuv->height = src_yuv->width;
			break;

		case ROTATE_270:
			func = yuyv_rotate_degree270_to_yuyv;
			rotate_yuv->width = src_yuv->height;
			rotate_yuv->height = src_yuv->width;
			break;

		case ROTATE_180:
			func = yuyv_rotate_degree180_to_yuyv;
			rotate_yuv->width = src_yuv->width;
			rotate_yuv->height = src_yuv->height;
			break;

		default:
			ret = BK_FAIL;
			break;
	}

#if YUV_ROTATE_IN_SRAM_ENABLE

		if (media_dtcm_share_buff == NULL)
		{
			if (s_rot_buf == NULL)
			{
				s_rot_buf = (rot_buf_t *)os_malloc(DVP_YUV_ROTATE_BLOCK_SIZE * 2 + sizeof(rot_buf_t));
				if (s_rot_buf == NULL)
				{
					ret = BK_ERR_NO_MEM;
					goto out;
				}

				s_rot_buf->rx_buf = (uint8_t *)s_rot_buf + sizeof(rot_buf_t);
				s_rot_buf->tx_buf = s_rot_buf->rx_buf + DVP_YUV_ROTATE_BLOCK_SIZE;
			}
		}
		else
		{
			if (s_rot_buf == NULL)
			{
				s_rot_buf = (rot_buf_t *)os_malloc(sizeof(rot_buf_t));
				if (s_rot_buf == NULL)
				{
					ret = BK_ERR_NO_MEM;
					goto out;
				}
			}
			s_rot_buf->rx_buf = media_dtcm_share_buff;
			s_rot_buf->tx_buf = s_rot_buf->rx_buf + 5 * 1024;
		}

		int i = 0, j = 0, k = 0;
		int src_width = src_yuv->width, src_height = src_yuv->height;
		int block_width = 40, block_height = 40;
		uint8_t *rx_block = s_rot_buf->rx_buf;
		uint8_t *tx_block = s_rot_buf->tx_buf;
		register uint8_t *cp_ptr = NULL;
		uint8_t *src_frame_temp = src_yuv->frame;
		uint8_t *dst_frame_temp = rotate_yuv->frame;
		for (j = (start_line / block_height); j < (end_line / block_height); j++)
		{
			for (i = 0; i < (src_width / block_width); i++)
			{
				for (k = 0; k < block_height; k++)
				{
					cp_ptr = src_frame_temp + i * block_width * 2 + j * block_height * src_width * 2 + k * src_width * 2;
					memcpy_word((uint32_t *)(rx_block + block_width * 2 * k), (uint32_t *)cp_ptr, block_width * 2 / 4);
				}

				func(rx_block, tx_block, block_width, block_height);

				for (k = 0; k < block_width; k++)
				{
					if (rot_notify->rot_angle == ROTATE_90)
					{
						cp_ptr = dst_frame_temp + (src_height / block_height - j - 1) * block_height * 2 + (i) * block_width * src_height * 2 + k * src_height * 2;
						memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
					}
					else //270
					{
						cp_ptr = dst_frame_temp + (src_width / block_width - 1 - i) * block_width * src_height * 2 + block_height * j * 2 + k * src_height * 2;
						memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
					}
				}
			}
		}

#else
		func(src_yuv->frame, rotate_yuv->frame, src_yuv->width, src_yuv->height);
#endif

out:
	if (ret != BK_OK)
	{
		LOGE("%s, %d\n", __func__, __LINE__);
	}

	return ret;
}

static void software_rotate_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	sw_rotate_config->task_state = true;
	rtos_set_semaphore(&sw_rotate_config->sw_rotate_sem);

	while(1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&sw_rotate_config->sw_rotate_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case EVENT_YUV_ROTATE_START_NOTIFY:
				{
					media_mailbox_msg_t *media_msg = (media_mailbox_msg_t *)msg.param;
					media_software_rotate_info_t *sw_rotate_info = NULL;
					if (media_msg == NULL)
					{
						LOGD("%s %d param error\r\n", __func__, __LINE__);
                        sw_rotate_to_media_major_msg.event = EVENT_YUV_ROTATE_COMPLETE_NOTIFY;
                        sw_rotate_to_media_major_msg.result = BK_FAIL;
                        msg_send_notify_to_media_minor_mailbox(&sw_rotate_to_media_major_msg, MAJOR_MODULE);
						break;
					}
					sw_rotate_info = (media_software_rotate_info_t *)media_msg->param;
					if (sw_rotate_info != NULL)
					{
						ROTATE_FRAME_START();
						yuv_frame_rotate_handle(sw_rotate_info);
						ROTATE_FRAME_END();
						sw_rotate_to_media_major_msg.param = (uint32_t)sw_rotate_info->cb;
					}
					else
					{
						ret = BK_FAIL;
					}

					sw_rotate_to_media_major_msg.event = EVENT_YUV_ROTATE_COMPLETE_NOTIFY;
					sw_rotate_to_media_major_msg.result = ret;
					msg_send_notify_to_media_minor_mailbox(&sw_rotate_to_media_major_msg, MAJOR_MODULE);
					break;
				}

				case EVENT_YUV_ROTATE_STOP_NOTIFY:
					goto exit;

				default:
					break;
			}
		}
	}

exit:
	LOGD("%s, exit\r\n", __func__);
	rtos_set_semaphore(&sw_rotate_config->sw_rotate_sem);
	rtos_delete_thread(NULL);
}

bool check_software_rotate_task_is_open(void)
{
	if (sw_rotate_config == NULL)
	{
		return false;
	}
	else
	{
		return sw_rotate_config->task_state;
	}
}

bk_err_t software_rotate_task_open(void)
{
	int ret = BK_OK;
	LOGD("%s\r\n", __func__);

	if (sw_rotate_config != NULL && sw_rotate_config->task_state)
	{
		LOGE("%s have been opened!\r\n", __func__);
		return ret;
	}

	sw_rotate_config = (sw_rotate_config_t *)os_malloc(sizeof(sw_rotate_config_t));
	if (sw_rotate_config == NULL)
	{
		LOGE("%s, malloc sw_rotate_config failed\r\n", __func__);
		return BK_FAIL;
	}

	os_memset(sw_rotate_config, 0, sizeof(sw_rotate_config_t));

	ret = rtos_init_semaphore(&sw_rotate_config->sw_rotate_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s, init sw_rotate_config->sw_rotate_sem failed\r\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&sw_rotate_config->sw_rotate_queue,
							"sw_rotate_que",
							sizeof(media_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init sw_rotate_que failed\r\n", __func__);
		goto error;
	}

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
	sw_rotate_config->sw_rotate_thread = xTaskCreateStatic( (TaskFunction_t)software_rotate_main,
										 "sw_rot_task_cp2",
										 512,
										 ( void * ) NULL,
										 9 - BEKEN_DEFAULT_WORKER_PRIORITY,
										 uxSWDecTaskStack,
										 &xSWDecTaskTCB );
#else
	ret = rtos_create_thread(&sw_rotate_config->sw_rotate_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"sw_rot_task_cp2",
							(beken_thread_function_t)software_rotate_main,
							1024 * 2,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_task failed\r\n", __func__);
		goto error;
	}
#endif

	rtos_get_semaphore(&sw_rotate_config->sw_rotate_sem, BEKEN_NEVER_TIMEOUT);
	LOGD("%s complete\r\n", __func__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	software_rotate_task_deinit();

	return ret;
}

bk_err_t software_rotate_task_close(void)
{
	LOGD("%s  %d\n", __func__, __LINE__);

	if (sw_rotate_config == NULL || !sw_rotate_config->task_state)
	{
		return BK_FAIL;
	}

	sw_rotate_config->task_state = false;

	software_rotate_task_send_msg(EVENT_YUV_ROTATE_STOP_NOTIFY, 0);

	rtos_get_semaphore(&sw_rotate_config->sw_rotate_sem, BEKEN_NEVER_TIMEOUT);

	software_rotate_task_deinit();

	LOGD("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}


