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
#include <driver/psram_types.h>
#include "bk_list.h"
#include "FreeRTOS.h"
#include "event_groups.h"


#ifdef __cplusplus
extern "C" {
#endif

#define INDEX_MASK(bit) (1U << bit)
#define INDEX_UNMASK(bit) (~(1U << bit))
#define FRAME_LIST_NODE_MAX         6
#define COMM_FRAME_LIST_MAX_NODE    (FRAME_LIST_NODE_MAX - 2)
#if (!CONFIG_MEDIA_PSRAM_SIZE_4M)
#define H26X_FRAME_LIST_MAX_NODE    FRAME_LIST_NODE_MAX
#else
#define H26X_FRAME_LIST_MAX_NODE    (FRAME_LIST_NODE_MAX - 2)
#endif
#define YUV_FRAME_LIST_MAX_NODE     (FRAME_LIST_NODE_MAX - 3)

typedef enum
{
	MODULE_WIFI,
	MODULE_DECODER,
	MODULE_LCD,
	MODULE_CAPTURE,
	MODULE_MAX,
} frame_module_t;

typedef struct {
    LIST_HEADER_T list;
    frame_buffer_t *frame;
    uint32_t read_mask;
    uint32_t free_mask;
} frame_node_t;

typedef struct {
    uint8_t trigger;
    uint8_t count;
    uint8_t camera_type;
    uint8_t invalid;
    uint8_t deinit;
    uint16_t camera_id;
    uint16_t img_format;
    uint32_t register_mask;
    beken_semaphore_t read_sem;
    LIST_HEADER_T ready;
    LIST_HEADER_T free;
    frame_node_t *node_list[FRAME_LIST_NODE_MAX];
} frame_list_node_t;

typedef struct {
    frame_list_node_t *node;
    LIST_HEADER_T list;
} frame_list_t;

/*************************display*****************************/
frame_buffer_t *frame_buffer_display_malloc(uint32_t size);
void frame_buffer_display_free(frame_buffer_t *frame);

/*************************encode*****************************/
frame_buffer_t *frame_buffer_encode_malloc(uint32_t size);
void frame_buffer_encode_free(frame_buffer_t *frame);

/********************camera frame list************************/
/*init/deinit camera frame list and init psram as frame buffer memory*/
bk_err_t frame_buffer_list_init(void);
bk_err_t frame_buffer_list_deinit(void);

/*get camera frame list by camera_id(port) & camera_type & img_format*/
frame_list_node_t *frame_buffer_list_get_by_type(uint16_t camera_id, uint8_t camera_type, uint16_t img_format);

/*get camera frame list by img_format*/
frame_list_node_t *frame_buffer_list_get_by_format(uint16_t img_format);

/*get main camera frame list*/
frame_list_node_t *frame_buffer_list_get_main_stream(void);

/*set main camera frame list*/
bk_err_t frame_buffer_list_set_main_stream(uint16_t camera_id, uint8_t camera_type, uint16_t img_format);

/*init(create) camera frame list with camera_id(port) & camera_type & img_format*/
void *frame_buffer_list_node_init(uint16_t camera_id, uint8_t camera_type, uint16_t img_format);

/*deinit camera frame list*/
bk_err_t frame_buffer_list_node_deinit(frame_list_node_t *node);

/*malloc a frame_buffer node from current camera frame list*/
frame_buffer_t *frame_buffer_fb_malloc(frame_list_node_t *node, uint32_t size);

/*free ready node to current camera frame list (free list)*/
bk_err_t frame_buffer_list_node_clear(frame_list_node_t *node);

/*free a frame_buffer node to current camera frame list (free list)*/
bk_err_t frame_buffer_fb_free(frame_list_node_t *node, frame_buffer_t *frame);

/*push a frame_buffer node to current camera frame list (ready list)*/
bk_err_t frame_buffer_fb_push(frame_list_node_t *node, frame_buffer_t *frame);

/*pop a frame_buffer node from current camera frame list duration timeout, and delect from this list(ready list)*/
frame_buffer_t *frame_buffer_fb_pop(frame_list_node_t *node, uint32_t timeout);

/*register module to read frame_buffer from current camera frame list*/
bk_err_t frame_buffer_fb_register(frame_list_node_t *node, frame_module_t module);

/*deregister module to not read frame_buffer from current camera frame list*/
bk_err_t frame_buffer_fb_deregister(frame_list_node_t *node, frame_module_t module);

/*module read frame_buffer from current camera frame list duration timeout*/
frame_buffer_t *frame_buffer_fb_read(frame_list_node_t *node, frame_module_t module, uint32_t timeout);

/*module read frame_buffer from current camera frame list and free*/
void frame_buffer_fb_read_free(frame_list_node_t *node, frame_buffer_t *frame, frame_module_t mode);

#ifdef __cplusplus
}
#endif
