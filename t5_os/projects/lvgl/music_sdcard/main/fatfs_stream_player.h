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

#ifndef FATFS_STREAM_PLAYER_PLAYER_H
#define FATFS_STREAM_PLAYER_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <../components/fatfs/ff.h>


#define MP3_FILE_COUNT_MAX  20
typedef enum {
	FATFS_STREAM_PLAYER_SCAN,
	FATFS_STREAM_PLAYER_START,
	FATFS_STREAM_PLAYER_STOP,
	FATFS_STREAM_PLAYER_RESUME,
	FATFS_STREAM_PLAYER_NEXT,
	FATFS_STREAM_PLAYER_EXIT,
	FATFS_STREAM_PLAYER_MAX,
} fatfs_stream_player_opcode_t;

typedef struct {
	fatfs_stream_player_opcode_t op;
	uint32_t param;
} fatfs_stream_player_msg_t;

extern int file_list_index;
extern int mp3_file_count;
extern FILINFO tf_mp3_file[MP3_FILE_COUNT_MAX];
bk_err_t fatfs_stream_player_stop(void);
bk_err_t fatfs_stream_player_send_msg(fatfs_stream_player_msg_t msg);
bk_err_t fatfs_stream_player_init(void);
bk_err_t fatfs_stream_player_deinit(void);
bk_err_t audio_tf_card_mount(int number);
bk_err_t audio_tf_card_scan(int number);
bk_err_t fatfs_stream_player_scan(void);
	int audio_tf_get_mp3_files_count(void);

#ifdef __cplusplus
}
#endif

#endif /*FATFS_STREAM_PLAYER_PLAYER_H*/

