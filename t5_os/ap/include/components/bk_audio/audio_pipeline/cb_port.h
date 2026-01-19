// Copyright 2025-2026 Beken
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

#ifndef _CB_PORT_H__
#define _CB_PORT_H__

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    port_stream_func cb;
    void *ctx;
} callback_port_cfg_t;

#define CALLBACK_PORT_CFG_DEFAULT() {       \
    .cb = NULL,                             \
    .ctx = NULL,                            \
}

audio_port_handle_t callback_port_init(callback_port_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif//_CB_PORT_H__

