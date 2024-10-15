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
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"
#include "ce_aca_internal.h"

#if defined(CE_LITE_ECP)

int32_t ce_ecp_point_init(ce_ecp_point_t *pt)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == pt) {
        return CE_ERROR_BAD_PARAMS;
    }

    pal_memset(pt, 0, sizeof(ce_ecp_point_t));
    ret = ce_bn_init(&(pt->X));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(pt->Y));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(pt->Z));
    CHECK_SUCCESS_GOTO(ret, end);

end:
    if (ret != CE_SUCCESS) {
        /* ce_bn_free is safe free even bn is not initialized */
        ce_bn_free(&(pt->X));
        ce_bn_free(&(pt->Y));
        ce_bn_free(&(pt->Z));
    }
    return ret;
}

void ce_ecp_point_free(ce_ecp_point_t *pt)
{
    if (NULL == pt) {
        return;
    }
    ce_bn_free(&(pt->X));
    ce_bn_free(&(pt->Y));
    ce_bn_free(&(pt->Z));
    pal_memset(pt, 0, sizeof(ce_ecp_point_t));
    return;
}

int32_t ce_ecp_group_init(ce_ecp_group_t *grp)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == grp) {
        return CE_ERROR_BAD_PARAMS;
    }

    pal_memset(grp, 0, sizeof(ce_ecp_group_t));

    grp->id = CE_ECP_DP_NONE;

    ret = ce_ecp_point_init(&(grp->G));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(grp->P));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(grp->A));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(grp->B));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(grp->N));
    CHECK_SUCCESS_GOTO(ret, end);

end:
    if (CE_SUCCESS != ret) {
        ce_ecp_point_free(&(grp->G));
        ce_bn_free(&(grp->P));
        ce_bn_free(&(grp->A));
        ce_bn_free(&(grp->B));
        ce_bn_free(&(grp->N));
    }
    return ret;
}

void ce_ecp_group_free(ce_ecp_group_t *grp)
{
    if (NULL == grp) {
        return;
    }
    ce_ecp_point_free(&(grp->G));
    ce_bn_free(&(grp->P));
    ce_bn_free(&(grp->A));
    ce_bn_free(&(grp->B));
    ce_bn_free(&(grp->N));
    pal_memset(grp, 0, sizeof(ce_ecp_group_t));
}

int32_t ce_ecp_keypair_init(ce_ecp_keypair_t *key)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == key) {
        return CE_ERROR_BAD_PARAMS;
    }

    pal_memset(key, 0, sizeof(ce_ecp_keypair_t));

    ret = ce_ecp_group_init(&(key->grp));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_ecp_point_init(&(key->Q));
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&(key->d));
    CHECK_SUCCESS_GOTO(ret, end);

end:
    if (CE_SUCCESS != ret) {
        ce_ecp_group_free(&(key->grp));
        ce_ecp_point_free(&(key->Q));
        ce_bn_free(&(key->d));
    }
    return ret;
}

void ce_ecp_keypair_free(ce_ecp_keypair_t *key)
{
    if (NULL == key) {
        return;
    }
    ce_ecp_group_free(&(key->grp));
    ce_ecp_point_free(&(key->Q));
    ce_bn_free(&(key->d));
    pal_memset(key, 0, sizeof(ce_ecp_keypair_t));
}

#if defined(CE_LITE_ECP_DP_SECP256R1)
static const uint8_t secp256r1_p[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};
static const uint8_t secp256r1_a[] = {
    0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};
static const uint8_t secp256r1_b[] = {
    0x4B, 0x60, 0xD2, 0x27, 0x3E, 0x3C, 0xCE, 0x3B, 0xF6, 0xB0, 0x53,
    0xCC, 0xB0, 0x06, 0x1D, 0x65, 0xBC, 0x86, 0x98, 0x76, 0x55, 0xBD,
    0xEB, 0xB3, 0xE7, 0x93, 0x3A, 0xAA, 0xD8, 0x35, 0xC6, 0x5A,
};
static const uint8_t secp256r1_gx[] = {
    0x96, 0xC2, 0x98, 0xD8, 0x45, 0x39, 0xA1, 0xF4, 0xA0, 0x33, 0xEB,
    0x2D, 0x81, 0x7D, 0x03, 0x77, 0xF2, 0x40, 0xA4, 0x63, 0xE5, 0xE6,
    0xBC, 0xF8, 0x47, 0x42, 0x2C, 0xE1, 0xF2, 0xD1, 0x17, 0x6B,
};
static const uint8_t secp256r1_gy[] = {
    0xF5, 0x51, 0xBF, 0x37, 0x68, 0x40, 0xB6, 0xCB, 0xCE, 0x5E, 0x31,
    0x6B, 0x57, 0x33, 0xCE, 0x2B, 0x16, 0x9E, 0x0F, 0x7C, 0x4A, 0xEB,
    0xE7, 0x8E, 0x9B, 0x7F, 0x1A, 0xFE, 0xE2, 0x42, 0xE3, 0x4F,
};
static const uint8_t secp256r1_n[] = {
    0x51, 0x25, 0x63, 0xFC, 0xC2, 0xCA, 0xB9, 0xF3, 0x84, 0x9E, 0x17,
    0xA7, 0xAD, 0xFA, 0xE6, 0xBC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif /* CE_LITE_ECP_DP_SECP256R1 */

#if defined(CE_LITE_ECP_DP_SECP521R1)
static const uint8_t secp521r1_p[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
};
static const uint8_t secp521r1_a[] = {
    0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
};
static const uint8_t secp521r1_b[] = {
    0x00, 0x3F, 0x50, 0x6B, 0xD4, 0x1F, 0x45, 0xEF, 0xF1, 0x34, 0x2C,
    0x3D, 0x88, 0xDF, 0x73, 0x35, 0x07, 0xBF, 0xB1, 0x3B, 0xBD, 0xC0,
    0x52, 0x16, 0x7B, 0x93, 0x7E, 0xEC, 0x51, 0x39, 0x19, 0x56, 0xE1,
    0x09, 0xF1, 0x8E, 0x91, 0x89, 0xB4, 0xB8, 0xF3, 0x15, 0xB3, 0x99,
    0x5B, 0x72, 0xDA, 0xA2, 0xEE, 0x40, 0x85, 0xB6, 0xA0, 0x21, 0x9A,
    0x92, 0x1F, 0x9A, 0x1C, 0x8E, 0x61, 0xB9, 0x3E, 0x95, 0x51, 0x00,
};
static const uint8_t secp521r1_gx[] = {
    0x66, 0xBD, 0xE5, 0xC2, 0x31, 0x7E, 0x7E, 0xF9, 0x9B, 0x42, 0x6A,
    0x85, 0xC1, 0xB3, 0x48, 0x33, 0xDE, 0xA8, 0xFF, 0xA2, 0x27, 0xC1,
    0x1D, 0xFE, 0x28, 0x59, 0xE7, 0xEF, 0x77, 0x5E, 0x4B, 0xA1, 0xBA,
    0x3D, 0x4D, 0x6B, 0x60, 0xAF, 0x28, 0xF8, 0x21, 0xB5, 0x3F, 0x05,
    0x39, 0x81, 0x64, 0x9C, 0x42, 0xB4, 0x95, 0x23, 0x66, 0xCB, 0x3E,
    0x9E, 0xCD, 0xE9, 0x04, 0x04, 0xB7, 0x06, 0x8E, 0x85, 0xC6, 0x00,
};
static const uint8_t secp521r1_gy[] = {
    0x50, 0x66, 0xD1, 0x9F, 0x76, 0x94, 0xBE, 0x88, 0x40, 0xC2, 0x72,
    0xA2, 0x86, 0x70, 0x3C, 0x35, 0x61, 0x07, 0xAD, 0x3F, 0x01, 0xB9,
    0x50, 0xC5, 0x40, 0x26, 0xF4, 0x5E, 0x99, 0x72, 0xEE, 0x97, 0x2C,
    0x66, 0x3E, 0x27, 0x17, 0xBD, 0xAF, 0x17, 0x68, 0x44, 0x9B, 0x57,
    0x49, 0x44, 0xF5, 0x98, 0xD9, 0x1B, 0x7D, 0x2C, 0xB4, 0x5F, 0x8A,
    0x5C, 0x04, 0xC0, 0x3B, 0x9A, 0x78, 0x6A, 0x29, 0x39, 0x18, 0x01,
};
static const uint8_t secp521r1_n[] = {
    0x09, 0x64, 0x38, 0x91, 0x1E, 0xB7, 0x6F, 0xBB, 0xAE, 0x47, 0x9C,
    0x89, 0xB8, 0xC9, 0xB5, 0x3B, 0xD0, 0xA5, 0x09, 0xF7, 0x48, 0x01,
    0xCC, 0x7F, 0x6B, 0x96, 0x2F, 0xBF, 0x83, 0x87, 0x86, 0x51, 0xFA,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
};
#endif /* CE_LITE_ECP_DP_SECP521R1 */

#if defined(CE_LITE_ECP_DP_SM2P256V1)
static const uint8_t sm2p256v1_p[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF};
static const uint8_t sm2p256v1_a[] = {
    0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF};
static const uint8_t sm2p256v1_b[] = {
    0x93, 0x0E, 0x94, 0x4D, 0x41, 0xBD, 0xBC, 0xDD, 0x92, 0x8F, 0xAB,
    0x15, 0xF5, 0x89, 0x97, 0xF3, 0xA7, 0x09, 0x65, 0xCF, 0x4B, 0x9E,
    0x5A, 0x4D, 0x34, 0x5E, 0x9F, 0x9D, 0x9E, 0xFA, 0xE9, 0x28};
static const uint8_t sm2p256v1_gx[] = {
    0xC7, 0x74, 0x4C, 0x33, 0x89, 0x45, 0x5A, 0x71, 0xE1, 0x0B, 0x66,
    0xF2, 0xBF, 0x0B, 0xE3, 0x8F, 0x94, 0xC9, 0x39, 0x6A, 0x46, 0x04,
    0x99, 0x5F, 0x19, 0x81, 0x19, 0x1F, 0x2C, 0xAE, 0xC4, 0x32};
static const uint8_t sm2p256v1_gy[] = {
    0xA0, 0xF0, 0x39, 0x21, 0xE5, 0x32, 0xDF, 0x02, 0x40, 0x47, 0x2A,
    0xC6, 0x7C, 0x87, 0xA9, 0xD0, 0x53, 0x21, 0x69, 0x6B, 0xE3, 0xCE,
    0xBD, 0x59, 0x9C, 0x77, 0xF6, 0xF4, 0xA2, 0x36, 0x37, 0xBC};
static const uint8_t sm2p256v1_n[] = {
    0x23, 0x41, 0xD5, 0x39, 0x09, 0xF4, 0xBB, 0x53, 0x2B, 0x05, 0xC6,
    0x21, 0x6B, 0xDF, 0x03, 0x72, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF};
#endif /* CE_LITE_ECP_DP_SM2P256V1 */

/* convert load bn data in little endian */
static int32_t bn_load_le(ce_bn_t *bn, const uint8_t *data, size_t size)
{
    int32_t ret  = CE_SUCCESS;
    uint8_t *tmp = NULL;
    size_t i     = 0;
    uint8_t val  = 0;

    tmp = pal_calloc(1, size);
    if (NULL == tmp) {
        return CE_ERROR_OOM;
    }

    /* convert little endian to big endian */
    for (i = 0; i < size / 2; i++) {
        val               = data[size - 1 - i];
        tmp[size - 1 - i] = data[i];
        tmp[i]            = val;
    }
    /* import to bn */
    ret = ce_bn_import(bn, (const uint8_t *)tmp, size);
    CHECK_SUCCESS_GOTO(ret, end);

end:
    if (NULL != tmp) {
        pal_free(tmp);
    }
    return ret;
}

/*
 * Make group available from embedded constants
 */
static int32_t _ecp_group_load(ce_ecp_group_t *grp,
                               const uint8_t *p,
                               size_t plen,
                               const uint8_t *a,
                               size_t alen,
                               const uint8_t *b,
                               size_t blen,
                               const uint8_t *gx,
                               size_t gxlen,
                               const uint8_t *gy,
                               size_t gylen,
                               const uint8_t *n,
                               size_t nlen)
{
    int32_t ret = CE_SUCCESS;

    ret = bn_load_le(&(grp->P), p, plen);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = bn_load_le(&(grp->A), a, alen);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = bn_load_le(&(grp->B), b, blen);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = bn_load_le(&(grp->N), n, nlen);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = bn_load_le(&(grp->G.X), gx, gxlen);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = bn_load_le(&(grp->G.Y), gy, gylen);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_import_u32(&(grp->G.Z), 1);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_bitlen(&(grp->P));
    PAL_ASSERT(ret > 0);
    grp->pbits = (size_t)(ret);

    ret = ce_bn_bitlen(&(grp->N));
    PAL_ASSERT(ret > 0);
    grp->nbits = (size_t)(ret);

    ret = CE_SUCCESS;
end:
    return ret;
}

#define LOAD_GROUP(G)                                                          \
    _ecp_group_load(grp, G##_p, sizeof(G##_p), G##_a, sizeof(G##_a), G##_b,    \
                    sizeof(G##_b), G##_gx, sizeof(G##_gx), G##_gy,             \
                    sizeof(G##_gy), G##_n, sizeof(G##_n))

/*
 * Set a group using well-known domain parameters
 */
int32_t ce_ecp_group_load(ce_ecp_group_t *grp, ce_ecp_group_id_t id)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == grp) {
        return CE_ERROR_BAD_PARAMS;
    }

    switch (id) {
#if defined(CE_LITE_ECP_DP_SECP256R1)
    case CE_ECP_DP_SECP256R1: {
        ret = LOAD_GROUP(secp256r1);
        break;
    }
#endif
#if defined(CE_LITE_ECP_DP_SECP521R1)
    case CE_ECP_DP_SECP521R1: {
        ret = LOAD_GROUP(secp521r1);
        break;
    }
#endif
#if defined(CE_LITE_ECP_DP_SM2P256V1)
    case CE_ECP_DP_SM2P256V1: {
        ret = LOAD_GROUP(sm2p256v1);
        break;
    }
#endif
    default: {
        ret = CE_ERROR_FEATURE_UNAVAIL;
        break;
    }
    }

    if (CE_SUCCESS == ret) {
        grp->id = id;
    }

    return ret;
}

#if defined(CE_LITE_ECP_PRIVATE)
int32_t ce_ecp_mul(const ce_ecp_group_t *grp,
                   ce_ecp_point_t *R,
                   const ce_bn_t *m,
                   const ce_ecp_point_t *P,
                   int32_t (*f_rng)(void *, uint8_t *, size_t),
                   void *p_rng)
{
    int32_t ret = CE_SUCCESS;
    int32_t result               = 0;

    if ((NULL == grp) || (NULL == R) || (NULL == m) || (NULL == P)) {
        return CE_ERROR_BAD_PARAMS;
    }

    /* Check that P->Z equals to 1 */
    ret = ce_bn_cmp_u32(&(P->Z), 1, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_BAD_INPUT_DATA;
        goto end;
    }

    /* avoid build warning of unused parameters */
    (void)(f_rng);
    (void)(p_rng);

    ret = ce_bn_ecp_mul((const ce_bn_t *)(&(grp->P)),
                        (const ce_bn_t *)(&(grp->A)),
                        (const ce_bn_t *)(&(P->X)),
                        (const ce_bn_t *)(&(P->Y)),
                        (const ce_bn_t *)(&(P->Z)),
                        (const ce_bn_t *)(m),
                        &(R->X),
                        &(R->Y),
                        &(R->Z));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = CE_SUCCESS;
end:
    return ret;
}
#endif /* CE_LITE_ECP_PRIVATE */

/*
 * R = m * P + n * Q
 * Note: unlike mbedtls, we don't support m == 1 or -1.
 */
int32_t ce_ecp_muladd(const ce_ecp_group_t *grp,
                      ce_ecp_point_t *R,
                      const ce_bn_t *m,
                      const ce_ecp_point_t *P,
                      const ce_bn_t *n,
                      const ce_ecp_point_t *Q)
{
    int32_t ret = CE_SUCCESS;

    if ((NULL == grp) || (NULL == R) || (NULL == m) || (NULL == P) ||
        (NULL == n) || (NULL == Q)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_ecp_muladd((const ce_bn_t *)(&(grp->P)),
                           (const ce_bn_t *)(&(grp->A)),
                           (const ce_bn_t *)(&(P->X)),
                           (const ce_bn_t *)(&(P->Y)),
                           (const ce_bn_t *)(&(P->Z)),
                           (const ce_bn_t *)(m),
                           (const ce_bn_t *)(&(Q->X)),
                           (const ce_bn_t *)(&(Q->Y)),
                           (const ce_bn_t *)(&(Q->Z)),
                           (const ce_bn_t *)(n),
                           &(R->X),
                           &(R->Y),
                           &(R->Z));
    CHECK_SUCCESS_GOTO(ret, end);

end:
    return ret;
}

int32_t ce_ecp_check_pubkey(const ce_ecp_group_t *grp, const ce_ecp_point_t *pt)
{
    int32_t ret    = CE_SUCCESS;
    int32_t result = 0;
    ce_bn_t YY     = {NULL};
    ce_bn_t RHS    = {NULL};
    ce_bn_t T      = {NULL};

    if ((NULL == grp) || (NULL == pt)) {
        return CE_ERROR_BAD_PARAMS;
    }

    /* Must use affine coordinates, Z == 1 */
    ret = ce_bn_cmp_u32(&(pt->Z), 1, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    /* 0 < X and Y < P */
    ret = ce_bn_cmp_u32(&(pt->X), 0, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (1 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_u32(&(pt->Y), 0, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (1 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    ret = ce_bn_cmp_bn(&(pt->X), &(grp->P), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (-1 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn(&(pt->Y), &(grp->P), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (-1 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    /* init YY and RHS */
    ret = ce_bn_init(&YY);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&RHS);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = ce_bn_init(&T);
    CHECK_SUCCESS_GOTO(ret, end);

    /**
     * YY = Y^2
     */
    ret = ce_bn_mul_mod(&YY, (const ce_bn_t *)(&(pt->Y)),
                        (const ce_bn_t *)(&(pt->Y)),
                        (const ce_bn_t *)(&(grp->P)));
    CHECK_SUCCESS_GOTO(ret, end);

    /*
     * RHS = X (X^2 + A) + B = X^3 + A X + B
     */
    /* 1. T = X^2 mod P */
    ret = ce_bn_mul_mod(&T, (const ce_bn_t *)(&(pt->X)),
                        (const ce_bn_t *)(&(pt->X)),
                        (const ce_bn_t *)(&(grp->P)));
    CHECK_SUCCESS_GOTO(ret, end);
    /* 2. RHS = (T + A) mod P */
    ret =
        ce_bn_add_mod(&RHS, (const ce_bn_t *)(&T), (const ce_bn_t *)(&(grp->A)),
                      (const ce_bn_t *)(&(grp->P)));
    CHECK_SUCCESS_GOTO(ret, end);
    /* 3. T = (X * RHS) mod P */
    ret = ce_bn_mul_mod(&T, (const ce_bn_t *)(&(pt->X)),
                        (const ce_bn_t *)(&RHS), (const ce_bn_t *)(&(grp->P)));
    CHECK_SUCCESS_GOTO(ret, end);
    /* 4. RHS = (T + B) mod P */
    ret =
        ce_bn_add_mod(&RHS, (const ce_bn_t *)(&T), (const ce_bn_t *)(&(grp->B)),
                      (const ce_bn_t *)(&(grp->P)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&RHS), (const ce_bn_t *)(&YY),
                             &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (0 != result) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

end:
    ce_bn_free(&YY);
    ce_bn_free(&RHS);
    ce_bn_free(&T);
    return ret;
}

/* in verify r/s, the parameter check is using check_privkey */
int32_t ce_ecp_check_privkey(const ce_ecp_group_t *grp, const ce_bn_t *d)
{
    int32_t ret    = CE_SUCCESS;
    int32_t result = 0;

    if ((NULL == grp) || (NULL == d)) {
        return CE_ERROR_BAD_PARAMS;
    }

    /* SEC1 3.2, private key d: 1 <= d < N */
    ret = ce_bn_cmp_u32(d, 1, &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result < 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn(d, &(grp->N), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result >= 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    ret = CE_SUCCESS;
end:
    return ret;
}

#if defined(CE_LITE_ECP_PRIVATE)
int32_t ce_ecp_check_pub_priv(const ce_ecp_keypair_t *pub,
                              const ce_ecp_keypair_t *prv)
{
    int32_t ret          = CE_SUCCESS;
    int32_t result       = 0;
    ce_ecp_point_t tmp_Q = {0};

    if ((NULL == pub) || (NULL == prv)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_ecp_check_pubkey((const ce_ecp_group_t *)(&(pub->grp)),
                              (const ce_ecp_point_t *)(&(pub->Q)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_check_pubkey((const ce_ecp_group_t *)(&(prv->grp)),
                              (const ce_ecp_point_t *)(&(prv->Q)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_check_privkey((const ce_ecp_group_t *)(&(prv->grp)),
                               (const ce_bn_t *)(&(prv->d)));
    CHECK_SUCCESS_GOTO(ret, end);

    /* group id match */
    if ((pub->grp.id == CE_ECP_DP_NONE) || (pub->grp.id != prv->grp.id)) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    /* Q match */
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(pub->Q.X)),
                             (const ce_bn_t *)(&(prv->Q.X)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(pub->Q.Y)),
                             (const ce_bn_t *)(&(prv->Q.Y)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(pub->Q.Z)),
                             (const ce_bn_t *)(&(prv->Q.Z)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

    /* compute priv->d * G */
    ret = ce_ecp_point_init(&tmp_Q);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_ecp_mul((const ce_ecp_group_t *)(&(prv->grp)), &tmp_Q,
                     (const ce_bn_t *)(&(prv->d)),
                     (const ce_ecp_point_t *)(&(prv->grp.G)), NULL, NULL);
    CHECK_SUCCESS_GOTO(ret, end);

    /* prv->Q == tmp_Q */
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(tmp_Q.X)),
                             (const ce_bn_t *)(&(prv->Q.X)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(tmp_Q.Y)),
                             (const ce_bn_t *)(&(prv->Q.Y)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }
    ret = ce_bn_cmp_bn_equal((const ce_bn_t *)(&(tmp_Q.Z)),
                             (const ce_bn_t *)(&(prv->Q.Z)), &result);
    CHECK_SUCCESS_GOTO(ret, end);
    if (result != 0) {
        ret = CE_ERROR_INVAL_KEY;
        goto end;
    }

end:
    ce_ecp_point_free(&tmp_Q);
    return ret;
}
#endif /* CE_LITE_ECP_PRIVATE */

/* Internal used */
int32_t ce_ecp_copy(ce_ecp_point_t *P, const ce_ecp_point_t *Q)
{
    int32_t ret = CE_SUCCESS;

    if ((NULL == Q) || (NULL == P)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_copy(&(P->X), (const ce_bn_t *)(&(Q->X)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_copy(&(P->Y), (const ce_bn_t *)(&(Q->Y)));
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_copy(&(P->Z), (const ce_bn_t *)(&(Q->Z)));
    CHECK_SUCCESS_GOTO(ret, end);

end:
    return ret;
}

int32_t ce_ecp_group_copy(ce_ecp_group_t *dst, const ce_ecp_group_t *src)
{
    if ((NULL == dst) || (NULL == src)) {
        return CE_ERROR_BAD_PARAMS;
    }

    return ce_ecp_group_load(dst, src->id);
}

#if defined(CE_LITE_ECP_PRIVATE)
int32_t ce_ecp_gen_privkey(const ce_ecp_group_t *grp,
                           ce_bn_t *d,
                           int32_t (*f_rng)(void *, uint8_t *, size_t),
                           void *p_rng)
{
    int32_t ret     = CE_SUCCESS;
    int32_t result1 = 0;
    int32_t result2 = 0;
    int32_t count   = 0;

    if ((NULL == grp) || (NULL == d) || (NULL == f_rng)) {
        return CE_ERROR_BAD_PARAMS;
    }

    /* Unlike mbedtls, here we import random data with max bit length N bits */
    do {
        ret = ce_bn_import_random_max_bit_len(d, grp->nbits, f_rng, p_rng);
        CHECK_SUCCESS_GOTO(ret, end);

        if (++count > CE_RANDOM_RETRY_CNT) {
            return CE_ERROR_GEN_RANDOM;
        }

        ret = ce_bn_cmp_u32((const ce_bn_t *)d, 1, &result1);
        CHECK_SUCCESS_GOTO(ret, end);
        ret = ce_bn_cmp_bn((const ce_bn_t *)d, (const ce_bn_t *)(&(grp->N)),
                           &result2);
        CHECK_SUCCESS_GOTO(ret, end);
    } while ((result1 < 0) || (result2 >= 0));

    ret = CE_SUCCESS;
end:
    return ret;
}
#endif /* CE_LITE_ECP_PRIVATE */

int32_t ce_ecp_is_zero(ce_ecp_point_t *pt)
{
    int32_t ret    = CE_SUCCESS;
    int32_t result = 0;

    if (NULL == pt) {
        return CE_ERROR_BAD_PARAMS;
    }
    ret = ce_bn_cmp_u32((const ce_bn_t *)(&(pt->Z)), 0, &result);
    CHECK_SUCCESS_GOTO(ret, end);

    return (result == 0) ? (1) : (0);
end:
    return ret;
}

/**
 * The following inline functions are used to read/write length in DER encoding
 */

int32_t ce_asn1_get_len(uint8_t **p, const uint8_t *limit, size_t *len)
{
    if ((NULL == p) || (NULL == *p) || (NULL == limit) || (NULL == len)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if ((limit - *p) < 1) {
        return CE_ERROR_NO_DATA;
    }

    if ((**p & 0x80) == 0) {
        *len = (size_t)((*p)[0]);
        (*p)++;
    } else {
        switch (**p & 0x7F) {
        case 1:
            if ((limit - *p) < 2) {
                return CE_ERROR_NO_DATA;
            }

            *len = (size_t)((*p)[1]);
            (*p) += 2;
            break;

        case 2:
            if ((limit - *p) < 3) {
                return CE_ERROR_NO_DATA;
            }

            *len = ((size_t)(*p)[1] << 8) | (*p)[2];
            (*p) += 3;
            break;
        default:
            return CE_ERROR_BAD_INPUT_DATA;
        }
    }

    if (*len > (size_t)(limit - *p)) {
        return CE_ERROR_NO_DATA;
    }

    return CE_SUCCESS;
}

int32_t
ce_asn1_get_tag(uint8_t **p, const uint8_t *limit, size_t *len, int32_t tag)
{
    if ((NULL == p) || (NULL == *p) || (NULL == limit) || (NULL == len)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if ((limit - *p) < 1) {
        return CE_ERROR_NO_DATA;
    }

    if (**p != tag) {
        return CE_ERROR_BAD_INPUT_DATA;
    }

    (*p)++;

    return ce_asn1_get_len(p, limit, len);
}

int32_t ce_asn1_get_mpi(uint8_t **p, const uint8_t *limit, ce_bn_t *X)
{
    int32_t ret = CE_SUCCESS;
    size_t len  = 0;

    if ((NULL == p) || (NULL == *p) || (NULL == limit) || (NULL == X)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_asn1_get_tag(p, limit, &len, 0x02);
    CHECK_SUCCESS_GOTO(ret, end);

    ret = ce_bn_import(X, (const uint8_t *)(*p), len);
    CHECK_SUCCESS_GOTO(ret, end);

    *p += len;
end:
    return ret;
}

#if defined(CE_LITE_ECP_PRIVATE)
int32_t ce_asn1_write_len(uint8_t **p, uint8_t *start, size_t len)
{
    if ((NULL == p) || (NULL == *p) || (NULL == start)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (len < 0x80) {
        if (*p - start < 1) {
            return CE_ERROR_SHORT_BUFFER;
        }

        *--(*p) = (uint8_t)len;
        return 1;
    }

    if (len <= 0xFF) {
        if (*p - start < 2) {
            return CE_ERROR_SHORT_BUFFER;
        }

        *--(*p) = (uint8_t)len;
        *--(*p) = 0x81;
        return 2;
    }

    if (len <= 0xFFFF) {
        if (*p - start < 3) {
            return CE_ERROR_SHORT_BUFFER;
        }

        *--(*p) = (len)&0xFF;
        *--(*p) = (len >> 8) & 0xFF;
        *--(*p) = 0x82;
        return 3;
    }

    return CE_ERROR_BAD_INPUT_DATA;
}

int32_t ce_asn1_write_tag(uint8_t **p, uint8_t *start, uint8_t tag)
{
    if ((NULL == p) || (NULL == *p) || (NULL == start)) {
        return CE_ERROR_BAD_PARAMS;
    }

    if (*p - start < 1) {
        return CE_ERROR_SHORT_BUFFER;
    }

    *--(*p) = tag;

    return 1;
}

int32_t ce_asn1_write_mpi(uint8_t **p, uint8_t *start, const ce_bn_t *X)
{
    int32_t ret = CE_SUCCESS;
    size_t len  = 0;

    if ((NULL == p) || (NULL == *p) || (NULL == start) || (NULL == X)) {
        return CE_ERROR_BAD_PARAMS;
    }

    ret = ce_bn_bitlen(X);
    PAL_ASSERT(ret >= 0);
    len = (ret + 7) / 8;

    if (*p < start || (size_t)(*p - start) < len) {
        ret = CE_ERROR_SHORT_BUFFER;
        goto end;
    }

    (*p) -= len;
    ret = ce_bn_export(X, *p, len);
    CHECK_SUCCESS_GOTO(ret, end);
    /**
     * DER format assumes 2s complement for numbers, so the leftmost bit
     * should be 0 for positive numbers and 1 for negative numbers.
     */
    if (**p & 0x80) {
        if (*p - start < 1) {
            ret = CE_ERROR_SHORT_BUFFER;
            goto end;
        }

        *--(*p) = 0x00;
        len += 1;
    }

    ret = ce_asn1_write_len(p, start, len);
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;

    ret = ce_asn1_write_tag(p, start, 0x02);
    if (ret < 0) {
        goto end;
    }
    len += (size_t)ret;
    return (int32_t)len;

end:
    return ret;
}
#endif /* CE_LITE_ECP_PRIVATE */

#endif /* CE_LITE_ECP */