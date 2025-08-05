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
#include "spi_hw.h"
#include "spi_hal.h"
#include "spi_ll.h"

#if CFG_HAL_DEBUG_SPI

void spi_struct_dump(spi_id_t id)
{
	spi_hw_t *hw = (spi_hw_t *)SPI_LL_REG_BASE(id);
	SOC_LOGD("base=%x\r\n", (uint32_t)hw);

	SOC_LOGD("  device_id=0x%x value=0x%x\n", &hw->dev_id, hw->dev_id);
	SOC_LOGD("  dev_version=0x%x value=0x%x\n", &hw->dev_version, hw->dev_version);

	SOC_LOGD("  global_ctrl=0x%x value=0x%x\n", &hw->global_ctrl, hw->global_ctrl.v);
	SOC_LOGD("    soft_reset:      %x\n", hw->global_ctrl.soft_reset);
	SOC_LOGD("    clk_gate_bypass: %x\n", hw->global_ctrl.clk_gate_bypass);
	SOC_LOGD("    reserved:        %x\n", hw->global_ctrl.reserved);
	SOC_LOGD("\r\n");

	SOC_LOGD("  dev_status=0x%x value=0x%x\n", &hw->dev_status, hw->dev_status);
	SOC_LOGD("\r\n");

	SOC_LOGD("  ctrl=0x%x value=0x%x\n", &hw->ctrl, hw->ctrl.v);
	SOC_LOGD("    tx_fifo_int_level:    %x\n", hw->ctrl.tx_fifo_int_level);
	SOC_LOGD("    rx_fifo_int_level:    %x\n", hw->ctrl.rx_fifo_int_level);
	SOC_LOGD("    tx_udf_int_en:        %x\n", hw->ctrl.tx_udf_int_en);
	SOC_LOGD("    rx_ovf_int_en:        %x\n", hw->ctrl.rx_ovf_int_en);
	SOC_LOGD("    tx_fifo_int_en:       %x\n", hw->ctrl.tx_fifo_int_en);
	SOC_LOGD("    rx_fifo_int_en:       %x\n", hw->ctrl.rx_fifo_int_en);
	SOC_LOGD("    clk_rate:             %x\n", hw->ctrl.clk_rate);
	SOC_LOGD("    slave_release_int_en: %x\n", hw->ctrl.slave_release_int_en);
	SOC_LOGD("    wire3_en:             %x\n", hw->ctrl.wire3_en);
	SOC_LOGD("    bit_width:            %x\n", hw->ctrl.bit_width);
	SOC_LOGD("    lsb_first_en:         %x\n", hw->ctrl.lsb_first_en);
	SOC_LOGD("    cpol:                 %x\n", hw->ctrl.cpol);
	SOC_LOGD("    cpha:                 %x\n", hw->ctrl.cpha);
	SOC_LOGD("    master_en:            %x\n", hw->ctrl.master_en);
	SOC_LOGD("    enable:               %x\n", hw->ctrl.enable);
	SOC_LOGD("    byte_interval:        %x\n", hw->ctrl.byte_interval);

	SOC_LOGD("\n");
	SOC_LOGD("  config=0x%x value=0x%x\n", &hw->cfg, hw->cfg.v);
	SOC_LOGD("    tx_en:            %x\n", hw->cfg.tx_en);
	SOC_LOGD("    rx_en:            %x\n", hw->cfg.rx_en);
	SOC_LOGD("    tx_finish_int_en: %x\n", hw->cfg.tx_finish_int_en);
	SOC_LOGD("    rx_finish_int_en: %x\n", hw->cfg.rx_finish_int_en);
	SOC_LOGD("    tx_trans_len:     %x\n", hw->cfg.tx_trans_len);
	SOC_LOGD("    rx_trans_len:     %x\n", hw->cfg.rx_trans_len);

	SOC_LOGD("\n");
	SOC_LOGD("  status=0x%x value=0x%x\n", &hw->int_status, hw->int_status.v);
	SOC_LOGD("    tx_fifo_wr_ready:  %x\n", hw->int_status.tx_fifo_wr_ready);
	SOC_LOGD("    rx_fifo_rd_ready:  %x\n", hw->int_status.rx_fifo_rd_ready);
	SOC_LOGD("    tx_fifo_int:       %x\n", hw->int_status.tx_fifo_int);
	SOC_LOGD("    rx_fifo_int:       %x\n", hw->int_status.rx_fifo_int);
	SOC_LOGD("    slave_release_int: %x\n", hw->int_status.slave_release_int);
	SOC_LOGD("    tx_underflow:      %x\n", hw->int_status.tx_underflow_int);
	SOC_LOGD("    rx_overflow:       %x\n", hw->int_status.rx_overflow_int);
	SOC_LOGD("    tx_finish_int:     %x\n", hw->int_status.tx_finish_int);
	SOC_LOGD("    rx_finish_int:     %x\n", hw->int_status.rx_finish_int);
	SOC_LOGD("    tx_fifo_clr:       %x\n", hw->int_status.tx_fifo_clr);
	SOC_LOGD("    rx_fifo_clr:       %x\n", hw->int_status.rx_fifo_clr);

	SOC_LOGD("\n");
	SOC_LOGD("  data=0x%x value=0x%x\n", &hw->data, hw->data.v);
	SOC_LOGD("    fifo_data:  %x\n", hw->data.fifo_data);
}

#endif

