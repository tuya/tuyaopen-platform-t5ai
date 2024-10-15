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

#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>
#include "aud_intf.h"
#include "aud_intf_types.h"
#include <driver/audio_ring_buff.h>
#include <modules/pm.h>
#include <asr_mb.h>
#include <driver/mailbox_channel.h>
#include "ui.h"
#include "voice_player.h"
#include "asr_app.h"

#include "BK7256_RegList.h"

#define ASR_APP_TAG "asr_app"
#define GPIO_DEBUG 0

#define ASR_BUFF_SIZE 8000  //>960*2
static beken_thread_t  asr_app_thread_hdl = NULL;
static beken_queue_t asr_app_msg_que = NULL;

static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

static uint8_t *asr_ring_buff = NULL;
static RingBufferContext asr_rb;
static int8_t *asr_buff = NULL;


typedef enum {
	ASR_APP_INIT = 0,
	ASR_APP_NOTIFY,
	ASR_APP_EXIT,
} asr_app_op_t;

typedef struct {
	asr_app_op_t op;
	uint32_t data;
} asr_app_msg_t;

bk_err_t asr_app_stop(void);

static bk_err_t asr_app_send_msg(asr_app_op_t op, uint32_t data)
{
	bk_err_t ret;

	asr_app_msg_t msg;
	msg.op = op;
	msg.data = data;

	if (asr_app_msg_que) {
		ret = rtos_push_to_queue(&asr_app_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			BK_LOGE(ASR_APP_TAG, "asr_app_send_int_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void asr_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
#if GPIO_DEBUG
	addAON_GPIO_Reg0x8 = 2;
	addAON_GPIO_Reg0x8 = 0;
#endif
	switch(cmd_buf->hdr.cmd)
	{
		case EVENT_ASR_OPEN:
		case EVENT_ASR_CLOSE:
		case EVENT_ASR_PROCESS:
			break;
		case EVENT_ASR_RESULT:
#if GPIO_DEBUG
			addAON_GPIO_Reg0x9 = 2;
			addAON_GPIO_Reg0x9 = 0;
#endif
			asr_app_send_msg(ASR_APP_NOTIFY, cmd_buf->param1);
			break;
		default:
			break;
	}
}

static bk_err_t asr_mailbox_chl_open(void)
{
	bk_err_t ret = BK_OK;
	ret = mb_chnl_open(MB_CHNL_AUD, NULL);
	if (ret == BK_OK)
		mb_chnl_ctrl(MB_CHNL_AUD, MB_CHNL_SET_RX_ISR, asr_mailbox_rx_isr);

	return ret;
}

static bk_err_t asr_mailbox_chal_close(void)
{
	return mb_chnl_close(MB_CHNL_AUD);
}

static bk_err_t asr_send_mb(asr_mb_event_t cmd, uint32 arg)
{
    bk_err_t ret = BK_FAIL;
    mb_chnl_cmd_t mb_cmd;
    mb_cmd.hdr.cmd = cmd;
    mb_cmd.param1 = arg;
    mb_cmd.param2 = 0;
    mb_cmd.param3 = 0;
    ret = mb_chnl_write(MB_CHNL_AUD, &mb_cmd);
    return ret;
}

/* mic file format: signal channel, 16K sample rate, 16bit width */
static int aud_mic_handle(uint8_t *data, unsigned int len)
{
	uint32 uiTemp = 0;
#if GPIO_DEBUG
	addAON_GPIO_Reg0x2 = 2;
	addAON_GPIO_Reg0x2 = 0;
#endif
	/* write data to file */
	if (ring_buffer_get_free_size(&asr_rb) >= len) {
		uiTemp = ring_buffer_write(&asr_rb, data, len);
		if (uiTemp != len) {
			BK_LOGE(ASR_APP_TAG, "%s, write data fail, uiTemp: %d \n", __func__, uiTemp);
		}
	}
	if (ring_buffer_get_fill_size(&asr_rb) >= 960) {
		uiTemp = ring_buffer_read(&asr_rb, (uint8_t *)asr_buff, 960);
//		extern void flush_dcache(void *va, long size);
//		flush_dcache((void *)asr_buff, 960);
		asr_send_mb(EVENT_ASR_PROCESS, (uint32_t)asr_buff);
#if GPIO_DEBUG
		addAON_GPIO_Reg0x3 = 2;
		addAON_GPIO_Reg0x3 = 0;
#endif
	}

	return len;
}


static void asr_app_main(void)
{
	bk_err_t ret = BK_OK;

	/* start voice play task */
//	voice_play_init();

	asr_app_msg_t msg;
	while(1) {
		ret = rtos_pop_from_queue(&asr_app_msg_que, &msg, BEKEN_WAIT_FOREVER);
		//rtos_delay_milliseconds(1000);
		if (kNoErr == ret) {
			if (msg.op == ASR_APP_EXIT) {
				break;
			} else if (msg.op == ASR_APP_NOTIFY) {
				switch (msg.data) {
					case ASR_RESULT_ARMINO:		//阿尔米诺
						ui_tabview_set(1);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
						voice_play_send_msg(VOICE_PLAY_START, "armino.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
						voice_play_send_msg(VOICE_PLAY_START, "armino_EN.pcm");
#endif
						break;

					case ASR_RESULT_RECEPTION:	//会客模式
						lv_event_send(ui_Ui2Panel2, LV_EVENT_CLICKED, NULL);
						if(lv_obj_has_state(ui_Ui2Panel2, LV_STATE_CHECKED)) {
							lv_obj_clear_state(ui_Ui2Panel2, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
							voice_play_send_msg(VOICE_PLAY_START, "hui_ke_mode_close.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
							voice_play_send_msg(VOICE_PLAY_START, "hui_ke_mode_close_EN.pcm");
#endif
						} else {
							lv_obj_add_state(ui_Ui2Panel2, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
							voice_play_send_msg(VOICE_PLAY_START, "hui_ke_mode_open.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
							voice_play_send_msg(VOICE_PLAY_START, "hui_ke_mode_open_EN.pcm");
#endif
						}
						lv_event_send(ui_Ui2Panel2, LV_EVENT_VALUE_CHANGED, NULL);
						break;

					case ASR_RESULT_MEAL:		//用餐模式
						lv_event_send(ui_Ui2Panel4, LV_EVENT_CLICKED, NULL);

						if(lv_obj_has_state(ui_Ui2Panel4, LV_STATE_CHECKED)) {
							lv_obj_clear_state(ui_Ui2Panel4, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
							voice_play_send_msg(VOICE_PLAY_START, "yong_can_mode_close.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
							voice_play_send_msg(VOICE_PLAY_START, "yong_can_mode_close_EN.pcm");
#endif
						} else {
							lv_obj_add_state(ui_Ui2Panel4, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
							voice_play_send_msg(VOICE_PLAY_START, "yong_can_mode_open.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
							voice_play_send_msg(VOICE_PLAY_START, "yong_can_mode_open_EN.pcm");
#endif
						}
						lv_event_send(ui_Ui2Panel4, LV_EVENT_VALUE_CHANGED, NULL);
						break;

					case ASR_RESULT_LEAVE:		//离开模式
						lv_event_send(ui_Ui2Panel3, LV_EVENT_CLICKED, NULL);
						lv_obj_clear_state(ui_Ui2Panel2, LV_STATE_CHECKED);
						lv_obj_clear_state(ui_Ui2Panel4, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
						voice_play_send_msg(VOICE_PLAY_START, "li_kai_mode.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
						voice_play_send_msg(VOICE_PLAY_START, "li_kai_mode_EN.pcm");
#endif
						break;

					case ASR_RESULT_GOHOME:		//回家模式
						lv_event_send(ui_Ui2Panel1, LV_EVENT_CLICKED, NULL);
						lv_obj_add_state(ui_Ui2Panel2, LV_STATE_CHECKED);
						lv_obj_add_state(ui_Ui2Panel4, LV_STATE_CHECKED);
#if CONFIG_86BOX_SMART_PANEL_VERSION_CN
						voice_play_send_msg(VOICE_PLAY_START, "hui_jia_mode.pcm");
#endif
#if CONFIG_86BOX_SMART_PANEL_VERSION_EN
						voice_play_send_msg(VOICE_PLAY_START, "hui_jia_mode_EN.pcm");
#endif
						break;

					default:
						break;
				}
			} else {
				BK_LOGE(ASR_APP_TAG, "not support op: %d \n", msg.op);
			}

		}
	}

	ret = bk_aud_intf_mic_stop();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_mic_stop fail, ret:%d \r\n", ret);
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_mic_stop complete \r\n");
	}
	BK_LOGI(ASR_APP_TAG, "stop mic \r\n");

	ret = bk_aud_intf_mic_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_mic_deinit complete \r\n");
	}

	ret = bk_aud_intf_drv_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_drv_deinit complete \r\n");
	}

	asr_send_mb(EVENT_ASR_CLOSE, 0);

	/* free buffer  */
	ring_buffer_clear(&asr_rb);
	if (asr_ring_buff) {
		os_free(asr_ring_buff);
		asr_ring_buff = NULL;
	}
	if (asr_buff) {
		os_free(asr_buff);
		asr_buff = NULL;
	}

	/* delete msg queue */
	ret = rtos_deinit_queue(&asr_app_msg_que);
	if (ret != kNoErr) {
		BK_LOGE(ASR_APP_TAG, "delete message queue fail \r\n");
	}
	asr_app_msg_que = NULL;
	BK_LOGI(ASR_APP_TAG, "delete message queue complete \r\n");

	/* delete task */
	asr_app_thread_hdl = NULL;
	rtos_delete_thread(NULL);

}

bk_err_t asr_app_start(void)
{
	bk_err_t ret = BK_OK;
	BK_LOGI(ASR_APP_TAG, "init asr app \r\n");

	/* init audio mailbox channel */
	if (BK_OK != asr_mailbox_chl_open()) {
		BK_LOGE(ASR_APP_TAG, "init mailbox fail \n");
		return BK_FAIL;
	}

	asr_ring_buff = os_malloc(ASR_BUFF_SIZE);
	if (asr_ring_buff ==  NULL) {
		BK_LOGE(ASR_APP_TAG, "os_malloc asr_ring_buff fail \n");
		goto exit;
	}

	ring_buffer_init(&asr_rb, asr_ring_buff, ASR_BUFF_SIZE, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	asr_buff = os_malloc(960);
	if (asr_buff ==  NULL) {
		BK_LOGE(ASR_APP_TAG, "os_malloc asr_buff fail \n");
		goto exit;
	}

	aud_intf_drv_setup.aud_intf_tx_mic_data = aud_mic_handle;
	ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		goto exit;
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_drv_init complete \r\n");
	}

	aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
	ret = bk_aud_intf_set_mode(aud_work_mode);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		goto exit;
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_set_mode complete \r\n");
	}

	aud_intf_mic_setup.samp_rate = 16000;
	ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
		goto exit;
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_mic_init complete \r\n");
	}
	BK_LOGI(ASR_APP_TAG, "init mic complete \r\n");

	ret = bk_aud_intf_mic_start();
	if (ret != BK_ERR_AUD_INTF_OK) {
		BK_LOGE(ASR_APP_TAG, "bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		goto exit;
	} else {
		BK_LOGI(ASR_APP_TAG, "bk_aud_intf_mic_start complete \r\n");
	}
	BK_LOGI(ASR_APP_TAG, "start asr test \r\n");

	ret = rtos_init_queue(&asr_app_msg_que,
						  "asr_app_que",
						  sizeof(asr_app_msg_t),
						  1);
	if (ret != kNoErr) {
		BK_LOGE(ASR_APP_TAG, "ceate asr app message queue fail \r\n");
		asr_app_msg_que = NULL;
		goto exit;
	}
	BK_LOGI(ASR_APP_TAG, "ceate asr app message queue complete \r\n");

	/* create task to asr */
	ret = rtos_create_thread(&asr_app_thread_hdl,
						 5,
						 "asr_app",
						 (beken_thread_function_t)asr_app_main,
						 1536,
						 NULL);
	if (ret != kNoErr) {
		BK_LOGE(ASR_APP_TAG, "create asr app task fail \r\n");
		asr_app_thread_hdl = NULL;
		goto exit;
	}
	BK_LOGI(ASR_APP_TAG, "create asr app task complete \r\n");

	return ret;
exit:
	asr_mailbox_chal_close();
	if (asr_ring_buff) {
		ring_buffer_clear(&asr_rb);
		os_free(asr_ring_buff);
		asr_ring_buff = NULL;
	}
	if (asr_buff) {
		os_free(asr_buff);
		asr_buff = NULL;
	}
	bk_aud_intf_mic_stop();
	bk_aud_intf_mic_deinit();
	aud_work_mode = AUD_INTF_WORK_MODE_NULL;
	bk_aud_intf_set_mode(aud_work_mode);
	bk_aud_intf_drv_deinit();
	if (asr_app_msg_que) {
		rtos_deinit_queue(&asr_app_msg_que);
		asr_app_msg_que = NULL;
	}

	return BK_FAIL;
}

bk_err_t asr_app_stop(void)
{
	return asr_app_send_msg(ASR_APP_EXIT, 0);
}

