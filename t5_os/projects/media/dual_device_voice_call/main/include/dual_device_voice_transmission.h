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

//#include "media_app.h"
//#include "lcd_act.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_SIZE                (2 * 1024)
#define THREAD_PROIRITY            4
#define VOICE_PORT                 5001
//#define VIDEO_PORT                 7180
#define TRANSFER_BUF_SIZE          1472    //(4 * 1024)


typedef enum {
	VOICE_DEVICE_ROLE_CLIENT = 0,
	VOICE_DEVICE_ROLE_SERVER
} voice_device_role_t;


bk_err_t dual_device_voice_transmission_udp_init(voice_device_role_t role);

bk_err_t dual_device_voice_transmission_udp_deinit(void);

#if 0
bk_err_t dual_device_voice_transmission_tcp_init(void);

bk_err_t dual_device_voice_transmission_tcp_deinit(void);

#endif

#ifdef __cplusplus
}
#endif


