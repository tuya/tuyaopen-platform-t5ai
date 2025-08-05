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
#include "jpeg_hal.h"
#include "jpeg_hw.h"
#include "jpeg_ll.h"

#if CFG_HAL_DEBUG_JPEG

void jpeg_struct_dump(void)
{
	SOC_LOGD("system_reg8:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x8 * 4));
	SOC_LOGD("system_reg9:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x9* 4));
	SOC_LOGD("system_rega:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0xa* 4));
	SOC_LOGD("system_clock_en_c:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0xc * 4));
	SOC_LOGD("system_clock_en_d:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0xd * 4));
	SOC_LOGD("cpu0_int_en_low:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x20 * 4));
	SOC_LOGD("cpu0_int_en_high:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x21 * 4));

	jpeg_hw_t *hw = (jpeg_hw_t *)JPEG_LL_REG_BASE(0);
	SOC_LOGD("base=%x\r\n", (uint32_t)hw);

	SOC_LOGD("  device_id=0x%x value=0x%x\r\n", &hw->dev_id, hw->dev_id);
	SOC_LOGD("  dev_version=0x%x value=0x%x\r\n", &hw->dev_version, hw->dev_version);

	SOC_LOGD("  global_ctrl=0x%x value=0x%x\r\n", &hw->global_ctrl, hw->global_ctrl.v);
	SOC_LOGD("    soft_reset:      %x\r\n", hw->global_ctrl.soft_reset);
	SOC_LOGD("    clk_gate_bypass: %x\r\n", hw->global_ctrl.clk_gate_bypass);
	SOC_LOGD("    reserved:        %x\r\n", hw->global_ctrl.reserved);
	SOC_LOGD("\r\n");

	SOC_LOGD("  dev_status=0x%x value=0x%x\r\n", &hw->dev_status, hw->dev_status);
	SOC_LOGD("\r\n");

	SOC_LOGD("  em_base_addr=0x%x value=0x%x\r\n", &hw->eof_offset, hw->eof_offset.v);
	SOC_LOGD("    reserved0:    %x\r\n", hw->eof_offset.reserved);
	SOC_LOGD("    eof_offset:   %x\r\n", hw->eof_offset.eof_offset);
	SOC_LOGD("\r\n");

	/* REG_0x5 */
	SOC_LOGD("  rx_fifo_data=0x%x value=0x%x\r\n", &hw->rx_fifo_data, hw->rx_fifo_data);

	/* REG_0x6 */
	SOC_LOGD("  int_status=0x%x value=0x%x\r\n", &hw->int_status, hw->int_status.v);
	SOC_LOGD("    int_status:     %x\r\n", hw->int_status.int_status);
	SOC_LOGD("    fifo_rd_finish: %x\r\n", hw->int_status.fifo_rd_finish);
	SOC_LOGD("    reserved:       %x\r\n", hw->int_status.reserved);
	SOC_LOGD("\r\n");

	/* REG_0x7 */
	SOC_LOGD("  byte_count_pfrm=0x%x value=0x%x\r\n", &hw->byte_count_pfrm, hw->byte_count_pfrm);
	SOC_LOGD("\r\n");

	/* REG_0x8 */
	SOC_LOGD("  fifo_status=0x%x value=0x%x\r\n", &hw->fifo_status, hw->fifo_status.v);
	SOC_LOGD("    reserved0:          %x\r\n", hw->fifo_status.reserved0);
	SOC_LOGD("    stream_fifo_empty:  %x\r\n", hw->fifo_status.stream_fifo_empty);
	SOC_LOGD("    stream_fifo_full:   %x\r\n", hw->fifo_status.stream_fifo_full);
	SOC_LOGD("    is_data_set:        %x\r\n", hw->fifo_status.is_data_set);
	SOC_LOGD("    reserved1:          %x\r\n", hw->fifo_status.reserved1);
	SOC_LOGD("\r\n");

	/* REG_0x9 */
	SOC_LOGD("  y_pcount=0x%x value=0x%x\r\n", &hw->y_count, hw->y_count.v);
	SOC_LOGD("    reserved0:          %x\r\n", hw->y_count.reserved0);
	SOC_LOGD("    y_count:            %x\r\n", hw->y_count.y_count);
	SOC_LOGD("\r\n");

	/* REG_0xa */
	SOC_LOGD("  byte_count_every_line=0x%x value=0x%x\r\n", &hw->byte_count_every_line, hw->byte_count_every_line);
	SOC_LOGD("\r\n");

	/* REG_0xc */
	SOC_LOGD("  int_en=0x%x value=0x%x\r\n", &hw->int_en, hw->int_en.v);
	SOC_LOGD("    frame_err_int_en:   %x\r\n", hw->int_en.frame_err_int_en);
	SOC_LOGD("    head_int_en:  %x\r\n", hw->int_en.head_int_en);
	SOC_LOGD("    sof_int_en:   %x\r\n", hw->int_en.sof_int_en);
	SOC_LOGD("    eof_int_en:   %x\r\n", hw->int_en.eof_int_en);
	SOC_LOGD("    reserved0:    %x\r\n", hw->int_en.reserved0);
	SOC_LOGD("    line_clear_int_en:  %x\r\n", hw->int_en.line_clear_int_en);
	SOC_LOGD("    reserved1:    %x\r\n", hw->int_en.reserved1);
	SOC_LOGD("\r\n");

	/* REG_0xd */
	SOC_LOGD("  cfg=0x%x value=0x%x\r\n", &hw->cfg, hw->int_en.v);
	SOC_LOGD("    reserved0:          %x\r\n", hw->cfg.reserved0);
	SOC_LOGD("    video_byte_reverse: %x\r\n", hw->cfg.video_byte_reverse);
	SOC_LOGD("    reserved1:          %x\r\n", hw->cfg.reserved1);
	SOC_LOGD("    jpeg_enc_en:        %x\r\n", hw->cfg.jpeg_enc_en);
	SOC_LOGD("    reserved2:          %x\r\n", hw->cfg.reserved2);
	SOC_LOGD("    x_pixel:            %x\r\n", hw->cfg.x_pixel);
	SOC_LOGD("    jpeg_enc_size:      %x\r\n", hw->cfg.jpeg_enc_size);
	SOC_LOGD("    bitrate_ctrl:       %x\r\n", hw->cfg.bitrate_ctrl);
	SOC_LOGD("    bitrate_step:       %x\r\n", hw->cfg.bitrate_step);
	SOC_LOGD("    auto_step:          %x\r\n", hw->cfg.auto_step);
	SOC_LOGD("    reserved3:          %x\r\n", hw->cfg.reserved3);
	SOC_LOGD("    bitrate_mode:       %x\r\n", hw->cfg.bitrate_mode);
	SOC_LOGD("    y_pixel:            %x\r\n", hw->cfg.y_pixel);
	SOC_LOGD("\r\n");

	SOC_LOGD("  target_byte_h=0x%x value=0x%x\r\n", &hw->target_byte_h, hw->target_byte_h);
	SOC_LOGD("  target_byte_l=0x%x value=0x%x\r\n", &hw->target_byte_l, hw->target_byte_l);
}

#endif

