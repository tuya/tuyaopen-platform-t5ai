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

#include <driver/lcd_types.h>
//#include <driver/media_types.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    IMG_DISPLAY_REQUEST,
    IMG_DISPLAY_EXIT,
} img_event_t;


typedef struct
{
	const lcd_device_t *lcd_device;      /**< lcd open  lcd device */
	uint16_t lcd_width;
	uint16_t lcd_height;
	uint8_t enable : 1;
	uint8_t decoder_en : 1;
	uint8_t rotate_en : 1;
	uint8_t scale_en : 1;

    uint8_t jpg_fmt_check: 1;  //first check jpeg fmt is YUV422 and YUV420

	beken_queue_t queue;
	beken_thread_t thread;
    beken_semaphore_t sem;

	media_decode_mode_t decode_mode;
	media_rotate_mode_t rotate_mode;
	media_rotate_t rotate;
	media_ppi_t scale_ppi;
    img_proc_order_t proc_order;

	frame_buffer_t *decoder_frame;
	frame_buffer_t *rotate_frame;
	frame_buffer_t *scale_frame;

	beken_mutex_t dec_lock;
	beken_mutex_t rot_lock;
	beken_mutex_t scale_lock;

	void (*fb_free) (frame_buffer_t *frame);
	frame_buffer_t *(*fb_malloc) (uint32_t size);
} img_info_t;

typedef struct
{
    uint32_t event;
    union
    {
        uint32_t param;
        void *ptr;
    };
    void *args;
} img_msg_t;


typedef void (*img_display_cb)(frame_buffer_t *frame);

bk_err_t bk_img_msg_send(img_msg_t *msg);
bk_err_t img_service_open(void);
//void img_event_handle(media_mailbox_msg_t *msg);
void img_register_display_cb(img_display_cb *disp_cb);

frame_buffer_t *rotate_frame_handler(frame_buffer_t *frame, media_rotate_t rotate);
frame_buffer_t *decoder_frame_handler(frame_buffer_t *frame);
frame_buffer_t *scale_frame_handler(frame_buffer_t *frame, media_ppi_t ppi);
bk_err_t image_rotate_set(media_rotate_t rotate);
bk_err_t image_scale_set(media_ppi_t ppi);
bk_err_t image_service_set_order(img_proc_order_t proc_order);

bk_err_t img_service_close(void);

#ifdef __cplusplus
}
#endif


