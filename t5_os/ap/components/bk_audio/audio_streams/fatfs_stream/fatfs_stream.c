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
#include <components/bk_audio/audio_streams/fatfs_stream.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include "ff.h"
#include "diskio.h"


#define TAG  "FTFS_STR"

//#define FATFS_DEBUG   //GPIO debug

#ifdef FATFS_DEBUG

#define FATFS_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define FATFS_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define FATFS_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define FATFS_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define FATFS_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define FATFS_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define FATFS_PROCESS_START()
#define FATFS_PROCESS_END()

#define FATFS_INPUT_START()
#define FATFS_INPUT_END()

#define FATFS_OUTPUT_START()
#define FATFS_OUTPUT_END()

#endif


typedef struct fatfs_stream
{
    audio_stream_type_t type;
    bool is_open;
    FIL *file;
    //    bool write_header;
} fatfs_stream_t;

static bk_err_t _fatfs_open(audio_element_handle_t self)
{
    bk_err_t ret = BK_OK;
    fatfs_stream_t *fatfs = (fatfs_stream_t *)audio_element_getdata(self);
    FRESULT fr;

    audio_element_info_t info;
    char *uri = audio_element_get_uri(self);
    if (uri == NULL)
    {
        BK_LOGE(TAG, "Error, uri is not set \n");
        return BK_FAIL;
    }
    char *path = strstr(uri, "1:/");
    BK_LOGD(TAG, "_fatfs_open, uri:%s \n", uri);
    audio_element_getinfo(self, &info);
    if (path == NULL)
    {
        BK_LOGE(TAG, "Error, need file path to open \n");
        return BK_FAIL;
    }
    if (fatfs->is_open)
    {
        BK_LOGE(TAG, "already opened \n");
        return BK_FAIL;
    }
    if (fatfs->type == AUDIO_STREAM_READER)
    {
        fr = f_open(*fp, path, 0x01);
        if (fr != BK_OK)
        {
            BK_LOGE(TAG, "[%s] Failed to open. File name: %s, error: %d, line: %d \n", audio_element_get_tag(self), path, fr, __LINE__);
            return BK_FAIL;
        }

        info.total_bytes = (int64_t)f_size(fatfs->file);
        BK_LOGD(TAG, "File size: 0x%x%x byte, file position: 0x%x%x \n", (int)(info.total_bytes >> 32), (int)info.total_bytes, (int)(info.byte_pos >> 32), (int)info.byte_pos);
        if (info.byte_pos > 0)
        {
            if (f_lseek(fatfs->file, info.byte_pos) < 0)
            {
                return BK_FAIL;
            }
        }
    }
    else if (fatfs->type == AUDIO_STREAM_WRITER)
    {
        fr = f_open(fatfs->file, path, 0x08 | 0x02);
        if (fr != BK_OK)
        {
            BK_LOGE(TAG, "[%s] Failed to open: %s, error: %d, line: %d \n", audio_element_get_tag(self), path, f_error(fatfs->file), __LINE__);
            return BK_FAIL;
        }
    }
    else
    {
        BK_LOGE(TAG, "FATFS must be Reader or Writer \n");
        return BK_FAIL;
    }
    fatfs->is_open = true;
    ret = audio_element_set_total_bytes(self, info.total_bytes);
    return ret;
}

static int _fatfs_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGV(TAG, "[%s] _fatfs_read, len: %d \n", audio_element_get_tag(self), len);
    FRESULT fr;

    fatfs_stream_t *fatfs = (fatfs_stream_t *)audio_element_getdata(self);
    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    FATFS_INPUT_START();

    BK_LOGV(TAG, "[%s] read len=%d, pos=%d/%d \n", audio_element_get_tag(self), len, (int)info.byte_pos, (int)info.total_bytes);
    /* use file descriptors to access files */
    int rlen = 0;
    fr = f_read(fatfs->file, buffer, len, &rlen);
    if (fr != BK_OK)
    {
        BK_LOGE(TAG, "[%s] The error is happened in reading data. Error: %s, line: %d \n", audio_element_get_tag(self), f_error(fatfs->file), __LINE__);
        rlen = -1;
    }

    if (rlen == 0)
    {
        BK_LOGW(TAG, "No more data, ret:%d \n", rlen);
    }
    else
    {
        audio_element_update_byte_pos(self, rlen);
    }

    FATFS_INPUT_END();

    return rlen;
}

static int _fatfs_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    fatfs_stream_t *fatfs = (fatfs_stream_t *)audio_element_getdata(self);
    FRESULT fr;
    audio_element_info_t info;
    audio_element_getinfo(self, &info);
    BK_LOGV(TAG, "[%s] _fatfs_write len: %d \n", audio_element_get_tag(self), len);

    FATFS_OUTPUT_START();

    int wlen = 0;
    fr = f_write(fatfs->file, buffer, len, &wlen);
    if (fr != BK_OK)
    {
        BK_LOGE(TAG, "[%s] writing data error. Error: %s, line: %d \n", audio_element_get_tag(self), f_error(fatfs->file), __LINE__);
        wlen = -1;
    }
    else
    {
        audio_element_update_byte_pos(self, wlen);
    }

    FATFS_OUTPUT_END();

    return wlen;
}

static int _fatfs_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    FATFS_PROCESS_START();

    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    FATFS_PROCESS_END();

    return w_size;
}

static bk_err_t _fatfs_close(audio_element_handle_t self)
{
    fatfs_stream_t *fatfs = (fatfs_stream_t *)audio_element_getdata(self);

    if (fatfs->is_open)
    {
        FRESULT fr = f_close(fatfs->file);
        if (fr != BK_OK)
        {
            BK_LOGE(TAG, "[%s] Failed to fatfs close, fr: %d. line: %d \n", audio_element_get_tag(self), f_error(fatfs->file), __LINE__);
        }
        fatfs->is_open = false;
        fatfs->file = NULL;
    }
    if (AEL_STATE_PAUSED != audio_element_get_state(self))
    {
        audio_element_report_info(self);
        audio_element_set_byte_pos(self, 0);
    }
    return BK_OK;
}

static bk_err_t _fatfs_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    fatfs_stream_t *fatfs = (fatfs_stream_t *)audio_element_getdata(self);
    if (fatfs)
    {
        audio_free(fatfs);
        fatfs = NULL;
    }

    return BK_OK;
}

audio_element_handle_t fatfs_stream_init(fatfs_stream_cfg_t *config)
{
    audio_element_handle_t el;
    fatfs_stream_t *fatfs = audio_calloc(1, sizeof(fatfs_stream_t));

    AUDIO_MEM_CHECK(TAG, fatfs, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _fatfs_open;
    cfg.close = _fatfs_close;
    cfg.process = _fatfs_process;
    cfg.destroy = _fatfs_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_type = PORT_TYPE_RB;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "file";
    fatfs->type = config->type;

    if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _fatfs_write;
    }
    else
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _fatfs_read;
    }
    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _fatfs_init_exit);
    audio_element_setdata(el, fatfs);

    return el;
_fatfs_init_exit:
    if (el)
    {
        audio_element_deinit(el);
    }
    if (fatfs)
    {
        audio_free(fatfs);
        fatfs = NULL;
    }
    return NULL;
}
