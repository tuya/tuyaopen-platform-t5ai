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

#include <common/bk_include.h>
#include <driver/media_types.h>
#include "aud_intf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MEDIA_UT_START,
	MEDIA_UT_STOP,
} media_ut_msg_type_t;

typedef struct{
	beken_thread_t media_unit_test_thread;
	beken_queue_t media_unit_test_queue;
	beken_semaphore_t media_unit_test_sem;
	char lcd_name[10];
	media_rotate_t rotate;
	uint32_t camera_ppi;
	camera_type_t camera_type;
	aud_intf_mic_type_t mic_type;
	aud_intf_spk_type_t spk_type;

	int id;
	int delay;
	int times;
} media_unit_test_config_t;

int media_unit_test_cli_init(void);

extern media_unit_test_config_t *media_unit_test_config;

#ifdef __cplusplus
}
#endif
