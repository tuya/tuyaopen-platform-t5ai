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
#include "ce_lite_common.h"
#include "ce_lite_bn.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"

#if defined(CE_LITE_BN)

int32_t ce_bn_init(ce_bn_t *bn)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == bn) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    bn->ctx = pal_calloc(1, sizeof(aca_op_t));
    if (NULL == bn->ctx) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    aca_op_init(bn->ctx);

end:
    return ret;
}

void ce_bn_free(ce_bn_t *bn)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        /* do nothing */
        return;
    }

    aca_op_free(bn->ctx);
    pal_free(bn->ctx);
    bn->ctx = NULL;
}

int32_t ce_bn_import(ce_bn_t *bn, const uint8_t *buf, size_t size)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    if ((0 != size) && (NULL == buf)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_import_bin(bn->ctx, buf, size);
}

int32_t ce_bn_export(const ce_bn_t *bn, uint8_t *buf, size_t size)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    if ((0 != size) && (NULL == buf)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_export_bin(bn->ctx, buf, size);
}

/**
 * The following are internal functions which are used by ecp/rsa/sm2, but are
 * not exported as CE Lite APIs.
 *
 * The coressponding header file is ce_aca_bn_internal.h
 */

/**
 * Used in:
 * 1. RSA SIGN && BLINDING
 * 2. ECP
 * 3. SM2DSA SIGN (Skip)
 */
#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECP)
int32_t ce_bn_import_u32(ce_bn_t *bn, uint32_t val)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_import_u32(bn->ctx, val);
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING) || (CE_LITE_ECP) */

/**
 * Used in:
 * 1. RSA SIGN && VERIFY
 * 2. ECP
 */
#if defined(CE_LITE_RSASSA) || defined(CE_LITE_ECP)
int32_t ce_bn_cmp_bn(const ce_bn_t *bna, const ce_bn_t *bnb, int32_t *result)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) || (NULL == result)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_cmp_bn(bna->ctx, bnb->ctx, result);
}
#endif /* CE_LITE_RSASSA || CE_LITE_ECP */

/**
 * Used in:
 * 1. ECP
 * 2. ECDSA SIGN (Skip)
 * 3. SM2DSA (Skip)
 */
#if defined(CE_LITE_ECP)
int32_t ce_bn_cmp_u32(const ce_bn_t *bna, uint32_t b, int32_t *result)
{
    if (((NULL == bna) || (NULL == bna->ctx)) || (NULL == result)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_cmp_u32(bna->ctx, b, result);
}
#endif /* CE_LITE_ECP */

/**
 * Used in:
 * 1. ECP
 * 2. ECDSA Verify (Skip)
 * 3. SM2DSA Verify (Skip)
 * 4. RSA Sign
 */
#if defined(CE_LITE_ECP) || defined(CE_LITE_RSASSA_SIGN)
int32_t ce_bn_cmp_bn_equal(const ce_bn_t *bna,
                           const ce_bn_t *bnb,
                           int32_t *result)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) || (NULL == result)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_cmp_bn_equal(bna->ctx, bnb->ctx, result);
}
#endif /* CE_LITE_ECP || CE_LITE_RSASSA_SIGN */

/**
 * Used in:
 * 1. ECP
 * 2. ECDSA Sign (Skip)
 * 3. SM2DSA Sign && Verify (Skip)
 */
#if defined(CE_LITE_ECP)
int32_t ce_bn_add_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_add_mod(bnr->ctx, bna->ctx, bnb->ctx, bnn->ctx);
}
#endif /* CE_LITE_ECP */

/**
 * Used in:
 * 1. SM2DSA Sign
 */
#if defined(CE_LITE_SM2DSA_SIGN)
int32_t ce_bn_sub_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_sub_mod(bnr->ctx, bna->ctx, bnb->ctx, bnn->ctx);
}
#endif /* CE_LITE_SM2DSA_SIGN */

/**
 * Used in:
 * 1. SM2DSA Sign
 * 2. ECDSA Sign && Verify
 * 3. RSA Sign && Blinding
 */
#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECDSA) || defined(CE_LITE_SM2DSA_SIGN)
int32_t ce_bn_inv_mod(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_inv_mod(bnr->ctx, bna->ctx, bnn->ctx);
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING)) || (CE_LITE_ECDSA) ||   \
          (CE_LITE_SM2DSA_SIGN) */
/**
 * Used in:
 * 1. ECP
 * 2. RSA Sign && Verify
 */
#if defined(CE_LITE_RSASSA) || defined(CE_LITE_ECP)
int32_t ce_bn_bitlen(const ce_bn_t *bn)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return (int32_t)aca_op_bitlen(bn->ctx);
}
#endif /* CE_LITE_RSASSA || CE_LITE_ECP */
/**
 * Used in:
 * 1. RSA Sign && Verify
 */
#if defined(CE_LITE_RSASSA)
int32_t ce_bn_get_bit(const ce_bn_t *bn, int32_t pos)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_get_bit_value(bn->ctx, (size_t)pos);
}
#endif /* CE_LITE_RSASSA */

/**
 * Used in:
 * 1. RSA Sign && Blinding
 * 2. ECP
 * 3. ECDSA Sign && Verify (Skip)
 * 4. SM2DSA Sign (Skip)
 */
#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECP)
int32_t ce_bn_mul_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bnb,
                      const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_mul_mod(bnr->ctx, bna->ctx, bnb->ctx, bnn->ctx);
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING) || (CE_LITE_ECP) */

/**
 * Used in:
 * 1. RSA Sign && Verify
 */
#if defined(CE_LITE_RSASSA)
int32_t ce_bn_exp_mod(ce_bn_t *bnr,
                      const ce_bn_t *bna,
                      const ce_bn_t *bne,
                      const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bne) || (NULL == bne->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_exp_mod(bnr->ctx, bna->ctx, bne->ctx, bnn->ctx);
}
#endif /* CE_LITE_RSASSA */

/**
 * Used in:
 * 1. ECDSA Sign && Verify
 */
#if defined(CE_LITE_ECDSA)
int32_t ce_bn_shift_r(ce_bn_t *bna, int32_t count)
{
    if ((NULL == bna) || (NULL == bna->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_shift_r(bna->ctx, count);
}
#endif /* CE_LITE_ECDSA */

/**
 * Used in:
 * 1. ECDSA Sign && Verify
 */
#if defined(CE_LITE_ECDSA)
int32_t ce_bn_mod_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_mod_red(bnr->ctx, bna->ctx, bnn->ctx);
}
#endif /* CE_LITE_ECDSA */

/**
 * Used in:
 * 1. RSA Sign && Blinding
 */
#if defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)
int32_t ce_bn_mul_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_mul(bnr->ctx, bna->ctx, bnb->ctx);
}
/**
 * Used in:
 * 1. RSA Sign && Blinding
 */
int32_t ce_bn_add_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_add(bnr->ctx, bna->ctx, bnb->ctx);
}
/**
 * Used in:
 * 1. RSA Sign && Blinding
 */
int32_t ce_bn_sub_bn(ce_bn_t *bnr, const ce_bn_t *bna, const ce_bn_t *bnb)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bnb) || (NULL == bnb->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_sub(bnr->ctx, bna->ctx, bnb->ctx);
}
/**
 * Used in:
 * 1. RSA Sign && Blinding
 */
int32_t ce_bn_exp_mod_large_e(ce_bn_t *bnr,
                              const ce_bn_t *bna,
                              const ce_bn_t *bne,
                              const ce_bn_t *bnn)
{
    if (((NULL == bna) || (NULL == bna->ctx)) ||
        ((NULL == bne) || (NULL == bne->ctx)) ||
        ((NULL == bnn) || (NULL == bnn->ctx)) ||
        ((NULL == bnr) || (NULL == bnr->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_exp_mod_large_e(bnr->ctx, bna->ctx, bne->ctx, bnn->ctx);
}
#endif /* CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING */

/**
 * Used in:
 * 1. RSA Sign && Blinding
 */
#if defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)
int32_t ce_bn_import_random(ce_bn_t *bn,
                            size_t size,
                            int32_t (*f_rng)(void *, uint8_t *, size_t),
                            void *p_rng)
{
    int32_t ret  = CE_SUCCESS;
    uint8_t *tmp = NULL;

    if ((NULL == bn) || (NULL == bn->ctx) || (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    tmp = pal_calloc(1, size);
    if (NULL == tmp) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    ret = f_rng(p_rng, tmp, size);
    if (CE_SUCCESS != ret) {
        ret = CE_ERROR_GEN_RANDOM;
        goto end;
    }

    ret = aca_op_import_bin(bn->ctx, tmp, size);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = CE_SUCCESS;
end:
    if (NULL != tmp) {
        pal_free(tmp);
    }
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING */

/**
 * Used in:
 * 1. ECP
 */
#if defined(CE_LITE_ECP_PRIVATE) ||                                            \
    (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING))
static int32_t
bn_import_random_bits(ce_bn_t *bn,
                      size_t bit_len,
                      int32_t (*f_rng)(void *, uint8_t *, size_t),
                      void *p_rng,
                      int32_t fix_msb)
{
    int32_t ret  = CE_SUCCESS;
    uint8_t *tmp = NULL;
    size_t size  = 0;
    size_t i     = 0;

    if ((NULL == bn) || (NULL == bn->ctx) || (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    size = (bit_len + 7) / 8;
    tmp  = pal_calloc(1, size);
    if (NULL == tmp) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    ret = f_rng(p_rng, tmp, size);
    if (CE_SUCCESS != ret) {
        ret = CE_ERROR_GEN_RANDOM;
        goto end;
    }

    /* Adjust byte[0] to fit bit length */
    for (i = 0; i < (size * 8) - bit_len; i++) {
        tmp[0] &= (~(((uint8_t)1) << (7 - i)));
    }
    if (fix_msb) {
        /* always set MSB to 1 if fix_msb */
        if ((bit_len & 7) == 0) {
            tmp[0] |= (((uint8_t)1) << 7);
        } else {
            tmp[0] |= (((uint8_t)1) << ((bit_len & 7) - 1));
        }
    }

    ret = aca_op_import_bin(bn->ctx, tmp, size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Double check bitlength */
    if (fix_msb) {
        PAL_ASSERT(aca_op_bitlen(bn->ctx) == bit_len);
    } else {
        PAL_ASSERT(aca_op_bitlen(bn->ctx) <= bit_len);
    }

    ret = CE_SUCCESS;
end:
    if (NULL != tmp) {
        pal_free(tmp);
    }
    return ret;
}
#endif /* CE_LITE_ECP_PRIVATE || (CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING) */

/**
 * Used in:
 * 1. ECP && Blinding
 */

#if (defined(CE_LITE_ECP_PRIVATE) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)
int32_t ce_bn_import_random_bits(ce_bn_t *bn,
                                 size_t bit_len,
                                 int32_t (*f_rng)(void *, uint8_t *, size_t),
                                 void *p_rng)
{
    return bn_import_random_bits(bn, bit_len, f_rng, p_rng, 1);
}
#endif /* (CE_LITE_ECP_PRIVATE && CE_LITE_BLINDING) ||                         \
          (CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING)   */

/**
 * Used in:
 * 1. ECP Gen Private
 */
#if defined(CE_LITE_ECP_PRIVATE)
int32_t
ce_bn_import_random_max_bit_len(ce_bn_t *bn,
                                size_t max_bit_len,
                                int32_t (*f_rng)(void *, uint8_t *, size_t),
                                void *p_rng)
{
    return bn_import_random_bits(bn, max_bit_len, f_rng, p_rng, 0);
}
#endif /* CE_LITE_ECP_PRIVATE */

/**
 * Used in:
 * 1. ECP
 * 2. RSA
 * 3. ECDSA
 * 4. SM2DSA
 */
#if defined(CE_LITE_ECP) || defined(CE_LITE_RSASSA)
int32_t ce_bn_copy(ce_bn_t *bn_dst, const ce_bn_t *bn_src)
{
    if (((NULL == bn_dst) || (NULL == bn_dst->ctx)) ||
        ((NULL == bn_src) || (NULL == bn_src->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_copy(bn_dst->ctx, bn_src->ctx);
}
#endif /* CE_LITE_ECP || CE_LITE_RSASSA */

void ce_bn_dump(const char *name, const ce_bn_t *bn)
{
    if ((NULL == bn) || (NULL == bn->ctx)) {
        return;
    }
    aca_op_dump(name, bn->ctx);
}

#if defined(CE_LITE_ECP_PRIVATE)
int32_t ce_bn_ecp_mul(const ce_bn_t *P,
                      const ce_bn_t *A,
                      const ce_bn_t *G_X,
                      const ce_bn_t *G_Y,
                      const ce_bn_t *G_Z,
                      const ce_bn_t *k,
                      ce_bn_t *R_X,
                      ce_bn_t *R_Y,
                      ce_bn_t *R_Z)
{
    if (((NULL == P) || (NULL == P->ctx)) ||
        ((NULL == A) || (NULL == A->ctx)) ||
        ((NULL == G_X) || (NULL == G_X->ctx)) ||
        ((NULL == G_Y) || (NULL == G_Y->ctx)) ||
        ((NULL == G_Z) || (NULL == G_Z->ctx)) ||
        ((NULL == k) || (NULL == k->ctx)) ||
        ((NULL == R_X) || (NULL == R_X->ctx)) ||
        ((NULL == R_Y) || (NULL == R_Y->ctx)) ||
        ((NULL == R_Z) || (NULL == R_Z->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_ecp_mul(P->ctx,
                          A->ctx,
                          G_X->ctx,
                          G_Y->ctx,
                          G_Z->ctx,
                          k->ctx,
                          R_X->ctx,
                          R_Y->ctx,
                          R_Z->ctx);
}
#endif /* CE_LITE_ECP_PRIVATE */

#if defined(CE_LITE_ECP)
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
                         ce_bn_t *R_Z)
{
    if (((NULL == P) || (NULL == P->ctx)) ||
        ((NULL == A) || (NULL == A->ctx)) ||
        ((NULL == G1_X) || (NULL == G1_X->ctx)) ||
        ((NULL == G1_Y) || (NULL == G1_Y->ctx)) ||
        ((NULL == G1_Z) || (NULL == G1_Z->ctx)) ||
        ((NULL == k1) || (NULL == k1->ctx)) ||
        ((NULL == G2_X) || (NULL == G2_X->ctx)) ||
        ((NULL == G2_Y) || (NULL == G2_Y->ctx)) ||
        ((NULL == G2_Z) || (NULL == G2_Z->ctx)) ||
        ((NULL == k2) || (NULL == k2->ctx)) ||
        ((NULL == R_X) || (NULL == R_X->ctx)) ||
        ((NULL == R_Y) || (NULL == R_Y->ctx)) ||
        ((NULL == R_Z) || (NULL == R_Z->ctx))) {
        return CE_ERROR_BAD_PARAMS;
    }
    return aca_op_ecp_muladd(P->ctx,
                             A->ctx,
                             G1_X->ctx,
                             G1_Y->ctx,
                             G1_Z->ctx,
                             k1->ctx,
                             G2_X->ctx,
                             G2_Y->ctx,
                             G2_Z->ctx,
                             k2->ctx,
                             R_X->ctx,
                             R_Y->ctx,
                             R_Z->ctx);
}
#endif /* CE_LITE_ECP */

#endif /* CE_LITE_BN */
