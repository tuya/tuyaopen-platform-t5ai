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

#include <os/os.h>
#include <common/bk_section.h>


#define MIPC_CHAN_SEND_FLAG_SYNC        (1 << 0)
#define MIPC_CHAN_HAND_FLAG_ASYNC       (1 << 1)

#define MIPC_CHAN_SEND_FLAG_DEFAULT     (0)

#define BK_IPC_CHANNEL_DEF(section_name)                    \
    extern bk_ipc_t section_name;                         \

#define BK_IPC_CHANNEL_REGISTER(_channel, _route, _rx_cb, _param, _tx_cb)                                  \
    bk_ipc_t _channel;                                                                                \
    BK_SECTION_ITEM_REGISTER(ipc_chan_reg, const bk_ipc_chan_cfg_t _channel##section) =             \
    {                                                                                                   \
                .ipc = &(_channel),                                                                     \
                .route = (_route),                                                                      \
                .name = #_channel,                                                                      \
                .rx_cb = _rx_cb,                                                                        \
                .param = _param,                                                                        \
                .tx_cb = _tx_cb,                                                                        \
    }

#define BK_IPC_CHANNEL_REGISTER_COUNT          \
    BK_SECTION_ITEM_COUNT(ipc_chan_reg, bk_ipc_chan_cfg_t)

#define BK_IPC_CHANNEL_REGISTER_GET(i)         \
    BK_SECTION_ITEM_GET(ipc_chan_reg, bk_ipc_chan_cfg_t, (i))


typedef void *ipc_obj_t;

typedef uint32_t (*bk_ipc_rx_cb)(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj);
typedef uint32_t (*bk_ipc_tx_cb)(ipc_obj_t ipc_obj);
typedef void *bk_ipc_t;

typedef enum
{
    IPC_CORE_CPU0,
    IPC_CORE_CPU1,
    IPC_CORE_MAX,
} bk_ipc_core_t;

#define IPC_CPU_COUNT   (IPC_CORE_MAX)

typedef enum
{
    IPC_ROUTE_CPU0_CPU1,
} bk_ipc_route_t;

typedef enum
{
    IPC_MSTATE_IDLE = 0,
    IPC_MSTATE_BUSY,
    IPC_MSTATE_ERROR,
} bk_ipc_mailbox_state_t;

typedef struct
{
    bk_ipc_t *ipc;
    bk_ipc_route_t route;
    char *name;
    bk_ipc_rx_cb rx_cb;
    bk_ipc_tx_cb tx_cb;
    void *param;
} bk_ipc_chan_cfg_t;

typedef struct
{
    bk_ipc_mailbox_state_t state;
    uint8_t thread_running;
    uint8_t vote_state;
    LIST_HEADER_T channel_list;

    LIST_HEADER_T local_list;
    LIST_HEADER_T remote_list;
    LIST_HEADER_T free_list;

    uint8_t cpu_state[IPC_CPU_COUNT];
    beken_semaphore_t cpu_wait[IPC_CPU_COUNT];
    beken_mutex_t wait_lock[IPC_CPU_COUNT];

    beken_mutex_t boot_lock;
    uint32_t boot_request;
    beken2_timer_t boot_timer;
    uint32_t boot_vote;

    beken_semaphore_t sem;
    beken_semaphore_t waiting_sem;
    uint32_t waiting_event;
    beken_thread_t thread;
} bk_ipc_info_t;

typedef struct
{
    uint32_t id;
    bk_ipc_chan_cfg_t *cfg;

    LIST_HEADER_T list;
    bk_ipc_info_t *ipc;
} bk_ipc_handle_t;


typedef struct
{
    LIST_HEADER_T list;
    uint32_t result;
    bk_ipc_handle_t *handle;
    beken_semaphore_t sem;
    void *data;
    uint32_t size;
    uint32_t flags;
} bk_ipc_data_t;


int bk_ipc_init(void);
int bk_ipc_deinit(void);
bk_ipc_core_t bk_ipc_cpu_id_get(void);
int bk_ipc_send(bk_ipc_t *ipc, void *data, uint32_t size, uint32_t flags, uint32_t *result);
bk_err_t bk_ipc_obj_extract(ipc_obj_t obj);
void *bk_ipc_obj_convert(ipc_obj_t obj, uint32_t *size);
bk_err_t bk_ipc_obj_free(ipc_obj_t obj, uint32_t result);

void mb_ipc_reset_notify(u32 cpu_id, u32 power_on);
int mb_ipc_cpu_is_power_on(u32 cpu_id);
int mb_ipc_cpu_is_power_off(u32 cpu_id);