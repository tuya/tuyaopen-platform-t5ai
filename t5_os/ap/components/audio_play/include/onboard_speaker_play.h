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


#ifndef __ONBOARD_SPEAKER_PLAY_H__
#define __ONBOARD_SPEAKER_PLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "audio_play.h"

audio_play_ops_t *get_onboard_speaker_play_ops(void);

#ifdef __cplusplus
}
#endif
#endif /* __ONBOARD_SPEAKER_PLAY_H__ */

