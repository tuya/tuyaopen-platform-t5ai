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
#include "count_util.h"


#define COUNT_UTIL_TAG "count_util"

#define LOGI(...) BK_LOGI(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(COUNT_UTIL_TAG, ##__VA_ARGS__)



static void count_util_callback(void *param)
{
    count_util_t *count_util = (count_util_t *)param;

    if (!count_util)
    {
        return;
    }

    uint32_t temp = count_util->data_size;

    count_util->data_size = count_util->data_size / 1024 / (count_util->timer_interval / 1000);

    LOGD("[%s] data_size: %d(Bytes), %uKB/s \n", count_util->tag, temp, count_util->data_size);
    count_util->data_size  = 0;
}

bk_err_t count_util_destroy(count_util_t *count_util)
{
    bk_err_t ret = BK_OK;

    if (!count_util)
    {
        LOGE("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (count_util->timer.handle)
    {
        ret = rtos_stop_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, stop count util timer fail \n", __func__, __LINE__);
        }

        ret = rtos_deinit_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, deinit count util timer fail \n", __func__, __LINE__);
        }
        count_util->timer.handle = NULL;
    }
    count_util->data_size = 0;
    count_util->timer_interval = 0;
    os_memset(count_util->tag, '\0', 20);
    LOGV("%s, %d, destroy count util timer complete \n", __func__, __LINE__);

    return ret;
}

bk_err_t count_util_create(count_util_t *count_util, uint32_t interval, char *tag)
{
    bk_err_t ret = BK_OK;

    if (!count_util || interval <= 0 || !tag)
    {
        LOGE("%s, %d, count_util: %p, interval: %d, tag: %p \n", __func__, __LINE__, count_util, interval, tag);
        return BK_FAIL;
    }

    if (count_util->timer.handle != NULL)
    {
        ret = rtos_deinit_timer(&count_util->timer);
        if (BK_OK != ret)
        {
            LOGE("%s, %d, deinit count util time fail \n", __func__, __LINE__);
            goto exit;
        }
        count_util->timer.handle = NULL;
    }

    count_util->data_size = 0;
    count_util->timer_interval = interval;
    if (os_strlen(tag) > 19)
    {
        LOGW("%s, %d, tag length: %d > 19 \n", __func__, __LINE__, os_strlen(tag));
        os_memcpy(count_util->tag, tag, 19);
        count_util->tag[19] = '\0';
    }
    else
    {
        os_memcpy(count_util->tag, tag, os_strlen(tag));
        count_util->tag[os_strlen(tag)] = '\0';
    }

    ret = rtos_init_timer(&count_util->timer, count_util->timer_interval, count_util_callback, count_util);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, init %s count util timer fail \n", __func__, __LINE__, count_util->tag);
        goto exit;
    }
    ret = rtos_start_timer(&count_util->timer);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, start %s count util timer fail \n", __func__, __LINE__, count_util->tag);
        goto exit;
    }
    LOGV("%s, %d, create %s count util timer complete \n", __func__, __LINE__, count_util->tag);

    return BK_OK;
exit:

    count_util_destroy(count_util);
    return BK_FAIL;
}

void count_util_add_size(count_util_t *count_util, int32_t size)
{
    if (!count_util)
    {
        LOGV("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return;
    }

    if (size > 0)
    {
        count_util->data_size += size;
    }
}

