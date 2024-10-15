// Copyright 2020-2023 Beken
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

#include <stdint.h>
#include "pal_log.h"
#include "hal_hw_fih.h"
#include "fih.h"

bool s_is_crypto_inited = false;

#if CONFIG_HW_FIH

static inline void hw_fih_delay(void)
{
	if (s_is_crypto_inited == true) {
		fih_delay();
	} else {
		PAL_LOG_DEBUG("hw_fih_delay: not delay\r\n");
	}
}

void bk_fih_set_src(uint32_t id, uint32_t data)
{
	hw_fih_delay();

	switch (id) {
#if CONFIG_FIH_MULTI_FIELDS
	case FIH_DATA_PUBLIC_KEY_HASH:
		PRRO_REG19_CMP0_DATA_SRC &= ~FIH_DATA_PUBLIC_KEY_HASH_MASK;
		PRRO_REG19_CMP0_DATA_SRC |= (data & 0xFFFF);
		break;
	case FIH_DATA_BOOT_TYPE:
		PRRO_REG19_CMP0_DATA_SRC &= ~FIH_DATA_BOOT_TYPE_MASK;
		PRRO_REG19_CMP0_DATA_SRC |= ((data & 0xF) << 16);
		break;
	case FIH_DATA_BOOT_FLAG:
		PRRO_REG19_CMP0_DATA_SRC &= ~FIH_DATA_BOOT_FLAG_MASK;
		PRRO_REG19_CMP0_DATA_SRC |= ((data & 0xF) << 20);
		break;
	case FIH_DATA_LCS:
		PRRO_REG19_CMP0_DATA_SRC &= ~FIH_DATA_LCS_MASK;
		PRRO_REG19_CMP0_DATA_SRC |= ((data & 0xF) << 24);
		break;
	case FIH_DATA_EFUSE:
		PRRO_REG19_CMP0_DATA_SRC &= ~FIH_DATA_EFUSE_MASK;
		PRRO_REG19_CMP0_DATA_SRC |= ((data & 0xF) << 28);
		break;
#else
	case FIH_DATA_PUBLIC_KEY_HASH:
		PRRO_REG19_CMP0_DATA_SRC = data;
		break;
#endif
	default:
		break;
	}

	PAL_LOG_INFO("fih: set src, id=%d data=%x src=%x\n", id, data, PRRO_REG19_CMP0_DATA_SRC);
}

void bk_fih_set_dst(uint32_t id, uint32_t data)
{
	hw_fih_delay();

	switch (id) {
#if CONFIG_FIH_MULTI_FIELDS
	case FIH_DATA_PUBLIC_KEY_HASH:
		PRRO_REG1A_CMP0_DATA_DST &= ~FIH_DATA_PUBLIC_KEY_HASH_MASK;
		PRRO_REG1A_CMP0_DATA_DST |= (data & 0xFFFF);
		break;
	case FIH_DATA_BOOT_TYPE:
		PRRO_REG1A_CMP0_DATA_DST &= ~FIH_DATA_BOOT_TYPE_MASK;
		PRRO_REG1A_CMP0_DATA_DST |= ((data & 0xF) << 16);
		break;
	case FIH_DATA_BOOT_FLAG:
		PRRO_REG1A_CMP0_DATA_DST &= ~FIH_DATA_BOOT_FLAG_MASK;
		PRRO_REG1A_CMP0_DATA_DST |= ((data & 0xF) << 20);
		break;
	case FIH_DATA_LCS:
		PRRO_REG1A_CMP0_DATA_DST &= ~FIH_DATA_LCS_MASK;
		PRRO_REG1A_CMP0_DATA_DST |= ((data & 0xF) << 24);
		break;
	case FIH_DATA_EFUSE:
		PRRO_REG1A_CMP0_DATA_DST &= ~FIH_DATA_EFUSE_MASK;
		PRRO_REG1A_CMP0_DATA_DST |= ((data & 0xF) << 28);
		break;
#else
	case FIH_DATA_PUBLIC_KEY_HASH:
		PRRO_REG1A_CMP0_DATA_DST = data;
		break;
#endif
	default:
		break;
	}

	PAL_LOG_INFO("fih: set dst, id=%d data=%x dst=%x\n", id, data, PRRO_REG1A_CMP0_DATA_DST);
}

void bk_fih_set_addr_range(uint32_t start, uint32_t end)
{
	PRRO_REG17_CMP0_ADDR_START = start;
	hw_fih_delay();
	PRRO_REG18_CMP0_ADDR_END = end;
	PAL_LOG_DEBUG("fih: addr range, start=%x end=%x\r\n", start, end);
}

void bk_fih_init(void)
{
	PRRO_SOFT_RESET;
	bk_fih_set_addr_range(0x02E00000, 0x02F00000);
	PRRO_REG1B_CMP1_ADDR_START = 0x02E00000;
	PRRO_REG1C_CMP1_ADDR_END = 0x02F00000;
	PRRO_REG1F_CMP2_ADDR_START = 0x02E00000;
	PRRO_REG20_CMP2_ADDR_END = 0x02F00000;
	PAL_LOG_DEBUG("fih: init\r\n");
}

void dump_prro_regs(void)
{
	PAL_LOG_INFO("prro cmp0_addr_start=%x\r\n", PRRO_REG17_CMP0_ADDR_START);
	PAL_LOG_INFO("prro cmp0_addr_end=%x\r\n", PRRO_REG18_CMP0_ADDR_END);
	PAL_LOG_INFO("prro cmp0_data_src=%x\r\n", PRRO_REG19_CMP0_DATA_SRC);
	PAL_LOG_INFO("prro cmp0_data_dst=%x\r\n", PRRO_REG1A_CMP0_DATA_DST);
	PAL_LOG_INFO("prro cmp1_addr_start=%x\r\n", PRRO_REG1B_CMP1_ADDR_START);
	PAL_LOG_INFO("prro cmp1_addr_end=%x\r\n", PRRO_REG1C_CMP1_ADDR_END);
	PAL_LOG_INFO("prro cmp1_data_src=%x\r\n", PRRO_REG1D_CMP1_DATA_SRC);
	PAL_LOG_INFO("prro cmp1_data_dst=%x\r\n", PRRO_REG1E_CMP1_DATA_DST);
	PAL_LOG_INFO("prro cmp2_addr_start=%x\r\n", PRRO_REG1F_CMP2_ADDR_START);
	PAL_LOG_INFO("prro cmp2_addr_end=%x\r\n", PRRO_REG20_CMP2_ADDR_END);
	PAL_LOG_INFO("prro cmp2_data_src=%x\r\n", PRRO_REG21_CMP2_DATA_SRC);
	PAL_LOG_INFO("prro cmp2_data_dst=%x\r\n", PRRO_REG22_CMP2_DATA_DST);
	PAL_LOG_INFO("prro mismatch int=%x\r\n", PRRO_REG23_CMP_INT_STATUS);
}
#endif
