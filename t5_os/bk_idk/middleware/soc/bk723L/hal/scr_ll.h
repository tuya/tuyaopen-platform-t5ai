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

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "scr_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCR_LL_REG_BASE   SOC_SCR_REG_BASE

//reg DeviceID:

static inline void scr_ll_set_DeviceID_value(uint32_t v) {
	scr_DeviceID_t *r = (scr_DeviceID_t*)(SOC_SCR_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_DeviceID_value(void) {
	scr_DeviceID_t *r = (scr_DeviceID_t*)(SOC_SCR_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t scr_ll_get_DeviceID_deviceid(void) {
	scr_DeviceID_t *r = (scr_DeviceID_t*)(SOC_SCR_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg VersionID:

static inline void scr_ll_set_VersionID_value(uint32_t v) {
	scr_VersionID_t *r = (scr_VersionID_t*)(SOC_SCR_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_VersionID_value(void) {
	scr_VersionID_t *r = (scr_VersionID_t*)(SOC_SCR_REG_BASE + (0x1 << 2));
	return r->v;
}

//reg Global_CTRL:

static inline void scr_ll_set_Global_CTRL_value(uint32_t v) {
	scr_Global_CTRL_t *r = (scr_Global_CTRL_t*)(SOC_SCR_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Global_CTRL_value(void) {
	scr_Global_CTRL_t *r = (scr_Global_CTRL_t*)(SOC_SCR_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void scr_ll_set_Global_CTRL_soft_rst(uint32_t v) {
	scr_Global_CTRL_t *r = (scr_Global_CTRL_t*)(SOC_SCR_REG_BASE + (0x2 << 2));
	r->soft_rst = v;
}

static inline void scr_ll_set_Global_CTRL_bypass_ckg(uint32_t v) {
	scr_Global_CTRL_t *r = (scr_Global_CTRL_t*)(SOC_SCR_REG_BASE + (0x2 << 2));
	r->bypass_ckg = v;
}

//reg Global_Status:

static inline void scr_ll_set_Global_Status_value(uint32_t v) {
	scr_Global_Status_t *r = (scr_Global_Status_t*)(SOC_SCR_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Global_Status_value(void) {
	scr_Global_Status_t *r = (scr_Global_Status_t*)(SOC_SCR_REG_BASE + (0x3 << 2));
	return r->v;
}

//reg CTRL1:

static inline void scr_ll_set_CTRL1_value(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_CTRL1_value(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void scr_ll_set_CTRL1_invlev(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->invlev = v;
}

static inline uint32_t scr_ll_get_CTRL1_invlev(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->invlev;
}

static inline void scr_ll_set_CTRL1_invord(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->invord = v;
}

static inline uint32_t scr_ll_get_CTRL1_invord(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->invord;
}

static inline void scr_ll_set_CTRL1_pech2fifo(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->pech2fifo = v;
}

static inline uint32_t scr_ll_get_CTRL1_pech2fifo(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->pech2fifo;
}

static inline void scr_ll_set_CTRL1_reserved_3_5(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->reserved_3_5 = v;
}

static inline uint32_t scr_ll_get_CTRL1_reserved_3_5(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->reserved_3_5;
}

static inline void scr_ll_set_CTRL1_clkstop(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->clkstop = v;
}

static inline uint32_t scr_ll_get_CTRL1_clkstop(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->clkstop;
}

static inline void scr_ll_set_CTRL1_clkstopval(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->clkstopval = v;
}

static inline uint32_t scr_ll_get_CTRL1_clkstopval(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->clkstopval;
}

static inline void scr_ll_set_CTRL1_txen(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->txen = v;
}

static inline uint32_t scr_ll_get_CTRL1_txen(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->txen;
}

static inline void scr_ll_set_CTRL1_rxen(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->rxen = v;
}

static inline uint32_t scr_ll_get_CTRL1_rxen(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->rxen;
}

static inline void scr_ll_set_CTRL1_ts2fifo(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->ts2fifo = v;
}

static inline uint32_t scr_ll_get_CTRL1_ts2fifo(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->ts2fifo;
}

static inline void scr_ll_set_CTRL1_t0t1(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->t0t1 = v;
}

static inline uint32_t scr_ll_get_CTRL1_t0t1(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->t0t1;
}

static inline void scr_ll_set_CTRL1_atrstflush(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->atrstflush = v;
}

static inline uint32_t scr_ll_get_CTRL1_atrstflush(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->atrstflush;
}

static inline void scr_ll_set_CTRL1_tcken(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->tcken = v;
}

static inline uint32_t scr_ll_get_CTRL1_tcken(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->tcken;
}

static inline void scr_ll_set_CTRL1_reserved_14_14(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->reserved_14_14 = v;
}

static inline uint32_t scr_ll_get_CTRL1_reserved_14_14(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->reserved_14_14;
}

static inline void scr_ll_set_CTRL1_ginten(uint32_t v) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	r->ginten = v;
}

static inline uint32_t scr_ll_get_CTRL1_ginten(void) {
	scr_CTRL1_t *r = (scr_CTRL1_t*)(SOC_SCR_REG_BASE + (0x4 << 2));
	return r->ginten;
}

//reg CTRL2:

static inline void scr_ll_set_CTRL2_value(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_CTRL2_value(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void scr_ll_set_CTRL2_reserved_0_0(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->reserved_0_0 = v;
}

static inline uint32_t scr_ll_get_CTRL2_reserved_0_0(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->reserved_0_0;
}

static inline void scr_ll_set_CTRL2_reserved_1_1(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->reserved_1_1 = v;
}

static inline uint32_t scr_ll_get_CTRL2_reserved_1_1(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->reserved_1_1;
}

static inline void scr_ll_set_CTRL2_warmrst(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->warmrst = v;
}

static inline uint32_t scr_ll_get_CTRL2_warmrst(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->warmrst;
}

static inline void scr_ll_set_CTRL2_act(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->act = v;
}

static inline uint32_t scr_ll_get_CTRL2_act(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->act;
}

static inline void scr_ll_set_CTRL2_deact(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->deact = v;
}

static inline uint32_t scr_ll_get_CTRL2_deact(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->deact;
}

static inline uint32_t scr_ll_get_CTRL2_vcc18(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->vcc18;
}

static inline void scr_ll_set_CTRL2_vcc30(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->vcc30 = v;
}

static inline uint32_t scr_ll_get_CTRL2_vcc30(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->vcc30;
}

static inline uint32_t scr_ll_get_CTRL2_vcc50(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->vcc50;
}

static inline void scr_ll_set_CTRL2_act_fast(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->act_fast = v;
}

static inline uint32_t scr_ll_get_CTRL2_act_fast(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->act_fast;
}

static inline void scr_ll_set_CTRL2_reserved_9_15(uint32_t v) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	r->reserved_9_15 = v;
}

static inline uint32_t scr_ll_get_CTRL2_reserved_9_15(void) {
	scr_CTRL2_t *r = (scr_CTRL2_t*)(SOC_SCR_REG_BASE + (0x5 << 2));
	return r->reserved_9_15;
}

//reg SCPADS:

static inline void scr_ll_set_SCPADS_value(uint32_t v) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_SCPADS_value(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline uint32_t scr_ll_get_SCPADS_diraccpads(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->diraccpads;
}

static inline uint32_t scr_ll_get_SCPADS_dscio(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscio;
}

static inline uint32_t scr_ll_get_SCPADS_dscclk(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscclk;
}

static inline uint32_t scr_ll_get_SCPADS_dscrst(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscrst;
}

static inline uint32_t scr_ll_get_SCPADS_dscvcc(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscvcc;
}

static inline uint32_t scr_ll_get_SCPADS_autoadeavpp(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->autoadeavpp;
}

static inline uint32_t scr_ll_get_SCPADS_dscvppen(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscvppen;
}

static inline uint32_t scr_ll_get_SCPADS_dscvpppp(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscvpppp;
}

static inline uint32_t scr_ll_get_SCPADS_dscfcb(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->dscfcb;
}

static inline uint32_t scr_ll_get_SCPADS_scpresent(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->scpresent;
}

static inline void scr_ll_set_SCPADS_reserved_10_15(uint32_t v) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	r->reserved_10_15 = v;
}

static inline uint32_t scr_ll_get_SCPADS_reserved_10_15(void) {
	scr_SCPADS_t *r = (scr_SCPADS_t*)(SOC_SCR_REG_BASE + (0x6 << 2));
	return r->reserved_10_15;
}

//reg INTEN1:

static inline void scr_ll_set_INTEN1_value(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_INTEN1_value(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline void scr_ll_set_INTEN1_txfidone(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->txfidone = v;
}

static inline uint32_t scr_ll_get_INTEN1_txfidone(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->txfidone;
}

static inline void scr_ll_set_INTEN1_txfiempty(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->txfiempty = v;
}

static inline uint32_t scr_ll_get_INTEN1_txfiempty(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->txfiempty;
}

static inline void scr_ll_set_INTEN1_rxfifull(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->rxfifull = v;
}

static inline uint32_t scr_ll_get_INTEN1_rxfifull(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->rxfifull;
}

static inline void scr_ll_set_INTEN1_clkstoprun(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->clkstoprun = v;
}

static inline uint32_t scr_ll_get_INTEN1_clkstoprun(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->clkstoprun;
}

static inline void scr_ll_set_INTEN1_txdone(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->txdone = v;
}

static inline uint32_t scr_ll_get_INTEN1_txdone(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->txdone;
}

static inline void scr_ll_set_INTEN1_rxdone(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->rxdone = v;
}

static inline uint32_t scr_ll_get_INTEN1_rxdone(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->rxdone;
}

static inline void scr_ll_set_INTEN1_txperr(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->txperr = v;
}

static inline uint32_t scr_ll_get_INTEN1_txperr(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->txperr;
}

static inline void scr_ll_set_INTEN1_rxperr(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->rxperr = v;
}

static inline uint32_t scr_ll_get_INTEN1_rxperr(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->rxperr;
}

static inline void scr_ll_set_INTEN1_c2cfull(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->c2cfull = v;
}

static inline uint32_t scr_ll_get_INTEN1_c2cfull(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->c2cfull;
}

static inline void scr_ll_set_INTEN1_rxthreshold(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->rxthreshold = v;
}

static inline uint32_t scr_ll_get_INTEN1_rxthreshold(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->rxthreshold;
}

static inline void scr_ll_set_INTEN1_atrfail(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->atrfail = v;
}

static inline uint32_t scr_ll_get_INTEN1_atrfail(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->atrfail;
}

static inline void scr_ll_set_INTEN1_atrdone(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->atrdone = v;
}

static inline uint32_t scr_ll_get_INTEN1_atrdone(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->atrdone;
}

static inline void scr_ll_set_INTEN1_screm(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->screm = v;
}

static inline uint32_t scr_ll_get_INTEN1_screm(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->screm;
}

static inline void scr_ll_set_INTEN1_scins(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->scins = v;
}

static inline uint32_t scr_ll_get_INTEN1_scins(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->scins;
}

static inline void scr_ll_set_INTEN1_scact(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->scact = v;
}

static inline uint32_t scr_ll_get_INTEN1_scact(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->scact;
}

static inline void scr_ll_set_INTEN1_scdeact(uint32_t v) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	r->scdeact = v;
}

static inline uint32_t scr_ll_get_INTEN1_scdeact(void) {
	scr_INTEN1_t *r = (scr_INTEN1_t*)(SOC_SCR_REG_BASE + (0x7 << 2));
	return r->scdeact;
}

//reg INTSTAT1:

static inline void scr_ll_set_INTSTAT1_value(uint32_t v) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_INTSTAT1_value(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void scr_ll_set_INTSTAT1_txfidone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_TXFIDONE_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_txfidone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->txfidone;
}

static inline void scr_ll_set_INTSTAT1_txfiempty(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_TXFIEMPTY_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_txfiempty(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->txfiempty;
}

static inline void scr_ll_set_INTSTAT1_rxfifull(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_RXFIFULL_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_rxfifull(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->rxfifull;
}

static inline void scr_ll_set_INTSTAT1_clkstoprun(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_CLKSTOPRUN_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_clkstoprun(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->clkstoprun;
}

static inline void scr_ll_set_INTSTAT1_txdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_TXDONE_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_txdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->txdone;
}

static inline void scr_ll_set_INTSTAT1_rxdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_RXDONE_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_rxdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->rxdone;
}

static inline void scr_ll_set_INTSTAT1_txperr(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_TXPERR_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_txperr(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->txperr;
}

static inline void scr_ll_set_INTSTAT1_rxperr(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_RXPERR_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_rxperr(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->rxperr;
}

static inline void scr_ll_set_INTSTAT1_c2cfull(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_C2CFULL_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_c2cfull(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->c2cfull;
}

static inline void scr_ll_set_INTSTAT1_rxthreshold(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_RXTHRESHOLD_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_rxthreshold(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->rxthreshold;
}

static inline void scr_ll_set_INTSTAT1_atrfail(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_ATRFAIL_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_atrfail(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->atrfail;
}

static inline void scr_ll_set_INTSTAT1_atrdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_ATRDONE_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_atrdone(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->atrdone;
}

static inline uint32_t scr_ll_get_INTSTAT1_screm(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->screm;
}

static inline uint32_t scr_ll_get_INTSTAT1_scins(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->scins;
}

static inline void scr_ll_set_INTSTAT1_scact(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_SCACT_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_scact(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->scact;
}

static inline void scr_ll_set_INTSTAT1_scdeact(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	r->v = BIT(SCR_INTSTAT1_SCDEACT_POS);
}

static inline uint32_t scr_ll_get_INTSTAT1_scdeact(void) {
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t*)(SOC_SCR_REG_BASE + (0x8 << 2));
	return r->scdeact;
}

//reg FIFOCTRL:

static inline void scr_ll_set_FIFOCTRL_value(uint32_t v) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_value(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_txfiempty(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->txfiempty;
}

static inline uint32_t scr_ll_get_FIFOCTRL_txfifull(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->txfifull;
}

static inline void scr_ll_set_FIFOCTRL_txfiflush(uint32_t v) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	r->txfiflush = v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_txfiflush(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->txfiflush;
}

static inline void scr_ll_set_FIFOCTRL_reserved_3_7(uint32_t v) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	r->reserved_3_7 = v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_reserved_3_7(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->reserved_3_7;
}

static inline uint32_t scr_ll_get_FIFOCTRL_rxfiempty(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->rxfiempty;
}

static inline uint32_t scr_ll_get_FIFOCTRL_rxfifull(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->rxfifull;
}

static inline void scr_ll_set_FIFOCTRL_rxfiflush(uint32_t v) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	r->rxfiflush = v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_rxfiflush(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->rxfiflush;
}

static inline void scr_ll_set_FIFOCTRL_reserved_11_15(uint32_t v) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	r->reserved_11_15 = v;
}

static inline uint32_t scr_ll_get_FIFOCTRL_reserved_11_15(void) {
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t*)(SOC_SCR_REG_BASE + (0x9 << 2));
	return r->reserved_11_15;
}

//reg Legacy_RX_FIFO_Counter:

static inline void scr_ll_set_Legacy_FIFO_Counter_value(uint32_t v) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Legacy_FIFO_Counter_value(void) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void scr_ll_set_Legacy_FIFO_Counter_legacy_rx_fifo_cnt(uint32_t v) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	r->legacy_rx_fifo_cnt = v;
}

static inline uint32_t scr_ll_get_Legacy_FIFO_Counter_legacy_rx_fifo_cnt(void) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	return r->legacy_rx_fifo_cnt;
}

static inline void scr_ll_set_Legacy_FIFO_Counter_legacy_tx_fifo_cnt(uint32_t v) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	r->legacy_tx_fifo_cnt = v;
}

static inline uint32_t scr_ll_get_Legacy_FIFO_Counter_legacy_tx_fifo_cnt(void) {
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0xa << 2));
	return r->legacy_tx_fifo_cnt;
}

//reg RX_FIFO_Threshold:

static inline void scr_ll_set_RX_FIFO_Threshold_value(uint32_t v) {
	scr_RX_FIFO_Threshold_t *r = (scr_RX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_RX_FIFO_Threshold_value(void) {
	scr_RX_FIFO_Threshold_t *r = (scr_RX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void scr_ll_set_RX_FIFO_Threshold_rx_fifo_thd(uint32_t v) {
	scr_RX_FIFO_Threshold_t *r = (scr_RX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0xb << 2));
	r->rx_fifo_thd = v;
}

static inline uint32_t scr_ll_get_RX_FIFO_Threshold_rx_fifo_thd(void) {
	scr_RX_FIFO_Threshold_t *r = (scr_RX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0xb << 2));
	return r->rx_fifo_thd;
}

//reg TX_Repeat:

static inline void scr_ll_set_TX_Repeat_value(uint32_t v) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_TX_Repeat_value(void) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	return r->v;
}

static inline void scr_ll_set_TX_Repeat_tx_repeat(uint32_t v) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	r->tx_repeat = v;
}

static inline uint32_t scr_ll_get_TX_Repeat_tx_repeat(void) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	return r->tx_repeat;
}

static inline void scr_ll_set_TX_Repeat_rx_repeat(uint32_t v) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	r->rx_repeat = v;
}

static inline uint32_t scr_ll_get_TX_Repeat_rx_repeat(void) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	return r->rx_repeat;
}

static inline void scr_ll_set_TX_Repeat_reserved_8_15(uint32_t v) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	r->reserved_8_15 = v;
}

static inline uint32_t scr_ll_get_TX_Repeat_reserved_8_15(void) {
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t*)(SOC_SCR_REG_BASE + (0xc << 2));
	return r->reserved_8_15;
}

//reg Smart_Card_Clock_Divisor:

static inline void scr_ll_set_Smart_Card_Clock_Divisor_value(uint32_t v) {
	scr_Smart_Card_Clock_Divisor_t *r = (scr_Smart_Card_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Smart_Card_Clock_Divisor_value(void) {
	scr_Smart_Card_Clock_Divisor_t *r = (scr_Smart_Card_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xd << 2));
	return r->v;
}

static inline void scr_ll_set_Smart_Card_Clock_Divisor_smart_card_clk_div(uint32_t v) {
	scr_Smart_Card_Clock_Divisor_t *r = (scr_Smart_Card_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xd << 2));
	r->smart_card_clk_div = v;
}

static inline uint32_t scr_ll_get_Smart_Card_Clock_Divisor_smart_card_clk_div(void) {
	scr_Smart_Card_Clock_Divisor_t *r = (scr_Smart_Card_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xd << 2));
	return r->smart_card_clk_div;
}

//reg Baud_Clock_Divisor:

static inline void scr_ll_set_Baud_Clock_Divisor_value(uint32_t v) {
	scr_Baud_Clock_Divisor_t *r = (scr_Baud_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xe << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Baud_Clock_Divisor_value(void) {
	scr_Baud_Clock_Divisor_t *r = (scr_Baud_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xe << 2));
	return r->v;
}

static inline void scr_ll_set_Baud_Clock_Divisor_band_clk_div(uint32_t v) {
	scr_Baud_Clock_Divisor_t *r = (scr_Baud_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xe << 2));
	r->band_clk_div = v;
}

static inline uint32_t scr_ll_get_Baud_Clock_Divisor_band_clk_div(void) {
	scr_Baud_Clock_Divisor_t *r = (scr_Baud_Clock_Divisor_t*)(SOC_SCR_REG_BASE + (0xe << 2));
	return r->band_clk_div;
}

//reg Smart_Card_Guardtime:

static inline void scr_ll_set_Smart_Card_Guardtime_value(uint32_t v) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Smart_Card_Guardtime_value(void) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	return r->v;
}

static inline void scr_ll_set_Smart_Card_Guardtime_smart_card_guardtime(uint32_t v) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	r->smart_card_guardtime = v;
}

static inline uint32_t scr_ll_get_Smart_Card_Guardtime_smart_card_guardtime(void) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	return r->smart_card_guardtime;
}

static inline void scr_ll_set_Smart_Card_Guardtime_reserved_8_15(uint32_t v) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	r->reserved_8_15 = v;
}

static inline uint32_t scr_ll_get_Smart_Card_Guardtime_reserved_8_15(void) {
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t*)(SOC_SCR_REG_BASE + (0xf << 2));
	return r->reserved_8_15;
}

//reg Activation_Deactivation_Time:

static inline void scr_ll_set_Activation_Deactivation_Time_value(uint32_t v) {
	scr_Activation_Deactivation_Time_t *r = (scr_Activation_Deactivation_Time_t*)(SOC_SCR_REG_BASE + (0x10 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Activation_Deactivation_Time_value(void) {
	scr_Activation_Deactivation_Time_t *r = (scr_Activation_Deactivation_Time_t*)(SOC_SCR_REG_BASE + (0x10 << 2));
	return r->v;
}

static inline void scr_ll_set_Activation_Deactivation_Time_activation_time(uint32_t v) {
	scr_Activation_Deactivation_Time_t *r = (scr_Activation_Deactivation_Time_t*)(SOC_SCR_REG_BASE + (0x10 << 2));
	r->activation_time = v;
}

static inline uint32_t scr_ll_get_Activation_Deactivation_Time_activation_time(void) {
	scr_Activation_Deactivation_Time_t *r = (scr_Activation_Deactivation_Time_t*)(SOC_SCR_REG_BASE + (0x10 << 2));
	return r->activation_time;
}

//reg Reset_Duration:

static inline void scr_ll_set_Reset_Duration_value(uint32_t v) {
	scr_Reset_Duration_t *r = (scr_Reset_Duration_t*)(SOC_SCR_REG_BASE + (0x11 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Reset_Duration_value(void) {
	scr_Reset_Duration_t *r = (scr_Reset_Duration_t*)(SOC_SCR_REG_BASE + (0x11 << 2));
	return r->v;
}

static inline void scr_ll_set_Reset_Duration_reset_duration(uint32_t v) {
	scr_Reset_Duration_t *r = (scr_Reset_Duration_t*)(SOC_SCR_REG_BASE + (0x11 << 2));
	r->reset_duration = v;
}

static inline uint32_t scr_ll_get_Reset_Duration_reset_duration(void) {
	scr_Reset_Duration_t *r = (scr_Reset_Duration_t*)(SOC_SCR_REG_BASE + (0x11 << 2));
	return r->reset_duration;
}

//reg ATR_Start_Limit:

static inline void scr_ll_set_ATR_Start_Limit_value(uint32_t v) {
	scr_ATR_Start_Limit_t *r = (scr_ATR_Start_Limit_t*)(SOC_SCR_REG_BASE + (0x12 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_ATR_Start_Limit_value(void) {
	scr_ATR_Start_Limit_t *r = (scr_ATR_Start_Limit_t*)(SOC_SCR_REG_BASE + (0x12 << 2));
	return r->v;
}

static inline void scr_ll_set_ATR_Start_Limit_atr_start_limit(uint32_t v) {
	scr_ATR_Start_Limit_t *r = (scr_ATR_Start_Limit_t*)(SOC_SCR_REG_BASE + (0x12 << 2));
	r->atr_start_limit = v;
}

static inline uint32_t scr_ll_get_ATR_Start_Limit_atr_start_limit(void) {
	scr_ATR_Start_Limit_t *r = (scr_ATR_Start_Limit_t*)(SOC_SCR_REG_BASE + (0x12 << 2));
	return r->atr_start_limit;
}

//reg Two_Characters_Delay_Limit_L:

static inline void scr_ll_set_Two_Characters_Delay_Limit_L_value(uint32_t v) {
	scr_Two_Characters_Delay_Limit_L_t *r = (scr_Two_Characters_Delay_Limit_L_t*)(SOC_SCR_REG_BASE + (0x13 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Two_Characters_Delay_Limit_L_value(void) {
	scr_Two_Characters_Delay_Limit_L_t *r = (scr_Two_Characters_Delay_Limit_L_t*)(SOC_SCR_REG_BASE + (0x13 << 2));
	return r->v;
}

static inline void scr_ll_set_Two_Characters_Delay_Limit_L_two_char_delay_limit_l(uint32_t v) {
	scr_Two_Characters_Delay_Limit_L_t *r = (scr_Two_Characters_Delay_Limit_L_t*)(SOC_SCR_REG_BASE + (0x13 << 2));
	r->two_char_delay_limit_l = v;
}

static inline uint32_t scr_ll_get_Two_Characters_Delay_Limit_L_two_char_delay_limit_l(void) {
	scr_Two_Characters_Delay_Limit_L_t *r = (scr_Two_Characters_Delay_Limit_L_t*)(SOC_SCR_REG_BASE + (0x13 << 2));
	return r->two_char_delay_limit_l;
}

//reg INTEN2:

static inline void scr_ll_set_INTEN2_value(uint32_t v) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_INTEN2_value(void) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void scr_ll_set_INTEN2_txthreshold(uint32_t v) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	r->txthreshold = v;
}

static inline uint32_t scr_ll_get_INTEN2_txthreshold(void) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	return r->txthreshold;
}

static inline void scr_ll_set_INTEN2_tckerr(uint32_t v) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	r->tckerr = v;
}

static inline uint32_t scr_ll_get_INTEN2_tckerr(void) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	return r->tckerr;
}

static inline void scr_ll_set_INTEN2_reserved_2_15(uint32_t v) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	r->reserved_2_15 = v;
}

static inline uint32_t scr_ll_get_INTEN2_reserved_2_15(void) {
	scr_INTEN2_t *r = (scr_INTEN2_t*)(SOC_SCR_REG_BASE + (0x14 << 2));
	return r->reserved_2_15;
}

//reg INTSTAT2:

static inline void scr_ll_set_INTSTAT2_value(uint32_t v) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_INTSTAT2_value(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void scr_ll_set_INTSTAT2_txthreshold(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	r->v = BIT(SCR_INTSTAT2_TXTHRESHOLD_POS);
}

static inline uint32_t scr_ll_get_INTSTAT2_txthreshold(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	return r->txthreshold;
}

static inline void scr_ll_set_INTSTAT2_tckerr(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	r->v = BIT(SCR_INTSTAT2_TCKERR_POS);
}

static inline uint32_t scr_ll_get_INTSTAT2_tckerr(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	return r->tckerr;
}

static inline void scr_ll_set_INTSTAT2_reserved_2_15(uint32_t v) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	r->reserved_2_15 = v;
}

static inline uint32_t scr_ll_get_INTSTAT2_reserved_2_15(void) {
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t*)(SOC_SCR_REG_BASE + (0x15 << 2));
	return r->reserved_2_15;
}

//reg TX_FIFO_Threshold:

static inline void scr_ll_set_TX_FIFO_Threshold_value(uint32_t v) {
	scr_TX_FIFO_Threshold_t *r = (scr_TX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_TX_FIFO_Threshold_value(void) {
	scr_TX_FIFO_Threshold_t *r = (scr_TX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void scr_ll_set_TX_FIFO_Threshold_tx_fifo_thd(uint32_t v) {
	scr_TX_FIFO_Threshold_t *r = (scr_TX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0x16 << 2));
	r->tx_fifo_thd = v;
}

static inline uint32_t scr_ll_get_TX_FIFO_Threshold_tx_fifo_thd(void) {
	scr_TX_FIFO_Threshold_t *r = (scr_TX_FIFO_Threshold_t*)(SOC_SCR_REG_BASE + (0x16 << 2));
	return r->tx_fifo_thd;
}

//reg TX_FIFO_Counter:

static inline void scr_ll_set_TX_FIFO_Counter_value(uint32_t v) {
	scr_TX_FIFO_Counter_t *r = (scr_TX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_TX_FIFO_Counter_value(void) {
	scr_TX_FIFO_Counter_t *r = (scr_TX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline uint32_t scr_ll_get_TX_FIFO_Counter_tx_fifo_cnt(void) {
	scr_TX_FIFO_Counter_t *r = (scr_TX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x17 << 2));
	return r->tx_fifo_cnt;
}

//reg RX_FIFO_Counter:

static inline void scr_ll_set_RX_FIFO_Counter_value(uint32_t v) {
	scr_RX_FIFO_Counter_t *r = (scr_RX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_RX_FIFO_Counter_value(void) {
	scr_RX_FIFO_Counter_t *r = (scr_RX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline uint32_t scr_ll_get_RX_FIFO_Counter_rx_fifo_cnt(void) {
	scr_RX_FIFO_Counter_t *r = (scr_RX_FIFO_Counter_t*)(SOC_SCR_REG_BASE + (0x18 << 2));
	return r->rx_fifo_cnt;
}

//reg Baud_Tune_Register:

static inline void scr_ll_set_Baud_Tune_Register_value(uint32_t v) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Baud_Tune_Register_value(void) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline void scr_ll_set_Baud_Tune_Register_baud_tune_reg(uint32_t v) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	r->baud_tune_reg = v;
}

static inline uint32_t scr_ll_get_Baud_Tune_Register_baud_tune_reg(void) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	return r->baud_tune_reg;
}

static inline void scr_ll_set_Baud_Tune_Register_reserved_3_15(uint32_t v) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	r->reserved_3_15 = v;
}

static inline uint32_t scr_ll_get_Baud_Tune_Register_reserved_3_15(void) {
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t*)(SOC_SCR_REG_BASE + (0x19 << 2));
	return r->reserved_3_15;
}

//reg Two_Characters_Delay_Limit_H:

static inline void scr_ll_set_Two_Characters_Delay_Limit_H_value(uint32_t v) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_Two_Characters_Delay_Limit_H_value(void) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline void scr_ll_set_Two_Characters_Delay_Limit_H_two_char_delay_limit_h(uint32_t v) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	r->two_char_delay_limit_h = v;
}

static inline uint32_t scr_ll_get_Two_Characters_Delay_Limit_H_two_char_delay_limit_h(void) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	return r->two_char_delay_limit_h;
}

static inline void scr_ll_set_Two_Characters_Delay_Limit_H_reserved_8_15(uint32_t v) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	r->reserved_8_15 = v;
}

static inline uint32_t scr_ll_get_Two_Characters_Delay_Limit_H_reserved_8_15(void) {
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t*)(SOC_SCR_REG_BASE + (0x1a << 2));
	return r->reserved_8_15;
}

//reg FIFO:

static inline void scr_ll_set_FIFO_value(uint32_t v) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	r->v = v;
}

static inline uint32_t scr_ll_get_FIFO_value(void) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	return r->v;
}

static inline void scr_ll_set_FIFO_fifo(uint32_t v) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	r->fifo = v;
}

static inline uint32_t scr_ll_get_FIFO_fifo(void) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	return r->fifo;
}

static inline void scr_ll_set_FIFO_reserved_8_15(uint32_t v) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	r->reserved_8_15 = v;
}

static inline uint32_t scr_ll_get_FIFO_reserved_8_15(void) {
	scr_FIFO_t *r = (scr_FIFO_t*)(SOC_SCR_REG_BASE + (0x80 << 2));
	return r->reserved_8_15;
}
#ifdef __cplusplus
}
#endif
