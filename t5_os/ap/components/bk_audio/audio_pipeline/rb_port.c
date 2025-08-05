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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_pipeline/ringbuf.h>


#define TAG  "RB_PORT"


typedef struct ringbuf_port
{
    uint32_t ringbuf_size;
    ringbuf_handle_t rb;
} ringbuf_port_t;


static bk_err_t _ringbuf_port_open(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _ringbuf_port_close(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _ringbuf_port_abort(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    return rb_abort(rb_port->rb);
}

static bk_err_t _ringbuf_port_reset(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    return rb_reset(rb_port->rb);
}

static bk_err_t _ringbuf_port_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    //BK_LOGD(TAG, "[%s] %s, len: %d\n", audio_port_get_tag(self), __func__, node_data->buff_size);

    int ret = rb_read(rb_port->rb, buffer, len, ticks_to_wait);
    if (ret < 0)
    {
        BK_LOGV(TAG, "[%s] ringbuf port read fail, ret:%d\n", audio_port_get_tag(self), ret);
    }

    return ret;
}

static bk_err_t _ringbuf_port_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    //BK_LOGD(TAG, "[%s] %s, len: %d\n", audio_port_get_tag(self), __func__, node_data->length);

    int ret = rb_write(rb_port->rb, (char *)buffer, len, ticks_to_wait);
    if (ret != len)
    {
        BK_LOGV(TAG, "[%s] ringbuf port write fail, ret:%d != leng:%d\n", audio_port_get_tag(self), ret, len);
    }

    return ret;
}

static bk_err_t _ringbuf_port_write_done(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    //BK_LOGD(TAG, "[%s] %s, len: %d\n", audio_port_get_tag(self), __func__, node_data->length);

    return rb_done_write(rb_port->rb);
}

static bk_err_t _ringbuf_port_get_size(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    //BK_LOGD(TAG, "[%s] %s, len: %d\n", audio_port_get_tag(self), __func__, node_data->length);

    return rb_get_size(rb_port->rb);
}

static bk_err_t _ringbuf_port_get_filled_size(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    //BK_LOGD(TAG, "[%s] %s, len: %d\n", audio_port_get_tag(self), __func__, node_data->length);

    return rb_bytes_filled(rb_port->rb);
}

static bk_err_t _ringbuf_port_destroy(audio_port_handle_t self)
{
    ringbuf_port_t *rb_port = (ringbuf_port_t *)audio_port_get_data(self);

    if (rb_port && rb_port->rb)
    {
        rb_destroy(rb_port->rb);
        rb_port->rb = NULL;
    }

    if (rb_port)
    {
        audio_free(rb_port);
        rb_port = NULL;
    }

    return BK_OK;
}

audio_port_handle_t ringbuf_port_init(ringbuf_port_cfg_t *config)
{
    ringbuf_port_t *rb_port = audio_calloc(1, sizeof(ringbuf_port_t));
    AUDIO_MEM_CHECK(TAG, rb_port, return NULL);

    rb_port->ringbuf_size = config->ringbuf_size;
    rb_port->rb = rb_create(rb_port->ringbuf_size, 1);
    AUDIO_MEM_CHECK(TAG, rb_port->rb, goto fail);

    audio_port_cfg_t cfg = {0};
    cfg.open = _ringbuf_port_open;
    cfg.close = _ringbuf_port_close;
    cfg.destroy = _ringbuf_port_destroy;
    cfg.abort = _ringbuf_port_abort;
    cfg.reset = _ringbuf_port_reset;
    cfg.read = _ringbuf_port_read;
    cfg.write = _ringbuf_port_write;
    cfg.write_done = _ringbuf_port_write_done;
    cfg.get_size = _ringbuf_port_get_size;
    cfg.get_filled_size = _ringbuf_port_get_filled_size;

    audio_port_handle_t port = audio_port_init(&cfg);
    AUDIO_MEM_CHECK(TAG, port, goto fail);
    audio_port_set_type(port, PORT_TYPE_RB);
    audio_port_set_data(port, rb_port);

    BK_LOGD(TAG, "ringbuf port init, port:%p\n", port);
    return port;

fail:
    if (rb_port && rb_port->rb)
    {
        rb_destroy(rb_port->rb);
        rb_port->rb = NULL;
    }

    if (rb_port)
    {
        audio_free(rb_port);
        rb_port = NULL;
    }

    return NULL;
}

