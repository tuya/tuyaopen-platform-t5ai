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

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <components/bk_audio/audio_utils/vfs_util.h>
#include "bk_posix.h"

#define VFS_UTIL_TAG "vfs_util"

#define LOGI(...) BK_LOGI(VFS_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(VFS_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(VFS_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(VFS_UTIL_TAG, ##__VA_ARGS__)


static bk_err_t config_file_name(char *file, char *name)
{
    if (file == NULL && name == NULL)
    {
        LOGE("%s, %d, param is NULL, file: %p, name: %p \n", __func__, __LINE__, file, name);
        return BK_FAIL;
    }

    if (os_strlen(name) + 1 > 50)
    {
        LOGE("%s, %d, name length: %d > 49 \n", __func__, __LINE__, os_strlen(name));
        return BK_FAIL;
    }

    sprintf(file, "%s", name);

    return BK_OK;
}

bk_err_t vfs_util_create(vfs_util_handle_t vfs_util, char *name)
{
    if (!name || !vfs_util)
    {
        LOGE("%s, %d, vfs_util: %p, name: %s \n", __func__, __LINE__, vfs_util, name);
        return BK_FAIL;
    }

    /* config file name */
    config_file_name(vfs_util->file_name, name);

    vfs_util->fd = open(vfs_util->file_name, O_CREAT | O_WRONLY);
    if (vfs_util->fd < 0)
    {
        LOGE("can't open %s\n", vfs_util->file_name);
        return BK_FAIL;
    }

    LOGD("open %s ok \n", vfs_util->file_name);

    return BK_OK;
}

bk_err_t vfs_util_destroy(vfs_util_handle_t vfs_util)
{
    if (!vfs_util)
    {
        LOGE("%s, %d, vfs_util is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* close sin file */
    int ret = close(vfs_util->fd);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, close %s fail \n", __func__, __LINE__, vfs_util->file_name);
    }

    LOGE("close %s ok \n", __func__, __LINE__, vfs_util->file_name);

    return BK_OK;
}

bk_err_t vfs_util_tx_data(vfs_util_handle_t vfs_util, void *data_buf, uint32_t len)
{
    if (!vfs_util || !data_buf || len == 0)
    {
        LOGE("%s, %d, vfs_util: %p, data_buf: %p, len: %d \n", __func__, __LINE__, vfs_util, data_buf, len);
        return BK_FAIL;
    }

    int ret = write(vfs_util->fd, data_buf, len);
    if (ret != len)
    {
        LOGE("%s, %d, write fail, ret: %d \n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

