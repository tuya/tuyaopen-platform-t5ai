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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	AUDIO_PLAYER_START,
	AUDIO_PLAYER_STOP,
	AUDIO_PLAYER_RESUME,
	AUDIO_PLAYER_NEXT,
	AUDIO_PLAYER_PRE,
	AUDIO_PLAYER_EXIT,
	AUDIO_PLAYER_WIFI_CONNECTED,
	AUDIO_PLAYER_WIFI_DISCONNECT,
	AUDIO_PLAYER_MAX,
} audio_player_opcode_t;


typedef struct {
	uint32_t song_time; // seconds length
	uint32_t song_current_time;
	uint32_t song_uri_id;
} audio_player_song_time_t;

typedef struct {
	audio_player_opcode_t op;
	
	
} audio_player_msg_t;


extern audio_player_song_time_t song_time;;

bk_err_t audio_player_start(void);

bk_err_t audio_player_stop(void);

bk_err_t audio_player_pause(void);

bk_err_t audio_player_play(void);

bk_err_t audio_player_set_volume(uint8_t vol_value);

bk_err_t audio_player_pre(void);

bk_err_t audio_player_next(void);

bk_err_t audio_player_appoint(uint32_t idx);

bk_err_t audio_player_init(void);

bk_err_t audio_player_deinit(void);

bk_err_t audio_player_resume(void);

uint32_t audio_player_get_song_length(void);

uint32_t audio_player_get_song_rate(void);

void audio_player_song_time_reset(void);

uint32_t audio_player_get_song_url_id(void);

bk_err_t audio_player_send_msg(audio_player_msg_t msg);

uint8_t audio_player_wifi_connect_status_get(void);

void audio_player_wifi_connect_status_set(uint8_t status);


#ifdef __cplusplus
}
#endif


