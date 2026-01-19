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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/fb_port.h>
#include <components/bk_audio/audio_pipeline/framebuf.h>


#define TAG  "FB_PORT"


typedef struct framebuf_port
{
//    uint32_t node_size;
//    uint32_t node_num;
//    uint32_t info_size;

    framebuf_handle_t fb;
} framebuf_port_t;


static bk_err_t _framebuf_port_open(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _framebuf_port_close(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _framebuf_port_abort(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    return fb_abort(fb_port->fb);
}

static bk_err_t _framebuf_port_reset(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    return fb_reset(fb_port->fb);
}

static bk_err_t _framebuf_port_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    framebuf_node_item_t *fb_node_item = NULL;
    int ret = fb_read(fb_port->fb, &fb_node_item, ticks_to_wait);
    if (ret > 0)
    {
        if (ret > len)
        {
            BK_LOGW(TAG, "[%s] %s, frame size:%d > buffer len:%d\n", audio_port_get_tag(self), __func__, ret, len);
            ret = PORT_SIZE_OUT_RANGE;
        }
        else
        {
            os_memcpy(buffer, fb_node_item->fb_node->buffer, fb_node_item->fb_node->length);
        }
    }

    fb_free(fb_port->fb, fb_node_item, 0);

    BK_LOGV(TAG, "[%s] %s, ret:%d\n", audio_port_get_tag(self), __func__, ret);
    return ret;
}

static bk_err_t _framebuf_port_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    framebuf_node_item_t *fb_node_item = NULL;
    int ret = fb_malloc(fb_port->fb, &fb_node_item, ticks_to_wait);
    if (ret > 0)
    {
        if (ret < len)
        {
            BK_LOGW(TAG, "[%s] %s, frame size:%d < buffer len:%d\n", audio_port_get_tag(self), __func__, ret, len);
            /* push framebuffer to free list */
            fb_free(fb_port->fb, fb_node_item, 0);
            ret = PORT_SIZE_OUT_RANGE;
        }
        else
        {
            os_memcpy(fb_node_item->fb_node->buffer, buffer, len);
            fb_node_item->fb_node->length = len;
            ret = fb_write(fb_port->fb, fb_node_item, 0);
        }
    }

    BK_LOGV(TAG, "%s, ret:%d\n", __func__, ret);
    return ret;
}

static bk_err_t _framebuf_port_get_total_node_num(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    return fb_get_total_node_num(fb_port->fb);
}

static bk_err_t _framebuf_port_get_ready_node_num(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    return fb_get_ready_node_num(fb_port->fb);
}

static bk_err_t _framebuf_port_destroy(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    if (fb_port && fb_port->fb)
    {
        fb_destroy(fb_port->fb);
        fb_port->fb = NULL;
    }

    if (fb_port)
    {
        audio_free(fb_port);
       fb_port = NULL;
    }

    return BK_OK;
}

static bk_err_t _framebuf_port_write_done(audio_port_handle_t self)
{
    framebuf_port_t *fb_port = (framebuf_port_t *)audio_port_get_data(self);

    return fb_done_write(fb_port->fb);
}

audio_port_handle_t framebuf_port_init(framebuf_port_cfg_t *config)
{
    framebuf_port_t *fb_port = audio_calloc(1, sizeof(framebuf_port_t));
    AUDIO_MEM_CHECK(TAG, fb_port, return NULL);

    fb_port->fb = fb_create(config->node_size, config->node_num, 4);        //not support info, TODO
    AUDIO_MEM_CHECK(TAG, fb_port->fb, goto fail);

    audio_port_cfg_t cfg = {0};
    cfg.open = _framebuf_port_open;
    cfg.close = _framebuf_port_close;
    cfg.destroy = _framebuf_port_destroy;
    cfg.abort = _framebuf_port_abort;
    cfg.reset = _framebuf_port_reset;
    cfg.read = _framebuf_port_read;
    cfg.write = _framebuf_port_write;
    cfg.write_done = _framebuf_port_write_done;
    cfg.get_size = _framebuf_port_get_total_node_num;
    cfg.get_filled_size  = _framebuf_port_get_ready_node_num;
    audio_port_handle_t port = audio_port_init(&cfg);
    AUDIO_MEM_CHECK(TAG, port, goto fail);
    audio_port_set_type(port, PORT_TYPE_FB);
    audio_port_set_data(port, fb_port);

    BK_LOGD(TAG, "framebuf port init, port:%p\n", port);
    return port;

fail:
    if (fb_port && fb_port->fb)
    {
        fb_destroy(fb_port->fb);
        fb_port->fb = NULL;
    }

    if (fb_port)
    {
        audio_free(fb_port);
       fb_port = NULL;
    }

    return NULL;
}

