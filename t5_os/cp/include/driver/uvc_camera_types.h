// Copyright 2020-2021 Beken
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

#pragma once

#include <driver/media_types.h>
#include <driver/dma_types.h>
#include <components/usb_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_UVC_CONNECT         (1)
#define BK_UVC_CONFIG_ERROR    (2)
#define BK_UVC_NO_MEMORY       (3)
#define BK_UVC_NO_RESOURCE     (4)
#define BK_UVC_PPI_ERROR       (5)
#define BK_UVC_DISCONNECT      (6)
#define BK_UVC_NO_RESPON       (7)
#define BK_UVC_DMA_ERROR       (8)
#define BK_UVC_NOT_PERMIT      (9)
#define BK_UVC_POWER_ERROR     (10)

typedef enum
{
    UVC_CLOSED_STATE,
    UVC_CLOSING_STATE,
    UVC_CONNECT_STATE,
    UVC_CONFIGING_STATE,
    UVC_STREAMING_STATE,
    UVC_DISCONNECT_STATE,
    UVC_STATE_UNKNOW,
} uvc_state_t;

typedef enum
{
    UVC_SINGLE_STREAM = 0,
    UVC_DOUBLE_STREAM,
} uvc_stream_type_t;

/*
*  Separate H264 from data[]......
*  &data[0] : Data Source
*  length   : the lenght of data[]
*  head     : the valid data for jpeg at the data[] head
*  tail     : the valif data for jpeg at the data[] tail
*  return   : the valid data length for jpeg //the function consumed data or remain data???
*/
typedef struct
{
    uint32_t data_len;
    uint8_t *data_off;
} uvc_separate_info_t;

typedef struct
{
    uvc_stream_type_t type;
    uint16 img_format; // image_format_t
    uint16_t width;
    uint16_t height;
    uint8_t fps;
    uint8_t port;
    uint8_t drop_num;
} uvc_config_t;

typedef struct
{
    void *(*frame_init) (uint16_t port, uint8_t camera_type, uint16_t image_format);
    int (*frame_deinit) (void *node);
    int (*frame_clear) (void *node);
    int (*frame_complete) (uint16_t image_format, void *node, frame_buffer_t* buffer);
    frame_buffer_t* (*frame_malloc)(uint16_t image_format, void *node, uint32_t size);
    int (*frame_free)(uint16_t image_format, void *node,frame_buffer_t* buffer);
} bk_uvc_callback_t;

typedef void (*camera_state_cb_t)(bk_usb_hub_port_info *param, uint32_t state);

typedef struct
{
    uint8_t id; // camera port_id
    void (*uvc_separate_packet_cb)(uint8_t *data, uint32_t length, uvc_separate_info_t *sepatate_info);
    bk_err_t (*uvc_init_packet_cb)(uvc_config_t *device, uint8_t init, bk_uvc_callback_t *cb);
    void (*uvc_eof_packet_cb)(uvc_config_t *device);
} uvc_separate_config_t;

#ifdef __cplusplus
}
#endif
