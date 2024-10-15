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

#include "efuse.h"
#include "hal_efuse.h"
#include "pal_log.h"

uint32_t s_efuse_data = 0;
bool g_efuse_inited = false;

int hal_efuse_init(void)
{
#if (CONFIG_EFUSE)
	uint8_t *efuse_byte_p = (uint8_t*)&s_efuse_data;
	int offset;

	bk_efuse_driver_init();
	for (offset = 0; offset < 4; offset ++) {
		bk_efuse_read_byte(offset, efuse_byte_p);
		efuse_byte_p++;
	}
#else
	s_efuse_data = HAL_EFUSE_SECURE_BOOT_SUPPORTED_BIT;
#endif
	g_efuse_inited = true;
	return BK_OK;
}

void hal_efuse_dump(void)
{
	PAL_LOG_INFO("efuse data=%x\n", s_efuse_data);
}

