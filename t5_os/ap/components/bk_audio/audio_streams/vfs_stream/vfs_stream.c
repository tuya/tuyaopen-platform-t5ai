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
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include "bk_posix.h"

#define TAG  "VFS_STR"

//#define VFS_DEBUG   //GPIO debug

#ifdef VFS_DEBUG

#define VFS_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define VFS_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define VFS_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define VFS_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define VFS_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define VFS_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define VFS_PROCESS_START()
#define VFS_PROCESS_END()

#define VFS_INPUT_START()
#define VFS_INPUT_END()

#define VFS_OUTPUT_START()
#define VFS_OUTPUT_END()

#endif


typedef struct vfs_stream
{
    audio_stream_type_t type;
    bool is_open;
    int fd;
} vfs_stream_t;

static bk_err_t _vfs_open(audio_element_handle_t self)
{
    bk_err_t ret = BK_OK;
    vfs_stream_t *vfs = (vfs_stream_t *)audio_element_getdata(self);

    audio_element_info_t info;
    char *uri = audio_element_get_uri(self);
    if (uri == NULL)
    {
        BK_LOGE(TAG, "Error, uri is not set \n");
        return BK_FAIL;
    }
    char *path = strstr(uri, VFS_SD_0_PATITION_0);
    BK_LOGD(TAG, "_fatfs_open, uri:%s \n", uri);
    audio_element_getinfo(self, &info);
    if (path == NULL)
    {
        BK_LOGE(TAG, "Error, need file path to open \n");
        return BK_FAIL;
    }
    if (vfs->is_open)
    {
        BK_LOGE(TAG, "already opened \n");
        return BK_FAIL;
    }
    if (vfs->type == AUDIO_STREAM_READER)
    {
        struct stat statbuf;
        ret = stat(path, &statbuf);
        if (ret < 0) {
            BK_LOGE(TAG, "stat file %s fail\n", path);
            return BK_FAIL;
        }
        BK_LOGV(TAG, "statbuf->st_size =%d, statbuf->st_mode = %d.\r\n", statbuf.st_size , statbuf.st_mode);
        info.total_bytes = (uint32_t)statbuf.st_size;// total byte

        vfs->fd = open(path, O_RDONLY);
        BK_LOGV(TAG, "vfs->fd = %d.\n", vfs->fd);
        if (vfs->fd < 0) {
            BK_LOGE(TAG, "can't open %s\n", path);
            return BK_FAIL;
        }

        BK_LOGV(TAG, "File size: 0x%x%x byte, file position: 0x%x%x \n", (int)(info.total_bytes >> 32), (int)info.total_bytes, (int)(info.byte_pos >> 32), (int)info.byte_pos);
        if (info.byte_pos > 0)
        {
            if (lseek(vfs->fd, info.byte_pos, SEEK_SET) < 0)
            {
                return BK_FAIL;
            }
        }

    }
    else if (vfs->type == AUDIO_STREAM_WRITER)
    {
        vfs->fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
        if (vfs->fd < 0) {
            BK_LOGE(TAG, "[%s] line %d, Failed to open %s, fd: %d\n",audio_element_get_tag(self), __LINE__, path, vfs->fd);
            return BK_FAIL;
        }
    }
    else
    {
        BK_LOGE(TAG, "FATFS must be Reader or Writer \n");
        return BK_FAIL;
    }
    vfs->is_open = true;
    ret = audio_element_set_total_bytes(self, info.total_bytes);
    return ret;
}

static int _vfs_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] %s, len: %d \n", audio_element_get_tag(el), __func__, len);

    vfs_stream_t *vfs = (vfs_stream_t *)audio_element_getdata(el);
    audio_element_info_t info;

    audio_element_getinfo(el, &info);

    VFS_INPUT_START();

    BK_LOGV(TAG, "[%s] read len=%d, pos=%d/%d, vfs->fd=%d. \n", audio_element_get_tag(el), len, (int)info.byte_pos, (int)info.total_bytes, vfs->fd);
    /* use file descriptors to access files */
    int rlen = 0;
    rlen = read(vfs->fd, (char *)buffer, len);
    if (rlen == 0)
    {
        BK_LOGW(TAG, "No more data, ret:%d \n", rlen);
    }
    else
    {
        BK_LOGV(TAG, "read success, rlen:%d \n", rlen);
        audio_element_update_byte_pos(el, rlen);
    }

    VFS_INPUT_END();

    return rlen;
}

static int _vfs_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    vfs_stream_t *vfs = (vfs_stream_t *)audio_element_getdata(el);
    audio_element_info_t info;
    audio_element_getinfo(el, &info);

    VFS_OUTPUT_START();

    int wlen = 0;
    wlen = write(vfs->fd, buffer, len);
    BK_LOGV(TAG, "[%s] %s, len: %d, wlen = %d. \n", audio_element_get_tag(el), __func__, len, wlen);
    if (wlen == len) {
        audio_element_update_byte_pos(el, wlen);
    }

    VFS_OUTPUT_END();

    return wlen;
}

static int _vfs_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    VFS_PROCESS_START();

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

    VFS_PROCESS_END();

    return w_size;
}

static bk_err_t _vfs_close(audio_element_handle_t self)
{
    vfs_stream_t *vfs = (vfs_stream_t *)audio_element_getdata(self);
    if (vfs->is_open)
    {
        int ret = close(vfs->fd);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] Failed to vfs close, ret: %d. line: %d \n", audio_element_get_tag(self), ret, __LINE__);
        }

        vfs->is_open = false;
        vfs->fd = 0;
    }
    if (AEL_STATE_PAUSED != audio_element_get_state(self))
    {
        audio_element_report_info(self);
        audio_element_set_byte_pos(self, 0);
    }
    return BK_OK;
}

static bk_err_t _vfs_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    vfs_stream_t *vfs = (vfs_stream_t *)audio_element_getdata(self);
    if (vfs)
    {
        audio_free(vfs);
        vfs = NULL;
    }

    return BK_OK;
}

audio_element_handle_t vfs_stream_init(vfs_stream_cfg_t *config)
{
    audio_element_handle_t el;
    vfs_stream_t *vfs = audio_calloc(1, sizeof(vfs_stream_t));

    AUDIO_MEM_CHECK(TAG, vfs, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _vfs_open;
    cfg.close = _vfs_close;
    cfg.process = _vfs_process;
    cfg.destroy = _vfs_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "file";
    vfs->type = config->type;

    if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.write = _vfs_write;
        cfg.in_type = PORT_TYPE_RB;
        cfg.out_type = PORT_TYPE_CB;
    }
    else
    {
        cfg.read = _vfs_read;
        cfg.in_type = PORT_TYPE_CB;
        cfg.out_type = PORT_TYPE_RB;
    }
    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _vfs_init_exit);
    audio_element_setdata(el, vfs);

    return el;
_vfs_init_exit:
    if (el)
    {
        audio_element_deinit(el);
    }
    if (vfs)
    {
        audio_free(vfs);
        vfs = NULL;
    }
    return NULL;
}
