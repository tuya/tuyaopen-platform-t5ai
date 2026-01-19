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
#include "timer_hw.h"
#include "timer_hal.h"
#include "timer_ll.h"

#if CFG_HAL_DEBUG_TIMER

void timer_struct_dump(void)
{
	timer_hw_t *hw = (timer_hw_t *)TIMER_LL_REG_BASE(0);
	SOC_LOGD("base=%x\r\n", (uint32_t)hw);

	for (int group = 0; group < SOC_TIMER_GROUP_NUM; group++) {
		SOC_LOGD("group(%x)\r\n", group);

		SOC_LOGD("  device_id=0x%x value=0x%x\n", &hw->group[group].dev_id, hw->group[group].dev_id);
		SOC_LOGD("  dev_version=0x%x value=0x%x\n", &hw->group[group].dev_version, hw->group[group].dev_version);


		SOC_LOGD("  global_ctrl=0x%x value=0x%x\n", &hw->group[group].global_ctrl, hw->group[group].global_ctrl.v);
		SOC_LOGD("    soft_reset:      %x\n", hw->group[group].global_ctrl.soft_reset);
		SOC_LOGD("    clk_gate_bypass: %x\n", hw->group[group].global_ctrl.clk_gate_bypass);
		SOC_LOGD("    reserved:        %x\n", hw->group[group].global_ctrl.reserved);
		SOC_LOGD("\r\n");

		SOC_LOGD("  dev_status=0x%x value=0x%x\n", &hw->group[group].dev_status, hw->group[group].dev_status);

		for (int chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_GROUP; chan++) {
			SOC_LOGD("  timer%x_val addr=%x value=%x\r\n", chan, &hw->group[group].timer_cnt[chan], hw->group[group].timer_cnt[chan]);
		}

		SOC_LOGD("\n");
		SOC_LOGD("  ctrl addr=%x value=%x\n", &hw->group[group].ctrl, hw->group[group].ctrl.v);
		SOC_LOGD("    timer0_en:  %x\n", hw->group[group].ctrl.timer0_en);
		SOC_LOGD("    timer1_en:  %x\n", hw->group[group].ctrl.timer1_en);
		SOC_LOGD("    timer2_en:  %x\n", hw->group[group].ctrl.timer2_en);
		SOC_LOGD("    clk_div:    %x\n", hw->group[group].ctrl.clk_div);
		SOC_LOGD("    timer0_int: %x\n", hw->group[group].ctrl.timer0_int_en);
		SOC_LOGD("    timer1_int: %x\n", hw->group[group].ctrl.timer1_int_en);
		SOC_LOGD("    timer2_int: %x\n", hw->group[group].ctrl.timer2_int_en);

		SOC_LOGD("\n");
		SOC_LOGD("  read_ctrl addr=%x value=%x\n", &hw->group[group].read_ctrl, hw->group[group].read_ctrl.v);
		SOC_LOGD("    timer_cnt_read: %x\n", hw->group[group].read_ctrl.timer_cnt_read);
		SOC_LOGD("    timer_index:    %x\n", hw->group[group].read_ctrl.timer_index);

		SOC_LOGD("\n");
		SOC_LOGD("  timer_cnt addr=%x value=%x\n", &hw->group[group].timer_read_value, hw->group[group].timer_read_value);
		SOC_LOGD("\n");
	}
}

#endif

