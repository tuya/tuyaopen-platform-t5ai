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
#include "stdint.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>


#define TAG   "AUD_EVT"

#define LISTEN_DEBUG(x) debug_listen_lists(x, __LINE__, __func__)

typedef struct audio_event_iface_item
{
    STAILQ_ENTRY(audio_event_iface_item)    next;
    QueueHandle_t                           queue;
    int                                     queue_size;
    int                                     mark_to_remove;
} audio_event_iface_item_t;

typedef STAILQ_HEAD(audio_event_iface_list, audio_event_iface_item) audio_event_iface_list_t;

/**
 * Audio event structure
 */
struct audio_event_iface
{
    QueueHandle_t               internal_queue;
    QueueHandle_t               external_queue;
    QueueSetHandle_t            queue_set;
    int                         internal_queue_size;
    int                         external_queue_size;
    int                         queue_set_size;
    audio_event_iface_list_t    listening_queues;
    void                        *context;
    on_event_iface_func         on_cmd;
    int                         wait_time;
    int                         type;
};


static void debug_listen_lists(audio_event_iface_handle_t listen, int line, const char *func)
{
    audio_event_iface_item_t *item;
    BK_LOGV(TAG, "FUNC:%s, LINE:%d \n", func, line);

    STAILQ_FOREACH(item, &listen->listening_queues, next)
    {
        BK_LOGV(TAG, "queue:%p, size:%d \n", item->queue, item->queue_size);
    }
}

audio_event_iface_handle_t audio_event_iface_init(audio_event_iface_cfg_t *config)
{
    audio_event_iface_handle_t evt = audio_calloc(1, sizeof(struct audio_event_iface));
    AUDIO_MEM_CHECK(TAG, evt, return NULL);
    evt->queue_set_size   = config->queue_set_size;
    evt->internal_queue_size = config->internal_queue_size;
    evt->external_queue_size = config->external_queue_size;
    evt->context = config->context;
    evt->on_cmd = config->on_cmd;
    evt->type = config->type;
    if (evt->queue_set_size)
    {
        evt->queue_set = xQueueCreateSet(evt->queue_set_size);
    }
    if (evt->internal_queue_size)
    {
        evt->internal_queue = xQueueCreate(evt->internal_queue_size, sizeof(audio_event_iface_msg_t));
        AUDIO_MEM_CHECK(TAG, evt->internal_queue, goto _event_iface_init_failed);
    }
    if (evt->external_queue_size)
    {
        evt->external_queue = xQueueCreate(evt->external_queue_size, sizeof(audio_event_iface_msg_t));
        AUDIO_MEM_CHECK(TAG, evt->external_queue, goto _event_iface_init_failed);
    }
    else
    {
        BK_LOGV(TAG, "This emiiter have no queue set,%p \n", evt);
    }

    STAILQ_INIT(&evt->listening_queues);
    return evt;
_event_iface_init_failed:
    if (evt->internal_queue)
    {
        vQueueDelete(evt->internal_queue);
    }
    if (evt->external_queue)
    {
        vQueueDelete(evt->external_queue);
    }
    return NULL;
}

static bk_err_t audio_event_iface_cleanup_listener(audio_event_iface_handle_t listen)
{
    audio_event_iface_item_t *item, *tmp;
    audio_event_iface_discard(listen);
    STAILQ_FOREACH_SAFE(item, &listen->listening_queues, next, tmp)
    {
        audio_event_iface_msg_t dummy;
        while (audio_event_iface_read(listen, &dummy, 0) == BK_OK);
        while (listen->queue_set && (xQueueRemoveFromSet(item->queue, listen->queue_set) != pdPASS))
        {
            BK_LOGD(TAG, "Error remove listener,%p \n", item->queue);
            while (audio_event_iface_read(listen, &dummy, 0) == BK_OK);
        }
    }
    if (listen->queue_set)
    {
        vQueueDelete(listen->queue_set);
        listen->queue_set = NULL;
    }
    return BK_OK;
}

static bk_err_t audio_event_iface_update_listener(audio_event_iface_handle_t listen)
{
    audio_event_iface_item_t *item;
    int queue_size = 0;
    STAILQ_FOREACH(item, &listen->listening_queues, next)
    {
        queue_size += item->queue_size;
    }
    if (queue_size)
    {
        listen->queue_set = xQueueCreateSet(queue_size);
    }
    STAILQ_FOREACH(item, &listen->listening_queues, next)
    {
        if (item->queue)
        {
            audio_event_iface_msg_t dummy;
            while (xQueueReceive(item->queue, &dummy, 0) == pdTRUE);
        }
        if (listen->queue_set && item->queue && xQueueAddToSet(item->queue, listen->queue_set) != pdPASS)
        {
            BK_LOGE(TAG, "Error add queue items to queue set \n");
            return BK_FAIL;
        }
    }
    LISTEN_DEBUG(listen);
    return BK_OK;
}

bk_err_t audio_event_iface_read(audio_event_iface_handle_t evt, audio_event_iface_msg_t *msg, TickType_t wait_time)
{
    if (evt->queue_set)
    {
        QueueSetMemberHandle_t active_queue;
        active_queue = xQueueSelectFromSet(evt->queue_set, wait_time);
        if (active_queue)
        {
            if (xQueueReceive(active_queue, msg, 0) == pdTRUE)
            {
                return BK_OK;
            }
        }
    }
    return BK_FAIL;
}

bk_err_t audio_event_iface_destroy(audio_event_iface_handle_t evt)
{
    audio_event_iface_cleanup_listener(evt);
    audio_event_iface_item_t *item, *tmp;
    STAILQ_FOREACH_SAFE(item, &evt->listening_queues, next, tmp)
    {
        STAILQ_REMOVE(&evt->listening_queues, item, audio_event_iface_item, next);
        audio_free(item);
    }
    if (evt->internal_queue)
    {
        audio_event_iface_set_cmd_waiting_timeout(evt, 0);
        vQueueDelete(evt->internal_queue);
    }
    if (evt->external_queue)
    {
        vQueueDelete(evt->external_queue);
    }
    if (evt->queue_set)
    {
        vQueueDelete(evt->queue_set);
    }
    audio_free(evt);
    return BK_OK;
}

bk_err_t audio_event_iface_set_listener(audio_event_iface_handle_t evt, audio_event_iface_handle_t listener)
{
    if ((NULL == evt->external_queue)
        || (0 == evt->external_queue_size))
    {
        return BK_ERR_ADK_INVALID_ARG;
    }
    audio_event_iface_item_t *item = audio_calloc(1, sizeof(audio_event_iface_item_t));
    AUDIO_MEM_CHECK(TAG, item, return BK_ERR_ADK_TIMEOUT);

    if (audio_event_iface_cleanup_listener(listener) != BK_OK)
    {
        AUDIO_ERROR(TAG, "Error cleanup listener");
        return BK_FAIL;
    }
    item->queue = evt->external_queue;
    item->queue_size = evt->external_queue_size;
    STAILQ_INSERT_TAIL(&listener->listening_queues, item, next);
    return audio_event_iface_update_listener(listener);
}

bk_err_t audio_event_iface_set_msg_listener(audio_event_iface_handle_t evt, audio_event_iface_handle_t listener)
{
    if ((NULL == evt->internal_queue)
        || (0 == evt->internal_queue_size))
    {
        return BK_ERR_ADK_INVALID_ARG;
    }
    audio_event_iface_item_t *item = audio_calloc(1, sizeof(audio_event_iface_item_t));
    AUDIO_MEM_CHECK(TAG, item, return BK_ERR_ADK_TIMEOUT);
    if (audio_event_iface_cleanup_listener(listener) != BK_OK)
    {
        AUDIO_ERROR(TAG, "Error cleanup listener");
        return BK_FAIL;
    }
    item->queue = evt->internal_queue;
    item->queue_size = evt->internal_queue_size;
    STAILQ_INSERT_TAIL(&listener->listening_queues, item, next);
    return audio_event_iface_update_listener(listener);
}

bk_err_t audio_event_iface_remove_listener(audio_event_iface_handle_t listen, audio_event_iface_handle_t evt)
{
    if ((NULL == evt->external_queue)
        || (0 == evt->external_queue_size))
    {
        return BK_ERR_ADK_INVALID_ARG;
    }
    audio_event_iface_item_t *item = NULL, *tmp = NULL;
    if (audio_event_iface_cleanup_listener(listen) != BK_OK)
    {
        return BK_FAIL;
    }
    STAILQ_FOREACH_SAFE(item, &listen->listening_queues, next, tmp)
    {
        if (item && evt->external_queue == item->queue)
        {
            STAILQ_REMOVE(&listen->listening_queues, item, audio_event_iface_item, next);
            audio_free(item);
        }
    }
    return audio_event_iface_update_listener(listen);
}

bk_err_t audio_event_iface_set_cmd_waiting_timeout(audio_event_iface_handle_t evt, TickType_t wait_time)
{
    evt->wait_time = wait_time;
    return BK_OK;
}

bk_err_t audio_event_iface_waiting_cmd_msg(audio_event_iface_handle_t evt)
{
    audio_event_iface_msg_t msg;
    if (evt->internal_queue && (xQueueReceive(evt->internal_queue, (void *)&msg, evt->wait_time) == pdTRUE))
    {
        if (evt->on_cmd)
        {
            return evt->on_cmd((void *)&msg, evt->context);
        }
    }
    return BK_OK;
}

bk_err_t audio_event_iface_cmd(audio_event_iface_handle_t evt, audio_event_iface_msg_t *msg)
{
    if (evt->internal_queue && (xQueueSend(evt->internal_queue, (void *)msg, 0) != pdPASS))
    {
        BK_LOGE(TAG, "There are no space to dispatch queue \n");
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t audio_event_iface_cmd_from_isr(audio_event_iface_handle_t evt, audio_event_iface_msg_t *msg)
{
    if (evt->internal_queue && (xQueueSendFromISR(evt->internal_queue, (void *)msg, 0) != pdPASS))
    {
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t audio_event_iface_sendout(audio_event_iface_handle_t evt, audio_event_iface_msg_t *msg)
{
    if (evt->external_queue)
    {
        if (xQueueSend(evt->external_queue, (void *)msg, 0) != pdPASS)
        {
            BK_LOGD(TAG, "There is no space in external queue \n");
            return BK_FAIL;
        }
    }
    return BK_OK;
}

bk_err_t audio_event_iface_discard(audio_event_iface_handle_t evt)
{
    audio_event_iface_msg_t msg;
    if (evt->external_queue && evt->external_queue_size)
    {
        while (xQueueReceive(evt->external_queue, &msg, 0) == pdTRUE);
    }
    if (evt->internal_queue && evt->internal_queue_size)
    {
        while (xQueueReceive(evt->internal_queue, &msg, 0) == pdTRUE);
    }
    if (evt->queue_set && evt->queue_set_size)
    {
        while (audio_event_iface_read(evt, &msg, 0) == BK_OK);
    }
    return BK_OK;
}

bk_err_t audio_event_iface_listen(audio_event_iface_handle_t evt, audio_event_iface_msg_t *msg, TickType_t wait_time)
{
    if (!evt)
    {
        return BK_FAIL;
    }
    if (audio_event_iface_read(evt, msg, wait_time) != BK_OK)
    {
        return BK_FAIL;
    }
    return BK_OK;
}

QueueHandle_t audio_event_iface_get_queue_handle(audio_event_iface_handle_t evt)
{
    if (!evt)
    {
        return NULL;
    }
    return evt->external_queue;
}

QueueHandle_t audio_event_iface_get_msg_queue_handle(audio_event_iface_handle_t evt)
{
    if (!evt)
    {
        return NULL;
    }
    return evt->internal_queue;
}
