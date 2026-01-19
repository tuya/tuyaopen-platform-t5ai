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


#ifndef __AUDIO_RECORD_H__
#define __AUDIO_RECORD_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    AUDIO_RECORD_UNKNOWN = 0,

    //AUDIO_RECORD_DEVICE,
    //AUDIO_RECORD_FILE,
    AUDIO_RECORD_ONBOARD_MIC,
    //AUDIO_RECORD_NET,
} audio_record_type_t;

typedef enum {
	AUDIO_MIC_MODE_DIFFEN = 0,
	AUDIO_MIC_MODE_SIGNAL_END,
	AUDIO_MIC_MODE_MAX,
} audio_mic_mode_t;

typedef enum
{
    AUDIO_RECORD_STA_IDLE = 0,
    AUDIO_RECORD_STA_RUNNING,
    AUDIO_RECORD_STA_PAUSED,
} audio_record_sta_t;

typedef enum
{
    AUDIO_RECORD_PAUSE,
    AUDIO_RECORD_RESUME,
    AUDIO_RECORD_SET_ADC_GAIN,
} audio_record_ctl_t;

typedef struct
{
    uint8_t port;
    uint8_t nChans;
    uint32_t sampRate;
    uint8_t bitsPerSample;
    int adc_gain;
    audio_mic_mode_t mic_mode;
    uint32_t frame_size;
    uint32_t pool_size;
} audio_record_cfg_t;

#define DEFAULT_AUDIO_RECORD_CONFIG() {     \
    .port = 0,                              \
    .nChans = 1,                            \
    .sampRate = 8000,                       \
    .bitsPerSample = 16,                    \
    .adc_gain = 0x2d,                       \
    .mic_mode = AUDIO_MIC_MODE_DIFFEN,      \
    .frame_size = 320,                      \
    .pool_size = 640,                       \
}


typedef struct audio_record audio_record_t;

typedef struct
{
    int (*open)(audio_record_t *record, audio_record_cfg_t *config);
    int (*read)(audio_record_t *record, char *buffer, uint32_t len);
    int (*control)(audio_record_t *record, audio_record_ctl_t ctl);
    int (*close)(audio_record_t *record);
} audio_record_ops_t;

struct audio_record
{
    audio_record_ops_t *ops;

    audio_record_cfg_t config;

    void *record_ctx;
};



/**
 * @brief     Create audio record with config
 *
 * This API create audio record handle according to record type and config.
 * This API should be called before other api.
 *
 * @param[in] record_type   The type of record
 * @param[in] config        Record config used in audio_record_open api
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
audio_record_t *audio_record_create(  audio_record_type_t record_type, audio_record_cfg_t *config);

/**
 * @brief      Destroy audio record
 *
 * This API Destroy audio record according to audio record handle.
 *
 *s
 * @param[in] record  The audio record handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_record_destroy(audio_record_t *record);

/**
 * @brief      Open audio record
 *
 * This API open audio record and start record.
 *
 *
 * @param[in] record  The audio record handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_record_open(audio_record_t *record);

/**
 * @brief      Close audio record
 *
 * This API stop record and close audio record.
 *
 *
 * @param[in] record  The audio record handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_record_close(audio_record_t *record);

/**
 * @brief      Read mic data to audio record
 *
 * This API read mic data from pool.
 * If memory in pool is empty, wait until the pool has enough mic data.
 *
 *
 * @param[in] record    The audio record handle
 * @param[in] buffer    The data buffer used to save mic data
 * @param[in] len       The length (byte) of mic data
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_record_read_data(audio_record_t *record, char *buffer, uint32_t len);

/**
 * @brief      Control audio record
 *
 * This API can control audio record, such as pause, resume and so on.
 *
 *
 * @param[in] record    The audio record handle
 * @param[in] ctl       The control opcode
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_record_control(audio_record_t *record, audio_record_ctl_t ctl);

/**
 * @brief      Open audio record
 *
 * This API open audio record and start record.
 *
 *
 * @param[in] record    The audio record handle
 * @param[in] value     The value of adc gain
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_set_adc_gain(audio_record_t *record, int value);

#ifdef __cplusplus
}
#endif
#endif /* __AUDIO_RECORD_H__ */

