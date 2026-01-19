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

#ifndef _FB_PORT_H__
#define _FB_PORT_H__

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/framebuf.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    uint32_t node_size;
    uint32_t node_num;
} framebuf_port_cfg_t;

#define FRAMEBUF_PORT_NODE_SIZE     (1 * 1024)
#define FRAMEBUF_PORT_NODE_NUM      (4)


#define FRAMEBUF_PORT_CFG_DEFAULT() {           \
    .node_size = FRAMEBUF_PORT_NODE_SIZE,       \
    .node_num = FRAMEBUF_PORT_NODE_NUM,         \
}

audio_port_handle_t framebuf_port_init(framebuf_port_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif//_FB_PORT_H__

