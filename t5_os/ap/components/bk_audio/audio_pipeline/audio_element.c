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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/fb_port.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_pipeline/cb_port.h>


#define TAG  "AUD_ELE"
#define DEFAULT_MAX_WAIT_TIME       (2000/portTICK_RATE_MS)


/**
 *  Audio Callback Abstract
 */
typedef struct audio_callback
{
    event_cb_func               cb;
    void                        *ctx;
} audio_callback_t;

typedef struct audio_multi_port
{
    audio_port_handle_t *       port;
    int                         max_port_num;
} audio_multi_port_t;

typedef enum
{
    EVENTS_TYPE_Q = 1,  /* Events through MessageQueue */
    EVENTS_TYPE_CB,     /* Events through Callback function */
} events_type_t;

typedef struct audio_rb_cfg
{
    int                         buf_size_expect;
    int                         rb_size;
} audio_rb_cfg_t;

typedef struct audio_fb_cfg
{
    int                         node_num_expect;
    int                         node_size;
    int                         node_num;
} audio_fb_cfg_t;

struct audio_element
{
    /* Functions/RingBuffers */
    el_io_func                  open;
    ctrl_func                   seek;
    process_func                process;
    el_io_func                  close;
    el_io_func                  destroy;
    port_type_t                 in_type;
    audio_port_handle_t         in;
    port_type_t                 out_type;
    audio_port_handle_t         out;

    audio_multi_port_t          multi_in;
    audio_multi_port_t          multi_out;

    /* Properties */
    volatile bool               is_open;
    audio_element_state_t       state;

    events_type_t               events_type;
    audio_event_iface_handle_t  iface_event;
    audio_callback_t            callback_event;

    int                         buf_size;
    char                        *buf;

    char                        *tag;
    int                         task_stack;
    int                         task_prio;
    int                         task_core;
    xSemaphoreHandle            lock;
    audio_element_info_t        info;
    audio_element_info_t        *report_info;

    beken_thread_t              audio_thread;

    /* PrivateData */
    void                        *data;
    EventGroupHandle_t          state_event;
    int                         input_wait_time;
    int                         output_wait_time;
    union
    {
        audio_rb_cfg_t          out_cfg_rb;
        audio_fb_cfg_t          out_cfg_fb;
    } out_cfg;
    volatile bool               is_running;
    volatile bool               task_run;
    volatile bool               stopping;
};

#define BIT6     0x00000040
#define BIT5     0x00000020
#define BIT4     0x00000010
#define BIT3     0x00000008
#define BIT2     0x00000004
#define BIT1     0x00000002
#define BIT0     0x00000001


const static int STOPPED_BIT = BIT0;
const static int STARTED_BIT = BIT1;
const static int BUFFER_REACH_LEVEL_BIT = BIT2;
const static int TASK_CREATED_BIT = BIT3;
const static int TASK_DESTROYED_BIT = BIT4;
const static int PAUSED_BIT = BIT5;
const static int RESUMED_BIT = BIT6;

static bk_err_t audio_element_on_cmd_error(audio_element_handle_t el);
static bk_err_t audio_element_on_cmd_stop(audio_element_handle_t el);

static bk_err_t audio_element_force_set_state(audio_element_handle_t el, audio_element_state_t new_state)
{
    el->state = new_state;
    return BK_OK;
}

static bk_err_t audio_element_cmd_send(audio_element_handle_t el, audio_element_msg_cmd_t cmd)
{
    audio_event_iface_msg_t msg =
    {
        .source = el,
        .source_type = AUDIO_ELEMENT_TYPE_ELEMENT,
        .cmd = cmd,
    };
    BK_LOGD(TAG, "[%s]evt internal cmd = %d \n", el->tag, msg.cmd);
    return audio_event_iface_cmd(el->iface_event, &msg);
}

static bk_err_t audio_element_msg_sendout(audio_element_handle_t el, audio_event_iface_msg_t *msg)
{
    msg->source = el;
    msg->source_type = AUDIO_ELEMENT_TYPE_ELEMENT;
    if (el->events_type == EVENTS_TYPE_CB && el->callback_event.cb)
    {
        return el->callback_event.cb(el, msg, el->callback_event.ctx);
    }
    return audio_event_iface_sendout(el->iface_event, msg);
}

bk_err_t audio_element_process_init(audio_element_handle_t el)
{
    if (el->open == NULL)
    {
        el->is_open = true;
        xEventGroupSetBits(el->state_event, STARTED_BIT);
        return BK_OK;
    }
    el->is_open = true;
    audio_element_force_set_state(el, AEL_STATE_INITIALIZING);
    bk_err_t ret = el->open(el);
    if (ret == BK_OK)
    {
        BK_LOGV(TAG, "[%s] el opened \n", el->tag);
        audio_element_force_set_state(el, AEL_STATE_RUNNING);
        audio_element_report_status(el, AEL_STATUS_STATE_RUNNING);
        xEventGroupSetBits(el->state_event, STARTED_BIT);
        return BK_OK;
    }
    else if (ret == AEL_IO_DONE)
    {
        BK_LOGW(TAG, "[%s] OPEN AEL_IO_DONE \n", el->tag);
        audio_element_force_set_state(el, AEL_STATE_RUNNING);
        audio_element_report_status(el, AEL_STATUS_STATE_RUNNING);
        return BK_OK;
    }
    else if (ret == AEL_IO_ABORT)
    {
        BK_LOGW(TAG, "[%s] AEL_IO_ABORT, %d \n", el->tag, ret);
        audio_element_on_cmd_stop(el);
    }
    else
    {
        BK_LOGE(TAG, "[%s] AEL_STATUS_ERROR_OPEN, %d \n", el->tag, ret);
        audio_element_force_set_state(el, AEL_STATE_ERROR);
        audio_element_report_status(el, AEL_STATUS_ERROR_OPEN);
        audio_element_on_cmd_error(el);
    }
    return BK_FAIL;
}

bk_err_t audio_element_process_deinit(audio_element_handle_t el)
{
    if (el->is_open && el->close)
    {
        BK_LOGD(TAG, "[%s] will be closed, line %d \n", el->tag, __LINE__);
        el->close(el);
    }
    el->is_open = false;
    return BK_OK;
}

static bk_err_t audio_element_on_cmd_error(audio_element_handle_t el)
{
    if (el->state != AEL_STATE_STOPPED)
    {
        BK_LOGW(TAG, "[%s] audio_element_on_cmd_error,%d \n", el->tag, el->state);
        audio_element_process_deinit(el);
        el->state = AEL_STATE_ERROR;
        audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
        el->is_running = false;
        xEventGroupSetBits(el->state_event, STOPPED_BIT);
    }
    return BK_OK;
}

static bk_err_t audio_element_on_cmd_stop(audio_element_handle_t el)
{
    if ((el->state != AEL_STATE_FINISHED) && (el->state != AEL_STATE_STOPPED))
    {
        audio_element_process_deinit(el);
        el->state = AEL_STATE_STOPPED;
        audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
        audio_element_report_status(el, AEL_STATUS_STATE_STOPPED);
        el->is_running = false;
        el->stopping = false;
        BK_LOGV(TAG, "[%s] audio_element_on_cmd_stop \n", el->tag);
        xEventGroupSetBits(el->state_event, STOPPED_BIT);
    }
    else
    {
        // Change element state to AEL_STATE_STOPPED, even if AEL_STATE_ERROR or AEL_STATE_FINISHED
        // Except AEL_STATE_STOPPED and is not running
        BK_LOGV(TAG, "[%s] audio_element_on_cmd_stop, state:%d \n", el->tag, el->state);
        if ((el->is_running == false) && (el->state == AEL_STATE_STOPPED))
        {
            el->stopping = false;
            return BK_OK;
        }
        el->state = AEL_STATE_STOPPED;
        el->is_running = false;
        el->stopping = false;
        audio_element_report_status(el, AEL_STATUS_STATE_STOPPED);
        xEventGroupSetBits(el->state_event, STOPPED_BIT);
    }
    return BK_OK;
}

static bk_err_t audio_element_on_cmd_finish(audio_element_handle_t el)
{
    if ((el->state == AEL_STATE_ERROR)
        || (el->state == AEL_STATE_STOPPED))
    {
        BK_LOGV(TAG, "[%s] audio_element_on_cmd_finish, state:%d \n", el->tag, el->state);
        return BK_OK;
    }
    audio_element_process_deinit(el);
    el->state = AEL_STATE_FINISHED;
    audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
    audio_element_report_status(el, AEL_STATUS_STATE_FINISHED);
    el->is_running = false;
    xEventGroupSetBits(el->state_event, STOPPED_BIT);
    BK_LOGV(TAG, "[%s] audio_element_on_cmd_finish \n", el->tag);
    return BK_OK;
}

static bk_err_t audio_element_on_cmd_resume(audio_element_handle_t el)
{
    if (el->state == AEL_STATE_RUNNING)
    {
        el->is_running = true;
        xEventGroupSetBits(el->state_event, RESUMED_BIT);
        return BK_OK;
    }
    if (el->state != AEL_STATE_INIT && el->state != AEL_STATE_RUNNING && el->state != AEL_STATE_PAUSED)
    {
        audio_element_reset_output_port(el);
    }
    //设置task运行状态
    el->is_running = true;
    //将RESUMED_BIT置1，让app task 中 audio_element_resume 接口继续向下运行
    xEventGroupSetBits(el->state_event, RESUMED_BIT);
    //调用element的open接口初始化
    if (audio_element_process_init(el) != BK_OK)
    {
        audio_element_abort_output_port(el);
        audio_element_abort_input_port(el);
        el->is_running = false;
        return BK_FAIL;
    }
    audio_event_iface_set_cmd_waiting_timeout(el->iface_event, 0);
    xEventGroupClearBits(el->state_event, STOPPED_BIT);
    return BK_OK;
}

static bk_err_t audio_element_on_cmd(audio_event_iface_msg_t *msg, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;

    if (msg->source_type != AUDIO_ELEMENT_TYPE_ELEMENT)
    {
        BK_LOGE(TAG, "[%s] Invalid event type, this event should be ELEMENT type \n", el->tag);
        return BK_FAIL;
    }
    bk_err_t ret = BK_OK;
    //process an event
    switch (msg->cmd)
    {
        case AEL_MSG_CMD_FINISH:
            BK_LOGV(TAG, "[%s] AEL_MSG_CMD_FINISH, state:%d \n", el->tag, el->state);
            ret = audio_element_on_cmd_finish(el);
            break;
        case AEL_MSG_CMD_STOP:
            BK_LOGV(TAG, "[%s] AEL_MSG_CMD_STOP, state:%d \n", el->tag, el->state);
            ret = audio_element_on_cmd_stop(el);
            break;
        case AEL_MSG_CMD_PAUSE:
            el->state = AEL_STATE_PAUSED;
            audio_element_process_deinit(el);
            audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
            audio_element_report_status(el, AEL_STATUS_STATE_PAUSED);
            el->is_running = false;
            BK_LOGD(TAG, "[%s] AEL_MSG_CMD_PAUSE \n", el->tag);
            xEventGroupSetBits(el->state_event, PAUSED_BIT);
            break;
        case AEL_MSG_CMD_RESUME:
            //调用命令处理函数
            BK_LOGD(TAG, "[%s] AEL_MSG_CMD_RESUME,state:%d \n", el->tag, el->state);
            ret = audio_element_on_cmd_resume(el);
            break;
        case AEL_MSG_CMD_DESTROY:
            el->is_running = false;
            BK_LOGV(TAG, "[%s] AEL_MSG_CMD_DESTROY \n", el->tag);
            ret = AEL_IO_ABORT;
    }
    return ret;
}

static bk_err_t audio_element_process_running(audio_element_handle_t el)
{
    int process_len = -1;
    if (el->state < AEL_STATE_RUNNING || !el->is_running)
    {
        return BK_ERR_ADK_INVALID_STATE;
    }
    //执行注册的处理函数
    process_len = el->process(el, el->buf, el->buf_size);
    if (process_len <= 0)
    {
        switch (process_len)
        {
            case AEL_IO_ABORT:
                BK_LOGV(TAG, "[%s] ERROR_PROCESS, AEL_IO_ABORT \n", el->tag);
                audio_element_on_cmd_stop(el);
                break;
            case AEL_IO_DONE:
            case AEL_IO_OK:
                // Re-open if reset_state function called
                if (audio_element_get_state(el) == AEL_STATE_INIT)
                {
                    return audio_element_on_cmd_resume(el);
                }
                audio_element_set_port_done(el);
                audio_element_on_cmd_finish(el);
                break;
            case AEL_IO_FAIL:
                BK_LOGE(TAG, "[%s] ERROR_PROCESS, AEL_IO_FAIL \n", el->tag);
                audio_element_report_status(el, AEL_STATUS_ERROR_PROCESS);
                audio_element_on_cmd_error(el);
                break;
            case AEL_IO_TIMEOUT:
                BK_LOGV(TAG, "[%s] ERROR_PROCESS, AEL_IO_TIMEOUT \n", el->tag);
                break;
            case AEL_PROCESS_FAIL:
                BK_LOGE(TAG, "[%s] ERROR_PROCESS, AEL_PROCESS_FAIL \n", el->tag);
                audio_element_report_status(el, AEL_STATUS_ERROR_PROCESS);
                audio_element_on_cmd_error(el);
                break;
            default:
                BK_LOGW(TAG, "[%s] Process return error,ret:%d \n", el->tag, process_len);
                break;
        }
    }
    return BK_OK;
}

int audio_element_input(audio_element_handle_t el, char *buffer, int wanted_size)
{
    int in_len = 0;

    if (el->in)
    {
        in_len = audio_port_read(el->in, buffer, wanted_size, el->input_wait_time);
    }
    else
    {
        BK_LOGE(TAG, "[%s] Invalid audio read port\n", el->tag);
        return BK_FAIL;
    }

    if (in_len <= 0)
    {
        switch (in_len)
        {
            case AEL_IO_ABORT:
                BK_LOGW(TAG, "IN-[%s] AEL_IO_ABORT \n", el->tag);
                break;
            case AEL_IO_DONE:
            case AEL_IO_OK:
                BK_LOGD(TAG, "IN-[%s] AEL_IO_DONE,%d \n", el->tag, in_len);
                break;
            case AEL_IO_FAIL:
                BK_LOGE(TAG, "IN-[%s] AEL_STATUS_ERROR_INPUT \n", el->tag);
                audio_element_report_status(el, AEL_STATUS_ERROR_INPUT);
                break;
            case AEL_IO_TIMEOUT:
                BK_LOGV(TAG, "IN-[%s] AEL_IO_TIMEOUT", el->tag);
                break;
            default:
                BK_LOGE(TAG, "IN-[%s] Input return not support,ret:%d \n", el->tag, in_len);
                break;
        }
    }
    return in_len;
}

int audio_element_output(audio_element_handle_t el, char *buffer, int write_size)
{
    int output_len = 0;

    if (el->out && write_size)
    {
        output_len = audio_port_write(el->out, buffer, write_size, el->output_wait_time);
        if (el->out_type == PORT_TYPE_RB)
        {
            if ((audio_port_get_filled_size(el->out) > el->out_cfg.out_cfg_rb.buf_size_expect) || (output_len < 0))
            {
                xEventGroupSetBits(el->state_event, BUFFER_REACH_LEVEL_BIT);
            }
        }
        else if (el->out_type == PORT_TYPE_FB)
        {
            //TODO
            //BK_LOGE(TAG, "%s, %d filled_size: %d\n", __func__, __LINE__, audio_port_get_filled_size(el->out));
            if ((audio_port_get_filled_size(el->out) > el->out_cfg.out_cfg_fb.node_num_expect) || (output_len < 0))
            {
                xEventGroupSetBits(el->state_event, BUFFER_REACH_LEVEL_BIT);
            }
        }
        else
        {
            //nothing
        }
    }

    if (output_len <= 0)
    {
        switch (output_len)
        {
            case AEL_IO_ABORT:
                BK_LOGW(TAG, "OUT-[%s] AEL_IO_ABORT \n", el->tag);
                break;
            case AEL_IO_DONE:
            case AEL_IO_OK:
                BK_LOGD(TAG, "OUT-[%s] AEL_IO_DONE,%d \n", el->tag, output_len);
                break;
            case AEL_IO_FAIL:
                BK_LOGE(TAG, "OUT-[%s] AEL_STATUS_ERROR_OUTPUT \n", el->tag);
                audio_element_report_status(el, AEL_STATUS_ERROR_OUTPUT);
                break;
            case AEL_IO_TIMEOUT:
                BK_LOGW(TAG, "OUT-[%s] AEL_IO_TIMEOUT \n", el->tag);
                break;
            default:
                BK_LOGE(TAG, "OUT-[%s] Output return not support,ret:%d \n", el->tag, output_len);
                break;
        }
    }
    return output_len;
}
void audio_element_task(void *pv)
{
    audio_element_handle_t el = (audio_element_handle_t)pv;
    el->task_run = true;
    xEventGroupSetBits(el->state_event, TASK_CREATED_BIT);
    audio_element_force_set_state(el, AEL_STATE_INIT);
    audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
    if (el->buf_size > 0)
    {
        el->buf = audio_calloc(1, el->buf_size);
        AUDIO_MEM_CHECK(TAG, el->buf,
        {
            el->task_run = false;
            BK_LOGE(TAG, "[%s] Error malloc element buffer \n", el->tag);
        });
    }
    //清零事件组中的stopped停止位
    xEventGroupClearBits(el->state_event, STOPPED_BIT);
    bk_err_t ret = BK_OK;
    while (el->task_run)
    {
        if ((ret = audio_event_iface_waiting_cmd_msg(el->iface_event)) != BK_OK)
        {
            //如果处理结果异常，将事件组中的停止位置1
            xEventGroupSetBits(el->state_event, STOPPED_BIT);
            /*
             * Do not exit task when audio_element_process_init failure to
             * make call audio_element_deinit safety.
            */
            if (ret == AEL_IO_ABORT)
            {
                break;
            }
        }
        if (audio_element_process_running(el) != BK_OK)
        {
            // continue;
        }
    }

    if (el->is_open && el->close)
    {
        BK_LOGV(TAG, "[%s-%p] el closed \n", el->tag, el);
        el->close(el);
        audio_element_force_set_state(el, AEL_STATE_STOPPED);
    }
    el->is_open = false;
    audio_free(el->buf);
    el->buf = NULL;
    el->stopping = false;
    el->task_run = false;
    //    BK_LOGV(TAG, "[%s-%p] el task deleted,%d \n", el->tag, el, uxTaskGetStackHighWaterMark(NULL));
    BK_LOGV(TAG, "[%s-%p] el task deleted \n", el->tag, el);
    xEventGroupSetBits(el->state_event, STOPPED_BIT);
    xEventGroupSetBits(el->state_event, RESUMED_BIT);
    xEventGroupSetBits(el->state_event, TASK_DESTROYED_BIT);
    el->audio_thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t audio_element_reset_state(audio_element_handle_t el)
{
    return audio_element_force_set_state(el, AEL_STATE_INIT);
}

audio_element_state_t audio_element_get_state(audio_element_handle_t el)
{
    if (el)
    {
        return el->state;
    }
    return BK_FAIL;
}

QueueHandle_t audio_element_get_event_queue(audio_element_handle_t el)
{
    if (!el)
    {
        return NULL;
    }
    return audio_event_iface_get_queue_handle(el->iface_event);
}

bk_err_t audio_element_setdata(audio_element_handle_t el, void *data)
{
    el->data = data;
    return BK_OK;
}

void *audio_element_getdata(audio_element_handle_t el)
{
    return el->data;
}

bk_err_t audio_element_set_tag(audio_element_handle_t el, const char *tag)
{
    if (el->tag)
    {
        audio_free(el->tag);
        el->tag = NULL;
    }

    if (tag)
    {
        el->tag = audio_strdup(tag);
        AUDIO_MEM_CHECK(TAG, el->tag,
        {
            return BK_ERR_ADK_NO_MEM;
        });
    }
    return BK_OK;
}

char *audio_element_get_tag(audio_element_handle_t el)
{
    return el->tag;
}

bk_err_t audio_element_set_uri(audio_element_handle_t el, const char *uri)
{
    while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
    if (el->info.uri)
    {
        audio_free(el->info.uri);
        el->info.uri = NULL;
    }

    if (uri)
    {
        el->info.uri = audio_strdup(uri);
        AUDIO_MEM_CHECK(TAG, el->info.uri,
        {
            xSemaphoreGive((QueueHandle_t)(el->lock));
            return BK_ERR_ADK_NO_MEM;
        });
    }
    xSemaphoreGive((QueueHandle_t)(el->lock));
    return BK_OK;
}

char *audio_element_get_uri(audio_element_handle_t el)
{
    while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
    char *uri = el->info.uri;
    xSemaphoreGive((QueueHandle_t)(el->lock));
    return uri;
}

bk_err_t audio_element_set_event_callback(audio_element_handle_t el, event_cb_func cb_func, void *ctx)
{
    el->events_type = EVENTS_TYPE_CB;
    el->callback_event.cb = cb_func;
    el->callback_event.ctx = ctx;
    return BK_OK;
}

bk_err_t audio_element_msg_set_listener(audio_element_handle_t el, audio_event_iface_handle_t listener)
{
    return audio_event_iface_set_listener(el->iface_event, listener);
}

bk_err_t audio_element_msg_remove_listener(audio_element_handle_t el, audio_event_iface_handle_t listener)
{
    return audio_event_iface_remove_listener(listener, el->iface_event);
}

bk_err_t audio_element_setinfo(audio_element_handle_t el, audio_element_info_t *info)
{
    if (info && el)
    {
        //FIXME: We will got reset if lock mutex here
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        os_memcpy(&el->info, info, sizeof(audio_element_info_t));
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_getinfo(audio_element_handle_t el, audio_element_info_t *info)
{
    if (info && el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        os_memcpy(info, &el->info, sizeof(audio_element_info_t));
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_report_info(audio_element_handle_t el)
{
    if (el)
    {
        audio_event_iface_msg_t msg = { 0 };
        msg.cmd = AEL_MSG_CMD_REPORT_MUSIC_INFO;
        msg.data = NULL;
        BK_LOGD(TAG, "REPORT_INFO,[%s]evt out cmd:%d, \n", el->tag, msg.cmd);
        audio_element_msg_sendout(el, &msg);
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_report_codec_fmt(audio_element_handle_t el)
{
    if (el)
    {
        audio_event_iface_msg_t msg = { 0 };
        msg.cmd = AEL_MSG_CMD_REPORT_CODEC_FMT;
        msg.data = NULL;
        BK_LOGV(TAG, "REPORT_FMT,[%s]evt out cmd:%d, \n", el->tag, msg.cmd);
        audio_element_msg_sendout(el, &msg);
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_report_status(audio_element_handle_t el, audio_element_status_t status)
{
    if (el)
    {
        audio_event_iface_msg_t msg = { 0 };
        msg.cmd = AEL_MSG_CMD_REPORT_STATUS;
        msg.data = (void *)status;
        msg.data_len = sizeof(status);
        BK_LOGV(TAG, "REPORT_STATUS,[%s]evt out cmd = %d,status:%d \n", el->tag, msg.cmd, status);
        return audio_element_msg_sendout(el, &msg);
    }
    return BK_FAIL;
}

bk_err_t audio_element_report_pos(audio_element_handle_t el)
{
    if (el)
    {
        audio_event_iface_msg_t msg = { 0 };
        msg.cmd = AEL_MSG_CMD_REPORT_POSITION;
        if (el->report_info == NULL)
        {
            el->report_info = audio_calloc(1, sizeof(audio_element_info_t));
            AUDIO_MEM_CHECK(TAG, el->report_info, return BK_ERR_ADK_NO_MEM);
        }

        audio_element_getinfo(el, el->report_info);
        msg.data = el->report_info;
        msg.data_len = sizeof(audio_element_info_t);
        BK_LOGV(TAG, "REPORT_POS,[%s]evt out cmd:%d, \n", el->tag, msg.cmd);
        audio_element_msg_sendout(el, &msg);
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_reset_input_port(audio_element_handle_t el)
{
    int ret = BK_OK;
    if (el->in)
    {
        ret |= audio_port_reset(el->in);
        for (int i = 0; i < el->multi_in.max_port_num; i++)
        {
            if (el->multi_in.port[i])
            {
                ret |= audio_port_reset(el->multi_in.port[i]);
            }
        }
    }
    return ret;
}

bk_err_t audio_element_reset_output_port(audio_element_handle_t el)
{
    int ret = BK_OK;
    if (el->out)
    {
        ret |= audio_port_reset(el->out);
        for (int i = 0; i < el->multi_out.max_port_num; i++)
        {
            if (el->multi_out.port[i])
            {
                ret |= audio_port_reset(el->multi_out.port[i]);
            }
        }
    }
    return BK_OK;
}

bk_err_t audio_element_abort_input_port(audio_element_handle_t el)
{
    int ret = BK_OK;
    if (el->in)
    {
        ret |= audio_port_abort(el->in);
        for (int i = 0; i < el->multi_in.max_port_num; i++)
        {
            if (el->multi_in.port[i])
            {
                ret |= audio_port_abort(el->multi_in.port[i]);
            }
        }
    }
    return BK_OK;
}

bk_err_t audio_element_abort_output_port(audio_element_handle_t el)
{
//    if (el->write_type != IO_TYPE_RB)
//    {
//        return BK_FAIL;
//    }
    int ret = BK_OK;
    if (el->out)
    {
        ret |= audio_port_abort(el->out);
        for (int i = 0; i < el->multi_out.max_port_num; ++i)
        {
            if (el->multi_out.port[i])
            {
                ret |= audio_port_abort(el->multi_out.port[i]);
            }
        }
    }
    return BK_OK;
}

bk_err_t audio_element_set_port_done(audio_element_handle_t el)
{
    int ret = BK_OK;
    if (NULL == el)
    {
        return BK_FAIL;
    }
    if (el->out)
    {
        ret |= audio_port_write_done(el->out);
        for (int i = 0; i < el->multi_out.max_port_num; i++)
        {
            if (el->multi_out.port[i])
            {
                ret |= audio_port_write_done(el->multi_out.port[i]);
            }
        }
    }
    return ret;
}

bk_err_t audio_element_set_input_port(audio_element_handle_t el, audio_port_handle_t port)
{
    if (port)
    {
        el->in = port;
        el->in_type = audio_port_get_type(port);
    }
    else
    {
        el->in = port;
    }

    return BK_OK;
}

audio_port_handle_t audio_element_get_input_port(audio_element_handle_t el)
{
    return el->in;
}

bk_err_t audio_element_set_output_port(audio_element_handle_t el, audio_port_handle_t port)
{
    if (port)
    {
        el->out = port;
        el->out_type = audio_port_get_type(port);
    }
    else
    {
        el->out = port;
    }

    return BK_OK;
}

audio_port_handle_t audio_element_get_output_port(audio_element_handle_t el)
{
    return el->out;
}

bk_err_t audio_element_set_input_timeout(audio_element_handle_t el, TickType_t timeout)
{
    if (el)
    {
        el->input_wait_time = timeout;
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_output_timeout(audio_element_handle_t el, TickType_t timeout)
{
    if (el)
    {
        el->output_wait_time = timeout;
        return BK_OK;
    }
    return BK_FAIL;
}

int audio_element_get_output_ringbuf_size(audio_element_handle_t el)
{
    if (el && el->out_type != PORT_TYPE_RB)
    {
        return BK_FAIL;
    }

    if (el)
    {
        return el->out_cfg.out_cfg_rb.rb_size;
    }
    return 0;
}

bk_err_t audio_element_set_output_ringbuf_size(audio_element_handle_t el, int rb_size)
{
    if (el && el->out_type != PORT_TYPE_RB)
    {
        return BK_FAIL;
    }

    if (el)
    {
        el->out_cfg.out_cfg_rb.rb_size = rb_size;
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_get_output_framebuf_size(audio_element_handle_t el, int *node_size, int *node_num)
{
    if (el && el->out_type != PORT_TYPE_FB)
    {
        return BK_FAIL;
    }

    if (el)
    {
        *node_size = el->out_cfg.out_cfg_fb.node_size;
        *node_num = el->out_cfg.out_cfg_fb.node_num;
    }

    return BK_OK;
}

bk_err_t audio_element_set_output_framebuf_size(audio_element_handle_t el, int node_size, int node_num)
{
    if (el && el->out_type != PORT_TYPE_FB)
    {
        return BK_FAIL;
    }

    if (el)
    {
        el->out_cfg.out_cfg_fb.node_size = node_size;
        el->out_cfg.out_cfg_fb.node_num = node_num;
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_wait_for_stop(audio_element_handle_t el)
{
    if (el->is_running == false)
    {
        BK_LOGV(TAG, "[%s] Element already stopped, return without waiting \n", el->tag);
        return BK_FAIL;
    }
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, STOPPED_BIT, false, true, DEFAULT_MAX_WAIT_TIME);
    bk_err_t ret = BK_ERR_ADK_TIMEOUT;
    if (uxBits & STOPPED_BIT)
    {
        ret = BK_OK;
    }
    return ret;
}

bk_err_t audio_element_wait_for_buffer(audio_element_handle_t el, int size_expect, TickType_t timeout)
{
    int ret = BK_FAIL;
    if (el->out_type == PORT_TYPE_RB)
    {
        el->out_cfg.out_cfg_rb.buf_size_expect = size_expect;
    }
    else if (el->out_type == PORT_TYPE_FB)
    {
        el->out_cfg.out_cfg_rb.buf_size_expect = size_expect;
    }
    else
    {
        BK_LOGE(TAG, "[%s] PORT_TYPE_CB not support wait for buffer\n", el->tag);
        return BK_FAIL;
    }

    if (el->out)
    {
        xEventGroupClearBits(el->state_event, BUFFER_REACH_LEVEL_BIT);
        EventBits_t uxBits = xEventGroupWaitBits(el->state_event, BUFFER_REACH_LEVEL_BIT, false, true, timeout);
        if ((uxBits & BUFFER_REACH_LEVEL_BIT) != 0)
        {
            ret = BK_OK;
        }
        else
        {
            ret = BK_FAIL;
        }
    }
    return ret;
}

audio_element_handle_t audio_element_init(audio_element_cfg_t *config)
{
    audio_element_handle_t el = audio_calloc(1, sizeof(struct audio_element));

    AUDIO_MEM_CHECK(TAG, el,
    {
        return NULL;
    });

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt_cfg.on_cmd = audio_element_on_cmd;
    evt_cfg.context = el;
    evt_cfg.queue_set_size = 0; // Element have no queue_set by default.
    evt_cfg.external_queue_size = 5;
    evt_cfg.internal_queue_size = 5;
    bool _success =
        (
            ((config->tag ? audio_element_set_tag(el, config->tag) : audio_element_set_tag(el, "unknown")) == BK_OK) &&
            (el->lock           = xSemaphoreCreateMutex())                   &&
            (el->iface_event    = audio_event_iface_init(&evt_cfg)) &&
            (el->state_event    = xEventGroupCreate())
        );

    AUDIO_MEM_CHECK(TAG, _success, goto _element_init_failed);

    el->open = config->open;
    el->process = config->process;
    el->close = config->close;
    el->destroy = config->destroy;
    el->seek = config->seek;
    el->multi_in.max_port_num = config->multi_in_port_num;
    el->multi_out.max_port_num = config->multi_out_port_num;
    el->in_type = config->in_type;
    el->out_type = config->out_type;
    if (el->multi_in.max_port_num > 0)
    {
        el->multi_in.port = (audio_port_handle_t *)audio_calloc(el->multi_in.max_port_num, sizeof(audio_port_handle_t));
        AUDIO_MEM_CHECK(TAG, el->multi_in.port, goto _element_init_failed);
    }
    if (el->multi_out.max_port_num > 0)
    {
        el->multi_out.port = (audio_port_handle_t *)audio_calloc(el->multi_out.max_port_num, sizeof(audio_port_handle_t));
        AUDIO_MEM_CHECK(TAG, el->multi_out.port, goto _element_init_failed);
    }

    if (config->task_stack > 0)
    {
        el->task_stack = config->task_stack;
    }
    if (config->task_prio)
    {
        el->task_prio = config->task_prio;
    }
    else
    {
        el->task_prio = DEFAULT_ELEMENT_TASK_PRIO;
    }
    if (config->task_core)
    {
        el->task_core = config->task_core;
    }
    else
    {
        el->task_core = DEFAULT_ELEMENT_TASK_CORE;
    }

    if (el->out_type == PORT_TYPE_RB)
    {
        if (config->out_block_size > 0 && config->out_block_num > 0)
        {
            el->out_cfg.out_cfg_rb.rb_size = config->out_block_size * config->out_block_num;
        }
        else
        {
            el->out_cfg.out_cfg_rb.rb_size = DEFAULT_ELEMENT_RINGBUF_SIZE;
        }
    }
    else if (el->out_type == PORT_TYPE_FB)
    {
        if (config->out_block_size > 0 && config->out_block_num > 0)
        {
            el->out_cfg.out_cfg_fb.node_size = config->out_block_size;
            el->out_cfg.out_cfg_fb.node_num = config->out_block_num;
        }
        else
        {
            el->out_cfg.out_cfg_fb.node_size = DEFAULT_ELEMENT_FRAMEBUF_SIZE;
            el->out_cfg.out_cfg_fb.node_num = DEFAULT_ELEMENT_FRAMEBUF_NUM;
        }
    }
    else
    {
        //nothing
    }

    el->data = config ->data;

    el->state = AEL_STATE_INIT;
    el->buf_size = config->buffer_len;

    audio_element_info_t info = AUDIO_ELEMENT_INFO_DEFAULT();
    audio_element_setinfo(el, &info);
    audio_element_set_input_timeout(el, portMAX_DELAY);
    audio_element_set_output_timeout(el, portMAX_DELAY);

    if (config->read != NULL)
    {
        if (el->in_type != PORT_TYPE_CB)
        {
            BK_LOGE(TAG, "[%s] port type is not PORT_TYPE_CB, but read_cb is not NULL\n", el->tag);
            goto _element_init_failed;
        }
        callback_port_cfg_t read_cb_config = {0};
        read_cb_config.cb = (port_stream_func)config->read;
        read_cb_config.ctx = el;
        el->in = callback_port_init(&read_cb_config);
        if (!el->in)
        {
            BK_LOGE(TAG, "[%s] audio in cb port init fail\n", el->tag);
            goto _element_init_failed;
        }
        /* set port tag: element_tag_in_cb */
        char *in_port_tag = audio_malloc(os_strlen(config->tag) + 7);
        AUDIO_MEM_CHECK(TAG, in_port_tag, goto _element_init_failed);
        os_snprintf(in_port_tag, os_strlen(config->tag) + 7, "%s_in_cb", config->tag);
        BK_LOGD(NULL, "in_port_tag tag: %s \n", in_port_tag);
        audio_port_set_tag(el->in, in_port_tag);
        audio_free(in_port_tag);
        in_port_tag = NULL;
    }

    if (config->write != NULL)
    {
        if (el->out_type != PORT_TYPE_CB)
        {
            BK_LOGE(TAG, "[%s] port type is not PORT_TYPE_CB, but write_cb is not NULL\n", el->tag);
            goto _element_init_failed;
        }
        callback_port_cfg_t write_cb_config = {0};
        write_cb_config.cb = (port_stream_func)config->write;
        write_cb_config.ctx = el;
        el->out = callback_port_init(&write_cb_config);
        if (!el->out)
        {
            BK_LOGE(TAG, "[%s] audio out cb port init fail\n", el->tag);
            goto _element_init_failed;
        }
        /* set port tag: element_tag_out_cb */
        char *out_port_tag = audio_malloc(os_strlen(config->tag) + 8);
        AUDIO_MEM_CHECK(TAG, out_port_tag, goto _element_init_failed);
        os_snprintf(out_port_tag, os_strlen(config->tag) + 8, "%s_out_cb", config->tag);
        audio_port_set_tag(el->out, out_port_tag);
        audio_free(out_port_tag);
        out_port_tag = NULL;
    }

    el->events_type = EVENTS_TYPE_Q;
    return el;
_element_init_failed:
    audio_element_set_uri(el, NULL);
    if (el->lock)
    {
        vSemaphoreDelete((QueueHandle_t)(el->lock));
    }
    if (el->state_event)
    {
        vEventGroupDelete(el->state_event);
    }
    if (el->iface_event)
    {
        audio_event_iface_destroy(el->iface_event);
    }
    if (el->tag)
    {
        audio_element_set_tag(el, NULL);
    }
    if (el->multi_in.port)
    {
        audio_free(el->multi_in.port);
        el->multi_in.port = NULL;
    }
    if (el->multi_out.port)
    {
        audio_free(el->multi_out.port);
        el->multi_out.port = NULL;
    }
    if (el->in)
    {
        audio_port_deinit(el->in);
        el->in = NULL;
    }
    if (el->out)
    {
        audio_port_deinit(el->out);
        el->out = NULL;
    }
    audio_free(el);
    return NULL;
}

bk_err_t audio_element_deinit(audio_element_handle_t el)
{
    audio_element_stop(el);
    audio_element_wait_for_stop(el);
    audio_element_terminate(el);
    vEventGroupDelete(el->state_event);

    audio_event_iface_destroy(el->iface_event);
    if (el->destroy)
    {
        el->destroy(el);
    }
    audio_element_set_tag(el, NULL);
    audio_element_set_uri(el, NULL);
    if (el->multi_in.port)
    {
        audio_free(el->multi_in.port);
        el->multi_in.port = NULL;
    }
    if (el->multi_out.port)
    {
        audio_free(el->multi_out.port);
        el->multi_out.port = NULL;
    }
    if (el->in)
    {
        if (audio_port_get_type(el->in) == PORT_TYPE_CB)
        {
            audio_port_deinit(el->in);
        }
        el->in = NULL;
    }
    if (el->out)
    {
        if (audio_port_get_type(el->out) == PORT_TYPE_CB)
        {
            audio_port_deinit(el->out);
        }
        el->out = NULL;
    }
    if (el->report_info)
    {
        audio_free(el->report_info);
    }
    vSemaphoreDelete((QueueHandle_t)(el->lock));
    el->lock = NULL;
    audio_free(el);
    return BK_OK;
}

bk_err_t audio_element_run(audio_element_handle_t el)
{
    char task_name[32];
    bk_err_t ret = BK_FAIL;
    if (el->task_run)
    {
        BK_LOGV(TAG, "[%s-%p] Element already created \n", el->tag, el);
        return BK_OK;
    }
    BK_LOGD(TAG, "[%s] Element starting... \n", el->tag);
    snprintf(task_name, 32, "el-%s", el->tag);
    audio_event_iface_discard(el->iface_event);
    xEventGroupClearBits(el->state_event, TASK_CREATED_BIT);
    if (el->task_stack > 0)
    {
        ret = audio_create_thread(&el->audio_thread,
                                    el->task_prio,
                                    el->tag,
                                    (beken_thread_function_t)audio_element_task,
                                    el->task_stack,
                                    (beken_thread_arg_t)el,
                                    el->task_core);
        if (ret == BK_FAIL)
        {
            audio_element_force_set_state(el, AEL_STATE_ERROR);
            audio_element_report_status(el, AEL_STATUS_ERROR_OPEN);
            BK_LOGE(TAG, "[%s] audio_thread_create failed \n", el->tag);
            return BK_FAIL;
        }
        EventBits_t uxBits = xEventGroupWaitBits(el->state_event, TASK_CREATED_BIT, false, true, DEFAULT_MAX_WAIT_TIME);

        if (uxBits & TASK_CREATED_BIT)
        {
            ret = BK_OK;
        }
    }
    else
    {
        el->task_run = true;
        el->is_running = true;
        audio_element_force_set_state(el, AEL_STATE_RUNNING);
        audio_element_report_status(el, AEL_STATUS_STATE_RUNNING);
        ret = BK_OK;
    }
    BK_LOGD(TAG, "[%s-%p] Element task created \n", el->tag, el);
    return ret;
}

static inline bk_err_t __audio_element_term(audio_element_handle_t el, TickType_t ticks_to_wait)
{
    xEventGroupClearBits(el->state_event, TASK_DESTROYED_BIT);
    if (audio_element_cmd_send(el, AEL_MSG_CMD_DESTROY) != BK_OK)
    {
        BK_LOGE(TAG, "[%s] Send destroy command failed \n", el->tag);
        return BK_FAIL;
    }
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, TASK_DESTROYED_BIT, false, true, ticks_to_wait);
    bk_err_t ret = BK_FAIL;
    if (uxBits & TASK_DESTROYED_BIT)
    {
        BK_LOGV(TAG, "[%s-%p] Element task destroyed \n", el->tag, el);
        ret = BK_OK;
    }
    else
    {
        BK_LOGW(TAG, "[%s-%p] Element task destroy timeout[%d] \n", el->tag, el, ticks_to_wait);
    }
    return ret;
}

bk_err_t audio_element_terminate(audio_element_handle_t el)
{
    if (!el->task_run)
    {
        BK_LOGW(TAG, "[%s] Element has not create when AUDIO_ELEMENT_TERMINATE \n", el->tag);
        return BK_OK;
    }
    if (el->task_stack <= 0)
    {
        el->task_run = false;
        el->is_running = false;
        return BK_OK;
    }
    return __audio_element_term(el, DEFAULT_MAX_WAIT_TIME);
}

bk_err_t audio_element_terminate_with_ticks(audio_element_handle_t el, TickType_t ticks_to_wait)
{
    if (!el->task_run)
    {
        BK_LOGW(TAG, "[%s] Element has not create when AUDIO_ELEMENT_TERMINATE, tick:%d \n", el->tag, ticks_to_wait);
        return BK_OK;
    }
    if (el->task_stack <= 0)
    {
        el->task_run = false;
        el->is_running = false;
        return BK_OK;
    }
    return __audio_element_term(el, ticks_to_wait);
}

bk_err_t audio_element_pause(audio_element_handle_t el)
{
    if (!el->task_run)
    {
        BK_LOGW(TAG, "[%s] Element has not create when AUDIO_ELEMENT_PAUSE \n", el->tag);
        return BK_FAIL;
    }
    if ((el->state >= AEL_STATE_PAUSED))
    {
        audio_element_force_set_state(el, AEL_STATE_PAUSED);
        BK_LOGV(TAG, "[%s] Element already paused, state:%d \n", el->tag, el->state);
        return BK_OK;
    }
    xEventGroupClearBits(el->state_event, PAUSED_BIT);
    if (el->task_stack <= 0)
    {
        el->is_running = false;
        audio_element_force_set_state(el, AEL_STATE_PAUSED);
        return BK_OK;
    }
    if (audio_element_cmd_send(el, AEL_MSG_CMD_PAUSE) != BK_OK)
    {
        BK_LOGE(TAG, "[%s] Element send cmd error when AUDIO_ELEMENT_PAUSE \n", el->tag);
        return BK_FAIL;
    }
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, PAUSED_BIT, false, true, DEFAULT_MAX_WAIT_TIME);
    bk_err_t ret = BK_FAIL;
    if (uxBits & PAUSED_BIT)
    {
        ret = BK_OK;
    }
    return ret;
}

bk_err_t audio_element_resume(audio_element_handle_t el, float wait_for_rb_threshold, TickType_t timeout)
{
    if (!el->task_run)
    {
        BK_LOGW(TAG, "[%s] Element has not create when AUDIO_ELEMENT_RESUME \n", el->tag);
        return BK_FAIL;
    }
    if (el->state == AEL_STATE_RUNNING)
    {
        audio_element_report_status(el, AEL_STATUS_STATE_RUNNING);
        BK_LOGV(TAG, "[%s] RESUME: Element is already running, state:%d, task_run:%d, is_running:%d \n",
                el->tag, el->state, el->task_run, el->is_running);
        return BK_OK;
    }
    if (el->task_stack <= 0)
    {
        //此element没有task，则直接设置运行状态
        el->is_running = true;
        audio_element_force_set_state(el, AEL_STATE_RUNNING);
        audio_element_report_status(el, AEL_STATUS_STATE_RUNNING);
        return BK_OK;
    }
    if (el->state == AEL_STATE_ERROR)
    {
        BK_LOGE(TAG, "[%s] RESUME: Element error, state:%d \n", el->tag, el->state);
        return BK_FAIL;
    }
    if (el->state == AEL_STATE_FINISHED)
    {
        BK_LOGD(TAG, "[%s] RESUME: Element has finished, state:%d \n", el->tag, el->state);
        audio_element_report_status(el, AEL_STATUS_STATE_FINISHED);
        return BK_OK;
    }
    //检查 ringbuffer 门限值是否合理
    if (wait_for_rb_threshold > 1 || wait_for_rb_threshold < 0)
    {
        return BK_FAIL;
    }
    int ret =  BK_OK;
    //清除事件组的RESUMED_BIT位，待task完成后再置为 1
    xEventGroupClearBits(el->state_event, RESUMED_BIT);
    //向task内部的 queue 发送cmd，去设置elsement为resume状态
    if (audio_element_cmd_send(el, AEL_MSG_CMD_RESUME) == BK_FAIL)
    {
        BK_LOGW(TAG, "[%s] Send resume command failed \n", el->tag);
        return BK_FAIL;
    }
    //等待时间组的RESUMED_BIT位被设置为1，等待AEL_MSG_CMD_RESUME指令的执行结果
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, RESUMED_BIT, false, true, timeout);
    if ((uxBits & RESUMED_BIT) != RESUMED_BIT)
    {
        BK_LOGW(TAG, "[%s-%p] RESUME timeout \n", el->tag, el);
        ret = BK_FAIL;
    }
    else
    {
        if (wait_for_rb_threshold != 0 && el->in_type != PORT_TYPE_CB)
        {
            ret = audio_element_wait_for_buffer(el, audio_port_get_size(el->in) * wait_for_rb_threshold, timeout);
        }
    }
    return ret;
}

bk_err_t audio_element_stop(audio_element_handle_t el)
{
    if (!el->task_run)
    {
        BK_LOGV(TAG, "[%s] Element has not create when AUDIO_ELEMENT_STOP \n", el->tag);
        return BK_FAIL;
    }
    if (el->is_running == false)
    {
        xEventGroupSetBits(el->state_event, STOPPED_BIT);
        audio_element_report_status(el, AEL_STATUS_STATE_STOPPED);
        BK_LOGE(TAG, "[%s] Element already stopped \n", el->tag);
        return BK_OK;
    }
    audio_element_abort_output_port(el);
    audio_element_abort_input_port(el);
    if (el->state == AEL_STATE_RUNNING)
    {
        xEventGroupClearBits(el->state_event, STOPPED_BIT);
    }
    if (el->task_stack <= 0)
    {
        el->is_running = false;
        audio_element_force_set_state(el, AEL_STATE_STOPPED);
        xEventGroupSetBits(el->state_event, STOPPED_BIT);
        audio_element_report_status(el, AEL_STATUS_STATE_STOPPED);
        return BK_OK;
    }
    if (el->state == AEL_STATE_PAUSED)
    {
        audio_event_iface_set_cmd_waiting_timeout(el->iface_event, 0);
    }
    if (el->stopping)
    {
        BK_LOGV(TAG, "[%s] Stop command has already sent, %d \n", el->tag, el->stopping);
        return BK_OK;
    }
    el->stopping = true;
    if (audio_element_cmd_send(el, AEL_MSG_CMD_STOP) != BK_OK)
    {
        el->stopping = false;
        BK_LOGW(TAG, "[%s-%p] Send stop command failed \n", el->tag, el);
        return BK_FAIL;
    }
    BK_LOGV(TAG, "[%s-%p] Send stop command \n", el->tag, el);
    return BK_OK;
}

bk_err_t audio_element_wait_for_stop_ms(audio_element_handle_t el, TickType_t ticks_to_wait)
{
    if (el->is_running == false)
    {
        BK_LOGV(TAG, "[%s] Element already stopped, return without waiting \n", el->tag);
        return BK_OK;
    }
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, STOPPED_BIT, false, true, ticks_to_wait);
    bk_err_t ret = BK_ERR_ADK_TIMEOUT;
    if (uxBits & STOPPED_BIT)
    {
        ret = BK_OK;
    }
    return ret;
}

bk_err_t audio_element_multi_input(audio_element_handle_t el, char *buffer, int wanted_size, int index, TickType_t ticks_to_wait)
{
    bk_err_t ret = BK_OK;
    if (index >= el->multi_in.max_port_num)
    {
        BK_LOGE(TAG, "The index of ringbuffer is gather than and equal to audio port maximum (%d). line %d \n", el->multi_in.max_port_num, __LINE__);
        return BK_FAIL;
    }
    if (el->multi_in.port[index])
    {
        ret = audio_port_read(el->multi_in.port[index], buffer, wanted_size, ticks_to_wait);
    }
    return ret;
}

bk_err_t audio_element_multi_output(audio_element_handle_t el, char *buffer, int wanted_size, TickType_t ticks_to_wait)
{
    bk_err_t ret = BK_OK;
    for (int i = 0; i < el->multi_out.max_port_num; i++)
    {
        if (el->multi_out.port[i])
        {
            ret |= audio_port_write(el->multi_out.port[i], buffer, wanted_size, ticks_to_wait);
        }
    }
    return ret;
}

bk_err_t audio_element_set_multi_input_port(audio_element_handle_t el, audio_port_handle_t port, int index)
{
    if ((index < el->multi_in.max_port_num) && port)
    {
        el->multi_in.port[index] = port;
        return BK_OK;
    }
    return BK_ERR_ADK_INVALID_ARG;
}

bk_err_t audio_element_set_multi_output_port(audio_element_handle_t el, audio_port_handle_t port, int index)
{
    if ((index < el->multi_out.max_port_num) && port)
    {
        el->multi_out.port[index] = port;
        return BK_OK;
    }
    return BK_ERR_ADK_INVALID_ARG;
}

audio_port_handle_t audio_element_get_multi_input_port(audio_element_handle_t el, int index)
{
    if (index < el->multi_in.max_port_num)
    {
        return el->multi_in.port[index];
    }
    return NULL;
}

audio_port_handle_t audio_element_get_multi_output_port(audio_element_handle_t el, int index)
{
    if (index < el->multi_out.max_port_num)
    {
        return el->multi_out.port[index];
    }
    return NULL;
}

bk_err_t audio_element_seek(audio_element_handle_t el, void *in_data, int in_size, void *out_data, int *out_size)
{
    bk_err_t ret = BK_OK;
    if (el && el->seek)
    {
        ret = el->seek(el, in_data, in_size, out_data, out_size);
    }
    else
    {
        ret = BK_ERR_ADK_NOT_SUPPORT;
    }
    return ret;
}

bool audio_element_is_stopping(audio_element_handle_t el)
{
    if (el)
    {
        return el->stopping;
    }
    return false;
}

bk_err_t audio_element_update_byte_pos(audio_element_handle_t el, int pos)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.byte_pos += pos;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_byte_pos(audio_element_handle_t el, int pos)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.byte_pos = pos;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_update_total_bytes(audio_element_handle_t el, int total_bytes)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.total_bytes += total_bytes;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_total_bytes(audio_element_handle_t el, int total_bytes)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.total_bytes = total_bytes;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_bps(audio_element_handle_t el, int bit_rate)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.bps = bit_rate;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_codec_fmt(audio_element_handle_t el, int format)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.codec_fmt = format;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_music_info(audio_element_handle_t el, int sample_rates, int channels, int bits)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.sample_rates = sample_rates;
        el->info.channels = channels;
        el->info.bits = bits;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

bk_err_t audio_element_set_duration(audio_element_handle_t el, int duration)
{
    if (el)
    {
        while (xSemaphoreTake((QueueHandle_t)(el->lock), portMAX_DELAY) != pdPASS);
        el->info.duration = duration;
        xSemaphoreGive((QueueHandle_t)(el->lock));
        return BK_OK;
    }
    return BK_FAIL;
}

port_type_t audio_element_get_output_port_type(audio_element_handle_t el)
{
    if (el)
    {
        return el->out_type;
    }

    return 0;
}

port_type_t audio_element_get_input_port_type(audio_element_handle_t el)
{
    if (el)
    {
        return el->in_type;
    }

    return 0;
}

bk_err_t audio_element_set_output_port_type(audio_element_handle_t el, port_type_t type)
{
    if (el)
    {
        return BK_FAIL;
    }

    el->out_type = type;

    return BK_OK;
}

bk_err_t audio_element_set_input_port_type(audio_element_handle_t el, port_type_t type)
{
    if (el)
    {
        return BK_FAIL;
    }

    el->in_type = type;

    return BK_OK;
}

