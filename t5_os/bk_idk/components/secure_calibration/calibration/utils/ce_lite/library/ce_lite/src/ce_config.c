/*
 * Copyright (c) 2020-2021, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "ce_lite_config.h"
#include "ce_common.h"
#include "ce_lite_trng.h"

const static ce_trng_conf_t g_trng_conf = {
    .src = {
        .grp0_en = 1,
        .grp1_en = 1,
        .grp2_en = 1,
        .grp3_en = 1,
        .src_sel = CE_TRNG_ENTROPY_SRC_INTERNAL,
    },
    .sample = {
        .div = 100,
        .dly = 3,
    },
    .ro = {
        .grp0_taps_en = 4,
        .grp1_taps_en = 5,
        .grp2_taps_en = 6,
        .grp3_taps_en = 7,
    },
    .postproc = {
        .prng_bypass = 1,
        .vn_bypass = 0,
        .crng_bypass = 0,
        .rsvd = 0,
        .lfsr_drop_num = 0,
        .lfsr_sel = 0,
        .fast_mode = 1,
    },
    .eval = {
        .adap_tst_th = 589,
        .rep_tst_th = 11,
        .adap_tst_en = 1,
        .rep_tst_en = 1,
        .ac_tst_en = 1,
        .ac_tst_th = 10,
    },
    .thr = {
        .vn_err_th = 1,
        .crng_err_th = 1,
        .rep_err_th = 1,
        .adap_err_th = 1,
    }
};

const ce_trng_conf_t *ce_get_trng_conf(void)
{
    return &g_trng_conf;
}

void ce_config_hw(void)
{
    PAL_LOG_INFO("Configure CE HW...\n");
    /* TODO: add your CE configurations here */
}