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
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct {
	uint8_t task_state : 1;
	uint8_t module_decode_status : 1;
	frame_list_node_t *stream;
	frame_buffer_t *jpeg_frame;
	beken_semaphore_t jdec_sem;
	beken_queue_t jdec_queue;
	beken_thread_t jdec_thread;
	beken_mutex_t jdec_lock;
} jpeg_get_config_t;

static jpeg_get_config_t *jpeg_get_config = NULL;

bk_err_t jpeg_get_task_send_msg(uint8_t type, uint32_t param)
{
    bk_err_t ret = BK_FAIL;

    // Check if configuration and queue are valid
    if (jpeg_get_config && jpeg_get_config->jdec_queue)
    {
        // Handle JPEGDEC_START message type
        if (type == JPEGDEC_START)
        {
            // Lock to ensure thread safety when checking/updating decode status
            rtos_lock_mutex(&jpeg_get_config->jdec_lock);
            if (jpeg_get_config->module_decode_status)
            {
                rtos_unlock_mutex(&jpeg_get_config->jdec_lock);
                return ret;  // Already decoding, return failure
            }
            jpeg_get_config->module_decode_status = true;
            rtos_unlock_mutex(&jpeg_get_config->jdec_lock);
        }

        // Prepare and send message
        media_msg_t msg = { .event = type, .param = param };
        ret = rtos_push_to_queue(&jpeg_get_config->jdec_queue, &msg, BEKEN_NO_WAIT);

        // Handle message send failure
        if (ret != BK_OK)
        {
            if (type == JPEGDEC_START)
            {
                // Rollback decode status if start message failed
                rtos_lock_mutex(&jpeg_get_config->jdec_lock);
                jpeg_get_config->module_decode_status = false;
                rtos_unlock_mutex(&jpeg_get_config->jdec_lock);
            }
            LOGE("%s %d, push failed\n", __func__, __LINE__);
        }
    }
    else
    {
        LOGE("%s, %d failed...\n", __func__, __LINE__);
    }

    return ret;
}

static void jpeg_get_start_handle(void)
{
	frame_list_node_t *stream = NULL;

	// Read JPEG frame
	while (jpeg_get_config->task_state)
	{
		stream = frame_buffer_list_get_main_stream();

		if (stream == NULL)
		{
			LOGW("%s, stream null\n", __func__);
			rtos_delay_milliseconds(100); // Reduce delay time
			continue;
		}

		// Handle stream switch
		if (jpeg_get_config->stream == NULL)
		{
			LOGD("%s, main_stream:%p %d\n", __func__, stream, stream->camera_id);
			jpeg_get_config->stream = stream;
			jpeg_decode_task_send_msg(JPEGDEC_STREAM, (uint32_t)stream);
			frame_buffer_fb_register(jpeg_get_config->stream, MODULE_DECODER);
		}
		else if (stream != jpeg_get_config->stream)
		{
			frame_buffer_fb_deregister(jpeg_get_config->stream, MODULE_DECODER);
			jpeg_get_config->stream = stream;
			jpeg_decode_task_send_msg(JPEGDEC_STREAM, (uint32_t)stream);
			frame_buffer_fb_register(jpeg_get_config->stream, MODULE_DECODER);
		}

#if CONFIG_MEDIA_PSRAM_SIZE_4M
		// Memory test code
		frame_buffer_t *decode_frame = frame_buffer_display_malloc(864 * 480 * 2);
		if(decode_frame != NULL)
		{
			frame_buffer_display_free(decode_frame);
		}
		else
		{
			rtos_lock_mutex(&jpeg_get_config->jdec_lock);
			if (jpeg_get_config->module_decode_status)
			{
				jpeg_get_config->module_decode_status = false;
			}
			rtos_unlock_mutex(&jpeg_get_config->jdec_lock);
			break;
		}
#endif

		// Read JPEG frame
		jpeg_get_config->jpeg_frame = frame_buffer_fb_read(jpeg_get_config->stream, MODULE_DECODER, 50);
		if (jpeg_get_config->jpeg_frame)
		{
			// Update decode status
			rtos_lock_mutex(&jpeg_get_config->jdec_lock);
			if (jpeg_get_config->module_decode_status)
			{
				jpeg_get_config->module_decode_status = false;
			}
			rtos_unlock_mutex(&jpeg_get_config->jdec_lock);

			// Send decode start message
			if (jpeg_decode_task_send_msg(JPEGDEC_START, (uint32_t)jpeg_get_config->jpeg_frame) != BK_OK)
			{
				frame_buffer_fb_read_free(jpeg_get_config->stream, jpeg_get_config->jpeg_frame, MODULE_DECODER);
			}
			break;
		}
		else
		{
			LOGV("%s, %d read frame timeout\n", __func__, __LINE__);
		}
	}
}

static void jpeg_get_task_deinit(void)
{
	if (jpeg_get_config)
	{
		frame_buffer_fb_deregister(jpeg_get_config->stream, MODULE_DECODER);
		if (jpeg_get_config->jdec_queue)
		{
			rtos_deinit_queue(&jpeg_get_config->jdec_queue);
			jpeg_get_config->jdec_queue = NULL;
		}
		if(jpeg_get_config->jdec_sem)
		{
			rtos_deinit_semaphore(&jpeg_get_config->jdec_sem);
		}
		if(jpeg_get_config->jdec_lock)
		{
			rtos_deinit_mutex(&jpeg_get_config->jdec_lock);
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
						jpeg_get_start_handle();
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
	LOGD("%s, exit\r\n", __func__);
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
	LOGV("%s(%d)\n", __func__, __LINE__);

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
	ret = rtos_init_mutex(&jpeg_get_config->jdec_lock);
	if (ret != BK_OK)
	{
		LOGE("%s, init jpeg_get_config->jdec_lock failed\r\n", __func__);
		goto error;
	}

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
	LOGD("%s(%d) complete\n", __func__, __LINE__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	jpeg_get_task_deinit();

	return ret;
}

bk_err_t jpeg_get_task_close()
{
	LOGV("%s(%d)\n", __func__, __LINE__);

	if (jpeg_get_config == NULL || !jpeg_get_config->task_state)
	{
		return BK_FAIL;
	}

	jpeg_get_config->task_state = false;

	jpeg_get_task_send_msg(JPEGDEC_STOP, 0);
	rtos_get_semaphore(&jpeg_get_config->jdec_sem, BEKEN_NEVER_TIMEOUT);

	jpeg_get_task_deinit();

	LOGD("%s(%d) complete\n", __func__, __LINE__);

	return BK_OK;
}

