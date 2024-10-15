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
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "raw_stream.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"

//#include "BK7256_RegList.h"


#define TAG  "RAW_STR"
typedef struct raw_stream {
	audio_stream_type_t type;
} raw_stream_t;

int raw_stream_read(audio_element_handle_t pipeline, char *buffer, int len)
{
//	addAON_GPIO_Reg0x4 = 2;

	int ret = audio_element_input(pipeline, buffer, len);
	if (ret == AEL_IO_DONE || ret == AEL_IO_OK) {
		audio_element_report_status(pipeline, AEL_STATUS_STATE_FINISHED);
	} else if (ret < 0) {
		audio_element_report_status(pipeline, AEL_STATUS_STATE_STOPPED);
	}
//	addAON_GPIO_Reg0x4 = 0;
	return ret;
}

int raw_stream_write(audio_element_handle_t pipeline, char *buffer, int len)
{
	int ret = audio_element_output(pipeline, buffer, len);
	if (ret == AEL_IO_DONE || ret == AEL_IO_OK) {
		audio_element_report_status(pipeline, AEL_STATUS_STATE_FINISHED);
	} else if (ret < 0) {
		audio_element_report_status(pipeline, AEL_STATUS_STATE_STOPPED);
	}
	return ret;
}

static bk_err_t _raw_destroy(audio_element_handle_t self)
{
	raw_stream_t *raw = (raw_stream_t *)audio_element_getdata(self);
	audio_free(raw);
	return BK_OK;
}

audio_element_handle_t raw_stream_init(raw_stream_cfg_t *config)
{
	raw_stream_t *raw = audio_calloc(1, sizeof(raw_stream_t));
	AUDIO_MEM_CHECK(TAG, raw, return NULL);

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	cfg.task_stack = -1;	// Not need creat task
	cfg.destroy = _raw_destroy;
	cfg.tag = "raw";
	cfg.out_rb_size = config->out_rb_size;
	raw->type = config->type;
	audio_element_handle_t el = audio_element_init(&cfg);
	AUDIO_MEM_CHECK(TAG, el, {audio_free(raw);return NULL;});
	audio_element_setdata(el, raw);

	/* set read data timeout */
	audio_element_set_input_timeout(el, 2000);	//15 / portTICK_RATE_MS

	BK_LOGD(TAG, "stream init,el:%p", el);
	return el;
}

