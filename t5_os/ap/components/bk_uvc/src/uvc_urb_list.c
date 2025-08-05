// Copyright 2023-2024 Beken
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
#include "media_utils.h"
#include "uvc_urb_list.h"
#include <driver/media_types.h>
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif
#define TAG "uvc_urb"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t urb_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t urb_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&urb_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void urb_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&urb_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

uvc_urb_list_t g_uvc_list = {0};

bk_err_t uvc_camera_urb_list_init(void)
{
    int ret = BK_OK;

    uvc_urb_list_t *mem_list = &g_uvc_list;
    uint32_t offset0 = 0, offset1 = 0;

    if (mem_list->enable)
    {
        LOGV("%s, urb list already init\r\n", __func__);
        return ret;
    }

    INIT_LIST_HEAD(&mem_list->free);
    INIT_LIST_HEAD(&mem_list->ready);

    ret = rtos_init_semaphore(&mem_list->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s sem init fail\n", __func__);
        return ret;
    }

    mem_list->count = CONFIG_UVC_URB_NUM;
    mem_list->size = CONFIG_UVC_URB_NUM * CONFIG_UVC_NUM_PACKET_PER_URB * UVC_MAX_PACKET_SIZE;

    mem_list->buffer = (uint8_t *)media_malloc(mem_list->size);

    LOGV("%s, mem_list->buffer:%p, size:%d\r\n", __func__, mem_list->buffer, mem_list->size);

    if (mem_list->buffer == NULL)
    {
        LOGE("%s, malloc buffer error\r\n", __func__);
        BK_ASSERT(0);
        ret = BK_FAIL;
    }

    for (uint8_t i = 0; i < mem_list->count; i++)
    {
        uvc_urb_node_t *node = (uvc_urb_node_t *)media_malloc(sizeof(uvc_urb_node_t)
                               + sizeof(struct usbh_iso_frame_packet) * CONFIG_UVC_NUM_PACKET_PER_URB);

        if (node == NULL)
        {
            LOGE("%s os_malloc node failed\n", __func__);
            BK_ASSERT(0);
            return BK_FAIL;
        }

        os_memset(node, 0, sizeof(uvc_urb_node_t)
                  + sizeof(struct usbh_iso_frame_packet) * CONFIG_UVC_NUM_PACKET_PER_URB);

        node->urb.num_of_iso_packets = CONFIG_UVC_NUM_PACKET_PER_URB;
        node->urb.transfer_buffer = mem_list->buffer + offset0;
        node->urb.transfer_buffer_length = UVC_MAX_PACKET_SIZE * node->urb.num_of_iso_packets;
        offset0 += node->urb.transfer_buffer_length;

        LOGV("node(%d): transfer_buffer:%p, transfer_buffer_length:%d\r\n",
             i,
             node->urb.transfer_buffer,
             node->urb.transfer_buffer_length);

        os_memset(node->urb.iso_packet, 0, sizeof(struct usbh_iso_frame_packet) * CONFIG_UVC_NUM_PACKET_PER_URB);

        offset1 = 0;

        for (uint8_t j = 0; j < node->urb.num_of_iso_packets; j++)
        {
            node->urb.iso_packet[j].transfer_buffer = node->urb.transfer_buffer + offset1;
            node->urb.iso_packet[j].transfer_buffer_length = UVC_MAX_PACKET_SIZE;
            node->urb.iso_packet[j].actual_length = 0;
            node->urb.iso_packet[j].errorcode = 0;
            offset1 += UVC_MAX_PACKET_SIZE;

            LOGV("iso_packet(%d): transfer_buffer:%p, transfer_buffer_length:%d\r\n",
                 j, node->urb.iso_packet[j].transfer_buffer,
                 node->urb.iso_packet[j].transfer_buffer_length);
        }

        LOGV("%s, %d, %p\r\n", __func__, __LINE__, &node->urb);

        list_add_tail(&node->list, &mem_list->free);
    }

    mem_list->enable = true;

    return ret;
}

bk_err_t uvc_camera_urb_list_deinit(void)
{
    int ret = BK_OK;
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL;
    LIST_HEADER_T *pos, *n;

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return ret;
    }

    if (!list_empty(&mem_list->free))
    {
        list_for_each_safe(pos, n, &mem_list->free)
        {
            tmp = list_entry(pos, uvc_urb_node_t, list);
            LOGV("free list: %p\n", tmp);
            if (tmp != NULL)
            {
                list_del(pos);
                os_free(tmp);
                tmp = NULL;
            }
        }

        INIT_LIST_HEAD(&mem_list->free);
    }

    if (!list_empty(&mem_list->ready))
    {
        list_for_each_safe(pos, n, &mem_list->ready)
        {
            LOGV("ready list: %p\n", tmp);
            tmp = list_entry(pos, uvc_urb_node_t, list);
            if (tmp != NULL)
            {
                list_del(pos);
                os_free(tmp);
                tmp = NULL;
            }
        }

        INIT_LIST_HEAD(&mem_list->ready);
    }

    os_free(mem_list->buffer);
    mem_list->buffer = NULL;

    mem_list->enable = false;

    rtos_deinit_semaphore(&mem_list->sem);

    LOGD("uvc urb list deinit finish\n");

    return ret;
}

void uvc_camera_urb_list_clear(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL;
    LIST_HEADER_T *pos, *n;

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return;
    }

    uint32_t flag = urb_enter_critical();

    list_for_each_safe(pos, n, &mem_list->ready)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            list_del(pos);
            list_add_tail(&tmp->list, &mem_list->free);
        }
    }

    urb_exit_critical(flag);
}

struct usbh_urb *uvc_camera_urb_malloc(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL, *node = NULL;
    LIST_HEADER_T *pos, *n;

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return NULL;
    }

    uint32_t flag = urb_enter_critical();

    list_for_each_safe(pos, n, &mem_list->free)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            node = tmp;
            list_del(pos);
            break;
        }
    }

    urb_exit_critical(flag);

    if (node == NULL)
    {
        LOGE("%s failed\n", __func__);
        return NULL;
    }

    node->urb.actual_length = 0;

    for (uint8_t j = 0; j < node->urb.num_of_iso_packets; j++)
    {
        node->urb.iso_packet[j].actual_length = 0;
        node->urb.iso_packet[j].errorcode = 0;
    }

    LOGV("%s, node:%p, %p\r\n", __func__, node, &node->urb);
    return &node->urb;
}

void uvc_camera_urb_free(struct usbh_urb *urb)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *node = NULL;

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return;
    }

    uint32_t flag = urb_enter_critical();

    node = list_entry(urb, uvc_urb_node_t, urb);
    urb->pipe = NULL;
    list_add_tail(&node->list, &mem_list->free);

    urb_exit_critical(flag);
}

void uvc_camera_urb_push(struct usbh_urb *urb)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *node = list_entry(urb, uvc_urb_node_t, urb);

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return;
    }

    uint32_t flag = urb_enter_critical();

    list_add_tail(&node->list, &mem_list->ready);

    urb_exit_critical(flag);

    rtos_set_semaphore(&mem_list->sem);
}

struct usbh_urb *uvc_camera_urb_pop(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL, *node = NULL;
    LIST_HEADER_T *pos, *n;

    mem_list = &g_uvc_list;

    if (mem_list->enable == false)
    {
        LOGE("%s already deinit\n", __func__);
        return NULL;
    }

    uint32_t flag = urb_enter_critical();

    list_for_each_safe(pos, n, &mem_list->ready)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            node = tmp;
            list_del(pos);
            break;
        }
    }

    urb_exit_critical(flag);

    if (node == NULL)
    {
        if (rtos_get_semaphore(&mem_list->sem, 100) != BK_OK)
        {
            LOGV("%s, get node timeout, do not urb push!\r\n", __func__);
        }
        return NULL;
    }

    return &node->urb;
}
