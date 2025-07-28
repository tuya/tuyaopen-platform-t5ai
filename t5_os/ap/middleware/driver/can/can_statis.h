// Copyright 2023-2024 Beken
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

#include <driver/hal/hal_can_types.h>
#include <driver/can_types.h>
#include <components/log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t isr_cnt;
	uint32_t rx_cnt;
	uint32_t tx_cnt;
	uint32_t beif_cnt;
	uint32_t alif_cnt;
	uint32_t epif_cnt;
	uint32_t ewarn_cnt;
	uint32_t aif_cnt;
} can_statis_t;

#if CONFIG_CAN_STATIS

#define CAN_STATIS_DEC()  can_statis_t* can_statis = NULL
#define CAN_STATIS_GET(_statis) (_statis) = can_statis_get_statis()

#define CAN_STATIS_SET(_statis, _v) do{\
		(_statis) = (_v);\
	} while(0)

#define CAN_STATIS_ADD(_statis, _v) do{\
		(_statis) += (_v);\
	} while(0)

bk_err_t can_statis_init(void);
can_statis_t* can_statis_get_statis();
void can_statis_dump();
#else
#define CAN_STATIS_DEC() 
#define CAN_STATIS_GET(_statis) 
#define CAN_STATIS_SET(_statis, _v)
#define CAN_STATIS_ADD(_statis, _v)
#define can_statis_init()
#define can_statis_get_statis() NULL
#define	can_statis_dump()
#endif

#define CAN_STATIS_INC(_statis) CAN_STATIS_ADD((_statis), 1)

#ifdef __cplusplus
}
#endif
