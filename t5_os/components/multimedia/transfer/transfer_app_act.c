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
#include <components/log.h>
#include <components/video_types.h>
#include "media_evt.h"
#include "media_app.h"
#include "transfer_act.h"
#include <driver/aon_rtc.h>

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#include "media_mailbox_list_util.h"

#define TAG "trs_app"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

media_debug_t *transfer_log = NULL;
static beken_thread_t transfer_app_task = NULL;
static beken_queue_t transfer_app_msg_que = NULL;
static beken_semaphore_t transfer_app_sem = NULL;
static frame_buffer_t *current_frame = NULL;
static media_mailbox_msg_t *transfer_app_node = NULL;
frame_cb_t frame_read_callback = NULL;
static bool transfer_app_task_running = false;

static bk_err_t transfer_app_task_send_msg(uint8_t type, uint32_t data)
{
	bk_err_t ret = kNoErr;
	trs_task_msg_t msg;

	if (transfer_app_msg_que)
	{
		msg.type = type;
		msg.data = data;

		ret = rtos_push_to_queue(&transfer_app_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			LOGE("transfer_app_task_send_msg failed, type:%d, %d\r\n", type, ret);
			return kNoResourcesErr;
		}

		return ret;
	}
	return kGeneralErr;
}

static uint32_t transfer_app_get_current_timer(void)
{
	uint64_t timer = 0;

#ifdef CONFIG_ARCH_RISCV
	timer = (riscv_get_mtimer() / 26) & 0xFFFFFFFF;// tick
#else // CONFIG_ARCH_RISCV

#ifdef CONFIG_AON_RTC
	timer = bk_aon_rtc_get_us() & 0xFFFFFFFF;
#endif

#endif // CONFIG_ARCH_RISCV

	return (uint32_t)timer;
}

static void transfer_app_task_send_handle(uint32_t param)
{
	uint32_t before = 0, after = 0;

	if (current_frame)
	{
#if (CONFIG_CACHE_ENABLE)
		flush_dcache(current_frame->frame, current_frame->size);
#endif

		before = transfer_app_get_current_timer();

		if (frame_read_callback)
		{
			frame_read_callback(current_frame);
		}

		after = transfer_app_get_current_timer();

		transfer_log->meantimes += (after - before);

		current_frame = NULL;

		// send finish notify to cp1
		msg_send_rsp_to_media_app_mailbox(transfer_app_node, BK_OK);
	}
}

static void transfer_app_task_entry(beken_thread_arg_t data)
{
	int ret = kNoErr;

	LOGI("%s start\r\n", __func__);

	transfer_app_task_running = true;
	rtos_set_semaphore(&transfer_app_sem);

	while (1)
	{
		trs_task_msg_t msg;
		ret = rtos_pop_from_queue(&transfer_app_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case TRS_TRANSFER_DATA:
					transfer_app_task_send_handle(msg.data);
					break;

				case TRS_TRANSFER_EXIT:
					goto exit;
					break;

				default:
					break;
			}
		}
	};

exit:

	LOGI("transfer_app_task exit\n");
	
	frame_read_callback = NULL;
	current_frame = NULL;
	transfer_app_node = NULL;

	transfer_app_task_running = false;

	rtos_deinit_queue(&transfer_app_msg_que);
	transfer_app_msg_que = NULL;

	transfer_app_task = NULL;
	rtos_set_semaphore(&transfer_app_sem);
	rtos_delete_thread(NULL);
}

bk_err_t transfer_app_task_init(frame_cb_t cb)
{
	int ret = BK_OK;

	if (transfer_app_task_running)
	{
		LOGI("transfer_app_task already init!\r\n");
		return ret;
	}

	frame_read_callback = cb;

	if (transfer_app_sem == NULL)
	{
		ret = rtos_init_semaphore(&transfer_app_sem, 1);
		if (ret != BK_OK)
		{
			LOGE("%s, transfer_app_sem init error", __func__);
			goto error;
		}
	}

	if ((!transfer_app_task) && (!transfer_app_msg_que))
	{
		ret = rtos_init_queue(&transfer_app_msg_que,
								"transfer_app_msg_que",
								sizeof(trs_task_msg_t),
								10);
		if (BK_OK != ret)
		{
			LOGE("%s transfer_app_queue init failed\n", __func__);
			ret = BK_ERR_NO_MEM;
			goto error;
		}

		ret = rtos_create_thread(&transfer_app_task,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"transfer_app_task",
								(beken_thread_function_t)transfer_app_task_entry,
								CONFIG_TRANS_APP_TASK_SIZE,
								NULL);

		if (BK_OK != ret)
		{
			LOGE("%s transfer_app_task init failed\n", __func__);
			ret = BK_ERR_NO_MEM;
			goto error;
		}
	}

	rtos_get_semaphore(&transfer_app_sem, BEKEN_NEVER_TIMEOUT);

	return ret;

error:

	frame_read_callback = NULL;

	if (transfer_app_sem)
	{
		rtos_deinit_semaphore(&transfer_app_sem);
		transfer_app_sem = NULL;
	}

	if (transfer_app_msg_que)
	{
		rtos_deinit_queue(&transfer_app_msg_que);
		transfer_app_msg_que = NULL;
	}

	if (transfer_app_task)
	{
		transfer_app_task = NULL;
	}

	return ret;
}

static bk_err_t transfer_app_task_start_handle(media_mailbox_msg_t *mailbox_msg)
{
	if (transfer_app_task_running)
	{
		transfer_app_node = mailbox_msg;
		current_frame = (frame_buffer_t *)transfer_app_node->param;

		if (transfer_app_task_send_msg(TRS_TRANSFER_DATA, mailbox_msg->param) != BK_OK)
		{
			msg_send_rsp_to_media_app_mailbox(mailbox_msg, BK_FAIL);
		}
	}
	else
	{
		LOGI("%s transfer_app_task not start\r\n", __func__);
		msg_send_rsp_to_media_app_mailbox(mailbox_msg, BK_OK);
	}

	return kNoErr;
}

static bk_err_t transfer_app_task_close_handle(media_mailbox_msg_t *mailbox_msg)
{
	if (transfer_app_task_running)
	{
		transfer_app_task_running = false;

		transfer_app_task_send_msg(TRS_TRANSFER_EXIT, mailbox_msg->param);

		rtos_get_semaphore(&transfer_app_sem, BEKEN_NEVER_TIMEOUT);

		rtos_deinit_semaphore(&transfer_app_sem);
		transfer_app_sem = NULL;
	}

	LOGI("%s transfer_app_task closed\r\n", __func__);

	return kNoErr;
}

static bk_err_t transfer_app_task_log_handle(media_mailbox_msg_t *mailbox_msg)
{
	transfer_log = (media_debug_t *)mailbox_msg->param;

	transfer_log->count = 0;
	transfer_log->meantimes = 0;
	transfer_log->retry_times = 0;

	return BK_OK;
}

bk_err_t transfer_app_event_handle(media_mailbox_msg_t *msg)
{
	int ret = kNoErr;

	switch (msg->event)
	{
		case EVENT_MEDIA_DATA_NOTIFY:
			ret = transfer_app_task_start_handle(msg);
			break;

		case EVENT_TRANSFER_CLOSE_IND:
			ret = transfer_app_task_close_handle(msg);
			break;

		case EVENT_TRANS_LOG_NOTIFY:
			ret = transfer_app_task_log_handle(msg);
			break;

		default:
			break;
	}

	return ret;
}

