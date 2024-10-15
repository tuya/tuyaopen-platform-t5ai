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

#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int (*voice_send_packet)(unsigned char *data, unsigned int len);
} voice_setup_t;

typedef enum {
	VOICE_SEND_MIC_IDLE = 0,
	VOICE_SEND_MIC_START,
	VOICE_SEND_MIC_EXIT
} voice_send_mic_op_t;

typedef struct {
	voice_send_mic_op_t op;
	void *param;
} voice_send_mic_msg_t;


bk_err_t voice_init(voice_setup_t setup);

bk_err_t voice_deinit(void);

bk_err_t voice_start(void);

bk_err_t voice_stop(void);

bk_err_t voice_write_spk_data(char *buffer, uint32_t size);


#ifdef __cplusplus
}
#endif

