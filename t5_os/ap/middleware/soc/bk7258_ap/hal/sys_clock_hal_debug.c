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
#include "system_hw.h"

#if 0
#if CFG_HAL_DEBUG_SYS_CLOCK

void sys_clock_struct_dump(void)
{
	sys_hw_t *hw = (sys_hw_t *)SOC_SYSTEM_REG_BASE;
	SOC_LOGD("base=%x\r\n", (uint32_t)hw);

	SOC_LOGD("  reg_0x8 addr=%x value=%x\n", &hw->reg0x8, hw->reg0x8.v);
	SOC_LOGD("    clkdiv_core:   %x\n", hw->reg0x8.clkdiv_core);
	SOC_LOGD("    cksel_core:    %x\n", hw->reg0x8.cksel_core);
	SOC_LOGD("    clkdiv_bus:    %x\n", hw->reg0x8.clkdiv_bus);
	SOC_LOGD("    clkdiv_uart0:  %x\n", hw->reg0x8.clkdiv_uart0);
	SOC_LOGD("    clksel_uart0:  %x\n", hw->reg0x8.clksel_uart0);
	SOC_LOGD("    clkdiv_uart1:  %x\n", hw->reg0x8.clkdiv_uart1);
	SOC_LOGD("    cksel_uart1:   %x\n", hw->reg0x8.cksel_uart1);
	SOC_LOGD("    clkdiv_uart2:  %x\n", hw->reg0x8.clkdiv_uart2);
	SOC_LOGD("    cksel_uart2:   %x\n", hw->reg0x8.cksel_uart2);
	SOC_LOGD("    cksel_sadc:    %x\n", hw->reg0x8.cksel_sadc);
	SOC_LOGD("    cksel_pwm0:    %x\n", hw->reg0x8.cksel_pwm0);
	SOC_LOGD("    cksel_pwm1:    %x\n", hw->reg0x8.cksel_pwm1);
	SOC_LOGD("    cksel_tim0:    %x\n", hw->reg0x8.cksel_tim0);
	SOC_LOGD("    cksel_tim1:    %x\n", hw->reg0x8.cksel_tim1);
	SOC_LOGD("    cksel_tim2:    %x\n", hw->reg0x8.cksel_tim2);
	SOC_LOGD("    reserved0:     %x\n", hw->reg0x8.reserved0);
	SOC_LOGD("    cksel_i2s:     %x\n", hw->reg0x8.cksel_i2s);
	SOC_LOGD("    cksel_aud:     %x\n", hw->reg0x8.cksel_aud);
	SOC_LOGD("    clkdiv_jpeg:   %x\n", hw->reg0x8.clkdiv_jpeg);
	SOC_LOGD("    cksel_jpeg:    %x\n", hw->reg0x8.cksel_jpeg);
	SOC_LOGD("    clkdiv_disp_l: %x\n", hw->reg0x8.clkdiv_disp_l);
	SOC_LOGD("\r\n");

	SOC_LOGD("  reg_0x9 addr=%x value=%x\n", &hw->reg0x9, hw->reg0x9.v);
	SOC_LOGD("    clkdiv_disp_h: %x\n", hw->reg0x9.clkdiv_disp_h);
	SOC_LOGD("    cksel_disp:    %x\n", hw->reg0x9.cksel_disp);
	SOC_LOGD("    ckdiv_psram:   %x\n", hw->reg0x9.ckdiv_psram);
	SOC_LOGD("    cksel_psram:   %x\n", hw->reg0x9.cksel_psram);
	SOC_LOGD("    ckdiv_qspi0:   %x\n", hw->reg0x9.ckdiv_qspi0);
	SOC_LOGD("    reserved0:     %x\n", hw->reg0x9.reserved0);
	SOC_LOGD("    ckdiv_sdio:    %x\n", hw->reg0x9.ckdiv_sdio);
	SOC_LOGD("    cksel_sdio:    %x\n", hw->reg0x9.cksel_sdio);
	SOC_LOGD("    ckdiv_auxs:    %x\n", hw->reg0x9.ckdiv_auxs);
	SOC_LOGD("    cksel_auxs:    %x\n", hw->reg0x9.cksel_auxs);
	SOC_LOGD("    cksel_flash:   %x\n", hw->reg0x9.cksel_flash);
	SOC_LOGD("    ckdiv_flash:   %x\n", hw->reg0x9.ckdiv_flash);
	SOC_LOGD("    ckdiv_i2s0:    %x\n", hw->reg0x9.ckdiv_i2s0);
	SOC_LOGD("    reserved1:     %x\n", hw->reg0x9.reserved1);
	SOC_LOGD("\r\n");

	SOC_LOGD("  reg_0xa addr=%x value=%x\n", &hw->reg0xa, hw->reg0xa.v);
	SOC_LOGD("    ckdiv_26m:   %x\n", hw->reg0xa.ckdiv_26m);
	SOC_LOGD("    ckdiv_wdt:   %x\n", hw->reg0xa.ckdiv_wdt);
	SOC_LOGD("    clksel_spi0: %x\n", hw->reg0xa.clksel_spi0);
	SOC_LOGD("    clksel_spi1: %x\n", hw->reg0xa.clksel_spi1);
	SOC_LOGD("    reserved0:   %x\n", hw->reg0xa.reserved0);
	SOC_LOGD("\r\n");

	SOC_LOGD("  reg_0xc addr=%x value=%x\n", &hw->reg0xc, hw->reg0xc.v);
	SOC_LOGD("    i2c0_cken:  %x\n", hw->reg0xc.i2c0_cken);
	SOC_LOGD("    spi0_cken:  %x\n", hw->reg0xc.spi0_cken);
	SOC_LOGD("    uart0_cken: %x\n", hw->reg0xc.uart0_cken);
	SOC_LOGD("    pwm0_cken:  %x\n", hw->reg0xc.pwm0_cken);
	SOC_LOGD("    tim0_cken:  %x\n", hw->reg0xc.tim0_cken);
	SOC_LOGD("    sadc_cken:  %x\n", hw->reg0xc.sadc_cken);
	SOC_LOGD("    irda_cken:  %x\n", hw->reg0xc.irda_cken);
	SOC_LOGD("    efuse_cken: %x\n", hw->reg0xc.efuse_cken);
	SOC_LOGD("    i2c1_cken:  %x\n", hw->reg0xc.i2c1_cken);
	SOC_LOGD("    spi1_cken:  %x\n", hw->reg0xc.spi1_cken);
	SOC_LOGD("    uart1_cken: %x\n", hw->reg0xc.uart1_cken);
	SOC_LOGD("    uart2_cken: %x\n", hw->reg0xc.uart2_cken);
	SOC_LOGD("    pwm1_cken:  %x\n", hw->reg0xc.pwm1_cken);
	SOC_LOGD("    tim1_cken:  %x\n", hw->reg0xc.tim1_cken);
	SOC_LOGD("    tim2_cken:  %x\n", hw->reg0xc.tim2_cken);
	SOC_LOGD("    otp_cken:   %x\n", hw->reg0xc.otp_cken);
	SOC_LOGD("    i2s_cken:   %x\n", hw->reg0xc.i2s_cken);
	SOC_LOGD("    usb_cken:   %x\n", hw->reg0xc.usb_cken);
	SOC_LOGD("    can_cken:   %x\n", hw->reg0xc.can_cken);
	SOC_LOGD("    psram_cken: %x\n", hw->reg0xc.psram_cken);
	SOC_LOGD("    qspi0_cken: %x\n", hw->reg0xc.qspi0_cken);
	SOC_LOGD("    qspi1_cken: %x\n", hw->reg0xc.qspi1_cken);
	SOC_LOGD("    sdio_cken:  %x\n", hw->reg0xc.sdio_cken);
	SOC_LOGD("    auxs_cken:  %x\n", hw->reg0xc.auxs_cken);
	SOC_LOGD("    btdm_cken:  %x\n", hw->reg0xc.btdm_cken);
	SOC_LOGD("    xvr_cken:   %x\n", hw->reg0xc.xvr_cken);
	SOC_LOGD("    mac_cken:   %x\n", hw->reg0xc.mac_cken);
	SOC_LOGD("    phy_cken:   %x\n", hw->reg0xc.phy_cken);
	SOC_LOGD("    jpeg_cken:  %x\n", hw->reg0xc.jpeg_cken);
	SOC_LOGD("    disp_cken:  %x\n", hw->reg0xc.disp_cken);
	SOC_LOGD("    aud_cken:   %x\n", hw->reg0xc.aud_cken);
	SOC_LOGD("    wdt_cken:   %x\n", hw->reg0xc.wdt_cken);
	SOC_LOGD("\r\n");

	SOC_LOGD("  reg_0xd addr=%x value=%x\n", &hw->reg0xd, hw->reg0xd.v);

	SOC_LOGD("  reg_0x20 addr=%x value=%x\n", &hw->reg0x20, hw->reg0x20.v);
	SOC_LOGD("  reg_0x21 addr=%x value=%x\n", &hw->reg0x21, hw->reg0x21.v);
}

#endif
#endif

