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
#include <components/bk_audio/audio_streams/frame_array_stream.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>


#define TAG  "FRAME_ARRAY_STR"

//#define FRAME_ARRAY_DEBUG   //GPIO debug

#ifdef FRAME_ARRAY_DEBUG

#define FRAME_ARRAY_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define FRAME_ARRAY_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define FRAME_ARRAY_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define FRAME_ARRAY_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define FRAME_ARRAY_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define FRAME_ARRAY_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define FRAME_ARRAY_PROCESS_START()
#define FRAME_ARRAY_PROCESS_END()

#define FRAME_ARRAY_INPUT_START()
#define FRAME_ARRAY_INPUT_END()

#define FRAME_ARRAY_OUTPUT_START()
#define FRAME_ARRAY_OUTPUT_END()

#endif


typedef struct frame_array_stream
{
    audio_stream_type_t type;
    char *              data_buf;               /**< Frame Array data buffer ptr */
    uint32_t            data_size;              /**< Frame Array data total size */
    uint16_t *          data_len_buf;           /**< Frame Array data length buffer ptr */
    uint32_t            data_len_array_size;    /**< Frame Array data length total size */
    uint32_t            data_offset;            /**< Frame Array data offset */
    uint32_t            data_len_array_offset;  /**< Frame Array data length array offset */
} frame_array_stream_t;

static bk_err_t _frame_array_open(audio_element_handle_t self)
{
    bk_err_t ret = BK_OK;
    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(self);

    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    frame_array->data_offset = 0;
    frame_array->data_len_array_offset = 0;

    BK_LOGD(TAG, "frame_array->type: %d, frame_array->data_buf: %p, size: %d \n", frame_array->type, frame_array->data_buf, frame_array->data_size);
    BK_LOGD(TAG, "frame_array->data_len_buf: %p, data_len_array_size: %d \n", frame_array->data_len_buf, frame_array->data_len_array_size);

    audio_element_set_input_timeout(self, 40 / portTICK_RATE_MS);

    if (info.byte_pos > 0)
    {
        
    }

    return ret;
}

static int _frame_array_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(el);

    FRAME_ARRAY_INPUT_START();

    if (len <= 0 || frame_array->data_offset >= frame_array->data_size || frame_array->data_len_array_offset >= frame_array->data_len_array_size)
    {
        BK_LOGD(TAG, "[%s] %s, data_offset: %d, data_size: %d,data_len_array_offset:%d, data_len_array_size:%d \n", 
               audio_element_get_tag(el), __func__, 
               frame_array->data_offset, frame_array->data_size,
               frame_array->data_len_array_offset,frame_array->data_len_array_size);
        return 0;
    }

    int rlen = frame_array->data_len_buf[frame_array->data_len_array_offset];

    BK_LOGV(TAG, "[%s] %s, rlen: %d, data_offset: %d, data_size: %d, data_len_array_offset: %d, data_len_array_size: %d \n", 
            audio_element_get_tag(el), __func__, rlen, 
            frame_array->data_offset, frame_array->data_size,
            frame_array->data_len_array_offset, frame_array->data_len_array_size);
    
    os_memcpy(buffer, &frame_array->data_buf[frame_array->data_offset], rlen);
    frame_array->data_offset += rlen;
    frame_array->data_len_array_offset += 1;

    audio_element_update_byte_pos(el, rlen);

    FRAME_ARRAY_INPUT_END();

    return rlen;
}

static int _frame_array_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(el);

    FRAME_ARRAY_OUTPUT_START();

    if (len <= 0 || frame_array->data_offset >= frame_array->data_size || frame_array->data_len_array_offset >= frame_array->data_len_array_size)
    {
        BK_LOGD(TAG, "[%s] %s, data_offset: %d, data_size: %d,data_len_array_offset:%d, data_len_array_size:%d \n", 
               audio_element_get_tag(el), __func__, 
               frame_array->data_offset, frame_array->data_size,
               frame_array->data_len_array_offset,frame_array->data_len_array_size);
        return 0;
    }

    int wlen = frame_array->data_len_buf[frame_array->data_len_array_offset];

    BK_LOGV(TAG, "[%s] %s, wlen: %d, data_offset: %d, data_size: %d, data_len_array_offset: %d, data_len_array_size: %d \n", 
            audio_element_get_tag(el), __func__, wlen, 
            frame_array->data_offset, frame_array->data_size,
            frame_array->data_len_array_offset, frame_array->data_len_array_size);
    
    
    os_memcpy(&frame_array->data_buf[frame_array->data_offset], buffer, wlen);
    frame_array->data_offset += wlen;
    frame_array->data_len_array_offset += 1;

    audio_element_update_byte_pos(el, wlen);

    FRAME_ARRAY_OUTPUT_END();

    return wlen;
}

static int _frame_array_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    FRAME_ARRAY_PROCESS_END();

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

    BK_LOGV(TAG, "[%s] %s r_size:%d,w_size:%d\n", audio_element_get_tag(self), __func__,r_size,w_size);

    FRAME_ARRAY_PROCESS_END();

    return w_size;
}

static bk_err_t _frame_array_close(audio_element_handle_t self)
{
    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(self);

    if (AEL_STATE_PAUSED != audio_element_get_state(self))
    {
        if (frame_array->type == AUDIO_STREAM_READER)
        {
            frame_array->data_offset = frame_array->data_size;
        }
        else
        {
            frame_array->data_offset = 0;
        }
        audio_element_set_byte_pos(self, 0);
    }
    return BK_OK;
}

static bk_err_t _frame_array_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(self);
    if (frame_array)
    {
        audio_free(frame_array);
        frame_array = NULL;
    }

    return BK_OK;
}

audio_element_handle_t frame_array_stream_init(frame_array_stream_cfg_t *config)
{
    audio_element_handle_t el;
    frame_array_stream_t *frame_array = audio_calloc(1, sizeof(frame_array_stream_t));

    AUDIO_MEM_CHECK(TAG, frame_array, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _frame_array_open;
    cfg.close = _frame_array_close;
    cfg.process = _frame_array_process;
    cfg.destroy = _frame_array_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "frame_array";
    frame_array->type = config->type;

    if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _frame_array_write;
        cfg.in_type = PORT_TYPE_FB;
    }
    else
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _frame_array_read;
        cfg.out_type = PORT_TYPE_FB;
    }
    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _frame_array_init_exit);
    audio_element_setdata(el, frame_array);

    return el;
_frame_array_init_exit:
    if (el)
    {
        audio_element_deinit(el);
    }
    if (frame_array)
    {
        audio_free(frame_array);
        frame_array = NULL;
    }
    return NULL;
}

bk_err_t frame_array_stream_set_data(audio_element_handle_t frame_array_stream, uint8_t *data_buf, int data_size, uint16_t *data_len_buf, int data_len_array_size)
{
    frame_array_stream_t *frame_array = (frame_array_stream_t *)audio_element_getdata(frame_array_stream);

    AUDIO_MEM_CHECK(TAG, frame_array, return BK_FAIL);

    BK_LOGD(TAG, "[%s] %s, array_buf: %p, array_size: %d \n", audio_element_get_tag(frame_array_stream), __func__, data_buf, data_size);

    if (data_buf && data_len_buf)
    {
        frame_array->data_buf = (char *)data_buf;
        frame_array->data_size = data_size;
        frame_array->data_len_buf = data_len_buf;
        frame_array->data_len_array_size = data_len_array_size;
    }

    return BK_OK;
}
