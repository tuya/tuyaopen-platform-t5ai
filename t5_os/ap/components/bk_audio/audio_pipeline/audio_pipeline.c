// Copyright 2022-2023 Beken
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

#include <string.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/fb_port.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_pipeline/cb_port.h>


#define TAG  "AUD_PIPE"

#define PIPELINE_DEBUG(x) debug_pipeline_lists(x, __LINE__, __func__)

typedef struct port_item
{
    STAILQ_ENTRY(port_item)     next;
    audio_port_handle_t         port;
    audio_element_handle_t      host_el;
    bool                        linked;
    bool                        kept_ctx;
} port_item_t;

typedef STAILQ_HEAD(port_list, port_item) port_list_t;

typedef struct audio_element_item
{
    STAILQ_ENTRY(audio_element_item) next;
    audio_element_handle_t           el;
    bool                             linked;
    bool                             kept_ctx;
    audio_element_status_t           el_state;
} audio_element_item_t;

typedef STAILQ_HEAD(audio_element_list, audio_element_item) audio_element_list_t;

struct audio_pipeline
{
    audio_element_list_t        el_list;
    port_list_t                 port_list;
    audio_element_state_t       state;
    xSemaphoreHandle            lock;
    bool                        linked;
    audio_event_iface_handle_t  listener;
};

static audio_element_item_t *audio_pipeline_get_el_item_by_tag(audio_pipeline_handle_t pipeline, const char *tag)
{
    audio_element_item_t *item;
    STAILQ_FOREACH(item, &pipeline->el_list, next)
    {
        char *el_tag = audio_element_get_tag(item->el);
        if (el_tag && strcasecmp(el_tag, tag) == 0)
        {
            return item;
        }
    }
    return NULL;
}

static audio_element_item_t *audio_pipeline_get_el_item_by_handle(audio_pipeline_handle_t pipeline, audio_element_handle_t el)
{
    audio_element_item_t *item;
    STAILQ_FOREACH(item, &pipeline->el_list, next)
    {
        if (item->el == el)
        {
            return item;
        }
    }
    return NULL;
}

bk_err_t audio_pipeline_change_state(audio_pipeline_handle_t pipeline, audio_element_state_t new_state)
{
    pipeline->state = new_state;
    return BK_OK;
}

static void add_rb_to_audio_pipeline(audio_pipeline_handle_t pipeline, audio_port_handle_t port, audio_element_handle_t host_el)
{
    port_item_t *port_item = (port_item_t *)audio_calloc(1, sizeof(port_item_t));
    AUDIO_MEM_CHECK(TAG, port_item, return);
    port_item->port = port;
    port_item->linked = true;
    port_item->kept_ctx = false;
    port_item->host_el = host_el;
    STAILQ_INSERT_TAIL(&pipeline->port_list, port_item, next);
}

static void debug_pipeline_lists(audio_pipeline_handle_t pipeline, int line, const char *func)
{
    audio_element_item_t *el_item, *el_tmp;
    port_item_t *port_item, *tmp;
    BK_LOGD(TAG, "FUNC:%s, LINE:%d \n", func, line);
    STAILQ_FOREACH_SAFE(el_item, &pipeline->el_list, next, el_tmp)
    {
        BK_LOGD(TAG, "el-list: linked:%d, kept:%d, el:%p, %16s, in_port:%p, out_port:%p \n",
                el_item->linked, el_item->kept_ctx,
                el_item->el, audio_element_get_tag(el_item->el),
                audio_element_get_input_port(el_item->el),
                audio_element_get_output_port(el_item->el));
    }
    STAILQ_FOREACH_SAFE(port_item, &pipeline->port_list, next, tmp)
    {
        BK_LOGD(TAG, "port-list: linked:%d, kept:%d, port:%p, host_el:%p, %16s \n", port_item->linked, port_item->kept_ctx,
                port_item->port, port_item->host_el,
                port_item->host_el != NULL ? audio_element_get_tag(port_item->host_el) : "NULL");
    }
}

audio_element_handle_t audio_pipeline_get_el_by_tag(audio_pipeline_handle_t pipeline, const char *tag)
{
    if (tag == NULL || pipeline == NULL)
    {
        BK_LOGE(TAG, "Invalid parameters, tag:%p, p:%p \n", tag, pipeline);
        return NULL;
    }
    audio_element_item_t *item;
    STAILQ_FOREACH(item, &pipeline->el_list, next)
    {
        char *el_tag = audio_element_get_tag(item->el);
        BK_LOGV(TAG, "Get_el_by_tag, el:%p, kept:%d, linked:%d el-tag:%16s, in_tag:%s \n",
                item->el, item->kept_ctx, item->linked, item->el != NULL ? audio_element_get_tag(item->el) : "NULL", tag);
        if (item->kept_ctx)
        {
            continue;
        }
        if (el_tag && strcasecmp(el_tag, tag) == 0)
        {
            return item->el;
        }
    }
    return NULL;
}

bk_err_t audio_pipeline_set_listener(audio_pipeline_handle_t pipeline, audio_event_iface_handle_t listener)
{
    audio_element_item_t *el_item;
    if (pipeline->listener)
    {
        audio_pipeline_remove_listener(pipeline);
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked == false)
        {
            continue;
        }
        if (audio_element_msg_set_listener(el_item->el, listener) != BK_OK)
        {
            BK_LOGE(TAG, "Error register event with: %s \n", (char *)audio_element_get_tag(el_item->el));
            return BK_FAIL;
        }
    }
    pipeline->listener = listener;
    return BK_OK;
}

bk_err_t audio_pipeline_remove_listener(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    if (pipeline->listener == NULL)
    {
        BK_LOGD(TAG, "There are no listener registered \n");
        return BK_FAIL;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked == false)
        {
            continue;
        }
        if (audio_element_msg_remove_listener(el_item->el, pipeline->listener) != BK_OK)
        {
            BK_LOGE(TAG, "Error unregister event with: %s \n", audio_element_get_tag(el_item->el));
            return BK_FAIL;
        }
    }
    pipeline->listener = NULL;
    return BK_OK;
}

audio_pipeline_handle_t audio_pipeline_init(audio_pipeline_cfg_t *config)
{
    audio_pipeline_handle_t pipeline;
    bool _success =
        (
            (pipeline       = audio_calloc(1, sizeof(struct audio_pipeline)))   &&
            (pipeline->lock = xSemaphoreCreateMutex())
        );

    AUDIO_MEM_CHECK(TAG, _success, return NULL);
    STAILQ_INIT(&pipeline->el_list);
    STAILQ_INIT(&pipeline->port_list);

    pipeline->state = AEL_STATE_INIT;
    return pipeline;
}

bk_err_t audio_pipeline_deinit(audio_pipeline_handle_t pipeline)
{
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unlink(pipeline);
    audio_element_item_t *el_item, *tmp;
    STAILQ_FOREACH_SAFE(el_item, &pipeline->el_list, next, tmp)
    {
        BK_LOGV(TAG, "[%16s]-[%p]element instance has been deleted \n", audio_element_get_tag(el_item->el), el_item->el);
        audio_element_deinit(el_item->el);
        audio_pipeline_unregister(pipeline, el_item->el);
    }
    vSemaphoreDelete(pipeline->lock);
    audio_free(pipeline);
    return BK_OK;
}

bk_err_t audio_pipeline_register(audio_pipeline_handle_t pipeline, audio_element_handle_t el, const char *name)
{
    audio_pipeline_unregister(pipeline, el);
    if (name)
    {
        audio_element_set_tag(el, name);
    }
    audio_element_item_t *el_item = audio_calloc(1, sizeof(audio_element_item_t));

    AUDIO_MEM_CHECK(TAG, el_item, return BK_ERR_ADK_NO_MEM);
    el_item->el = el;
    el_item->linked = false;
    STAILQ_INSERT_TAIL(&pipeline->el_list, el_item, next);
    return BK_OK;
}

bk_err_t audio_pipeline_unregister(audio_pipeline_handle_t pipeline, audio_element_handle_t el)
{
    audio_element_item_t *el_item, *tmp;
    STAILQ_FOREACH_SAFE(el_item, &pipeline->el_list, next, tmp)
    {
        if (el_item->el == el)
        {
            STAILQ_REMOVE(&pipeline->el_list, el_item, audio_element_item, next);
            audio_free(el_item);
            return BK_OK;
        }
    }
    return BK_FAIL;
}

bk_err_t audio_pipeline_resume(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    bool wait_first_el = true;
    bk_err_t ret = BK_OK;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        BK_LOGV(TAG, "resume,linked:%d, state:%d,[%s-%p] \n", el_item->linked,
                audio_element_get_state(el_item->el), audio_element_get_tag(el_item->el), el_item->el);
        if (false == el_item->linked)
        {
            continue;
        }
        if (wait_first_el)
        {
            ret |= audio_element_resume(el_item->el, 0, 2000 / portTICK_RATE_MS);
            wait_first_el = false;
        }
        else
        {
            ret |= audio_element_resume(el_item->el, 0, 2000 / portTICK_RATE_MS);
        }
    }
    audio_pipeline_change_state(pipeline, AEL_STATE_RUNNING);
    return ret;
}

bk_err_t audio_pipeline_pause(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (false == el_item->linked)
        {
            continue;
        }
        BK_LOGV(TAG, "pause [%s]  %p \n", audio_element_get_tag(el_item->el), el_item->el);
        audio_element_pause(el_item->el);
    }

    return BK_OK;
}

bk_err_t audio_pipeline_run(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    if (pipeline->state != AEL_STATE_INIT)
    {
        BK_LOGD(TAG, "Pipeline already started, state:%d \n", pipeline->state);
        return BK_OK;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        BK_LOGV(TAG, "start el[%16s], linked:%d, state:%d,[%p],  \n", audio_element_get_tag(el_item->el), el_item->linked,  audio_element_get_state(el_item->el), el_item->el);
        if (el_item->linked
            && ((AEL_STATE_INIT == audio_element_get_state(el_item->el))
                || (AEL_STATE_STOPPED == audio_element_get_state(el_item->el))
                || (AEL_STATE_FINISHED == audio_element_get_state(el_item->el))
                || (AEL_STATE_ERROR == audio_element_get_state(el_item->el))))
        {
            audio_element_run(el_item->el);
        }
    }
    AUDIO_MEM_SHOW(TAG);

    if (BK_FAIL == audio_pipeline_resume(pipeline))
    {
        BK_LOGE(TAG, "audio_pipeline_resume failed \n");
        audio_pipeline_change_state(pipeline, AEL_STATE_ERROR);
        audio_pipeline_terminate(pipeline);
        return BK_FAIL;
    }
    else
    {
        audio_pipeline_change_state(pipeline, AEL_STATE_RUNNING);
    }

    BK_LOGD(TAG, "Pipeline started \n");
    return BK_OK;
}

bk_err_t audio_pipeline_terminate(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    BK_LOGV(TAG, "Destroy audio_pipeline elements \n");
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            audio_element_terminate(el_item->el);
        }
    }
    return BK_OK;
}

bk_err_t audio_pipeline_stop(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    BK_LOGV(TAG, "audio_element_stop \n");
    if (pipeline->state != AEL_STATE_RUNNING)
    {
        BK_LOGD(TAG, "Without stop, st:%d \n", pipeline->state);
        return BK_FAIL;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            audio_element_stop(el_item->el);
        }
    }
    return BK_OK;
}

static inline bk_err_t __audio_pipeline_wait_stop(audio_pipeline_handle_t pipeline, TickType_t ticks_to_wait)
{
    audio_element_item_t *el_item;
    bk_err_t ret = BK_OK;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            bk_err_t res = audio_element_wait_for_stop_ms(el_item->el, ticks_to_wait);
            if (res == BK_ERR_ADK_TIMEOUT)
            {
                BK_LOGD(TAG, "Wait stop timeout, el:%p, tag:%s \n",
                        el_item->el, audio_element_get_tag(el_item->el) == NULL ? "NULL" : audio_element_get_tag(el_item->el));
            }
            else
            {
                audio_element_reset_state(el_item->el);
            }
            BK_LOGV(TAG, "[%s] stop result:%d \n", audio_element_get_tag(el_item->el) == NULL ? "NULL" : audio_element_get_tag(el_item->el), res);
            ret |= res;
        }
    }
    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
    return ret;
}

bk_err_t audio_pipeline_wait_for_stop(audio_pipeline_handle_t pipeline)
{
    if (pipeline->state != AEL_STATE_RUNNING)
    {
        BK_LOGD(TAG, "Without wait stop, st:%d \n", pipeline->state);
        return BK_FAIL;
    }

    BK_LOGV(TAG, "%s - IN \n", __func__);
    bk_err_t ret = __audio_pipeline_wait_stop(pipeline, portMAX_DELAY);
    BK_LOGV(TAG, "%s - OUT \n", __func__);
    return ret;
}

static bk_err_t _pipeline_rb_linked(audio_pipeline_handle_t pipeline, audio_element_handle_t el, bool first, bool last)
{
    static audio_port_handle_t port = NULL;
    port_item_t *port_item = NULL;
    char *out_port_tag = NULL;

    if (last)
    {
        audio_element_set_input_port(el, port);
    }
    else
    {
        if (!first)
        {
            /* config input port type */
            audio_element_set_input_port_type(el, audio_port_get_type(port));
            audio_element_set_input_port(el, port);
        }
        port_item = audio_calloc(1, sizeof(port_item_t));
        AUDIO_MEM_CHECK(TAG, port_item, {return BK_ERR_ADK_NO_MEM;});

        /* create audio output port according to port_type */
        if (audio_element_get_output_port_type(el) == PORT_TYPE_FB)
        {
            framebuf_port_cfg_t fb_port_config = {0};
            audio_element_get_output_framebuf_size(el, (int *)(uintptr_t *)&fb_port_config.node_size, (int *)(uintptr_t *)&fb_port_config.node_num);
            port = framebuf_port_init(&fb_port_config);
            /* set port tag: element_tag_in_fb */
            out_port_tag = audio_malloc(os_strlen(audio_element_get_tag(el)) + 8);
            if (out_port_tag)
            {
                os_snprintf(out_port_tag, os_strlen(audio_element_get_tag(el)) + 8, "%s_out_fb", audio_element_get_tag(el));
                audio_port_set_tag(port, out_port_tag);
                audio_free(out_port_tag);
                out_port_tag = NULL;
            }
        }
        else
        {
            ringbuf_port_cfg_t rb_port_config ={0};
            rb_port_config.ringbuf_size = audio_element_get_output_ringbuf_size(el);
            port = ringbuf_port_init(&rb_port_config);
            /* set port tag: element_tag_in_cb */
            out_port_tag = audio_malloc(os_strlen(audio_element_get_tag(el)) + 8);
            if (out_port_tag)
            {
                os_snprintf(out_port_tag, os_strlen(audio_element_get_tag(el)) + 8, "%s_out_rb", audio_element_get_tag(el));
                audio_port_set_tag(port, out_port_tag);
                audio_free(out_port_tag);
                out_port_tag = NULL;
            }
        }
        AUDIO_MEM_CHECK(TAG, port,
        {
            BK_LOGE(TAG, "[%s] output port init fail\n", audio_element_get_tag(el));
            audio_free(port_item);
            return BK_ERR_ADK_NO_MEM;
        });

        port_item->port = port;
        port_item->linked = true;
        port_item->kept_ctx = false;
        port_item->host_el = el;
        STAILQ_INSERT_TAIL(&pipeline->port_list, port_item, next);
        audio_element_set_output_port(el, port);
        BK_LOGD(TAG, "link el->port, el:%p, tag:%s, port:%p \n", el, audio_element_get_tag(el) == NULL ? "NULL" : audio_element_get_tag(el), port);
    }
    return BK_OK;
}

bk_err_t audio_pipeline_link(audio_pipeline_handle_t pipeline, const char *link_tag[], int link_num)
{
    bk_err_t ret = BK_OK;
    bool first = false, last = false;
    if (pipeline->linked)
    {
        audio_pipeline_unlink(pipeline);
    }
    for (int i = 0; i < link_num; i++)
    {
        audio_element_item_t *item = audio_pipeline_get_el_item_by_tag(pipeline, link_tag[i]);
        if (item == NULL)
        {
            BK_LOGE(TAG, "There is 1 link_tag invalid: %s \n", link_tag[i]);
            return BK_FAIL;
        }
        item->linked = true;
        item->kept_ctx = false;
        audio_element_handle_t el = item->el;
        first = (i == 0);
        last = (i == link_num - 1);
        ret = _pipeline_rb_linked(pipeline, el, first, last);
        if (ret != BK_OK)
        {
            return ret;
        }
    }
    pipeline->linked = true;
    PIPELINE_DEBUG(pipeline);
    return BK_OK;
}

bk_err_t audio_pipeline_unlink(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    port_item_t *port_item, *tmp;
    if (!pipeline->linked)
    {
        return BK_OK;
    }
    audio_pipeline_remove_listener(pipeline);
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            el_item->linked = false;
            el_item->kept_ctx = false;
            //BK_LOGD(TAG, "[%s] el->in: %p, el->out: %p\n",  audio_element_get_tag(el_item->el), audio_element_get_input_port(el_item->el), audio_element_get_output_port(el_item->el));
            /* check whether element port is the same as the port of port_item */
            if (audio_element_get_input_port_type(el_item->el) != PORT_TYPE_CB)
            {
                audio_element_set_input_port(el_item->el, NULL);
            }
            if (audio_element_get_output_port_type(el_item->el) != PORT_TYPE_CB)
            {
                audio_element_set_output_port(el_item->el, NULL);
            }
            BK_LOGV(TAG, "audio_pipeline_unlink, %p, %s \n", el_item->el, audio_element_get_tag(el_item->el));
            //BK_LOGD(TAG, "[%s] el->in: %p, el->out: %p\n",  audio_element_get_tag(el_item->el), audio_element_get_input_port(el_item->el), audio_element_get_output_port(el_item->el));
        }
    }
    STAILQ_FOREACH_SAFE(port_item, &pipeline->port_list, next, tmp)
    {
        BK_LOGV(TAG, "audio_pipeline_unlink, PORT:%p, host_el:%p \n", port_item->port, port_item->host_el);
        STAILQ_REMOVE(&pipeline->port_list, port_item, port_item, next);
        if (port_item->host_el)
        {
            /* check whether element port is the same as the port of port_item */
            if (audio_element_get_input_port(port_item->host_el) == port_item->port)
            {
                audio_element_set_input_port(port_item->host_el, NULL);
            }
            if (audio_element_get_output_port(port_item->host_el) == port_item->port)
            {
                audio_element_set_output_port(port_item->host_el, NULL);
            }
        }
        audio_port_deinit(port_item->port);
        port_item->linked = false;
        port_item->kept_ctx = false;
        port_item->host_el = NULL;
        audio_free(port_item);
    }
    BK_LOGD(TAG, "audio_pipeline_unlinked \n");
    STAILQ_INIT(&pipeline->port_list);
    pipeline->linked = false;
    return BK_OK;
}

bk_err_t audio_pipeline_reset_items_state(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    BK_LOGV(TAG, "audio_pipeline_reset_items_state \n");
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            el_item->el_state = AEL_STATUS_NONE;
        }
    }
    return BK_OK;
}

bk_err_t audio_pipeline_reset_port(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            audio_element_reset_input_port(el_item->el);
            audio_element_reset_output_port(el_item->el);
        }
    }
    return BK_OK;
}

bk_err_t audio_pipeline_reset_elements(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next)
    {
        if (el_item->linked)
        {
            audio_element_reset_state(el_item->el);
        }
    }
    return BK_OK;
}

