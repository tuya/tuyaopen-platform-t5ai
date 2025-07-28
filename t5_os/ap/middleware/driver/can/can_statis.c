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

#include <os/mem.h>
//#include "can_types.h"
#include "can_hw.h"
#include "can_statis.h"

#if CONFIG_CAN_STATIS

#define TAG "can_statis"

static can_statis_t s_can_statis = {0};

bk_err_t can_statis_init(void)
{
	os_memset(&s_can_statis, 0, sizeof(s_can_statis));
	return BK_OK;
}

can_statis_t* can_statis_get_statis()
{
	return &s_can_statis;
}

void can_statis_dump()
{
	BK_LOGD(TAG, "dump can statis:\r\n");
	BK_LOGD(TAG, "isr_cnt:     %d\r\n", s_can_statis.isr_cnt);
	BK_LOGD(TAG, "rx_cnt:       %d\r\n", s_can_statis.rx_cnt);
	BK_LOGD(TAG, "tx_cnt:       %d\r\n", s_can_statis.tx_cnt);
	BK_LOGD(TAG, "beif_cnt:      %d\r\n", s_can_statis.beif_cnt);
	BK_LOGD(TAG, "alif_cnt: %d\r\n", s_can_statis.alif_cnt);
	BK_LOGD(TAG, "epif_cnt:       %d\r\n", s_can_statis.epif_cnt);
	BK_LOGD(TAG, "ewarn_cnt:         %d\r\n", s_can_statis.ewarn_cnt);
	BK_LOGD(TAG, "aif_cnt:        %d\r\n", s_can_statis.aif_cnt);
}

#endif
