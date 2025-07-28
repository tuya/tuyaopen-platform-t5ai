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

bk_err_t lcd_display_service_init(void);

bk_err_t lcd_display_open(lcd_open_t *config);

bk_err_t lcd_display_close(void);

bool check_lcd_task_is_open(void);

uint8_t lcd_display_get_type(void);

bk_err_t lcd_display_frame_request(frame_buffer_t *frame);


