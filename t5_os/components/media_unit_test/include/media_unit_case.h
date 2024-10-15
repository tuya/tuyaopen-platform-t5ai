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

#ifdef __cplusplus
extern "C" {
#endif

extern beken_semaphore_t wait_sem;
extern uint32_t audio_data_count;

void ut_mem_show(void);
bool ut_check_jpeg(void);
bool ut_check_h264(void);
bool ut_check_lcd(void);
bool ut_check_dec(void);
bool ut_check_read_frame(void);
bool ut_check_part(uint8_t jpeg_check, uint8_t dec_check, uint8_t lcd_check, uint8_t h264_check, uint8_t fps_wifi_check, uint8_t audio_check);

void ut_uvc_connect_callback(bk_uvc_device_brief_info_t *info, uvc_state_t state);
void ut_read_frame_callback(frame_buffer_t *frame);
int ut_audio_send_data_callback(unsigned char *data, unsigned int len);
void ut_uac_connect_callback(uint8_t state);

int lcd_on_off_test(int argc, char **argv, int delay, int times);
int h264_on_off_test(int argc, char **argv, int delay, int times);
int pipeline_on_off_test(int argc, char **argv, int delay, int times);
int read_frame_on_off_test(int argc, char **argv, int delay, int times);
int audio_on_off_test(int argc, char **argv, int delay, int times);

void set_media_ut_exit(uint8_t flag);
uint8_t get_media_ut_exit(void);


#ifdef __cplusplus
}
#endif
