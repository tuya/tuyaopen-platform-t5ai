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

#include <driver/jpeg_dec.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "yuv_encode.h"

#include "mux_pipeline.h"

#define TAG "jpeg_get"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
	uint8_t task_state : 1;
	uint8_t module_decode_status : 1;
	uint8_t module_decode_cp1_status : 1;
	frame_buffer_t *jpeg_frame;
	beken_semaphore_t jdec_sem;
	beken_queue_t jdec_queue;
	beken_thread_t jdec_thread;
} jpeg_get_config_t;

static void jpeg_decode_line_complete_handler(jpeg_dec_res_t *result);

static jpeg_get_config_t *jpeg_get_config = NULL;

bk_err_t jpeg_get_task_send_msg(uint8_t type, uint32_t param)
{
	int ret = BK_OK;
	media_msg_t msg;

	if (jpeg_get_config && jpeg_get_config->jdec_queue)
	{
		if (jpeg_get_config->module_decode_status == 1 && param == MODULE_DECODER)
		{
			return BK_OK;
		}
		else if (jpeg_get_config->module_decode_cp1_status == 1 && param == MODULE_DECODER_CP1)
		{
			return BK_OK;
		}
		msg.event = type;
		msg.param = param;
		if (msg.param == MODULE_DECODER)
		{
			jpeg_get_config->module_decode_status = 1;
		}
		else if (msg.param == MODULE_DECODER_CP1)
		{
			jpeg_get_config->module_decode_cp1_status = 1;
		}
		ret = rtos_push_to_queue(&jpeg_get_config->jdec_queue, &msg, BEKEN_NO_WAIT);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}

	return ret;
}

static void jpeg_get_start_handle(frame_module_t frame_module)
{
	// step 1: read a jpeg frame
	while (jpeg_get_config->task_state)
	{
		jpeg_get_config->jpeg_frame = frame_buffer_fb_read(frame_module);
		if (jpeg_get_config->jpeg_frame)
		{
			jpeg_decode_task_send_more_msg(JPEGDEC_START, (uint32_t)jpeg_get_config->jpeg_frame, frame_module);
			if (frame_module == MODULE_DECODER)
			{
				jpeg_get_config->module_decode_status = 0;
			}
			else if (frame_module == MODULE_DECODER_CP1)
			{
				jpeg_get_config->module_decode_cp1_status = 0;
			}
			break;
		}
	}
}

static void jpeg_get_task_deinit(void)
{
	if (jpeg_get_config)
	{
		frame_buffer_fb_deregister(MODULE_DECODER, FB_INDEX_JPEG);
		if (jpeg_get_config->jdec_queue)
		{
			rtos_deinit_queue(&jpeg_get_config->jdec_queue);
			jpeg_get_config->jdec_queue = NULL;
		}
		if(jpeg_get_config->jdec_sem)
		{
			rtos_deinit_semaphore(&jpeg_get_config->jdec_sem);
		}
		jpeg_get_config->jdec_thread = NULL;


		os_free(jpeg_get_config);
		jpeg_get_config = NULL;
	}
}

static void jpeg_get_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	jpeg_get_config->task_state = true;

	rtos_set_semaphore(&jpeg_get_config->jdec_sem);

	while(1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&jpeg_get_config->jdec_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case JPEGDEC_START:
					if (jpeg_get_config->task_state)
					{
						jpeg_get_start_handle(msg.param);
					}
					else
					{
						goto exit;
					}
					break;

				case JPEGDEC_STOP:
				{
					goto exit;
				}
				break;

				default:
					break;
			}
		}
	}

exit:
	rtos_set_semaphore(&jpeg_get_config->jdec_sem);
	LOGI("%s, exit\r\n", __func__);
	rtos_delete_thread(NULL);
}

bool check_jpeg_get_task_is_open(void)
{
	if (jpeg_get_config == NULL)
	{
		return false;
	}
	else
	{
		return jpeg_get_config->task_state;
	}
}

bk_err_t jpeg_get_task_open(void)
{
	int ret = BK_OK;
	LOGI("%s(%d)\n", __func__, __LINE__);

	if (jpeg_get_config != NULL && jpeg_get_config->task_state)
	{
		LOGE("%s have been opened!\r\n", __func__);
		return ret;
	}

	jpeg_get_config = (jpeg_get_config_t *)os_malloc(sizeof(jpeg_get_config_t));
	if (jpeg_get_config == NULL)
	{
		LOGE("%s, malloc jpeg_get_config failed\r\n", __func__);
		return BK_FAIL;
	}

	os_memset(jpeg_get_config, 0, sizeof(jpeg_get_config_t));

	ret = rtos_init_semaphore(&jpeg_get_config->jdec_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s, init jpeg_get_config->jdec_sem failed\r\n", __func__);
		goto error;
	}
	// step 5: init jdec_task
	frame_buffer_fb_register(MODULE_DECODER, FB_INDEX_JPEG);

	ret = rtos_init_queue(&jpeg_get_config->jdec_queue,
							"jpeg_get_que",
							sizeof(media_msg_t),
							3);

	if (ret != BK_OK)
	{
		LOGE("%s, init jpeg_get_que failed\r\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&jpeg_get_config->jdec_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY - 1,
							"jpeg_get_task",
							(beken_thread_function_t)jpeg_get_main,
							1024,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, init jpeg_get_task failed\r\n", __func__);
		goto error;
	}

	rtos_get_semaphore(&jpeg_get_config->jdec_sem, BEKEN_NEVER_TIMEOUT);
	LOGI("%s(%d) complete\n", __func__, __LINE__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	jpeg_get_task_deinit();

	return ret;
}

bk_err_t jpeg_get_task_close()
{
	LOGI("%s(%d)\n", __func__, __LINE__);

	if (jpeg_get_config == NULL || !jpeg_get_config->task_state)
	{
		return BK_FAIL;
	}

	jpeg_get_config->task_state = false;

	frame_buffer_fb_deregister(MODULE_DECODER, FB_INDEX_JPEG);
	frame_buffer_fb_deregister(MODULE_DECODER_CP1, FB_INDEX_JPEG);

	jpeg_get_task_send_msg(JPEGDEC_STOP, 0);
	rtos_get_semaphore(&jpeg_get_config->jdec_sem, BEKEN_NEVER_TIMEOUT);

	jpeg_get_task_deinit();

	LOGI("%s(%d) complete\n", __func__, __LINE__);

	return BK_OK;
}

