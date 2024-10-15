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

#include "scr_hw.h"
#include "scr_hal.h"

void scr_hal_soft_reset(void)
{
	scr_ll_set_Global_CTRL_soft_rst(1);
}

void scr_hal_bypass_ckg(void)
{
	scr_ll_set_Global_CTRL_bypass_ckg(1);
}

void scr_hal_set_ctrl(scr_ctrl_idx_t ctrl_idx, uint32_t enable)
{
	uint32_t reg;

	switch (ctrl_idx) {
		case BK_SCR_CTRL_INV_LEVEL:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_INVLEV_MASK << SCR_CTRL1_INVLEV_POS);
			} else {
				reg &= ~(SCR_CTRL1_INVLEV_MASK << SCR_CTRL1_INVLEV_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_INV_ORD:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_INVORD_MASK << SCR_CTRL1_INVORD_POS);
			} else {
				reg &= ~(SCR_CTRL1_INVORD_MASK << SCR_CTRL1_INVORD_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_PECH2FIFO:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_PECH2FIFO_MASK << SCR_CTRL1_PECH2FIFO_POS);
			} else {
				reg &= ~(SCR_CTRL1_PECH2FIFO_MASK << SCR_CTRL1_PECH2FIFO_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_CLK_STOP:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_CLKSTOP_MASK << SCR_CTRL1_CLKSTOP_POS);
			} else {
				reg &= ~(SCR_CTRL1_CLKSTOP_MASK << SCR_CTRL1_CLKSTOP_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_CLK_STOP_VAL:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_CLKSTOPVAL_MASK << SCR_CTRL1_CLKSTOPVAL_POS);
			} else {
				reg &= ~(SCR_CTRL1_CLKSTOPVAL_MASK << SCR_CTRL1_CLKSTOPVAL_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_TX_EN:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_TXEN_MASK << SCR_CTRL1_TXEN_POS);
			} else {
				reg &= ~(SCR_CTRL1_TXEN_MASK << SCR_CTRL1_TXEN_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_RX_EN:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_RXEN_MASK << SCR_CTRL1_RXEN_POS);
			} else {
				reg &= ~(SCR_CTRL1_RXEN_MASK << SCR_CTRL1_RXEN_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_TS2FIFO:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_TS2FIFO_MASK << SCR_CTRL1_TS2FIFO_POS);
			} else {
				reg &= ~(SCR_CTRL1_TS2FIFO_MASK << SCR_CTRL1_TS2FIFO_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_T0T1:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_T0T1_MASK << SCR_CTRL1_T0T1_POS);
			} else {
				reg &= ~(SCR_CTRL1_T0T1_MASK << SCR_CTRL1_T0T1_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_ATR_START_FLUSH_FIFO:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_ATRSTFLUSH_MASK << SCR_CTRL1_ATRSTFLUSH_POS);
			} else {
				reg &= ~(SCR_CTRL1_ATRSTFLUSH_MASK << SCR_CTRL1_ATRSTFLUSH_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_TCK_EN:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_TCKEN_MASK << SCR_CTRL1_TCKEN_POS);
			} else {
				reg &= ~(SCR_CTRL1_TCKEN_MASK << SCR_CTRL1_TCKEN_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_GLOBAL_INTR_EN:
			reg = scr_ll_get_CTRL1_value();
			if (enable) {
				reg |= (SCR_CTRL1_GINTEN_MASK << SCR_CTRL1_GINTEN_POS);
			} else {
				reg &= ~(SCR_CTRL1_GINTEN_MASK << SCR_CTRL1_GINTEN_POS);
			}
			scr_ll_set_CTRL1_value(reg);
			break;

		case BK_SCR_CTRL_WARM_RST:
			reg = scr_ll_get_CTRL2_value();
			if (enable) {
				reg |= (SCR_CTRL2_WARMRST_MASK << SCR_CTRL2_WARMRST_POS);
			} else {
				reg &= ~(SCR_CTRL2_WARMRST_MASK << SCR_CTRL2_WARMRST_POS);
			}
			scr_ll_set_CTRL2_value(reg);
			break;

		case BK_SCR_CTRL_ACT:
			reg = scr_ll_get_CTRL2_value();
			if (enable) {
				reg |= (SCR_CTRL2_ACT_MASK << SCR_CTRL2_ACT_POS);
			} else {
				reg &= ~(SCR_CTRL2_ACT_MASK << SCR_CTRL2_ACT_POS);
			}
			scr_ll_set_CTRL2_value(reg);
			break;

		case BK_SCR_CTRL_DEACT:
			reg = scr_ll_get_CTRL2_value();
			if (enable) {
				reg |= (SCR_CTRL2_DEACT_MASK << SCR_CTRL2_DEACT_POS);
			} else {
				reg &= ~(SCR_CTRL2_DEACT_MASK << SCR_CTRL2_DEACT_POS);
			}
			scr_ll_set_CTRL2_value(reg);
			break;

		case BK_SCR_CTRL_VCC:
			reg = scr_ll_get_CTRL2_value();
			if (enable) {
				reg |= (SCR_CTRL2_VCC30_MASK << SCR_CTRL2_VCC30_POS);
			} else {
				reg &= ~(SCR_CTRL2_VCC30_MASK << SCR_CTRL2_VCC30_POS);
			}
			scr_ll_set_CTRL2_value(reg);
			break;

		case BK_SCR_CTRL_ACT_FAST:
			reg = scr_ll_get_CTRL2_value();
			if (enable) {
				reg |= (SCR_CTRL2_ACT_FAST_MASK << SCR_CTRL2_ACT_FAST_POS);
			} else {
				reg &= ~(SCR_CTRL2_ACT_FAST_MASK << SCR_CTRL2_ACT_FAST_POS);
			}
			scr_ll_set_CTRL2_value(reg);
			break;

		default:
			scr_ll_set_CTRL1_value(enable & 0xFFFF);
			scr_ll_set_CTRL2_value((enable >> 16) & 0x1FF);
			break;
	}
}

uint32_t scr_hal_get_ctrl(scr_ctrl_idx_t ctrl_idx)
{
	uint32_t reg;

	switch (ctrl_idx) {
		case BK_SCR_CTRL_INV_LEVEL:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_INVLEV_POS) & SCR_CTRL1_INVLEV_MASK);
			break;

		case BK_SCR_CTRL_INV_ORD:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_INVORD_POS) & SCR_CTRL1_INVORD_MASK);
			break;

		case BK_SCR_CTRL_PECH2FIFO:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_PECH2FIFO_POS) & SCR_CTRL1_PECH2FIFO_MASK);
			break;

		case BK_SCR_CTRL_CLK_STOP:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_CLKSTOP_POS) & SCR_CTRL1_CLKSTOP_MASK);
			break;

		case BK_SCR_CTRL_CLK_STOP_VAL:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_CLKSTOPVAL_POS) & SCR_CTRL1_CLKSTOPVAL_MASK);
			break;

		case BK_SCR_CTRL_TX_EN:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_TXEN_POS) & SCR_CTRL1_TXEN_MASK);
			break;

		case BK_SCR_CTRL_RX_EN:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_RXEN_POS) & SCR_CTRL1_RXEN_MASK);
			break;

		case BK_SCR_CTRL_TS2FIFO:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_TS2FIFO_POS) & SCR_CTRL1_TS2FIFO_MASK);
			break;

		case BK_SCR_CTRL_T0T1:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_T0T1_POS) & SCR_CTRL1_T0T1_MASK);
			break;

		case BK_SCR_CTRL_ATR_START_FLUSH_FIFO:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_ATRSTFLUSH_POS) & SCR_CTRL1_ATRSTFLUSH_MASK);
			break;

		case BK_SCR_CTRL_TCK_EN:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_TCKEN_POS) & SCR_CTRL1_TCKEN_MASK);
			break;

		case BK_SCR_CTRL_GLOBAL_INTR_EN:
			reg = scr_ll_get_CTRL1_value();
			reg = ((reg >> SCR_CTRL1_GINTEN_POS) & SCR_CTRL1_GINTEN_MASK);
			break;

		case BK_SCR_CTRL_WARM_RST:
			reg = scr_ll_get_CTRL2_value();
			reg = ((reg >> SCR_CTRL2_WARMRST_POS) & SCR_CTRL2_WARMRST_MASK);
			break;

		case BK_SCR_CTRL_ACT:
			reg = scr_ll_get_CTRL2_value();
			reg = ((reg >> SCR_CTRL2_ACT_POS) & SCR_CTRL2_ACT_MASK);
			break;

		case BK_SCR_CTRL_DEACT:
			reg = scr_ll_get_CTRL2_value();
			reg = ((reg >> SCR_CTRL2_DEACT_POS) & SCR_CTRL2_DEACT_MASK);
			break;

		case BK_SCR_CTRL_VCC:
			reg = scr_ll_get_CTRL2_value();
			reg = ((reg >> SCR_CTRL2_VCC30_POS) & SCR_CTRL2_VCC30_MASK);
			break;

		case BK_SCR_CTRL_ACT_FAST:
			reg = scr_ll_get_CTRL2_value();
			reg = ((reg >> SCR_CTRL2_ACT_FAST_POS) & SCR_CTRL2_ACT_FAST_MASK);
			break;

		default:
			reg = scr_ll_get_CTRL1_value();
			reg |= (scr_ll_get_CTRL2_value() << 16);
			break;
	}

	return reg;
}

void scr_hal_set_intr(scr_intr_idx_t intr_idx, uint32_t enable)
{
	uint32_t reg;

	switch (intr_idx) {
		case BK_SCR_INTR_TX_FIFO_DONE:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_TXFIDONE_MASK << SCR_INTEN1_TXFIDONE_POS);
			}else {
				reg &= ~(SCR_INTEN1_TXFIDONE_MASK << SCR_INTEN1_TXFIDONE_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_TX_FIFO_EMPTY:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_TXFIEMPTY_MASK << SCR_INTEN1_TXFIEMPTY_POS);
			} else {
				reg &= ~(SCR_INTEN1_TXFIEMPTY_MASK << SCR_INTEN1_TXFIEMPTY_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_RX_FIFO_FULL:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_RXFIFULL_MASK << SCR_INTEN1_RXFIFULL_POS);
			} else {
				reg &= ~(SCR_INTEN1_RXFIFULL_MASK << SCR_INTEN1_RXFIFULL_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_CLK_STOP_RUN:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_CLKSTOPRUN_MASK << SCR_INTEN1_CLKSTOPRUN_POS);
			} else {
				reg &= ~(SCR_INTEN1_CLKSTOPRUN_MASK << SCR_INTEN1_CLKSTOPRUN_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_TX_DONE:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_TXDONE_MASK << SCR_INTEN1_TXDONE_POS);
			} else {
				reg &= ~(SCR_INTEN1_TXDONE_MASK << SCR_INTEN1_TXDONE_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_RX_DONE:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_RXDONE_MASK << SCR_INTEN1_RXDONE_POS);
			} else {
				reg &= ~(SCR_INTEN1_RXDONE_MASK << SCR_INTEN1_RXDONE_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_TX_PERR:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_TXPERR_MASK << SCR_INTEN1_TXPERR_POS);
			} else {
				reg &= ~(SCR_INTEN1_TXPERR_MASK << SCR_INTEN1_TXPERR_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_RX_PERR:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_RXPERR_MASK << SCR_INTEN1_RXPERR_POS);
			} else {
				reg &= ~(SCR_INTEN1_RXPERR_MASK << SCR_INTEN1_RXPERR_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_C2C_FULL:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_C2CFULL_MASK << SCR_INTEN1_C2CFULL_POS);
			} else {
				reg &= ~(SCR_INTEN1_C2CFULL_MASK << SCR_INTEN1_C2CFULL_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_RX_THD:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_RXTHRESHOLD_MASK << SCR_INTEN1_RXTHRESHOLD_POS);
			} else {
				reg &= ~(SCR_INTEN1_RXTHRESHOLD_MASK << SCR_INTEN1_RXTHRESHOLD_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_ATR_FAIL:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_ATRFAIL_MASK << SCR_INTEN1_ATRFAIL_POS);
			} else {
				reg &= ~(SCR_INTEN1_ATRFAIL_MASK << SCR_INTEN1_ATRFAIL_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_ATR_DONE:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_ATRDONE_MASK << SCR_INTEN1_ATRDONE_POS);
			} else {
				reg &= ~(SCR_INTEN1_ATRDONE_MASK << SCR_INTEN1_ATRDONE_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_SC_REM:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_SCREM_MASK << SCR_INTEN1_SCREM_POS);
			} else {
				reg &= ~(SCR_INTEN1_SCREM_MASK << SCR_INTEN1_SCREM_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_SC_INS:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_SCINS_MASK << SCR_INTEN1_SCINS_POS);
			} else {
				reg &= ~(SCR_INTEN1_SCINS_MASK << SCR_INTEN1_SCINS_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_SC_ACT:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_SCACT_MASK << SCR_INTEN1_SCACT_POS);
			} else {
				reg &= ~(SCR_INTEN1_SCACT_MASK << SCR_INTEN1_SCACT_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_SC_DEACT:
			reg = scr_ll_get_INTEN1_value();
			if (enable) {
				reg |= (SCR_INTEN1_SCDEACT_MASK << SCR_INTEN1_SCDEACT_POS);
			} else {
				reg &= ~(SCR_INTEN1_SCDEACT_MASK << SCR_INTEN1_SCDEACT_POS);
			}
			scr_ll_set_INTEN1_value(reg);
			break;

		case BK_SCR_INTR_TX_THD:
			reg = scr_ll_get_INTEN2_value();
			if (enable) {
				reg |= (SCR_INTEN2_TXTHRESHOLD_MASK << SCR_INTEN2_TXTHRESHOLD_POS);
			} else {
				reg &= ~(SCR_INTEN2_TXTHRESHOLD_MASK << SCR_INTEN2_TXTHRESHOLD_POS);
			}
			scr_ll_set_INTEN2_value(reg);
			break;

		case BK_SCR_INTR_TCK_ERR:
			reg = scr_ll_get_INTEN2_value();
			if (enable) {
				reg |= (SCR_INTEN2_TCKERR_MASK << SCR_INTEN2_TCKERR_POS);
			} else {
				reg &= ~(SCR_INTEN2_TCKERR_MASK << SCR_INTEN2_TCKERR_POS);
			}
			scr_ll_set_INTEN2_value(reg);
			break;

		default:
			scr_ll_set_INTEN1_value(enable & 0xFFFF);
			scr_ll_set_INTEN2_value((enable >> 16) & 0x3);
			break;
	}
}

uint32_t scr_hal_get_intr(scr_intr_idx_t intr_idx)
{
	uint32_t reg;

	switch (intr_idx) {
		case BK_SCR_INTR_TX_FIFO_DONE:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_TXFIDONE_POS) & SCR_INTEN1_TXFIDONE_MASK);
			break;

		case BK_SCR_INTR_TX_FIFO_EMPTY:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_TXFIEMPTY_POS) & SCR_INTEN1_TXFIEMPTY_MASK);
			break;

		case BK_SCR_INTR_RX_FIFO_FULL:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_RXFIFULL_POS) & SCR_INTEN1_RXFIFULL_MASK);
			break;

		case BK_SCR_INTR_CLK_STOP_RUN:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_CLKSTOPRUN_POS) & SCR_INTEN1_CLKSTOPRUN_MASK);
			break;

		case BK_SCR_INTR_TX_DONE:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_TXDONE_POS) & SCR_INTEN1_TXDONE_MASK);
			break;

		case BK_SCR_INTR_RX_DONE:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_RXDONE_POS) & SCR_INTEN1_RXDONE_MASK);
			break;

		case BK_SCR_INTR_TX_PERR:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_TXPERR_POS) & SCR_INTEN1_TXPERR_MASK);
			break;

		case BK_SCR_INTR_RX_PERR:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_RXPERR_POS) & SCR_INTEN1_RXPERR_MASK);
			break;

		case BK_SCR_INTR_C2C_FULL:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_C2CFULL_POS) & SCR_INTEN1_C2CFULL_MASK);
			break;

		case BK_SCR_INTR_RX_THD:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_RXTHRESHOLD_POS) & SCR_INTEN1_RXTHRESHOLD_MASK);
			break;

		case BK_SCR_INTR_ATR_FAIL:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_ATRFAIL_POS) & SCR_INTEN1_ATRFAIL_MASK);
			break;

		case BK_SCR_INTR_ATR_DONE:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_ATRDONE_POS) & SCR_INTEN1_ATRDONE_MASK);
			break;

		case BK_SCR_INTR_SC_REM:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_SCREM_POS) & SCR_INTEN1_SCREM_MASK);
			break;

		case BK_SCR_INTR_SC_INS:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_SCINS_POS) & SCR_INTEN1_SCINS_MASK);
			break;

		case BK_SCR_INTR_SC_ACT:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_SCACT_POS) & SCR_INTEN1_SCACT_MASK);
			break;

		case BK_SCR_INTR_SC_DEACT:
			reg = scr_ll_get_INTEN1_value();
			reg = ((reg >> SCR_INTEN1_SCDEACT_POS) & SCR_INTEN1_SCDEACT_MASK);
			break;

		case BK_SCR_INTR_TX_THD:
			reg = scr_ll_get_INTEN2_value();
			reg = ((reg >> SCR_INTEN2_TXTHRESHOLD_POS) & SCR_INTEN2_TXTHRESHOLD_MASK);
			break;

		case BK_SCR_INTR_TCK_ERR:
			reg = scr_ll_get_INTEN2_value();
			reg = ((reg >> SCR_INTEN2_TCKERR_POS) & SCR_INTEN2_TCKERR_MASK);
			break;

		default:
			reg = scr_ll_get_INTEN1_value();
			reg |= (scr_ll_get_INTEN2_value() << 16);
			break;
	}

	return reg;
}


uint32_t scr_hal_get_intr_status(scr_intr_idx_t intr_idx)
{
	uint32_t reg;

	switch (intr_idx) {
		case BK_SCR_INTR_TX_FIFO_DONE:
			reg = scr_ll_get_INTSTAT1_txfidone();
			break;

		case BK_SCR_INTR_TX_FIFO_EMPTY:
			reg = scr_ll_get_INTSTAT1_txfiempty();
			break;

		case BK_SCR_INTR_RX_FIFO_FULL:
			reg = scr_ll_get_INTSTAT1_rxfifull();
			break;

		case BK_SCR_INTR_CLK_STOP_RUN:
			reg = scr_ll_get_INTSTAT1_clkstoprun();
			break;

		case BK_SCR_INTR_TX_DONE:
			reg = scr_ll_get_INTSTAT1_txdone();
			break;

		case BK_SCR_INTR_RX_DONE:
			reg = scr_ll_get_INTSTAT1_rxdone();
			break;

		case BK_SCR_INTR_C2C_FULL:
			reg = scr_ll_get_INTSTAT1_c2cfull();
			break;

		case BK_SCR_INTR_RX_THD:
			reg = scr_ll_get_INTSTAT1_rxthreshold();
			break;

		case BK_SCR_INTR_ATR_FAIL:
			reg = scr_ll_get_INTSTAT1_atrfail();
			break;

		case BK_SCR_INTR_ATR_DONE:
			reg = scr_ll_get_INTSTAT1_atrdone();
			break;

		case BK_SCR_INTR_SC_ACT:
			reg = scr_ll_get_INTSTAT1_scact();
			break;

		case BK_SCR_INTR_SC_DEACT:
			reg = scr_ll_get_INTSTAT1_scdeact();
			break;

		case BK_SCR_INTR_TX_THD:
			reg = scr_ll_get_INTSTAT2_txthreshold();
			break;

		case BK_SCR_INTR_TCK_ERR:
			reg = scr_ll_get_INTSTAT2_tckerr();
			break;

		default:
			reg = scr_ll_get_INTSTAT1_value();
			reg |= (scr_ll_get_INTSTAT2_value() << 16);
			break;
	}

	return reg;
}

void scr_hal_clr_intr_status(scr_intr_idx_t intr_idx)
{
	uint32_t reg;

	switch (intr_idx) {
		case BK_SCR_INTR_TX_FIFO_DONE:
			scr_ll_set_INTSTAT1_txfidone();
			break;

		case BK_SCR_INTR_TX_FIFO_EMPTY:
			scr_ll_set_INTSTAT1_txfiempty();
			break;

		case BK_SCR_INTR_RX_FIFO_FULL:
			scr_ll_set_INTSTAT1_rxfifull();
			break;

		case BK_SCR_INTR_CLK_STOP_RUN:
			scr_ll_set_INTSTAT1_clkstoprun();
			break;

		case BK_SCR_INTR_TX_DONE:
			scr_ll_set_INTSTAT1_txdone();
			break;

		case BK_SCR_INTR_RX_DONE:
			scr_ll_set_INTSTAT1_rxdone();
			break;

		case BK_SCR_INTR_TX_PERR:
			scr_ll_set_INTSTAT1_txperr();
			break;

		case BK_SCR_INTR_RX_PERR:
			scr_ll_set_INTSTAT1_rxperr();
			break;

		case BK_SCR_INTR_C2C_FULL:
			scr_ll_set_INTSTAT1_c2cfull();
			break;

		case BK_SCR_INTR_RX_THD:
			scr_ll_set_INTSTAT1_rxthreshold();
			break;

		case BK_SCR_INTR_ATR_FAIL:
			scr_ll_set_INTSTAT1_atrfail();
			break;

		case BK_SCR_INTR_ATR_DONE:
			scr_ll_set_INTSTAT1_atrdone();
			break;

		case BK_SCR_INTR_SC_ACT:
			scr_ll_set_INTSTAT1_scact();
			break;

		case BK_SCR_INTR_SC_DEACT:
			scr_ll_set_INTSTAT1_scdeact();
			break;

		case BK_SCR_INTR_TX_THD:
			scr_ll_set_INTSTAT2_txthreshold();
			break;

		case BK_SCR_INTR_TCK_ERR:
			scr_ll_set_INTSTAT2_tckerr();
			break;

		default:
			reg = scr_ll_get_INTSTAT1_value();
			scr_ll_set_INTSTAT1_value(reg);
			reg = scr_ll_get_INTSTAT2_value();
			scr_ll_set_INTSTAT2_value(reg);
			break;
	}
}

uint32_t scr_hal_check_intr_status(scr_intr_idx_t intr_idx, uint32_t intr_status)
{
	uint32_t reg;

	switch (intr_idx) {
		case BK_SCR_INTR_TX_FIFO_DONE:
			reg = (intr_status >> SCR_INTSTAT1_TXFIDONE_POS) & SCR_INTSTAT1_TXFIDONE_MASK;
			break;

		case BK_SCR_INTR_TX_FIFO_EMPTY:
			reg = (intr_status >> SCR_INTSTAT1_TXFIEMPTY_POS) & SCR_INTSTAT1_TXFIEMPTY_MASK;
			break;

		case BK_SCR_INTR_RX_FIFO_FULL:
			reg = (intr_status >> SCR_INTSTAT1_RXFIFULL_POS) & SCR_INTSTAT1_RXFIFULL_MASK;
			break;

		case BK_SCR_INTR_CLK_STOP_RUN:
			reg = (intr_status >> SCR_INTSTAT1_CLKSTOPRUN_POS) & SCR_INTSTAT1_CLKSTOPRUN_MASK;
			break;

		case BK_SCR_INTR_TX_DONE:
			reg = (intr_status >> SCR_INTSTAT1_TXDONE_POS) & SCR_INTSTAT1_TXDONE_MASK;
			break;

		case BK_SCR_INTR_RX_DONE:
			reg = (intr_status >> SCR_INTSTAT1_RXDONE_POS) & SCR_INTSTAT1_RXDONE_MASK;
			break;

		case BK_SCR_INTR_TX_PERR:
			reg = (intr_status >> SCR_INTSTAT1_TXPERR_POS) & SCR_INTSTAT1_TXPERR_MASK;
			break;

		case BK_SCR_INTR_RX_PERR:
			reg = (intr_status >> SCR_INTSTAT1_RXPERR_POS) & SCR_INTSTAT1_RXPERR_MASK;
			break;

		case BK_SCR_INTR_C2C_FULL:
			reg = (intr_status >> SCR_INTSTAT1_C2CFULL_POS) & SCR_INTSTAT1_C2CFULL_MASK;
			break;

		case BK_SCR_INTR_RX_THD:
			reg = (intr_status >> SCR_INTSTAT1_RXTHRESHOLD_POS) & SCR_INTSTAT1_RXTHRESHOLD_MASK;
			break;

		case BK_SCR_INTR_ATR_FAIL:
			reg = (intr_status >> SCR_INTSTAT1_ATRFAIL_POS) & SCR_INTSTAT1_ATRFAIL_MASK;
			break;

		case BK_SCR_INTR_ATR_DONE:
			reg = (intr_status >> SCR_INTSTAT1_ATRDONE_POS) & SCR_INTSTAT1_ATRDONE_MASK;
			break;

		case BK_SCR_INTR_SC_REM:
			reg = (intr_status >> SCR_INTSTAT1_SCREM_POS) & SCR_INTSTAT1_SCREM_MASK;
			break;

		case BK_SCR_INTR_SC_INS:
			reg = (intr_status >> SCR_INTSTAT1_SCINS_POS) & SCR_INTSTAT1_SCINS_MASK;
			break;

		case BK_SCR_INTR_SC_ACT:
			reg = (intr_status >> SCR_INTSTAT1_SCACT_POS) & SCR_INTSTAT1_SCACT_MASK;
			break;

		case BK_SCR_INTR_SC_DEACT:
			reg = (intr_status >> SCR_INTSTAT1_SCDEACT_POS) & SCR_INTSTAT1_SCDEACT_MASK;
			break;

		case BK_SCR_INTR_TX_THD:
			reg = (intr_status >> SCR_INTSTAT2_TXTHRESHOLD_POS) & SCR_INTSTAT2_TXTHRESHOLD_MASK;
			break;

		case BK_SCR_INTR_TCK_ERR:
			reg = (intr_status >> SCR_INTSTAT2_TCKERR_POS) & SCR_INTSTAT2_TCKERR_MASK;
			break;

		default:
			reg = 0;
			break;
	}

	return reg;
}

/** FIFOCTRL start*/
uint32_t scr_hal_get_tx_fifo_empty(void)
{
	return scr_ll_get_FIFOCTRL_txfiempty();
}

uint32_t scr_hal_get_tx_fifo_full(void)
{
	return scr_ll_get_FIFOCTRL_txfifull();
}

void scr_hal_set_tx_fifo_flush(void)
{
	scr_ll_set_FIFOCTRL_txfiflush(1);
}

uint32_t scr_hal_get_rx_fifo_empty(void)
{
	return scr_ll_get_FIFOCTRL_rxfiempty();
}

uint32_t scr_hal_get_rx_fifo_full(void)
{
	return scr_ll_get_FIFOCTRL_rxfifull();
}

void scr_hal_set_rx_fifo_flush(void)
{
	scr_ll_set_FIFOCTRL_rxfiflush(1);
}
/** FIFOCTRL end*/

void scr_hal_set_rx_fifo_threshold(uint32_t value)
{
	scr_ll_set_RX_FIFO_Threshold_rx_fifo_thd(value & 0xFF);
}

uint32_t scr_hal_get_rx_fifo_threshold(void)
{
	return (scr_ll_get_RX_FIFO_Threshold_rx_fifo_thd() & 0xFF);
}

void scr_hal_set_tx_fifo_threshold(uint32_t value)
{
	scr_ll_set_TX_FIFO_Threshold_tx_fifo_thd(value & 0xFFFF);
}

uint32_t scr_hal_get_tx_fifo_threshold(void)
{
	return (scr_ll_get_TX_FIFO_Threshold_tx_fifo_thd() & 0xFFFF);
}

void scr_hal_set_tx_repeat(uint32_t value)
{
	scr_ll_set_TX_Repeat_tx_repeat(value & 0xF);
}

uint32_t scr_hal_get_tx_repeat(void)
{
	return (scr_ll_get_TX_Repeat_tx_repeat() & 0xF);
}

void scr_hal_set_rx_repeat(uint32_t value)
{
	scr_ll_set_TX_Repeat_rx_repeat(value & 0xF);
}

uint32_t scr_hal_get_rx_repeat(void)
{
	return (scr_ll_get_TX_Repeat_rx_repeat() & 0xF);
}

void scr_hal_set_smart_card_clock_divisor(uint32_t value)
{
	scr_ll_set_Smart_Card_Clock_Divisor_smart_card_clk_div(value & 0xFFFF);
}

uint32_t scr_hal_get_smart_card_clock_divisor(void)
{
	return (scr_ll_get_Smart_Card_Clock_Divisor_smart_card_clk_div() & 0xFFFF);
}

void scr_hal_set_baud_clock_divisor(uint32_t value)
{
	scr_ll_set_Baud_Clock_Divisor_band_clk_div(value & 0xFFFF);
}

uint32_t scr_hal_get_baud_clock_divisor(void)
{
	return (scr_ll_get_Baud_Clock_Divisor_band_clk_div() & 0xFFFF);
}

void scr_hal_set_smart_card_guardtime(uint32_t value)
{
	scr_ll_set_Smart_Card_Guardtime_smart_card_guardtime(value & 0xFF);
}

uint32_t scr_hal_get_smart_card_guardtime(void)
{
	return (scr_ll_get_Smart_Card_Guardtime_smart_card_guardtime() & 0xFF);
}

void scr_hal_set_active_deactive_time(uint32_t value)
{
	scr_ll_set_Activation_Deactivation_Time_activation_time(value & 0xFFFF);
}

uint32_t scr_hal_get_active_deactive_time(void)
{
	return (scr_ll_get_Activation_Deactivation_Time_activation_time() & 0xFFFF);
}

void scr_hal_set_reset_duration(uint32_t value)
{
	scr_ll_set_Reset_Duration_reset_duration(value & 0xFFFF);
}

uint32_t scr_hal_get_reset_duration(void)
{
	return (scr_ll_get_Reset_Duration_reset_duration() & 0xFFFF);
}

void scr_hal_set_ATR_start_limit(uint32_t value)
{
	scr_ll_set_ATR_Start_Limit_atr_start_limit(value & 0xFFFF);
}

uint32_t scr_hal_get_ATR_start_limit(void)
{
	return (scr_ll_get_ATR_Start_Limit_atr_start_limit() & 0xFFFF);
}

void scr_hal_set_two_char_delay_limit(uint32_t value)
{
	scr_ll_set_Two_Characters_Delay_Limit_L_two_char_delay_limit_l(value & 0xFFFF);
	scr_ll_set_Two_Characters_Delay_Limit_H_two_char_delay_limit_h((value >> 16) & 0xFF);
}

uint32_t scr_hal_get_two_char_delay_limit(void)
{
	uint32_t reg;

	reg = scr_ll_get_Two_Characters_Delay_Limit_L_two_char_delay_limit_l() & 0xFFFF;
	reg |= ((scr_ll_get_Two_Characters_Delay_Limit_H_two_char_delay_limit_h() & 0xFF) << 16);

	return reg;
}

uint32_t scr_hal_get_tx_fifo_counter(void)
{
	return (scr_ll_get_TX_FIFO_Counter_tx_fifo_cnt() & 0xFFFF);
}

uint32_t scr_hal_get_rx_fifo_counter(void)
{
	return (scr_ll_get_RX_FIFO_Counter_rx_fifo_cnt() & 0xFFFF);
}

void scr_hal_set_baud_tune(uint32_t value)
{
	scr_ll_set_Baud_Tune_Register_baud_tune_reg(value & 0x7);
}

uint32_t scr_hal_get_baud_tune(void)
{
	return (scr_ll_get_Baud_Tune_Register_baud_tune_reg() & 0x7);
}

uint32_t scr_hal_get_fifo_data(void)
{
	return scr_ll_get_FIFO_fifo();
}

void scr_hal_set_fifo_data(uint32_t data)
{
	scr_ll_set_FIFO_fifo(data);
}
