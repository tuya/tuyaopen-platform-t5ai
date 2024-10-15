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

#ifndef __CE_TRNG_H__
#define __CE_TRNG_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"
#include "ce_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TRNG control registers
 */
#define TRNG_ENTROPY_SRC_OFS    (TRNG_CTL_OFS + 0x04U)
#define TRNG_ENTROPY_SMP_OFS    (TRNG_CTL_OFS + 0x08U)
#define TRNG_RO_SETTING_OFS     (TRNG_CTL_OFS + 0x0CU)
#define TRNG_POST_PROC_OFS      (TRNG_CTL_OFS + 0x10U)
#define TRNG_EVAL_SETTING_OFS   (TRNG_CTL_OFS + 0x14U)

#define TRNG_STAT_OFS           (TRNG_CTL_OFS + 0x18U)
#define TRNG_INTR_OFS           (TRNG_CTL_OFS + 0x1CU)
#define TRNG_INTR_MSK_OFS       (TRNG_CTL_OFS + 0x20U)
#define TRNG_ERR_CNT_TH_OFS     (TRNG_CTL_OFS + 0x24U)

#define TRNG_INTR_ERR_ADAP      (1 << 3U)
#define TRNG_INTR_ERR_REP       (1 << 2U)
#define TRNG_INTR_ERR_CRNG      (1 << 1U)
#define TRNG_INTR_ERR_VN        (1 << 0U)
/**
 *  RNG Pool control registers
 */
#define RNP_CTRL_OFS            (RNP_OFS + 0x00U)
#define RNP_STAT_OFS            (RNP_OFS + 0x04U)
#define RNP_INTR_OFS            (RNP_OFS + 0x08U)
#define RNP_INTR_MSK_OFS        (RNP_OFS + 0x0CU)
#define RNP_DAT_WORD0_OFS       (RNP_OFS + 0x10U)

#define RNP_STAT_AUTOCORR_TEST_ERR (1 << 0U)

#define RNP_MAX_SIZE            (32U)

/**
 * \brief           This function initializes the TRNG module with the default
 *                  configuration.
 *                  Note: there is a hardcode configuration for TRNG in
 *                  ce_config.c. And users can modify it to fit their
 *                  customized platform.
 * \return          always return \c CE_SUCCESS.
 */
int32_t trng_engine_init(void);

/**
 * \brief            This function deinit the trng module, stops
 *                   the rnp clock.
 *  \return          none.
 */
void trng_engine_exit(void);


#ifdef __cplusplus
}
#endif

#endif /* !__CE_TRNG_H__ */