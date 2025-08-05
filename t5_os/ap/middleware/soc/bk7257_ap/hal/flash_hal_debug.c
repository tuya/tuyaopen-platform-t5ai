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
#include "flash_hal.h"
#include "flash_hw.h"
#include "flash_ll.h"

#if CFG_HAL_DEBUG_FLASH

void flash_struct_dump(void)
{
	flash_hw_t *hw = (flash_hw_t *)FLASH_LL_REG_BASE(0);
	SOC_LOGD("base=%x\r\n", (uint32_t)hw);

	/* REG_0x0 */
	SOC_LOGD("  dev_id=0x%x value=0x%x\n", &hw->dev_id, hw->dev_id);

	/* REG_0x1 */
	SOC_LOGD("  dev_version=0x%x value=0x%x\n", &hw->dev_version, hw->dev_version);

	/* REG_0x2 */
	SOC_LOGD("  global_ctrl=0x%x value=0x%x\n", &hw->global_ctrl, hw->global_ctrl.v);
	SOC_LOGD("    soft_reset:      %x\n", hw->global_ctrl.soft_reset);
	SOC_LOGD("    clk_gate_bypass: %x\n", hw->global_ctrl.clk_gate_bypass);
	SOC_LOGD("    reserved:        %x\n", hw->global_ctrl.reserved);
	SOC_LOGD("\r\n");

	/* REG_0x3 */
	SOC_LOGD("  global_status=0x%x value=0x%x\n", &hw->global_status, hw->global_status);
	SOC_LOGD("\r\n");

	/* REG_0x4 */
	SOC_LOGD("  op_ctrl=0x%x value=0x%x\n", &hw->op_ctrl, hw->op_ctrl.v);
	SOC_LOGD("    reserved: %x\n", hw->op_ctrl.reserved);
	SOC_LOGD("    op_sw:    %x\n", hw->op_ctrl.op_sw);
	SOC_LOGD("    wp_value: %x\n", hw->op_ctrl.wp_value);
	SOC_LOGD("    busy_sw:  %x\n", hw->op_ctrl.busy_sw);
	SOC_LOGD("\r\n");

	/* REG_0x5, data to be written from software to flash */
	SOC_LOGD("  data_sw_flash=0x%x value=0x%x\n", &hw->data_sw_flash, hw->data_sw_flash);

	/* REG_0x6, data read from flash to software */
	SOC_LOGD("  data_flash_sw=0x%x value=0x%x\n", &hw->data_flash_sw, hw->data_flash_sw);
	SOC_LOGD("\r\n");

	/* REG_0x7 */
	SOC_LOGD("  cmd_cfg=0x%x value=0x%x\n", &hw->cmd_cfg, hw->cmd_cfg.v);
	SOC_LOGD("    wrsr_cmd_reg: %x\n", hw->cmd_cfg.wrsr_cmd_reg);
	SOC_LOGD("    rdsr_cmd_reg: %x\n", hw->cmd_cfg.rdsr_cmd_reg);
	SOC_LOGD("    wrsr_cmd_sel: %x\n", hw->cmd_cfg.wrsr_cmd_sel);
	SOC_LOGD("    rdsr_cmd_sel: %x\n", hw->cmd_cfg.rdsr_cmd_sel);
	SOC_LOGD("    reserved:     %x\n", hw->cmd_cfg.reserved);
	SOC_LOGD("\r\n");

	/* REG_0x8 */
	SOC_LOGD("  rd_flash_id=0x%x value=0x%x\n", &hw->rd_flash_id, hw->rd_flash_id);
	SOC_LOGD("\r\n");

	/* REG_0x9 */
	SOC_LOGD("  state=0x%x value=0x%x\n", &hw->state, hw->state.v);
	SOC_LOGD("    status_reg:  %x\n", hw->state.status_reg);
	SOC_LOGD("    crc_err_num: %x\n", hw->state.crc_err_num);
	SOC_LOGD("    byte_sel_wr: %x\n", hw->state.byte_sel_wr);
	SOC_LOGD("    byte_sel_rd: %x\n", hw->state.byte_sel_rd);
	SOC_LOGD("    m_value:     %x\n", hw->state.m_value);
	SOC_LOGD("    pw_write:    %x\n", hw->state.pw_write);
	SOC_LOGD("    otp_sel:     %x\n", hw->state.otp_sel);
	SOC_LOGD("\r\n");

	/* REG_0xA */
	SOC_LOGD("  config=0x%x value=0x%x\n", &hw->config, hw->config.v);
	SOC_LOGD("    clk_cfg:        %x\n", hw->config.clk_cfg);
	SOC_LOGD("    mode_sel:       %x\n", hw->config.mode_sel);
	SOC_LOGD("    cpu_data_wr_en: %x\n", hw->config.cpu_data_wr_en);
	SOC_LOGD("    wrsr_data:      %x\n", hw->config.wrsr_data);
	SOC_LOGD("    crc_en:         %x\n", hw->config.crc_en);
	SOC_LOGD("    allff_reg:      %x\n", hw->config.allff_reg);
	SOC_LOGD("    fclk_gate_xaes: %x\n", hw->config.fclk_gate_xaes);
	SOC_LOGD("    reserved:       %x\n", hw->config.reserved);
	SOC_LOGD("\r\n");

	/* REG_0xB */
	SOC_LOGD("  ps_ctrl=0x%x value=0x%x\n", &hw->ps_ctrl, hw->ps_ctrl.v);
	SOC_LOGD("    tres1_trdp_delay_cnt: %x\n", hw->ps_ctrl.tres1_trdp_delay_cnt);
	SOC_LOGD("    tdp_trdpdd_delay_cnt: %x\n", hw->ps_ctrl.tdp_trdpdd_delay_cnt);
	SOC_LOGD("    dpd_fbd:              %x\n", hw->ps_ctrl.dpd_fbd);
	SOC_LOGD("    prefetch_version:     %x\n", hw->ps_ctrl.prefetch_version);
	SOC_LOGD("    reserved:             %x\n", hw->ps_ctrl.reserved);
	SOC_LOGD("    dpd_status:           %x\n", hw->ps_ctrl.dpd_status);
	SOC_LOGD("\r\n");

	/* REG_0xC */
	SOC_LOGD("  page_ctrl=0x%x value=0x%x\n", &hw->page_ctrl, hw->page_ctrl.v);
	SOC_LOGD("    mem_data:     %x\n", hw->page_ctrl.mem_data);
	SOC_LOGD("    reserved:     %x\n", hw->page_ctrl.reserved);
	SOC_LOGD("    mem_addr_clr: %x\n", hw->page_ctrl.mem_addr_clr);
	SOC_LOGD("\r\n");

	/* REG_0xd ~ REG_0x14 */
	for(uint32_t i = 0; i < SOC_SEC_ADDR_NUM; i++) {
		SOC_LOGD("  sec_addr%d_start=0x%x value=0x%x\n", i, &hw->sec_addr[i].sec_addr_start, hw->sec_addr[i].sec_addr_start.v);
		SOC_LOGD("    flash_sec_start_addr%d: %x\n", i, hw->sec_addr[i].sec_addr_start.flash_sec_start_addr);
		SOC_LOGD("    flash_sec_addr%d_en:    %x\n", i, hw->sec_addr[i].sec_addr_start.flash_sec_addr_en);
		SOC_LOGD("    reserved:               %x\n", hw->sec_addr[i].sec_addr_start.reserved);
		SOC_LOGD("\r\n");

		SOC_LOGD("  sec_addr%d_end=0x%x value=0x%x\n", i, &hw->sec_addr[i].sec_addr_end, hw->sec_addr[i].sec_addr_end.v);
		SOC_LOGD("    flash_sec_end_addr%d: %x\n", i, hw->sec_addr[i].sec_addr_end.flash_sec_end_addr);
		SOC_LOGD("    reserved:             %x\n", hw->sec_addr[i].sec_addr_end.reserved);
		SOC_LOGD("\r\n");
	}

	/* REG_0x15 */
	SOC_LOGD("  op_cmd=0x%x value=0x%x\n", &hw->op_cmd, hw->op_cmd.v);
	SOC_LOGD("    addr_sw_reg: %x\n", hw->op_cmd.addr_sw_reg);
	SOC_LOGD("    op_type_sw:  %x\n", hw->op_cmd.op_type_sw);
	SOC_LOGD("    reserved:    %x\n", hw->op_cmd.reserved);
	SOC_LOGD("\r\n");
}

#endif

