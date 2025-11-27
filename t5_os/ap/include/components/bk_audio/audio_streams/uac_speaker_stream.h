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


#ifndef _UAC_SPEAKER_STREAM_H_
#define _UAC_SPEAKER_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/usbh_hub_multiple_classes_api.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   UAC Speaker Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    E_USB_HUB_PORT_INDEX          port_index;          /*!< usb hub port index */
    uint16_t                      format;              /*!< uac speaker data format */
    uint8_t                       chl_num;             /*!< speaker channel number */
    uint8_t                       bits;                /*!< Bit wide (8, 16 bits) */
    uint32_t                      samp_rate;           /*!< speaker sample rate */
    uint32_t                      frame_size;          /*!< size of one frame speaker data */
    uint32_t                      volume;              /*!< speaker dac gain */
    bool                          auto_connect;        /*!< Automatic connect enable */
    int                           multi_out_port_num;  /*!< The number of multiple output audio port */
    uint32_t                      pool_size;           /*!< speaker data pool size, the unit is byte */
    uint32_t                      pool_play_thold;     /*!< the play threshold of pool, the unit is byte */
    uint32_t                      pool_pause_thold;    /*!< the pause threshold of pool, the unit is byte */
    int                           task_stack;          /*!< Task stack size */
    int                           task_core;           /*!< Task running in core (0 or 1) */
    int                           task_prio;           /*!< Task priority (based on freeRTOS priority) */
} uac_speaker_stream_cfg_t;


#define UAC_SPEAKER_STREAM_TASK_STACK          (2048)
#define UAC_SPEAKER_STREAM_TASK_CORE           (0)
#define UAC_SPEAKER_STREAM_TASK_PRIO           (3)

#define UAC_SPEAKER_STREAM_CFG_DEFAULT() DEFAULT_UAC_SPEAKER_STREAM_CONFIG()

#define DEFAULT_UAC_SPEAKER_STREAM_CONFIG() {            \
    .port_index = USB_HUB_PORT_1,                        \
    .format = AUDIO_FORMAT_PCM,                          \
    .chl_num = 1,                                        \
    .bits = 16,                                          \
    .samp_rate = 8000,                                   \
    .frame_size = 320,                                   \
    .volume = 5,                                         \
    .auto_connect = true,                                \
    .multi_out_port_num = 0,                             \
    .pool_size = 0,                                      \
    .pool_play_thold = 0,                                \
    .pool_pause_thold = 0,                               \
    .task_stack = UAC_SPEAKER_STREAM_TASK_STACK,         \
    .task_core = UAC_SPEAKER_STREAM_TASK_CORE,           \
    .task_prio = UAC_SPEAKER_STREAM_TASK_PRIO,           \
}

/**
 * @brief      Create a handle to an Audio Element to stream data to another Element.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t uac_speaker_stream_init(uac_speaker_stream_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
