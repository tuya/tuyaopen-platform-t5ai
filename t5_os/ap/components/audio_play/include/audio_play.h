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


#ifndef __AUDIO_PLAY_H__
#define __AUDIO_PLAY_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    AUDIO_PLAY_UNKNOWN = 0,

    //AUDIO_PLAY_DEVICE,
    //AUDIO_PLAY_FILE,
    AUDIO_PLAY_ONBOARD_SPEAKER,
    //AUDIO_SINK_NET,
} audio_play_type_t;

typedef enum {
	AUDIO_PLAY_MODE_DIFFEN = 0,
	AUDIO_PLAY_MODE_SIGNAL_END,
	AUDIO_PLAY_MODE_MAX,
} audio_play_mode_t;

typedef enum
{
    AUDIO_PLAY_STA_IDLE = 0,
    AUDIO_PLAY_STA_RUNNING,
    AUDIO_PLAY_STA_PAUSED,
} audio_play_sta_t;

typedef enum
{
    AUDIO_PLAY_PAUSE,
    AUDIO_PLAY_RESUME,
    AUDIO_PLAY_MUTE,
    AUDIO_PLAY_UNMUTE,
    AUDIO_PLAY_SET_VOLUME,
} audio_play_ctl_t;

typedef struct
{
    uint8_t port;                   /*!< select port when connect multiple speaker, default 0 when connect one device */
    uint8_t nChans;
    uint32_t sampRate;
    uint8_t bitsPerSample;
    int volume;
    audio_play_mode_t play_mode;
    uint32_t frame_size;            /*!< frame size unit byte */
    uint32_t pool_size;             /*!< the size (unit byte) of ringbuffer pool saved speaker data need to play */
} audio_play_cfg_t;

#define DEFAULT_AUDIO_PLAY_CONFIG() {       \
    .port = 0,                              \
    .nChans = 1,                            \
    .sampRate = 8000,                       \
    .bitsPerSample = 16,                    \
    .volume = 0x2d,                         \
    .play_mode = AUDIO_PLAY_MODE_DIFFEN,    \
    .frame_size = 320,                      \
    .pool_size = 640,                       \
}

typedef struct audio_play audio_play_t;

typedef struct
{
    int (*open)(audio_play_t *play, audio_play_cfg_t *config);
    int (*write)(audio_play_t *play, char *buffer, uint32_t len);
    int (*control)(audio_play_t *play, audio_play_ctl_t ctl);
    int (*close)(audio_play_t *play);
} audio_play_ops_t;

struct audio_play
{
    audio_play_ops_t *ops;

    audio_play_cfg_t config;

    void *play_ctx;
};



/**
 * @brief     Create audio play with config
 *
 * This API create audio play handle according to play type and config.
 * This API should be called before other api.
 *
 * @param[in] play_type The type of play
 * @param[in] config    Play config used in audio_play_open api
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
audio_play_t *audio_play_create(  audio_play_type_t play_type, audio_play_cfg_t *config);

/**
 * @brief      Destroy audio play
 *
 * This API Destroy audio play according to audio play handle.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_destroy(audio_play_t *play);

/**
 * @brief      Open audio play
 *
 * This API open audio play and start play.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_open(audio_play_t *play);

/**
 * @brief      Close audio play
 *
 * This API stop play and close audio play.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_close(audio_play_t *play);

/**
 * @brief      Write speaker data to audio play
 *
 * This API write speaker data to pool.
 * If memory in pool is not enough, wait until the pool has enough memory.
 *
 *
 * @param[in] play      The audio play handle
 * @param[in] buffer    The speaker data buffer
 * @param[in] len       The length (byte) of speaker data
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_write_data(audio_play_t *play, char *buffer, uint32_t len);

/**
 * @brief      Control audio play
 *
 * This API can control audio play, such as pause, resume and so on.
 *
 *
 * @param[in] play  The audio play handle
 * @param[in] ctl   The control opcode
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_control(audio_play_t *play, audio_play_ctl_t ctl);

/**
 * @brief      Open audio play
 *
 * This API open audio play and start play.
 *
 *
 * @param[in] play      The audio play handle
 * @param[in] volume    The volume value
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_set_volume(audio_play_t *play, int volume);

#ifdef __cplusplus
}
#endif
#endif /* __AUDIO_PLAY_H__ */

