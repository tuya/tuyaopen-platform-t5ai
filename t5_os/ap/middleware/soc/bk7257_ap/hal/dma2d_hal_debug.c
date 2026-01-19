// Copyright 2021-2022 Beken
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

#include <soc/soc.h>
#include <soc/soc_port.h>
#include "hal_config.h"
#include "dma2d_struct.h"
#include "dma2d_ll_macro_def.h"

//#if CFG_HAL_DEBUG_DMA2D
//TODO modify it
#if 0
void dma2d_struct_dump()
{
	SOC_LOGD("    dma2d_reg_base=0x%x\r\n", DMA2D_LL_REG_BASE);

	SOC_LOGD("\n");
	SOC_LOGD("    device_id value=0x%x\n", dma2d_ll_get_output_offset_out_line_offset());

	SOC_LOGD("\n");
	SOC_LOGD("    version_id value=0x%x\n", dma2d_ll_get_version_id_value());

	SOC_LOGD("\n");
	SOC_LOGD("    soft_reset:    %x\n", dma2d_ll_get_module_control_soft_reset());
	SOC_LOGD("    clk_gate:    %x\n", dma2d_ll_get_module_control_clk_gate());

	SOC_LOGD("\n");
	SOC_LOGD("    global_status value=0x%x\n", dma2d_ll_get_global_status());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_control_reg value=0x%x\n", dma2d_ll_get_dma2d_control_reg_value());
	SOC_LOGD("    tran_start:    %x\n", dma2d_ll_get_dma2d_control_reg_tran_start());
	SOC_LOGD("    tran_suspend:    %x\n", dma2d_ll_get_dma2d_control_reg_tran_suspend());
	SOC_LOGD("    tran_abort:    %x\n", dma2d_ll_get_dma2d_control_reg_tran_abort());
	SOC_LOGD("    line_offset_mode:    %x\n", dma2d_ll_get_dma2d_control_reg_line_offset_mode());
	SOC_LOGD("    error_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_error_int_en());
	SOC_LOGD("    complete_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_complete_int_en());
	SOC_LOGD("    waterm_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_waterm_int_en());
	SOC_LOGD("    clut_error_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_clut_error_int_en());
	SOC_LOGD("    clut_cmplt_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_clut_cmplt_int_en());
	SOC_LOGD("    config_error_int_en:    %x\n", dma2d_ll_get_dma2d_control_reg_config_error_int_en());
	SOC_LOGD("    mode:    %x\n", dma2d_ll_get_dma2d_control_reg_mode());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_int_status value=0x%x\n", dma2d_ll_get_dma2d_int_status_value());
	SOC_LOGD("    error_int:    %x\n", dma2d_ll_get_dma2d_int_status_error_int());
	SOC_LOGD("    complete_int:    %x\n", dma2d_ll_get_dma2d_int_status_complete_int());
	SOC_LOGD("    waterm_int:    %x\n", dma2d_ll_get_dma2d_int_status_waterm_int());
	SOC_LOGD("    clut_error_int:    %x\n", dma2d_ll_get_dma2d_int_status_clut_error_int());
	SOC_LOGD("    clut_cmplt_int:    %x\n", dma2d_ll_get_dma2d_int_status_clut_cmplt_int());
	SOC_LOGD("    config_error_int:    %x\n", dma2d_ll_get_dma2d_int_status_config_error_int());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_fg_address value=0x%x\n", dma2d_ll_get_dma2d_fg_address_value());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_fg_offset value=0x%x\n", dma2d_ll_get_dma2d_fg_offset_value());
	SOC_LOGD("    fg_line_offset:	%x\n", dma2d_ll_get_dma2d_fg_offset_fg_line_offset());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_bg_address value=0x%x\n", dma2d_ll_get_dma2d_bg_address_value());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_bg_offset value=0x%x\n", dma2d_ll_get_dma2d_bg_offset_value());
	SOC_LOGD("    bg_line_offset:	%x\n", dma2d_ll_get_dma2d_bg_offset_bg_line_offset());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_fg_pfc_ctrl value=0x%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_value());
	SOC_LOGD("    fg_color_mode:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_color_mode());
	SOC_LOGD("    fg_clut_color_mode:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_clut_color_mode());
	SOC_LOGD("    fg_start_clut:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_start_clut());
	SOC_LOGD("    fg_clut_size:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_clut_size());
	SOC_LOGD("    fg_alpha_mode:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_alpha_mode());
	SOC_LOGD("    alpha_invert:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_alpha_invert());
	SOC_LOGD("    fg_rb_swap:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_rb_swap());
	SOC_LOGD("    fg_alpha:	%x\n", dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_alpha());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_fg_color_reg value=0x%x\n", dma2d_ll_get_dma2d_fg_color_reg_value());
	SOC_LOGD("    blue_value:	%x\n", dma2d_ll_get_dma2d_fg_color_reg_blue_value());
	SOC_LOGD("    green_value:	%x\n", dma2d_ll_get_dma2d_fg_color_reg_green_value());
	SOC_LOGD("    red_value:	%x\n", dma2d_ll_get_dma2d_fg_color_reg_red_value());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_bg_pfc_ctrl value=0x%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_value());
	SOC_LOGD("    bg_color_mode:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_color_mode());
	SOC_LOGD("    bg_clut_color_mode:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_clut_color_mode());
	SOC_LOGD("    bg_start_clut:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_start_clut());
	SOC_LOGD("    bg_clut_size:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_clut_size());
	SOC_LOGD("    bg_alpha_mode:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_alpha_mode());
	SOC_LOGD("    alpha_invert:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_alpha_invert());
	SOC_LOGD("    bg_rb_swap:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_rb_swap());
	SOC_LOGD("    bg_alpha:	%x\n", dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_alpha());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_bg_color_reg value=0x%x\n", dma2d_ll_get_dma2d_bg_color_reg_value());
	SOC_LOGD("    blue_value:	%x\n", dma2d_ll_get_dma2d_bg_color_reg_blue_value());
	SOC_LOGD("    green_value:	%x\n", dma2d_ll_get_dma2d_bg_color_reg_green_value());
	SOC_LOGD("    red_value:	%x\n", dma2d_ll_get_dma2d_bg_color_reg_red_value());

	SOC_LOGD("\n");
	SOC_LOGD("    fg_clut_mem_address value=0x%x\n", dma2d_ll_get_fg_clut_mem_address_value());

	SOC_LOGD("\n");
	SOC_LOGD("    bg_clut_mem_address value=0x%x\n", dma2d_ll_get_bg_clut_mem_address_value());

	SOC_LOGD("\n");
	SOC_LOGD("    out_pfc_contrl value=0x%x\n", dma2d_ll_get_out_pfc_contrl_value());
	SOC_LOGD("    out_color_mode:	%x\n", dma2d_ll_get_out_pfc_contrl_out_color_mode());
	SOC_LOGD("    out_alpha_invert:	%x\n", dma2d_ll_get_out_pfc_contrl_out_alpha_invert());
	SOC_LOGD("    out_rb_swap:	%x\n", dma2d_ll_get_out_pfc_contrl_out_rb_swap());

	SOC_LOGD("\n");
	SOC_LOGD("    out_color_reg value=0x%x\n", dma2d_ll_get_out_color_reg_value());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_out_mem_address value=0x%x\n", dma2d_ll_get_dma2d_out_mem_address_value());

	SOC_LOGD("\n");
	SOC_LOGD("    output_offset value=0x%x\n", dma2d_ll_get_output_offset_value());
	SOC_LOGD("    out_line_offset:	%x\n", dma2d_ll_get_output_offset_out_line_offset());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_number_of_line value=0x%x\n", dma2d_ll_get_dma2d_number_of_line_value());
	SOC_LOGD("    number_line:	%x\n", dma2d_ll_get_dma2d_number_of_line_number_line());
	SOC_LOGD("    pixel_line:	%x\n", dma2d_ll_get_dma2d_number_of_line_pexel_line());

	SOC_LOGD("\n");
	SOC_LOGD("    dma2d_line_watermark value=0x%x\n", dma2d_ll_get_dma2d_line_watermark_value());
	SOC_LOGD("    line_watermark:	%x\n", dma2d_ll_get_dma2d_line_watermark_line_watermark());

}

#endif

