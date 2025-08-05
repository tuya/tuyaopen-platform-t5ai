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

#include <common/bk_include.h>
#include <driver/uvc_camera_types.h>
#include <components/video_types.h>
#include <components/usb_types.h>
#include <driver/h264_types.h>
#include "frame_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*media_transfer_send_cb)(uint8_t *data, uint32_t length);
typedef int (*media_transfer_prepare_cb)(uint8_t *data, uint32_t length);
typedef void* (*media_transfer_get_tx_buf_cb)(void);
typedef int (*media_transfer_get_tx_size_cb)(void);
typedef bool (*media_transfer_drop_check_cb)(frame_buffer_t *frame,uint32_t count, uint16_t ext_size);

typedef struct {
	media_transfer_send_cb send;
	media_transfer_prepare_cb prepare;
	media_transfer_drop_check_cb drop_check;
	media_transfer_get_tx_buf_cb get_tx_buf;
	media_transfer_get_tx_size_cb get_tx_size;
} media_transfer_cb_t;

uint32_t media_app_get_lcd_devices_num(void);

uint32_t media_app_get_lcd_devices_list(void);
uint32_t media_app_get_lcd_device_by_id(uint32_t id);
bk_err_t media_app_lcd_fmt(pixel_format_t fmt);
bk_err_t media_app_set_rotate(media_rotate_t rotate);
bk_err_t media_app_set_scale(media_ppi_t ppi);
bk_err_t media_app_set_proc_order(img_proc_order_t order);
bk_err_t media_app_lcd_disp_open(void *config);
bk_err_t media_app_lcd_disp_close(void);
bk_err_t media_app_jdec_open(uint32_t dec_type);
bk_err_t media_app_jdec_close(void);
bk_err_t media_app_camera_open(camera_handle_t *handle, media_camera_device_t *device);
bk_err_t media_app_camera_close(camera_handle_t *handle);
bk_err_t media_app_get_main_camera_stream(frame_list_node_t *node);
bk_err_t media_app_pipeline_h264_open(void *config);
bk_err_t media_app_pipeline_h264_close(void);
bk_err_t media_app_h264_regenerate_idr(camera_type_t type);
uint32_t media_app_get_lcd_status(void);

bk_err_t media_app_register_read_frame_callback(image_format_t fmt, frame_cb_t cb);
bk_err_t media_app_unregister_read_frame_callback(void);

bk_err_t media_app_storage_open(frame_cb_t cb);
bk_err_t media_app_storage_close(void);
bk_err_t media_app_capture(image_format_t format, char *name);
bk_err_t media_app_save_start(image_format_t format, char *name);
bk_err_t media_app_save_stop(void);
bk_err_t media_app_init(void);

#ifdef __cplusplus
}
#endif
