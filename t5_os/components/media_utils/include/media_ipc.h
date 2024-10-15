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

#include "bk_list.h"

#define MIPC_CHAN_SEND_FLAG_SYNC        (1 << 0)
#define MIPC_CHAN_SEND_FLAG_DEFAULT     (0)

typedef int (*media_ipc_callback)(uint8_t *data, uint32_t size, void *param);
typedef void *meida_ipc_t;

typedef enum
{
	MIPC_CORE_CPU0,
	MIPC_CORE_CPU1,
	MIPC_CORE_CPU2,
} media_ipc_core_t;

#define MIPC_FLAGS_ACK      (1 << 4)

typedef enum
{
	MIPC_MSTATE_IDLE = 0,
	MIPC_MSTATE_BUSY,
	MIPC_MSTATE_ERROR,
} media_ipc_mailbox_state_t;


typedef struct
{
	//media_ipc_channel_t channel;
	char *name;
	media_ipc_callback cb;
	void *param;
} media_ipc_chan_cfg_t;

typedef struct
{
	uint32_t id;
	media_ipc_chan_cfg_t cfg;
	LIST_HEADER_T local_list;
	LIST_HEADER_T remote_list;
	LIST_HEADER_T free_list;
	LIST_HEADER_T list;
	meida_ipc_t *ipc;
} media_ipc_handle_t;

typedef struct
{
	media_ipc_mailbox_state_t state;
	uint8_t thread_running;
	//media_ipc_handle_t *handle[MIPC_CHANNEL_MAX];
	LIST_HEADER_T channel_list;
	beken_semaphore_t sem;
	beken_semaphore_t event;
	beken_thread_t thread;
} media_ipc_info_t;


typedef struct
{
	media_ipc_handle_t *handle;
	LIST_HEADER_T list;
	beken_semaphore_t sem;
	///meida_ipc_t *ipc;
	void *data;
	uint32_t size;
	uint32_t flags;
} media_ipc_data_t;


int media_ipc_init(void);
media_ipc_core_t media_ipc_cpu_id_get(void);
int media_ipc_channel_open(meida_ipc_t *ipc, media_ipc_chan_cfg_t *cfg);
int media_ipc_send(meida_ipc_t *ipc, void *data, uint32_t size, uint32_t flags);
int media_ipc_channel_close(meida_ipc_t *ipc);

