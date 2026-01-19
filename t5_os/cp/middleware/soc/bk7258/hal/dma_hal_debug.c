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
#include "dma_hw.h"
#include "dma_hal.h"
#include "dma_ll.h"

#if CFG_HAL_DEBUG_DMA

static void dma_id_to_hw_ch(dma_id_t id,uint8_t *dma_num,uint8_t *channle)
{
	if(dma_num){
    *dma_num = id / SOC_DMA_CHAN_NUM_PER_UNIT;
	}
    if(channle){
	*channle = id % SOC_DMA_CHAN_NUM_PER_UNIT;
    }
}

void dma_struct_dump(dma_id_t id)
{
   uint8_t dma_num,dma_channel;
    dma_id_to_hw_ch(id,&dma_num,&dma_channel);
	dma_hw_t *hw = (dma_hw_t *)DMA_LL_REG_BASE(dma_num);

	SOC_LOGD("base=0x%x\r\n", (uint32_t)hw);
    id = dma_channel;
	SOC_LOGD("channel(%x)\r\n", id);

	SOC_LOGD("  ctrl addr=0x%x value=0x%x\n", &hw->config_group[id].ctrl, hw->config_group[id].ctrl.v);
	SOC_LOGD("    dma_en:                 %x\n", hw->config_group[id].ctrl.enable);
	SOC_LOGD("    finish_int_en:          %x\n", hw->config_group[id].ctrl.finish_int_en);
	SOC_LOGD("    half_finish_int_en:     %x\n", hw->config_group[id].ctrl.half_finish_int_en);
	SOC_LOGD("    dma_work_mode:          %x\n", hw->config_group[id].ctrl.mode);
	SOC_LOGD("    src_data_width:         %x\n", hw->config_group[id].ctrl.src_data_width);
	SOC_LOGD("    dest_data_width:        %x\n", hw->config_group[id].ctrl.dest_data_width);
	SOC_LOGD("    src_addr_increment_en:  %x\n", hw->config_group[id].ctrl.src_addr_inc_en);
	SOC_LOGD("    dest_addr_increment_en: %x\n", hw->config_group[id].ctrl.dest_addr_inc_en);
	SOC_LOGD("    src_addr_loop_en:       %x\n", hw->config_group[id].ctrl.src_addr_loop_en);
	SOC_LOGD("    dest_addr_loop_en:      %x\n", hw->config_group[id].ctrl.dest_addr_loop_en);
	SOC_LOGD("    channel_prioprity:      %x\n", hw->config_group[id].ctrl.chan_prio);
	SOC_LOGD("    reserved:               %x\n", hw->config_group[id].ctrl.reserved);
	SOC_LOGD("    transfer_len:           %x\n", hw->config_group[id].ctrl.transfer_len);
	SOC_LOGD("\n");

	SOC_LOGD("  dest_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_start_addr, hw->config_group[id].dest_start_addr);
	SOC_LOGD("  src_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_start_addr, hw->config_group[id].src_start_addr);
	SOC_LOGD("  dest_loop_end_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_loop_end_addr, hw->config_group[id].dest_loop_end_addr);
	SOC_LOGD("  dest_loop_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].dest_loop_start_addr, hw->config_group[id].dest_loop_start_addr);
	SOC_LOGD("  src_loop_end_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_loop_end_addr, hw->config_group[id].src_loop_end_addr);
	SOC_LOGD("  src_loop_start_addr addr=0x%x value=0x%x\n", &hw->config_group[id].src_loop_start_addr, hw->config_group[id].src_loop_start_addr);
	SOC_LOGD("\n");

	SOC_LOGD("  req_mux addr=0x%x value=0x%x\n", &hw->config_group[id].req_mux, hw->config_group[id].req_mux.v);
	SOC_LOGD("    source_request_mux:  %x\n", hw->config_group[id].req_mux.src_req_mux);
	SOC_LOGD("    dest_request_mux:    %x\n", hw->config_group[id].req_mux.dest_req_mux);
	SOC_LOGD("    reserved:            %x\n", hw->config_group[id].req_mux.reserved0);
	SOC_LOGD("    src_read_interval:   %x\n", hw->config_group[id].req_mux.src_read_interval);
	SOC_LOGD("    dest_write_interval: %x\n", hw->config_group[id].req_mux.dest_write_interval);
	SOC_LOGD("    pixel_trans_type: %x\n", hw->config_group[id].req_mux.pixel_trans_type);
	SOC_LOGD("    reserved:            %x\n", hw->config_group[id].req_mux.reserved1);
	SOC_LOGD("\n");

	SOC_LOGD("  DMA%x source_pause_addr addr=0x%x value=0x%x\n", id, &hw->config_group[id].src_pause_addr, hw->config_group[id].src_pause_addr);
	SOC_LOGD("  DMA%x dest_pause_addr addr=0x%x value=0x%x\n", id, &hw->config_group[id].dest_pause_addr, hw->config_group[id].dest_pause_addr);
	SOC_LOGD("  DMA%x source_read_addr addr=0x%x value=0x%x\n", id, &hw->config_group[id].src_rd_addr, hw->config_group[id].src_rd_addr);
	SOC_LOGD("  DMA%x dest_write_addr addr=0x%x value=0x%x\n", id, &hw->config_group[id].dest_wr_addr, hw->config_group[id].dest_wr_addr);
	SOC_LOGD("\n");

	SOC_LOGD("  dma_status addr=0x%x value=0x%x\n", &hw->config_group[id].status, hw->config_group[id].status.v);
	SOC_LOGD("    remain_len_indication:   %x\n", hw->config_group[id].status.remain_len);
	SOC_LOGD("    flush_src_buff:          %x\n", hw->config_group[id].status.flush_src_buff);
	SOC_LOGD("    reserved:                %x\n", hw->config_group[id].status.reserved);
	SOC_LOGD("    finish_int_counter:      %x\n", hw->config_group[id].status.finish_int_counter);
	SOC_LOGD("    half_finish_int_counter: %x\n", hw->config_group[id].status.half_finish_int_counter);
	SOC_LOGD("\n");

	SOC_LOGD("  prio_mode addr=0x%x value=0x%x\n", &hw->prio_mode, hw->prio_mode.v);
	SOC_LOGD("    prio_mode: %x\n", hw->prio_mode.prio_mode);
	SOC_LOGD("    reserved:  %x\n", hw->prio_mode.reserved);
	SOC_LOGD("\n");

#if CONFIG_SPE
	SOC_LOGD("  secure_attr addr=0x%x value=0x%x\n", &hw->secure_attr, hw->secure_attr.v);
	SOC_LOGD("  int_status_nonsec addr=0x%x value=0x%x\n", &hw->privileged_attr, hw->privileged_attr.v);
#endif
	SOC_LOGD("  int_status_sec addr=0x%x value=0x%x\n", &hw->int_status0, hw->int_status0.v);
	SOC_LOGD("  int_status_nonsec addr=0x%x value=0x%x\n", &hw->int_status1, hw->int_status1.v);
//	SOC_LOGD("  int_status3=0x%x value=0x%x\n", &hw->int_status3, hw->int_status3.v);
//	SOC_LOGD("  int_status4=0x%x value=0x%x\n", &hw->int_status4, hw->int_status4.v);
//	SOC_LOGD("  int_allocate addr=0x%x value=0x%x\n", &hw->int_allocate, hw->int_allocate.v);
	SOC_LOGD("\n");
}

#endif

