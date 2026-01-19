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

#ifndef _FRAMEBUF_H__
#define _FRAMEBUF_H__

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bsd_queue.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FB_OK           (BK_OK)
#define FB_FAIL         (BK_FAIL)
#define FB_DONE         (-2)
#define FB_ABORT        (-3)
#define FB_TIMEOUT      (-4)


struct framebuf_node{
    uint8_t *         buffer;                             /**< data buffer address */
    uint32_t          size;                               /**< node data buffer size */
    uint32_t          length;                             /**< valid data length, unit byte */
    void *            info;                               /**< node information */
    uint32_t          info_size;                          /**< node information size */
};

typedef struct framebuf_node *framebuf_node_t;

typedef struct framebuf_node_item {
    STAILQ_ENTRY(framebuf_node_item)    next;
    framebuf_node_t                     fb_node;
} framebuf_node_item_t;

typedef struct framebuf *framebuf_handle_t;

/**
 * @brief      Create framebuffer with total size = node_size * n_nodes
 *
 * @param[in]  node_size        Size of each frame buffer node
 * @param[in]  n_nodes          Number of frame buffer nodes
 * @param[in]  info_size        Size of node information
 *
 * @return     framebuf_handle_t
 */
framebuf_handle_t fb_create(int node_size, int n_nodes, int info_size);

/**
 * @brief      Cleanup and free all memory created by framebuf_handle_t
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t fb_destroy(framebuf_handle_t fb);

/**
 * @brief      Abort waiting until there is space for reading or mallocing of the framebuffer
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t fb_abort(framebuf_handle_t fb);

/**
 * @brief      Mark the framebuffer as done writing, no more data will be written to the framebuffer
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t fb_done_write(framebuf_handle_t fb);

/**
 * @brief      Reset framebuffer, clear all values as initial state
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t fb_reset(framebuf_handle_t fb);

/**
 * @brief      Get each node size of framebuffer (in bytes)
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return     node size of framebuffer
 */
int fb_get_node_size(framebuf_handle_t fb);

/**
 * @brief      Get the total node number of framebuffer
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return     total node number of framebuffer
 */
int fb_get_total_node_num(framebuf_handle_t fb);

/**
 * @brief      Get the ready node(filled valid data) number of framebuffer
 *
 * @param[in]  fb    The framebuffer handle
 *
 * @return     ready node number of framebuffer
 */
int fb_get_ready_node_num(framebuf_handle_t fb);

/**
 * @brief      Read framebuffer node from ready framebuffer node list and wait `tick_to_wait` ticks until enough framebuffer node to read
 *             if the ready framebuffer list is exist node.
 *
 * @param[in]  fb             The framebuffer handle
 * @param[out] fb_node_item   The framebuffer node item to read out
 * @param[in]  ticks_to_wait  The ticks to wait
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
int fb_read(framebuf_handle_t fb, framebuf_node_item_t **fb_node_item, TickType_t ticks_to_wait);

/**
 * @brief      Write framebuffer node to ready framebuffer and wait `tick_to_wait` ticks until can write
 *
 * @param[in]  fb             The framebuffer handle
 * @param[out] fb_node_item   The framebuffer node item to write
 * @param[in]  ticks_to_wait  The ticks to wait
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
int fb_write(framebuf_handle_t fb, framebuf_node_item_t *fb_node_item, TickType_t ticks_to_wait);

/**
 * @brief      Get framebuffer node from free framebuffer list and wait `tick_to_wait` ticks until enough framebuffer node to malloc
 *             if the free framebuffer list available is exist node.
 *
 * @param[in]   fb             The framebuffer handle
 * @param[out]  fb_node_item   The framebuffer node item to malloc
 * @param[in]   ticks_to_wait  The ticks to wait
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
int fb_malloc(framebuf_handle_t fb, framebuf_node_item_t **fb_node_item, TickType_t ticks_to_wait);

/**
 * @brief      Push framebuffer node to free framebuffer and wait `tick_to_wait` ticks until can push
 *
 * @param[in]  fb             The framebuffer handle
 * @param[in]  fb_node_item   The framebuffer node item to free
 * @param[in]  ticks_to_wait  The ticks to wait
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
int fb_free(framebuf_handle_t fb, framebuf_node_item_t *fb_node_item, TickType_t ticks_to_wait);

/**
 * @brief      Debug framebuffer, printf framebuffer node information
 *
 * @param[in]  fb             The framebuffer handle
 * @param[in]  line           The line
 * @param[in]  func           The function call this api
 *
 * @return     None
 */
void debug_fb_node_lists(framebuf_handle_t fb, int line, const char *func);

/**
 * @brief      Debug framebuffer node, printf framebuffer node information
 *
 * @param[in]  fb_node_item    The framebuffer node item to debug
 * @param[in]  line            The line
 * @param[in]  func            The function call this api
 *
 * @return     None
 */
void debug_fb_node(framebuf_node_item_t *fb_node_item, int line, const char *func);


#ifdef __cplusplus
}
#endif

#endif
