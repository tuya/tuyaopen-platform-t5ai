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

#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "audio_record.h"
#include "onboard_mic_record.h"


#define AUDIO_RECORD_TAG "aud_rec"

#define LOGI(...) BK_LOGI(AUDIO_RECORD_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUDIO_RECORD_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUDIO_RECORD_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUDIO_RECORD_TAG, ##__VA_ARGS__)


audio_record_t *audio_record_create(  audio_record_type_t record_type, audio_record_cfg_t *config)
{
    audio_record_ops_t *temp_ops = NULL;

    /* check whether record type support */
    switch (record_type)
    {
        case AUDIO_RECORD_ONBOARD_MIC:
            temp_ops = get_onboard_mic_record_ops();
            break;

        default:
            break;
    }

    /* check config */
    //TODO

    if (!temp_ops)
    {
        LOGE("%s, %d, record_type: %d not support\n", __func__, __LINE__, record_type);
    }

    audio_record_t *record = os_malloc(sizeof(audio_record_t));
    if (!record)
    {
        LOGE("%s, %d, os_malloc record_handle: %d fail\n", __func__, __LINE__, sizeof(audio_record_t));
        return NULL;
    }
    os_memset(record, 0, sizeof(audio_record_t));

    record->ops = temp_ops;
    os_memcpy(&record->config, config, sizeof(audio_record_cfg_t));

    return record;
}

bk_err_t audio_record_destroy(audio_record_t *record)
{
    bk_err_t ret = BK_OK;

    if (!record)
    {
        return BK_OK;
    }

    ret = audio_record_close(record);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_record_close fail, ret: %d\n", __func__, __LINE__, ret);
    }

    os_free(record);

    return BK_OK;
}

bk_err_t audio_record_open(audio_record_t *record)
{
    bk_err_t ret = BK_OK;

    if (!record)
    {
        LOGE("%s, %d, param error, record: %p\n", __func__, __LINE__, record);
        return BK_FAIL;
    }

    ret = record->ops->open(record, &record->config);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_record_open fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_record_close(audio_record_t *record)
{
    bk_err_t ret = BK_OK;

    if (!record)
    {
        LOGE("%s, %d, param error, record: %p\n", __func__, __LINE__, record);
        return BK_FAIL;
    }

    ret = record->ops->close(record);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_record_close fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_record_read_data(audio_record_t *record, char *buffer, uint32_t len)
{
    if (!record || !buffer || !len)
    {
        LOGE("%s, %d, params error, record: %p, data_buf: %p, len: %d\n", __func__, __LINE__, record, buffer, len);
        return BK_FAIL;
    }

    return record->ops->read(record, buffer, len);
}

bk_err_t audio_record_control(audio_record_t *record, audio_record_ctl_t ctl)
{
    bk_err_t ret = BK_OK;

    if (!record)
    {
        LOGE("%s, %d, param error, record: %p\n", __func__, __LINE__, record);
        return BK_FAIL;
    }

    ret = record->ops->control(record, ctl);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_record_control fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_play_set_adc_gain(audio_record_t *record, int value)
{
    bk_err_t ret = BK_OK;

    if (!record)
    {
        LOGE("%s, %d, param error, record: %p\n", __func__, __LINE__, record);
        return BK_FAIL;
    }

    int old_adc_gain = record->config.adc_gain;
    record->config.adc_gain = value;

    ret = record->ops->control(record, AUDIO_RECORD_SET_ADC_GAIN);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_record_control fail, ret: %d\n", __func__, __LINE__, ret);
        record->config.adc_gain = old_adc_gain;
        return BK_FAIL;
    }

    return BK_OK;
}


