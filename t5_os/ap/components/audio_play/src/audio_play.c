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

#include "audio_play.h"
#include "onboard_speaker_play.h"


#define AUDIO_PLAY_TAG "aud_play"

#define LOGI(...) BK_LOGI(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUDIO_PLAY_TAG, ##__VA_ARGS__)


audio_play_t *audio_play_create(  audio_play_type_t play_type, audio_play_cfg_t *config)
{
    audio_play_ops_t *temp_ops = NULL;

    if (!config)
    {
        return NULL;
    }

    /* check whether play type support */
    switch (play_type)
    {
        case AUDIO_PLAY_ONBOARD_SPEAKER:
            temp_ops = get_onboard_speaker_play_ops();
            break;

        default:
            break;
    }

    /* check play_info */
    //TODO

    if (!temp_ops)
    {
        LOGE("%s, %d, play_type: %d not support\n", __func__, __LINE__, play_type);
    }

    audio_play_t *play = os_malloc(sizeof(audio_play_t));
    if (!play)
    {
        LOGE("%s, %d, os_malloc play_handle: %d fail\n", __func__, __LINE__, sizeof(audio_play_t));
        return NULL;
    }
    os_memset(play, 0, sizeof(audio_play_t));

    play->ops = temp_ops;
    os_memcpy(&play->config, config, sizeof(audio_play_cfg_t));

    return play;
}

bk_err_t audio_play_destroy(audio_play_t *play)
{
    bk_err_t ret = BK_OK;

    if (!play)
    {
        return BK_OK;
    }

    ret = audio_play_close(play);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_play_close fail, ret: %d\n", __func__, __LINE__, ret);
    }

    os_free(play);

    return BK_OK;
}

bk_err_t audio_play_open(audio_play_t *play)
{
    bk_err_t ret = BK_OK;

    if (!play)
    {
        LOGE("%s, %d, param error, play: %p\n", __func__, __LINE__, play);
        return BK_FAIL;
    }

    ret = play->ops->open(play, &play->config);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_play_open fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_play_close(audio_play_t *play)
{
    bk_err_t ret = BK_OK;

    if (!play)
    {
        LOGE("%s, %d, param error, play: %p\n", __func__, __LINE__, play);
        return BK_FAIL;
    }

    ret = play->ops->close(play);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_play_close fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_play_write_data(audio_play_t *play, char *buffer, uint32_t len)
{
    if (!play || !buffer || !len)
    {
        LOGE("%s, %d, params error, play: %p, buffer: %p, len: %d\n", __func__, __LINE__, play, buffer, len);
        return BK_FAIL;
    }

    return play->ops->write(play, buffer, len);
}


bk_err_t audio_play_control(audio_play_t *play, audio_play_ctl_t ctl)
{
    bk_err_t ret = BK_OK;

    if (!play)
    {
        LOGE("%s, %d, param error, play: %p\n", __func__, __LINE__, play);
        return BK_FAIL;
    }

    ret = play->ops->control(play, ctl);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_play_control fail, ret: %d\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_play_set_volume(audio_play_t *play, int volume)
{
    bk_err_t ret = BK_OK;

    if (!play)
    {
        LOGE("%s, %d, param error, play: %p\n", __func__, __LINE__, play);
        return BK_FAIL;
    }

    int old_volume = play->config.volume;
    play->config.volume = volume;

    ret = play->ops->control(play, AUDIO_PLAY_SET_VOLUME);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, audio_play_control fail, ret: %d\n", __func__, __LINE__, ret);
        play->config.volume = old_volume;
        return BK_FAIL;
    }

    return BK_OK;
}

