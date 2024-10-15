// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "fatfs_coprocess.h"


#define TAG "aud_cop"

#define MSG_QUE_COUNT  20

static beken_thread_t audio_coprocess_task = NULL;
static beken_queue_t audio_coprocess_msg_que = NULL;


bk_err_t audio_coprocess_send_msg(audio_msg_t *msg)
{
	int ret = kNoErr;

	if (audio_coprocess_msg_que)
	{
		ret = rtos_push_to_queue(&audio_coprocess_msg_que, msg, BEKEN_NO_WAIT);

		if (kNoErr != ret)
		{
			BK_LOGE(TAG, "%s failed\n", __func__);
			return kOverrunErr;
		}

		return ret;
	}

	return kNoResourcesErr;
}

static bk_err_t audio_element_open_handle(media_mailbox_msg_t *mb_msg)
{
	bk_err_t ret = BK_OK;
	BK_LOGD(TAG, "%s \n", __func__);
	audio_element_mb_t *audio_msg = (audio_element_mb_t *)mb_msg->param;
	audio_element_coprocess_cfg_t cfg = {0};
	fatfs_init_param_t *init_param = (fatfs_init_param_t *)audio_msg->param;

	switch (audio_msg->module) {
		case AUDIO_ELEMENT_FATFS:
			cfg.task_stack = init_param->task_stack;
			cfg.task_prio = init_param->task_prio;
			audio_element_coprocess_ctx_t *coprocess_ctx = fatfs_coprocess_create(&cfg);
			/* send response mailbox message to cpu1 */
			if (coprocess_ctx) {
				audio_msg->coprocess_hdl = coprocess_ctx;
				msg_send_rsp_to_media_app_mailbox(mb_msg, BK_OK);
			} else {
				audio_msg->coprocess_hdl = NULL;
				msg_send_rsp_to_media_app_mailbox(mb_msg, BK_FAIL);
			}
			break;

		default:
			break;
	}

	return ret;
}

static void audio_coprocess_task_main(beken_thread_arg_t data)
{
	int ret = kNoErr;

	while (1)
	{
		audio_msg_t msg;
		media_mailbox_msg_t *mb_msg;
		audio_element_coprocess_ctx_t *coprocess_ctx = NULL;

		ret = rtos_pop_from_queue(&audio_coprocess_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			mb_msg = (media_mailbox_msg_t *)msg.param;
			audio_element_mb_t *audio_msg = (audio_element_mb_t *)mb_msg->param;
			switch (msg.event)
			{
				case (AUD_EVENT << MEDIA_EVT_BIT):			//init event, init task
					audio_element_open_handle(mb_msg);
					break;

				default:
					coprocess_ctx = (audio_element_coprocess_ctx_t *)audio_msg->coprocess_hdl;
					if (coprocess_ctx && coprocess_ctx->audio_element_coprocess_msg_que)
					{
						ret = rtos_push_to_queue(&coprocess_ctx->audio_element_coprocess_msg_que, &msg, BEKEN_NO_WAIT);
						if (kNoErr != ret)
						{
							BK_LOGE(TAG, "%s send msg failed, line:%d \n", __func__, __LINE__);
							//send response mailbox msg to cpu1
							msg_send_rsp_to_media_app_mailbox(mb_msg, BK_FAIL);
						}
					} else {
						BK_LOGE(TAG, "cannot send msg to element coprocessor \n");
						msg_send_rsp_to_media_app_mailbox(mb_msg, BK_FAIL);
					}
					break;
			}
		} else {
			BK_LOGE(TAG, "%s rtos_pop_from_queue failed, line:%d \n", __func__, __LINE__);
			goto exit;
		}
	}

exit:

	rtos_deinit_queue(&audio_coprocess_msg_que);
	audio_coprocess_msg_que = NULL;

	audio_coprocess_task = NULL;
	rtos_delete_thread(NULL);
}


bk_err_t audio_coprocess_task_init(void)
{
	int ret = kNoErr;

	if ((!audio_coprocess_task) && (!audio_coprocess_msg_que))
	{
		ret = rtos_init_queue(&audio_coprocess_msg_que,
								"aud_cop_msg_que",
								sizeof(audio_msg_t),
								MSG_QUE_COUNT);
		if (kNoErr != ret)
		{
			BK_LOGE(TAG, "audio_coprocess_msg_que init failed\n");
			ret = kNoMemoryErr;
			goto error;
		}

		ret = rtos_create_thread(&audio_coprocess_task,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"aud_cop",
								(beken_thread_function_t)audio_coprocess_task_main,
								2 * 1024,
								NULL);
		if (kNoErr != ret)
		{
			BK_LOGE(TAG, "audio_coprocess_task init failed\n");
			ret = kNoMemoryErr;
			goto error;
		}
	}

	BK_LOGI(TAG, "%s create audio coprocess task complete \n", __func__);

	return ret;

error:
	if (audio_coprocess_msg_que)
	{
		rtos_deinit_queue(&audio_coprocess_msg_que);
		audio_coprocess_msg_que = NULL;
	}
	if (audio_coprocess_task)
	{
		audio_coprocess_task = NULL;
	}

	return ret;
}

