// Copyright 2023-2024 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fatfs_coprocess.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "audio_mem.h"

#define TAG  "FTFS_COP"

static bk_err_t fatfs_open_handle(media_mailbox_msg_t *msg)
{
	FRESULT fr;
	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_open_param_t *fatfs_open = (fatfs_open_param_t *)audio_msg->param;
	FIL *fp = os_malloc(sizeof(FIL));
	if (!fp) {
		BK_LOGE(TAG, "%s calloc fp fail, line: %d \n", __func__, __LINE__);
		goto exit;
	}
	os_memset(fp, 0x00, sizeof(FIL));

	fr = f_open(fp, fatfs_open->path, fatfs_open->mode);
	if (fr != FR_OK) {
		BK_LOGE(TAG, "Failed to open. File name: %s, error: %d, line: %d \n", fatfs_open->path, fr, __LINE__);
		goto exit;
	}
	fatfs_open->fp = (void *)fp;
	BK_LOGD(TAG, "%s fatfs_open->fp: %p \n", __func__, fatfs_open->fp);
	/* send response mailbox message to cpu1 */
	msg_send_rsp_to_media_app_mailbox(msg, BK_OK);

	return BK_OK;

exit:
	if (fp) {
		os_free(fp);
		fp = NULL;
	}
	msg_send_rsp_to_media_app_mailbox(msg, BK_FAIL);
	return BK_FAIL;
}

static bk_err_t fatfs_size_handle(media_mailbox_msg_t *msg)
{
	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_fp_param_t *fatfs_fp = (fatfs_fp_param_t *)audio_msg->param;
	/* send response mailbox message to cpu1 */
	fatfs_fp->size = f_size((FIL *)fatfs_fp->fp);
	msg_send_rsp_to_media_app_mailbox(msg, BK_OK);
	return BK_OK;
}

static bk_err_t fatfs_lseek_handle(media_mailbox_msg_t *msg)
{
	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_lseek_param_t *fatfs_lseek = (fatfs_lseek_param_t *)audio_msg->param;
	/* send response mailbox message to cpu1 */
	bk_err_t ret = f_lseek((FIL *)fatfs_lseek->fp, fatfs_lseek->ofs);
	if (ret >= 0) {
		fatfs_lseek->result = ret;
		msg_send_rsp_to_media_app_mailbox(msg, BK_OK);
	} else {
		fatfs_lseek->result = 0;
		msg_send_rsp_to_media_app_mailbox(msg, ret);
	}
	return ret;
}

static int fatfs_read_handle(media_mailbox_msg_t *msg)
{
	FRESULT fr;
	int ret = BK_OK;
	/* use file descriptors to access files */
	uint32 rlen = 0;

	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_rw_param_t *fatfs_rd = (fatfs_rw_param_t *)audio_msg->param;
	fr = f_read((FIL *)fatfs_rd->fp, fatfs_rd->buff, fatfs_rd->len, &rlen);
	if (fr == FR_OK) {
		fatfs_rd->result = rlen;
	}else {
		BK_LOGE(TAG, "%s read data fail. Error message: %s \n", __func__, f_error((FIL *)fatfs_rd->fp));
		ret = BK_FAIL;
		fatfs_rd->result = 0;
	}
	/* send response mailbox message to cpu1 */
	msg_send_rsp_to_media_app_mailbox(msg, ret);
	return ret;
}

static int fatfs_write_handle(media_mailbox_msg_t *msg)
{
	FRESULT fr;
	int ret = BK_OK;
	uint32 wlen = 0;

	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_rw_param_t *fatfs_wt = (fatfs_rw_param_t *)audio_msg->param;
	fr = f_write((FIL *)fatfs_wt->fp, fatfs_wt->buff, fatfs_wt->len, &wlen);
	if (fr == FR_OK) {
		fatfs_wt->result = wlen;
	} else {
		BK_LOGE(TAG, "%s write data fail. Error message: %s \n", __func__, f_error((FIL *)fatfs_wt->fp));
		ret = BK_FAIL;
		fatfs_wt->result = 0;
	}
	/* send response mailbox message to cpu1 */
	msg_send_rsp_to_media_app_mailbox(msg, ret);
	return ret;
}

static bk_err_t fatfs_close_handle(media_mailbox_msg_t *msg)
{
	audio_element_mb_t *audio_msg = (audio_element_mb_t *)msg->param;
	fatfs_fp_param_t *fatfs_fp = (fatfs_fp_param_t *)audio_msg->param;
	/* send response mailbox message to cpu1 */
	BK_LOGD(TAG, "%s fatfs_close->fp: %p \n", __func__, fatfs_fp->fp);
	bk_err_t ret = f_close((FIL *)fatfs_fp->fp);
	if (ret == BK_OK) {
		os_free((FIL *)fatfs_fp->fp);
		fatfs_fp->fp = NULL;
	} else {
		BK_LOGE(TAG, "%s fatfs close fail, ret: %d \n", __func__, ret);
	}
	msg_send_rsp_to_media_app_mailbox(msg, ret);
	return ret;
}

static void fatfs_coprocess_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	audio_element_coprocess_ctx_t *fatfs_coprocess_ctx = (audio_element_coprocess_ctx_t *)param_data;

	rtos_set_semaphore(&fatfs_coprocess_ctx->sem);

	audio_msg_t msg;
	media_mailbox_msg_t *mb_msg;
	while(1) {
		ret = rtos_pop_from_queue(&fatfs_coprocess_ctx->audio_element_coprocess_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			mb_msg = (media_mailbox_msg_t *)msg.param;
			switch (mb_msg->event) {
				case EVENT_FATFS_DEINIT:
					BK_LOGD(TAG, "goto: EVENT_FATFS_DEINIT \r\n");
					goto fatfs_coprocess_exit;
					break;

				case EVENT_FATFS_OPEN:
					BK_LOGD(TAG, "goto: EVENT_FATFS_OPEN \r\n");
					fatfs_open_handle(mb_msg);
					break;

				case EVENT_FATFS_CLOSE:
					BK_LOGD(TAG, "goto: EVENT_FATFS_CLOSE \r\n");
					fatfs_close_handle(mb_msg);
					break;

				case EVENT_FATFS_READ:
					BK_LOGD(TAG, "goto: EVENT_FATFS_READ \r\n");
					fatfs_read_handle(mb_msg);
					break;

				case EVENT_FATFS_WRITE:
					BK_LOGD(TAG, "goto: EVENT_FATFS_WRITE \r\n");
					fatfs_write_handle(mb_msg);
					break;

				case EVENT_FATFS_SIZE:
					BK_LOGD(TAG, "goto: EVENT_FATFS_SIZE \r\n");
					fatfs_size_handle(mb_msg);
					break;

				case EVENT_FATFS_LSEEK:
					BK_LOGD(TAG, "goto: EVENT_FATFS_LSEEK \r\n");
					fatfs_lseek_handle(mb_msg);
					break;

				default:
					break;
			}
		}
	}

fatfs_coprocess_exit:
	/* delete msg queue */
	ret = rtos_deinit_queue(&fatfs_coprocess_ctx->audio_element_coprocess_msg_que);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "delete fatfs coprocess message queue fail \r\n");
	}
	fatfs_coprocess_ctx->audio_element_coprocess_msg_que = NULL;
	BK_LOGI(TAG, "delete fatfs coprocess message queue complete \r\n");

	/* delete task */
	fatfs_coprocess_ctx->audio_element_coprocess_thread_hdl = NULL;

	if (fatfs_coprocess_ctx && fatfs_coprocess_ctx->audio_element_data) {
		os_free(fatfs_coprocess_ctx->audio_element_data);
		fatfs_coprocess_ctx->audio_element_data = NULL;
	}
	if (fatfs_coprocess_ctx) {
		os_free(fatfs_coprocess_ctx);
		fatfs_coprocess_ctx = NULL;
	}

	/* send response mailbox message to cpu1 */
	msg_send_rsp_to_media_app_mailbox(mb_msg, BK_OK);

	BK_LOGI(TAG, "delete fatfs coprocess task complete \r\n");
	rtos_delete_thread(NULL);
}


audio_element_coprocess_ctx_t *fatfs_coprocess_create(audio_element_coprocess_cfg_t *setup_cfg)
{
	bk_err_t ret = BK_OK;

	audio_element_coprocess_ctx_t *fatfs_coprocess_ctx = (audio_element_coprocess_ctx_t *)os_malloc(sizeof(audio_element_coprocess_ctx_t));
	if (!fatfs_coprocess_ctx) {
		BK_LOGE(TAG, "malloc fatfs_coprocess_ctx fail. line: %d \n", __LINE__);
		goto exit;
	}
	os_memset(fatfs_coprocess_ctx, 0x00, sizeof(audio_element_coprocess_ctx_t));

	/* init FIL */
	fatfs_coprocess_ctx->audio_element_data = os_malloc(sizeof(FIL));
	if (!fatfs_coprocess_ctx->audio_element_data) {
		BK_LOGE(TAG, "malloc fatfs_coprocess_ctx->audio_element_data fail. line: %d \n", __LINE__);
		goto exit;
	}
	os_memset(fatfs_coprocess_ctx->audio_element_data, 0x00, sizeof(FIL));

	ret = rtos_init_semaphore(&fatfs_coprocess_ctx->sem, 1);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "malloc fatfs_coprocess_ctx fail. line: %d \n", __LINE__);
		goto exit;
	}

	ret = rtos_init_queue(&fatfs_coprocess_ctx->audio_element_coprocess_msg_que,
						  "fatfs_cop_que",
						  sizeof(audio_msg_t),
						  30);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "ceate fatfs coprocess message queue fail \n");
		goto exit;
	}
	BK_LOGI(TAG, "ceate fatfs coprocess message queue complete \n");

	ret = rtos_create_thread(&fatfs_coprocess_ctx->audio_element_coprocess_thread_hdl,
						 setup_cfg->task_prio,
						 "fatfs_cop",
						 (beken_thread_function_t)fatfs_coprocess_main,
						 setup_cfg->task_stack,
						 (beken_thread_arg_t)fatfs_coprocess_ctx);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "create fatfs coprocess task fail \n");
		fatfs_coprocess_ctx->audio_element_coprocess_thread_hdl = NULL;
		goto exit;
	}
	BK_LOGI(TAG, "create fatfs coprocess task complete \n");

	rtos_get_semaphore(&fatfs_coprocess_ctx->sem, portMAX_DELAY);

	rtos_deinit_semaphore(&fatfs_coprocess_ctx->sem);

	BK_LOGI(TAG, "fatfs_coprocess_ctx: %p, msg_que: %p \n", fatfs_coprocess_ctx, fatfs_coprocess_ctx->audio_element_coprocess_msg_que);

	return fatfs_coprocess_ctx;

exit:
	if (fatfs_coprocess_ctx && fatfs_coprocess_ctx->sem) {
		rtos_deinit_semaphore(&fatfs_coprocess_ctx->sem);
		fatfs_coprocess_ctx->sem = NULL;
	}
	if (fatfs_coprocess_ctx && fatfs_coprocess_ctx->audio_element_coprocess_msg_que) {
		rtos_deinit_queue(&fatfs_coprocess_ctx->audio_element_coprocess_msg_que);
		fatfs_coprocess_ctx->audio_element_coprocess_msg_que = NULL;
	}
	if (fatfs_coprocess_ctx && fatfs_coprocess_ctx->audio_element_data) {
		os_free(fatfs_coprocess_ctx->audio_element_data);
		fatfs_coprocess_ctx->audio_element_data = NULL;
	}
	if (fatfs_coprocess_ctx) {
		os_free(fatfs_coprocess_ctx);
		fatfs_coprocess_ctx = NULL;
	}

	return NULL;
}
