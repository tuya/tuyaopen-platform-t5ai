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
#include "ce_lite_sm2dsa.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"
#include "ce_aca_internal.h"

#if defined(CE_LITE_SM2DSA)

int32_t ce_sm2dsa_verify(const uint8_t *buf,
                         size_t size,
                         const ce_ecp_point_t *Q,
                         const ce_bn_t *r,
                         const ce_bn_t *s)
{
    int32_t ret            = CE_SUCCESS;
    ce_ecp_group_t sm2_grp = {0};
    ce_bn_t e              = {NULL};
    ce_bn_t u1             = {NULL};
    ce_bn_t u2             = {NULL};
    ce_ecp_point_t R       = {0};
    int32_t result         = 0;

    if ((NULL == buf) || (0 == size) || (NULL == Q) || (NULL == r) ||
        (NULL == s)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_ecp_group_init(&sm2_grp);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_ecp_group_load(&sm2_grp, CE_ECP_DP_SM2P256V1);
    CHECK_SUCCESS_GOTO(ret, end);

    /* only support verify digest size <= 256 bits. */
    if ((size * 8) > sm2_grp.pbits) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* make sure r and s are in range 1..n-1 */
    ret = ce_ecp_check_privkey((const ce_ecp_group_t *)(&sm2_grp), r);
    if (CE_SUCCESS != ret) {
        if (CE_ERROR_INVAL_KEY == ret) {
            /* Actually we are checking signature, so change the return value */
            ret = CE_ERROR_VERIFY_SIG;
        }
        goto end;
    }
    ret = ce_ecp_check_privkey((const ce_ecp_group_t *)(&sm2_grp), s);
    if (CE_SUCCESS != ret) {
        if (CE_ERROR_INVAL_KEY == ret) {
            /* Actually we are checking signature, so change the return value */
            ret = CE_ERROR_VERIFY_SIG;
        }
        goto end;
    }

    /* make sure Q is valid */
    ret = ce_ecp_check_pubkey((const ce_ecp_group_t *)(&sm2_grp), Q);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_point_init(&R);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&e);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&u1);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&u2);
    CHECK_SUCCESS_GOTO(ret, end);

    /* import digest to e */
    ret = ce_bn_import(&e, buf, size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* t = (r + s) mod n */
    ret = ce_bn_add_mod(&u2, r, s, (const ce_bn_t *)(&(sm2_grp.N)));
    CHECK_SUCCESS_GOTO(ret, end);

    /* compare t with 0 */
    ret = ce_bn_cmp_u32((const ce_bn_t *)(&u2), 0, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result == 0) {
        ret = CE_ERROR_VERIFY_SIG;
        goto end;
    }

    /* R = s * G + u2 * Q */
    ret = ce_ecp_muladd((const ce_ecp_group_t *)(&sm2_grp),
                        &R,
                        s,
                        (const ce_ecp_point_t *)(&(sm2_grp.G)),
                        (const ce_bn_t *)(&u2),
                        Q);
    CHECK_SUCCESS_GOTO(ret, end);

    /* (R.x + e) mod n*/
    ret = ce_bn_add_mod(&u1,
                        (const ce_bn_t *)(&(R.X)),
                        (const ce_bn_t *)(&e),
                        (const ce_bn_t *)(&(sm2_grp.N)));
    CHECK_SUCCESS_GOTO(ret, end);

    /* check if u1 is equal to r */
    ret =
        ce_bn_cmp_bn_equal((const ce_bn_t *)(&u1), (const ce_bn_t *)r, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_VERIFY_SIG;
        goto end;
    }

end:
    ce_ecp_group_free(&sm2_grp);
    ce_ecp_point_free(&R);
    ce_bn_free(&e);
    ce_bn_free(&u1);
    ce_bn_free(&u2);
    return ret;
}

#if defined(CE_LITE_SM2DSA_SIGN)

int32_t ce_sm2dsa_sign(const ce_bn_t *d,
                       const uint8_t *buf,
                       size_t size,
                       ce_bn_t *r,
                       ce_bn_t *s,
                       int32_t (*f_rng)(void *, uint8_t *, size_t),
                       void *p_rng)
{
    int32_t ret       = CE_SUCCESS;
    int32_t key_tries = 0, sign_tries = 0;
    ce_ecp_group_t sm2_grp = {0};
    ce_ecp_point_t R       = {0};
    ce_bn_t k              = {NULL};
    ce_bn_t e              = {NULL};
    ce_bn_t tmp0           = {NULL};
    ce_bn_t tmp1           = {NULL};
#if defined(CE_LITE_BLINDING)
    ce_bn_t blinding = {NULL};
#endif
    int32_t result1 = 0, result2 = 0;

    if ((NULL == buf) || (0 == size) || (NULL == d) || (NULL == r) ||
        (NULL == s) || (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_ecp_group_init(&sm2_grp);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_ecp_group_load(&sm2_grp, CE_ECP_DP_SM2P256V1);
    CHECK_SUCCESS_GOTO(ret, end);

    /* only support sign digest size <= 256 bits. */
    if ((size * 8) > sm2_grp.pbits) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* Make sure d is in range 1..n-1 */
    ret = ce_ecp_check_privkey((const ce_ecp_group_t *)(&sm2_grp), d);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Init tmp variables */
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

    /* import digest to e */
    ret = ce_bn_import(&e, buf, size);
    CHECK_SUCCESS_GOTO(ret, end);

    sign_tries = 0;
    do {
        if (sign_tries++ > CE_RANDOM_RETRY_CNT) {
            ret = CE_ERROR_GEN_RANDOM;
            goto end;
        }
        /*
         * Step 3: Generate a suitable ephemeral private key k
         */
        key_tries = 0;
        do {
            if (key_tries++ > CE_RANDOM_RETRY_CNT) {
                ret = CE_ERROR_GEN_RANDOM;
                goto end;
            }

            ret = ce_ecp_gen_privkey((const ce_ecp_group_t *)(&sm2_grp), &k,
                                     f_rng, p_rng);
            CHECK_SUCCESS_GOTO(ret, end);
            /**
             * Step 4: R = [k]*G, R = (x1, y1)
             * Improvement: use blinding if necessary.
             */
            ret = ce_ecp_mul((const ce_ecp_group_t *)(&sm2_grp),
                             &R,
                             (const ce_bn_t *)(&k),
                             (const ce_ecp_point_t *)(&(sm2_grp.G)),
                             f_rng,
                             p_rng);
            CHECK_SUCCESS_GOTO(ret, end);

            /* Step 5: r = (e + x1) mod N, where x1 = R.X */
            ret = ce_bn_add_mod(r,
                                (const ce_bn_t *)(&e),
                                (const ce_bn_t *)(&(R.X)),
                                (const ce_bn_t *)(&(sm2_grp.N)));
            CHECK_SUCCESS_GOTO(ret, end);

            /**
             * Check r ==0 or r + k == N.
             * Improvement: r + k == N --> (r + k) mod N == 0
             */
            ret = ce_bn_cmp_u32((const ce_bn_t *)r, 0, &result1);
            CHECK_SUCCESS_GOTO(ret, end);

            /* tmp0 = (r + k) mod N */
            ret = ce_bn_add_mod(&tmp0,
                                (const ce_bn_t *)r,
                                (const ce_bn_t *)(&k),
                                (const ce_bn_t *)(&(sm2_grp.N)));
            CHECK_SUCCESS_GOTO(ret, end);

            ret = ce_bn_cmp_u32((const ce_bn_t *)(&tmp0), 0, &result2);
            CHECK_SUCCESS_GOTO(ret, end);
        } while ((result1 == 0) || (result2 == 0));

#if defined(CE_LITE_BLINDING)
        /*
         * Generate a random value to blind inv_mod in next step,
         * avoiding a potential timing leak.
         */
        ret = ce_ecp_gen_privkey((const ce_ecp_group_t *)(&sm2_grp), &blinding,
                                 f_rng, p_rng);
        CHECK_SUCCESS_GOTO(ret, end);
#endif

        /*
         * Step 6: compute s = (((1 + d)^-1) * (k - r * d)) mod N
         *
         * Here use blinding:
         *  s = (k - r * d) / (1 + d)
         *    = blinding * (k - r * d) / blinding * (1 + d)
         *
         * Note: don't use e here, because e contains the input hash
         */

        /* tmp0 = (r * d) mod N */
        ret = ce_bn_mul_mod(&tmp0,
                            (const ce_bn_t *)r,
                            (const ce_bn_t *)d,
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);
        /* tmp0 = (k - tmp0) mod N */
        ret = ce_bn_sub_mod(&tmp0,
                            (const ce_bn_t *)(&k),
                            (const ce_bn_t *)(&tmp0),
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

#if defined(CE_LITE_BLINDING)
        /* tmp0 = tmp0 * blinding mod N */
        ret = ce_bn_mul_mod(&tmp0,
                            (const ce_bn_t *)(&tmp0),
                            (const ce_bn_t *)(&blinding),
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

#endif
        /* Now tmp0 contains upper, don't pollute it! */

        /* tmp1 = 1 + d */
        /* Note: k is polluted here */
        ret = ce_bn_import_u32(&k, 1);
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_bn_add_mod(&tmp1, (const ce_bn_t *)(&k), (const ce_bn_t *)d,
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

#if defined(CE_LITE_BLINDING)
        /* tmp1 = tmp1 * blinding mod N */
        ret = ce_bn_mul_mod(&tmp1, (const ce_bn_t *)(&tmp1),
                            (const ce_bn_t *)(&blinding),
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

#endif
        /* tmp1 = (tmp1^1) mod N */
        ret = ce_bn_inv_mod(&tmp1, (const ce_bn_t *)(&tmp1),
                            (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

        /* s = (tmp0 * tmp1) mod N */
        ret =
            ce_bn_mul_mod(s, (const ce_bn_t *)(&tmp0), (const ce_bn_t *)(&tmp1),
                          (const ce_bn_t *)(&(sm2_grp.N)));
        CHECK_SUCCESS_GOTO(ret, end);

        ret = ce_bn_cmp_u32((const ce_bn_t *)s, 0, &result1);
        CHECK_SUCCESS_GOTO(ret, end);
    } while (result1 == 0);

    ret = CE_SUCCESS;
end:
    ce_ecp_group_free(&sm2_grp);
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
#endif /* CE_LITE_SM2DSA_SIGN */

int32_t ce_sm2_compute_id_digest(ce_algo_t md_type,
                                 const ce_ecp_point_t *Q,
                                 const uint8_t *id,
                                 size_t idlen,
                                 uint8_t *za)
{
    int32_t ret              = CE_SUCCESS;
    uint8_t entl[2]          = {0};
    uint8_t *buf             = NULL;
    size_t p_size            = 0;
    ce_dgst_ctx_t ctx        = {NULL};
    ce_ecp_group_t sm2_grp   = {0};
    int32_t dgst_need_finish = 0;

    if ((NULL == Q) || (NULL == id) || (NULL == za) || (idlen > 0xFFFF / 8)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (CE_ALG_SM3 != md_type) {
        return CE_ERROR_FEATURE_UNAVAIL;
    }

    ret = ce_ecp_group_init(&sm2_grp);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_ecp_group_load(&sm2_grp, CE_ECP_DP_SM2P256V1);
    CHECK_SUCCESS_GOTO(ret, end);

    /* prepare buffer */
    p_size = (sm2_grp.pbits + 7) / 8;
    buf    = pal_calloc(1, p_size);
    if (buf == NULL) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    /* init one dgst context */
    ret = ce_dgst_init(&ctx, md_type);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Start dgst context */
    ret = ce_dgst_start(&ctx);
    CHECK_SUCCESS_GOTO(ret, end);
    dgst_need_finish = 1;

    /* update ENTL */
    entl[0] = ((idlen * 8) >> 8) & 0xFF;
    entl[1] = (idlen * 8) & 0xFF;
    ret     = ce_dgst_update(&ctx, (const uint8_t *)entl, sizeof(entl));
    CHECK_SUCCESS_GOTO(ret, end);

    /* update ID */
    ret = ce_dgst_update(&ctx, (const uint8_t *)id, idlen);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update A */
    ret = ce_bn_export((const ce_bn_t *)(&(sm2_grp.A)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update B */
    ret = ce_bn_export((const ce_bn_t *)(&(sm2_grp.B)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update GX */
    ret = ce_bn_export((const ce_bn_t *)(&(sm2_grp.G.X)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update GY */
    ret = ce_bn_export((const ce_bn_t *)(&(sm2_grp.G.Y)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update QX */
    ret = ce_bn_export((const ce_bn_t *)(&(Q->X)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* update QY */
    ret = ce_bn_export((const ce_bn_t *)(&(Q->Y)), buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_dgst_update(&ctx, (const uint8_t *)buf, p_size);
    CHECK_SUCCESS_GOTO(ret, end);

    /* Calculate result */
    ret = ce_dgst_finish(&ctx, za);
    CHECK_SUCCESS_GOTO(ret, end);
    dgst_need_finish = 0;

end:
    if (buf) {
        pal_free(buf);
    }
    if (dgst_need_finish) {
        (void)ce_dgst_finish(&ctx, NULL);
    }

    /* always free hash ctx */
    ce_dgst_free(&ctx);
    ce_ecp_group_free(&sm2_grp);
    return ret;
}

int32_t ce_sm2dsa_init(ce_sm2dsa_context_t *ctx)
{
    return ce_ecp_keypair_init(ctx);
}

void ce_sm2dsa_free(ce_sm2dsa_context_t *ctx)
{
    return ce_ecp_keypair_free(ctx);
}

int32_t ce_sm2dsa_from_keypair(ce_sm2dsa_context_t *ctx,
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
        ce_sm2dsa_free(ctx);
    }
    return ret;
}

int32_t ce_sm2dsa_read_signature(const ce_sm2dsa_context_t *ctx,
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
    ret = ce_sm2dsa_verify(hash, hlen, (const ce_ecp_point_t *)(&(ctx->Q)),
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

#if defined(CE_LITE_SM2DSA_SIGN)
int32_t ce_sm2dsa_write_signature(const ce_sm2dsa_context_t *ctx,
                                  const uint8_t *hash,
                                  size_t hlen,
                                  uint8_t *sig,
                                  size_t *slen,
                                  int32_t (*f_rng)(void *, uint8_t *, size_t),
                                  void *p_rng)
{
    int32_t ret                    = CE_SUCCESS;
    uint8_t buf[CE_SM2DSA_MAX_LEN] = {0};
    uint8_t *p                     = buf + sizeof(buf);
    size_t len                     = 0;
    ce_bn_t r                      = {NULL};
    ce_bn_t s                      = {NULL};

    if ((NULL == ctx) || (NULL == hash) || (NULL == sig) || (NULL == slen) ||
        (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_init(&r);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_init(&s);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_sm2dsa_sign((const ce_bn_t *)(&(ctx->d)), hash, hlen, &r, &s,
                         f_rng, p_rng);
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
#endif /* CE_LITE_SM2DSA_SIGN */

#endif /* CE_LITE_SM2DSA */
