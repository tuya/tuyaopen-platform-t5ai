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
#include "aon_pmu_hw.h"
#include "aon_pmu_hal.h"

typedef void (*aon_pmu_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	aon_pmu_dump_fn_t fn;
} aon_pmu_reg_fn_map_t;

static void aon_pmu_dump_r0(void)
{
	aon_pmu_r0_t *r = (aon_pmu_r0_t *)(SOC_AON_PMU_REG_BASE + (0x0 << 2));

	SOC_LOGD("r0: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x0 << 2)));
	SOC_LOGD("	memchk_bps: %-8x\r\n", r->memchk_bps);
	SOC_LOGD("	fast_boot: %-8x\r\n", r->fast_boot);
	SOC_LOGD("	ota_finish: %-8x\r\n", r->ota_finish);
	SOC_LOGD("	bl2_deep_sleep: %-8x\r\n", r->bl2_deep_sleep);
	SOC_LOGD("	reset_reason_cp: %-8x\r\n", r->reset_reason_cp);
	SOC_LOGD("	gpio_retention_bitmap: %-8x\r\n", r->gpio_retention_bitmap);
	SOC_LOGD("	reset_reason_ap: %-8x\r\n", r->reset_reason_ap);
	SOC_LOGD("	gpio_sleep: %-8x\r\n", r->gpio_sleep);
}

static void aon_pmu_dump_r1(void)
{
	aon_pmu_r1_t *r = (aon_pmu_r1_t *)(SOC_AON_PMU_REG_BASE + (0x1 << 2));

	SOC_LOGD("r1: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x1 << 2)));
	SOC_LOGD("	touch_select: %-8x\r\n", r->touch_select);
	SOC_LOGD("	touch_int_en: %-8x\r\n", r->touch_int_en);
	SOC_LOGD("	usbplug_int_en: %-8x\r\n", r->usbplug_int_en);
	SOC_LOGD("	clk_mod: %-8x\r\n", r->clk_mod);
	SOC_LOGD("	reserved_bit_22_31: %-8x\r\n", r->reserved_bit_22_31);
}

static void aon_pmu_dump_r2(void)
{
	aon_pmu_r2_t *r = (aon_pmu_r2_t *)(SOC_AON_PMU_REG_BASE + (0x2 << 2));

	SOC_LOGD("r2: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x2 << 2)));
	SOC_LOGD("	wdt_rst_ana: %-8x\r\n", r->wdt_rst_ana);
	SOC_LOGD("	wdt_rst_top: %-8x\r\n", r->wdt_rst_top);
	SOC_LOGD("	wdt_rst_aon: %-8x\r\n", r->wdt_rst_aon);
	SOC_LOGD("	wdt_rst_awt: %-8x\r\n", r->wdt_rst_awt);
	SOC_LOGD("	wdt_rst_gpio: %-8x\r\n", r->wdt_rst_gpio);
	SOC_LOGD("	wdt_rst_rtc: %-8x\r\n", r->wdt_rst_rtc);
	SOC_LOGD("	wdt_rst_wdt: %-8x\r\n", r->wdt_rst_wdt);
	SOC_LOGD("	wdt_rst_pmu: %-8x\r\n", r->wdt_rst_pmu);
	SOC_LOGD("	wdt_rst_phy: %-8x\r\n", r->wdt_rst_phy);
	SOC_LOGD("	reserved_bit_9_31: %-8x\r\n", r->reserved_bit_9_31);
}

static void aon_pmu_dump_r3(void)
{
	SOC_LOGD("r3: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x3 << 2)));
}

static void aon_pmu_dump_rsv_4_3f(void)
{
	for (uint32_t idx = 0; idx < 60; idx++) {
		SOC_LOGD("rsv_4_3f: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x4 + idx) << 2)));
	}
}

static void aon_pmu_dump_r40(void)
{
	aon_pmu_r40_t *r = (aon_pmu_r40_t *)(SOC_AON_PMU_REG_BASE + (0x40 << 2));

	SOC_LOGD("r40: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x40 << 2)));
	SOC_LOGD("	wake1_delay: %-8x\r\n", r->wake1_delay);
	SOC_LOGD("	wake2_delay: %-8x\r\n", r->wake2_delay);
	SOC_LOGD("	wake3_delay: %-8x\r\n", r->wake3_delay);
	SOC_LOGD("	halt1_delay: %-8x\r\n", r->halt1_delay);
	SOC_LOGD("	halt2_delay: %-8x\r\n", r->halt2_delay);
	SOC_LOGD("	halt3_delay: %-8x\r\n", r->halt3_delay);
	SOC_LOGD("	halt_volt: %-8x\r\n", r->halt_volt);
	SOC_LOGD("	halt_xtal: %-8x\r\n", r->halt_xtal);
	SOC_LOGD("	halt_core: %-8x\r\n", r->halt_core);
	SOC_LOGD("	halt_flash: %-8x\r\n", r->halt_flash);
	SOC_LOGD("	halt_rosc: %-8x\r\n", r->halt_rosc);
	SOC_LOGD("	halt_resten: %-8x\r\n", r->halt_resten);
	SOC_LOGD("	halt_isolat: %-8x\r\n", r->halt_isolat);
	SOC_LOGD("	halt_clkena: %-8x\r\n", r->halt_clkena);
}

static void aon_pmu_dump_r41(void)
{
	aon_pmu_r41_t *r = (aon_pmu_r41_t *)(SOC_AON_PMU_REG_BASE + (0x41 << 2));

	SOC_LOGD("r41: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x41 << 2)));
	SOC_LOGD("	lpo_config: %-8x\r\n", r->lpo_config);
	SOC_LOGD("	flshsck_iocap: %-8x\r\n", r->flshsck_iocap);
	SOC_LOGD("	wakeup_ena: %-8x\r\n", r->wakeup_ena);
	SOC_LOGD("	io_drv: %-8x\r\n", r->io_drv);
	SOC_LOGD("	reserved_bit_12_13: %-8x\r\n", r->reserved_bit_12_13);
	SOC_LOGD("	xtal_sel: %-8x\r\n", r->xtal_sel);
	SOC_LOGD("	reserved_bit_15_23: %-8x\r\n", r->reserved_bit_15_23);
	SOC_LOGD("	halt_lpo: %-8x\r\n", r->halt_lpo);
	SOC_LOGD("	halt_busrst: %-8x\r\n", r->halt_busrst);
	SOC_LOGD("	halt_busiso: %-8x\r\n", r->halt_busiso);
	SOC_LOGD("	halt_buspwd: %-8x\r\n", r->halt_buspwd);
	SOC_LOGD("	halt_blpiso: %-8x\r\n", r->halt_blpiso);
	SOC_LOGD("	halt_blppwd: %-8x\r\n", r->halt_blppwd);
	SOC_LOGD("	halt_wlpiso: %-8x\r\n", r->halt_wlpiso);
	SOC_LOGD("	halt_wlppwd: %-8x\r\n", r->halt_wlppwd);
}

static void aon_pmu_dump_rsv_42_42(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGD("rsv_42_42: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x42 + idx) << 2)));
	}
}

static void aon_pmu_dump_r43(void)
{
	aon_pmu_r43_t *r = (aon_pmu_r43_t *)(SOC_AON_PMU_REG_BASE + (0x43 << 2));

	SOC_LOGD("r43: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x43 << 2)));
	SOC_LOGD("	clr_int_touched: %-8x\r\n", r->clr_int_touched);
	SOC_LOGD("	clr_int_usbplug: %-8x\r\n", r->clr_int_usbplug);
	SOC_LOGD("	clr_wakeup: %-8x\r\n", r->clr_wakeup);
	SOC_LOGD("	reserved_bit_18_31: %-8x\r\n", r->reserved_bit_18_31);
}

static void aon_pmu_dump_rsv_44_6f(void)
{
	for (uint32_t idx = 0; idx < 44; idx++) {
		SOC_LOGD("rsv_44_6f: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x44 + idx) << 2)));
	}
}

static void aon_pmu_dump_r70(void)
{
	aon_pmu_r70_t *r = (aon_pmu_r70_t *)(SOC_AON_PMU_REG_BASE + (0x70 << 2));

	SOC_LOGD("r70: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x70 << 2)));
	SOC_LOGD("	int_touched: %-8x\r\n", r->int_touched);
	SOC_LOGD("	int_usbplug: %-8x\r\n", r->int_usbplug);
	SOC_LOGD("	reserved_bit_17_31: %-8x\r\n", r->reserved_bit_17_31);
}

static void aon_pmu_dump_r71(void)
{
	aon_pmu_r71_t *r = (aon_pmu_r71_t *)(SOC_AON_PMU_REG_BASE + (0x71 << 2));

	SOC_LOGD("r71: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x71 << 2)));
	SOC_LOGD("	touch_state: %-8x\r\n", r->touch_state);
	SOC_LOGD("	usbplug_state: %-8x\r\n", r->usbplug_state);
	SOC_LOGD("	reserved_bit_17_19: %-8x\r\n", r->reserved_bit_17_19);
	SOC_LOGD("	wakeup_source: %-8x\r\n", r->wakeup_source);
	SOC_LOGD("	reserved_bit_25_31: %-8x\r\n", r->reserved_bit_25_31);
}

static void aon_pmu_dump_rsv_72_7b(void)
{
	for (uint32_t idx = 0; idx < 10; idx++) {
		SOC_LOGD("rsv_72_7b: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x72 + idx) << 2)));
	}
}

static void aon_pmu_dump_r7c(void)
{
	SOC_LOGD("r7c: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7c << 2)));
}

static void aon_pmu_dump_r7d(void)
{
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t *)(SOC_AON_PMU_REG_BASE + (0x7d << 2));

	SOC_LOGD("r7d: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7d << 2)));
	SOC_LOGD("	lcal: %-8x\r\n", r->lcal);
	SOC_LOGD("	l: %-8x\r\n", r->l);
	SOC_LOGD("	adc_cal: %-8x\r\n", r->adc_cal);
	SOC_LOGD("	bgcal: %-8x\r\n", r->bgcal);
	SOC_LOGD("	pll_26m_unlock: %-8x\r\n", r->pll_26m_unlock);
	SOC_LOGD("	dpll_unlock: %-8x\r\n", r->dpll_unlock);
	SOC_LOGD("	h1: %-8x\r\n", r->h1);
}

static void aon_pmu_dump_r7e(void)
{
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t *)(SOC_AON_PMU_REG_BASE + (0x7e << 2));

	SOC_LOGD("r7e: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7e << 2)));
	SOC_LOGD("	cbcal: %-8x\r\n", r->cbcal);
	SOC_LOGD("	ad_state: %-8x\r\n", r->ad_state);
	SOC_LOGD("	td_states1: %-8x\r\n", r->td_states1);
	SOC_LOGD("	h2: %-8x\r\n", r->h2);
}

static void aon_pmu_dump_r7f(void)
{
	SOC_LOGD("r7f: %-8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7f << 2)));
}

static aon_pmu_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, aon_pmu_dump_r0},
	{0x1, 0x1, aon_pmu_dump_r1},
	{0x2, 0x2, aon_pmu_dump_r2},
	{0x3, 0x3, aon_pmu_dump_r3},
	{0x40, 0x40, aon_pmu_dump_r40},
	{0x41, 0x41, aon_pmu_dump_r41},
	{0x43, 0x43, aon_pmu_dump_r43},
	{0x70, 0x70, aon_pmu_dump_r70},
	{0x71, 0x71, aon_pmu_dump_r71},
	{0x7c, 0x7c, aon_pmu_dump_r7c},
	{0x7d, 0x7d, aon_pmu_dump_r7d},
	{0x7e, 0x7e, aon_pmu_dump_r7e},
	{0x7f, 0x7f, aon_pmu_dump_r7f},
	{-1, -1, 0}
};

void aon_pmu_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
