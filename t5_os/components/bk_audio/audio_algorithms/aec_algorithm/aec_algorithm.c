// Copyright 2022-2023 Beken
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
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "aec_algorithm.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include <os/os.h>


#define TAG  "AEC_ALGORITHM"

typedef struct aec_algorithm {
	aec_cfg_t aec_cfg;
	int out_rb_block;
	AECContext *aec_ctx;
	int16_t* ref_addr;
	int16_t* mic_addr;
	int16_t* out_addr;
	uint32_t frame_size;		//20ms data
} aec_algorithm_t;


static bk_err_t _aec_algorithm_open(audio_element_handle_t self)
{
	uint32_t val = 0;
	uint32_t aec_context_size = 0;

	BK_LOGD(TAG, "[%s] _aec_algorithm_open \n", audio_element_get_tag(self));
	aec_algorithm_t *aec = (aec_algorithm_t *)audio_element_getdata(self);

	aec_context_size = aec_size(AEC_DELAY_SAMPLE_POINTS_MAX);
	BK_LOGD(TAG, "sizeof(AECContext) = %d\n", aec_context_size);

	/* init */
	aec->aec_ctx = (AECContext*)audio_malloc(aec_context_size);

	//采样率可以配置8000或者16000
	aec_init(aec->aec_ctx, aec->aec_cfg.fs);

	//获取结构体内部可以复用的ram作为每帧tx,rx,out数据的临时buffer; ram很宽裕的话也可以在外部单独申请获取
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_TX_BUF, (uint32_t)(&val)); aec->mic_addr = (int16_t*)val;
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_RX_BUF, (uint32_t)(&val)); aec->ref_addr = (int16_t*)val;
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_OUT_BUF,(uint32_t)(&val)); aec->out_addr = (int16_t*)val;

	//以下是参数调节示例,aec_init中都已经有默认值,可以直接先用默认值
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_FLAGS, 0x1f);										//库内各模块开关; aec_init内默认赋值0x1f;

	///回声消除相关
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_MIC_DELAY, aec->aec_cfg.delay_points);		//设置参考信号延迟(采样点数，需要dump数据观察)
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EC_DEPTH, aec->aec_cfg.ec_depth);			//建议取值范围1~50; 后面几个参数建议先用aec_init内的默认值，具体需要根据实际情况调试; 总得来说回声越大需要调的越大
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_TxRxThr, aec->aec_cfg.TxRxThr);				//建议取值范围10~64
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_TxRxFlr, aec->aec_cfg.TxRxFlr); 			//建议取值范围1~10
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_REF_SCALE, aec->aec_cfg.ref_scale);			//取值0,1,2；rx数据如果幅值太小的话适当放大
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_VOL, 14);									//通话过程中如果需要经常调节喇叭音量就设置下当前音量等级
	///降噪相关
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_LEVEL, aec->aec_cfg.ns_level);			//建议取值范围1~8；值越小底噪越小
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_PARA, aec->aec_cfg.ns_para);				//只能取值0,1,2; 降噪由弱到强，建议默认值
	///drc(输出音量相关)
	aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_DRC, 0x15);									//建议取值范围0x10~0x1f;   越大输出声音越大

	BK_LOGI(TAG, "[%s] _aec_algorithm_open, frame_20ms_size: %d \n", audio_element_get_tag(self), aec->aec_cfg.fs * 2 / 1000 * 20);

	return BK_OK;
}

static bk_err_t _aec_algorithm_close(audio_element_handle_t self)
{
	BK_LOGD(TAG, "[%s] _aec_algorithm_close \n", audio_element_get_tag(self));
	return BK_OK;
}

static int _aec_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	BK_LOGD(TAG, "[%s] _aec_algorithm_process, in_len: %d \n", audio_element_get_tag(self), in_len);
	aec_algorithm_t *aec = (aec_algorithm_t *)audio_element_getdata(self);

	int r_size = audio_element_input(self, in_buffer, in_len);
	if (r_size != in_len)
		BK_LOGE(TAG, "mic_data Waring: r_size=%d, in_len=%d \n", r_size, in_len);
	if (aec->aec_cfg.mode == AEC_MODE_HARDWARE) {
		int16_t * lr_data_ptr = (int16_t *)in_buffer;
		for (uint16_t i = 0; i < r_size/4; i++) {
			aec->mic_addr[i] = lr_data_ptr[2*i];
			aec->ref_addr[i] = lr_data_ptr[2*i + 1];
		}
	} else {
		aec->mic_addr = (int16_t *)in_buffer;
		int r_size = audio_element_multi_input(self, (char *)aec->ref_addr, in_len, 0, 0);
		if (r_size != in_len) {
			BK_LOGD(TAG, "ref_data Waring: r_size=%d, in_len=%d line:%d \n", r_size, in_len, __LINE__);
			os_memset(aec->ref_addr, 0, in_len);
		} else {
			//BK_LOGD(TAG, "ref_data Waring: r_size=%d, line:%d \n", r_size, __LINE__);
		}
	}

	int w_size = 0;
	if (r_size > 0) {
		aec_proc(aec->aec_ctx, aec->ref_addr, aec->mic_addr, aec->out_addr);
		w_size = audio_element_output(self, (char *)aec->out_addr, aec->frame_size);
	} else {
		w_size = r_size;
	}
	return w_size;
}

static bk_err_t _aec_algorithm_destroy(audio_element_handle_t self)
{
	aec_algorithm_t *aec = (aec_algorithm_t *)audio_element_getdata(self);
	if (aec->aec_ctx) {
		audio_free(aec->aec_ctx);
		aec->aec_ctx = NULL;
	}
	audio_free(aec);
	return BK_OK;
}

audio_element_handle_t aec_algorithm_init(aec_algorithm_cfg_t *config)
{
	audio_element_handle_t el;

	/* check config */
	if (config->aec_cfg.fs != 8000 && config->aec_cfg.fs != 16000) {
		BK_LOGE(TAG, "check config->aec_cfg.fs fail \n");
		return NULL;
	}

	aec_algorithm_t *aec_alg = audio_calloc(1, sizeof(aec_algorithm_t));
	AUDIO_MEM_CHECK(TAG, aec_alg, return NULL);

	aec_alg->frame_size = config->aec_cfg.fs / 1000 * 2 * 20;

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	cfg.open = _aec_algorithm_open;
	cfg.close = _aec_algorithm_close;
	cfg.seek = NULL;
	cfg.process = _aec_algorithm_process;
	cfg.destroy = _aec_algorithm_destroy;
	cfg.read = NULL;
	cfg.write = NULL;
	cfg.task_stack = config->task_stack;
	cfg.task_prio = config->task_prio;
	cfg.task_core = config->task_core;
	/* 20ms, 16bit */
	cfg.out_rb_size = aec_alg->frame_size * config->out_rb_block;
	if (config->aec_cfg.mode == AEC_MODE_HARDWARE) {
		cfg.buffer_len = aec_alg->frame_size * 2;
	} else {
		cfg.buffer_len = aec_alg->frame_size;
		cfg.multi_in_rb_num = 1;
	}

	cfg.tag = "aec_algorithm";
	el = audio_element_init(&cfg);
	AUDIO_MEM_CHECK(TAG, el, goto _aec_algorithm_init_exit);

	aec_alg->aec_cfg.mode = config->aec_cfg.mode;
	aec_alg->aec_cfg.fs = config->aec_cfg.fs;
	aec_alg->aec_cfg.delay_points = config->aec_cfg.delay_points;
	aec_alg->aec_cfg.ec_depth = config->aec_cfg.ec_depth;
	aec_alg->aec_cfg.TxRxThr = config->aec_cfg.TxRxThr;
	aec_alg->aec_cfg.TxRxFlr = config->aec_cfg.TxRxFlr;
	aec_alg->aec_cfg.ref_scale = config->aec_cfg.ref_scale;
	aec_alg->aec_cfg.ns_level = config->aec_cfg.ns_level;
	aec_alg->aec_cfg.ns_para = config->aec_cfg.ns_para;
	aec_alg->out_rb_block = config->out_rb_block;
	aec_alg->aec_ctx = NULL;
	aec_alg->ref_addr = NULL;
	aec_alg->mic_addr = NULL;
	aec_alg->out_addr = NULL;
	audio_element_setdata(el, aec_alg);
	return el;
_aec_algorithm_init_exit:
	audio_free(aec_alg);
	return NULL;
}

