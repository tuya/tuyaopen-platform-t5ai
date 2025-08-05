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

#include "hal_config.h"
#include "pwm_hw.h"
#include "pwm_ll.h"
#include "pwm_hal_v1px.h"

//TODO finally we will automatically generate this code

#if CFG_HAL_DEBUG_PWM
void pwm_struct_dump(void)
{
	pwm_hw_t *hw = (pwm_hw_t *)PWM_LL_REG_BASE(0); //Currently only 1 PWM unit
	SOC_LOGD("TODO:base=%x\n", (uint32_t)hw);

#if 0
	for (int group = 0; group < SOC_PWM_GROUP_NUM; group ++) {
		SOC_LOGD("group(%x)\n", group);
		SOC_LOGD("  ctrl=%x value=%x\n", &hw->group[group].ctrl, hw->group[group].ctrl.v);

		SOC_LOGD("    chan0_mode: %x\n", hw->group[group].ctrl.chan0_mode);
		SOC_LOGD("    chan0_en: %x\n", hw->group[group].ctrl.chan0_en);
		SOC_LOGD("    chan0_int_en: %x\n", hw->group[group].ctrl.chan0_int_en);
		SOC_LOGD("    chan0_timer_stop: %x\n", hw->group[group].ctrl.chan0_timer_stop);
		SOC_LOGD("    chan0_init_level: %x\n", hw->group[group].ctrl.chan0_init_level);
		SOC_LOGD("    chan0_cfg_update: %x\n", hw->group[group].ctrl.chan0_cfg_update);

		SOC_LOGD("    chan1_mode: %x\n", hw->group[group].ctrl.chan1_mode);
		SOC_LOGD("    chan1_en: %x\n", hw->group[group].ctrl.chan1_en);
		SOC_LOGD("    chan1_int_en: %x\n", hw->group[group].ctrl.chan1_int_en);
		SOC_LOGD("    chan1_timer_stop: %x\n", hw->group[group].ctrl.chan1_timer_stop);
		SOC_LOGD("    chan1_init_level: %x\n", hw->group[group].ctrl.chan1_init_level);
		SOC_LOGD("    chan1_cfg_update: %x\n", hw->group[group].ctrl.chan1_cfg_update);

		SOC_LOGD("    pre_div: %x\n", hw->group[group].ctrl.pre_div);
		SOC_LOGD("    group_pwm_en: %x\n", hw->group[group].ctrl.group_pwm_en);
		SOC_LOGD("    group_en: %x\n", hw->group[group].ctrl.group_en);
		SOC_LOGD("    reserved: %x\n", hw->group[group].ctrl.reserved);
		SOC_LOGD("    chan0_int_st: %x\n", hw->group[group].ctrl.chan0_int_st);
		SOC_LOGD("    chan1_int_st: %x\n", hw->group[group].ctrl.chan1_int_st);

		SOC_LOGD("\n");
		for (int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_GROUP; chan++) {
			SOC_LOGD("  chan(%x) duty t1=%x t2=%x t3=%x period=%x\n", chan,
					 hw->group[group].duty_cycle[chan].t1, hw->group[group].duty_cycle[chan].t2,
					 hw->group[group].duty_cycle[chan].t3, hw->group[group].duty_cycle[chan].t4);
		}

		SOC_LOGD("\n");
		SOC_LOGD("  cna_read_ctrl(%x)=%x\n", &hw->group[group].cnt_read_ctrl, hw->group[group].cnt_read_ctrl);
		SOC_LOGD("    rd0=%x\n", hw->group[group].cnt_read_ctrl.rd0);
		SOC_LOGD("    rd1=%x\n", hw->group[group].cnt_read_ctrl.rd1);

		SOC_LOGD("\n");
		for (int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_GROUP; chan++) {
			SOC_LOGD("  chan(%x) cnt_or_capture_rdata=%x\n", chan,
					 hw->group[group].cnt_or_capture_rdata[chan]);
		}
	}
#endif
}

#endif
