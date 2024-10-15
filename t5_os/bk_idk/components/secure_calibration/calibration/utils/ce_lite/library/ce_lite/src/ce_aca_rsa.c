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
#include "ce_lite_hash.h"
#include "ce_lite_rsa.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"
#include "ce_aca_internal.h"

#if defined(CE_LITE_RSASSA)

int32_t ce_rsa_init(ce_rsa_context_t *ctx)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        return CE_ERROR_BAD_PARAMS;
    }

    pal_memset(ctx, 0, sizeof(ce_rsa_context_t));

    ret = ce_bn_init(&(ctx->N));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(ctx->E));
    CHECK_SUCCESS_GOTO(ret, end);

#if defined(CE_LITE_RSASSA_SIGN)
    ret = ce_bn_init(&(ctx->D));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(ctx->P));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(ctx->Q));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(ctx->Vi));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(ctx->Vf));
    CHECK_SUCCESS_GOTO(ret, end);
#endif /* CE_LITE_RSASSA_SIGN */

end:
    if (ret != CE_SUCCESS) {
        /* ce_bn_free is safe free even bn is not initialized */
        ce_bn_free(&(ctx->N));
        ce_bn_free(&(ctx->E));
#if defined(CE_LITE_RSASSA_SIGN)
        ce_bn_free(&(ctx->D));
        ce_bn_free(&(ctx->P));
        ce_bn_free(&(ctx->Q));
        ce_bn_free(&(ctx->Vi));
        ce_bn_free(&(ctx->Vf));
#endif /* CE_LITE_RSASSA_SIGN */
    }
    return ret;
}

void ce_rsa_free(ce_rsa_context_t *ctx)
{
    if (NULL == ctx) {
        return;
    }
    ce_bn_free(&(ctx->N));
    ce_bn_free(&(ctx->E));
#if defined(CE_LITE_RSASSA_SIGN)
    ce_bn_free(&(ctx->D));
    ce_bn_free(&(ctx->P));
    ce_bn_free(&(ctx->Q));
    ce_bn_free(&(ctx->Vi));
    ce_bn_free(&(ctx->Vf));
#endif /* CE_LITE_RSASSA_SIGN */
    pal_memset(ctx, 0, sizeof(ce_rsa_context_t));
    return;
}

int32_t ce_rsa_import(ce_rsa_context_t *ctx,
                      const ce_bn_t *N,
                      const ce_bn_t *P,
                      const ce_bn_t *Q,
                      const ce_bn_t *D,
                      const ce_bn_t *E)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (N != NULL) {
        ret = ce_bn_copy(&(ctx->N), N);
        CHECK_SUCCESS_GOTO(ret, end);
        ret = ce_bn_bitlen((const ce_bn_t *)(&(ctx->N)));
        PAL_ASSERT(ret > 0);
        ctx->len = ((size_t)ret + 7) / 8;
    }

    if (E != NULL) {
        ret = ce_bn_copy(&(ctx->E), E);
        CHECK_SUCCESS_GOTO(ret, end);
    }

#if defined(CE_LITE_RSASSA_SIGN)
    if (D != NULL) {
        ret = ce_bn_copy(&(ctx->D), D);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    if (P != NULL) {
        ret = ce_bn_copy(&(ctx->P), P);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    if (Q != NULL) {
        ret = ce_bn_copy(&(ctx->Q), Q);
        CHECK_SUCCESS_GOTO(ret, end);
    }
#endif /* CE_LITE_RSASSA_SIGN */

    ret = CE_SUCCESS;
end:
    return ret;
}

int32_t ce_rsa_import_raw(ce_rsa_context_t *ctx,
                          const uint8_t *N,
                          size_t N_len,
                          const uint8_t *P,
                          size_t P_len,
                          const uint8_t *Q,
                          size_t Q_len,
                          const uint8_t *D,
                          size_t D_len,
                          const uint8_t *E,
                          size_t E_len)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (N != NULL) {
        ret = ce_bn_import(&(ctx->N), N, N_len);
        CHECK_SUCCESS_GOTO(ret, end);
        ret = ce_bn_bitlen((const ce_bn_t *)(&(ctx->N)));
        PAL_ASSERT(ret > 0);
        ctx->len = ((size_t)ret + 7) / 8;
    }

    if (E != NULL) {
        ret = ce_bn_import(&(ctx->E), E, E_len);
        CHECK_SUCCESS_GOTO(ret, end);
    }

#if defined(CE_LITE_RSASSA_SIGN)
    if (D != NULL) {
        ret = ce_bn_import(&(ctx->D), D, D_len);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    if (P != NULL) {
        ret = ce_bn_import(&(ctx->P), P, P_len);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    if (Q != NULL) {
        ret = ce_bn_import(&(ctx->Q), Q, Q_len);
        CHECK_SUCCESS_GOTO(ret, end);
    }
#endif /* CE_LITE_RSASSA_SIGN */

    ret = CE_SUCCESS;
end:
    return ret;
}

/**
 * Check the RSA context. This function checks both private and public cases.
 */
static int32_t rsa_check_context(const ce_rsa_context_t *ctx, int32_t is_priv)
{
    int32_t ret    = CE_SUCCESS;
    int32_t result = 0;

    ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->N));
    PAL_ASSERT(ret >= 0);
    if (0 == ret) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    if ((8 * (ctx->len) != (size_t)(ret)) ||
        (8 * (ctx->len) > CE_RSA_MAX_KEY_BITS) ||
        (8 * (ctx->len) < CE_RSA_MIN_KEY_BITS)) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    /*
     * 1. Modular exponentiation needs positive, odd moduli.
     */
    /* Modular exponentiation wrt. N is always used for
     * RSA public key operations. */
    ret = ce_bn_get_bit((const ce_bn_t *)(&ctx->N), 0);
    PAL_ASSERT(ret >= 0);
    if (ret == 0) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

#if defined(CE_LITE_RSASSA_SIGN)
    /**
     * Check private key. Here also assume that D bits MUST > 2
     */
    if (is_priv) {
        ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->D));
        PAL_ASSERT(ret >= 0);
        if ((0 == ret) || (ret < 2) || (ret > (8 * (ctx->len)))) {
            ret = CE_ERROR_BAD_INPUT_DATA;
            goto end;
        }

        ret = ce_bn_cmp_bn((const ce_bn_t *)(&ctx->D),
                           (const ce_bn_t *)(&ctx->N), &result);
        CHECK_SUCCESS_GOTO(ret, end);
        if (result >= 0) {
            ret = CE_ERROR_BAD_INPUT_DATA;
            goto end;
        }
    }
#else  /* !CE_LITE_RSASSA_SIGN */
    PAL_ASSERT(!is_priv);
#endif /* CE_LITE_RSASSA_SIGN */

    /* Check public E. Minimal E is 3, so E bit length > 2 */
    ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->E));
    PAL_ASSERT(ret >= 0);
    if ((0 == ret) || (ret < 2) || (ret > (8 * (ctx->len)))) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = ce_bn_cmp_bn((const ce_bn_t *)(&ctx->E), (const ce_bn_t *)(&ctx->N),
                       &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result >= 0) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    /* E must be odd */
    ret = ce_bn_get_bit((const ce_bn_t *)(&ctx->E), 0);
    PAL_ASSERT(ret >= 0);
    if (0 == ret) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = CE_SUCCESS;
end:
    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)
/**
 * Check the RSA context. This function checks both private and public cases.
 */
static int32_t rsa_check_blinding_context(const ce_rsa_context_t *ctx)
{
    int32_t ret = CE_SUCCESS;

    /* blinding requires P and Q != NULL, and P/Q bitlength < N bitlength / 2 */

    ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->P));
    PAL_ASSERT(ret >= 0);
    if ((0 == ret) || (ret > 4 * (ctx->len))) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->Q));
    PAL_ASSERT(ret >= 0);
    if ((0 == ret) || (ret > 4 * (ctx->len))) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = CE_SUCCESS;
end:
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING */

int32_t ce_rsa_check_pubkey(const ce_rsa_context_t *ctx)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = rsa_check_context(ctx, 0);
    if (CE_ERROR_BAD_INPUT_DATA == ret) {
        /* convert bad input data to invalid key */
        ret = CE_ERROR_INVAL_KEY;
    }
    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN)
int32_t ce_rsa_check_privkey(const ce_rsa_context_t *ctx)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = rsa_check_context(ctx, 1);
    if (CE_ERROR_BAD_INPUT_DATA == ret) {
        /* convert bad input data to invalid key */
        ret = CE_ERROR_INVAL_KEY;
    }
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */

#if defined(CE_LITE_RSASSA_SIGN)
int32_t ce_rsa_check_pub_priv(const ce_rsa_context_t *pub,
                              const ce_rsa_context_t *prv)
{
    int32_t ret    = CE_SUCCESS;
    int32_t result = 0;

    if ((NULL == pub) || (NULL == prv)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = rsa_check_context(pub, 0);
    if (CE_ERROR_BAD_INPUT_DATA == ret) {
        /* convert bad input data to invalid key */
        ret = CE_ERROR_INVAL_KEY;
    }
    CHECK_SUCCESS_GOTO(ret, end);

    ret = rsa_check_context(prv, 1);
    if (CE_ERROR_BAD_INPUT_DATA == ret) {
        /* convert bad input data to invalid key */
        ret = CE_ERROR_INVAL_KEY;
    }
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(pub->N)),
                             (const ce_bn_t *)(&(prv->N)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(pub->E)),
                             (const ce_bn_t *)(&(prv->E)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    ret = CE_SUCCESS;
end:
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */

int32_t ce_rsa_public(ce_rsa_context_t *ctx,
                      const uint8_t *input,
                      uint8_t *output)
{
    int32_t ret     = CE_SUCCESS;
    ce_bn_t tmp_in  = {NULL};
    ce_bn_t tmp_out = {NULL};
    int32_t result  = 0;

    if ((NULL == ctx) || (NULL == input) || (NULL == output)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_init(&tmp_in);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp_out);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_import(&tmp_in, input, ctx->len);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_cmp_bn(&tmp_in, (const ce_bn_t *)(&(ctx->N)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result >= 0) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = ce_bn_exp_mod(&tmp_out, (const ce_bn_t *)(&tmp_in),
                        (const ce_bn_t *)(&(ctx->E)),
                        (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_export((const ce_bn_t *)(&tmp_out), output, ctx->len);
    CHECK_SUCCESS_GOTO(ret, end);

end:
    ce_bn_free(&tmp_in);
    ce_bn_free(&tmp_out);
    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN)
#if defined(CE_LITE_BLINDING)
/*
 * Generate or update blinding values, see section 10 of:
 *  KOCHER, Paul C. Timing attacks on implementations of Diffie-Hellman, RSA,
 *  DSS, and other systems. In : Advances in Cryptology-CRYPTO'96. Springer
 *  Berlin Heidelberg, 1996. p. 104-113.
 */
static int32_t rsa_prepare_blinding(ce_rsa_context_t *ctx,
                                    int32_t (*f_rng)(void *, uint8_t *, size_t),
                                    void *p_rng)
{
    int32_t ret   = CE_SUCCESS;
    int32_t count = 0;

    ret = ce_bn_bitlen((const ce_bn_t *)(&ctx->Vf));
    PAL_ASSERT(ret >= 0);

    if (0 != ret) {
        /* Vf/Vi already contains blinding data, check */
        PAL_ASSERT(ce_bn_bitlen((const ce_bn_t *)(&ctx->Vi)) > 0);

        /* We already have blinding values, just update them by squaring */
        ret = ce_bn_mul_mod(&(ctx->Vi), (const ce_bn_t *)(&(ctx->Vi)),
                            (const ce_bn_t *)(&(ctx->Vi)),
                            (const ce_bn_t *)(&(ctx->N)));
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_bn_mul_mod(&(ctx->Vf), (const ce_bn_t *)(&(ctx->Vf)),
                            (const ce_bn_t *)(&(ctx->Vf)),
                            (const ce_bn_t *)(&(ctx->N)));
        CHECK_SUCCESS_GOTO(ret, end);

        ret = CE_SUCCESS;
        goto end;
    } else {
        /* do nothing */
    }

    do {
        if (count++ > CE_RANDOM_RETRY_CNT) {
            ret = CE_ERROR_GEN_RANDOM;
            goto end;
        }

        ret = ce_bn_import_random_bits(&(ctx->Vf), (8 * ctx->len) - 1, f_rng,
                                       p_rng);
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_bn_inv_mod(&(ctx->Vi), (const ce_bn_t *)(&(ctx->Vf)),
                            (const ce_bn_t *)(&(ctx->N)));
        if ((CE_SUCCESS != ret) && (CE_ERROR_NOT_ACCEPTABLE != ret)) {
            goto end;
        }
    } while (CE_ERROR_NOT_ACCEPTABLE == ret);

    /**
     * Blinding value: Vi =  Vf^(-e) mod N
     * (Vi already contains Vf^-1 at this point)
     */
    ret = ce_bn_exp_mod(&(ctx->Vi), (const ce_bn_t *)(&(ctx->Vi)),
                        (const ce_bn_t *)(&(ctx->E)),
                        (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = CE_SUCCESS;
end:
    return ret;
}

/*
 * Exponent blinding supposed to prevent side-channel attacks using multiple
 * traces of measurements to recover the RSA key. The more collisions are there,
 * the more bits of the key can be recovered. See [3].
 *
 * Collecting n collisions with m bit long blinding value requires 2^(m-m/n)
 * observations on avarage.
 *
 * For example with 28 byte blinding to achieve 2 collisions the adversary has
 * to make 2^112 observations on avarage.
 *
 * (With the currently (as of 2017 April) known best algorithms breaking 2048
 * bit RSA requires approximately as much time as trying out 2^112 random keys.
 * Thus in this sense with 28 byte blinding the security is not reduced by
 * side-channel attacks like the one in [3])
 *
 * This countermeasure does not help if the key recovery is possible with a
 * single trace.
 */
#define RSA_EXPONENT_BLINDING 28

static int32_t rsa_private_core(ce_rsa_context_t *ctx,
                                ce_bn_t *in,
                                ce_bn_t *out,
                                int32_t (*f_rng)(void *, uint8_t *, size_t),
                                void *p_rng)
{
    int32_t ret       = 0;
    ce_bn_t const1    = {NULL};
    ce_bn_t tmp1      = {NULL};
    ce_bn_t tmp2      = {NULL};
    ce_bn_t tmp3      = {NULL};
    int32_t reset_tmp = 0;

    if (NULL == f_rng) {
        /* f_rng is NULL, no blinding */
        ret = ce_bn_exp_mod(out, (const ce_bn_t *)(in),
                            (const ce_bn_t *)(&(ctx->D)),
                            (const ce_bn_t *)(&(ctx->N)));
        CHECK_SUCCESS_GOTO(ret, end);
        ret = CE_SUCCESS;
        goto end;
    }

    /* Check blinding context here */
    ret = rsa_check_blinding_context((const ce_rsa_context_t *)ctx);
    CHECK_SUCCESS_GOTO(ret, end);

    /* prepare blinding context */
    ret = rsa_prepare_blinding(ctx, f_rng, p_rng);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_init(&const1);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp1);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp2);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp3);
    CHECK_SUCCESS_GOTO(ret, end);

    reset_tmp = 1;

    /* tmp3 = in * Vi */
    ret =
        ce_bn_mul_mod(&tmp3, (const ce_bn_t *)in, (const ce_bn_t *)(&(ctx->Vi)),
                      (const ce_bn_t *)(&(ctx->N)));

    ret = ce_bn_import_u32(&const1, 1);
    CHECK_SUCCESS_GOTO(ret, end);

    /* import rnd with random */
    ret = ce_bn_import_random(&tmp1, RSA_EXPONENT_BLINDING, f_rng, p_rng);
    CHECK_SUCCESS_GOTO(ret, end);

    /* tmp2 = P - 1 */
    ret = ce_bn_sub_bn(&tmp2, (const ce_bn_t *)(&(ctx->P)),
                       (const ce_bn_t *)(&const1));
    CHECK_SUCCESS_GOTO(ret, end);

    /* tmp1 = tmp1 * tmp2 */
    ret =
        ce_bn_mul_bn(&tmp1, (const ce_bn_t *)(&tmp1), (const ce_bn_t *)(&tmp2));
    CHECK_SUCCESS_GOTO(ret, end);

    /* tmp2 = Q - 1 */
    ret = ce_bn_sub_bn(&tmp2, (const ce_bn_t *)(&(ctx->Q)),
                       (const ce_bn_t *)(&const1));
    CHECK_SUCCESS_GOTO(ret, end);

    /* tmp1 = tmp1 * tmp2 */
    ret =
        ce_bn_mul_bn(&tmp1, (const ce_bn_t *)(&tmp1), (const ce_bn_t *)(&tmp2));
    CHECK_SUCCESS_GOTO(ret, end);

    /* tmp1 = D + tmp1 */
    ret = ce_bn_add_bn(&tmp1, (const ce_bn_t *)(&(ctx->D)),
                       (const ce_bn_t *)(&tmp1));
    CHECK_SUCCESS_GOTO(ret, end);

    /* set tmp2 to 0 before exp mode*/
    ret = ce_bn_import_u32(&tmp2, 0);
    CHECK_SUCCESS_GOTO(ret, end);

    /* exp mode with large e(tmp1) */
    ret = ce_bn_exp_mod_large_e(&tmp2, (const ce_bn_t *)(&tmp3),
                                (const ce_bn_t *)(&tmp1),
                                (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    /* Unblinding */
    /* out = tmp2 * Vf mod N */
    ret = ce_bn_mul_mod(out, (const ce_bn_t *)(&tmp2),
                        (const ce_bn_t *)(&(ctx->Vf)),
                        (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);
end:
    /* set tmp1 and tmp2 to 0 before exit */
    if (reset_tmp) {
        ret = ce_bn_import_u32(&tmp1, 0);
        CHECK_SUCCESS_GOTO(ret, end);
        ret = ce_bn_import_u32(&tmp2, 0);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    ce_bn_free(&const1);
    ce_bn_free(&tmp1);
    ce_bn_free(&tmp2);
    ce_bn_free(&tmp3);
    return ret;
}

#else  /* !CE_LITE_BLINDING */

static int32_t rsa_private_core(ce_rsa_context_t *ctx,
                                ce_bn_t *in,
                                ce_bn_t *out,
                                int32_t (*f_rng)(void *, uint8_t *, size_t),
                                void *p_rng)
{
    int32_t ret = 0;

    (void)(f_rng);
    (void)(p_rng);

    ret =
        ce_bn_exp_mod(out, (const ce_bn_t *)(in), (const ce_bn_t *)(&(ctx->D)),
                      (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);
end:
    return ret;
}
#endif /* CE_LITE_BLINDING */
#endif /* CE_LITE_RSASSA_SIGN */

#if defined(CE_LITE_RSASSA_SIGN)
/*
 * Do an RSA private key operation
 */
int32_t ce_rsa_private(ce_rsa_context_t *ctx,
                       int32_t (*f_rng)(void *, uint8_t *, size_t),
                       void *p_rng,
                       uint8_t *input,
                       uint8_t *output)
{
    int32_t ret     = CE_SUCCESS;
    int32_t result  = 0;
    ce_bn_t tmp_in  = {NULL};
    ce_bn_t tmp_out = {NULL};
    ce_bn_t C       = {NULL};

    if ((NULL == ctx) || (NULL == input) || (NULL == output)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_init(&tmp_in);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp_out);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&C);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_import(&tmp_in, input, ctx->len);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_cmp_bn(&tmp_in, (const ce_bn_t *)(&(ctx->N)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result >= 0) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    ret = rsa_private_core(ctx, &tmp_in, &tmp_out, f_rng, p_rng);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Verify the result to prevent glitching attacks. */
    ret = ce_bn_exp_mod(&C, (const ce_bn_t *)(&tmp_out),
                        (const ce_bn_t *)(&(ctx->E)),
                        (const ce_bn_t *)(&(ctx->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&C), (const ce_bn_t *)(&tmp_in),
                             &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_VERIFY_SIG;
        goto end;
    }

    ret = ce_bn_export((const ce_bn_t *)(&tmp_out), output, ctx->len);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = CE_SUCCESS;
end:
    ce_bn_free(&tmp_in);
    ce_bn_free(&tmp_out);
    ce_bn_free(&C);
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */

#if defined(CE_LITE_RSA_PKCS_V21)
/**
 * Generate and apply the MGF1 operation (from PKCS#1 v2.1) to a buffer.
 *
 * \param dst       buffer to mask
 * \param dlen      length of destination buffer
 * \param src       source of the mask generation
 * \param slen      length of the source buffer
 * \param md        message digest type to use
 */
static int32_t
mgf_mask(uint8_t *dst, size_t dlen, uint8_t *src, size_t slen, ce_algo_t md)
{
    int32_t ret                    = CE_SUCCESS;
    uint8_t mask[CE_MAX_HASH_SIZE] = {0};
    uint8_t counter[4]             = {0};
    uint8_t *p                     = NULL;
    uint32_t hlen                  = 0;
    size_t i                       = 0;
    size_t use_len                 = 0;
    ce_dgst_ctx_t dgst_ctx         = {NULL};
    int32_t dgst_need_finish       = 0;

    pal_memset(mask, 0, sizeof(mask));
    pal_memset(counter, 0, sizeof(counter));

    hlen = ce_md_size(md);
    PAL_ASSERT(hlen != 0);

    ret = ce_dgst_init(&dgst_ctx, md);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Generate and apply dbMask */
    p = dst;
    while (dlen > 0) {
        use_len = hlen;
        if (dlen < hlen) {
            use_len = dlen;
        }

        ret = ce_dgst_start(&dgst_ctx);
        CHECK_SUCCESS_GOTO(ret, end);
        dgst_need_finish = 1;

        ret = ce_dgst_update(&dgst_ctx, src, slen);
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_dgst_update(&dgst_ctx, counter, 4);
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_dgst_finish(&dgst_ctx, mask);
        CHECK_SUCCESS_GOTO(ret, end);
        dgst_need_finish = 0;

        for (i = 0; i < use_len; ++i) {
            *p++ ^= mask[i];
        }

        counter[3]++;

        dlen -= use_len;
    }

end:
    pal_memset(mask, 0, sizeof(mask));
    if (dgst_need_finish) {
        (void)ce_dgst_finish(&dgst_ctx, NULL);
    }
    ce_dgst_free(&dgst_ctx);
    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN)
/* Do pss encoding as from PKCS#1 v2.1 9.1.1 */
static int32_t rsa_pss_encode(uint8_t *output,
                              const uint8_t *input,
                              size_t input_len,
                              size_t rsa_key_bits,
                              int32_t target_salt_len,
                              ce_algo_t hash_id,
                              int32_t (*f_rng)(void *, uint8_t *, size_t),
                              void *p_rng)
{
    int32_t ret     = CE_SUCCESS;
    size_t embits   = 0;
    size_t emlen    = 0;
    size_t sign_len = 0;
    size_t slen     = 0;
    size_t slen_max = 0;
    size_t hlen     = 0;
    uint8_t *h      = NULL;
    uint8_t *db     = NULL;
    uint8_t *salt   = NULL;
    uint8_t *m_     = NULL;
    size_t tmp_len  = 0;

    embits   = rsa_key_bits - 1;
    sign_len = (rsa_key_bits + 7) / 8;

    /* calc emlen = ceil(embits/8) */
    emlen = CE_ROUND_UP(embits, 8) / 8;

    /* double check emlen vs sign_len */
    if ((embits & 0x7) == 0) {
        PAL_ASSERT(emlen == sign_len - 1);
    } else {
        PAL_ASSERT(emlen == sign_len);
    }

    hlen = ce_md_size(hash_id);
    PAL_ASSERT(hlen > 0);

    /* Check hlen, emLen < hLen + sLen + 2, the minimal sLen is hashlen - 2 */
    if (emlen < hlen + hlen - 2 + 2) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }
    if (emlen >= hlen + hlen + 2) {
        slen_max = hlen;
    } else {
        slen_max = emlen - hlen - 2;
    }

    PAL_ASSERT(slen_max >= hlen - 2);

    /* target_salt_len only support max and positive length */
    if (target_salt_len == CE_RSA_SALT_LEN_MAX) {
        slen = slen_max;
    } else if (target_salt_len < 0) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    } else {
        /* check target_salt_len >= hlen - 2 && target_salt_len <= slen_max */
        if ((target_salt_len < hlen - 2) || (target_salt_len > slen_max)) {
            ret = CE_ERROR_BAD_INPUT_DATA;
            goto end;
        }
        slen = target_salt_len;
    }

    /* double check salt length. This accord with Step3 */
    PAL_ASSERT(emlen >= hlen + slen + 2);

    /* 4. Generate salt */
    if (slen) {
        salt = pal_calloc(1, slen);
        if (NULL == salt) {
            ret = CE_ERROR_OOM;
            goto end;
        }
        ret = f_rng(p_rng, salt, slen);
        if (CE_SUCCESS != ret) {
            ret = CE_ERROR_GEN_RANDOM;
            goto end;
        }
    } else {
        salt = NULL;
    }

    /* 5. Format M' */
    tmp_len = 8 + input_len + slen;
    m_      = pal_calloc(1, tmp_len);
    if (NULL == m_) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    pal_memset(m_, 0, 8);
    pal_memcpy(m_ + 8, input, input_len);
    if (slen) {
        pal_memcpy(m_ + 8 + input_len, salt, slen);
    } else {
        /* do nothing */
    }
    /* 6. Calculate H = Hash(M') */
    h = pal_calloc(1, CE_MAX_HASH_SIZE);
    if (NULL == h) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    ret = ce_dgst(hash_id, (const uint8_t *)(m_), tmp_len, h);
    CHECK_SUCCESS_GOTO(ret, end);

    /* 7 && 8. Format DB = PS || 0x01 || salt */
    db = pal_calloc(1, emlen - hlen - 1);
    if (NULL == db) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    /* set ps. tmp_len = ps length */
    tmp_len = emlen - slen - hlen - 2;
    if (tmp_len) {
        pal_memset(db, 0, tmp_len);
    }
    db[tmp_len] = 0x01;
    if (slen) {
        pal_memcpy(db + tmp_len + 1, salt, slen);
    }

    /* 9 && 10: maskedDB = DB xor dbMask */
    ret = mgf_mask(db, emlen - hlen - 1, h, hlen, hash_id);
    CHECK_SUCCESS_GOTO(ret, end);

    /* 11. Set the leftmost 8emLen - emBits bits of the leftmost octet in
     * maskedDB to zero. */
    db[0] &= (0xFF >> (8 * emlen - embits));

    /* 12 && 13: output final signature */
    /* tmp_len is db length */
    tmp_len = emlen - hlen - 1;
    pal_memset(output, 0, sign_len);
    pal_memcpy(output + (sign_len - emlen), db, tmp_len);
    pal_memcpy(output + (sign_len - emlen + tmp_len), h, ce_md_size(hash_id));
    output[sign_len - 1] = 0xbc;

end:
    if (salt) {
        pal_free(salt);
    }
    if (m_) {
        pal_free(m_);
    }
    if (h) {
        pal_free(h);
    }
    if (db) {
        pal_free(db);
    }
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */

static int32_t rsa_pss_decode(const uint8_t *sign,
                              const uint8_t *input,
                              size_t input_len,
                              size_t rsa_key_bits,
                              int32_t target_salt_len,
                              ce_algo_t hash_id)
{
    int32_t ret        = CE_SUCCESS;
    size_t embits      = 0;
    size_t emlen       = 0;
    size_t sign_len    = 0;
    size_t slen        = 0;
    size_t expect_slen = 0;
    const uint8_t *em  = NULL;
    const uint8_t *h   = NULL;
    uint8_t *db        = NULL;
    size_t db_len      = 0;
    uint8_t *salt      = NULL;
    uint8_t *h_        = NULL;
    size_t i           = 0;
    uint8_t *m_        = NULL;
    size_t tmp_len     = 0;
    size_t hlen        = 0;

    embits   = rsa_key_bits - 1;
    sign_len = (rsa_key_bits + 7) / 8;

    /* calc emlen = ceil(embits/8) */
    emlen = CE_ROUND_UP(embits, 8) / 8;

    /* double check emlen vs sign_len */
    if ((embits & 0x7) == 0) {
        PAL_ASSERT(emlen == sign_len - 1);
        em = sign + 1;
    } else {
        PAL_ASSERT(emlen == sign_len);
        em = sign;
    }

    hlen = ce_md_size(hash_id);
    PAL_ASSERT(hlen > 0);

    /* 3. Check emLen < hLen + sLen + 2. we don't have sLen currently */
    if (emlen < hlen + 2) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    if (target_salt_len == CE_RSA_SALT_LEN_MAX) {
        if (emlen >= hlen + hlen + 2) {
            expect_slen = hlen;
        } else {
            expect_slen = emlen - hlen - 2;
        }
    } else if (target_salt_len == CE_RSA_SALT_LEN_ANY) {
        expect_slen = CE_RSA_SALT_LEN_ANY;
    } else {
        expect_slen = target_salt_len;
    }

    /* 4. Check 0xbc */
    if (sign[sign_len - 1] != 0xbc) {
        ret = CE_ERROR_INVAL_PADDING;
        goto end;
    }

    /* 5. get maskedDB(equals to em) and h */
    db_len = emlen - hlen - 1;
    db     = pal_calloc(1, db_len);
    if (NULL == db) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    h = em + db_len;
    PAL_ASSERT(h + hlen + 1 == sign + sign_len);

    /* 6. Check left most data */
    if ((em != sign) && (sign[0] != 0x00)) {
        ret = CE_ERROR_INVAL_PADDING;
        goto end;
    }

    if ((em[0] >> (8 - (8 * emlen - embits))) != 0x00) {
        ret = CE_ERROR_INVAL_PADDING;
        goto end;
    } else {
        /* do nothing */
    }

    /* 7 && 8: calc DB */
    pal_memcpy(db, em, db_len);
    ret = mgf_mask(db, db_len, (uint8_t *)h, hlen, hash_id);
    CHECK_SUCCESS_GOTO(ret, end);

    /* 9. set leftmost 8*emlen - embits to 0 */
    db[0] &= (0xFF >> (emlen * 8 - embits));

    /* 10. find salt start */
    i = 0;
    while (1) {
        if (i >= db_len) {
            salt = NULL;
            /* doesn't found 0x01 */
            break;
        }
        if (db[i] == 0) {
            i++;
            continue;
        } else if (db[i] == 0x01) {
            salt = db + i + 1;
            break;
        } else {
            salt = NULL;
            /* find invalid padding */
            break;
        }
    }
    if (NULL == salt) {
        ret = CE_ERROR_INVAL_PADDING;
        goto end;
    }
    slen = db + db_len - salt;

    /* Check salt length */
    if ((expect_slen != CE_RSA_SALT_LEN_ANY) && (slen != expect_slen)) {
        ret = CE_ERROR_INVAL_PADDING;
        goto end;
    }

    /* 12. Format m_ */
    tmp_len = 8 + input_len + slen;
    m_      = pal_calloc(1, tmp_len);
    if (NULL == m_) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    pal_memset(m_, 0, 8);
    pal_memcpy(m_ + 8, input, input_len);
    if (slen) {
        pal_memcpy(m_ + 8 + input_len, salt, slen);
    } else {
        /* do nothing */
    }
    /* 13. Calculate H' = Hash(M') */
    h_ = pal_calloc(1, CE_MAX_HASH_SIZE);
    if (NULL == h_) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    ret = ce_dgst(hash_id, (const uint8_t *)(m_), tmp_len, h_);
    CHECK_SUCCESS_GOTO(ret, end);

    /* 14. Compare h with h_ */
    if (ce_safer_memcmp(h, h_, hlen) != 0) {
        ret = CE_ERROR_VERIFY_SIG;
    } else {
        ret = CE_SUCCESS;
    }
end:
    if (db) {
        pal_free(db);
    }
    if (m_) {
        pal_free(m_);
    }
    if (h_) {
        pal_free(h_);
    }
    return ret;
}

int32_t ce_rsa_rsassa_pss_verify(ce_rsa_context_t *ctx,
                                 const uint8_t *hash,
                                 size_t hashlen,
                                 ce_algo_t mgf1_hash_id,
                                 int32_t expected_salt_len,
                                 const uint8_t *sig)
{
    int32_t ret     = 0;
    uint8_t *buf    = NULL;
    size_t key_bits = 0;
    size_t siglen   = 0;

    if ((NULL == ctx) || (NULL == hash) || (0 == hashlen) || (NULL == sig)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (!((mgf1_hash_id == CE_ALG_SHA256) || (mgf1_hash_id == CE_ALG_SHA1) ||
        (mgf1_hash_id == CE_ALG_SHA224))) {
        return CE_ERROR_BAD_PARAMS;
    }

    siglen = ctx->len;

    key_bits = ce_bn_bitlen((const ce_bn_t *)(&(ctx->N)));
    PAL_ASSERT(siglen == (key_bits + 7) / 8);

    buf = pal_calloc(1, siglen);
    if (NULL == buf) {
        return CE_ERROR_OOM;
    }

    ret = ce_rsa_public(ctx, sig, buf);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = rsa_pss_decode(buf, hash, hashlen, key_bits, expected_salt_len,
                         mgf1_hash_id);
end:
    pal_memset(buf, 0, siglen);
    pal_free(buf);
    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN)
int32_t ce_rsa_rsassa_pss_sign(ce_rsa_context_t *ctx,
                               int32_t (*f_rng)(void *, uint8_t *, size_t),
                               void *p_rng,
                               const uint8_t *hash,
                               size_t hashlen,
                               ce_algo_t mgf1_hash_id,
                               int32_t salt_len,
                               uint8_t *sig)
{
    int32_t ret     = 0;
    uint8_t *buf    = NULL;
    size_t key_bits = 0;
    size_t siglen   = 0;

    if ((NULL == ctx) || (NULL == hash) || (0 == hashlen) || (NULL == sig) ||
        (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (!((mgf1_hash_id == CE_ALG_SHA256) || (mgf1_hash_id == CE_ALG_SHA1) ||
        (mgf1_hash_id == CE_ALG_SHA224))) {
        return CE_ERROR_BAD_PARAMS;
    }

    siglen = ctx->len;

    key_bits = ce_bn_bitlen((const ce_bn_t *)(&(ctx->N)));
    PAL_ASSERT(siglen == (key_bits + 7) / 8);

    buf = pal_calloc(1, siglen);
    if (NULL == buf) {
        return CE_ERROR_OOM;
    }
    ret = rsa_pss_encode(buf, hash, hashlen, key_bits, salt_len, mgf1_hash_id,
                         f_rng, p_rng);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_rsa_private(ctx, f_rng, p_rng, buf, sig);
    CHECK_SUCCESS_GOTO(ret, end);

end:
    pal_memset(buf, 0, siglen);
    pal_free(buf);
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */
#endif /* CE_LITE_RSA_PKCS_V21 */

#if defined(CE_LITE_RSA_PKCS_V15)

#if defined(CE_LITE_SHA256)
const static uint8_t sha256_encoding_t_data[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
#endif /* CE_LITE_SHA256 */

#if defined(CE_LITE_SHA224)
const static uint8_t sha224_encoding_t_data[] = {
    0x30, 0x2d, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x04, 0x05, 0x00, 0x04, 0x1c};
#endif /* CE_LITE_SHA224 */

#if defined(CE_LITE_SHA1)
const static uint8_t sha1_encoding_t_data[] = {
    0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
    0x1a, 0x05, 0x00, 0x04, 0x14};
#endif /* CE_LITE_SHA1 */

static int32_t ce_md_get_encoding_t_info(ce_algo_t md_alg,
                                         const uint8_t **encoding_t_data,
                                         size_t *encoding_t_size)
{
    switch (md_alg) {
#if defined(CE_LITE_SHA256)
    case CE_ALG_SHA256:
        *encoding_t_data = sha256_encoding_t_data;
        *encoding_t_size = sizeof(sha256_encoding_t_data);
        return CE_SUCCESS;
#endif /* CE_LITE_SHA256 */

#if defined(CE_LITE_SHA224)
    case CE_ALG_SHA224:
        *encoding_t_data = sha224_encoding_t_data;
        *encoding_t_size = sizeof(sha224_encoding_t_data);
        return CE_SUCCESS;
#endif /* CE_LITE_SHA224 */

#if defined(CE_LITE_SHA1)
    case CE_ALG_SHA1:
        *encoding_t_data = sha1_encoding_t_data;
        *encoding_t_size = sizeof(sha1_encoding_t_data);
        return CE_SUCCESS;
#endif /* CE_LITE_SHA1 */

    default:
        return CE_ERROR_NOT_SUPPORTED;
    }
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-V1_5-SIGN function
 */

/* Construct a PKCS v1.5 encoding of a hashed message
 *
 * This is used both for signature generation and verification.
 *
 * Parameters:
 * - md_alg:  Identifies the hash algorithm used to generate the given hash;
 *            0 if raw data is signed.
 * - hashlen: Length of hash in case hashlen is 0.
 * - hash:    Buffer containing the hashed message or the raw data.
 * - dst_len: Length of the encoded message.
 * - dst:     Buffer to hold the encoded message.
 *
 * Assumptions:
 * - hash has size hashlen if md_alg == 0.
 * - hash has size corresponding to md_alg if md_alg != 0.
 * - dst points to a buffer of size at least dst_len.
 *
 */
static int32_t rsa_rsassa_pkcs1_v15_encode(ce_algo_t md_alg,
                                           uint32_t hashlen,
                                           const uint8_t *hash,
                                           size_t dst_len,
                                           uint8_t *dst)
{
    int32_t ret                    = CE_SUCCESS;
    size_t ps_len                  = 0;
    uint8_t *p                     = NULL;
    const uint8_t *encoding_t_data = NULL;
    size_t encoding_t_size         = 0;

    /* Are we signing hashed or raw data? */
    if (md_alg != 0) {
        ret = ce_md_get_encoding_t_info(md_alg, &encoding_t_data,
                                        &encoding_t_size);
        if (CE_SUCCESS != ret) {
            return ret;
        }
        hashlen = ce_md_size(md_alg);
        PAL_ASSERT(hashlen > 0);

        /* Check emLen >= tLen + 11 */
        if (dst_len < encoding_t_size + hashlen + 11) {
            return CE_ERROR_BAD_INPUT_DATA;
        }

        /* Calculate ps_length */
        ps_len = dst_len - encoding_t_size - hashlen - 3;
        PAL_ASSERT(ps_len >= 8);
    } else {
        if (dst_len < hashlen + 11) {
            return CE_ERROR_BAD_INPUT_DATA;
        }

        ps_len = dst_len - hashlen - 3;
    }

    p = dst;

    /* Write PS */
    *p++ = 0x00;
    *p++ = 0x01;
    pal_memset(p, 0xFF, ps_len);
    p += ps_len;
    *p++ = 0x00;

    /* Are we signing raw data? */
    if (md_alg == 0) {
        pal_memcpy(p, hash, hashlen);
        return CE_SUCCESS;
    }

    /* copy encoding T */
    pal_memcpy(p, encoding_t_data, encoding_t_size);
    p += encoding_t_size;

    pal_memcpy(p, hash, hashlen);
    p += hashlen;

    /* Just a sanity-check. */
    PAL_ASSERT(p == dst + dst_len);

    return CE_SUCCESS;
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-v1_5-VERIFY function
 */
int32_t ce_rsa_rsassa_pkcs1_v15_verify(ce_rsa_context_t *ctx,
                                       ce_algo_t md_alg,
                                       size_t hashlen,
                                       const uint8_t *hash,
                                       const uint8_t *sig)
{
    int32_t ret      = 0;
    size_t sig_len   = 0;
    uint8_t *encoded = NULL, *encoded_expected = NULL;

    if ((NULL == ctx) || (NULL == hash) || (0 == hashlen) || (NULL == sig)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (!((md_alg == CE_ALG_SHA256) || (md_alg == CE_ALG_SHA1) ||
          (md_alg == 0) || (md_alg == CE_ALG_SHA224))) {
        return CE_ERROR_BAD_PARAMS;
    }

    sig_len = ctx->len;

    /*
     * Prepare expected PKCS1 v1.5 encoding of hash.
     */
    encoded = pal_calloc(1, sig_len);
    if (NULL == encoded) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    encoded_expected = pal_calloc(1, sig_len);
    if (NULL == encoded_expected) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    ret = rsa_rsassa_pkcs1_v15_encode(md_alg, hashlen, hash, sig_len,
                                      encoded_expected);
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Apply RSA primitive to get what should be PKCS1 encoded hash.
     */
    ret = ce_rsa_public(ctx, sig, encoded);
    if (ret != 0) {
        goto end;
    }

    /*
     * Compare
     */
    if (ce_safer_memcmp(encoded, encoded_expected, sig_len) != 0) {
        ret = CE_ERROR_VERIFY_SIG;
    } else {
        ret = CE_SUCCESS;
    }

end:
    if (encoded != NULL) {
        pal_memset(encoded, 0, sig_len);
        pal_free(encoded);
    }

    if (encoded_expected != NULL) {
        pal_memset(encoded_expected, 0, sig_len);
        pal_free(encoded_expected);
    }

    return ret;
}

#if defined(CE_LITE_RSASSA_SIGN)
int32_t
ce_rsa_rsassa_pkcs1_v15_sign(ce_rsa_context_t *ctx,
                             int32_t (*f_rng)(void *, uint8_t *, size_t),
                             void *p_rng,
                             ce_algo_t md_alg,
                             size_t hashlen,
                             const uint8_t *hash,
                             uint8_t *sig)
{
    int32_t ret      = 0;
    size_t sig_len   = 0;
    uint8_t *encoded = NULL;

    if ((NULL == ctx) || (NULL == hash) || (0 == hashlen) || (NULL == sig)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (!((md_alg == CE_ALG_SHA256) || (md_alg == CE_ALG_SHA1) ||
          (md_alg == 0) || (md_alg == CE_ALG_SHA224))) {
        return CE_ERROR_BAD_PARAMS;
    }

    sig_len = ctx->len;

    /*
     * Prepare expected PKCS1 v1.5 encoding of hash.
     */
    encoded = pal_calloc(1, sig_len);
    if (NULL == encoded) {
        ret = CE_ERROR_OOM;
        goto end;
    }
    ret = rsa_rsassa_pkcs1_v15_encode(md_alg, hashlen, hash, sig_len, encoded);
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Apply RSA primitive to get what should be PKCS1 encoded hash.
     */
    ret = ce_rsa_private(ctx, f_rng, p_rng, encoded, sig);
    if (ret != 0) {
        goto end;
    }

end:
    if (encoded != NULL) {
        pal_memset(encoded, 0, sig_len);
        pal_free(encoded);
    }

    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN */
#endif /* CE_LITE_RSA_PKCS_V15 */

#endif /* CE_LITE_RSASSA */