// Copyright 2023-2028 Beken
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

#include "os/os.h"
#include "common/bk_assert.h"
#include "sdkconfig.h"
#include "armstar.h"

#define MPU_MAX_NUM_REGIONS     (16UL)  
#define MPU_MAX_NUM_ATTRS       (8UL)   

static ARM_MPU_Region_t *s_mpu_regions;
static uint32_t s_mpu_regions_cnt = 0;
static uint8_t *s_mpu_attrs;
static uint32_t s_mpu_attrs_cnt = 0;

extern void mpu_soc_cfg(void);

void mpu_register_regions(ARM_MPU_Region_t *regions, uint32_t region_cnt)
{
	BK_ASSERT(regions);
	BK_ASSERT(region_cnt);

	s_mpu_regions = regions;
	s_mpu_regions_cnt = region_cnt;
}

void mpu_register_attrs(uint8_t *attrs, uint32_t cnt)
{
	BK_ASSERT(attrs);
	BK_ASSERT(cnt);

	s_mpu_attrs = attrs;
	s_mpu_attrs_cnt = cnt;
}

void mpu_clear(uint32_t rnr)
{
    ARM_MPU_Disable();
    ARM_MPU_ClrRegion(rnr);
#if CONFIG_SPE
    ARM_MPU_Enable(MPU_CTRL_HFNMIENA_Msk);
#else
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);
#endif
}

void mpu_cfg(uint32_t rnr, uint32_t rbar, uint32_t rlar)
{
    ARM_MPU_Disable();
    ARM_MPU_SetRegion(rnr, rbar, rlar);
#if CONFIG_SPE
    ARM_MPU_Enable(MPU_CTRL_HFNMIENA_Msk);
#else
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);
#endif
    SCB_CleanInvalidateDCache();
}

void mpu_enable(void)
{
    uint32_t region_num;
    uint32_t attr_num;

	mpu_soc_cfg();
	region_num = s_mpu_regions_cnt;
	attr_num = s_mpu_attrs_cnt;
	
    ARM_MPU_Disable();

    if (region_num > MPU_MAX_NUM_REGIONS){
        region_num = MPU_MAX_NUM_REGIONS;
    }

    for (int i = 0; i < region_num; i++) {
        ARM_MPU_SetRegion(i, s_mpu_regions[i].RBAR, s_mpu_regions[i].RLAR);
    }

    for (int j = 0; j < attr_num; j++) {
        ARM_MPU_SetMemAttr(j, s_mpu_attrs[j]);
    }

#if CONFIG_SPE
    ARM_MPU_Enable(MPU_CTRL_HFNMIENA_Msk);
#else
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);
#endif
}

void mpu_disable(void)
{
    ARM_MPU_Disable();
    for (uint32_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
      ARM_MPU_ClrRegion(i);
    }
}

void mpu_dump(void)
{
    BK_DUMP_OUT("******************** Dump mpu config begin ********************\r\n");
    BK_DUMP_OUT("MPU->TYPE: 0x%08X.\r\n", MPU->TYPE);
    BK_DUMP_OUT("MPU->CTRL: 0x%08X.\r\n", MPU->CTRL);
    for (uint32_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
        MPU->RNR = i;
        BK_DUMP_OUT("MPU->RNR: %d.\r\n", MPU->RNR);
        BK_DUMP_OUT("MPU->RBAR: 0x%08X.\r\n", MPU->RBAR);
        BK_DUMP_OUT("MPU->RLAR: 0x%08X.\r\n", MPU->RLAR);
        BK_DUMP_OUT("MPU->RBAR_A1: 0x%08X.\r\n", MPU->RBAR_A1);
        BK_DUMP_OUT("MPU->RLAR_A1: 0x%08X.\r\n", MPU->RLAR_A1);
        BK_DUMP_OUT("MPU->RBAR_A2: 0x%08X.\r\n", MPU->RBAR_A2);
        BK_DUMP_OUT("MPU->RLAR_A2: 0x%08X.\r\n", MPU->RLAR_A2);
        BK_DUMP_OUT("MPU->RBAR_A3: 0x%08X.\r\n", MPU->RBAR_A3);
        BK_DUMP_OUT("MPU->RLAR_A3: 0x%08X.\r\n", MPU->RLAR_A3);
        BK_DUMP_OUT("MPU->MAIR0: 0x%08X.\r\n", MPU->MAIR0);
        BK_DUMP_OUT("MPU->MAIR1: 0x%08X.\r\n", MPU->MAIR1);
    }
    BK_DUMP_OUT("******************** Dump mpu config begin ********************\r\n");
}

