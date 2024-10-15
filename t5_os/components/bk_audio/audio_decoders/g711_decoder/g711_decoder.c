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
#include "g711_decoder.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "modules/g711.h"
#include <os/os.h>


#define TAG  "G711_DECODER"

typedef struct g711_decoder {
	g711_decoder_mode_t     dec_mode;       /*!< 0: a-law  1: u-law */
} g711_decoder_t;


static bk_err_t _g711_decoder_open(audio_element_handle_t self)
{
	BK_LOGD(TAG, "[%s] _g711_decoder_open \n", audio_element_get_tag(self));
//	BK_LOGI(TAG, "[%s] 0xD5 ->G711-> 0x%04x \n", audio_element_get_tag(self), alaw2linear(0xD5));

	return BK_OK;
}

static bk_err_t _g711_decoder_close(audio_element_handle_t self)
{
	BK_LOGD(TAG, "[%s] _g711_decoder_close \n", audio_element_get_tag(self));
	return BK_OK;
}

static int _g711_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
	BK_LOGD(TAG, "[%s] _g711_decoder_process \n", audio_element_get_tag(self));
	g711_decoder_t *g711_dec = (g711_decoder_t *)audio_element_getdata(self);

	int r_size = audio_element_input(self, in_buffer, in_len);

	int w_size = 0;
	if (r_size > 0) {
		int16_t *g711_out_ptr = audio_malloc(r_size<<1);
		AUDIO_MEM_CHECK(TAG, g711_out_ptr, return -1);

		uint8_t *law = (uint8_t *)in_buffer;
		if (g711_dec->dec_mode == G711_DEC_MODE_U_LOW) {
			for (uint32_t i = 0; i < r_size; i++) {
				g711_out_ptr[i] = ulaw2linear(law[i]);
			}
		} else {
			for (uint32_t i = 0; i < r_size; i++) {
				g711_out_ptr[i] = alaw2linear(law[i]);
			}
		}

		w_size = audio_element_output(self, (char *)g711_out_ptr, r_size<<1);
		audio_free(g711_out_ptr);
	} else {
		w_size = r_size;
	}
	return w_size;
}

static bk_err_t _g711_decoder_destroy(audio_element_handle_t self)
{
	g711_decoder_t *g711_dec = (g711_decoder_t *)audio_element_getdata(self);
	audio_free(g711_dec);
	return BK_OK;
}

audio_element_handle_t g711_decoder_init(g711_decoder_cfg_t *config)
{
	audio_element_handle_t el;
	g711_decoder_t *g711_dec = audio_calloc(1, sizeof(g711_decoder_t));

	AUDIO_MEM_CHECK(TAG, g711_dec, return NULL);

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	cfg.open = _g711_decoder_open;
	cfg.close = _g711_decoder_close;
	cfg.seek = NULL;
	cfg.process = _g711_decoder_process;
	cfg.destroy = _g711_decoder_destroy;
	cfg.read = NULL;
	cfg.write = NULL;
	cfg.task_stack = config->task_stack;
	cfg.task_prio = config->task_prio;
	cfg.task_core = config->task_core;
	cfg.out_rb_size = config->out_rb_size;
	cfg.buffer_len = config->buf_sz;

	cfg.tag = "g711_decoder";
	g711_dec->dec_mode = config->dec_mode;

	el = audio_element_init(&cfg);

	AUDIO_MEM_CHECK(TAG, el, goto _g711_decoder_init_exit);
	audio_element_setdata(el, g711_dec);
	return el;
_g711_decoder_init_exit:
	audio_free(g711_dec);
	return NULL;
}

