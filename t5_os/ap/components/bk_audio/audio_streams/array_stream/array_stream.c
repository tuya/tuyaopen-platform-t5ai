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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/array_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>


#define TAG  "ARRAY_STR"

//#define ARRAY_DEBUG   //GPIO debug

#ifdef ARRAY_DEBUG

#define ARRAY_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define ARRAY_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define ARRAY_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define ARRAY_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define ARRAY_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define ARRAY_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define ARRAY_PROCESS_START()
#define ARRAY_PROCESS_END()

#define ARRAY_INPUT_START()
#define ARRAY_INPUT_END()

#define ARRAY_OUTPUT_START()
#define ARRAY_OUTPUT_END()

#endif


typedef struct array_stream
{
    audio_stream_type_t type;
    char *              array_buf;      /**< Array buffer ptr */
    uint32_t            array_size;     /**< Array total size */
    uint32_t            array_offset;   /**< Array offset */
} array_stream_t;

static bk_err_t _array_open(audio_element_handle_t self)
{
    bk_err_t ret = BK_OK;
    array_stream_t *array = (array_stream_t *)audio_element_getdata(self);

    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    array->array_offset = 0;

    BK_LOGD(TAG, "array->type: %d, array->array_buf: %p, size: %d \n", array->type, array->array_buf, array->array_size);

    audio_element_set_input_timeout(self, 40 / portTICK_RATE_MS);
    //audio_element_set_output_timeout(self, 40 / portTICK_RATE_MS);

    //BK_LOGD(TAG, "array size: 0x%x%x byte, array position: 0x%x%x \n", (int)(info.total_bytes >> 32), (int)info.total_bytes, (int)(info.byte_pos >> 32), (int)info.byte_pos);
    if (info.byte_pos > 0)
    {
        //array->array_offset = info.byte_pos;
    }

    return ret;
}

static int _array_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    array_stream_t *array = (array_stream_t *)audio_element_getdata(el);

    ARRAY_INPUT_START();

    if (len <= 0 || array->array_offset >= array->array_size)
    {
        BK_LOGD(TAG, "[%s] %s, array_offset: %d, array_size: %d \n", audio_element_get_tag(el), __func__, array->array_offset, array->array_size);
        return 0;
    }

    int rlen = 0;
    if (len > (array->array_size - array->array_offset))
    {
        rlen = array->array_size - array->array_offset;
    }
    else
    {
        rlen = len;
    }
    //BK_LOGD(TAG, "[%s] %s, rlen: %d, array_offset: %d, array_size: %d \n", audio_element_get_tag(el), __func__, rlen, array->array_offset, array->array_size);
    os_memcpy(buffer, &array->array_buf[array->array_offset], rlen);
    array->array_offset += rlen;

    audio_element_update_byte_pos(el, rlen);

    ARRAY_INPUT_END();

    return rlen;
}

static int _array_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    array_stream_t *array = (array_stream_t *)audio_element_getdata(el);

    ARRAY_OUTPUT_START();

    if (len <= 0 || array->array_offset >= array->array_size)
    {
        BK_LOGD(TAG, "[%s] %s, array_offset: %d, array_size: %d \n", audio_element_get_tag(el), __func__, array->array_offset, array->array_size);
        return 0;
    }

    int wlen = 0;
    if (len > (array->array_size - array->array_offset))
    {
        wlen = array->array_size - array->array_offset;
    }
    else
    {
        wlen = len;
    }

    //BK_LOGD(TAG, "[%s] %s, wlen: %d, array_offset: %d, array_size: %d \n", audio_element_get_tag(el), __func__, wlen, array->array_offset, array->array_size);
    os_memcpy(&array->array_buf[array->array_offset], buffer, wlen);
    array->array_offset += wlen;

    audio_element_update_byte_pos(el, wlen);

    ARRAY_OUTPUT_END();

    return wlen;
}

static int _array_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    ARRAY_PROCESS_END();

    int r_size = audio_element_input(self, in_buffer, in_len);
    //BK_LOGD(TAG, "[%s] %s r_size: %d \n", audio_element_get_tag(self), __func__, r_size);
    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    ARRAY_PROCESS_END();

    return w_size;
}

static bk_err_t _array_close(audio_element_handle_t self)
{
    array_stream_t *array = (array_stream_t *)audio_element_getdata(self);

    if (AEL_STATE_PAUSED != audio_element_get_state(self))
    {
        if (array->type == AUDIO_STREAM_READER)
        {
            array->array_offset = array->array_size;
        }
        else
        {
            array->array_offset = 0;
        }
        audio_element_set_byte_pos(self, 0);
    }
    return BK_OK;
}

static bk_err_t _array_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    array_stream_t *array = (array_stream_t *)audio_element_getdata(self);
    if (array)
    {
        audio_free(array);
        array = NULL;
    }

    return BK_OK;
}

audio_element_handle_t array_stream_init(array_stream_cfg_t *config)
{
    audio_element_handle_t el;
    array_stream_t *array = audio_calloc(1, sizeof(array_stream_t));

    AUDIO_MEM_CHECK(TAG, array, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _array_open;
    cfg.close = _array_close;
    cfg.process = _array_process;
    cfg.destroy = _array_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "array";
    array->type = config->type;

    if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _array_write;
        cfg.in_type = PORT_TYPE_RB;
    }
    else
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _array_read;
        cfg.out_type = PORT_TYPE_RB;
    }
    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _array_init_exit);
    audio_element_setdata(el, array);

    return el;
_array_init_exit:
    if (el)
    {
        audio_element_deinit(el);
    }
    if (array)
    {
        audio_free(array);
        array = NULL;
    }
    return NULL;
}

bk_err_t array_stream_set_data(audio_element_handle_t array_stream, uint8_t *array_buf, int array_size)
{
    array_stream_t *array = (array_stream_t *)audio_element_getdata(array_stream);

    AUDIO_MEM_CHECK(TAG, array, return BK_FAIL);

    BK_LOGD(TAG, "[%s] %s, array_buf: %p, array_size: %d \n", audio_element_get_tag(array_stream), __func__, array_buf, array_size);

    if (array)
    {
        array->array_buf = (char *)array_buf;
        array->array_size = array_size;
    }

    return BK_OK;
}
