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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "sys_hw.h"
#include "sys_hal.h"

#if CONFIG_HAL_DEBUG_SYS
typedef void (*sys_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	sys_dump_fn_t fn;
} sys_reg_fn_map_t;

static void sys_dump_device_id(void)
{
	SOC_LOGD("device_id: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x0 << 2)));
}

static void sys_dump_version_id(void)
{
	SOC_LOGD("version_id: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x1 << 2)));
}

static void sys_dump_cpu_storage_connect_op_select(void)
{
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t *)(SOC_SYS_REG_BASE + (0x2 << 2));

	SOC_LOGD("cpu_storage_connect_op_select: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x2 << 2)));
	SOC_LOGD("	boot_mode: %8x\r\n", r->boot_mode);
	SOC_LOGD("	reserved_bit_1_6: %8x\r\n", r->reserved_bit_1_6);
	SOC_LOGD("	jtag_core_sel: %8x\r\n", r->jtag_core_sel);
	SOC_LOGD("	flash_sel: %8x\r\n", r->flash_sel);
	SOC_LOGD("	reserved_bit_10_31: %8x\r\n", r->reserved_bit_10_31);
}

static void sys_dump_cpu_current_run_status(void)
{
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t *)(SOC_SYS_REG_BASE + (0x3 << 2));

	SOC_LOGD("cpu_current_run_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x3 << 2)));
	SOC_LOGD("	core0_halted: %8x\r\n", r->core0_halted);
	SOC_LOGD("	core1_halted: %8x\r\n", r->core1_halted);
	SOC_LOGD("	core2_halted: %8x\r\n", r->core2_halted);
	SOC_LOGD("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGD("	cpu0_sw_reset: %8x\r\n", r->cpu0_sw_reset);
	SOC_LOGD("	cpu1_sw_reset: %8x\r\n", r->cpu1_sw_reset);
	SOC_LOGD("	cpu2_sw_reset: %8x\r\n", r->cpu2_sw_reset);
	SOC_LOGD("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGD("	cpu0_pwr_dw_state: %8x\r\n", r->cpu0_pwr_dw_state);
	SOC_LOGD("	cpu1_pwr_dw_state: %8x\r\n", r->cpu1_pwr_dw_state);
	SOC_LOGD("	cpu2_pwr_dw_state: %8x\r\n", r->cpu2_pwr_dw_state);
	SOC_LOGD("	reserved_bit_11_31: %8x\r\n", r->reserved_bit_11_31);
}

static void sys_dump_cpu0_int_halt_clk_op(void)
{
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t *)(SOC_SYS_REG_BASE + (0x4 << 2));

	SOC_LOGD("cpu0_int_halt_clk_op: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4 << 2)));
	SOC_LOGD("	cpu0_sw_rst: %8x\r\n", r->cpu0_sw_rst);
	SOC_LOGD("	cpu0_pwr_dw: %8x\r\n", r->cpu0_pwr_dw);
	SOC_LOGD("	cpu0_int_mask: %8x\r\n", r->cpu0_int_mask);
	SOC_LOGD("	cpu0_halt: %8x\r\n", r->cpu0_halt);
	SOC_LOGD("	cpu0_speed: %8x\r\n", r->cpu0_speed);
	SOC_LOGD("	cpu0_rxevt_sel: %8x\r\n", r->cpu0_rxevt_sel);
	SOC_LOGD("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGD("	cpu0_offset: %8x\r\n", r->cpu0_offset);
}

static void sys_dump_cpu1_int_halt_clk_op(void)
{
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t *)(SOC_SYS_REG_BASE + (0x5 << 2));

	SOC_LOGD("cpu1_int_halt_clk_op: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x5 << 2)));
	SOC_LOGD("	cpu1_sw_rst: %8x\r\n", r->cpu1_sw_rst);
	SOC_LOGD("	cpu1_pwr_dw: %8x\r\n", r->cpu1_pwr_dw);
	SOC_LOGD("	cpu1_int_mask: %8x\r\n", r->cpu1_int_mask);
	SOC_LOGD("	cpu1_halt: %8x\r\n", r->cpu1_halt);
	SOC_LOGD("	cpu1_speed : %8x\r\n", r->cpu1_speed );
	SOC_LOGD("	cpu1_rxevt_sel: %8x\r\n", r->cpu1_rxevt_sel);
	SOC_LOGD("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGD("	cpu1_offset: %8x\r\n", r->cpu1_offset);
}

static void sys_dump_cpu2_int_halt_clk_op(void)
{
	sys_cpu2_int_halt_clk_op_t *r = (sys_cpu2_int_halt_clk_op_t *)(SOC_SYS_REG_BASE + (0x6 << 2));

	SOC_LOGD("cpu2_int_halt_clk_op: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x6 << 2)));
	SOC_LOGD("	cpu2_sw_rst: %8x\r\n", r->cpu2_sw_rst);
	SOC_LOGD("	cpu2_pwr_dw: %8x\r\n", r->cpu2_pwr_dw);
	SOC_LOGD("	cpu2_int_mask: %8x\r\n", r->cpu2_int_mask);
	SOC_LOGD("	cpu2_halt: %8x\r\n", r->cpu2_halt);
	SOC_LOGD("	cpu2_speed : %8x\r\n", r->cpu2_speed );
	SOC_LOGD("	cpu2_rxevt_sel: %8x\r\n", r->cpu2_rxevt_sel);
	SOC_LOGD("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGD("	cpu2_offset: %8x\r\n", r->cpu2_offset);
}

static void sys_dump_reserved_reg0x7(void)
{
	SOC_LOGD("reserved_reg0x7: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x7 << 2)));
}

static void sys_dump_cpu_clk_div_mode1(void)
{
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t *)(SOC_SYS_REG_BASE + (0x8 << 2));

	SOC_LOGD("cpu_clk_div_mode1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x8 << 2)));
	SOC_LOGD("	clkdiv_core: %8x\r\n", r->clkdiv_core);
	SOC_LOGD("	cksel_core: %8x\r\n", r->cksel_core);
	SOC_LOGD("	clkdiv_bus: %8x\r\n", r->clkdiv_bus);
	SOC_LOGD("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGD("	clkdiv_uart0: %8x\r\n", r->clkdiv_uart0);
	SOC_LOGD("	clksel_uart0: %8x\r\n", r->clksel_uart0);
	SOC_LOGD("	clkdiv_uart1: %8x\r\n", r->clkdiv_uart1);
	SOC_LOGD("	cksel_uart1: %8x\r\n", r->cksel_uart1);
	SOC_LOGD("	clkdiv_uart2: %8x\r\n", r->clkdiv_uart2);
	SOC_LOGD("	cksel_uart2: %8x\r\n", r->cksel_uart2);
	SOC_LOGD("	cksel_sadc: %8x\r\n", r->cksel_sadc);
	SOC_LOGD("	cksel_pwm0: %8x\r\n", r->cksel_pwm0);
	SOC_LOGD("	cksel_pwm1: %8x\r\n", r->cksel_pwm1);
	SOC_LOGD("	cksel_tim0: %8x\r\n", r->cksel_tim0);
	SOC_LOGD("	cksel_tim1: %8x\r\n", r->cksel_tim1);
	SOC_LOGD("	cksel_tim2: %8x\r\n", r->cksel_tim2);
	SOC_LOGD("	cksel_can: %8x\r\n", r->cksel_can);
	SOC_LOGD("	cksel_i2s: %8x\r\n", r->cksel_i2s);
	SOC_LOGD("	cksel_aud: %8x\r\n", r->cksel_aud);
	SOC_LOGD("	clkdiv_jpeg: %8x\r\n", r->clkdiv_jpeg);
	SOC_LOGD("	cksel_jpeg: %8x\r\n", r->cksel_jpeg);
	SOC_LOGD("	clkdiv_disp_l: %8x\r\n", r->clkdiv_disp_l);
}

static void sys_dump_cpu_clk_div_mode2(void)
{
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t *)(SOC_SYS_REG_BASE + (0x9 << 2));

	SOC_LOGD("cpu_clk_div_mode2: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x9 << 2)));
	SOC_LOGD("	clkdiv_disp_h: %8x\r\n", r->clkdiv_disp_h);
	SOC_LOGD("	cksel_disp: %8x\r\n", r->cksel_disp);
	SOC_LOGD("	ckdiv_psram: %8x\r\n", r->ckdiv_psram);
	SOC_LOGD("	cksel_psram: %8x\r\n", r->cksel_psram);
	SOC_LOGD("	ckdiv_qspi0: %8x\r\n", r->ckdiv_qspi0);
	SOC_LOGD("	cksel_qspi0: %8x\r\n", r->cksel_qspi0);
	SOC_LOGD("	cksel_i2s1: %8x\r\n", r->cksel_i2s1);
	SOC_LOGD("	cksel_i2s2: %8x\r\n", r->cksel_i2s2);
	SOC_LOGD("	reserved_13_13: %8x\r\n", r->reserved_13_13);
	SOC_LOGD("	ckdiv_sdio: %8x\r\n", r->ckdiv_sdio);
	SOC_LOGD("	cksel_sdio: %8x\r\n", r->cksel_sdio);
	SOC_LOGD("	ckdiv_auxs: %8x\r\n", r->ckdiv_auxs);
	SOC_LOGD("	cksel_auxs: %8x\r\n", r->cksel_auxs);
	SOC_LOGD("	cksel_flash: %8x\r\n", r->cksel_flash);
	SOC_LOGD("	ckdiv_flash: %8x\r\n", r->ckdiv_flash);
	SOC_LOGD("	ckdiv_i2s0: %8x\r\n", r->ckdiv_i2s0);
	SOC_LOGD("	clkdiv_auxs: %8x\r\n", r->clkdiv_auxs);
}

static void sys_dump_cpu_26m_wdt_clk_div(void)
{
	sys_cpu_26m_wdt_clk_div_t *r = (sys_cpu_26m_wdt_clk_div_t *)(SOC_SYS_REG_BASE + (0xa << 2));

	SOC_LOGD("cpu_26m_wdt_clk_div: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xa << 2)));
	SOC_LOGD("	ckdiv_26m: %8x\r\n", r->ckdiv_26m);
	SOC_LOGD("	ckdiv_wdt: %8x\r\n", r->ckdiv_wdt);
	SOC_LOGD("	clksel_spi0: %8x\r\n", r->clksel_spi0);
	SOC_LOGD("	clksel_spi1: %8x\r\n", r->clksel_spi1);
	SOC_LOGD("	ckdiv_qspi1: %8x\r\n", r->ckdiv_qspi1);
	SOC_LOGD("	cksel_qspi1: %8x\r\n", r->cksel_qspi1);
	SOC_LOGD("	ckdiv_enet: %8x\r\n", r->ckdiv_enet);
	SOC_LOGD("	cksel_enet: %8x\r\n", r->cksel_enet);
	SOC_LOGD("	ckdiv_jpeg: %8x\r\n", r->ckdiv_jpeg);
	SOC_LOGD("	cksel_jpeg: %8x\r\n", r->cksel_jpeg);
	SOC_LOGD("	cksel_auxs_cis: %8x\r\n", r->cksel_auxs_cis);
	SOC_LOGD("	ckdiv_auxs_cis: %8x\r\n", r->ckdiv_auxs_cis);
	SOC_LOGD("	reserved_22_31: %8x\r\n", r->reserved_22_31);
}

static void sys_dump_cpu_anaspi_freq(void)
{
	sys_cpu_anaspi_freq_t *r = (sys_cpu_anaspi_freq_t *)(SOC_SYS_REG_BASE + (0xb << 2));

	SOC_LOGD("cpu_anaspi_freq: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xb << 2)));
	SOC_LOGD("	anaspi_freq: %8x\r\n", r->anaspi_freq);
	SOC_LOGD("	reserved_bit_6_31: %8x\r\n", r->reserved_bit_6_31);
}

static void sys_dump_cpu_device_clk_enable(void)
{
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t *)(SOC_SYS_REG_BASE + (0xc << 2));

	SOC_LOGD("cpu_device_clk_enable: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xc << 2)));
	SOC_LOGD("	i2c0_cken: %8x\r\n", r->i2c0_cken);
	SOC_LOGD("	spi0_cken: %8x\r\n", r->spi0_cken);
	SOC_LOGD("	uart0_cken: %8x\r\n", r->uart0_cken);
	SOC_LOGD("	pwm0_cken: %8x\r\n", r->pwm0_cken);
	SOC_LOGD("	tim0_cken: %8x\r\n", r->tim0_cken);
	SOC_LOGD("	sadc_cken: %8x\r\n", r->sadc_cken);
	SOC_LOGD("	irda_cken: %8x\r\n", r->irda_cken);
	SOC_LOGD("	efuse_cken: %8x\r\n", r->efuse_cken);
	SOC_LOGD("	i2c1_cken: %8x\r\n", r->i2c1_cken);
	SOC_LOGD("	spi1_cken: %8x\r\n", r->spi1_cken);
	SOC_LOGD("	uart1_cken: %8x\r\n", r->uart1_cken);
	SOC_LOGD("	uart2_cken: %8x\r\n", r->uart2_cken);
	SOC_LOGD("	pwm1_cken: %8x\r\n", r->pwm1_cken);
	SOC_LOGD("	tim1_cken: %8x\r\n", r->tim1_cken);
	SOC_LOGD("	tim2_cken: %8x\r\n", r->tim2_cken);
	SOC_LOGD("	otp_cken: %8x\r\n", r->otp_cken);
	SOC_LOGD("	i2s_cken: %8x\r\n", r->i2s_cken);
	SOC_LOGD("	usb_cken: %8x\r\n", r->usb_cken);
	SOC_LOGD("	can_cken: %8x\r\n", r->can_cken);
	SOC_LOGD("	psram_cken: %8x\r\n", r->psram_cken);
	SOC_LOGD("	qspi0_cken: %8x\r\n", r->qspi0_cken);
	SOC_LOGD("	qspi1_cken: %8x\r\n", r->qspi1_cken);
	SOC_LOGD("	sdio_cken: %8x\r\n", r->sdio_cken);
	SOC_LOGD("	auxs_cken: %8x\r\n", r->auxs_cken);
	SOC_LOGD("	btdm_cken: %8x\r\n", r->btdm_cken);
	SOC_LOGD("	xvr_cken: %8x\r\n", r->xvr_cken);
	SOC_LOGD("	mac_cken: %8x\r\n", r->mac_cken);
	SOC_LOGD("	phy_cken: %8x\r\n", r->phy_cken);
	SOC_LOGD("	jpeg_cken: %8x\r\n", r->jpeg_cken);
	SOC_LOGD("	disp_cken: %8x\r\n", r->disp_cken);
	SOC_LOGD("	aud_cken: %8x\r\n", r->aud_cken);
	SOC_LOGD("	wdt_cken: %8x\r\n", r->wdt_cken);
}

static void sys_dump_reserver_reg0xd(void)
{
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t *)(SOC_SYS_REG_BASE + (0xd << 2));

	SOC_LOGD("reserver_reg0xd: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xd << 2)));
	SOC_LOGD("	h264_cken: %8x\r\n", r->h264_cken);
	SOC_LOGD("	i2s1_cken: %8x\r\n", r->i2s1_cken);
	SOC_LOGD("	i2s2_cken: %8x\r\n", r->i2s2_cken);
	SOC_LOGD("	yuv_cken: %8x\r\n", r->yuv_cken);
	SOC_LOGD("	slcd_cken: %8x\r\n", r->slcd_cken);
	SOC_LOGD("	lin_cken: %8x\r\n", r->lin_cken);
	SOC_LOGD("	scr_cken: %8x\r\n", r->scr_cken);
	SOC_LOGD("	enet_cken: %8x\r\n", r->enet_cken);
	SOC_LOGD("	jpeg_cken: %8x\r\n", r->jpeg_cken);
	SOC_LOGD("	cis_auxs_cken: %8x\r\n", r->cis_auxs_cken);
	SOC_LOGD("	reserved_10_31: %8x\r\n", r->reserved_10_31);
}

static void sys_dump_cpu_device_mem_ctrl1(void)
{
	sys_cpu_device_mem_ctrl1_t *r = (sys_cpu_device_mem_ctrl1_t *)(SOC_SYS_REG_BASE + (0xe << 2));

	SOC_LOGD("cpu_device_mem_ctrl1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xe << 2)));
	SOC_LOGD("	uart1_sd: %8x\r\n", r->uart1_sd);
	SOC_LOGD("	uart2_sd: %8x\r\n", r->uart2_sd);
	SOC_LOGD("	spi1_sd: %8x\r\n", r->spi1_sd);
	SOC_LOGD("	sdio_sd: %8x\r\n", r->sdio_sd);
	SOC_LOGD("	usb_sd: %8x\r\n", r->usb_sd);
	SOC_LOGD("	can_sd: %8x\r\n", r->can_sd);
	SOC_LOGD("	qspi0_sd: %8x\r\n", r->qspi0_sd);
	SOC_LOGD("	qspi1_sd: %8x\r\n", r->qspi1_sd);
	SOC_LOGD("	pram_sd: %8x\r\n", r->pram_sd);
	SOC_LOGD("	fft_sd: %8x\r\n", r->fft_sd);
	SOC_LOGD("	abc_sd: %8x\r\n", r->abc_sd);
	SOC_LOGD("	aud_sd: %8x\r\n", r->aud_sd);
	SOC_LOGD("	i2s0_sd: %8x\r\n", r->i2s0_sd);
	SOC_LOGD("	i2s1_sd: %8x\r\n", r->i2s1_sd);
	SOC_LOGD("	i2s2_sd: %8x\r\n", r->i2s2_sd);
	SOC_LOGD("	jpge_sd: %8x\r\n", r->jpge_sd);
	SOC_LOGD("	yuv_sd: %8x\r\n", r->yuv_sd);
	SOC_LOGD("	jpgd_sd: %8x\r\n", r->jpgd_sd);
	SOC_LOGD("	disp_sd: %8x\r\n", r->disp_sd);
	SOC_LOGD("	dmad_sd: %8x\r\n", r->dmad_sd);
	SOC_LOGD("	h26f_sd: %8x\r\n", r->h26f_sd);
	SOC_LOGD("	mac_sd: %8x\r\n", r->mac_sd);
	SOC_LOGD("	phy_sd: %8x\r\n", r->phy_sd);
	SOC_LOGD("	xvr_sd: %8x\r\n", r->xvr_sd);
	SOC_LOGD("	irda_sd: %8x\r\n", r->irda_sd);
	SOC_LOGD("	la_sd: %8x\r\n", r->la_sd);
	SOC_LOGD("	flsh_sd: %8x\r\n", r->flsh_sd);
	SOC_LOGD("	uart_sd: %8x\r\n", r->uart_sd);
	SOC_LOGD("	spi0_sd: %8x\r\n", r->spi0_sd);
	SOC_LOGD("	enc_sd: %8x\r\n", r->enc_sd);
	SOC_LOGD("	dma0_sd: %8x\r\n", r->dma0_sd);
	SOC_LOGD("	dma1_sd: %8x\r\n", r->dma1_sd);
}

static void sys_dump_cpu_device_mem_ctrl2(void)
{
	sys_cpu_device_mem_ctrl2_t *r = (sys_cpu_device_mem_ctrl2_t *)(SOC_SYS_REG_BASE + (0xf << 2));

	SOC_LOGD("cpu_device_mem_ctrl2: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0xf << 2)));
	SOC_LOGD("	uart1_ds: %8x\r\n", r->uart1_ds);
	SOC_LOGD("	uart2_ds: %8x\r\n", r->uart2_ds);
	SOC_LOGD("	spi1_ds: %8x\r\n", r->spi1_ds);
	SOC_LOGD("	sdio_ds: %8x\r\n", r->sdio_ds);
	SOC_LOGD("	usb_ds: %8x\r\n", r->usb_ds);
	SOC_LOGD("	can_ds: %8x\r\n", r->can_ds);
	SOC_LOGD("	qspi0_ds: %8x\r\n", r->qspi0_ds);
	SOC_LOGD("	qspi1_ds: %8x\r\n", r->qspi1_ds);
	SOC_LOGD("	pram_ds: %8x\r\n", r->pram_ds);
	SOC_LOGD("	fft_ds: %8x\r\n", r->fft_ds);
	SOC_LOGD("	abc_ds: %8x\r\n", r->abc_ds);
	SOC_LOGD("	aud_ds: %8x\r\n", r->aud_ds);
	SOC_LOGD("	i2s0_ds: %8x\r\n", r->i2s0_ds);
	SOC_LOGD("	i2s1_ds: %8x\r\n", r->i2s1_ds);
	SOC_LOGD("	i2s2_ds: %8x\r\n", r->i2s2_ds);
	SOC_LOGD("	jpge_ds: %8x\r\n", r->jpge_ds);
	SOC_LOGD("	yuv_ds: %8x\r\n", r->yuv_ds);
	SOC_LOGD("	jpgd_ds: %8x\r\n", r->jpgd_ds);
	SOC_LOGD("	disp_ds: %8x\r\n", r->disp_ds);
	SOC_LOGD("	dmad_ds: %8x\r\n", r->dmad_ds);
	SOC_LOGD("	h26f_ds: %8x\r\n", r->h26f_ds);
	SOC_LOGD("	mac_ds: %8x\r\n", r->mac_ds);
	SOC_LOGD("	phy_ds: %8x\r\n", r->phy_ds);
	SOC_LOGD("	xvr_ds: %8x\r\n", r->xvr_ds);
	SOC_LOGD("	irda_ds: %8x\r\n", r->irda_ds);
	SOC_LOGD("	la_ds: %8x\r\n", r->la_ds);
	SOC_LOGD("	flsh_ds: %8x\r\n", r->flsh_ds);
	SOC_LOGD("	uart_ds: %8x\r\n", r->uart_ds);
	SOC_LOGD("	spi0_ds: %8x\r\n", r->spi0_ds);
	SOC_LOGD("	enc_ds: %8x\r\n", r->enc_ds);
	SOC_LOGD("	dma0_ds: %8x\r\n", r->dma0_ds);
	SOC_LOGD("	dma1_ds: %8x\r\n", r->dma1_ds);
}

static void sys_dump_cpu_power_sleep_wakeup(void)
{
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t *)(SOC_SYS_REG_BASE + (0x10 << 2));

	SOC_LOGD("cpu_power_sleep_wakeup: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x10 << 2)));
	SOC_LOGD("	pwd_mem1: %8x\r\n", r->pwd_mem1);
	SOC_LOGD("	pwd_mem2: %8x\r\n", r->pwd_mem2);
	SOC_LOGD("	pwd_mem3: %8x\r\n", r->pwd_mem3);
	SOC_LOGD("	pwd_encp: %8x\r\n", r->pwd_encp);
	SOC_LOGD("	pwd_bakp: %8x\r\n", r->pwd_bakp);
	SOC_LOGD("	pwd_ahbp: %8x\r\n", r->pwd_ahbp);
	SOC_LOGD("	pwd_audp: %8x\r\n", r->pwd_audp);
	SOC_LOGD("	pwd_vidp: %8x\r\n", r->pwd_vidp);
	SOC_LOGD("	pwd_btsp: %8x\r\n", r->pwd_btsp);
	SOC_LOGD("	pwd_wifp_mac: %8x\r\n", r->pwd_wifp_mac);
	SOC_LOGD("	pwd_wifp_phy: %8x\r\n", r->pwd_wifp_phy);
	SOC_LOGD("	pwd_mem0: %8x\r\n", r->pwd_mem0);
	SOC_LOGD("	pwd_mem4: %8x\r\n", r->pwd_mem4);
	SOC_LOGD("	pwd_ofdm: %8x\r\n", r->pwd_ofdm);
	SOC_LOGD("	pwd_mem5: %8x\r\n", r->pwd_mem5);
	SOC_LOGD("	rom_pgen: %8x\r\n", r->rom_pgen);
	SOC_LOGD("	sleep_en_need_flash_idle: %8x\r\n", r->sleep_en_need_flash_idle);
	SOC_LOGD("	sleep_en_need_cpu1_wfi: %8x\r\n", r->sleep_en_need_cpu1_wfi);
	SOC_LOGD("	sleep_en_need_cpu0_wfi: %8x\r\n", r->sleep_en_need_cpu0_wfi);
	SOC_LOGD("	sleep_en_global: %8x\r\n", r->sleep_en_global);
	SOC_LOGD("	sleep_bus_idle_bypass: %8x\r\n", r->sleep_bus_idle_bypass);
	SOC_LOGD("	sleep_en_need_cpu2_wfi: %8x\r\n", r->sleep_en_need_cpu2_wfi);
	SOC_LOGD("	bts_soft_wakeup_req: %8x\r\n", r->bts_soft_wakeup_req);
	SOC_LOGD("	rom_rd_disable: %8x\r\n", r->rom_rd_disable);
	SOC_LOGD("	otp_rd_disable: %8x\r\n", r->otp_rd_disable);
	SOC_LOGD("	tcm0_pgen: %8x\r\n", r->tcm0_pgen);
	SOC_LOGD("	cpu0_subpwdm_en: %8x\r\n", r->cpu0_subpwdm_en);
	SOC_LOGD("	cpu2_ticktimer_32k_enable: %8x\r\n", r->cpu2_ticktimer_32k_enable);
	SOC_LOGD("	share_mem_clkgating_disable: %8x\r\n", r->share_mem_clkgating_disable);
	SOC_LOGD("	cpu0_ticktimer_32k_enable: %8x\r\n", r->cpu0_ticktimer_32k_enable);
	SOC_LOGD("	cpu1_ticktimer_32k_enable: %8x\r\n", r->cpu1_ticktimer_32k_enable);
	SOC_LOGD("	busmatrix_busy: %8x\r\n", r->busmatrix_busy);
}

static void sys_dump_cpu0_lv_sleep_cfg(void)
{
	sys_cpu0_lv_sleep_cfg_t *r = (sys_cpu0_lv_sleep_cfg_t *)(SOC_SYS_REG_BASE + (0x11 << 2));

	SOC_LOGD("cpu0_lv_sleep_cfg: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x11 << 2)));
	SOC_LOGD("	cpu0_cache_ret_en: %8x\r\n", r->cpu0_cache_ret_en);
	SOC_LOGD("	cpu0_cache_sleeppwd_en: %8x\r\n", r->cpu0_cache_sleeppwd_en);
	SOC_LOGD("	cpu0_fpu_sleeppwd_en: %8x\r\n", r->cpu0_fpu_sleeppwd_en);
	SOC_LOGD("	sys2flsh_2wire: %8x\r\n", r->sys2flsh_2wire);
}

static void sys_dump_cpu_device_mem_ctrl3(void)
{
	sys_cpu_device_mem_ctrl3_t *r = (sys_cpu_device_mem_ctrl3_t *)(SOC_SYS_REG_BASE + (0x12 << 2));

	SOC_LOGD("cpu_device_mem_ctrl3: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x12 << 2)));
	SOC_LOGD("	ram0_mem_ds_deep_sleep: %8x\r\n", r->ram0_mem_ds_deep_sleep);
	SOC_LOGD("	ram1_deep_sleep: %8x\r\n", r->ram1_deep_sleep);
	SOC_LOGD("	ram2_deep_sleep: %8x\r\n", r->ram2_deep_sleep);
	SOC_LOGD("	ram3_deep_sleep: %8x\r\n", r->ram3_deep_sleep);
	SOC_LOGD("	ram4_deep_sleep: %8x\r\n", r->ram4_deep_sleep);
	SOC_LOGD("	cpu0_icache_itag_deep_sleep: %8x\r\n", r->cpu0_icache_itag_deep_sleep);
	SOC_LOGD("	cpu0_dcache_dtag_deep_sleep: %8x\r\n", r->cpu0_dcache_dtag_deep_sleep);
	SOC_LOGD("	cpu1_icache_itag_deep_sleep: %8x\r\n", r->cpu1_icache_itag_deep_sleep);
	SOC_LOGD("	cpu1_dcache_dtag_deep_sleep: %8x\r\n", r->cpu1_dcache_dtag_deep_sleep);
	SOC_LOGD("	cpu0_itcm_deep_sleep: %8x\r\n", r->cpu0_itcm_deep_sleep);
	SOC_LOGD("	cpu0_dtcm_deep_sleep: %8x\r\n", r->cpu0_dtcm_deep_sleep);
	SOC_LOGD("	cpu1_itcm_deep_sleep: %8x\r\n", r->cpu1_itcm_deep_sleep);
	SOC_LOGD("	cpu1_dtcm_deep_sleep: %8x\r\n", r->cpu1_dtcm_deep_sleep);
	SOC_LOGD("	rott_deep_sleep: %8x\r\n", r->rott_deep_sleep);
	SOC_LOGD("	scal0_deep_sleep: %8x\r\n", r->scal0_deep_sleep);
	SOC_LOGD("	scal1_deep_sleep: %8x\r\n", r->scal1_deep_sleep);
	SOC_LOGD("	ram0_mem_sd_shutdown: %8x\r\n", r->ram0_mem_sd_shutdown);
	SOC_LOGD("	ram1_shutdown: %8x\r\n", r->ram1_shutdown);
	SOC_LOGD("	ram2_shutdown: %8x\r\n", r->ram2_shutdown);
	SOC_LOGD("	ram3_shutdown: %8x\r\n", r->ram3_shutdown);
	SOC_LOGD("	ram4_shutdown: %8x\r\n", r->ram4_shutdown);
	SOC_LOGD("	cpu0_icache_itag_shutdown: %8x\r\n", r->cpu0_icache_itag_shutdown);
	SOC_LOGD("	cpu0_dcache_dtag_shutdown: %8x\r\n", r->cpu0_dcache_dtag_shutdown);
	SOC_LOGD("	cpu1_icache_itag_shutdown: %8x\r\n", r->cpu1_icache_itag_shutdown);
	SOC_LOGD("	cpu1_dcache_dtag_shutdown: %8x\r\n", r->cpu1_dcache_dtag_shutdown);
	SOC_LOGD("	cpu0_itcm_shutdown: %8x\r\n", r->cpu0_itcm_shutdown);
	SOC_LOGD("	cpu0_dtcm_shutdown: %8x\r\n", r->cpu0_dtcm_shutdown);
	SOC_LOGD("	cpu1_itcm_shutdown: %8x\r\n", r->cpu1_itcm_shutdown);
	SOC_LOGD("	cpu1_dtcm_shutdown: %8x\r\n", r->cpu1_dtcm_shutdown);
	SOC_LOGD("	rott_shutdown: %8x\r\n", r->rott_shutdown);
	SOC_LOGD("	scal0_shutdown: %8x\r\n", r->scal0_shutdown);
	SOC_LOGD("	scal1_shutdown: %8x\r\n", r->scal1_shutdown);
}

static void sys_dump_rsv_13_1f(void)
{
	for (uint32_t idx = 0; idx < 13; idx++) {
		SOC_LOGD("rsv_13_1f: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x13 + idx) << 2)));
	}
}

static void sys_dump_cpu0_int_0_31_en(void)
{
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t *)(SOC_SYS_REG_BASE + (0x20 << 2));

	SOC_LOGD("cpu0_int_0_31_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x20 << 2)));
	SOC_LOGD("	cpu0_dma0_nsec_intr_en: %8x\r\n", r->cpu0_dma0_nsec_intr_en);
	SOC_LOGD("	cpu0_encp_sec_intr_en: %8x\r\n", r->cpu0_encp_sec_intr_en);
	SOC_LOGD("	cpu0_encp_nsec_intr_en: %8x\r\n", r->cpu0_encp_nsec_intr_en);
	SOC_LOGD("	cpu0_timer0_int_en: %8x\r\n", r->cpu0_timer0_int_en);
	SOC_LOGD("	cpu0_uart_int_en: %8x\r\n", r->cpu0_uart_int_en);
	SOC_LOGD("	cpu0_pwm0_int_en: %8x\r\n", r->cpu0_pwm0_int_en);
	SOC_LOGD("	cpu0_i2c0_int_en: %8x\r\n", r->cpu0_i2c0_int_en);
	SOC_LOGD("	cpu0_spi0_int_en: %8x\r\n", r->cpu0_spi0_int_en);
	SOC_LOGD("	cpu0_sadc_int_en: %8x\r\n", r->cpu0_sadc_int_en);
	SOC_LOGD("	cpu0_irda_int_en: %8x\r\n", r->cpu0_irda_int_en);
	SOC_LOGD("	cpu0_sdio_int_en: %8x\r\n", r->cpu0_sdio_int_en);
	SOC_LOGD("	cpu0_gdma_int_en: %8x\r\n", r->cpu0_gdma_int_en);
	SOC_LOGD("	cpu0_la_int_en: %8x\r\n", r->cpu0_la_int_en);
	SOC_LOGD("	cpu0_timer1_int_en: %8x\r\n", r->cpu0_timer1_int_en);
	SOC_LOGD("	cpu0_i2c1_int_en: %8x\r\n", r->cpu0_i2c1_int_en);
	SOC_LOGD("	cpu0_uart1_int_en: %8x\r\n", r->cpu0_uart1_int_en);
	SOC_LOGD("	cpu0_uart2_int_en: %8x\r\n", r->cpu0_uart2_int_en);
	SOC_LOGD("	cpu0_spi1_int_en: %8x\r\n", r->cpu0_spi1_int_en);
	SOC_LOGD("	cpu0_can_int_en: %8x\r\n", r->cpu0_can_int_en);
	SOC_LOGD("	cpu0_usb_int_en: %8x\r\n", r->cpu0_usb_int_en);
	SOC_LOGD("	cpu0_qspi0_int_en: %8x\r\n", r->cpu0_qspi0_int_en);
	SOC_LOGD("	cpu0_fft_int_en: %8x\r\n", r->cpu0_fft_int_en);
	SOC_LOGD("	cpu0_sbc_int_en: %8x\r\n", r->cpu0_sbc_int_en);
	SOC_LOGD("	cpu0_aud_int_en: %8x\r\n", r->cpu0_aud_int_en);
	SOC_LOGD("	cpu0_i2s0_int_en: %8x\r\n", r->cpu0_i2s0_int_en);
	SOC_LOGD("	cpu0_jpegenc_int_en: %8x\r\n", r->cpu0_jpegenc_int_en);
	SOC_LOGD("	cpu0_jpegdec_int_en: %8x\r\n", r->cpu0_jpegdec_int_en);
	SOC_LOGD("	cpu0_lcd_display_int_en: %8x\r\n", r->cpu0_lcd_display_int_en);
	SOC_LOGD("	cpu0_dma2d_int_en: %8x\r\n", r->cpu0_dma2d_int_en);
	SOC_LOGD("	cpu0_wifi_int_phy_mpb_en: %8x\r\n", r->cpu0_wifi_int_phy_mpb_en);
	SOC_LOGD("	cpu0_wifi_int_phy_riu_en: %8x\r\n", r->cpu0_wifi_int_phy_riu_en);
	SOC_LOGD("	cpu0_wifi_mac_int_tx_rx_timer_en: %8x\r\n", r->cpu0_wifi_mac_int_tx_rx_timer_en);
}

static void sys_dump_cpu0_int_32_63_en(void)
{
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t *)(SOC_SYS_REG_BASE + (0x21 << 2));

	SOC_LOGD("cpu0_int_32_63_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x21 << 2)));
	SOC_LOGD("	cpu0_wifi_mac_int_tx_rx_misc_en: %8x\r\n", r->cpu0_wifi_mac_int_tx_rx_misc_en);
	SOC_LOGD("	cpu0_wifi_mac_int_rx_trigger_en: %8x\r\n", r->cpu0_wifi_mac_int_rx_trigger_en);
	SOC_LOGD("	cpu0_wifi_mac_int_tx_trigger_en: %8x\r\n", r->cpu0_wifi_mac_int_tx_trigger_en);
	SOC_LOGD("	cpu0_wifi_mac_int_prot_trigger_en: %8x\r\n", r->cpu0_wifi_mac_int_prot_trigger_en);
	SOC_LOGD("	cpu0_wifi_mac_int_gen_en: %8x\r\n", r->cpu0_wifi_mac_int_gen_en);
	SOC_LOGD("	cpu0_wifi_hsu_irq_en: %8x\r\n", r->cpu0_wifi_hsu_irq_en);
	SOC_LOGD("	cpu0_wifi_int_mac_wakeup_en: %8x\r\n", r->cpu0_wifi_int_mac_wakeup_en);
	SOC_LOGD("	cpu0_dm_irq_en: %8x\r\n", r->cpu0_dm_irq_en);
	SOC_LOGD("	cpu0_ble_irq_en: %8x\r\n", r->cpu0_ble_irq_en);
	SOC_LOGD("	cpu0_bt_irq_en: %8x\r\n", r->cpu0_bt_irq_en);
	SOC_LOGD("	cpu0_qspi1_int_en: %8x\r\n", r->cpu0_qspi1_int_en);
	SOC_LOGD("	cpu0_pwm1_int_en: %8x\r\n", r->cpu0_pwm1_int_en);
	SOC_LOGD("	cpu0_i2s1_int_en: %8x\r\n", r->cpu0_i2s1_int_en);
	SOC_LOGD("	cpu0_i2s2_int_en: %8x\r\n", r->cpu0_i2s2_int_en);
	SOC_LOGD("	cpu0_h264_int_en: %8x\r\n", r->cpu0_h264_int_en);
	SOC_LOGD("	cpu0_sdmadc_int_en: %8x\r\n", r->cpu0_sdmadc_int_en);
	SOC_LOGD("	cpu0_mbox0_int_en: %8x\r\n", r->cpu0_mbox0_int_en);
	SOC_LOGD("	cpu0_mbox1_int_en: %8x\r\n", r->cpu0_mbox1_int_en);
	SOC_LOGD("	cpu0_bmc64_int_en: %8x\r\n", r->cpu0_bmc64_int_en);
	SOC_LOGD("	cpu0_dpll_unlock_int_en: %8x\r\n", r->cpu0_dpll_unlock_int_en);
	SOC_LOGD("	cpu0_touched_int_en: %8x\r\n", r->cpu0_touched_int_en);
	SOC_LOGD("	cpu0_usbplug_int_en: %8x\r\n", r->cpu0_usbplug_int_en);
	SOC_LOGD("	cpu0_rtc_int_en: %8x\r\n", r->cpu0_rtc_int_en);
	SOC_LOGD("	cpu0_gpio_int_en: %8x\r\n", r->cpu0_gpio_int_en);
	SOC_LOGD("	cpu0_dma1_sec_int_en: %8x\r\n", r->cpu0_dma1_sec_int_en);
	SOC_LOGD("	cpu0_dma1_nsec_int_en: %8x\r\n", r->cpu0_dma1_nsec_int_en);
	SOC_LOGD("	cpu0_yuvb_int_en: %8x\r\n", r->cpu0_yuvb_int_en);
	SOC_LOGD("	cpu0_rott_int_en: %8x\r\n", r->cpu0_rott_int_en);
	SOC_LOGD("	cpu0_7816_int_en: %8x\r\n", r->cpu0_7816_int_en);
	SOC_LOGD("	cpu0_lin_int_en: %8x\r\n", r->cpu0_lin_int_en);
	SOC_LOGD("	cpu0_scal1_int_en: %8x\r\n", r->cpu0_scal1_int_en);
	SOC_LOGD("	cpu0_mailbox_int_en: %8x\r\n", r->cpu0_mailbox_int_en);
}

static void sys_dump_cpu1_int_0_31_en(void)
{
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t *)(SOC_SYS_REG_BASE + (0x22 << 2));

	SOC_LOGD("cpu1_int_0_31_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x22 << 2)));
	SOC_LOGD("	cpu1_dma0_nsec_intr_en: %8x\r\n", r->cpu1_dma0_nsec_intr_en);
	SOC_LOGD("	cpu1_encp_sec_intr_en: %8x\r\n", r->cpu1_encp_sec_intr_en);
	SOC_LOGD("	cpu1_encp_nsec_intr_en: %8x\r\n", r->cpu1_encp_nsec_intr_en);
	SOC_LOGD("	cpu1_timer0_int_en: %8x\r\n", r->cpu1_timer0_int_en);
	SOC_LOGD("	cpu1_uart_int_en: %8x\r\n", r->cpu1_uart_int_en);
	SOC_LOGD("	cpu1_pwm0_int_en: %8x\r\n", r->cpu1_pwm0_int_en);
	SOC_LOGD("	cpu1_i2c0_int_en: %8x\r\n", r->cpu1_i2c0_int_en);
	SOC_LOGD("	cpu1_spi0_int_en: %8x\r\n", r->cpu1_spi0_int_en);
	SOC_LOGD("	cpu1_sadc_int_en: %8x\r\n", r->cpu1_sadc_int_en);
	SOC_LOGD("	cpu1_irda_int_en: %8x\r\n", r->cpu1_irda_int_en);
	SOC_LOGD("	cpu1_sdio_int_en: %8x\r\n", r->cpu1_sdio_int_en);
	SOC_LOGD("	cpu1_gdma_int_en: %8x\r\n", r->cpu1_gdma_int_en);
	SOC_LOGD("	cpu1_la_int_en: %8x\r\n", r->cpu1_la_int_en);
	SOC_LOGD("	cpu1_timer1_int_en: %8x\r\n", r->cpu1_timer1_int_en);
	SOC_LOGD("	cpu1_i2c1_int_en: %8x\r\n", r->cpu1_i2c1_int_en);
	SOC_LOGD("	cpu1_uart1_int_en: %8x\r\n", r->cpu1_uart1_int_en);
	SOC_LOGD("	cpu1_uart2_int_en: %8x\r\n", r->cpu1_uart2_int_en);
	SOC_LOGD("	cpu1_spi1_int_en: %8x\r\n", r->cpu1_spi1_int_en);
	SOC_LOGD("	cpu1_can_int_en: %8x\r\n", r->cpu1_can_int_en);
	SOC_LOGD("	cpu1_usb_int_en: %8x\r\n", r->cpu1_usb_int_en);
	SOC_LOGD("	cpu1_qspi0_int_en: %8x\r\n", r->cpu1_qspi0_int_en);
	SOC_LOGD("	cpu1_fft_int_en: %8x\r\n", r->cpu1_fft_int_en);
	SOC_LOGD("	cpu1_sbc_int_en: %8x\r\n", r->cpu1_sbc_int_en);
	SOC_LOGD("	cpu1_aud_int_en: %8x\r\n", r->cpu1_aud_int_en);
	SOC_LOGD("	cpu1_i2s0_int_en: %8x\r\n", r->cpu1_i2s0_int_en);
	SOC_LOGD("	cpu1_jpegenc_int_en: %8x\r\n", r->cpu1_jpegenc_int_en);
	SOC_LOGD("	cpu1_jpegdec_int_en: %8x\r\n", r->cpu1_jpegdec_int_en);
	SOC_LOGD("	cpu1_lcd_display_int_en: %8x\r\n", r->cpu1_lcd_display_int_en);
	SOC_LOGD("	cpu1_dma2d_int_en: %8x\r\n", r->cpu1_dma2d_int_en);
	SOC_LOGD("	cpu1_wifi_int_phy_mpb_en: %8x\r\n", r->cpu1_wifi_int_phy_mpb_en);
	SOC_LOGD("	cpu1_wifi_int_phy_riu_en: %8x\r\n", r->cpu1_wifi_int_phy_riu_en);
	SOC_LOGD("	cpu1_wifi_mac_int_tx_rx_timer_en: %8x\r\n", r->cpu1_wifi_mac_int_tx_rx_timer_en);
}

static void sys_dump_cpu1_int_32_63_en(void)
{
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t *)(SOC_SYS_REG_BASE + (0x23 << 2));

	SOC_LOGD("cpu1_int_32_63_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x23 << 2)));
	SOC_LOGD("	cpu1_wifi_mac_int_tx_rx_misc_en: %8x\r\n", r->cpu1_wifi_mac_int_tx_rx_misc_en);
	SOC_LOGD("	cpu1_wifi_mac_int_rx_trigger_en: %8x\r\n", r->cpu1_wifi_mac_int_rx_trigger_en);
	SOC_LOGD("	cpu1_wifi_mac_int_tx_trigger_en: %8x\r\n", r->cpu1_wifi_mac_int_tx_trigger_en);
	SOC_LOGD("	cpu1_wifi_mac_int_prot_trigger_en: %8x\r\n", r->cpu1_wifi_mac_int_prot_trigger_en);
	SOC_LOGD("	cpu1_wifi_mac_int_gen_en: %8x\r\n", r->cpu1_wifi_mac_int_gen_en);
	SOC_LOGD("	cpu1_wifi_hsu_irq_en: %8x\r\n", r->cpu1_wifi_hsu_irq_en);
	SOC_LOGD("	cpu1_wifi_int_mac_wakeup_en: %8x\r\n", r->cpu1_wifi_int_mac_wakeup_en);
	SOC_LOGD("	cpu1_dm_irq_en: %8x\r\n", r->cpu1_dm_irq_en);
	SOC_LOGD("	cpu1_ble_irq_en: %8x\r\n", r->cpu1_ble_irq_en);
	SOC_LOGD("	cpu1_bt_irq_en: %8x\r\n", r->cpu1_bt_irq_en);
	SOC_LOGD("	cpu1_qspi1_int_en: %8x\r\n", r->cpu1_qspi1_int_en);
	SOC_LOGD("	cpu1_pwm1_int_en: %8x\r\n", r->cpu1_pwm1_int_en);
	SOC_LOGD("	cpu1_i2s1_int_en: %8x\r\n", r->cpu1_i2s1_int_en);
	SOC_LOGD("	cpu1_i2s2_int_en: %8x\r\n", r->cpu1_i2s2_int_en);
	SOC_LOGD("	cpu1_h264_int_en: %8x\r\n", r->cpu1_h264_int_en);
	SOC_LOGD("	cpu1_sdmadc_int_en: %8x\r\n", r->cpu1_sdmadc_int_en);
	SOC_LOGD("	cpu1_eth_int_en: %8x\r\n", r->cpu1_eth_int_en);
	SOC_LOGD("	cpu1_mbox1_int_en: %8x\r\n", r->cpu1_mbox1_int_en);
	SOC_LOGD("	cpu1_bmc64_int_en: %8x\r\n", r->cpu1_bmc64_int_en);
	SOC_LOGD("	cpu1_dpll_unlock_int_en: %8x\r\n", r->cpu1_dpll_unlock_int_en);
	SOC_LOGD("	cpu1_touched_int_en: %8x\r\n", r->cpu1_touched_int_en);
	SOC_LOGD("	cpu1_usbplug_int_en: %8x\r\n", r->cpu1_usbplug_int_en);
	SOC_LOGD("	cpu1_rtc_int_en: %8x\r\n", r->cpu1_rtc_int_en);
	SOC_LOGD("	cpu1_gpio_int_en: %8x\r\n", r->cpu1_gpio_int_en);
	SOC_LOGD("	cpu1_dma1_sec_int_en: %8x\r\n", r->cpu1_dma1_sec_int_en);
	SOC_LOGD("	cpu1_dma1_nsec_int_en: %8x\r\n", r->cpu1_dma1_nsec_int_en);
	SOC_LOGD("	cpu1_yuvb_int_en: %8x\r\n", r->cpu1_yuvb_int_en);
	SOC_LOGD("	cpu1_rott_int_en: %8x\r\n", r->cpu1_rott_int_en);
	SOC_LOGD("	cpu1_7816_int_en: %8x\r\n", r->cpu1_7816_int_en);
	SOC_LOGD("	cpu1_lin_int_en: %8x\r\n", r->cpu1_lin_int_en);
	SOC_LOGD("	cpu1_scal1_int_en: %8x\r\n", r->cpu1_scal1_int_en);
	SOC_LOGD("	cpu1_mailbox_int_en: %8x\r\n", r->cpu1_mailbox_int_en);
}

static void sys_dump_cpu2_int_0_31_en(void)
{
	sys_cpu2_int_0_31_en_t *r = (sys_cpu2_int_0_31_en_t *)(SOC_SYS_REG_BASE + (0x24 << 2));

	SOC_LOGD("cpu2_int_0_31_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x24 << 2)));
	SOC_LOGD("	cpu2_dma0_nsec_intr_en: %8x\r\n", r->cpu2_dma0_nsec_intr_en);
	SOC_LOGD("	cpu2_encp_sec_intr_en: %8x\r\n", r->cpu2_encp_sec_intr_en);
	SOC_LOGD("	cpu2_encp_nsec_intr_en: %8x\r\n", r->cpu2_encp_nsec_intr_en);
	SOC_LOGD("	cpu2_timer0_int_en: %8x\r\n", r->cpu2_timer0_int_en);
	SOC_LOGD("	cpu2_uart_int_en: %8x\r\n", r->cpu2_uart_int_en);
	SOC_LOGD("	cpu2_pwm0_int_en: %8x\r\n", r->cpu2_pwm0_int_en);
	SOC_LOGD("	cpu2_i2c0_int_en: %8x\r\n", r->cpu2_i2c0_int_en);
	SOC_LOGD("	cpu2_spi0_int_en: %8x\r\n", r->cpu2_spi0_int_en);
	SOC_LOGD("	cpu2_sadc_int_en: %8x\r\n", r->cpu2_sadc_int_en);
	SOC_LOGD("	cpu2_irda_int_en: %8x\r\n", r->cpu2_irda_int_en);
	SOC_LOGD("	cpu2_sdio_int_en: %8x\r\n", r->cpu2_sdio_int_en);
	SOC_LOGD("	cpu2_gdma_int_en: %8x\r\n", r->cpu2_gdma_int_en);
	SOC_LOGD("	cpu2_la_int_en: %8x\r\n", r->cpu2_la_int_en);
	SOC_LOGD("	cpu2_timer1_int_en: %8x\r\n", r->cpu2_timer1_int_en);
	SOC_LOGD("	cpu2_i2c1_int_en: %8x\r\n", r->cpu2_i2c1_int_en);
	SOC_LOGD("	cpu2_uart1_int_en: %8x\r\n", r->cpu2_uart1_int_en);
	SOC_LOGD("	cpu2_uart2_int_en: %8x\r\n", r->cpu2_uart2_int_en);
	SOC_LOGD("	cpu2_spi1_int_en: %8x\r\n", r->cpu2_spi1_int_en);
	SOC_LOGD("	cpu2_can_int_en: %8x\r\n", r->cpu2_can_int_en);
	SOC_LOGD("	cpu2_usb_int_en: %8x\r\n", r->cpu2_usb_int_en);
	SOC_LOGD("	cpu2_qspi0_int_en: %8x\r\n", r->cpu2_qspi0_int_en);
	SOC_LOGD("	cpu2_fft_int_en: %8x\r\n", r->cpu2_fft_int_en);
	SOC_LOGD("	cpu2_sbc_int_en: %8x\r\n", r->cpu2_sbc_int_en);
	SOC_LOGD("	cpu2_aud_int_en: %8x\r\n", r->cpu2_aud_int_en);
	SOC_LOGD("	cpu2_i2s0_int_en: %8x\r\n", r->cpu2_i2s0_int_en);
	SOC_LOGD("	cpu2_jpegenc_int_en: %8x\r\n", r->cpu2_jpegenc_int_en);
	SOC_LOGD("	cpu2_jpegdec_int_en: %8x\r\n", r->cpu2_jpegdec_int_en);
	SOC_LOGD("	cpu2_lcd_display_int_en: %8x\r\n", r->cpu2_lcd_display_int_en);
	SOC_LOGD("	cpu2_dma2d_int_en: %8x\r\n", r->cpu2_dma2d_int_en);
	SOC_LOGD("	cpu2_wifi_int_phy_mpb_en: %8x\r\n", r->cpu2_wifi_int_phy_mpb_en);
	SOC_LOGD("	cpu2_wifi_int_phy_riu_en: %8x\r\n", r->cpu2_wifi_int_phy_riu_en);
	SOC_LOGD("	cpu2_wifi_mac_int_tx_rx_timer_en: %8x\r\n", r->cpu2_wifi_mac_int_tx_rx_timer_en);
}

static void sys_dump_cpu2_int_32_63_en(void)
{
	sys_cpu2_int_32_63_en_t *r = (sys_cpu2_int_32_63_en_t *)(SOC_SYS_REG_BASE + (0x25 << 2));

	SOC_LOGD("cpu2_int_32_63_en: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x25 << 2)));
	SOC_LOGD("	cpu2_wifi_mac_int_tx_rx_misc_en: %8x\r\n", r->cpu2_wifi_mac_int_tx_rx_misc_en);
	SOC_LOGD("	cpu2_wifi_mac_int_rx_trigger_en: %8x\r\n", r->cpu2_wifi_mac_int_rx_trigger_en);
	SOC_LOGD("	cpu2_wifi_mac_int_tx_trigger_en: %8x\r\n", r->cpu2_wifi_mac_int_tx_trigger_en);
	SOC_LOGD("	cpu2_wifi_mac_int_prot_trigger_en: %8x\r\n", r->cpu2_wifi_mac_int_prot_trigger_en);
	SOC_LOGD("	cpu2_wifi_mac_int_gen_en: %8x\r\n", r->cpu2_wifi_mac_int_gen_en);
	SOC_LOGD("	cpu2_wifi_hsu_irq_en: %8x\r\n", r->cpu2_wifi_hsu_irq_en);
	SOC_LOGD("	cpu2_wifi_int_mac_wakeup_en: %8x\r\n", r->cpu2_wifi_int_mac_wakeup_en);
	SOC_LOGD("	cpu2_dm_irq_en: %8x\r\n", r->cpu2_dm_irq_en);
	SOC_LOGD("	cpu2_ble_irq_en: %8x\r\n", r->cpu2_ble_irq_en);
	SOC_LOGD("	cpu2_bt_irq_en: %8x\r\n", r->cpu2_bt_irq_en);
	SOC_LOGD("	cpu2_qspi1_int_en: %8x\r\n", r->cpu2_qspi1_int_en);
	SOC_LOGD("	cpu2_pwm1_int_en: %8x\r\n", r->cpu2_pwm1_int_en);
	SOC_LOGD("	cpu2_i2s1_int_en: %8x\r\n", r->cpu2_i2s1_int_en);
	SOC_LOGD("	cpu2_i2s2_int_en: %8x\r\n", r->cpu2_i2s2_int_en);
	SOC_LOGD("	cpu2_h264_int_en: %8x\r\n", r->cpu2_h264_int_en);
	SOC_LOGD("	cpu2_sdmadc_int_en: %8x\r\n", r->cpu2_sdmadc_int_en);
	SOC_LOGD("	cpu2_mbox0_int_en: %8x\r\n", r->cpu2_mbox0_int_en);
	SOC_LOGD("	cpu2_mbox1_int_en: %8x\r\n", r->cpu2_mbox1_int_en);
	SOC_LOGD("	cpu2_bmc64_int_en: %8x\r\n", r->cpu2_bmc64_int_en);
	SOC_LOGD("	cpu2_dpll_unlock_int_en: %8x\r\n", r->cpu2_dpll_unlock_int_en);
	SOC_LOGD("	cpu2_touched_int_en: %8x\r\n", r->cpu2_touched_int_en);
	SOC_LOGD("	cpu2_usbplug_int_en: %8x\r\n", r->cpu2_usbplug_int_en);
	SOC_LOGD("	cpu2_rtc_int_en: %8x\r\n", r->cpu2_rtc_int_en);
	SOC_LOGD("	cpu2_gpio_int_en: %8x\r\n", r->cpu2_gpio_int_en);
	SOC_LOGD("	cpu2_dma1_sec_int_en: %8x\r\n", r->cpu2_dma1_sec_int_en);
	SOC_LOGD("	cpu2_dma1_nsec_int_en: %8x\r\n", r->cpu2_dma1_nsec_int_en);
	SOC_LOGD("	cpu2_yuvb_int_en: %8x\r\n", r->cpu2_yuvb_int_en);
	SOC_LOGD("	cpu2_rott_int_en: %8x\r\n", r->cpu2_rott_int_en);
	SOC_LOGD("	cpu2_7816_int_en: %8x\r\n", r->cpu2_7816_int_en);
	SOC_LOGD("	cpu2_lin_int_en: %8x\r\n", r->cpu2_lin_int_en);
	SOC_LOGD("	cpu2_scal1_int_en: %8x\r\n", r->cpu2_scal1_int_en);
	SOC_LOGD("	cpu2_mailbox_int_en: %8x\r\n", r->cpu2_mailbox_int_en);
}

static void sys_dump_rsv_26_27(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGD("rsv_26_27: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x26 + idx) << 2)));
	}
}

static void sys_dump_cpu0_int_0_31_status(void)
{
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t *)(SOC_SYS_REG_BASE + (0x28 << 2));

	SOC_LOGD("cpu0_int_0_31_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x28 << 2)));
	SOC_LOGD("	cpu0_dma0_nsec_intr_st: %8x\r\n", r->cpu0_dma0_nsec_intr_st);
	SOC_LOGD("	cpu0_encp_sec_intr_st: %8x\r\n", r->cpu0_encp_sec_intr_st);
	SOC_LOGD("	cpu0_encp_nsec_intr_st: %8x\r\n", r->cpu0_encp_nsec_intr_st);
	SOC_LOGD("	cpu0_timer0_int_st: %8x\r\n", r->cpu0_timer0_int_st);
	SOC_LOGD("	cpu0_uart_int_st: %8x\r\n", r->cpu0_uart_int_st);
	SOC_LOGD("	cpu0_pwm0_int_st: %8x\r\n", r->cpu0_pwm0_int_st);
	SOC_LOGD("	cpu0_i2c0_int_st: %8x\r\n", r->cpu0_i2c0_int_st);
	SOC_LOGD("	cpu0_spi0_int_st: %8x\r\n", r->cpu0_spi0_int_st);
	SOC_LOGD("	cpu0_sadc_int_st: %8x\r\n", r->cpu0_sadc_int_st);
	SOC_LOGD("	cpu0_irda_int_st: %8x\r\n", r->cpu0_irda_int_st);
	SOC_LOGD("	cpu0_sdio_int_st: %8x\r\n", r->cpu0_sdio_int_st);
	SOC_LOGD("	cpu0_gdma_int_st: %8x\r\n", r->cpu0_gdma_int_st);
	SOC_LOGD("	cpu0_la_int_st: %8x\r\n", r->cpu0_la_int_st);
	SOC_LOGD("	cpu0_timer1_int_st: %8x\r\n", r->cpu0_timer1_int_st);
	SOC_LOGD("	cpu0_i2c1_int_st: %8x\r\n", r->cpu0_i2c1_int_st);
	SOC_LOGD("	cpu0_uart1_int_st: %8x\r\n", r->cpu0_uart1_int_st);
	SOC_LOGD("	cpu0_uart2_int_st: %8x\r\n", r->cpu0_uart2_int_st);
	SOC_LOGD("	cpu0_spi1_int_st: %8x\r\n", r->cpu0_spi1_int_st);
	SOC_LOGD("	cpu0_can_int_st: %8x\r\n", r->cpu0_can_int_st);
	SOC_LOGD("	cpu0_usb_int_st: %8x\r\n", r->cpu0_usb_int_st);
	SOC_LOGD("	cpu0_qspi0_int_st: %8x\r\n", r->cpu0_qspi0_int_st);
	SOC_LOGD("	cpu0_fft_int_st: %8x\r\n", r->cpu0_fft_int_st);
	SOC_LOGD("	cpu0_sbc_int_st: %8x\r\n", r->cpu0_sbc_int_st);
	SOC_LOGD("	cpu0_aud_int_st: %8x\r\n", r->cpu0_aud_int_st);
	SOC_LOGD("	cpu0_i2s0_int_st: %8x\r\n", r->cpu0_i2s0_int_st);
	SOC_LOGD("	cpu0_jpegenc_int_st: %8x\r\n", r->cpu0_jpegenc_int_st);
	SOC_LOGD("	cpu0_jpegdec_int_st: %8x\r\n", r->cpu0_jpegdec_int_st);
	SOC_LOGD("	cpu0_lcd_display_int_st: %8x\r\n", r->cpu0_lcd_display_int_st);
	SOC_LOGD("	cpu0_dma2d_int_st: %8x\r\n", r->cpu0_dma2d_int_st);
	SOC_LOGD("	cpu0_wifi_int_phy_mpb_st: %8x\r\n", r->cpu0_wifi_int_phy_mpb_st);
	SOC_LOGD("	cpu0_wifi_int_phy_riu_st: %8x\r\n", r->cpu0_wifi_int_phy_riu_st);
	SOC_LOGD("	cpu0_wifi_mac_int_tx_rx_timer_st: %8x\r\n", r->cpu0_wifi_mac_int_tx_rx_timer_st);
}

static void sys_dump_cpu0_int_32_63_status(void)
{
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t *)(SOC_SYS_REG_BASE + (0x29 << 2));

	SOC_LOGD("cpu0_int_32_63_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x29 << 2)));
	SOC_LOGD("	cpu0_wifi_mac_int_tx_rx_misc_st: %8x\r\n", r->cpu0_wifi_mac_int_tx_rx_misc_st);
	SOC_LOGD("	cpu0_wifi_mac_int_rx_trigger_st: %8x\r\n", r->cpu0_wifi_mac_int_rx_trigger_st);
	SOC_LOGD("	cpu0_wifi_mac_int_tx_trigger_st: %8x\r\n", r->cpu0_wifi_mac_int_tx_trigger_st);
	SOC_LOGD("	cpu0_wifi_mac_int_prot_trigger_st: %8x\r\n", r->cpu0_wifi_mac_int_prot_trigger_st);
	SOC_LOGD("	cpu0_wifi_mac_int_gen_st: %8x\r\n", r->cpu0_wifi_mac_int_gen_st);
	SOC_LOGD("	cpu0_wifi_hsu_irq_st: %8x\r\n", r->cpu0_wifi_hsu_irq_st);
	SOC_LOGD("	cpu0_wifi_int_mac_wakeup_st: %8x\r\n", r->cpu0_wifi_int_mac_wakeup_st);
	SOC_LOGD("	cpu0_dm_irq_st: %8x\r\n", r->cpu0_dm_irq_st);
	SOC_LOGD("	cpu0_ble_irq_st: %8x\r\n", r->cpu0_ble_irq_st);
	SOC_LOGD("	cpu0_bt_irq_st: %8x\r\n", r->cpu0_bt_irq_st);
	SOC_LOGD("	cpu0_qspi1_int_st: %8x\r\n", r->cpu0_qspi1_int_st);
	SOC_LOGD("	cpu0_pwm1_int_st: %8x\r\n", r->cpu0_pwm1_int_st);
	SOC_LOGD("	cpu0_i2s1_int_st: %8x\r\n", r->cpu0_i2s1_int_st);
	SOC_LOGD("	cpu0_i2s2_int_st: %8x\r\n", r->cpu0_i2s2_int_st);
	SOC_LOGD("	cpu0_h264_int_st: %8x\r\n", r->cpu0_h264_int_st);
	SOC_LOGD("	cpu0_sdmadc_int_st: %8x\r\n", r->cpu0_sdmadc_int_st);
	SOC_LOGD("	cpu0_mbox0_int_st: %8x\r\n", r->cpu0_mbox0_int_st);
	SOC_LOGD("	cpu0_mbox1_int_st: %8x\r\n", r->cpu0_mbox1_int_st);
	SOC_LOGD("	cpu0_bmc64_int_st: %8x\r\n", r->cpu0_bmc64_int_st);
	SOC_LOGD("	cpu0_dpll_unlock_int_st: %8x\r\n", r->cpu0_dpll_unlock_int_st);
	SOC_LOGD("	cpu0_touched_int_st: %8x\r\n", r->cpu0_touched_int_st);
	SOC_LOGD("	cpu0_usbplug_int_st: %8x\r\n", r->cpu0_usbplug_int_st);
	SOC_LOGD("	cpu0_rtc_int_st: %8x\r\n", r->cpu0_rtc_int_st);
	SOC_LOGD("	cpu0_gpio_int_st: %8x\r\n", r->cpu0_gpio_int_st);
	SOC_LOGD("	cpu0_dma1_sec_int_st: %8x\r\n", r->cpu0_dma1_sec_int_st);
	SOC_LOGD("	cpu0_dma1_nsec_int_st: %8x\r\n", r->cpu0_dma1_nsec_int_st);
	SOC_LOGD("	cpu0_yuvb_int_st: %8x\r\n", r->cpu0_yuvb_int_st);
	SOC_LOGD("	reserved0: %8x\r\n", r->reserved0);
}

static void sys_dump_cpu1_int_0_31_status(void)
{
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t *)(SOC_SYS_REG_BASE + (0x2a << 2));

	SOC_LOGD("cpu1_int_0_31_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x2a << 2)));
	SOC_LOGD("	cpu1_dma0_nsec_intr_st: %8x\r\n", r->cpu1_dma0_nsec_intr_st);
	SOC_LOGD("	cpu1_encp_sec_intr_st: %8x\r\n", r->cpu1_encp_sec_intr_st);
	SOC_LOGD("	cpu1_encp_nsec_intr_st: %8x\r\n", r->cpu1_encp_nsec_intr_st);
	SOC_LOGD("	cpu1_timer0_int_st: %8x\r\n", r->cpu1_timer0_int_st);
	SOC_LOGD("	cpu1_uart_int_st: %8x\r\n", r->cpu1_uart_int_st);
	SOC_LOGD("	cpu1_pwm0_int_st: %8x\r\n", r->cpu1_pwm0_int_st);
	SOC_LOGD("	cpu1_i2c0_int_st: %8x\r\n", r->cpu1_i2c0_int_st);
	SOC_LOGD("	cpu1_spi0_int_st: %8x\r\n", r->cpu1_spi0_int_st);
	SOC_LOGD("	cpu1_sadc_int_st: %8x\r\n", r->cpu1_sadc_int_st);
	SOC_LOGD("	cpu1_irda_int_st: %8x\r\n", r->cpu1_irda_int_st);
	SOC_LOGD("	cpu1_sdio_int_st: %8x\r\n", r->cpu1_sdio_int_st);
	SOC_LOGD("	cpu1_gdma_int_st: %8x\r\n", r->cpu1_gdma_int_st);
	SOC_LOGD("	cpu1_la_int_st: %8x\r\n", r->cpu1_la_int_st);
	SOC_LOGD("	cpu1_timer1_int_st: %8x\r\n", r->cpu1_timer1_int_st);
	SOC_LOGD("	cpu1_i2c1_int_st: %8x\r\n", r->cpu1_i2c1_int_st);
	SOC_LOGD("	cpu1_uart1_int_st: %8x\r\n", r->cpu1_uart1_int_st);
	SOC_LOGD("	cpu1_uart2_int_st: %8x\r\n", r->cpu1_uart2_int_st);
	SOC_LOGD("	cpu1_spi1_int_st: %8x\r\n", r->cpu1_spi1_int_st);
	SOC_LOGD("	cpu1_can_int_st: %8x\r\n", r->cpu1_can_int_st);
	SOC_LOGD("	cpu1_usb_int_st: %8x\r\n", r->cpu1_usb_int_st);
	SOC_LOGD("	cpu1_qspi0_int_st: %8x\r\n", r->cpu1_qspi0_int_st);
	SOC_LOGD("	cpu1_fft_int_st: %8x\r\n", r->cpu1_fft_int_st);
	SOC_LOGD("	cpu1_sbc_int_st: %8x\r\n", r->cpu1_sbc_int_st);
	SOC_LOGD("	cpu1_aud_int_st: %8x\r\n", r->cpu1_aud_int_st);
	SOC_LOGD("	cpu1_i2s0_int_st: %8x\r\n", r->cpu1_i2s0_int_st);
	SOC_LOGD("	cpu1_jpegenc_int_st: %8x\r\n", r->cpu1_jpegenc_int_st);
	SOC_LOGD("	cpu1_jpegdec_int_st: %8x\r\n", r->cpu1_jpegdec_int_st);
	SOC_LOGD("	cpu1_lcd_display_int_st: %8x\r\n", r->cpu1_lcd_display_int_st);
	SOC_LOGD("	cpu1_dma2d_int_st: %8x\r\n", r->cpu1_dma2d_int_st);
	SOC_LOGD("	cpu1_wifi_int_phy_mpb_st: %8x\r\n", r->cpu1_wifi_int_phy_mpb_st);
	SOC_LOGD("	cpu1_wifi_int_phy_riu_st: %8x\r\n", r->cpu1_wifi_int_phy_riu_st);
	SOC_LOGD("	cpu1_wifi_mac_int_tx_rx_timer_st: %8x\r\n", r->cpu1_wifi_mac_int_tx_rx_timer_st);
}

static void sys_dump_cpu1_int_32_63_status(void)
{
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t *)(SOC_SYS_REG_BASE + (0x2b << 2));

	SOC_LOGD("cpu1_int_32_63_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x2b << 2)));
	SOC_LOGD("	cpu1_wifi_mac_int_tx_rx_misc_st: %8x\r\n", r->cpu1_wifi_mac_int_tx_rx_misc_st);
	SOC_LOGD("	cpu1_wifi_mac_int_rx_trigger_st: %8x\r\n", r->cpu1_wifi_mac_int_rx_trigger_st);
	SOC_LOGD("	cpu1_wifi_mac_int_tx_trigger_st: %8x\r\n", r->cpu1_wifi_mac_int_tx_trigger_st);
	SOC_LOGD("	cpu1_wifi_mac_int_prot_trigger_st: %8x\r\n", r->cpu1_wifi_mac_int_prot_trigger_st);
	SOC_LOGD("	cpu1_wifi_mac_int_gen_st: %8x\r\n", r->cpu1_wifi_mac_int_gen_st);
	SOC_LOGD("	cpu1_wifi_hsu_irq_st: %8x\r\n", r->cpu1_wifi_hsu_irq_st);
	SOC_LOGD("	cpu1_wifi_int_mac_wakeup_st: %8x\r\n", r->cpu1_wifi_int_mac_wakeup_st);
	SOC_LOGD("	cpu1_dm_irq_st: %8x\r\n", r->cpu1_dm_irq_st);
	SOC_LOGD("	cpu1_ble_irq_st: %8x\r\n", r->cpu1_ble_irq_st);
	SOC_LOGD("	cpu1_bt_irq_st: %8x\r\n", r->cpu1_bt_irq_st);
	SOC_LOGD("	cpu1_qspi1_int_st: %8x\r\n", r->cpu1_qspi1_int_st);
	SOC_LOGD("	cpu1_pwm1_int_st: %8x\r\n", r->cpu1_pwm1_int_st);
	SOC_LOGD("	cpu1_i2s1_int_st: %8x\r\n", r->cpu1_i2s1_int_st);
	SOC_LOGD("	cpu1_i2s2_int_st: %8x\r\n", r->cpu1_i2s2_int_st);
	SOC_LOGD("	cpu1_h264_int_st: %8x\r\n", r->cpu1_h264_int_st);
	SOC_LOGD("	cpu1_sdmadc_int_st: %8x\r\n", r->cpu1_sdmadc_int_st);
	SOC_LOGD("	cpu1_mbox0_int_st: %8x\r\n", r->cpu1_mbox0_int_st);
	SOC_LOGD("	cpu1_mbox1_int_st: %8x\r\n", r->cpu1_mbox1_int_st);
	SOC_LOGD("	cpu1_bmc64_int_st: %8x\r\n", r->cpu1_bmc64_int_st);
	SOC_LOGD("	cpu1_dpll_unlock_int_st: %8x\r\n", r->cpu1_dpll_unlock_int_st);
	SOC_LOGD("	cpu1_touched_int_st: %8x\r\n", r->cpu1_touched_int_st);
	SOC_LOGD("	cpu1_usbplug_int_st: %8x\r\n", r->cpu1_usbplug_int_st);
	SOC_LOGD("	cpu1_rtc_int_st: %8x\r\n", r->cpu1_rtc_int_st);
	SOC_LOGD("	cpu1_gpio_int_st: %8x\r\n", r->cpu1_gpio_int_st);
	SOC_LOGD("	cpu1_dma1_sec_int_st: %8x\r\n", r->cpu1_dma1_sec_int_st);
	SOC_LOGD("	cpu1_dma1_nsec_int_st: %8x\r\n", r->cpu1_dma1_nsec_int_st);
	SOC_LOGD("	cpu1_yuvb_int_st: %8x\r\n", r->cpu1_yuvb_int_st);
	SOC_LOGD("	reserved0: %8x\r\n", r->reserved0);
}

static void sys_dump_cpu2_int_0_31_status(void)
{
	sys_cpu2_int_0_31_status_t *r = (sys_cpu2_int_0_31_status_t *)(SOC_SYS_REG_BASE + (0x2c << 2));

	SOC_LOGD("cpu2_int_0_31_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x2c << 2)));
	SOC_LOGD("	cpu2_dma0_nsec_intr_st: %8x\r\n", r->cpu2_dma0_nsec_intr_st);
	SOC_LOGD("	cpu2_encp_sec_intr_st: %8x\r\n", r->cpu2_encp_sec_intr_st);
	SOC_LOGD("	cpu2_encp_nsec_intr_st: %8x\r\n", r->cpu2_encp_nsec_intr_st);
	SOC_LOGD("	cpu2_timer0_int_st: %8x\r\n", r->cpu2_timer0_int_st);
	SOC_LOGD("	cpu2_uart_int_st: %8x\r\n", r->cpu2_uart_int_st);
	SOC_LOGD("	cpu2_pwm0_int_st: %8x\r\n", r->cpu2_pwm0_int_st);
	SOC_LOGD("	cpu2_i2c0_int_st: %8x\r\n", r->cpu2_i2c0_int_st);
	SOC_LOGD("	cpu2_spi0_int_st: %8x\r\n", r->cpu2_spi0_int_st);
	SOC_LOGD("	cpu2_sadc_int_st: %8x\r\n", r->cpu2_sadc_int_st);
	SOC_LOGD("	cpu2_irda_int_st: %8x\r\n", r->cpu2_irda_int_st);
	SOC_LOGD("	cpu2_sdio_int_st: %8x\r\n", r->cpu2_sdio_int_st);
	SOC_LOGD("	cpu2_gdma_int_st: %8x\r\n", r->cpu2_gdma_int_st);
	SOC_LOGD("	cpu2_la_int_st: %8x\r\n", r->cpu2_la_int_st);
	SOC_LOGD("	cpu2_timer1_int_st: %8x\r\n", r->cpu2_timer1_int_st);
	SOC_LOGD("	cpu2_i2c1_int_st: %8x\r\n", r->cpu2_i2c1_int_st);
	SOC_LOGD("	cpu2_uart1_int_st: %8x\r\n", r->cpu2_uart1_int_st);
	SOC_LOGD("	cpu2_uart2_int_st: %8x\r\n", r->cpu2_uart2_int_st);
	SOC_LOGD("	cpu2_spi1_int_st: %8x\r\n", r->cpu2_spi1_int_st);
	SOC_LOGD("	cpu2_can_int_st: %8x\r\n", r->cpu2_can_int_st);
	SOC_LOGD("	cpu2_usb_int_st: %8x\r\n", r->cpu2_usb_int_st);
	SOC_LOGD("	cpu2_qspi0_int_st: %8x\r\n", r->cpu2_qspi0_int_st);
	SOC_LOGD("	cpu2_fft_int_st: %8x\r\n", r->cpu2_fft_int_st);
	SOC_LOGD("	cpu2_sbc_int_st: %8x\r\n", r->cpu2_sbc_int_st);
	SOC_LOGD("	cpu2_aud_int_st: %8x\r\n", r->cpu2_aud_int_st);
	SOC_LOGD("	cpu2_i2s0_int_st: %8x\r\n", r->cpu2_i2s0_int_st);
	SOC_LOGD("	cpu2_jpegenc_int_st: %8x\r\n", r->cpu2_jpegenc_int_st);
	SOC_LOGD("	cpu2_jpegdec_int_st: %8x\r\n", r->cpu2_jpegdec_int_st);
	SOC_LOGD("	cpu2_lcd_display_int_st: %8x\r\n", r->cpu2_lcd_display_int_st);
	SOC_LOGD("	cpu2_dma2d_int_st: %8x\r\n", r->cpu2_dma2d_int_st);
	SOC_LOGD("	cpu2_wifi_int_phy_mpb_st: %8x\r\n", r->cpu2_wifi_int_phy_mpb_st);
	SOC_LOGD("	cpu2_wifi_int_phy_riu_st: %8x\r\n", r->cpu2_wifi_int_phy_riu_st);
	SOC_LOGD("	cpu2_wifi_mac_int_tx_rx_timer_st: %8x\r\n", r->cpu2_wifi_mac_int_tx_rx_timer_st);
}

static void sys_dump_cpu2_int_32_63_status(void)
{
	sys_cpu2_int_32_63_status_t *r = (sys_cpu2_int_32_63_status_t *)(SOC_SYS_REG_BASE + (0x2d << 2));

	SOC_LOGD("cpu2_int_32_63_status: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x2d << 2)));
	SOC_LOGD("	cpu2_wifi_mac_int_tx_rx_misc_st: %8x\r\n", r->cpu2_wifi_mac_int_tx_rx_misc_st);
	SOC_LOGD("	cpu2_wifi_mac_int_rx_trigger_st: %8x\r\n", r->cpu2_wifi_mac_int_rx_trigger_st);
	SOC_LOGD("	cpu2_wifi_mac_int_tx_trigger_st: %8x\r\n", r->cpu2_wifi_mac_int_tx_trigger_st);
	SOC_LOGD("	cpu2_wifi_mac_int_prot_trigger_st: %8x\r\n", r->cpu2_wifi_mac_int_prot_trigger_st);
	SOC_LOGD("	cpu2_wifi_mac_int_gen_st: %8x\r\n", r->cpu2_wifi_mac_int_gen_st);
	SOC_LOGD("	cpu2_wifi_hsu_irq_st: %8x\r\n", r->cpu2_wifi_hsu_irq_st);
	SOC_LOGD("	cpu2_wifi_int_mac_wakeup_st: %8x\r\n", r->cpu2_wifi_int_mac_wakeup_st);
	SOC_LOGD("	cpu2_dm_irq_st: %8x\r\n", r->cpu2_dm_irq_st);
	SOC_LOGD("	cpu2_ble_irq_st: %8x\r\n", r->cpu2_ble_irq_st);
	SOC_LOGD("	cpu2_bt_irq_st: %8x\r\n", r->cpu2_bt_irq_st);
	SOC_LOGD("	cpu2_qspi1_int_st: %8x\r\n", r->cpu2_qspi1_int_st);
	SOC_LOGD("	cpu2_pwm1_int_st: %8x\r\n", r->cpu2_pwm1_int_st);
	SOC_LOGD("	cpu2_i2s1_int_st: %8x\r\n", r->cpu2_i2s1_int_st);
	SOC_LOGD("	cpu2_i2s2_int_st: %8x\r\n", r->cpu2_i2s2_int_st);
	SOC_LOGD("	cpu2_h264_int_st: %8x\r\n", r->cpu2_h264_int_st);
	SOC_LOGD("	cpu2_sdmadc_int_st: %8x\r\n", r->cpu2_sdmadc_int_st);
	SOC_LOGD("	cpu2_mbox0_int_st: %8x\r\n", r->cpu2_mbox0_int_st);
	SOC_LOGD("	cpu2_mbox1_int_st: %8x\r\n", r->cpu2_mbox1_int_st);
	SOC_LOGD("	cpu2_bmc64_int_st: %8x\r\n", r->cpu2_bmc64_int_st);
	SOC_LOGD("	cpu2_dpll_unlock_int_st: %8x\r\n", r->cpu2_dpll_unlock_int_st);
	SOC_LOGD("	cpu2_touched_int_st: %8x\r\n", r->cpu2_touched_int_st);
	SOC_LOGD("	cpu2_usbplug_int_st: %8x\r\n", r->cpu2_usbplug_int_st);
	SOC_LOGD("	cpu2_rtc_int_st: %8x\r\n", r->cpu2_rtc_int_st);
	SOC_LOGD("	cpu2_gpio_int_st: %8x\r\n", r->cpu2_gpio_int_st);
	SOC_LOGD("	cpu2_dma1_sec_int_st: %8x\r\n", r->cpu2_dma1_sec_int_st);
	SOC_LOGD("	cpu2_dma1_nsec_int_st: %8x\r\n", r->cpu2_dma1_nsec_int_st);
	SOC_LOGD("	cpu2_yuvb_int_st: %8x\r\n", r->cpu2_yuvb_int_st);
	SOC_LOGD("	reserved0: %8x\r\n", r->reserved0);
}

static void sys_dump_rsv_2e_2f(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGD("rsv_2e_2f: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x2e + idx) << 2)));
	}
}

static void sys_dump_gpio_config0(void)
{
	SOC_LOGD("gpio_config0: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x30 << 2)));
}

static void sys_dump_gpio_config1(void)
{
	SOC_LOGD("gpio_config1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x31 << 2)));
}

static void sys_dump_gpio_config2(void)
{
	SOC_LOGD("gpio_config2: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x32 << 2)));
}

static void sys_dump_gpio_config3(void)
{
	SOC_LOGD("gpio_config3: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x33 << 2)));
}

static void sys_dump_gpio_config4(void)
{
	SOC_LOGD("gpio_config4: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x34 << 2)));
}

static void sys_dump_gpio_config5(void)
{
	SOC_LOGD("gpio_config5: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x35 << 2)));
}

static void sys_dump_gpio_config6(void)
{
	SOC_LOGD("gpio_config6: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x36 << 2)));
}

static void sys_dump_rsv_37_37(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGD("rsv_37_37: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x37 + idx) << 2)));
	}
}

static void sys_dump_sys_debug_config0(void)
{
	SOC_LOGD("sys_debug_config0: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x38 << 2)));
}

static void sys_dump_sys_debug_config1(void)
{
	SOC_LOGD("sys_debug_config1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x39 << 2)));
}

static void sys_dump_rsv_3a_3f(void)
{
	for (uint32_t idx = 0; idx < 6; idx++) {
		SOC_LOGD("rsv_3a_3f: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x3a + idx) << 2)));
	}
}

static void sys_dump_ana_reg0(void)
{
	sys_ana_reg0_t *r = (sys_ana_reg0_t *)(SOC_SYS_REG_BASE + (0x40 << 2));

	SOC_LOGD("ana_reg0: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x40 << 2)));
	SOC_LOGD("	dpll_tsten: %8x\r\n", r->dpll_tsten);
	SOC_LOGD("	cp: %8x\r\n", r->cp);
	SOC_LOGD("	spideten: %8x\r\n", r->spideten);
	SOC_LOGD("	hvref: %8x\r\n", r->hvref);
	SOC_LOGD("	lvref: %8x\r\n", r->lvref);
	SOC_LOGD("	rzctrl26m: %8x\r\n", r->rzctrl26m);
	SOC_LOGD("	looprzctrl: %8x\r\n", r->looprzctrl);
	SOC_LOGD("	rpc: %8x\r\n", r->rpc);
	SOC_LOGD("	openloop_en: %8x\r\n", r->openloop_en);
	SOC_LOGD("	cksel: %8x\r\n", r->cksel);
	SOC_LOGD("	spitrig: %8x\r\n", r->spitrig);
	SOC_LOGD("	band0: %8x\r\n", r->band0);
	SOC_LOGD("	band1: %8x\r\n", r->band1);
	SOC_LOGD("	band: %8x\r\n", r->band);
	SOC_LOGD("	bandmanual: %8x\r\n", r->bandmanual);
	SOC_LOGD("	dsptrig: %8x\r\n", r->dsptrig);
	SOC_LOGD("	lpen_dpll: %8x\r\n", r->lpen_dpll);
	SOC_LOGD("	nc_28_30: %8x\r\n", r->nc_28_30);
	SOC_LOGD("	vctrl_dpllldo: %8x\r\n", r->vctrl_dpllldo);
}

static void sys_dump_ana_reg1(void)
{
	sys_ana_reg1_t *r = (sys_ana_reg1_t *)(SOC_SYS_REG_BASE + (0x41 << 2));

	SOC_LOGD("ana_reg1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x41 << 2)));
	SOC_LOGD("	nc_0_0: %8x\r\n", r->nc_0_0);
	SOC_LOGD("	nc_1_1: %8x\r\n", r->nc_1_1);
	SOC_LOGD("	msw: %8x\r\n", r->msw);
	SOC_LOGD("	ictrl: %8x\r\n", r->ictrl);
	SOC_LOGD("	osc_trig: %8x\r\n", r->osc_trig);
	SOC_LOGD("	osccal_trig: %8x\r\n", r->osccal_trig);
	SOC_LOGD("	cnti: %8x\r\n", r->cnti);
	SOC_LOGD("	spi_rst: %8x\r\n", r->spi_rst);
	SOC_LOGD("	amsel: %8x\r\n", r->amsel);
	SOC_LOGD("	divctrl: %8x\r\n", r->divctrl);
	SOC_LOGD("	nc_30_30: %8x\r\n", r->nc_30_30);
	SOC_LOGD("	nc_31_31: %8x\r\n", r->nc_31_31);
}

static void sys_dump_ana_reg2(void)
{
	sys_ana_reg2_t *r = (sys_ana_reg2_t *)(SOC_SYS_REG_BASE + (0x42 << 2));

	SOC_LOGD("ana_reg2: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x42 << 2)));
	SOC_LOGD("	xtalh_ctune: %8x\r\n", r->xtalh_ctune);
	SOC_LOGD("	force_26mpll: %8x\r\n", r->force_26mpll);
	SOC_LOGD("	gadc_cmp_ictrl: %8x\r\n", r->gadc_cmp_ictrl);
	SOC_LOGD("	gadc_inbuf_ictrl: %8x\r\n", r->gadc_inbuf_ictrl);
	SOC_LOGD("	gadc_refbuf_ictrl: %8x\r\n", r->gadc_refbuf_ictrl);
	SOC_LOGD("	gadc_nobuf_enable: %8x\r\n", r->gadc_nobuf_enable);
	SOC_LOGD("	vref_scale: %8x\r\n", r->vref_scale);
	SOC_LOGD("	scal_en: %8x\r\n", r->scal_en);
	SOC_LOGD("	gadc_capcal_en: %8x\r\n", r->gadc_capcal_en);
	SOC_LOGD("	gadc_capcal: %8x\r\n", r->gadc_capcal);
	SOC_LOGD("	sp_nt_ctrl: %8x\r\n", r->sp_nt_ctrl);
}

static void sys_dump_ana_reg3(void)
{
	sys_ana_reg3_t *r = (sys_ana_reg3_t *)(SOC_SYS_REG_BASE + (0x43 << 2));

	SOC_LOGD("ana_reg3: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x43 << 2)));
	SOC_LOGD("	lbw0v9: %8x\r\n", r->lbw0v9);
	SOC_LOGD("	sdclk_sel: %8x\r\n", r->sdclk_sel);
	SOC_LOGD("	sarrlck_rlten: %8x\r\n", r->sarrlck_rlten);
	SOC_LOGD("	sarrlck_inv: %8x\r\n", r->sarrlck_inv);
	SOC_LOGD("	bufdrvtrm0v9: %8x\r\n", r->bufdrvtrm0v9);
	SOC_LOGD("	nc_5_5: %8x\r\n", r->nc_5_5);
	SOC_LOGD("	inbufen0v9: %8x\r\n", r->inbufen0v9);
	SOC_LOGD("	hres_sel0v9: %8x\r\n", r->hres_sel0v9);
	SOC_LOGD("	hpssren: %8x\r\n", r->hpssren);
	SOC_LOGD("	ck_sel: %8x\r\n", r->ck_sel);
	SOC_LOGD("	anabuf_sel_rx: %8x\r\n", r->anabuf_sel_rx);
	SOC_LOGD("	pwd_xtalldo: %8x\r\n", r->pwd_xtalldo);
	SOC_LOGD("	iamp: %8x\r\n", r->iamp);
	SOC_LOGD("	vddren: %8x\r\n", r->vddren);
	SOC_LOGD("	xamp: %8x\r\n", r->xamp);
	SOC_LOGD("	vosel: %8x\r\n", r->vosel);
	SOC_LOGD("	en_xtalh_sleep: %8x\r\n", r->en_xtalh_sleep);
	SOC_LOGD("	xtal40_en: %8x\r\n", r->xtal40_en);
	SOC_LOGD("	bufictrl: %8x\r\n", r->bufictrl);
	SOC_LOGD("	ibias_ctrl: %8x\r\n", r->ibias_ctrl);
	SOC_LOGD("	icore_ctrl: %8x\r\n", r->icore_ctrl);
}

static void sys_dump_ana_reg4(void)
{
	sys_ana_reg4_t *r = (sys_ana_reg4_t *)(SOC_SYS_REG_BASE + (0x44 << 2));

	SOC_LOGD("ana_reg4: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x44 << 2)));
	SOC_LOGD("	anabuf_sel_tx: %8x\r\n", r->anabuf_sel_tx);
	SOC_LOGD("	trng_tsten: %8x\r\n", r->trng_tsten);
	SOC_LOGD("	itune_ref: %8x\r\n", r->itune_ref);
	SOC_LOGD("	itune_opa: %8x\r\n", r->itune_opa);
	SOC_LOGD("	itune_cmp: %8x\r\n", r->itune_cmp);
	SOC_LOGD("	rnooise_sel: %8x\r\n", r->rnooise_sel);
	SOC_LOGD("	fslow_sel: %8x\r\n", r->fslow_sel);
	SOC_LOGD("	ffast_sel: %8x\r\n", r->ffast_sel);
	SOC_LOGD("	trng_tstck_sel: %8x\r\n", r->trng_tstck_sel);
	SOC_LOGD("	cktst_sel: %8x\r\n", r->cktst_sel);
	SOC_LOGD("	ck_tst_enbale: %8x\r\n", r->ck_tst_enbale);
	SOC_LOGD("	sw_bias: %8x\r\n", r->sw_bias);
	SOC_LOGD("	crb: %8x\r\n", r->crb);
	SOC_LOGD("	port_enablel: %8x\r\n", r->port_enablel);
}

static void sys_dump_ana_reg5(void)
{
	sys_ana_reg5_t *r = (sys_ana_reg5_t *)(SOC_SYS_REG_BASE + (0x45 << 2));

	SOC_LOGD("ana_reg5: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x45 << 2)));
	SOC_LOGD("	nc_0_0: %8x\r\n", r->nc_0_0);
	SOC_LOGD("	en_xtall: %8x\r\n", r->en_xtall);
	SOC_LOGD("	en_dco: %8x\r\n", r->en_dco);
	SOC_LOGD("	nc_3_3: %8x\r\n", r->nc_3_3);
	SOC_LOGD("	en_temp: %8x\r\n", r->en_temp);
	SOC_LOGD("	en_dpll: %8x\r\n", r->en_dpll);
	SOC_LOGD("	en_cb: %8x\r\n", r->en_cb);
	SOC_LOGD("	en_lcd: %8x\r\n", r->en_lcd);
	SOC_LOGD("	nc_8_9: %8x\r\n", r->nc_8_9);
	SOC_LOGD("	adc_div: %8x\r\n", r->adc_div);
	SOC_LOGD("	rosc_disable: %8x\r\n", r->rosc_disable);
	SOC_LOGD("	pwdaudpll: %8x\r\n", r->pwdaudpll);
	SOC_LOGD("	pwd_rosc_spi: %8x\r\n", r->pwd_rosc_spi);
	SOC_LOGD("	nc_15_15: %8x\r\n", r->nc_15_15);
	SOC_LOGD("	itune_xtall: %8x\r\n", r->itune_xtall);
	SOC_LOGD("	xtall_ten: %8x\r\n", r->xtall_ten);
	SOC_LOGD("	rosc_tsten: %8x\r\n", r->rosc_tsten);
	SOC_LOGD("	bcal_start: %8x\r\n", r->bcal_start);
	SOC_LOGD("	bcal_en: %8x\r\n", r->bcal_en);
	SOC_LOGD("	bcal_sel: %8x\r\n", r->bcal_sel);
	SOC_LOGD("	vbias: %8x\r\n", r->vbias);
}

static void sys_dump_ana_reg6(void)
{
	sys_ana_reg6_t *r = (sys_ana_reg6_t *)(SOC_SYS_REG_BASE + (0x46 << 2));

	SOC_LOGD("ana_reg6: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x46 << 2)));
	SOC_LOGD("	calib_interval: %8x\r\n", r->calib_interval);
	SOC_LOGD("	modify_interval: %8x\r\n", r->modify_interval);
	SOC_LOGD("	xtal_wakeup_time: %8x\r\n", r->xtal_wakeup_time);
	SOC_LOGD("	spi_trig: %8x\r\n", r->spi_trig);
	SOC_LOGD("	modifi_auto: %8x\r\n", r->modifi_auto);
	SOC_LOGD("	calib_auto: %8x\r\n", r->calib_auto);
	SOC_LOGD("	cal_mode: %8x\r\n", r->cal_mode);
	SOC_LOGD("	manu_ena: %8x\r\n", r->manu_ena);
	SOC_LOGD("	manu_cin: %8x\r\n", r->manu_cin);
}

static void sys_dump_ana_reg7(void)
{
	SOC_LOGD("ana_reg7: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x47 << 2)));
}

static void sys_dump_ana_reg8(void)
{
	sys_ana_reg8_t *r = (sys_ana_reg8_t *)(SOC_SYS_REG_BASE + (0x48 << 2));

	SOC_LOGD("ana_reg8: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x48 << 2)));
	SOC_LOGD("	ioldo_lp: %8x\r\n", r->ioldo_lp);
	SOC_LOGD("	coreldo_hp: %8x\r\n", r->coreldo_hp);
	SOC_LOGD("	dldohp: %8x\r\n", r->dldohp);
	SOC_LOGD("	t_vanaldosel: %8x\r\n", r->t_vanaldosel);
	SOC_LOGD("	r_vanaldosel: %8x\r\n", r->r_vanaldosel);
	SOC_LOGD("	en_trsw: %8x\r\n", r->en_trsw);
	SOC_LOGD("	aldohp: %8x\r\n", r->aldohp);
	SOC_LOGD("	anacurlim: %8x\r\n", r->anacurlim);
	SOC_LOGD("	violdosel: %8x\r\n", r->violdosel);
	SOC_LOGD("	iocurlim: %8x\r\n", r->iocurlim);
	SOC_LOGD("	valoldosel: %8x\r\n", r->valoldosel);
	SOC_LOGD("	alopowsel: %8x\r\n", r->alopowsel);
	SOC_LOGD("	en_fast_aloldo: %8x\r\n", r->en_fast_aloldo);
	SOC_LOGD("	aloldohp: %8x\r\n", r->aloldohp);
	SOC_LOGD("	bgcal: %8x\r\n", r->bgcal);
	SOC_LOGD("	vbgcalmode: %8x\r\n", r->vbgcalmode);
	SOC_LOGD("	vbgcalstart: %8x\r\n", r->vbgcalstart);
	SOC_LOGD("	pwd_bgcal: %8x\r\n", r->pwd_bgcal);
	SOC_LOGD("	spi_envbg: %8x\r\n", r->spi_envbg);
}

static void sys_dump_ana_reg9(void)
{
	sys_ana_reg9_t *r = (sys_ana_reg9_t *)(SOC_SYS_REG_BASE + (0x49 << 2));

	SOC_LOGD("ana_reg9: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x49 << 2)));
	SOC_LOGD("	wkgpiosel1: %8x\r\n", r->wkgpiosel1);
	SOC_LOGD("	rst_wks1v: %8x\r\n", r->rst_wks1v);
	SOC_LOGD("	wkgpiosel2: %8x\r\n", r->wkgpiosel2);
	SOC_LOGD("	spi_latch1v: %8x\r\n", r->spi_latch1v);
	SOC_LOGD("	digcurlim: %8x\r\n", r->digcurlim);
	SOC_LOGD("	pupres_enb1v: %8x\r\n", r->pupres_enb1v);
	SOC_LOGD("	pdnres_en1v: %8x\r\n", r->pdnres_en1v);
	SOC_LOGD("	d_veasel1v: %8x\r\n", r->d_veasel1v);
	SOC_LOGD("	ensfsdd: %8x\r\n", r->ensfsdd);
	SOC_LOGD("	vcorehsel: %8x\r\n", r->vcorehsel);
	SOC_LOGD("	vcorelsel: %8x\r\n", r->vcorelsel);
	SOC_LOGD("	vlden: %8x\r\n", r->vlden);
	SOC_LOGD("	en_fast_coreldo: %8x\r\n", r->en_fast_coreldo);
	SOC_LOGD("	pwdcoreldo: %8x\r\n", r->pwdcoreldo);
	SOC_LOGD("	vdighsel: %8x\r\n", r->vdighsel);
	SOC_LOGD("	vdigsel: %8x\r\n", r->vdigsel);
	SOC_LOGD("	vdd12lden: %8x\r\n", r->vdd12lden);
}

static void sys_dump_ana_reg10(void)
{
	sys_ana_reg10_t *r = (sys_ana_reg10_t *)(SOC_SYS_REG_BASE + (0x4a << 2));

	SOC_LOGD("ana_reg10: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4a << 2)));
	SOC_LOGD("	nc_0_2: %8x\r\n", r->nc_0_2);
	SOC_LOGD("	vbspbuflp1v: %8x\r\n", r->vbspbuflp1v);
	SOC_LOGD("	azcdcnt_manu: %8x\r\n", r->azcdcnt_manu);
	SOC_LOGD("	timer_sel: %8x\r\n", r->timer_sel);
	SOC_LOGD("	vddgpio_sel: %8x\r\n", r->vddgpio_sel);
	SOC_LOGD("	en_usbvcc1v8: %8x\r\n", r->en_usbvcc1v8);
	SOC_LOGD("	en_usbvcc3v: %8x\r\n", r->en_usbvcc3v);
	SOC_LOGD("	nc_14_14: %8x\r\n", r->nc_14_14);
	SOC_LOGD("	spi_timerwken: %8x\r\n", r->spi_timerwken);
	SOC_LOGD("	spi_byp32pwd: %8x\r\n", r->spi_byp32pwd);
	SOC_LOGD("	sd: %8x\r\n", r->sd);
	SOC_LOGD("	nc_18_18: %8x\r\n", r->nc_18_18);
	SOC_LOGD("	iobyapssen: %8x\r\n", r->iobyapssen);
	SOC_LOGD("	ckfs: %8x\r\n", r->ckfs);
	SOC_LOGD("	ckintsel: %8x\r\n", r->ckintsel);
	SOC_LOGD("	osccaltrig: %8x\r\n", r->osccaltrig);
	SOC_LOGD("	mroscsel: %8x\r\n", r->mroscsel);
	SOC_LOGD("	mrosci_cal: %8x\r\n", r->mrosci_cal);
	SOC_LOGD("	mrosccap_cal: %8x\r\n", r->mrosccap_cal);
}

static void sys_dump_ana_reg11(void)
{
	sys_ana_reg11_t *r = (sys_ana_reg11_t *)(SOC_SYS_REG_BASE + (0x4b << 2));

	SOC_LOGD("ana_reg11: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4b << 2)));
	SOC_LOGD("	sfsr: %8x\r\n", r->sfsr);
	SOC_LOGD("	ensfsaa: %8x\r\n", r->ensfsaa);
	SOC_LOGD("	apfms: %8x\r\n", r->apfms);
	SOC_LOGD("	atmpo_sel: %8x\r\n", r->atmpo_sel);
	SOC_LOGD("	ampoen: %8x\r\n", r->ampoen);
	SOC_LOGD("	enpowa: %8x\r\n", r->enpowa);
	SOC_LOGD("	avea_sel: %8x\r\n", r->avea_sel);
	SOC_LOGD("	aforcepfm: %8x\r\n", r->aforcepfm);
	SOC_LOGD("	acls: %8x\r\n", r->acls);
	SOC_LOGD("	aswrsten: %8x\r\n", r->aswrsten);
	SOC_LOGD("	aripc: %8x\r\n", r->aripc);
	SOC_LOGD("	arampc: %8x\r\n", r->arampc);
	SOC_LOGD("	arampcen: %8x\r\n", r->arampcen);
	SOC_LOGD("	aenburst: %8x\r\n", r->aenburst);
	SOC_LOGD("	apfmen: %8x\r\n", r->apfmen);
	SOC_LOGD("	aldosel: %8x\r\n", r->aldosel);
}

static void sys_dump_ana_reg12(void)
{
	sys_ana_reg12_t *r = (sys_ana_reg12_t *)(SOC_SYS_REG_BASE + (0x4c << 2));

	SOC_LOGD("ana_reg12: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4c << 2)));
	SOC_LOGD("	buckd_softst: %8x\r\n", r->buckd_softst);
	SOC_LOGD("	dzcdcnt_manu: %8x\r\n", r->dzcdcnt_manu);
	SOC_LOGD("	clk_sel: %8x\r\n", r->clk_sel);
	SOC_LOGD("	dpfms: %8x\r\n", r->dpfms);
	SOC_LOGD("	dtmpo_sel: %8x\r\n", r->dtmpo_sel);
	SOC_LOGD("	dmpoen: %8x\r\n", r->dmpoen);
	SOC_LOGD("	dforcepfm: %8x\r\n", r->dforcepfm);
	SOC_LOGD("	dcls: %8x\r\n", r->dcls);
	SOC_LOGD("	dswrsten: %8x\r\n", r->dswrsten);
	SOC_LOGD("	dripc: %8x\r\n", r->dripc);
	SOC_LOGD("	drampc: %8x\r\n", r->drampc);
	SOC_LOGD("	drampcen: %8x\r\n", r->drampcen);
	SOC_LOGD("	denburst: %8x\r\n", r->denburst);
	SOC_LOGD("	dpfmen: %8x\r\n", r->dpfmen);
	SOC_LOGD("	dldosel: %8x\r\n", r->dldosel);
}

static void sys_dump_ana_reg13(void)
{
	sys_ana_reg13_t *r = (sys_ana_reg13_t *)(SOC_SYS_REG_BASE + (0x4d << 2));

	SOC_LOGD("ana_reg13: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4d << 2)));
	SOC_LOGD("	pwdovp1v: %8x\r\n", r->pwdovp1v);
	SOC_LOGD("	asoft_stc: %8x\r\n", r->asoft_stc);
	SOC_LOGD("	dldo_czsel: %8x\r\n", r->dldo_czsel);
	SOC_LOGD("	dldo_rzsel: %8x\r\n", r->dldo_rzsel);
	SOC_LOGD("	nc_10_11: %8x\r\n", r->nc_10_11);
	SOC_LOGD("	vtrxspisel: %8x\r\n", r->vtrxspisel);
	SOC_LOGD("	nc_14_19: %8x\r\n", r->nc_14_19);
	SOC_LOGD("	aldo_czsel: %8x\r\n", r->aldo_czsel);
	SOC_LOGD("	aldo_rzsel: %8x\r\n", r->aldo_rzsel);
	SOC_LOGD("	nc_25_27: %8x\r\n", r->nc_25_27);
	SOC_LOGD("	psldo_swb: %8x\r\n", r->psldo_swb);
	SOC_LOGD("	vpsramsel: %8x\r\n", r->vpsramsel);
	SOC_LOGD("	enpsram: %8x\r\n", r->enpsram);
}

static void sys_dump_ana_reg14(void)
{
	sys_ana_reg14_t *r = (sys_ana_reg14_t *)(SOC_SYS_REG_BASE + (0x4e << 2));

	SOC_LOGD("ana_reg14: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4e << 2)));
	SOC_LOGD("	reg: %8x\r\n", r->reg);
	SOC_LOGD("	en_adcmode: %8x\r\n", r->en_adcmode);
	SOC_LOGD("	en_out_test1v: %8x\r\n", r->en_out_test1v);
	SOC_LOGD("	nc_12_12: %8x\r\n", r->nc_12_12);
	SOC_LOGD("	sel_seri_cap: %8x\r\n", r->sel_seri_cap);
	SOC_LOGD("	en_seri_cap: %8x\r\n", r->en_seri_cap);
	SOC_LOGD("	cal_ctrl: %8x\r\n", r->cal_ctrl);
	SOC_LOGD("	cal_vth: %8x\r\n", r->cal_vth);
	SOC_LOGD("	crg: %8x\r\n", r->crg);
	SOC_LOGD("	vrefs: %8x\r\n", r->vrefs);
	SOC_LOGD("	gain_s: %8x\r\n", r->gain_s);
	SOC_LOGD("	td_latch1v: %8x\r\n", r->td_latch1v);
	SOC_LOGD("	pwd_td: %8x\r\n", r->pwd_td);
}

static void sys_dump_ana_reg15(void)
{
	sys_ana_reg15_t *r = (sys_ana_reg15_t *)(SOC_SYS_REG_BASE + (0x4f << 2));

	SOC_LOGD("ana_reg15: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4f << 2)));
	SOC_LOGD("	test_number1v: %8x\r\n", r->test_number1v);
	SOC_LOGD("	test_period1v: %8x\r\n", r->test_period1v);
	SOC_LOGD("	chs: %8x\r\n", r->chs);
	SOC_LOGD("	chs_sel_cal1v: %8x\r\n", r->chs_sel_cal1v);
	SOC_LOGD("	cal_done_clr1v: %8x\r\n", r->cal_done_clr1v);
	SOC_LOGD("	en_cal_force1v: %8x\r\n", r->en_cal_force1v);
	SOC_LOGD("	en_cal_auto1v: %8x\r\n", r->en_cal_auto1v);
	SOC_LOGD("	en_scan: %8x\r\n", r->en_scan);
}

static void sys_dump_ana_reg16(void)
{
	sys_ana_reg16_t *r = (sys_ana_reg16_t *)(SOC_SYS_REG_BASE + (0x50 << 2));

	SOC_LOGD("ana_reg16: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x50 << 2)));
	SOC_LOGD("	int_en: %8x\r\n", r->int_en);
	SOC_LOGD("	int_en16: %8x\r\n", r->int_en16);
	SOC_LOGD("	nc_17_17: %8x\r\n", r->nc_17_17);
	SOC_LOGD("	nc_18_18: %8x\r\n", r->nc_18_18);
	SOC_LOGD("	cal_number1v: %8x\r\n", r->cal_number1v);
	SOC_LOGD("	cal_period1v: %8x\r\n", r->cal_period1v);
}

static void sys_dump_ana_reg17(void)
{
	sys_ana_reg17_t *r = (sys_ana_reg17_t *)(SOC_SYS_REG_BASE + (0x51 << 2));

	SOC_LOGD("ana_reg17: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x51 << 2)));
	SOC_LOGD("	int_clr: %8x\r\n", r->int_clr);
	SOC_LOGD("	int_clr16: %8x\r\n", r->int_clr16);
	SOC_LOGD("	ck_adc_sel: %8x\r\n", r->ck_adc_sel);
	SOC_LOGD("	int_clr_sel: %8x\r\n", r->int_clr_sel);
	SOC_LOGD("	en_lpmod: %8x\r\n", r->en_lpmod);
	SOC_LOGD("	en_testcmp1v: %8x\r\n", r->en_testcmp1v);
	SOC_LOGD("	en_man_wr1v: %8x\r\n", r->en_man_wr1v);
	SOC_LOGD("	en_manmod1v: %8x\r\n", r->en_manmod1v);
	SOC_LOGD("	cap_calspi1v: %8x\r\n", r->cap_calspi1v);
}

static void sys_dump_ana_reg18(void)
{
	sys_ana_reg18_t *r = (sys_ana_reg18_t *)(SOC_SYS_REG_BASE + (0x52 << 2));

	SOC_LOGD("ana_reg18: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x52 << 2)));
	SOC_LOGD("	iselaud: %8x\r\n", r->iselaud);
	SOC_LOGD("	audck_rlcen1v: %8x\r\n", r->audck_rlcen1v);
	SOC_LOGD("	lchckinven1v: %8x\r\n", r->lchckinven1v);
	SOC_LOGD("	enaudbias: %8x\r\n", r->enaudbias);
	SOC_LOGD("	enadcbias: %8x\r\n", r->enadcbias);
	SOC_LOGD("	enmicbias: %8x\r\n", r->enmicbias);
	SOC_LOGD("	adcckinven1v: %8x\r\n", r->adcckinven1v);
	SOC_LOGD("	dacfb2st0v9: %8x\r\n", r->dacfb2st0v9);
	SOC_LOGD("	nc_8_8: %8x\r\n", r->nc_8_8);
	SOC_LOGD("	micbias_trm: %8x\r\n", r->micbias_trm);
	SOC_LOGD("	micbias_voc: %8x\r\n", r->micbias_voc);
	SOC_LOGD("	vrefsel1v: %8x\r\n", r->vrefsel1v);
	SOC_LOGD("	capswspi: %8x\r\n", r->capswspi);
	SOC_LOGD("	adref_sel: %8x\r\n", r->adref_sel);
	SOC_LOGD("	nc_24_29: %8x\r\n", r->nc_24_29);
	SOC_LOGD("	lchck_sel: %8x\r\n", r->lchck_sel);
	SOC_LOGD("	spi_dacckpssel: %8x\r\n", r->spi_dacckpssel);
}

static void sys_dump_ana_reg19(void)
{
	sys_ana_reg19_t *r = (sys_ana_reg19_t *)(SOC_SYS_REG_BASE + (0x53 << 2));

	SOC_LOGD("ana_reg19: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x53 << 2)));
	SOC_LOGD("	isel: %8x\r\n", r->isel);
	SOC_LOGD("	micirsel1: %8x\r\n", r->micirsel1);
	SOC_LOGD("	micdacit: %8x\r\n", r->micdacit);
	SOC_LOGD("	micdacih: %8x\r\n", r->micdacih);
	SOC_LOGD("	micsingleen: %8x\r\n", r->micsingleen);
	SOC_LOGD("	dccompen: %8x\r\n", r->dccompen);
	SOC_LOGD("	micgain: %8x\r\n", r->micgain);
	SOC_LOGD("	micdacen: %8x\r\n", r->micdacen);
	SOC_LOGD("	stg2lsen1v: %8x\r\n", r->stg2lsen1v);
	SOC_LOGD("	openloopcal1v: %8x\r\n", r->openloopcal1v);
	SOC_LOGD("	callatch: %8x\r\n", r->callatch);
	SOC_LOGD("	vcmsel: %8x\r\n", r->vcmsel);
	SOC_LOGD("	dwamode: %8x\r\n", r->dwamode);
	SOC_LOGD("	r2ren: %8x\r\n", r->r2ren);
	SOC_LOGD("	nc_26_27: %8x\r\n", r->nc_26_27);
	SOC_LOGD("	micen: %8x\r\n", r->micen);
	SOC_LOGD("	rst: %8x\r\n", r->rst);
	SOC_LOGD("	bpdwa1v: %8x\r\n", r->bpdwa1v);
	SOC_LOGD("	hcen1stg: %8x\r\n", r->hcen1stg);
}

static void sys_dump_ana_reg20(void)
{
	sys_ana_reg20_t *r = (sys_ana_reg20_t *)(SOC_SYS_REG_BASE + (0x54 << 2));

	SOC_LOGD("ana_reg20: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x54 << 2)));
	SOC_LOGD("	hpdac: %8x\r\n", r->hpdac);
	SOC_LOGD("	calcon_sel: %8x\r\n", r->calcon_sel);
	SOC_LOGD("	oscdac: %8x\r\n", r->oscdac);
	SOC_LOGD("	ocendac: %8x\r\n", r->ocendac);
	SOC_LOGD("	vcmsel: %8x\r\n", r->vcmsel);
	SOC_LOGD("	adjdacref: %8x\r\n", r->adjdacref);
	SOC_LOGD("	dcochg: %8x\r\n", r->dcochg);
	SOC_LOGD("	diffen: %8x\r\n", r->diffen);
	SOC_LOGD("	endaccal: %8x\r\n", r->endaccal);
	SOC_LOGD("	nc_15_15: %8x\r\n", r->nc_15_15);
	SOC_LOGD("	lendcoc: %8x\r\n", r->lendcoc);
	SOC_LOGD("	nc_17_17: %8x\r\n", r->nc_17_17);
	SOC_LOGD("	lenvcmd: %8x\r\n", r->lenvcmd);
	SOC_LOGD("	dacdrven: %8x\r\n", r->dacdrven);
	SOC_LOGD("	nc_20_20: %8x\r\n", r->nc_20_20);
	SOC_LOGD("	daclen: %8x\r\n", r->daclen);
	SOC_LOGD("	dacg: %8x\r\n", r->dacg);
	SOC_LOGD("	dacmute: %8x\r\n", r->dacmute);
	SOC_LOGD("	dacdwamode_sel: %8x\r\n", r->dacdwamode_sel);
	SOC_LOGD("	dacsel: %8x\r\n", r->dacsel);
}

static void sys_dump_ana_reg21(void)
{
	sys_ana_reg21_t *r = (sys_ana_reg21_t *)(SOC_SYS_REG_BASE + (0x55 << 2));

	SOC_LOGD("ana_reg21: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x55 << 2)));
	SOC_LOGD("	lmdcin: %8x\r\n", r->lmdcin);
	SOC_LOGD("	nc_8_15: %8x\r\n", r->nc_8_15);
	SOC_LOGD("	spirst_ovc: %8x\r\n", r->spirst_ovc);
	SOC_LOGD("	nc_17_17: %8x\r\n", r->nc_17_17);
	SOC_LOGD("	enidacl: %8x\r\n", r->enidacl);
	SOC_LOGD("	dac3rdhc0v9: %8x\r\n", r->dac3rdhc0v9);
	SOC_LOGD("	hc2s: %8x\r\n", r->hc2s);
	SOC_LOGD("	rfb_ctrl: %8x\r\n", r->rfb_ctrl);
	SOC_LOGD("	vcmsel: %8x\r\n", r->vcmsel);
	SOC_LOGD("	enbs: %8x\r\n", r->enbs);
	SOC_LOGD("	calck_sel0v9: %8x\r\n", r->calck_sel0v9);
	SOC_LOGD("	bpdwa0v9: %8x\r\n", r->bpdwa0v9);
	SOC_LOGD("	looprst0v9: %8x\r\n", r->looprst0v9);
	SOC_LOGD("	oct0v9: %8x\r\n", r->oct0v9);
	SOC_LOGD("	sout0v9: %8x\r\n", r->sout0v9);
	SOC_LOGD("	hc0v9: %8x\r\n", r->hc0v9);
}

static void sys_dump_ana_reg22(void)
{
	sys_ana_reg22_t *r = (sys_ana_reg22_t *)(SOC_SYS_REG_BASE + (0x56 << 2));

	SOC_LOGD("ana_reg22: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x56 << 2)));
	SOC_LOGD("	nc_0_0: %8x\r\n", r->nc_0_0);
	SOC_LOGD("	lvref: %8x\r\n", r->lvref);
	SOC_LOGD("	hvref: %8x\r\n", r->hvref);
	SOC_LOGD("	bandm: %8x\r\n", r->bandm);
	SOC_LOGD("	nc_12_23: %8x\r\n", r->nc_12_23);
	SOC_LOGD("	n_int: %8x\r\n", r->n_int);
	SOC_LOGD("	nc_30_30: %8x\r\n", r->nc_30_30);
	SOC_LOGD("	errdet_en: %8x\r\n", r->errdet_en);
}

static void sys_dump_ana_reg23(void)
{
	sys_ana_reg23_t *r = (sys_ana_reg23_t *)(SOC_SYS_REG_BASE + (0x57 << 2));

	SOC_LOGD("ana_reg23: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x57 << 2)));
	SOC_LOGD("	int_mod: %8x\r\n", r->int_mod);
	SOC_LOGD("	nsyn: %8x\r\n", r->nsyn);
	SOC_LOGD("	open_enb: %8x\r\n", r->open_enb);
	SOC_LOGD("	reset: %8x\r\n", r->reset);
	SOC_LOGD("	ioffset: %8x\r\n", r->ioffset);
	SOC_LOGD("	lpfrz: %8x\r\n", r->lpfrz);
	SOC_LOGD("	vsel: %8x\r\n", r->vsel);
	SOC_LOGD("	nc_12_14: %8x\r\n", r->nc_12_14);
	SOC_LOGD("	pwd_lockdet: %8x\r\n", r->pwd_lockdet);
	SOC_LOGD("	nc_16_17: %8x\r\n", r->nc_16_17);
	SOC_LOGD("	spi_trigger: %8x\r\n", r->spi_trigger);
	SOC_LOGD("	manual: %8x\r\n", r->manual);
	SOC_LOGD("	test_en: %8x\r\n", r->test_en);
	SOC_LOGD("	nc_21_22: %8x\r\n", r->nc_21_22);
	SOC_LOGD("	icp: %8x\r\n", r->icp);
	SOC_LOGD("	nc_25_27: %8x\r\n", r->nc_25_27);
	SOC_LOGD("	errdet_spien: %8x\r\n", r->errdet_spien);
	SOC_LOGD("	dlysel: %8x\r\n", r->dlysel);
	SOC_LOGD("	audioen: %8x\r\n", r->audioen);
}

static void sys_dump_ana_reg24(void)
{
	sys_ana_reg24_t *r = (sys_ana_reg24_t *)(SOC_SYS_REG_BASE + (0x58 << 2));

	SOC_LOGD("ana_reg24: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x58 << 2)));
	SOC_LOGD("	int_clr_cal: %8x\r\n", r->int_clr_cal);
	SOC_LOGD("	int_en_cal: %8x\r\n", r->int_en_cal);
}

static void sys_dump_ana_reg25(void)
{
	sys_ana_reg25_t *r = (sys_ana_reg25_t *)(SOC_SYS_REG_BASE + (0x59 << 2));

	SOC_LOGD("ana_reg25: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x59 << 2)));
	SOC_LOGD("	int_mod: %8x\r\n", r->int_mod);
	SOC_LOGD("	nsyn: %8x\r\n", r->nsyn);
	SOC_LOGD("	open_enb: %8x\r\n", r->open_enb);
	SOC_LOGD("	reset: %8x\r\n", r->reset);
	SOC_LOGD("	ioffsetl: %8x\r\n", r->ioffsetl);
	SOC_LOGD("	lpfrz: %8x\r\n", r->lpfrz);
	SOC_LOGD("	vsel: %8x\r\n", r->vsel);
	SOC_LOGD("	vsel_cal: %8x\r\n", r->vsel_cal);
	SOC_LOGD("	pwd_lockdet: %8x\r\n", r->pwd_lockdet);
	SOC_LOGD("	lockdet_bypass: %8x\r\n", r->lockdet_bypass);
	SOC_LOGD("	ckref_loop_sel: %8x\r\n", r->ckref_loop_sel);
	SOC_LOGD("	spi_trigger: %8x\r\n", r->spi_trigger);
	SOC_LOGD("	manual: %8x\r\n", r->manual);
	SOC_LOGD("	test_ckaudio_en: %8x\r\n", r->test_ckaudio_en);
	SOC_LOGD("	ck2xen: %8x\r\n", r->ck2xen);
	SOC_LOGD("	icp: %8x\r\n", r->icp);
	SOC_LOGD("	cktst_sel: %8x\r\n", r->cktst_sel);
	SOC_LOGD("	edgesel_nck: %8x\r\n", r->edgesel_nck);
	SOC_LOGD("	nloaddlyen: %8x\r\n", r->nloaddlyen);
	SOC_LOGD("	bypass_caldone_auto: %8x\r\n", r->bypass_caldone_auto);
	SOC_LOGD("	cal_res_spi: %8x\r\n", r->cal_res_spi);
	SOC_LOGD("	audioen: %8x\r\n", r->audioen);
}

static void sys_dump_ana_reg26(void)
{
	sys_ana_reg26_t *r = (sys_ana_reg26_t *)(SOC_SYS_REG_BASE + (0x5a << 2));

	SOC_LOGD("ana_reg26: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x5a << 2)));
	SOC_LOGD("	n: %8x\r\n", r->n);
	SOC_LOGD("	calres_spien: %8x\r\n", r->calres_spien);
	SOC_LOGD("	calrefen: %8x\r\n", r->calrefen);
}

static void sys_dump_ana_reg27(void)
{
	sys_ana_reg27_t *r = (sys_ana_reg27_t *)(SOC_SYS_REG_BASE + (0x5b << 2));

	SOC_LOGD("ana_reg27: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x5b << 2)));
	SOC_LOGD("	isel: %8x\r\n", r->isel);
	SOC_LOGD("	micirsel1: %8x\r\n", r->micirsel1);
	SOC_LOGD("	micdacit: %8x\r\n", r->micdacit);
	SOC_LOGD("	micdacih: %8x\r\n", r->micdacih);
	SOC_LOGD("	micsingleen: %8x\r\n", r->micsingleen);
	SOC_LOGD("	dccompen: %8x\r\n", r->dccompen);
	SOC_LOGD("	micgain: %8x\r\n", r->micgain);
	SOC_LOGD("	micdacen: %8x\r\n", r->micdacen);
	SOC_LOGD("	stg2lsen1v: %8x\r\n", r->stg2lsen1v);
	SOC_LOGD("	openloopcal1v: %8x\r\n", r->openloopcal1v);
	SOC_LOGD("	callatch: %8x\r\n", r->callatch);
	SOC_LOGD("	vcmsel: %8x\r\n", r->vcmsel);
	SOC_LOGD("	dwamode: %8x\r\n", r->dwamode);
	SOC_LOGD("	r2ren: %8x\r\n", r->r2ren);
	SOC_LOGD("	nc_26_27: %8x\r\n", r->nc_26_27);
	SOC_LOGD("	micen: %8x\r\n", r->micen);
	SOC_LOGD("	rst: %8x\r\n", r->rst);
	SOC_LOGD("	bpdwa1v: %8x\r\n", r->bpdwa1v);
	SOC_LOGD("	hcen1stg: %8x\r\n", r->hcen1stg);
}

static sys_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, sys_dump_device_id},
	{0x1, 0x1, sys_dump_version_id},
	{0x2, 0x2, sys_dump_cpu_storage_connect_op_select},
	{0x3, 0x3, sys_dump_cpu_current_run_status},
	{0x4, 0x4, sys_dump_cpu0_int_halt_clk_op},
	{0x5, 0x5, sys_dump_cpu1_int_halt_clk_op},
	{0x6, 0x6, sys_dump_cpu2_int_halt_clk_op},
	{0x7, 0x7, sys_dump_reserved_reg0x7},
	{0x8, 0x8, sys_dump_cpu_clk_div_mode1},
	{0x9, 0x9, sys_dump_cpu_clk_div_mode2},
	{0xa, 0xa, sys_dump_cpu_26m_wdt_clk_div},
	{0xb, 0xb, sys_dump_cpu_anaspi_freq},
	{0xc, 0xc, sys_dump_cpu_device_clk_enable},
	{0xd, 0xd, sys_dump_reserver_reg0xd},
	{0xe, 0xe, sys_dump_cpu_device_mem_ctrl1},
	{0xf, 0xf, sys_dump_cpu_device_mem_ctrl2},
	{0x10, 0x10, sys_dump_cpu_power_sleep_wakeup},
	{0x11, 0x11, sys_dump_cpu0_lv_sleep_cfg},
	{0x12, 0x12, sys_dump_cpu_device_mem_ctrl3},
	{0x13, 0x20, sys_dump_rsv_13_1f},
	{0x20, 0x20, sys_dump_cpu0_int_0_31_en},
	{0x21, 0x21, sys_dump_cpu0_int_32_63_en},
	{0x22, 0x22, sys_dump_cpu1_int_0_31_en},
	{0x23, 0x23, sys_dump_cpu1_int_32_63_en},
	{0x24, 0x24, sys_dump_cpu2_int_0_31_en},
	{0x25, 0x25, sys_dump_cpu2_int_32_63_en},
	{0x26, 0x28, sys_dump_rsv_26_27},
	{0x28, 0x28, sys_dump_cpu0_int_0_31_status},
	{0x29, 0x29, sys_dump_cpu0_int_32_63_status},
	{0x2a, 0x2a, sys_dump_cpu1_int_0_31_status},
	{0x2b, 0x2b, sys_dump_cpu1_int_32_63_status},
	{0x2c, 0x2c, sys_dump_cpu2_int_0_31_status},
	{0x2d, 0x2d, sys_dump_cpu2_int_32_63_status},
	{0x2e, 0x30, sys_dump_rsv_2e_2f},
	{0x30, 0x30, sys_dump_gpio_config0},
	{0x31, 0x31, sys_dump_gpio_config1},
	{0x32, 0x32, sys_dump_gpio_config2},
	{0x33, 0x33, sys_dump_gpio_config3},
	{0x34, 0x34, sys_dump_gpio_config4},
	{0x35, 0x35, sys_dump_gpio_config5},
	{0x36, 0x36, sys_dump_gpio_config6},
	{0x37, 0x38, sys_dump_rsv_37_37},
	{0x38, 0x38, sys_dump_sys_debug_config0},
	{0x39, 0x39, sys_dump_sys_debug_config1},
	{0x3a, 0x40, sys_dump_rsv_3a_3f},
	{0x40, 0x40, sys_dump_ana_reg0},
	{0x41, 0x41, sys_dump_ana_reg1},
	{0x42, 0x42, sys_dump_ana_reg2},
	{0x43, 0x43, sys_dump_ana_reg3},
	{0x44, 0x44, sys_dump_ana_reg4},
	{0x45, 0x45, sys_dump_ana_reg5},
	{0x46, 0x46, sys_dump_ana_reg6},
	{0x47, 0x47, sys_dump_ana_reg7},
	{0x48, 0x48, sys_dump_ana_reg8},
	{0x49, 0x49, sys_dump_ana_reg9},
	{0x4a, 0x4a, sys_dump_ana_reg10},
	{0x4b, 0x4b, sys_dump_ana_reg11},
	{0x4c, 0x4c, sys_dump_ana_reg12},
	{0x4d, 0x4d, sys_dump_ana_reg13},
	{0x4e, 0x4e, sys_dump_ana_reg14},
	{0x4f, 0x4f, sys_dump_ana_reg15},
	{0x50, 0x50, sys_dump_ana_reg16},
	{0x51, 0x51, sys_dump_ana_reg17},
	{0x52, 0x52, sys_dump_ana_reg18},
	{0x53, 0x53, sys_dump_ana_reg19},
	{0x54, 0x54, sys_dump_ana_reg20},
	{0x55, 0x55, sys_dump_ana_reg21},
	{0x56, 0x56, sys_dump_ana_reg22},
	{0x57, 0x57, sys_dump_ana_reg23},
	{0x58, 0x58, sys_dump_ana_reg24},
	{0x59, 0x59, sys_dump_ana_reg25},
	{0x5a, 0x5a, sys_dump_ana_reg26},
	{0x5b, 0x5b, sys_dump_ana_reg27},
	{-1, -1, 0}
};

void sys_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
#endif

