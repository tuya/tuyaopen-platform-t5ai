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
#include "i2s_hw.h"
#include "i2s_hal.h"

typedef void (*i2s_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	i2s_dump_fn_t fn;
} i2s_reg_fn_map_t;

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x0 << 2)));
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x1 << 2)));
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x2 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x2 << 2)));
	SOC_LOGD("	soft_reset: %8x\r\n", r->soft_reset);
	SOC_LOGD("	clkg_bypass: %8x\r\n", r->clkg_bypass);
	SOC_LOGD("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x3 << 2)));
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x4 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x4 << 2)));
	SOC_LOGD("	bitratio: %8x\r\n", r->bitratio);
	SOC_LOGD("	smpratio: %8x\r\n", r->smpratio);
	SOC_LOGD("	pcm_dlen: %8x\r\n", r->pcm_dlen);
	SOC_LOGD("	datalen: %8x\r\n", r->datalen);
	SOC_LOGD("	synclen: %8x\r\n", r->synclen);
	SOC_LOGD("	lsbfirst: %8x\r\n", r->lsbfirst);
	SOC_LOGD("	sclkinv: %8x\r\n", r->sclkinv);
	SOC_LOGD("	lrckrp: %8x\r\n", r->lrckrp);
	SOC_LOGD("	modesel: %8x\r\n", r->modesel);
	SOC_LOGD("	msten: %8x\r\n", r->msten);
	SOC_LOGD("	i2spcmen: %8x\r\n", r->i2spcmen);
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x5 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x5 << 2)));
	SOC_LOGD("	rxint_en: %8x\r\n", r->rxint_en);
	SOC_LOGD("	txint_en: %8x\r\n", r->txint_en);
	SOC_LOGD("	rxovf_en: %8x\r\n", r->rxovf_en);
	SOC_LOGD("	txudf_en: %8x\r\n", r->txudf_en);
	SOC_LOGD("	rxint_level: %8x\r\n", r->rxint_level);
	SOC_LOGD("	txint_level: %8x\r\n", r->txint_level);
	SOC_LOGD("	txfifo_clr: %8x\r\n", r->txfifo_clr);
	SOC_LOGD("	rxfifo_clr: %8x\r\n", r->rxfifo_clr);
	SOC_LOGD("	smpratio_h2b: %8x\r\n", r->smpratio_h2b);
	SOC_LOGD("	bitratio_h4b: %8x\r\n", r->bitratio_h4b);
	SOC_LOGD("	lrcom_store: %8x\r\n", r->lrcom_store);
	SOC_LOGD("	parallel_en: %8x\r\n", r->parallel_en);
	SOC_LOGD("	reserved_18_31: %8x\r\n", r->reserved_18_31);
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x6 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x6 << 2)));
	SOC_LOGD("	rxint: %8x\r\n", r->rxint);
	SOC_LOGD("	txint: %8x\r\n", r->txint);
	SOC_LOGD("	rxovf: %8x\r\n", r->rxovf);
	SOC_LOGD("	txudf: %8x\r\n", r->txudf);
	SOC_LOGD("	rxfifo_rd_ready: %8x\r\n", r->rxfifo_rd_ready);
	SOC_LOGD("	txfifo_wr_ready: %8x\r\n", r->txfifo_wr_ready);
	SOC_LOGD("	reserved_6_31: %8x\r\n", r->reserved_6_31);
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x7 << 2)));
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x8 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x8 << 2)));
	SOC_LOGD("	rx2int_en: %8x\r\n", r->rx2int_en);
	SOC_LOGD("	tx2int_en: %8x\r\n", r->tx2int_en);
	SOC_LOGD("	rx2ovf_en: %8x\r\n", r->rx2ovf_en);
	SOC_LOGD("	tx2udf_en: %8x\r\n", r->tx2udf_en);
	SOC_LOGD("	rx3int_en: %8x\r\n", r->rx3int_en);
	SOC_LOGD("	tx3nt_en: %8x\r\n", r->tx3nt_en);
	SOC_LOGD("	rx3ovf_en: %8x\r\n", r->rx3ovf_en);
	SOC_LOGD("	tx3udf_en: %8x\r\n", r->tx3udf_en);
	SOC_LOGD("	rx4int_en: %8x\r\n", r->rx4int_en);
	SOC_LOGD("	tx4nt_en: %8x\r\n", r->tx4nt_en);
	SOC_LOGD("	rx4ovf_en: %8x\r\n", r->rx4ovf_en);
	SOC_LOGD("	tx4udf_en: %8x\r\n", r->tx4udf_en);
	SOC_LOGD("	reserved_12_31: %8x\r\n", r->reserved_12_31);
}

static void i2s_dump_(void)
{
	i2s__t *r = (i2s__t *)(SOC_I2S_REG_BASE + (0x9 << 2));

	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0x9 << 2)));
	SOC_LOGD("	rx2int: %8x\r\n", r->rx2int);
	SOC_LOGD("	tx2int: %8x\r\n", r->tx2int);
	SOC_LOGD("	rx2ovf: %8x\r\n", r->rx2ovf);
	SOC_LOGD("	tx2udf: %8x\r\n", r->tx2udf);
	SOC_LOGD("	rx3int: %8x\r\n", r->rx3int);
	SOC_LOGD("	tx3int: %8x\r\n", r->tx3int);
	SOC_LOGD("	rx3ovf: %8x\r\n", r->rx3ovf);
	SOC_LOGD("	tx3udf: %8x\r\n", r->tx3udf);
	SOC_LOGD("	rx4int: %8x\r\n", r->rx4int);
	SOC_LOGD("	tx4int: %8x\r\n", r->tx4int);
	SOC_LOGD("	rx4ovf: %8x\r\n", r->rx4ovf);
	SOC_LOGD("	tx4udf: %8x\r\n", r->tx4udf);
	SOC_LOGD("	reserved_12_31: %8x\r\n", r->reserved_12_31);
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0xa << 2)));
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0xb << 2)));
}

static void i2s_dump_(void)
{
	SOC_LOGD(": %8x\r\n", REG_READ(SOC_I2S_REG_BASE + (0xc << 2)));
}

static i2s_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, i2s_dump_},
	{0x1, 0x1, i2s_dump_},
	{0x2, 0x2, i2s_dump_},
	{0x3, 0x3, i2s_dump_},
	{0x4, 0x4, i2s_dump_},
	{0x5, 0x5, i2s_dump_},
	{0x6, 0x6, i2s_dump_},
	{0x7, 0x7, i2s_dump_},
	{0x8, 0x8, i2s_dump_},
	{0x9, 0x9, i2s_dump_},
	{0xa, 0xa, i2s_dump_},
	{0xb, 0xb, i2s_dump_},
	{0xc, 0xc, i2s_dump_},
	{-1, -1, 0}
};

void i2s_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
