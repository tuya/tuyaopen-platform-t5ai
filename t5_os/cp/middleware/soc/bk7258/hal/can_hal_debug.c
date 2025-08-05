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
#include "can_hw.h"
// #include "can_hal.h"

typedef void (*can_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	can_dump_fn_t fn;
} can_reg_fn_map_t;

static void can_dump_rid_esi(void)
{
	can_id_esi_t *r = (can_id_esi_t *)(SOC_CAN_REG_BASE + (0x0 << 2));

	SOC_LOGD("rid_esi: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x0 << 2)));
	SOC_LOGD("	id: %8x\r\n", r->id);
	SOC_LOGD("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGD("	esi: %8x\r\n", r->esi);
}

static void can_dump_rbuf_ctrl(void)
{
	can_buf_ctrl_t *r = (can_buf_ctrl_t *)(SOC_CAN_REG_BASE + (0x1 << 2));

	SOC_LOGD("rbuf_ctrl: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x1 << 2)));
	SOC_LOGD("	dlc: %8x\r\n", r->dlc);
	SOC_LOGD("	brs: %8x\r\n", r->brs);
	SOC_LOGD("	fdf: %8x\r\n", r->fdf);
	SOC_LOGD("	rtr: %8x\r\n", r->rtr);
	SOC_LOGD("	ide: %8x\r\n", r->ide);
	SOC_LOGD("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void can_dump_rdata(void)
{
	for (uint32_t idx = 0; idx < 16; idx++) {
		SOC_LOGD("rdata: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + ((0x2 + idx) << 2)));
	}
}

static void can_dump_tid_esi(void)
{
	can_id_esi_t *r = (can_id_esi_t *)(SOC_CAN_REG_BASE + (0x14 << 2));

	SOC_LOGD("tid_esi: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x14 << 2)));
	SOC_LOGD("	tid: %8x\r\n", r->id);
	SOC_LOGD("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGD("	esi: %8x\r\n", r->esi);
}

static void can_dump_tbuf_ctrl(void)
{
	can_buf_ctrl_t *r = (can_buf_ctrl_t *)(SOC_CAN_REG_BASE + (0x15 << 2));

	SOC_LOGD("tbuf_ctrl: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x15 << 2)));
	SOC_LOGD("	dlc: %8x\r\n", r->dlc);
	SOC_LOGD("	brs: %8x\r\n", r->brs);
	SOC_LOGD("	fdf: %8x\r\n", r->fdf);
	SOC_LOGD("	rtr: %8x\r\n", r->rtr);
	SOC_LOGD("	ide: %8x\r\n", r->ide);
	SOC_LOGD("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void can_dump_tdata(void)
{
	for (uint32_t idx = 0; idx < 16; idx++) {
		SOC_LOGD("tdata: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + ((0x16 + idx) << 2)));
	}
}

static void can_dump_tts_l(void)
{
	SOC_LOGD("tts_l: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x26 << 2)));
}

static void can_dump_tts_h(void)
{
	SOC_LOGD("tts_H: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x27 << 2)));
}

static void can_dump_cfg(void)
{
	can_cfg_t *r = (can_cfg_t *)(SOC_CAN_REG_BASE + (0x28 << 2));
	SOC_LOGD("cfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x28 << 2)));
	SOC_LOGD("	busoff : %8x\r\n", r->busoff );
	SOC_LOGD("	tactive : %8x\r\n", r->tactive );
	SOC_LOGD("	ractive : %8x\r\n", r->ractive );
	SOC_LOGD("	tsss : %8x\r\n", r->tsss );
	SOC_LOGD("	tpss : %8x\r\n", r->tpss );
	SOC_LOGD("	lbmi: %8x\r\n", r->lbmi);
	SOC_LOGD("	lbme: %8x\r\n", r->lbme);
	SOC_LOGD("	reset: %8x\r\n", r->reset);
	SOC_LOGD("	tsa : %8x\r\n", r->tsa );
	SOC_LOGD("	tsall : %8x\r\n", r->tsall );
	SOC_LOGD("	tsone : %8x\r\n", r->tsone );
	SOC_LOGD("	tpa : %8x\r\n", r->tpa );
	SOC_LOGD("	tpe : %8x\r\n", r->tpe );
	SOC_LOGD("	stby : %8x\r\n", r->stby );
	SOC_LOGD("	lom : %8x\r\n", r->lom );
	SOC_LOGD("	tbsel : %8x\r\n", r->tbsel );
	SOC_LOGD("	tsstat : %8x\r\n", r->tsstat );
	SOC_LOGD("	reserved_18_19: %8x\r\n", r->reserved_18_19);
	SOC_LOGD("	tttbm : %8x\r\n", r->tttbm );
	SOC_LOGD("	tsmode : %8x\r\n", r->tsmode );
	SOC_LOGD("	tsnext : %8x\r\n", r->tsnext );
	SOC_LOGD("	fd_iso : %8x\r\n", r->fd_iso );
	SOC_LOGD("	rstat : %8x\r\n", r->rstat );
	SOC_LOGD("	reserved_26_26: %8x\r\n", r->reserved_26_26);
	SOC_LOGD("	rball : %8x\r\n", r->rball );
	SOC_LOGD("	rrel : %8x\r\n", r->rrel );
	SOC_LOGD("	rov : %8x\r\n", r->rov );
	SOC_LOGD("	rom : %8x\r\n", r->rom );
	SOC_LOGD("	sack : %8x\r\n", r->sack );
}


static void can_dump_ie(void)
{
	can_ie_t *r = (can_ie_t *)(SOC_CAN_REG_BASE + (0x29 << 2));

	SOC_LOGD("ie: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x29 << 2)));
	SOC_LOGD("	tsff : %8x\r\n", r->tsff );
	SOC_LOGD("	eie : %8x\r\n", r->eie );
	SOC_LOGD("	tsie : %8x\r\n", r->tsie );
	SOC_LOGD("	tpie : %8x\r\n", r->tpie );
	SOC_LOGD("	rafie : %8x\r\n", r->rafie );
	SOC_LOGD("	rfie : %8x\r\n", r->rfie );
	SOC_LOGD("	roie : %8x\r\n", r->roie );
	SOC_LOGD("	rie : %8x\r\n", r->rie );
	SOC_LOGD("	aif : %8x\r\n", r->aif );
	SOC_LOGD("	eif : %8x\r\n", r->eif );
	SOC_LOGD("	tsif : %8x\r\n", r->tsif );
	SOC_LOGD("	tpif : %8x\r\n", r->tpif );
	SOC_LOGD("	rafif : %8x\r\n", r->rafif );
	SOC_LOGD("	rfif : %8x\r\n", r->rfif );
	SOC_LOGD("	roif : %8x\r\n", r->roif );
	SOC_LOGD("	rif : %8x\r\n", r->rif );
	SOC_LOGD("	beif : %8x\r\n", r->beif );
	SOC_LOGD("	beie : %8x\r\n", r->beie );
	SOC_LOGD("	alif : %8x\r\n", r->alif );
	SOC_LOGD("	alie : %8x\r\n", r->alie );
	SOC_LOGD("	epif : %8x\r\n", r->epif );
	SOC_LOGD("	epie : %8x\r\n", r->epie );
	SOC_LOGD("	epass : %8x\r\n", r->epass );
	SOC_LOGD("	ewarn : %8x\r\n", r->ewarn );
	SOC_LOGD("	ewl: %8x\r\n", r->ewl);
	SOC_LOGD("	afwl: %8x\r\n", r->afwl);
}


static void can_dump_sseg(void)
{
	can_sseg_t *r = (can_sseg_t *)(SOC_CAN_REG_BASE + (0x2a << 2));

	SOC_LOGD("sseg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2a << 2)));
	SOC_LOGD("	s_seg_1: %8x\r\n", r->s_seg_1);
	SOC_LOGD("	s_seg_2: %8x\r\n", r->s_seg_2);
	SOC_LOGD("	reserved_15_15: %8x\r\n", r->reserved_15_15);
	SOC_LOGD("	s_sjw: %8x\r\n", r->s_sjw);
	SOC_LOGD("	reserved_23_23: %8x\r\n", r->reserved_23_23);
	SOC_LOGD("	s_presc: %8x\r\n", r->s_presc);
}

static void can_dump_fseg(void)
{
	can_fseg_t *r = (can_fseg_t *)(SOC_CAN_REG_BASE + (0x2b << 2));

	SOC_LOGD("fseg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2b << 2)));
	SOC_LOGD("	f_seg_1: %8x\r\n", r->f_seg_1);
	SOC_LOGD("	reserved_5_7: %8x\r\n", r->reserved_5_7);
	SOC_LOGD("	f_seg_2: %8x\r\n", r->f_seg_2);
	SOC_LOGD("	reserved_12_15: %8x\r\n", r->reserved_12_15);
	SOC_LOGD("	f_sjw: %8x\r\n", r->f_sjw);
	SOC_LOGD("	reserved_20_23: %8x\r\n", r->reserved_20_23);
	SOC_LOGD("	_presc: %8x\r\n", r->f_presc);
}

static void can_dump_cap(void)
{
	can_cap_t *r = (can_cap_t *)(SOC_CAN_REG_BASE + (0x2c << 2));

	SOC_LOGD("ealcap: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2c << 2)));
	SOC_LOGD("	alc: %8x\r\n", r->alc);
	SOC_LOGD("	koer: %8x\r\n", r->koer);
	SOC_LOGD("	sspoff: %8x\r\n", r->sspoff);
	SOC_LOGD("	tdcen: %8x\r\n", r->tdcen);
	SOC_LOGD("	recnt: %8x\r\n", r->recnt);
	SOC_LOGD("	tecnt: %8x\r\n", r->tecnt);
}

static void can_dump_acf(void)
{
	can_acf_t *r = (can_acf_t *)(SOC_CAN_REG_BASE + (0x2d << 2));

	SOC_LOGD("acf: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2d << 2)));
	SOC_LOGD("	acfadr : %8x\r\n", r->acfadr );
	SOC_LOGD("	reserved_4_4: %8x\r\n", r->reserved_4_4);
	SOC_LOGD("	selmask : %8x\r\n", r->selmask );
	SOC_LOGD("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGD("	timeen: %8x\r\n", r->timeen);
	SOC_LOGD("	timepos: %8x\r\n", r->timepos);
	SOC_LOGD("	reserved_10_15: %8x\r\n", r->reserved_10_15);
	SOC_LOGD("	ae_x: %8x\r\n", r->acf_en);
}



static void can_dump_aid(void)
{
	can_aid_t *r = (can_aid_t *)(SOC_CAN_REG_BASE + (0x2e << 2));

	SOC_LOGD("aid: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2e << 2)));
	SOC_LOGD("	acode_or_amask: %8x\r\n", r->acode_or_amask);
	SOC_LOGD("	aide: %8x\r\n", r->aide);
	SOC_LOGD("	aidee: %8x\r\n", r->aidee);
	SOC_LOGD("	reserved_31_31: %8x\r\n", r->reserved_31_31);
}

static void can_dump_ttcfg(void)
{
	can_ttcfg_t *r = (can_ttcfg_t *)(SOC_CAN_REG_BASE + (0x2f << 2));

	SOC_LOGD("ttcfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2f << 2)));
	SOC_LOGD("	ver_0: %8x\r\n", r->ver_0);
	SOC_LOGD("	ver_1: %8x\r\n", r->ver_1);
	SOC_LOGD("	tbptr : %8x\r\n", r->tbptr );
	SOC_LOGD("	tbf : %8x\r\n", r->tbf );
	SOC_LOGD("	tbe : %8x\r\n", r->tbe );
	SOC_LOGD("	tten : %8x\r\n", r->tten );
	SOC_LOGD("	t_presc : %8x\r\n", r->t_presc );
	SOC_LOGD("	ttif : %8x\r\n", r->ttif );
	SOC_LOGD("	ttie : %8x\r\n", r->ttie );
	SOC_LOGD("	teif : %8x\r\n", r->teif );
	SOC_LOGD("	wtif : %8x\r\n", r->wtif );
	SOC_LOGD("	wtie : %8x\r\n", r->wtie );
}

static void can_dump_ref_msg(void)
{
	can_ref_msg_t *r = (can_ref_msg_t *)(SOC_CAN_REG_BASE + (0x30 << 2));

	SOC_LOGD("ref_msg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x30 << 2)));
	SOC_LOGD("	ref_id: %8x\r\n", r->ref_id);
	SOC_LOGD("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGD("	ref_ide: %8x\r\n", r->ref_ide);
}

static void can_dump_trig_cfg(void)
{
	can_trig_cfg_t *r = (can_trig_cfg_t *)(SOC_CAN_REG_BASE + (0x31 << 2));

	SOC_LOGD("trig_cfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x31 << 2)));
	SOC_LOGD("	ttptr : %8x\r\n", r->ttptr );
	SOC_LOGD("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGD("	ttype: %8x\r\n", r->ttype);
	SOC_LOGD("	reserved_11_11: %8x\r\n", r->reserved_11_11);
	SOC_LOGD("	tew: %8x\r\n", r->tew);
	SOC_LOGD("	tttrig : %8x\r\n", r->tttrig );
}

static void can_dump_mem_stat(void)
{
	can_mem_stat_t *r = (can_mem_stat_t *)(SOC_CAN_REG_BASE + (0x32 << 2));

	SOC_LOGD("mem_stat: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x32 << 2)));
	SOC_LOGD("	tt_wtrig: %8x\r\n", r->ttwtrig);
	SOC_LOGD("	mpen : %8x\r\n", r->mpen );
	SOC_LOGD("	mdwie : %8x\r\n", r->mdwie );
	SOC_LOGD("	mdwif : %8x\r\n", r->mdwif );
	SOC_LOGD("	mdeif : %8x\r\n", r->mdeif );
	SOC_LOGD("	maeif : %8x\r\n", r->maeif );
	SOC_LOGD("	reserved_21_23: %8x\r\n", r->reserved_21_23);
	SOC_LOGD("	acfa : %8x\r\n", r->acfa );
	SOC_LOGD("	txs : %8x\r\n", r->txs );
	SOC_LOGD("	txb : %8x\r\n", r->txb );
	SOC_LOGD("	heloc : %8x\r\n", r->heloc );
	SOC_LOGD("	reserved_29_31: %8x\r\n", r->reserved_29_31);
}

static void can_dump_mem_es(void)
{
	can_mem_es_t *r = (can_mem_es_t *)(SOC_CAN_REG_BASE + (0x33 << 2));

	SOC_LOGD("mem_es: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x33 << 2)));
	SOC_LOGD("	mebp1 : %8x\r\n", r->mebp1 );
	SOC_LOGD("	me1ee : %8x\r\n", r->me1ee );
	SOC_LOGD("	meaee : %8x\r\n", r->meaee );
	SOC_LOGD("	mebp2 : %8x\r\n", r->mebp2 );
	SOC_LOGD("	me2ee : %8x\r\n", r->me2ee );
	SOC_LOGD("	reserved_15_15: %8x\r\n", r->reserved_15_15);
	SOC_LOGD("	meeec : %8x\r\n", r->meeec );
	SOC_LOGD("	menec : %8x\r\n", r->menec );
	SOC_LOGD("	mel : %8x\r\n", r->mel );
	SOC_LOGD("	mes : %8x\r\n", r->mes );
	SOC_LOGD("	reserved_27_31: %8x\r\n", r->reserved_27_31);
}

static void can_dump_scfg(void)
{
	can_scfg_t *r = (can_scfg_t *)(SOC_CAN_REG_BASE + (0x34 << 2));

	SOC_LOGD("scfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x34 << 2)));
	SOC_LOGD("	xmren : %8x\r\n", r->xmren );
	SOC_LOGD("	seif : %8x\r\n", r->seif );
	SOC_LOGD("	swie : %8x\r\n", r->swie );
	SOC_LOGD("	swif : %8x\r\n", r->swif );
	SOC_LOGD("	fstim : %8x\r\n", r->fstim );
	SOC_LOGD("	reserved_7_31: %8x\r\n", r->reserved_7_31);
}

static void can_dump_fd(void)
{
	can_fd_t *r = (can_fd_t *)(SOC_CAN_REG_BASE + (0x200 << 2));

	SOC_LOGD("fd: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x200 << 2)));
	SOC_LOGD("	can_fd_enable: %8x\r\n", r->can_fd_enable);
	SOC_LOGD("	reserved_1_31: %8x\r\n", r->reserved_1_31);
}

static can_reg_fn_map_t s_can_fn[] =
{
	{0x0, 0x0, can_dump_rid_esi},
	{0x1, 0x1, can_dump_rbuf_ctrl},
	{0x2, 0x11, can_dump_rdata},
	{0x14, 0x14, can_dump_tid_esi},
	{0x15, 0x15, can_dump_tbuf_ctrl},
	{0x16, 0x25, can_dump_tdata},
	{0x26, 0x26, can_dump_tts_l},
	{0x27, 0x27, can_dump_tts_h},
	{0x28, 0x28, can_dump_cfg},
	{0x29, 0x29, can_dump_ie},
	{0x2a, 0x2a, can_dump_sseg},
	{0x2b, 0x2b, can_dump_fseg},
	{0x2c, 0x2c, can_dump_cap},
	{0x2d, 0x2d, can_dump_acf},
	{0x2e, 0x2e, can_dump_aid},
	{0x2f, 0x2f, can_dump_ttcfg},
	{0x30, 0x30, can_dump_ref_msg},
	{0x31, 0x31, can_dump_trig_cfg},
	{0x32, 0x32, can_dump_mem_stat},
	{0x33, 0x33, can_dump_mem_es},
	{0x34, 0x34, can_dump_scfg},
	{0x200, 0x200, can_dump_fd},
	{-1, -1, 0}
};

void can_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_can_fn)/sizeof(s_can_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_can_fn[idx].start) && (end >= s_can_fn[idx].end)) {
			s_can_fn[idx].fn();
		}
	}
}
