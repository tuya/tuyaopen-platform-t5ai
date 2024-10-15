// Copyright 2022-2023 Beken
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

#include "soc.h"
#include "hal_perf.h"
#include "pal_log.h"

#if CONFIG_PERF

typedef struct {
	uint32_t point;
	uint32_t gpio_up_id;
	uint32_t gpio_down_id;
} point_t;

static point_t s_points[] = {
        {P_START,                 15, 14},
        {P_MAIN_START,            17, 15},
        {P_PUBKEY_HASH_START,     18, 17},
        {P_PUBKEY_HASH_END,       19, 18},
        {P_MANIFEST_VERIFY_START, 20, 19},
        {P_MANIFEST_VERIFY_END,   14, 20},
        {P_IMG_LOAD_START,        15, 14},
        {P_IMG_LOAD_END,          17, 15},
        {P_IMG_HASH_START,        18, 17},
        {P_IMG_HASH_END,          19, 18},
        {P_JUMP,                  20, 19}
};

void hal_perf_debug(uint32_t point)
{
	if (point >= sizeof(s_points)/sizeof(s_points[0])) {
		return;
	}

	if (s_points[point].point != point) {
		PAL_LOG_ERR("invalid point table\r\n");
		while(1);
	}

	GPIO_UP(s_points[point].gpio_up_id);
	GPIO_DOWN(s_points[point].gpio_down_id);
}

#endif
