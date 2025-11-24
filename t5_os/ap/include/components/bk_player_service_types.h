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


#ifndef __BK_PLAYER_SERVICE_TYPES__
#define __BK_PLAYER_SERVICE_TYPES__

#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


typedef enum
{
    PLAYER_STATE_NONE = 0,
    PLAYER_STATE_IDLE,
    PLAYER_STATE_STOPED,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
} bk_player_state_t;

typedef enum
{
    PLAYER_EVENT_START = 0,
    PLAYER_EVENT_STOP,
    PLAYER_EVENT_FAILURE,
    PLAYER_EVENT_PAUSE,
    PLAYER_EVENT_RESUME,
    PLAYER_EVENT_FINISH,

    PLAYER_EVENT_MUSIC_INFO,
} bk_player_event_t;

typedef enum
{
    PLAYER_URI_TYPE_ARRAY = 0,
    PLAYER_URI_TYPE_VFS,
    PLAYER_URI_TYPE_URL,
} player_uri_type_t;

typedef struct
{
    player_uri_type_t uri_type;
    char *uri;
    uint32_t total_len;
} player_uri_info_t;

typedef struct player *bk_player_handle_t;
typedef int (*player_event_handle_cb)(int, void *, void *);

typedef struct
{
    spk_type_t              spk_type;

    union
    {
        onboard_speaker_stream_cfg_t    onboard_spk_cfg;
        uac_speaker_stream_cfg_t        uac_spk_cfg;
        uint8_t                         reserve;
    } spk_cfg;

    player_event_handle_cb      event_handle;   /*!< player event handle callback */
    void *                      args;           /*!< the parameter of event_handle func */
} bk_player_cfg_t;

/* Create player to read data, decode audio data to pcm data. But not playback pcm data. */
#define DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG() {                  \
    .spk_type = SPK_TYPE_INVALID,                               \
    .spk_cfg.reserve = 0,                                       \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

/* Create player to read data, decode audio data and playback. */
#define DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG() {                 \
    .spk_type = SPK_TYPE_ONBOARD,                               \
    .spk_cfg.onboard_spk_cfg = {                                \
        .chl_num = 1,                                           \
        .sample_rate = 8000,                                    \
        .dig_gain = 0x2d,                                       \
        .ana_gain = 0x07,                                       \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
        .bits = 16,                                             \
        .clk_src = AUD_CLK_XTAL,                                \
        .multi_out_port_num = 1,                                \
        .frame_size = 320,                                      \
        .pool_length = 0,                                       \
        .pool_play_thold = 0,                                   \
        .pool_pause_thold = 0,                                  \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
        .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
