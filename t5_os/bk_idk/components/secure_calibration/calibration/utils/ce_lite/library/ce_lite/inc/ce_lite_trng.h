/*
 * Copyright (c) 2021, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#ifndef __CE_LITE_TRNG_H__
#define __CE_LITE_TRNG_H__

#include "ce_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif
#define CE_TRNG_ADAP_TEST_ERR_MASK      (0x00U)
#define CE_TRNG_REP_TEST_ERR_MASK       (0x01U)
#define CE_TRNG_CRNG_ERR_MASK           (0x02U)
#define CE_TRNG_VN_ERR_MASK             (0x03U)
#define CE_TRNG_AUTOCORR_TEST_ERR_MASK  (0x04U)

#define CE_TRNG_ENTROPY_SRC_INTERNAL    (0x0U)
#define CE_TRNG_ENTROPY_SRC_EXTERNAL    (0x1U)
/******************************************************************************/
/*                            TRNG APIs                                       */
/******************************************************************************/
/**
 *  entropy source
 */
typedef union {
    uint32_t val;
    struct
    {
        uint32_t grp0_en:1;    /* inverter chain0 0x0:disable 0x1:enable */
        uint32_t grp1_en:1;    /* inverter chain1 0x0:disable 0x1:enable */
        uint32_t grp2_en:1;    /* inverter chain2 0x0:disable 0x1:enable */
        uint32_t grp3_en:1;    /* inverter chain3 0x0:disable 0x1:enable */
        uint32_t src_sel:1;    /* entropy source 0x0:internal 0x01:external */
    };
} ce_trng_entropy_src_t;

/**
 *  entropy sample
 */
typedef union {
    uint32_t val;
    struct {
      uint32_t div:16;              /* Sampling rate divider value, source
                                       is Merak core clock */
      uint32_t dly:8;               /* Sampling process delay */
    };
} ce_trng_entropy_sample_t;


/**
 * post process
 */
typedef union {
    uint32_t val;
    struct {
        uint32_t prng_bypass:1;      /* PRNG 0x0:enable the PRNG module,
                                        0x1:bypass */
        uint32_t vn_bypass:1;        /* VN 0x0:enable the VN module,
                                        0x1:bypass */
        uint32_t crng_bypass:1;      /* CRNG 0x0:enable the CRNG module,
                                        0x1:bypass */
        uint32_t rsvd:8;
        uint32_t lfsr_drop_num:2;    /* Number of lfsr dropped data during
                                        every block. 0x0:not drop any bits
                                        0x1:128b 0x2:256b 0x3:512b */
        uint32_t lfsr_sel:1;         /* 0x0:no xor as feedback, just as
                                        shifter on the feed seed stage,
                                        0x1:always xor */
        uint32_t fast_mode:1;        /* 0x1:will work in the fast mode and
                                        data out by trng clock */
    };
} ce_trng_postproc_t;

/**
 * evaluation settings
 */
typedef union {
    uint32_t val;
    struct {
        uint32_t adap_tst_th:10;    /* Adaptive test cutoff threshold */
        uint32_t rep_tst_th:6;      /* Repetition test cutoff threshold */
        uint32_t adap_tst_en:1;     /* Adaptive test 0x1:enable 0x0:disable*/
        uint32_t rep_tst_en:1;      /* Repetition test 0x1:enable
                                       0x0:disable */
        uint32_t ac_tst_en:1;       /* Autocorrelation test 0x1:enable
                                       0x0:disable */
        uint32_t ac_tst_th:4;       /* Adaptive test cutoff threshold */
    };
} ce_trng_eval_t;

/**
 *  ro settings
 */
typedef union {
    uint32_t val;
    struct {
        uint32_t grp0_taps_en:3;    /* select the 8 schemes of select loops
                                       of RO for group #0 chains */
        uint32_t rsvd3:1;
        uint32_t grp1_taps_en:3;    /* select the 8 schemes of select loops
                                       of RO for group #1 chains */
        uint32_t rsvd7:1;
        uint32_t grp2_taps_en:3;    /* select the 8 schemes of select loops
                                       of RO for group #2 chains */
        uint32_t rsvd11:1;
        uint32_t grp3_taps_en:3;    /* select the 8 schemes of select loops
                                       of RO for group #3 chains */
        uint32_t rsvd15:1;
    };
} ce_trng_ro_t;

/**
 * threshold settings
 */
typedef struct {
    uint32_t vn_err_th:4;       /* VN error counter threshold */
    uint32_t rsvd:4;
    uint32_t crng_err_th:8;     /* CRNG error counter threshold */
    uint32_t rep_err_th:8;      /* REPET error counter threshold */
    uint32_t adap_err_th:8;     /* ADAPT error counter threshold */
} ce_trng_err_thr_t;

/**
 *  TRNG configuration
 */
typedef struct {
    ce_trng_entropy_src_t src;         /* entropy source */
    ce_trng_entropy_sample_t sample;   /* entropy sample */
    ce_trng_ro_t ro;                   /* ro settings */
    ce_trng_postproc_t postproc;       /* post process */
    ce_trng_eval_t eval;               /* evaluation setting */
    ce_trng_err_thr_t thr;             /* error count threshold */
} ce_trng_conf_t;

/**
 * TRNG request structure
 */
typedef struct ce_trng_request {
    bool b_conf;                    /**< whether need to reconfig trng */
    ce_trng_conf_t conf;            /**< configuration */
    void *buf;                      /**< buf for a member */
    size_t size;                    /**< member size in byte */
    size_t nmemb;                   /**< number of members */
    bool bskip;                     /**< skip data on errors */
    void (*on_data)(void *data, size_t len); /**< spit out data */
    void (*on_error)(uint32_t err);      /**< report errors */
} ce_trng_request_t;

/**
 * \brief           This function dumps random data via specified
 *                  config \c req.
 * \param[in] req   Request holding specified trng config and user defined
 *                  parameters. Via this param you can specify config and
 *                  total size of random data for the dump operation.
 *
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_GENERIC on CE hardware failure.
 *                  \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 *                  \c CE_ERROR_NOT_ACCEPTABLE on the wrong host other
 *                     than host#0.
 */
int32_t ce_trng_dump(ce_trng_request_t *req);

/**
 * \brief            This function reads random data from the entropy source
 * \param[out] buf   Buffer to hold the specified \c len bytes random data
 *                   from entropy.
 * \param[in]  len   Length of random data that caller expects to read.
 *
 * \return           \c CE_SUCCESS on success.
 *                   \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 *                   \c CE_ERROR_GENERIC on CE hardware failure.
 */
int32_t ce_trng_read(uint8_t *buf, size_t len);


/**
 *  \brief      This function gets customized TRNG configuration. There is a
 *              default hardcode config in ce_config.c, user can modify it
 *              base on the customized platform.
 *
 *  \return     The customized TRNG config.
 */
extern const ce_trng_conf_t *ce_get_trng_conf(void);


#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_TRNG_H__ */
