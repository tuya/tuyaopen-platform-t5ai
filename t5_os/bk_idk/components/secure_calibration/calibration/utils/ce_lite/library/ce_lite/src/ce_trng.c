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

#include "ce_lite_trng.h"
#include "ce_trng.h"

#if defined(CE_LITE_TRNG)

static void trng_setup_config(const ce_trng_conf_t *conf)
{
    PAL_ASSERT(conf != NULL);

    IO_WRITE32(TRNG_ENTROPY_SRC_OFS, conf->src.val);
    IO_WRITE32(TRNG_ENTROPY_SMP_OFS, conf->sample.val);
    IO_WRITE32(TRNG_RO_SETTING_OFS, conf->ro.val);
    IO_WRITE32(TRNG_POST_PROC_OFS, conf->postproc.val);
    IO_WRITE32(TRNG_EVAL_SETTING_OFS, conf->eval.val);
}

static void trng_clk_enable(void)
{
    uint32_t reg;

    /* enable clock */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg |
                        (1U << TOP_CTL_CLOCK_CTL_TRNG_CLK_EN_SHIFT));
}

int32_t trng_engine_init(void)
{
    uint32_t ver;

    ver = IO_READ32(TOP_STAT_OFS);
    if (ver < 0xDB25U) {
        return CE_ERROR_NOT_SUPPORTED;
    }
#if (0U == CE_CFG_HOST_ID)
    trng_clk_enable();
    /** get the customized configuration for TRNG and set it to engine */
    trng_setup_config(ce_get_trng_conf());
#endif

    return CE_SUCCESS;
}

void trng_engine_exit(void)
{
    uint32_t val;

    val = IO_READ32(RNP_CTRL_OFS);
    val &= ~(0x1U);
    IO_WRITE32(RNP_CTRL_OFS, val);
}

static inline void trng_rnp_fill(void)
{
    uint32_t val;
    /** wait for rnp idle */
    IO_CHECK32(RNP_STAT_OFS, 0x6U, 0x0U);
    /** trigger to fill rnp */
    val = IO_READ32(RNP_CTRL_OFS);
    val |= 0x1U;
    IO_WRITE32(RNP_CTRL_OFS, val);
    /** check fill status */
    IO_CHECK32(RNP_INTR_OFS, 0x1U, 0x1U);
    /** eoi */
    IO_WRITE32(RNP_INTR_OFS, val);
}

static inline void trng_rnp_read(uint8_t *buf, size_t len)
{
    uint32_t val;
    size_t cp_len;
    size_t left_len = len;
    size_t i = 0;
    PAL_ASSERT(buf != NULL);

    do {
        /** assert that len shouldn't gt 32 bytes*/
        PAL_ASSERT(i < 8);
        val = IO_READ32(RNP_DAT_WORD0_OFS + (sizeof(val) * i++));
        cp_len  = CE_MIN(left_len, sizeof(val));
        pal_memcpy(buf + len - left_len, &val, cp_len);
        left_len -= cp_len;
    } while (left_len > 0);
}

#if defined (CE_LITE_TRNG_DUMP)
int32_t ce_trng_dump(ce_trng_request_t *req)
{
#define BM_IS_SET(b, msk)   ((msk) == ((b) & (msk)))
    size_t left_len;
    size_t read_len;
    size_t i = 0;
    uint32_t err;
    uint32_t val;

    if ((NULL == req) || ((req->size > 0) && (NULL == req->buf))) {
        return CE_ERROR_BAD_PARAMS;
    }
    if (0U != CE_CFG_HOST_ID) {
        return CE_ERROR_NOT_ACCEPTABLE;
    }
    if (req->b_conf) {
        trng_setup_config(&req->conf);
    }
    for (i = 0; i < req->nmemb; i++) {
        left_len = req->size;
        while (left_len > 0) {
            err = 0;
            trng_rnp_fill();
            read_len = CE_MIN(left_len, RNP_MAX_SIZE);
            /** trigger RNP to fill random number pool */
            trng_rnp_fill();
            trng_rnp_read(req->buf + req->size - left_len, read_len);
            left_len -= read_len;
            /** read health check data */
            val = IO_READ32(TRNG_INTR_OFS);
            IO_WRITE32(TRNG_INTR_OFS, val);
            if (BM_IS_SET(val, TRNG_INTR_ERR_ADAP)) {
                err |= 1 << CE_TRNG_ADAP_TEST_ERR_MASK;
            }
            if (BM_IS_SET(val, TRNG_INTR_ERR_REP)) {
                err |= 1 << CE_TRNG_REP_TEST_ERR_MASK;
            }
            if (BM_IS_SET(val, TRNG_INTR_ERR_CRNG)) {
                err |= 1 << CE_TRNG_CRNG_ERR_MASK;
            }
            if (BM_IS_SET(val, TRNG_INTR_ERR_VN)) {
                err |= 1 << CE_TRNG_VN_ERR_MASK;
            }
            val = IO_READ32(RNP_STAT_OFS);
            if (BM_IS_SET(val, RNP_STAT_AUTOCORR_TEST_ERR)) {
                err |= 1 << CE_TRNG_AUTOCORR_TEST_ERR_MASK;
            }
            if (req->on_error != NULL) {
                req->on_error(err);
            }
        }
        if (req->on_data != NULL) {
            req->on_data(req->buf, req->size);
        }
    }
    return CE_SUCCESS;
#undef BM_IS_SET
}
#endif /* CE_LITE_TRNG_DUMP */

int32_t ce_trng_read(uint8_t *buf, size_t len)
{
#if CONFIG_PRNG
    #define FLASH_BEGIN 0x02000000
    #define FLASH_SZ    0x10000

    static uint32_t s_flash_offset = FLASH_BEGIN;
    uint32_t flash_addr;
    uint32_t i = 0;

    while (i < len) {
        flash_addr = FLASH_BEGIN + (s_flash_offset++ % FLASH_SZ);
        buf[i] = (*(uint8_t*)flash_addr + i) & 0xFF;
        if (i > 0) {
            if (buf[i] == buf[i-1]) {
                continue;
            }
        }

        i++;
    }

    return CE_SUCCESS;
#else
    size_t left_len = len;
    size_t read_len;
    uint32_t val;

    if ((len != 0) && (NULL == buf)) {
        return CE_ERROR_BAD_PARAMS;
    }

    while (left_len > 0) {
        /** trigger RNP to fill random number pool */
        trng_rnp_fill();
        /** read appropriate size of data */
        read_len = CE_MIN(left_len , RNP_MAX_SIZE);
        trng_rnp_read(buf + len - left_len, read_len);
        if (0U == CE_CFG_HOST_ID) {
            /** eoi */
            val = IO_READ32(TRNG_INTR_OFS);
            IO_WRITE32(TRNG_INTR_OFS, val);
        }
        left_len -= read_len;
    }

    return CE_SUCCESS;
#endif
}
#endif /* CE_LITE_TRNG */
