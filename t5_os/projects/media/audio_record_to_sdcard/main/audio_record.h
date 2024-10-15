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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	AUDIO_RECORD_IDLE = 0,
	AUDIO_RECORD_EXIT,
	AUDIO_RECORD_MAX
} audio_record_op_t;

typedef struct {
	audio_record_op_t op;
	void *param;
} audio_record_msg_t;

typedef struct {
	char *file_name;
	uint32_t samp_rate;
} audio_record_setup_t;

bk_err_t audio_record_to_sdcard_start(char *file_name, uint32_t samp_rate);

bk_err_t audio_record_to_sdcard_stop(void);

#ifdef __cplusplus
}
#endif


