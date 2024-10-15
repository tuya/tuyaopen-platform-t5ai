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

#ifndef __CE_ACA_INTERNAL_H__
#define __CE_ACA_INTERNAL_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_lite_bn.h"
#include "ce_lite_ecp.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following are internal functions which are used by ecp/rsa/sm2, but are
 * not exported as CE Lite APIs.
 */
int32_t ce_bn_import_u32(ce_bn_t *bn, uint32_t val);
int32_t ce_bn_cmp_bn(const ce_bn_t *bna, const ce_bn_t *bnb, int32_t *result);
int32_t ce_bn_cmp_u32(const ce_bn_t *bna, uint32_t b, int32_t *result);
int32_t ce_bn_cmp_bn_equal(const ce_bn_t *bna,
                           const ce_bn_t *bnb,
                           int32_t *result);

/**
 * Requirement:
 * A/B bit length <= N bit length
 */
int32_t ce_bn_add_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn);
/**
 * Requirement:
 * A/B bit length <= N bit length
 */
int32_t ce_bn_sub_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn);
/**
 * Requirement:
 * 1. A bit length <= N block bits
 * 2. N is odd.
 */
int32_t ce_bn_inv_mod(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnn);
int32_t ce_bn_bitlen(const ce_bn_t *bn);
int32_t ce_bn_get_bit(const ce_bn_t *bn, int32_t pos);

/**
 * Requirement:
 * A/B bit length <= N bit length
 */
int32_t ce_bn_mul_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn);
/**
 * Requirement:
 * A/B bit length <= N bit length
 */
int32_t ce_bn_exp_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bne,
                      const ce_bn_t *bnn);

/**
 * Requirement:
 * 0 < shift_num <= 64
 */
int32_t ce_bn_shift_r(ce_bn_t *bna, int32_t count);
/**
 * Requirement:
 * A bit length <= N bit length
 */
int32_t ce_bn_mod_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnn);
int32_t ce_bn_mul_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb);
int32_t ce_bn_add_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb);

/**
 * Requirement:
 * A >= B
 */
int32_t ce_bn_sub_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb);

/**
 * Requirement:
 * 1. E bit length > N bit length
 * 2. E bit length <= 2 * ROUND_DOWN((N_bit_len - 1), 128) + 128
 * 3. A bit length < N bit length
 */
int32_t ce_bn_exp_mod_large_e(ce_bn_t *bnr,
                              const ce_bn_t *bna,
                              const ce_bn_t *bne,
                              const ce_bn_t *bnn);
int32_t ce_bn_import_random(ce_bn_t *bn,
                            size_t size,
                            int32_t (*f_rng)(void *, uint8_t *, size_t),
                            void *p_rng);
int32_t ce_bn_import_random_bits(ce_bn_t *bn,
                                 size_t bit_len,
                                 int32_t (*f_rng)(void *, uint8_t *, size_t),
                                 void *p_rng);
int32_t
ce_bn_import_random_max_bit_len(ce_bn_t *bn,
                                size_t max_bit_len,
                                int32_t (*f_rng)(void *, uint8_t *, size_t),
                                void *p_rng);
int32_t ce_bn_copy(ce_bn_t *bn_dst, const ce_bn_t *bn_src);
void ce_bn_dump(const char *name, const ce_bn_t *bn);

int32_t ce_bn_ecp_mul(const ce_bn_t *P,
                      const ce_bn_t *A,
                      const ce_bn_t *G_X,
                      const ce_bn_t *G_Y,
                      const ce_bn_t *G_Z,
                      const ce_bn_t *k,
                      ce_bn_t *R_X,
                      ce_bn_t *R_Y,
                      ce_bn_t *R_Z);
/**
 * Requirement:
 * G1_Z and G2_Z must be 1
 */
int32_t ce_bn_ecp_muladd(const ce_bn_t *P,
                         const ce_bn_t *A,
                         const ce_bn_t *G1_X,
                         const ce_bn_t *G1_Y,
                         const ce_bn_t *G1_Z,
                         const ce_bn_t *k1,
                         const ce_bn_t *G2_X,
                         const ce_bn_t *G2_Y,
                         const ce_bn_t *G2_Z,
                         const ce_bn_t *k2,
                         ce_bn_t *R_X,
                         ce_bn_t *R_Y,
                         ce_bn_t *R_Z);

int32_t ce_ecp_gen_privkey(const ce_ecp_group_t *grp,
                           ce_bn_t *d,
                           int32_t (*f_rng)(void *, uint8_t *, size_t),
                           void *p_rng);
int32_t ce_ecp_copy(ce_ecp_point_t *P, const ce_ecp_point_t *Q);
int32_t ce_ecp_group_copy(ce_ecp_group_t *dst, const ce_ecp_group_t *src);
int32_t ce_ecp_is_zero(ce_ecp_point_t *pt);

int32_t ce_asn1_get_len(uint8_t **p, const uint8_t *limit, size_t *len);
int32_t
ce_asn1_get_tag(uint8_t **p, const uint8_t *limit, size_t *len, int32_t tag);
int32_t ce_asn1_get_mpi(uint8_t **p, const uint8_t *limit, ce_bn_t *X);
int32_t ce_asn1_write_len(uint8_t **p, uint8_t *start, size_t len);
int32_t ce_asn1_write_tag(uint8_t **p, uint8_t *start, uint8_t tag);
int32_t ce_asn1_write_mpi(uint8_t **p, uint8_t *start, const ce_bn_t *X);

static inline size_t ce_md_size(ce_algo_t md_algo)
{
    switch (md_algo) {

#if defined(CE_LITE_SHA256)
    case CE_ALG_SHA256:
        return CE_SHA256_HASH_SIZE;
#endif
#if defined(CE_LITE_SHA224)
    case CE_ALG_SHA224:
        return CE_SHA224_HASH_SIZE;
#endif

#if defined(CE_LITE_SHA1)
    case CE_ALG_SHA1:
        return CE_SHA1_HASH_SIZE;
#endif

#if defined(CE_LITE_SM3)
    case CE_ALG_SM3:
        return CE_SM3_HASH_SIZE;
#endif

    default:
        return 0;
    }
}

/* constant-time buffer comparison */
static inline int32_t ce_safer_memcmp(const uint8_t *a,
                                      const uint8_t *b,
                                      size_t n)
{
    size_t i;
    const uint8_t *A = (const uint8_t *)a;
    const uint8_t *B = (const uint8_t *)b;
    uint8_t diff     = 0;

    for (i = 0; i < n; i++) {
        diff |= A[i] ^ B[i];
    }

    return (int32_t)(diff);
}

#ifdef __cplusplus
}
#endif

#endif /*__CE_ACA_INTERNAL_H__*/
