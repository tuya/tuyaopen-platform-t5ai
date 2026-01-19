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

#ifndef __EQ_H__
#define __EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CON_AUD_EQ_BANDS        16
#define FILTER_COEFS_FRA_BITS   (20)
#define FILTER_PREGAIN_FRA_BITS (14)

#define BlockLen  (128)

typedef struct
{
    int32_t a[2];
    int32_t b[3];
} eq_para_t;

typedef struct
{
    uint16_t chl_num;                       /*!< The number of audio data channel */
    uint16_t eq_valid_num;                  /*!< The number of valid eq coef */
    int16_t eq_gain;                        /*!< The gain of eq */
    eq_para_t eq_para[CON_AUD_EQ_BANDS];    /*!< The parameters of eq */
} eq_cfg_t;

typedef struct eq *eq_handle_t;

#define EQ_GAIN     (1 << FILTER_PREGAIN_FRA_BITS)
//#define INVALID_EQ  {0,0},{0x100000,0,0}

#define DEFAULT_EQ_CONFIG() {                                           \
    .chl_num = 2,                                                       \
    .eq_valid_num = 1,                                                  \
    .eq_gain = EQ_GAIN,                                                 \
    .eq_para = {                                                        \
        {{2008639, -961899}, {1026958, -2008639, 983517}},              \
    },                                                                  \
}


/**
 * @brief     Create EQ with config
 *
 * This API create eq handle according to config.
 * This API use psram memory.
 *
 * @param[in] config    EQ config
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
eq_handle_t eq_create(eq_cfg_t *config);

/**
 * @brief      Destroy EQ
 *
 * This API Destroy eq according to eq handle.
 *
 *
 * @param[in] eq_handle  The eq handle
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t eq_destroy(eq_handle_t eq_handle);

/**
 * @brief     EQ process dual channel, 16bit audio data
 *
 * This API create audio play handle according to play type and config.
 * This API should be called before other api.
 *
 * @param[in] eq_handle  The handle of eq
 * @param[in] buff       The intput buffer save audio data need to process
 * @param[in] sample_num The numbers of sample point in buffer
 * @param[out] buff      The output buffer save audio data has been process by eq
 *
 * @return
 *    - none
 */
void eq_process(eq_handle_t eq_handle,int16_t *buff, uint16_t sample_num);

/**
 * @brief     Set EQ parameter
 *
 * This API set eq parameters.
 *
 * @param[in] eq_handle  The eq handle
 * @param[in] config     EQ config
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t eq_set_param(eq_handle_t eq_handle, eq_cfg_t *config);

/**
 * @brief     Create EQ with config
 *
 * This API set eq gain.
 *
 * @param[in] eq_handle  The eq handle
 * @param[in] gain       EQ gain
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t eq_set_gain(eq_handle_t eq_handle, int16_t gain);

/**
 * @brief     Dump EQ information
 *
 * This API debug eq parameters through dump information
 *
 * @param[in] eq_handle  The eq handle
 *
 * @return
 *    - none
 */
void eq_dump(eq_handle_t eq_handle);

#ifdef __cplusplus
}
#endif
#endif /* __EQ_H__ */

