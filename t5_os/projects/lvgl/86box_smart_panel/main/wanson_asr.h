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

#ifndef __WANSON_ASR_H__
#define __WANSON_ASR_H__

#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WANSON_ASR_IDLE = 0,
	WANSON_ASR_START,
	WANSON_ASR_EXIT
} wanson_asr_op_t;

typedef struct {
	wanson_asr_op_t op;
	void *param;
} wanson_asr_msg_t;


bk_err_t wanson_asr_init(void);

bk_err_t wanson_asr_deinit(void);

bk_err_t wanson_asr_start(void);

bk_err_t wanson_asr_stop(void);

#ifdef __cplusplus
}
#endif

#endif