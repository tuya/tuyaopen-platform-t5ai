// Copyright 2024-2025 Beken
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
#include <driver/int.h>
#include <components/log.h>
#include "frame_buffer.h"
#include "psram_mem_slab.h"
#include "avdk_crc.h"
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif

#include "mlist.h"

#define TAG "frame_buffer"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define FB_ALLOCATED_PATTERN      (0x8338)
#define FB_FREE_PATTERN           (0xF00F)
#define FB_OPTIMIZER_ENABLE       (1)

#define ALIGN_BITS  (5)

typedef struct
{
    frame_list_node_t *main;
    LIST_HEADER_T list;
} fb_info_t;


static fb_info_t *fb_info = NULL;

#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t fb_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t fb_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&fb_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void fb_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&fb_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

frame_buffer_t *frame_buffer_display_malloc(uint32_t size)
{
    frame_buffer_t *frame = bk_psram_frame_buffer_malloc(PSRAM_HEAP_YUV, size + sizeof(frame_buffer_t) + (1 << ALIGN_BITS));

    if (frame == NULL)
    {
        return NULL;
    }

    os_memset(frame, 0, sizeof(frame_buffer_t));
    frame->frame = (uint8_t *)((((uint32_t)(frame + 1) >> ALIGN_BITS) + 1) << ALIGN_BITS);
    frame->size = size;
    frame->flag = FB_ALLOCATED_PATTERN;
    frame->frame_crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);

    return frame;
}

void frame_buffer_display_free(frame_buffer_t *frame)
{
    if (frame == NULL)
    {
        LOGE("%s %d buffer is NULL\n", __func__, __LINE__);
        return;
    }
    if (frame->flag == FB_ALLOCATED_PATTERN)
    {
        uint8_t crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);
        if (crc == frame->frame_crc)
        {
            frame->flag = FB_FREE_PATTERN;
            frame->cb = NULL;
            bk_psram_frame_buffer_free(frame);
        }
        else
        {
            LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
        }
    }
    else
    {
        LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
    }
}

frame_buffer_t *frame_buffer_encode_malloc(uint32_t size)
{
    frame_buffer_t *frame = bk_psram_frame_buffer_malloc(PSRAM_HEAP_ENCODE, size + sizeof(frame_buffer_t) + (1 << ALIGN_BITS));

    if (frame == NULL)
    {
        return NULL;
    }

    os_memset(frame, 0, sizeof(frame_buffer_t));
    frame->frame = (uint8_t *)((((uint32_t)(frame + 1) >> ALIGN_BITS) + 1) << ALIGN_BITS);
    frame->size = size;
    frame->flag = FB_ALLOCATED_PATTERN;
    frame->frame_crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);

    return frame;
}

void frame_buffer_encode_free(frame_buffer_t *frame)
{
    if (frame == NULL)
    {
        LOGE("%s %d buffer is NULL\n", __func__, __LINE__);
        return;
    }

    if (frame->flag == FB_ALLOCATED_PATTERN)
    {
        uint8_t crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);
        if (crc == frame->frame_crc)
        {
            frame->flag = FB_FREE_PATTERN;
            bk_psram_frame_buffer_free(frame);
        }
        else
        {
            LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
        }
    }
    else
    {
        LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
    }
}

static bk_err_t frame_buffer_fb_node_remove(frame_node_t *f_node, LIST_HEADER_T *list)
{
    frame_node_t *tmp = NULL;
    LIST_HEADER_T *pos, *n;
    bk_err_t ret = BK_FAIL;

    list_for_each_safe(pos, n, list)
    {
        tmp = list_entry(pos, frame_node_t, list);
        if (tmp != NULL
            && (tmp == f_node))
        {
            list_del(pos);
            ret = BK_OK;
            break;
        }
    }

    return ret;
}

bk_err_t frame_buffer_list_init(void)
{
    if (fb_info != NULL)
    {
        return BK_OK;
    }

    fb_info = (fb_info_t *)os_malloc(sizeof(fb_info_t));
    if (fb_info == NULL)
    {
        BK_ASSERT_EX(0, "%s malloc fail\n", __func__);
        return BK_FAIL;
    }

    os_memset(fb_info, 0, sizeof(fb_info_t));

    INIT_LIST_HEAD(&fb_info->list);

    bk_psram_frame_buffer_init();

    return BK_OK;
}

bk_err_t frame_buffer_list_deinit(void)
{
    if (fb_info == NULL)
    {
        return BK_OK;
    }

    if (!list_empty(&fb_info->list))
    {
        LOGE("%s, list not empty\n", __func__);
        return BK_FAIL;
    }

    os_free(fb_info);
    fb_info = NULL;

    return BK_OK;
}

frame_list_node_t *frame_buffer_list_get_by_type(uint16_t camera_id, uint8_t camera_type, uint16_t img_format)
{
    frame_list_t *tmp = NULL;
    frame_list_node_t *node = NULL;
    LIST_HEADER_T *pos, *n;

    uint32_t flag = fb_enter_critical();

    if (!list_empty(&fb_info->list))
    {
        list_for_each_safe(pos, n, &fb_info->list)
        {
            tmp = list_entry(pos, frame_list_t, list);
            if (tmp != NULL && tmp->node)
            {
                node = tmp->node;
                if (node->camera_id == camera_id
                        && node->camera_type == camera_type
                        && node->img_format == img_format)
                {
                    break;
                }
                node = NULL;
            }
        }
    }

    fb_exit_critical(flag);

    return node;
}

frame_list_node_t *frame_buffer_list_get_by_format(uint16_t img_format)
{
    frame_list_t *tmp = NULL;
    frame_list_node_t *node = NULL;
    LIST_HEADER_T *pos, *n;

    uint32_t flag = fb_enter_critical();

    if (!list_empty(&fb_info->list))
    {
        list_for_each_safe(pos, n, &fb_info->list)
        {
            tmp = list_entry(pos, frame_list_t, list);
            if (tmp != NULL && tmp->node)
            {
                node = tmp->node;
                if (node->img_format == img_format && node->invalid == false)
                {
                    break;
                }
                node = NULL;
            }
        }
    }

    fb_exit_critical(flag);

    return node;
}

frame_list_node_t *frame_buffer_list_get_main_stream(void)
{
    if (fb_info == NULL)
    {
        return NULL;
    }

    return fb_info->main;
}

bk_err_t frame_buffer_list_set_main_stream(uint16_t camera_id, uint8_t camera_type, uint16_t img_format)
{
    frame_list_node_t *node = frame_buffer_list_get_by_type(camera_id, camera_type, img_format);

    if (node && fb_info->main != node)
    {
        fb_info->main = node;
        return BK_OK;
    }
    else
    {
        return BK_FAIL;
    }
}

void *frame_buffer_list_node_init(uint16_t camera_id, uint8_t camera_type, uint16_t img_format)
{
    frame_list_node_t *node = NULL;
    frame_list_t *fb_list_node = NULL;
    frame_node_t *f_node = NULL;

    node = frame_buffer_list_get_by_type(camera_id, camera_type, img_format);

    if (node)
    {
        node->invalid = false;
        node->deinit = false;
        if (fb_info->main == NULL || fb_info->main->invalid)
        {
            fb_info->main = node;

            LOGD("%s, main_stream:%p\n", __func__, fb_info->main);
        }
        return node;
    }

    fb_list_node = (frame_list_t *)os_malloc(sizeof(frame_list_t));
    if (fb_list_node == NULL)
    {
        BK_ASSERT_EX(0, "%s, %d malloc fail\n", __func__, __LINE__);
        return NULL;
    }

    os_memset(fb_list_node, 0, sizeof(frame_list_t));

    INIT_LIST_HEAD(&fb_list_node->list);

    fb_list_node->node = (frame_list_node_t *)os_malloc(sizeof(frame_list_node_t));
    if (fb_list_node->node == NULL)
    {
        BK_ASSERT_EX(0, "%s, %d malloc fail\n", __func__, __LINE__);
        return NULL;
    }

    os_memset(fb_list_node->node, 0 , sizeof(frame_list_node_t));
    rtos_init_semaphore(&fb_list_node->node->read_sem, 1);
    fb_list_node->node->camera_id = camera_id;
    fb_list_node->node->camera_type = camera_type;
    fb_list_node->node->img_format = img_format;
    INIT_LIST_HEAD(&fb_list_node->node->ready);
    INIT_LIST_HEAD(&fb_list_node->node->free);

    if (img_format == IMAGE_H264 || img_format == IMAGE_H265)
    {
        fb_list_node->node->count = H26X_FRAME_LIST_MAX_NODE;
    }
    else if (img_format == IMAGE_MJPEG)
    {
        fb_list_node->node->count = COMM_FRAME_LIST_MAX_NODE;
    }
    else
    {
        // IMAGE_YUV
        fb_list_node->node->count = YUV_FRAME_LIST_MAX_NODE;
    }

    for (uint8_t i = 0; i < fb_list_node->node->count; i++)
    {
        f_node = (frame_node_t *)os_malloc(sizeof(frame_node_t));
        if (f_node == NULL)
        {
            BK_ASSERT_EX(0, "%s, %d malloc fail\n", __func__, __LINE__);
        }

        LOGV("%s, %p %d\n", __func__, f_node, i);
        os_memset(f_node, 0, sizeof(frame_node_t));

        fb_list_node->node->node_list[i] = f_node;
        list_add_tail(&f_node->list, &fb_list_node->node->free);
    }

    list_add_tail(&fb_list_node->list, &fb_info->list);

    LOGD("%s, new_stream:%p, %d-%d-%d\n", __func__, fb_list_node->node, camera_id, camera_type, img_format);

    if (fb_info->main == NULL || fb_info->main->invalid)
    {
        fb_info->main = fb_list_node->node;

        LOGD("%s, main_stream:%p\n", __func__, fb_info->main);
    }

    return (void *)fb_list_node->node;
}

bk_err_t frame_buffer_list_node_deinit(frame_list_node_t *node)
{
    frame_node_t *tmp = NULL;
    frame_list_t *tmp1 = NULL, *tmp_node = NULL;
    LIST_HEADER_T *pos, *n;

    if (node == NULL)
    {
        return BK_OK;
    }

    if (node->register_mask != 0)
    {
        LOGE("there are modes not deregister: %d, deinit:%d\n", node->register_mask, node->deinit);
        node->deinit = true;
        return BK_OK;
    }

    if (!list_empty(&node->free))
    {
        list_for_each_safe(pos, n, &node->free)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL)
            {
                if (tmp->frame)
                {
                    frame_buffer_encode_free(tmp->frame);
                    tmp->frame = NULL;
                }
                list_del(pos);
                os_free(tmp);
                tmp = NULL;
            }
        }
    }

    if (!list_empty(&node->ready))
    {
        list_for_each_safe(pos, n, &node->ready)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL)
            {
                if (tmp->frame)
                {
                    frame_buffer_encode_free(tmp->frame);
                    tmp->frame = NULL;
                }
                list_del(pos);
                os_free(tmp);
                tmp = NULL;
            }
        }
    }

    list_for_each_safe(pos, n, &fb_info->list)
    {
        tmp1 = list_entry(pos, frame_list_t, list);
        if (tmp1 != NULL
            && (tmp1->node == node))
        {
            tmp_node = tmp1;
            list_del(pos);
            break;
        }
    }

    if (tmp_node == NULL)
    {
        LOGE("%s, %d not find this node\n", __func__, __LINE__);
    }
    else
    {
        os_memset(&node->node_list[0], 0, sizeof(frame_node_t *) * FRAME_LIST_NODE_MAX);
        rtos_deinit_semaphore(&node->read_sem);
        if (fb_info->main == node)
        {
            fb_info->main = NULL;
        }
        os_free(tmp_node->node);
        tmp_node->node = NULL;
        os_free(tmp_node);
        tmp_node = NULL;
    }

    LOGD("%s, %p, %d\n", __func__, node, __LINE__);

    return BK_OK;
}

bk_err_t frame_buffer_list_node_invalid(frame_list_node_t *node)
{
    if (node == NULL)
    {
        LOGW("%s, %d node NULL\n", __func__, __LINE__);
        return BK_OK;
    }

    node->invalid = true;
    LOGW("%s, %d, node:%p\n", __func__, __LINE__, node);

    return BK_OK;
}

frame_buffer_t *frame_buffer_fb_malloc(frame_list_node_t *node, uint32_t size)
{
    frame_node_t *tmp = NULL, *f_node = NULL;
    LIST_HEADER_T *pos, *n;

    if (node == NULL || size == 0)
    {
        BK_ASSERT_EX(0, "%s, %d size:%d node NULL\n", __func__, __LINE__, size);
        return NULL;
    }

    uint32_t flag = fb_enter_critical();

    if (!list_empty(&node->free))
    {
        list_for_each_safe(pos, n, &node->free)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL)
            {
                f_node = tmp;
                list_del(pos);
                break;
            }
        }
    }

    if (f_node == NULL)
    {
        if (node->img_format == IMAGE_H264 || node->img_format == IMAGE_H265)
        {
            //
        }
        else
        {
            if (!list_empty(&node->ready))
            {
                list_for_each_safe(pos, n, &node->ready)
                {
                    tmp = list_entry(pos, frame_node_t, list);
                    if (tmp != NULL && tmp->free_mask == tmp->read_mask)
                    {
                        f_node = tmp;
                        list_del(pos);
                        break;
                    }
                }
            }
        }
    }

    if (f_node)
    {
        if (f_node->frame == NULL)
        {
            if (node->img_format != IMAGE_YUV)
            {
                f_node->frame = frame_buffer_encode_malloc(size);
            }
            else
            {
                f_node->frame = frame_buffer_display_malloc(size);
            }
            if (f_node->frame == NULL)
            {
                LOGW("%s %d malloc fail, img_format:%d\n", __func__, __LINE__, node->img_format);
                list_add_tail(&f_node->list, &node->free);
                f_node = NULL;
            }
        }
    }

    fb_exit_critical(flag);

    if (f_node == NULL)
    {
        return NULL;
    }

    LOGV("%s, %p %p %p\n", __func__, f_node, f_node->frame, f_node->frame->frame);

    f_node->frame->type = 0;
    f_node->frame->fmt = 0;
    f_node->frame->crc = 0;
    f_node->frame->timestamp = 0;
    f_node->frame->width = 0;
    f_node->frame->height = 0;
    f_node->frame->length = 0;
    f_node->frame->sequence = 0;
    f_node->frame->h264_type = 0;
    f_node->frame->cb = NULL;
    return f_node->frame;
}

bk_err_t frame_buffer_fb_free(frame_list_node_t *node, frame_buffer_t *frame)
{
    frame_node_t *f_node = NULL;
    uint8_t index = 0;

    if (node == NULL || frame == NULL)
    {
        LOGW("%s, %d node/frame NULL\n", __func__, __LINE__);
        return BK_OK;
    }

    uint32_t flag = fb_enter_critical();

    for (index = 0; index < node->count; index++)
    {
        if (frame == node->node_list[index]->frame)
        {
            break;
        }
    }

    if (index == node->count)
    {
        LOGE("%s, %d not find this frame in frame_list\n", __func__, __LINE__);
        goto out;
    }

    f_node = node->node_list[index];

    LOGV("%s %p %p %p\n", __func__, f_node, f_node->frame, frame);

    f_node->free_mask = 0;
    f_node->read_mask = 0;
    if (node->img_format == IMAGE_YUV)
    {
        frame_buffer_display_free(frame);
    }
    else
    {
        frame_buffer_encode_free(frame);
    }
    f_node->frame = NULL;
    list_add_tail(&f_node->list, &node->free);

out:

    fb_exit_critical(flag);

    return BK_OK;
}

bk_err_t frame_buffer_fb_push(frame_list_node_t *node, frame_buffer_t *frame)
{
    frame_node_t *f_node = NULL;
    uint8_t index = 0;

    if (node == NULL || frame == NULL)
    {
        LOGW("%s, %d node/frame NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    uint32_t flag = fb_enter_critical();

    for (index = 0; index < node->count; index++)
    {
        if (frame == node->node_list[index]->frame)
        {
            break;
        }
    }

    if (index == node->count)
    {
        fb_exit_critical(flag);
        LOGE("%s, %d not find this frame in frame_list\n", __func__, __LINE__);
        return BK_OK;
    }

    f_node = node->node_list[index];

    LOGV("%s, %p %p %p %d\n", __func__, f_node, frame, frame->frame, index);

    f_node->free_mask = 0;
    f_node->read_mask = 0;
    list_add_tail(&f_node->list, &node->ready);
    fb_exit_critical(flag);
    if (node->trigger)
    {
        node->trigger = false;
        rtos_set_semaphore(&node->read_sem);
    }

    return BK_OK;
}

frame_buffer_t *frame_buffer_fb_pop(frame_list_node_t *node, uint32_t timeout)
{
    LIST_HEADER_T *pos, *n;
    frame_node_t *tmp = NULL, *f_node = NULL;

    if (node == NULL || node->register_mask)
    {
        LOGW("%s, node:%p node null or mask been register\n", __func__, node);
        return NULL;
    }

    uint32_t flag = fb_enter_critical();

    if (!list_empty(&node->ready))
    {
        list_for_each_safe(pos, n, &node->ready)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL)
            {
                f_node = tmp;
                list_del(pos);
                break;
            }
        }
    }

    fb_exit_critical(flag);

    if (f_node)
    {
        return f_node->frame;
    }
    else
    {
        rtos_get_semaphore(&node->read_sem, timeout);
        return NULL;
    }
}

bk_err_t frame_buffer_fb_register(frame_list_node_t *node, frame_module_t module)
{
    if (node == NULL)
    {
        LOGW("%s, %d node/frame NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        LOGV("%s, %p, %d, %d\n", __func__, node, node->register_mask, module);
    }

    uint32_t flag = fb_enter_critical();

    if ((node->register_mask & INDEX_MASK(module)) == 0)
    {
        node->register_mask |= INDEX_MASK(module);
    }

    fb_exit_critical(flag);
    LOGD("%s, %p, %d, %d\n", __func__, node, node->register_mask, module);

    return BK_OK;
}

bk_err_t frame_buffer_fb_deregister(frame_list_node_t *node, frame_module_t module)
{
    LIST_HEADER_T *pos, *n;
    frame_node_t *tmp = NULL;
    if (node == NULL)
    {
        LOGW("%s, %d node/frame NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        LOGV("%s, %p, %d, %d\n", __func__, node, node->register_mask, module);
    }

    uint32_t flag = fb_enter_critical();

    if (node->register_mask & INDEX_MASK(module))
    {
        node->register_mask &= INDEX_UNMASK(module);
        rtos_set_semaphore(&node->read_sem);
    }

    if (node->register_mask == 0 && !list_empty(&node->ready))
    {
        list_for_each_safe(pos, n, &node->ready)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL)
            {
                if (tmp->frame)
                {
                    frame_buffer_encode_free(tmp->frame);
                    tmp->frame = NULL;
                }
                list_del(pos);
                list_add_tail(&tmp->list, &node->free);
            }
        }
    }

    fb_exit_critical(flag);

    if (node->register_mask == 0 && node->deinit)
    {
        node->deinit = false;
        frame_buffer_list_node_deinit(node);
    }

    LOGD("%s, %p, %d, %d\n", __func__, node, node->register_mask, module);

    return BK_OK;
}

static frame_buffer_t *frame_buffer_fb_read_pop(LIST_HEADER_T *list, frame_module_t module)
{
    LIST_HEADER_T *pos, *n;
    frame_node_t *tmp = NULL, *f_node = NULL;

    if (!list_empty(list))
    {
        list_for_each_safe(pos, n, list)
        {
            tmp = list_entry(pos, frame_node_t, list);
            if (tmp != NULL && ((tmp->read_mask & INDEX_MASK(module)) == 0))
            {
                f_node = tmp;
                break;
            }
        }
    }

    if (f_node)
    {
        f_node->read_mask |= INDEX_MASK(module);
    }
    else
    {
        return NULL;
    }

    return f_node->frame;
}

frame_buffer_t *frame_buffer_fb_read(frame_list_node_t *node, frame_module_t module, uint32_t timeout)
{
    frame_buffer_t *frame = NULL;
    frame_node_t *tmp = NULL;
    LIST_HEADER_T *pos, *n;

    if (node == NULL)
    {
        LOGW("%s, %d null ptr\n", __func__, node);
        return frame;
    }

    uint32_t flag = fb_enter_critical();

    if ((node->register_mask & INDEX_MASK(module)) == 0)
    {
        LOGE("%s, %d, read mode not register\n", __func__, module);
        goto out;
    }

    if (node->invalid)
    {
        // clear all ready node
//        node->invalid = false;
        if (!list_empty(&node->ready))
        {
            list_for_each_safe(pos, n, &node->ready)
            {
                tmp = list_entry(pos, frame_node_t, list);
                if (tmp != NULL)
                {
                    tmp->free_mask = 0;
                    tmp->read_mask = 0;
                    list_del(pos);
                    list_add_tail(&tmp->list, &node->free);
                    tmp = NULL;
                }
            }
        }
    }

    frame = frame_buffer_fb_read_pop(&node->ready, module);

    if (frame == NULL)
    {
        node->trigger = true;
        fb_exit_critical(flag);
        if (BK_OK != rtos_get_semaphore(&node->read_sem, timeout))
        {
            LOGV("%s, timeout:%dms, module:%d\n", __func__, timeout, module);
        }

        flag = fb_enter_critical();
        frame = frame_buffer_fb_read_pop(&node->ready, module);
    }

out:
    fb_exit_critical(flag);
    return frame;
}

frame_list_node_t *frame_buffer_fb_get_stream_by_frame(frame_buffer_t *frame)
{
    frame_list_t *tmp = NULL;
    frame_list_node_t *node = NULL;
    LIST_HEADER_T *pos, *n;
    uint8_t index = 0;

    if (frame == NULL)
    {
        LOGW("%s, frame null\n", __func__);
        return NULL;
    }

    uint32_t flag = fb_enter_critical();
    if (!list_empty(&fb_info->list))
    {
        list_for_each_safe(pos, n, &fb_info->list)
        {
            tmp = list_entry(pos, frame_list_t, list);
            if (tmp != NULL)
            {
                for (index = 0; index < tmp->node->count; index++)
                {
                    if (frame == tmp->node->node_list[index]->frame)
                    {
                        node = tmp->node;
                        break;
                    }
                }
            }

            if (node)
            {
                break;
            }
        }
    }

    fb_exit_critical(flag);

    return node;
}

void frame_buffer_fb_read_free(frame_list_node_t *node, frame_buffer_t *frame, frame_module_t mode)
{
    uint8_t index = 0;
    frame_node_t *f_node = NULL;
    frame_list_node_t *curr_node = NULL;

    if (node == NULL || frame == NULL)
    {
        LOGW("%s, %d null ptr\n", __func__, node);
        return;
    }

#if FB_OPTIMIZER_ENABLE

    curr_node = frame_buffer_fb_get_stream_by_frame(frame);

    if (curr_node == NULL)
    {
        LOGW("%s, %d null ptr\n", __func__, curr_node);
        return;
    }

#else
    curr_node = node;
#endif

    uint32_t flag = fb_enter_critical();

    for (index = 0; index < curr_node->count; index++)
    {
        if (frame == curr_node->node_list[index]->frame)
        {
            break;
        }
    }

    if (index == curr_node->count)
    {
        LOGE("%s, %d not find this frame in frame_list\n", __func__, __LINE__);
        goto out;
    }

    f_node = curr_node->node_list[index];

    if ((node->register_mask & INDEX_MASK(mode)) == 0)
    {
        LOGW("%s, %d, %d not register...\n", __func__, node->register_mask, mode);
    }

    if (f_node->free_mask & INDEX_MASK(mode))
    {
        LOGE("%s %p, refree: %p, %d\n", __func__, node, frame, mode);
        goto out;
    }
    else
    {
        f_node->free_mask |= INDEX_MASK(mode);
    }

    if (f_node->read_mask)
    {
        if ((f_node->read_mask == f_node->free_mask)
            && ((f_node->free_mask & curr_node->register_mask) == curr_node->register_mask)) // this judge may case bug
        {
            if (BK_OK != frame_buffer_fb_node_remove(f_node, &curr_node->ready))
            {
                LOGE("%s remove failed\n", __func__);
                goto out;
            }

            f_node->free_mask = 0;
            f_node->read_mask = 0;
            if (curr_node->img_format == IMAGE_YUV)
            {
                frame_buffer_display_free(frame);
            }
            else
            {
                frame_buffer_encode_free(frame);
            }
            f_node->frame = NULL;

            list_add_tail(&f_node->list, &curr_node->free);
        }
        else
        {
            if (f_node->read_mask == f_node->free_mask)
            {
                LOGV("%s, mask:%x, read-free:%x-%x\n", __func__, curr_node->register_mask, f_node->read_mask, f_node->free_mask);
            }
        }

//        if (curr_node->trigger)
//        {
//            curr_node->trigger = false;
//            rtos_set_semaphore(&curr_node->read_sem);
//        }
    }
    else
    {
        /* safte check */
        LOGW("%s, %d-%d frame not read\n", __func__, f_node->free_mask, curr_node->register_mask);
    }

out:

	fb_exit_critical(flag);
}

