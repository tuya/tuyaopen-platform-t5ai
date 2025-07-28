// Copyright 2022-2023 Beken
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
#include "yuv_buf_hal.h"
#include "yuv_buf_hw.h"

#if CFG_HAL_DEBUG_YUV_BUF

void yuv_buf_struct_dump(void)
{
	yuv_buf_hw_t *hw = (yuv_buf_hw_t *)YUV_BUF_LL_REG_BASE;
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

	/* REG_0x04 */
	SOC_LOGD("  ctrl=0x%x value=0x%x\r\n", &hw->ctrl, hw->ctrl.v);
	SOC_LOGD("    yuv_mode:           %x\r\n", hw->ctrl.yuv_mode);
	SOC_LOGD("    jpeg_word_reverse:  %x\r\n", hw->ctrl.jpeg_word_reverse);
	SOC_LOGD("    yuv_fmt_sel:        %x\r\n", hw->ctrl.yuv_fmt_sel);
	SOC_LOGD("    reserved:           %x\r\n", hw->ctrl.reserved0);
	SOC_LOGD("    vck_edge:           %x\r\n", hw->ctrl.vck_edge);
	SOC_LOGD("    hsync_rev:          %x\r\n", hw->ctrl.hsync_rev);
	SOC_LOGD("    vsync_rev:          %x\r\n", hw->ctrl.vsync_rev);
	SOC_LOGD("    partial_display_en: %x\r\n", hw->ctrl.partial_display_en);
	SOC_LOGD("    sync_edge_dect_en:  %x\r\n", hw->ctrl.sync_edge_dect_en);
	SOC_LOGD("    mclk_div:           %x\r\n", hw->ctrl.mclk_div);
	SOC_LOGD("    h264_mode:          %x\r\n", hw->ctrl.h264_mode);
	SOC_LOGD("    bps_cis:            %x\r\n", hw->ctrl.bps_cis);
	SOC_LOGD("    memrev:           %x\r\n", hw->ctrl.memrev);
	SOC_LOGD("    mclk_hold_en:       %x\r\n", hw->ctrl.mclk_hold_en);
	SOC_LOGD("    sen_mem_clr:        %x\r\n", hw->ctrl.sen_mem_clr);
	SOC_LOGD("    sen_mem_en_hd:      %x\r\n", hw->ctrl.sen_mem_en_hd);
	SOC_LOGD("    bus_dat_byte_reve:  %x\r\n", hw->ctrl.bus_dat_byte_reve);
	SOC_LOGD("    soi_hsync:          %x\r\n", hw->ctrl.soi_hsync);
	SOC_LOGD("    reserved:           %x\r\n", hw->ctrl.reserved2);
	SOC_LOGD("\r\n");

	/* REG_0x05 */
	SOC_LOGD("  pixel=0x%x value=0x%x\r\n", &hw->pixel, hw->pixel.v);
	SOC_LOGD("    x_pixel:      %x\r\n", hw->pixel.x_pixel);
	SOC_LOGD("    y_pixel:      %x\r\n", hw->pixel.y_pixel);
	SOC_LOGD("    frame_blk:    %x\r\n", hw->pixel.frame_blk);
	SOC_LOGD("\r\n");

	/* REG_0x06 */
	SOC_LOGD("  x_partial_offset=0x%x value=0x%x\r\n", &hw->x_partial_offset, hw->x_partial_offset.v);
	SOC_LOGD("    x_partial_offset_l: %x\r\n", hw->x_partial_offset.x_partial_offset_l);
	SOC_LOGD("    reserved0:          %x\r\n", hw->x_partial_offset.reserved0);
	SOC_LOGD("    x_partial_offset_r: %x\r\n", hw->x_partial_offset.x_partial_offset_r);
	SOC_LOGD("    reserved1:          %x\r\n", hw->x_partial_offset.reserved1);
	SOC_LOGD("\r\n");

	/* REG_0x07 */
	SOC_LOGD("  y_partial_offset=0x%x value=0x%x\r\n", &hw->y_partial_offset, hw->y_partial_offset.v);
	SOC_LOGD("    y_partial_offset_l: %x\r\n", hw->y_partial_offset.y_partial_offset_l);
	SOC_LOGD("    reserved0:          %x\r\n", hw->y_partial_offset.reserved0);
	SOC_LOGD("    y_partial_offset_r: %x\r\n", hw->y_partial_offset.y_partial_offset_r);
	SOC_LOGD("    reserved1:          %x\r\n", hw->y_partial_offset.reserved1);
	SOC_LOGD("\r\n");

	/* REG_0x08 */
	SOC_LOGD("  em_base_addr=0x%x value=0x%x\r\n", &hw->em_base_addr, hw->em_base_addr);
	SOC_LOGD("\r\n");

	/* REG_0x09 */
	SOC_LOGD("  int_en=0x%x value=0x%x\r\n", &hw->int_en, hw->int_en.v);
	SOC_LOGD("    vsync_nege_int_en: %x\r\n", hw->int_en.vsync_nege_int_en);
	SOC_LOGD("    yuv_arv_int_en:    %x\r\n", hw->int_en.yuv_arv_int_en);
	SOC_LOGD("    sm0_wr_int_en:     %x\r\n", hw->int_en.sm0_wr_int_en);
	SOC_LOGD("    sm1_wr_int_en:     %x\r\n", hw->int_en.sm1_wr_int_en);
	SOC_LOGD("    sen_full_int_en:   %x\r\n", hw->int_en.sen_full_int_en);
	SOC_LOGD("    enc_line_int_en:   %x\r\n", hw->int_en.enc_line_int_en);
	SOC_LOGD("    sen_resl_int_en:   %x\r\n", hw->int_en.sen_resl_int_en);
	SOC_LOGD("    h264_err_int_en:   %x\r\n", hw->int_en.h264_err_int_en);
	SOC_LOGD("    enc_slow_int_en:   %x\r\n", hw->int_en.enc_slow_int_en);
	SOC_LOGD("    reserved:          %x\r\n", hw->int_en.reserved);
	SOC_LOGD("\r\n");

	/* REG_0xa */
	SOC_LOGD("  int_status=0x%x value=0x%x\r\n", &hw->int_status, hw->int_status.v);
	SOC_LOGD("    vsync_nege_int: %x\r\n", hw->int_status.vsync_nege_int);
	SOC_LOGD("    yuv_arv_int:    %x\r\n", hw->int_status.yuv_arv_int);
	SOC_LOGD("    sm0_wr_int:     %x\r\n", hw->int_status.sm0_wr_int);
	SOC_LOGD("    sm1_wr_int:     %x\r\n", hw->int_status.sm1_wr_int);
	SOC_LOGD("    sen_full_int:   %x\r\n", hw->int_status.sen_full_int);
	SOC_LOGD("    enc_line_int:   %x\r\n", hw->int_status.enc_line_int);
	SOC_LOGD("    sen_resl_int:   %x\r\n", hw->int_status.sen_resl_int);
	SOC_LOGD("    h264_err_int:   %x\r\n", hw->int_status.h264_err_int);
	SOC_LOGD("    enc_slow_int:   %x\r\n", hw->int_status.enc_slow_int);
	SOC_LOGD("    reserved:       %x\r\n", hw->int_status.reserved);
	SOC_LOGD("\r\n");

	/* REG_0xb */
	SOC_LOGD("  int_masks=0x%x value=0x%x\r\n", &hw->int_masks, hw->int_masks.v);
	SOC_LOGD("    int_vsync_nege_mask: %x\r\n", hw->int_masks.int_vsync_nege_mask);
	SOC_LOGD("    int_resl_err_mask:   %x\r\n", hw->int_masks.int_resl_err_mask);
	SOC_LOGD("    int_sens_full_mask:  %x\r\n", hw->int_masks.int_sens_full_mask);
	SOC_LOGD("    int_enc_slow_mask:   %x\r\n", hw->int_masks.int_enc_slow_mask);
	SOC_LOGD("    int_h264_err_mask:   %x\r\n", hw->int_masks.int_h264_err_mask);
	SOC_LOGD("    reserved:            %x\r\n", hw->int_masks.reserved);
	SOC_LOGD("\r\n");

	/* REG_0xc */
	SOC_LOGD("  emr_base_addr=0x%x value=0x%x\r\n", &hw->emr_base_addr, hw->emr_base_addr);
	SOC_LOGD("\r\n");

	/* REG_0xd */
	SOC_LOGD("  resize_pixel=0x%x value=0x%x\r\n", &hw->resize_pixel, hw->resize_pixel.v);
	SOC_LOGD("    bps_pingpong:           %x\r\n", hw->resize_pixel.bps_pingpong);
	SOC_LOGD("    x_pixel_resize:         %x\r\n", hw->resize_pixel.x_pixel_resize);
	SOC_LOGD("    y_pixel_resize:         %x\r\n", hw->resize_pixel.y_pixel_resize);
	SOC_LOGD("    reserved:               %x\r\n", hw->resize_pixel.reserved);
	SOC_LOGD("\r\n");

	/* REG_0xf */
	SOC_LOGD("  renc_start=0x%x value=0x%x\r\n", &hw->renc_start, hw->renc_start.v);
	SOC_LOGD("    renc_start: %x\r\n", hw->renc_start.renc_start);
	SOC_LOGD("    reserved:   %x\r\n", hw->renc_start.reserved);
}

#endif

