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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <components/bk_audio/audio_pipeline/framebuf.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>

#define TAG   "FRAMEBUF"


typedef STAILQ_HEAD(framebuf_node_list, framebuf_node_item) framebuf_node_list_t;

struct framebuf {
    uint8_t                     fb_total_node_num;           /**< frame buffer total node num includes free list and ready list */
    uint32_t                    fb_node_size;                /**< frame buffer node size, unit byte */
    framebuf_node_list_t        free_fb_node_list;           /**< free frame buffer node list */
    framebuf_node_list_t        ready_fb_node_list;          /**< ready frame buffer node list */

    SemaphoreHandle_t           can_read;                    /**< frame buffer can read when ready frame buffer node list is not empty */
    SemaphoreHandle_t           can_malloc;                  /**< frame buffer can malloc when free frame buffer node list is not empty */
    SemaphoreHandle_t           lock;                        /**< frame buffer lock */
    bool                        abort_read;                  /**< abort read frame buffer node */
    bool                        abort_malloc;                /**< abort malloc frame buffer node */
    bool                        is_done_write;               /**< To signal that we are done writing */
};

static bk_err_t fb_abort_read(framebuf_handle_t fb);
static bk_err_t fb_abort_malloc(framebuf_handle_t fb);

framebuf_handle_t fb_create(int node_size, int n_nodes, int info_size)
{
    if (node_size <= 0 || n_nodes < 1) {
        BK_LOGE(TAG, "Invalid size or number \n");
        return NULL;
    }

    framebuf_handle_t fb;
    bool _success =
        (
            (fb             = (framebuf_handle_t)audio_calloc(1, sizeof(struct framebuf))) &&
            (fb->can_read   = xSemaphoreCreateBinary())             &&
            (fb->lock       = xSemaphoreCreateMutex())              &&
            (fb->can_malloc  = xSemaphoreCreateBinary())
        );

    AUDIO_MEM_CHECK(TAG, _success, goto _fb_init_failed);

    fb->fb_node_size = node_size;
    fb->fb_total_node_num = n_nodes;

    STAILQ_INIT(&fb->free_fb_node_list);
    STAILQ_INIT(&fb->ready_fb_node_list);

    for (uint32_t i = 0; i < n_nodes; i++) {

        framebuf_node_item_t *fb_node_item_ptr = (framebuf_node_item_t *)audio_calloc(1, sizeof(framebuf_node_item_t));
        AUDIO_MEM_CHECK(TAG, fb_node_item_ptr, goto _fb_init_failed);

        framebuf_node_t fb_node = (framebuf_node_t)audio_calloc(1, sizeof(struct framebuf_node));
        AUDIO_MEM_CHECK(TAG, fb_node, goto _fb_init_failed);
        fb_node->size = fb->fb_node_size;
        fb_node->info_size = info_size;
        fb_node->buffer = (uint8_t *)audio_calloc(1, fb_node->size);
        AUDIO_MEM_CHECK(TAG, fb_node->buffer, goto _fb_init_failed);
        fb_node->info = (void *)audio_calloc(1, info_size);
        AUDIO_MEM_CHECK(TAG, fb_node->info, goto _fb_init_failed);

        fb_node_item_ptr->fb_node = fb_node;
        STAILQ_INSERT_TAIL(&fb->free_fb_node_list, fb_node_item_ptr, next);
    }

    fb->abort_read = false;
    fb->abort_malloc = false;
    fb->is_done_write = false;

    return fb;
_fb_init_failed:
    fb_destroy(fb);
    return NULL;
}

bk_err_t fb_destroy(framebuf_handle_t fb)
{
    if (fb == NULL) {
        //return BK_ERR_VTSDF_INVALID_ARG;
        return FB_FAIL;
    }

    framebuf_node_item_t *fb_node_item_ptr, *fb_node_tmp;

    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->free_fb_node_list, next, fb_node_tmp) {
        BK_LOGV(TAG, "%d, fb_node_buffer:%p size:%d \n", __LINE__, fb_node_item_ptr->fb_node->buffer, fb_node_item_ptr->fb_node->size);
        STAILQ_REMOVE(&fb->free_fb_node_list, fb_node_item_ptr, framebuf_node_item, next);
        if (fb_node_item_ptr->fb_node && fb_node_item_ptr->fb_node->buffer) {
            audio_free(fb_node_item_ptr->fb_node->buffer);
            fb_node_item_ptr->fb_node->buffer = NULL;
        }
        if (fb_node_item_ptr->fb_node && fb_node_item_ptr->fb_node->info) {
            audio_free(fb_node_item_ptr->fb_node->info);
            fb_node_item_ptr->fb_node->info = NULL;
        }
        if (fb_node_item_ptr->fb_node) {
            audio_free(fb_node_item_ptr->fb_node);
            fb_node_item_ptr->fb_node = NULL;
        }
        if (fb_node_item_ptr) {
            audio_free(fb_node_item_ptr);
            fb_node_item_ptr = NULL;
        }
    }

    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->ready_fb_node_list, next, fb_node_tmp) {
        BK_LOGV(TAG, "%d, fb_node_buffer:%p size:%d \n", __LINE__, fb_node_item_ptr->fb_node->buffer, fb_node_item_ptr->fb_node->size);
        STAILQ_REMOVE(&fb->ready_fb_node_list, fb_node_item_ptr, framebuf_node_item, next);
        if (fb_node_item_ptr->fb_node && fb_node_item_ptr->fb_node->buffer) {
            audio_free(fb_node_item_ptr->fb_node->buffer);
            fb_node_item_ptr->fb_node->buffer = NULL;
        }
        if (fb_node_item_ptr->fb_node && fb_node_item_ptr->fb_node->info) {
            audio_free(fb_node_item_ptr->fb_node->info);
            fb_node_item_ptr->fb_node->info = NULL;
        }
        if (fb_node_item_ptr->fb_node) {
            audio_free(fb_node_item_ptr->fb_node);
            fb_node_item_ptr->fb_node = NULL;
        }
        if (fb_node_item_ptr) {
            audio_free(fb_node_item_ptr);
            fb_node_item_ptr = NULL;
        }
    }

    if (fb->can_read) {
        vSemaphoreDelete(fb->can_read);
        fb->can_read = NULL;
    }
    if (fb->can_malloc) {
        vSemaphoreDelete(fb->can_malloc);
        fb->can_malloc = NULL;
    }
    if (fb->lock) {
        vSemaphoreDelete(fb->lock);
        fb->lock = NULL;
    }
    audio_free(fb);
    fb = NULL;
    return BK_OK;
}

bk_err_t fb_reset(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }

    framebuf_node_item_t *fb_node_item_ptr, *fb_node_tmp;

    /* move frame_buffer node from ready_fb_node_list to free_fb_node_list */
    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->ready_fb_node_list, next, fb_node_tmp) {
        BK_LOGV(TAG, "%d, fb_node_buffer:%p size:%d \n", __LINE__, fb_node_item_ptr->fb_node->buffer, fb_node_item_ptr->fb_node->size);
        STAILQ_REMOVE(&fb->free_fb_node_list, fb_node_item_ptr, framebuf_node_item, next);

        fb_node_item_ptr->fb_node->length = 0;
        os_memset(fb_node_item_ptr->fb_node->info, 0, fb_node_item_ptr->fb_node->info_size);
        STAILQ_INSERT_TAIL(&fb->free_fb_node_list, fb_node_item_ptr, next);
    }

    fb->is_done_write = false;
    fb->abort_read = false;
    fb->abort_malloc = false;
    return BK_OK;
}

#define fb_release(handle) xSemaphoreGive(handle)
#define fb_block(handle, time) xSemaphoreTake(handle, time)

int fb_read(framebuf_handle_t fb, framebuf_node_item_t **fb_node_item, TickType_t ticks_to_wait)
{
    int ret_val = 0;

    if (fb == NULL) {
        return FB_FAIL;
    }

    framebuf_node_item_t *fb_node_item_ptr = NULL, *fb_node_tmp;

    while (1) {
        /* take frame buffer lock */
        if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
            *fb_node_item = NULL;
            ret_val = FB_TIMEOUT;
            goto read_err;
        }

        /* check whether frame_buffer node is exists in ready_fb_node_list */
        STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->ready_fb_node_list, next, fb_node_tmp) {
            BK_LOGV(TAG, "%d, fb_node_buffer:%p size:%d \n", __LINE__, fb_node_item_ptr->fb_node->buffer, fb_node_item_ptr->fb_node->size);
            if (fb_node_item_ptr) {
                STAILQ_REMOVE(&fb->ready_fb_node_list, fb_node_item_ptr, framebuf_node_item, next);
                *fb_node_item = fb_node_item_ptr;
//                BK_LOGE(TAG, "fb_node_item:%p, *fb_node_item:%p fb_node_item_ptr:%p \n", fb_node_item, *fb_node_item, fb_node_item_ptr);
                ret_val = fb_node_item_ptr->fb_node->length;
                fb_release(fb->lock);
                goto read_err;
            }
        }

        if (fb->is_done_write)
        {
            ret_val = FB_DONE;
            fb_release(fb->lock);
            goto read_err;
        }

        /* no data to read, release thread block to allow other threads to write data */
        if (fb->abort_read) {
            *fb_node_item = NULL;
            ret_val = FB_ABORT;
            fb_release(fb->lock);
            goto read_err;
        }

        fb_release(fb->lock);
        //wait till some frame_buffer node available to read
        if (fb_block(fb->can_read, ticks_to_wait) != pdTRUE) {
            *fb_node_item = NULL;
            ret_val = FB_TIMEOUT;
            goto read_err;
        }
        continue;
    }

read_err:

    return ret_val;
}

int fb_free(framebuf_handle_t fb, framebuf_node_item_t *fb_node_item, TickType_t ticks_to_wait)
{
    int ret = 0;

    if (fb == NULL || fb_node_item == NULL) {
        return FB_FAIL;
    }

    /* take frame buffer lock */
    if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
        return FB_TIMEOUT;
    }

    ret = fb_node_item->fb_node->length;

    fb_node_item->fb_node->length = 0;
    os_memset(fb_node_item->fb_node->info, 0, fb_node_item->fb_node->info_size);

    STAILQ_INSERT_TAIL(&fb->free_fb_node_list, fb_node_item, next);

    fb_release(fb->lock);
    fb_release(fb->can_malloc);

    return ret;
}

int fb_malloc(framebuf_handle_t fb, framebuf_node_item_t **fb_node_item, TickType_t ticks_to_wait)
{
    int ret_val = 0;

    if (fb == NULL) {
        return FB_FAIL;
    }

    framebuf_node_item_t *fb_node_item_ptr = NULL, *fb_node_tmp;

    while (1) {
        /* take frame buffer lock */
        if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
            *fb_node_item = NULL;
            ret_val = FB_TIMEOUT;
            goto out;
        }

        /* check whether frame_buffer node is exists in free_fb_node_list */
        STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->free_fb_node_list, next, fb_node_tmp) {
            BK_LOGV(TAG, "%s, %d, fb_node_buffer:%p size:%d \n", __func__, __LINE__, fb_node_item_ptr->fb_node->buffer, fb_node_item_ptr->fb_node->size);
            if (fb_node_item_ptr) {
                STAILQ_REMOVE(&fb->free_fb_node_list, fb_node_item_ptr, framebuf_node_item, next);
                *fb_node_item = fb_node_item_ptr;
                ret_val = fb_node_item_ptr->fb_node->size;
                fb_release(fb->lock);
                goto out;
            }
        }

        /* no free frame_buffer node to malloc, release thread block to allow other threads to free frame_buffer node */
        if (fb->is_done_write) {
            ret_val = FB_DONE;
            fb_release(fb->lock);
            goto out;
        }

        if (fb->abort_malloc) {
            *fb_node_item = NULL;
            ret_val = FB_ABORT;
            fb_release(fb->lock);
            goto out;
        }

        fb_release(fb->lock);
        //wait till some frame_buffer node available to read
        if (fb_block(fb->can_malloc, ticks_to_wait) != pdTRUE) {
            *fb_node_item = NULL;
            ret_val = FB_TIMEOUT;
            goto out;
        }
        continue;
    }

out:

    return ret_val;
}

int fb_write(framebuf_handle_t fb, framebuf_node_item_t *fb_node_item, TickType_t ticks_to_wait)
{
    if (fb == NULL || fb_node_item == NULL) {
        return FB_FAIL;
    }

    /* take frame buffer lock */
    if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
        return FB_TIMEOUT;
    }

    STAILQ_INSERT_TAIL(&fb->ready_fb_node_list, fb_node_item, next);

    int data_length = fb_node_item->fb_node->length;

    fb_release(fb->lock);
    fb_release(fb->can_read);

    return data_length;
}

static bk_err_t fb_abort_read(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }
    fb->abort_read = true;
    xSemaphoreGive(fb->can_read);
    return BK_OK;
}

static bk_err_t fb_abort_malloc(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }
    fb->abort_malloc = true;
    xSemaphoreGive(fb->can_malloc);
    return BK_OK;
}

bk_err_t fb_abort(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }
    bk_err_t err = fb_abort_read(fb);
    err |= fb_abort_malloc(fb);
    return err;
}

bk_err_t fb_done_write(framebuf_handle_t fb)
{
    if (fb == NULL)
    {
        return BK_ERR_ADK_INVALID_ARG;
    }
    fb->is_done_write = true;
    fb_release(fb->can_read);
    return BK_OK;
}

int fb_get_node_size(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }
    return fb->fb_node_size;
}

int fb_get_total_node_num(framebuf_handle_t fb)
{
    if (fb == NULL) {
        return FB_FAIL;
    }
    return fb->fb_total_node_num;
}

int fb_get_ready_node_num(framebuf_handle_t fb)
{
    framebuf_node_item_t *fb_node_item_ptr, *fb_node_tmp;
    int ready_node_num = 0;

    if (fb == NULL) {
        return FB_FAIL;
    }

    /* take frame buffer lock */
    if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
        return FB_TIMEOUT;
    }

    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->ready_fb_node_list, next, fb_node_tmp) {
        if (fb_node_item_ptr)
        {
            ready_node_num++;
        }
    }

    fb_release(fb->lock);

    return ready_node_num;
}

void debug_fb_node_lists(framebuf_handle_t fb, int line, const char *func)
{
    framebuf_node_item_t *fb_node_item_ptr, *fb_node_tmp;

    if (fb == NULL) {
        return;
    }

    /* take frame buffer lock */
    if (fb_block(fb->lock, portMAX_DELAY) != pdTRUE) {
        return;
    }

    BK_LOGD(TAG, "FUNC:%s, LINE:%d \n", func, line);

    BK_LOGD(TAG, "fb: %p, fb_total_node_num: %d, fb_total_node_size:%d \n", fb, fb->fb_total_node_num, fb->fb_node_size);

    BK_LOGD(TAG, "free-node-list: \n");
    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->free_fb_node_list, next, fb_node_tmp) {
        BK_LOGD(TAG, "node_ptr:%p, length:%d, buffer:%p, size: %d, info:%p, info_size:%d\n",
                fb_node_item_ptr,
                fb_node_item_ptr->fb_node->length,
                fb_node_item_ptr->fb_node->buffer,
                fb_node_item_ptr->fb_node->size,
                fb_node_item_ptr->fb_node->info,
                fb_node_item_ptr->fb_node->info_size);
    }
    BK_LOGD(TAG, "\n");

    BK_LOGD(TAG, "ready-node-list: \n");
    STAILQ_FOREACH_SAFE(fb_node_item_ptr, &fb->free_fb_node_list, next, fb_node_tmp) {
        BK_LOGD(TAG, "node_ptr:%p, length:%d, buffer:%p, size: %d, info:%p, info_size:%d\n",
                fb_node_item_ptr,
                fb_node_item_ptr->fb_node->length,
                fb_node_item_ptr->fb_node->buffer,
                fb_node_item_ptr->fb_node->size,
                fb_node_item_ptr->fb_node->info,
                fb_node_item_ptr->fb_node->info_size);
    }

    fb_release(fb->lock);

    BK_LOGD(TAG, "\n");
}

void debug_fb_node(framebuf_node_item_t *fb_node_item, int line, const char *func)
{
    if (fb_node_item == NULL) {
        return;
    }

    BK_LOGD(TAG, "FUNC:%s, LINE:%d \n", func, line);

    BK_LOGD(TAG, "node_ptr:%p, length:%d, buffer:%p, size: %d, info:%p, info_size:%d\n",
            fb_node_item,
            fb_node_item->fb_node->length,
            fb_node_item->fb_node->buffer,
            fb_node_item->fb_node->size,
            fb_node_item->fb_node->info,
            fb_node_item->fb_node->info_size);
}

