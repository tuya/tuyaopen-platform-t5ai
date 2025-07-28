// Copyright 2024-2025 Beken
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
#include "tfcard_util.h"
#include "ff.h"
#include "diskio.h"


#define TFCARD_UTIL_TAG "tfcard_util"

#define LOGI(...) BK_LOGI(TFCARD_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TFCARD_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TFCARD_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TFCARD_UTIL_TAG, ##__VA_ARGS__)




static bk_err_t config_file_name(char *file, char *name)
{
    if (file == NULL && name == NULL)
    {
        LOGE("%s, %d, param is NULL, file: %p, name: %p \n", __func__, __LINE__, file, name);
        return BK_FAIL;
    }

    if (os_strlen(name) + os_strlen("1:/") + 1 > 50)
    {
        LOGE("%s, %d, name length: %d > 46 \n", __func__, __LINE__, os_strlen(name));
        return BK_FAIL;
    }

    sprintf(file, "1:/%s", name);

    return BK_OK;
}

bk_err_t tfcard_util_create(tfcard_util_t      *tfcard_util, char *name)
{
    FRESULT fr;

    if (!tfcard_util || !name)
    {
        LOGE("%s, %d, tfcard_util: %p, name: %p \n", __func__, __LINE__, tfcard_util, name);
        return BK_FAIL;
    }

    /* config file name */
    config_file_name(tfcard_util->file_name, name);

    /*open file to save sin data of AEC */
    fr = f_open(&tfcard_util->fd, tfcard_util->file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK)
    {
        LOGE("%s, %d, open %s fail \n", __func__, __LINE__, tfcard_util->file_name);
        return BK_FAIL;
    }

    LOGD("open %s ok \n", tfcard_util->file_name);

    return BK_OK;
}

bk_err_t tfcard_util_destroy(tfcard_util_t *tfcard_util)
{
    FRESULT fr;

    if (!tfcard_util)
    {
        LOGE("%s, %d, tfcard_util is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* close sin file */
    fr = f_close(&tfcard_util->fd);
    if (fr != FR_OK)
    {
        LOGE("%s, %d, close %s fail \n", __func__, __LINE__, tfcard_util->file_name);
    }

    os_free(tfcard_util);

    LOGE("close %s ok \n", __func__, __LINE__, tfcard_util->file_name);

    return BK_OK;
}

bk_err_t tfcard_util_tx_data(tfcard_util_t *tfcard_util, void *data_buf, uint32_t len)
{
    FRESULT fr;
    uint32 uiTemp = 0;

    if (!tfcard_util || !data_buf || len == 0)
    {
        LOGE("%s, %d, tfcard_util: %p, data_buf: %p, len: %d \n", __func__, __LINE__, tfcard_util, data_buf, len);
        return BK_FAIL;
    }

    fr = f_write(&tfcard_util->fd, data_buf, len, &uiTemp);
    if (fr != FR_OK)
    {
        LOGE("%s, %d, write fail, fr: %d \n", __func__, __LINE__, fr);
        return BK_FAIL;
    }

    return BK_OK;
}

