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
#include "ce_lite_ecp.h"
#include "ce_lite_hash.h"
#include "ce_lite_ecdsa.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"
#include "ce_aca_internal.h"

#if defined(CE_LITE_ECDSA)

/*
 * Derive a suitable integer for group grp from a buffer of length len
 * SEC1 4.1.3 step 5 aka SEC1 4.1.4 step 3
 */
static int32_t _derive_mpi(const ce_ecp_group_t *grp,
                           ce_bn_t *x,
                           const uint8_t *buf,
                           size_t blen)
{
    int32_t ret     = CE_SUCCESS;
    size_t n_size   = (grp->nbits + 7) / 8;
    size_t use_size = blen > n_size ? n_size : blen;

    ret = ce_bn_import(x, buf, use_size);
    CHECK_SUCCESS_GOTO(ret, end);
    if (use_size * 8 > grp->nbits) {
        ret = ce_bn_shift_r(x, use_size * 8 - grp->nbits);
        CHECK_SUCCESS_GOTO(ret, end);
    }

    /* Skip reduction because we support e > N in latter mul mod */
end:
    return ret;
}

int32_t ce_ecdsa_verify(const ce_ecp_group_t *grp,
                        const uint8_t *buf,
                        size_t size,
                        const ce_ecp_point_t *Q,
                        const ce_bn_t *r,
                        const ce_bn_t *s)
{
    int32_t ret      = CE_SUCCESS;
    ce_bn_t e        = {NULL};
    ce_bn_t s_inv    = {NULL};
    ce_bn_t u1       = {NULL};
    ce_bn_t u2       = {NULL};
    ce_ecp_point_t R = {0};
    int32_t result   = 0;

    if ((NULL == grp) || (NULL == buf) || (0 == size) || (NULL == Q) ||
        (NULL == r) || (NULL == s)) {
        return CE_ERROR_BAD_PARAMS;
    }

    /*
     * Step 1: make sure r and s are in range 1..n-1
     */
    ret = ce_ecp_check_privkey(grp, r);
    if (CE_SUCCESS != ret) {
        if (CE_ERROR_INVAL_KEY == ret) {
            /* Actually we are checking signature, so change the return value */
            ret = CE_ERROR_VERIFY_SIG;
        }
        goto end;
    }
    ret = ce_ecp_check_privkey(grp, s);
    if (CE_SUCCESS != ret) {
        if (CE_ERROR_INVAL_KEY == ret) {
            /* Actually we are checking signature, so change the return value */
            ret = CE_ERROR_VERIFY_SIG;
        }
        goto end;
    }

    /* make sure Q is valid */
    ret = ce_ecp_check_pubkey(grp, Q);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_point_init(&R);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&e);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&s_inv);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&u1);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&u2);
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Step 3: derive MPI from hashed message
     */
    ret = _derive_mpi(grp, &e, buf, size);
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Step 4: u1 = e / s mod n, u2 = r / s mod n
     */
    ret = ce_bn_inv_mod(&s_inv, s, (const ce_bn_t *)(&(grp->N)));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_mul_mod(&u1, (const ce_bn_t *)(&e), (const ce_bn_t *)(&s_inv),
                        (const ce_bn_t *)(&(grp->N)));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_mul_mod(&u2, r, (const ce_bn_t *)(&s_inv),
                        (const ce_bn_t *)(&(grp->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Step 5: R = u1 G + u2 Q
     */
    ret = ce_ecp_muladd(grp, &R, (const ce_bn_t *)(&u1),
                        (const ce_ecp_point_t *)(&(grp->G)),
                        (const ce_bn_t *)(&u2), Q);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_ecp_is_zero(&R);
    if (ret < 0) {
        goto end;
    }
    if (1 == ret) {
        ret = CE_ERROR_VERIFY_SIG;
        goto end;
    }
    /*
     * Step 6: convert xR to an integer (no-op)
     * Step 7: reduce xR mod n (gives v)
     */
    ret = ce_bn_mod_bn(&(u1), (const ce_bn_t *)(&(R.X)),
                       (const ce_bn_t *)(&(grp->N)));
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * Step 8: check if v (that is, u1) is equal to r
     */
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(u1)), r, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_VERIFY_SIG;
        goto end;
    }

    ret = CE_SUCCESS;
end:
    ce_ecp_point_free(&R);
    ce_bn_free(&e);
    ce_bn_free(&s_inv);
    ce_bn_free(&u1);
    ce_bn_free(&u2);
    return ret;
}

#if defined(CE_LITE_ECDSA_SIGN)
int32_t ce_ecdsa_sign(const ce_ecp_group_t *grp,
                      const ce_bn_t *d,
                      const uint8_t *buf,
                      size_t size,
                      ce_bn_t *r,
                      ce_bn_t *s,
                      int32_t (*f_rng)(void *, uint8_t *, size_t),
                      void *p_rng)
{
    int32_t ret        = CE_SUCCESS;
    int32_t key_tries  = 0;
    int32_t sign_tries = 0;
    ce_ecp_point_t R   = {0};
    ce_bn_t k          = {NULL};
    ce_bn_t e          = {NULL};
    ce_bn_t tmp0       = {NULL};
    ce_bn_t tmp1       = {NULL};
#if defined(CE_LITE_BLINDING)
    ce_bn_t blinding = {NULL};
#endif
    int32_t result = 0;

    /* Make sure d is in range 1..n-1 */
    ret = ce_ecp_check_privkey(grp, d);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_point_init(&R);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&k);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&e);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp0);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&tmp1);
    CHECK_SUCCESS_GOTO(ret, end);
#if defined(CE_LITE_BLINDING)
    ret = ce_bn_init(&blinding);
    CHECK_SUCCESS_GOTO(ret, end);
#endif

    /**
     * derive MPI from hashed message
     */
    ret = _derive_mpi(grp, &e, buf, size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* don't pollute e in the following calculation */

    sign_tries = 0;
    do {
        if (sign_tries++ > CE_RANDOM_RETRY_CNT) {
            ret = CE_ERROR_GEN_RANDOM;
            goto end;
        }
        /*
         * Steps 1-3: generate a suitable ephemeral keypair
         * and set r = xR mod n
         */
        key_tries = 0;
        do {
            if (key_tries++ > CE_RANDOM_RETRY_CNT) {
                ret = CE_ERROR_GEN_RANDOM;
                goto end;
            }

            ret = ce_ecp_gen_privkey(grp, &k, f_rng, p_rng);
            CHECK_SUCCESS_GOTO(ret, end);

            ret = ce_ecp_mul(grp,
                             &R,
                             (const ce_bn_t *)(&k),
                             (const ce_ecp_point_t *)(&(grp->G)),
                             f_rng,
                             p_rng);
            CHECK_SUCCESS_GOTO(ret, end);

            ret = ce_bn_mod_bn(r, (const ce_bn_t *)(&(R.X)),
                               (const ce_bn_t *)(&(grp->N)));
            CHECK_SUCCESS_GOTO(ret, end);

            ret = ce_bn_cmp_u32((const ce_bn_t *)r, 0, &result);
            CHECK_SUCCESS_GOTO(ret, end);
        } while (result == 0);

#if defined(CE_LITE_BLINDING)
        /*
         * Generate a random value to blind inv_mod in next step,
         * avoiding a potential timing leak.
         */
        ret = ce_ecp_gen_privkey(grp, &blinding, f_rng, p_rng);
        CHECK_SUCCESS_GOTO(ret, end);
#endif
        /*
         * Step 6: compute s = (e + r * d) / k =
         *              blinding * (e + r * d) / (k * blinding) mod N
         * blinding == 1 if blinding is disabled
         */
        /* tmp0 = r * d mod N */
        ret = ce_bn_mul_mod(&tmp0, (const ce_bn_t *)r, (const ce_bn_t *)d,
                            (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);
        /* tmp0 = (tmp0 + e) mod N */
        ret = ce_bn_add_mod(&tmp0, (const ce_bn_t *)(&e),
                            (const ce_bn_t *)(&tmp0),
                            (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);
        /* tmp0 contains high */
#if defined(CE_LITE_BLINDING)
        /* blinding high */
        /* tmp0 = tmp0 * blinding */
        ret = ce_bn_mul_mod(&tmp0, (const ce_bn_t *)(&tmp0),
                            (const ce_bn_t *)(&blinding),
                            (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);

        /* tmp0 contains blinded high */

        /* blinding low */
        /* k = k * blinding */
        ret = ce_bn_mul_mod(&k, (const ce_bn_t *)(&k),
                            (const ce_bn_t *)(&blinding),
                            (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);
#endif
        /* tmp1 = inv_k */
        ret = ce_bn_inv_mod(&tmp1, (const ce_bn_t *)(&k),
                            (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);

        /* tmp1 contains low or blinded low */

        /* s = tmp0 * tmp1 */
        ret =
            ce_bn_mul_mod(s, (const ce_bn_t *)(&tmp0), (const ce_bn_t *)(&tmp1),
                          (const ce_bn_t *)(&(grp->N)));
        CHECK_SUCCESS_GOTO(ret, end);
        ret = ce_bn_cmp_u32(s, 0, &result);
        CHECK_SUCCESS_GOTO(ret, end);
    } while (result == 0);

    ret = CE_SUCCESS;
end:
    ce_ecp_point_free(&R);
    ce_bn_free(&k);
    ce_bn_free(&e);
    ce_bn_free(&tmp0);
    ce_bn_free(&tmp1);
#if defined(CE_LITE_BLINDING)
    ce_bn_free(&blinding);
#endif
    return ret;
}
#endif /* CE_LITE_ECDSA_SIGN */

int32_t ce_ecdsa_init(ce_ecdsa_context_t *ctx)
{
    return ce_ecp_keypair_init(ctx);
}

void ce_ecdsa_free(ce_ecdsa_context_t *ctx)
{
    return ce_ecp_keypair_free(ctx);
}

int32_t ce_ecdsa_from_keypair(ce_ecdsa_context_t *ctx,
                              const ce_ecp_keypair_t *key)
{
    int32_t ret = CE_SUCCESS;

    if ((NULL == ctx) || (NULL == key)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_ecp_group_copy(&(ctx->grp), (const ce_ecp_group_t *)(&(key->grp)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_copy(&(ctx->Q), (const ce_ecp_point_t *)(&(key->Q)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_copy(&(ctx->d), (const ce_bn_t *)(&(key->d)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = CE_SUCCESS;
end:
    if (CE_SUCCESS != ret) {
        ce_ecdsa_free(ctx);
    }
    return ret;
}

int32_t ce_ecdsa_read_signature(const ce_ecdsa_context_t *ctx,
                                const uint8_t *hash,
                                size_t hlen,
                                const uint8_t *sig,
                                size_t slen)
{
    int32_t ret          = CE_SUCCESS;
    uint8_t *p           = (uint8_t *)(sig);
    const uint8_t *limit = (const uint8_t *)(sig + slen);
    size_t len           = 0;
    ce_bn_t r            = {NULL};
    ce_bn_t s            = {NULL};

    if ((NULL == ctx) || (NULL == hash) || (NULL == sig)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_init(&r);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_init(&s);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_asn1_get_tag(&p, limit, &len, 0x30);
    if (CE_SUCCESS != ret) {
        /* always return bad input data for ASN1 parse error */
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    if (p + len != limit) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    /* get bn r and s */
    ret = ce_asn1_get_mpi(&p, limit, &r);
    if (CE_SUCCESS != ret) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }
    ret = ce_asn1_get_mpi(&p, limit, &s);
    if (CE_SUCCESS != ret) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    /* verify */
    ret = ce_ecdsa_verify((const ce_ecp_group_t *)(&(ctx->grp)), hash, hlen,
                          (const ce_ecp_point_t *)(&(ctx->Q)),
                          (const ce_bn_t *)(&r), (const ce_bn_t *)(&s));
    CHECK_SUCCESS_GOTO(ret, end);

    /**
     * At this point we know that the buffer starts with a valid signature.
     * Return 0 if the buffer just contains the signature, and a specific
     * error code if the valid signature is followed by more data.
     */
    if (p != limit) {
        ret = CE_ERROR_SIG_LENGTH;
        goto end;
    }
end:
    ce_bn_free(&r);
    ce_bn_free(&s);
    return ret;
}

#if defined(CE_LITE_ECDSA_SIGN)
int32_t ce_ecdsa_write_signature(const ce_ecdsa_context_t *ctx,
                                 const uint8_t *hash,
                                 size_t hlen,
                                 uint8_t *sig,
                                 size_t *slen,
                                 int32_t (*f_rng)(void *, uint8_t *, size_t),
                                 void *p_rng)
{
    int32_t ret                   = CE_SUCCESS;
    uint8_t buf[CE_ECDSA_MAX_LEN] = {0};
    uint8_t *p                    = buf + sizeof(buf);
    size_t len                    = 0;
    ce_bn_t r                     = {NULL};
    ce_bn_t s                     = {NULL};

    if ((NULL == ctx) || (NULL == hash) || (NULL == sig) || (NULL == slen) ||
        (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_init(&r);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_init(&s);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecdsa_sign((const ce_ecp_group_t *)(&(ctx->grp)),
                        (const ce_bn_t *)(&(ctx->d)), hash, hlen, &r, &s, f_rng,
                        p_rng);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Write to der in reverse order */
    ret = ce_asn1_write_mpi(&p, buf, (const ce_bn_t *)(&s));
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;

    ret = ce_asn1_write_mpi(&p, buf, (const ce_bn_t *)(&r));
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;

    ret = ce_asn1_write_len(&p, buf, len);
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;

    ret = ce_asn1_write_tag(&p, buf, 0x30);
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;

    /* Copy to output */
    pal_memcpy(sig, p, len);
    *slen = len;

    ret = CE_SUCCESS;
end:
    ce_bn_free(&r);
    ce_bn_free(&s);
    return ret;
}
#endif /* CE_LITE_ECDSA_SIGN */
#endif /* CE_LITE_ECDSA */