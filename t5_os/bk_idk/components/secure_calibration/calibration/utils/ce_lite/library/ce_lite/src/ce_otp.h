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

#ifndef __CE_OTP_H__
#define __CE_OTP_H__

#include "ce_lite_config.h"
#include "ce_config.h"
#include "ce_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OTP registers
 */
/* The offset of otp_setting of target host */
#define OTP_SETTING (OTP_CTL_OFS+ 0x00U)
#define OTP_SETTING_DIRECT_RD_SHIFT    (0x08U)
#define OTP_SETTING_DIRECT_RD_WIDTH    (0x01U)
#define OTP_SETTING_GENERAL_CTRL_SHIFT (0x09U)
#define OTP_SETTING_GENERAL_CTRL_WIDTH (0x08U)
#if defined(CE_LITE_OTP_PUF)
/**
 * PUF Gerneral control
 */
#define OTP_SETTING_GNRCTL_PUF_PTA_SHIFT       (0x09U)
#define OTP_SETTING_GNRCTL_PUF_PTA_WIDTH       (0x02U)
#define OTP_SETTING_GNRCTL_PAPUF_SHIFT         (0x0BU)
#define OTP_SETTING_GNRCTL_PAPUF_WIDTH         (0x01U)
#define OTP_SETTING_GNRCTL_PUF_PRESETN_SHIFT   (0x0CU)
#define OTP_SETTING_GNRCTL_PUF_PRESETN_WIDTH   (0x01U)
#define OTP_SETTING_GNRCTL_PUF_PDSTB_SHIFT     (0x0DU)
#define OTP_SETTING_GNRCTL_PUF_PDSTB_WIDTH     (0x01U)
#define OTP_SETTING_GNRCTL_PUF_PENVDD2_SHIFT   (0x0EU)
#define OTP_SETTING_GNRCTL_PUF_PENVDD2_WIDTH   (0x01U)
#endif

/* The offset of otp indirect write address of target host */
#define OTP_WR_ADDR (OTP_CTL_OFS + 0x04U)

/* The offset of data value to write to otp of target host */
#define OTP_WR_DATA (OTP_CTL_OFS + 0x08U)

/* The offset of otp write trigger of target host */
#define OTP_WR_TRIGGER (OTP_CTL_OFS + 0x0CU)

/* The offset of otp states of target host */
#define OTP_UPDATE_STAT (OTP_CTL_OFS + 0x10U)
#define OTP_UPDATE_BUSY (1U << 0)
#define OTP_UPDATE_FAIL (1U << 1)
#define OTP_READY       (1U << 2)

/* Below registers are only valid when CE_LITE_OTP_DUMMY
 * is configured
 * */


/* The offset of dummy otp config of target host */
#define OTP_DUMMY_CFG (OTP_CTL_OFS + 0x28U)
#define OTP_DUMMY_CFG_LCS_DR_SHIFT   (0x00U)
#define OTP_DUMMY_CFG_LCS_DR_WIDTH   (0x01U)
#define OTP_DUMMY_CFG_LCS_DD_SHIFT   (0x01U)
#define OTP_DUMMY_CFG_LCS_DD_WIDTH   (0x01U)
#define OTP_DUMMY_CFG_LCS_DM_SHIFT   (0x02U)
#define OTP_DUMMY_CFG_LCS_DM_WIDTH   (0x01U)
#define OTP_DUMMY_CFG_LCS_CM_SHIFT   (0x03U)
#define OTP_DUMMY_CFG_LCS_CM_WIDTH   (0x01U)
#define OTP_DUMMY_CFG_LOCK_EN_SHIFT  (0x04U)
#define OTP_DUMMY_CFG_LOCK_EN_WIDTH  (0x01U)

/* The offset of dummy otp device root key[31:0] of target host */
#define OTP_DUMMY_DEVICE_RK0 (OTP_CTL_OFS + 0x2CU)

/* The offset of dummy otp device root key[63:32] of target host */
#define OTP_DUMMY_DEVICE_RK1 (OTP_CTL_OFS + 0x30U)

/* The offset of dummy otp device root key[95:64] of target host */
#define OTP_DUMMY_DEVICE_RK2 (OTP_CTL_OFS + 0x34U)

/* The offset of dummy otp device root key[127:96] of target host */
#define OTP_DUMMY_DEVICE_RK3 (OTP_CTL_OFS + 0x38U)

/* The offset of dummy otp model key[31:0] of target host */
#define OTP_DUMMY_MODEL_KEY0 (OTP_CTL_OFS + 0x3CU)

/* The offset of dummy otp model key[63:32] of target host */
#define OTP_DUMMY_MODEL_KEY1 (OTP_CTL_OFS + 0x40U)

/* The offset of dummy otp model key[95:64] of target host */
#define OTP_DUMMY_MODEL_KEY2 (OTP_CTL_OFS + 0x44U)

/* The offset of dummy otp model key[127:96] of target host */
#define OTP_DUMMY_MODEL_KEY3 (OTP_CTL_OFS + 0x48U)

#define CE_OTP_USER_SEC_REGION_OFFSET(_cfg)        (0x80U + (_cfg).otp_ns_sz)
#define CE_OTP_TEST_REGION_OFFSET(_cfg)                                     \
        (CE_OTP_USER_SEC_REGION_OFFSET(_cfg) + (_cfg).otp_s_sz)

int32_t ce_otp_drv_init(void);
void ce_otp_drv_exit(void);

#ifdef __cplusplus
}
#endif

#endif /*__CE_OTP_H__*/
