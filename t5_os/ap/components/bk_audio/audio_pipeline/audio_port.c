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
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>

#define TAG  "AUD_PORT"
//#define DEFAULT_MAX_WAIT_TIME       (2000/portTICK_RATE_MS)

#if 0
typedef struct
{
    int sample_rates;                           /*!< Sample rates in Hz */
    int channels;                               /*!< Number of audio channel, mono is 1, stereo is 2 */
    int bits;                                   /*!< Bit wide (8, 16, 24, 32 bits) */
    int bps;                                    /*!< Bit per second */
    bk_codec_type_t format;                     /*!< data format (optional) */
} audio_element_info_t;

/**
 *  I/O Element Abstract
 */
typedef struct io_callback
{
    stream_func                 cb;
    void                        *ctx;
} io_callback_t;

typedef enum
{
    IO_TYPE_RB = 1, /* I/O through ringbuffer */
    IO_TYPE_FB = 1, /* I/O through ringbuffer */
    IO_TYPE_CB,     /* I/O through callback */
} io_type_t;

struct audio_port
{
    io_type_t                   type;
    union
    {
        ringbuf_handle_t        rb;
        io_callback_t           cb;
        framebuf_handle_t       fb;
    } in; 
} audio_port_t;
#endif

struct audio_port
{
    port_io_func            open;
    port_io_func            close;
    port_io_func            destroy;
    port_io_func            abort;
    port_io_func            reset;
    port_stream_func        read;
    port_stream_func        write;
    port_io_func            write_done;
    port_io_func            get_size;
    port_io_func            get_filled_size;

    char *                  tag;

    /* PrivateData */
    void *                  data;
    port_type_t             type;

    //int                     malloc_wait_time;
    //int                     read_wait_time;
    //int                     write_wait_time;
};


audio_port_handle_t audio_port_init(audio_port_cfg_t *config)
{
    audio_port_handle_t port = audio_calloc(1, sizeof(struct audio_port));
    AUDIO_MEM_CHECK(TAG, port, return NULL);

    bool _success = ((config->tag ? audio_port_set_tag(port, config->tag) : audio_port_set_tag(port, "unknown_port")) == BK_OK);
    AUDIO_MEM_CHECK(TAG, _success, goto _port_init_failed);

    port->open = config->open;
    port->close = config->close;
    port->destroy = config->destroy;
    port->abort = config->abort;
    port->reset = config->reset;
    port->read = config->read;
    port->write = config->write;
    port->write_done = config->write_done;
    port->get_size = config->get_size;
    port->get_filled_size = config->get_filled_size;

#if 0
    audio_port_set_malloc_timeout(port, portMAX_DELAY);
    audio_port_set_read_timeout(port, portMAX_DELAY);
    audio_port_set_write_timeout(port, portMAX_DELAY);
#endif

    return port;
_port_init_failed:
    if (port->tag)
    {
        audio_port_set_tag(port, NULL);
    }
    audio_free(port);
    return NULL;
}

bk_err_t audio_port_deinit(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_OK);

    if (port->destroy)
    {
        port->destroy(port);
    }

    audio_port_set_tag(port, NULL);
    audio_free(port);
    return BK_OK;
}

bk_err_t audio_port_set_tag(audio_port_handle_t port, const char *tag)
{
    if (port->tag)
    {
        audio_free(port->tag);
        port->tag = NULL;
    }

    if (tag)
    {
        port->tag = audio_strdup(tag);
        AUDIO_MEM_CHECK(TAG, port->tag, return BK_ERR_ADK_NO_MEM);
    }
    return BK_OK;
}

char *audio_port_get_tag(audio_port_handle_t port)
{
    return port->tag;
}

bk_err_t audio_port_open(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->open)
    {
        return port->open(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_close(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->close)
    {
        return port->close(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_abort(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->abort)
    {
        return port->abort(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_reset(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->reset)
    {
        return port->reset(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_read(audio_port_handle_t port, char *buffer, int len, TickType_t ticks_to_wait)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->read)
    {
        return port->read(port, buffer, len, ticks_to_wait, NULL);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_write(audio_port_handle_t port, char *buffer, int len, TickType_t ticks_to_wait)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->write)
    {
        return port->write(port, buffer, len, ticks_to_wait, NULL);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_write_done(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->reset)
    {
        return port->write_done(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_get_size(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->get_size)
    {
        return port->get_size(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_get_filled_size(audio_port_handle_t port)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    if (port->get_filled_size)
    {
        return port->get_filled_size(port);
    }
    else
    {
        return BK_FAIL;
    }
}

bk_err_t audio_port_set_data(audio_port_handle_t port, void *data)
{
    port->data = data;
    return BK_OK;
}

void *audio_port_get_data(audio_port_handle_t port)
{
    return port->data;
}

bk_err_t audio_port_set_type(audio_port_handle_t port, port_type_t type)
{
    port->type = type;
    return BK_OK;
}

bk_err_t audio_port_get_type(audio_port_handle_t port)
{
    return port->type;
}

#if 0
bk_err_t audio_port_set_malloc_timeout(audio_port_handle_t port, TickType_t timeout)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    port->malloc_wait_time = timeout;
    return BK_OK;
}

bk_err_t audio_port_set_read_timeout(audio_port_handle_t port, TickType_t timeout)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    port->read_wait_time = timeout;
    return BK_OK;
}

bk_err_t audio_port_set_write_timeout(audio_port_handle_t port, TickType_t timeout)
{
    AUDIO_MEM_CHECK(TAG, port, return BK_ERR_ADK_INVALID_PARAMETER);

    port->write_wait_time = timeout;
    return BK_OK;
}
#endif

