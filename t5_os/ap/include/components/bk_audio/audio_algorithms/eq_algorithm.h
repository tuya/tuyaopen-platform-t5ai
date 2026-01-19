// Copyright 2023-2024 Beken
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

#ifndef _EQ_ALGORITHM_H_
#define _EQ_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/eq.h>

#include <components/audio_param_ctrl.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _app_eq_t
{
   uint8_t eq_en;
   uint32_t framecnt;
   uint32_t filters;
   int32_t globle_gain;
   eq_para_t eq_para[CON_AUD_EQ_BANDS];
   app_eq_load_t eq_load;
}app_eq_t;


/**
 * @brief      EQ algorithm configurations
 */
typedef struct
{
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    app_eq_t                eq_cal_para;        /*!< eq calculation parameters from/to audio tools */
    int                     eq_chl_num;         /*!< eq channel number */
    int                     eq_frame_size;      /*!< eq frame size */
    int                     out_block_num;      /*!< Number of output block, the size of block is frame size of 20ms audio data */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
} eq_algorithm_cfg_t;

#define EQ_ALGORITHM_TASK_STACK          (1 * 1024)
#define EQ_ALGORITHM_TASK_CORE           (1)
#define EQ_ALGORITHM_TASK_PRIO           (5)

#define EQ0 1
#define EQ0A0   -1668050
#define EQ0A1    734106
#define EQ0B0    934084
#define EQ0B1   -1715174
#define EQ0B2    801474
#define EQ0FREQ  0x44160000
#define EQ0GAIN  0xc1700000
#define EQ0QVAL  0x3f800000
#define EQ0FTYPE 0x01

#define EQ1 1    
#define EQ1A0    -1764715
#define EQ1A1     784980
#define EQ1B0     1034243
#define EQ1B1    -1764715
#define EQ1B2     799312
#define EQ1FREQ   0x442f0000
#define EQ1GAIN   0xbf800000
#define EQ1QVAL   0x3f800000
#define EQ1FTYPE  0x0
    
#define EQSAMP         0x3e80
#define EQGAIN         0x4000
#define EQFGAIN        0x00000000
#define EQGLOBALGAIN  (uint32_t)(1.12f * (1 << FILTER_PREGAIN_FRA_BITS))
#define EQFRAMESIZE   EQSAMP*20/1000*2

#define DEFAULT_EQ_ALGORITHM_CONFIG() {                                             \
    .task_stack            = EQ_ALGORITHM_TASK_STACK,                               \
    .task_core             = EQ_ALGORITHM_TASK_CORE,                                \
    .task_prio             = EQ_ALGORITHM_TASK_PRIO,                                \
    .eq_cal_para = {                                                                \
        .eq_en = 1,                                                                 \
        .filters = 2,                                                               \
        .globle_gain = EQGLOBALGAIN,                                                \
        .eq_para[0].a[0] = -EQ0A0,                                                  \
        .eq_para[0].a[1] = -EQ0A1,                                                  \
        .eq_para[0].b[0] = EQ0B0,                                                   \
        .eq_para[0].b[1] = EQ0B1,                                                   \
        .eq_para[0].b[2] = EQ0B2,                                                   \
        .eq_para[1].a[0] = -EQ1A0,                                                  \
        .eq_para[1].a[1] = -EQ1A1,                                                  \
        .eq_para[1].b[0] = EQ1B0,                                                   \
        .eq_para[1].b[1] = EQ1B1,                                                   \
        .eq_para[1].b[2] = EQ1B2,                                                   \
        .eq_load.f_gain     = EQFGAIN,                                              \
        .eq_load.samplerate = EQSAMP,                                               \
        .eq_load.eq_load_para[0].freq   = EQ0FREQ,                                  \
        .eq_load.eq_load_para[0].gain   = EQ0GAIN,                                  \
        .eq_load.eq_load_para[0].q_val  = EQ0QVAL,                                  \
        .eq_load.eq_load_para[0].type   = EQ0FTYPE,                                 \
        .eq_load.eq_load_para[0].enable  = EQ0,                                     \
        .eq_load.eq_load_para[1].freq   = EQ1FREQ,                                  \
        .eq_load.eq_load_para[1].gain   = EQ1GAIN,                                  \
        .eq_load.eq_load_para[1].q_val  = EQ1QVAL,                                  \
        .eq_load.eq_load_para[1].type   = EQ1FTYPE,                                 \
        .eq_load.eq_load_para[1].enable  = EQ1,                                     \
    },                                                                              \
    .eq_chl_num            = 1,                                                     \
    .eq_frame_size         = EQFRAMESIZE,                                           \
    .out_block_num         = 2,                                                     \
    .multi_out_port_num    = 0,                                                     \
}

/**
 * @brief      Create a eq algorithm of Audio Element to do EQ
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t eq_algorithm_init(eq_algorithm_cfg_t *config);

/**
 * @brief      Set the EQ algorithm configuration
 *
 * @param[in]      eq_algorithm  The EQ algorithm audio element handle
 * @param[in]      eq_config     The EQ configuration to be set
 *
 * @return     Error code
 *                 - 0: Success
 *                 - Non-zero: Failed
 */
bk_err_t eq_algorithm_set_config(audio_element_handle_t eq_algorithm, void *eq_config);

/**
 * @brief      Get the EQ algorithm configuration
 *
 * @param[in]      eq_algorithm  The EQ algorithm audio element handle
 * @param[out]     eq_load       The buffer to store the EQ configuration
 *
 * @return     Error code
 *                 - 0: Success
 *                 - Non-zero: Failed
 */
bk_err_t eq_algorithm_get_config(audio_element_handle_t eq_algorithm, void *eq_load);

#ifdef __cplusplus
}
#endif

#endif

