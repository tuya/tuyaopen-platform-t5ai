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

#ifdef __cplusplus
extern "C" {
#endif


typedef volatile union {
	struct {
		uint32_t deviceid                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} scr_DeviceID_t;


typedef volatile union {
	struct {
		uint32_t versionid                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} scr_VersionID_t;


typedef volatile union {
	struct {
		uint32_t soft_rst                 :  1; /**<bit[0 : 0] */
		uint32_t bypass_ckg               :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_31        : 30; /**<bit[2 : 31] */
	};
	uint32_t v;
} scr_Global_CTRL_t;


typedef volatile union {
	struct {
		uint32_t global_status            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} scr_Global_Status_t;


typedef volatile union {
	struct {
		uint32_t invlev                   :  1; /**<bit[0 : 0] */
		uint32_t invord                   :  1; /**<bit[1 : 1] */
		uint32_t pech2fifo                :  1; /**<bit[2 : 2] */
		uint32_t reserved_3_5             :  3; /**<bit[3 : 5] */
		uint32_t clkstop                  :  1; /**<bit[6 : 6] */
		uint32_t clkstopval               :  1; /**<bit[7 : 7] */
		uint32_t txen                     :  1; /**<bit[8 : 8] */
		uint32_t rxen                     :  1; /**<bit[9 : 9] */
		uint32_t ts2fifo                  :  1; /**<bit[10 : 10] */
		uint32_t t0t1                     :  1; /**<bit[11 : 11] */
		uint32_t atrstflush               :  1; /**<bit[12 : 12] */
		uint32_t tcken                    :  1; /**<bit[13 : 13] */
		uint32_t reserved_14_14           :  1; /**<bit[14 : 14] */
		uint32_t ginten                   :  1; /**<bit[15 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_CTRL1_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_0             :  1; /**<bit[0 : 0] */
		uint32_t reserved_1_1             :  1; /**<bit[1 : 1] */
		uint32_t warmrst                  :  1; /**<bit[2 : 2] */
		uint32_t act                      :  1; /**<bit[3 : 3] */
		uint32_t deact                    :  1; /**<bit[4 : 4] */
		uint32_t vcc18                    :  1; /**<bit[5 : 5] */
		uint32_t vcc30                    :  1; /**<bit[6 : 6] */
		uint32_t vcc50                    :  1; /**<bit[7 : 7] */
		uint32_t act_fast                 :  1; /**<bit[8 : 8] */
		uint32_t reserved_9_15            :  7; /**<bit[9 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_CTRL2_t;


typedef volatile union {
	struct {
		uint32_t diraccpads               :  1; /**<bit[0 : 0] */
		uint32_t dscio                    :  1; /**<bit[1 : 1] */
		uint32_t dscclk                   :  1; /**<bit[2 : 2] */
		uint32_t dscrst                   :  1; /**<bit[3 : 3] */
		uint32_t dscvcc                   :  1; /**<bit[4 : 4] */
		uint32_t autoadeavpp              :  1; /**<bit[5 : 5] */
		uint32_t dscvppen                 :  1; /**<bit[6 : 6] */
		uint32_t dscvpppp                 :  1; /**<bit[7 : 7] */
		uint32_t dscfcb                   :  1; /**<bit[8 : 8] */
		uint32_t scpresent                :  1; /**<bit[9 : 9] */
		uint32_t reserved_10_15           :  6; /**<bit[10 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_SCPADS_t;


typedef volatile union {
	struct {
		uint32_t txfidone                 :  1; /**<bit[0 : 0] */
		uint32_t txfiempty                :  1; /**<bit[1 : 1] */
		uint32_t rxfifull                 :  1; /**<bit[2 : 2] */
		uint32_t clkstoprun               :  1; /**<bit[3 : 3] */
		uint32_t txdone                   :  1; /**<bit[4 : 4] */
		uint32_t rxdone                   :  1; /**<bit[5 : 5] */
		uint32_t txperr                   :  1; /**<bit[6 : 6] */
		uint32_t rxperr                   :  1; /**<bit[7 : 7] */
		uint32_t c2cfull                  :  1; /**<bit[8 : 8] */
		uint32_t rxthreshold              :  1; /**<bit[9 : 9] */
		uint32_t atrfail                  :  1; /**<bit[10 : 10] */
		uint32_t atrdone                  :  1; /**<bit[11 : 11] */
		uint32_t screm                    :  1; /**<bit[12 : 12] */
		uint32_t scins                    :  1; /**<bit[13 : 13] */
		uint32_t scact                    :  1; /**<bit[14 : 14] */
		uint32_t scdeact                  :  1; /**<bit[15 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_INTEN1_t;


typedef volatile union {
	struct {
		uint32_t txfidone                 :  1; /**<bit[0 : 0] */
		uint32_t txfiempty                :  1; /**<bit[1 : 1] */
		uint32_t rxfifull                 :  1; /**<bit[2 : 2] */
		uint32_t clkstoprun               :  1; /**<bit[3 : 3] */
		uint32_t txdone                   :  1; /**<bit[4 : 4] */
		uint32_t rxdone                   :  1; /**<bit[5 : 5] */
		uint32_t txperr                   :  1; /**<bit[6 : 6] */
		uint32_t rxperr                   :  1; /**<bit[7 : 7] */
		uint32_t c2cfull                  :  1; /**<bit[8 : 8] */
		uint32_t rxthreshold              :  1; /**<bit[9 : 9] */
		uint32_t atrfail                  :  1; /**<bit[10 : 10] */
		uint32_t atrdone                  :  1; /**<bit[11 : 11] */
		uint32_t screm                    :  1; /**<bit[12 : 12] */
		uint32_t scins                    :  1; /**<bit[13 : 13] */
		uint32_t scact                    :  1; /**<bit[14 : 14] */
		uint32_t scdeact                  :  1; /**<bit[15 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_INTSTAT1_t;


typedef volatile union {
	struct {
		uint32_t txfiempty                :  1; /**<bit[0 : 0] */
		uint32_t txfifull                 :  1; /**<bit[1 : 1] */
		uint32_t txfiflush                :  1; /**<bit[2 : 2] */
		uint32_t reserved_3_7             :  5; /**<bit[3 : 7] */
		uint32_t rxfiempty                :  1; /**<bit[8 : 8] */
		uint32_t rxfifull                 :  1; /**<bit[9 : 9] */
		uint32_t rxfiflush                :  1; /**<bit[10 : 10] */
		uint32_t reserved_11_15           :  5; /**<bit[11 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_FIFOCTRL_t;


typedef volatile union {
	struct {
		uint32_t legacy_rx_fifo_cnt       :  8; /**<bit[0 : 7] */
		uint32_t legacy_tx_fifo_cnt       :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Legacy_FIFO_Counter_t;


typedef volatile union {
	struct {
		uint32_t rx_fifo_thd              : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_RX_FIFO_Threshold_t;


typedef volatile union {
	struct {
		uint32_t tx_repeat                :  4; /**<bit[0 : 3] */
		uint32_t rx_repeat                :  4; /**<bit[4 : 7] */
		uint32_t reserved_8_15            :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_TX_Repeat_t;


typedef volatile union {
	struct {
		uint32_t smart_card_clk_div       : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Smart_Card_Clock_Divisor_t;


typedef volatile union {
	struct {
		uint32_t band_clk_div             : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Baud_Clock_Divisor_t;


typedef volatile union {
	struct {
		uint32_t smart_card_guardtime     :  8; /**<bit[0 : 7] */
		uint32_t reserved_8_15            :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Smart_Card_Guardtime_t;


typedef volatile union {
	struct {
		uint32_t activation_time          : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Activation_Deactivation_Time_t;


typedef volatile union {
	struct {
		uint32_t reset_duration           : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Reset_Duration_t;


typedef volatile union {
	struct {
		uint32_t atr_start_limit          : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_ATR_Start_Limit_t;


typedef volatile union {
	struct {
		uint32_t two_char_delay_limit_l   : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Two_Characters_Delay_Limit_L_t;


typedef volatile union {
	struct {
		uint32_t txthreshold              :  1; /**<bit[0 : 0] */
		uint32_t tckerr                   :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_15            : 14; /**<bit[2 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_INTEN2_t;


typedef volatile union {
	struct {
		uint32_t txthreshold              :  1; /**<bit[0 : 0] */
		uint32_t tckerr                   :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_15            : 14; /**<bit[2 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_INTSTAT2_t;


typedef volatile union {
	struct {
		uint32_t tx_fifo_thd              : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_TX_FIFO_Threshold_t;


typedef volatile union {
	struct {
		uint32_t tx_fifo_cnt              : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_TX_FIFO_Counter_t;


typedef volatile union {
	struct {
		uint32_t rx_fifo_cnt              : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_RX_FIFO_Counter_t;


typedef volatile union {
	struct {
		uint32_t baud_tune_reg            :  3; /**<bit[0 : 2] */
		uint32_t reserved_3_15            : 13; /**<bit[3 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Baud_Tune_Register_t;


typedef volatile union {
	struct {
		uint32_t two_char_delay_limit_h   :  8; /**<bit[0 : 7] */
		uint32_t reserved_8_15            :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_Two_Characters_Delay_Limit_H_t;


typedef volatile union {
	struct {
		uint32_t fifo                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_8_15            :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} scr_FIFO_t;

typedef volatile struct {
	volatile scr_DeviceID_t DeviceID;
	volatile scr_VersionID_t VersionID;
	volatile scr_Global_CTRL_t Global_CTRL;
	volatile scr_Global_Status_t Global_Status;
	volatile scr_CTRL1_t CTRL1;
	volatile scr_CTRL2_t CTRL2;
	volatile scr_SCPADS_t SCPADS;
	volatile scr_INTEN1_t INTEN1;
	volatile scr_INTSTAT1_t INTSTAT1;
	volatile scr_FIFOCTRL_t FIFOCTRL;
	volatile scr_Legacy_FIFO_Counter_t Legacy_FIFO_Counter;
	volatile scr_RX_FIFO_Threshold_t RX_FIFO_Threshold;
	volatile scr_TX_Repeat_t TX_Repeat;
	volatile scr_Smart_Card_Clock_Divisor_t Smart_Card_Clock_Divisor;
	volatile scr_Baud_Clock_Divisor_t Baud_Clock_Divisor;
	volatile scr_Smart_Card_Guardtime_t Smart_Card_Guardtime;
	volatile scr_Activation_Deactivation_Time_t Activation_Deactivation_Time;
	volatile scr_Reset_Duration_t Reset_Duration;
	volatile scr_ATR_Start_Limit_t ATR_Start_Limit;
	volatile scr_Two_Characters_Delay_Limit_L_t Two_Characters_Delay_Limit_L;
	volatile scr_INTEN2_t INTEN2;
	volatile scr_INTSTAT2_t INTSTAT2;
	volatile scr_TX_FIFO_Threshold_t TX_FIFO_Threshold;
	volatile scr_TX_FIFO_Counter_t TX_FIFO_Counter;
	volatile scr_RX_FIFO_Counter_t RX_FIFO_Counter;
	volatile scr_Baud_Tune_Register_t Baud_Tune_Register;
	volatile scr_Two_Characters_Delay_Limit_H_t Two_Characters_Delay_Limit_H;
	volatile uint32_t rsv_1b_7f[101];
	volatile scr_FIFO_t FIFO;
} scr_hw_t;

#ifdef __cplusplus
}
#endif
