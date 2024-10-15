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
#ifndef CE_LITE_CHECK_CONFIG_H
#define CE_LITE_CHECK_CONFIG_H

#include "ce_lite_config.h"

#if defined(CE_LITE_SHA256) || defined(CE_LITE_SHA1) ||                    \
    defined(CE_LITE_SM3)  || defined(CE_LITE_SHA224)
#define CE_LITE_HASH
#endif

#if defined(CE_LITE_RSASSA) && (!defined(CE_LITE_BN))
#error "CE_LITE_RSASSA requires CE_LITE_BN"
#endif

#if defined(CE_LITE_RSASSA_SIGN) && (!defined(CE_LITE_RSASSA))
#error "CE_LITE_RSASSA_SIGN requires CE_LITE_RSASSA"
#endif

#if defined(CE_LITE_RSA_PKCS_V15) && (!defined(CE_LITE_RSASSA))
#error "CE_LITE_RSA_PKCS_V15 requires CE_LITE_RSASSA"
#endif

#if defined(CE_LITE_RSA_PKCS_V21) && (!defined(CE_LITE_RSASSA))
#error "CE_LITE_RSA_PKCS_V21 requires CE_LITE_RSASSA"
#endif

#if defined(CE_LITE_RSA_PKCS_V21) &&                                           \
    ((!defined(CE_LITE_SHA256)) &&                                             \
    (!defined(CE_LITE_SHA1)) && (!defined(CE_LITE_SHA224)))
#error "CE_LITE_RSA_PKCS_V21 requires one of hash algorithms: SHA256, SHA224, SHA1"
#endif

#if defined(CE_LITE_ECDSA_SIGN) || defined(CE_LITE_SM2DSA_SIGN)
#define CE_LITE_ECP_PRIVATE
#endif

#if defined(CE_LITE_ECP) && (!defined(CE_LITE_BN))
#error "CE_LITE_ECP requires CE_LITE_BN"
#endif

#if defined(CE_LITE_ECP) &&                                                    \
    (!defined(CE_LITE_ECP_DP_SECP256R1) &&                                     \
     !defined(CE_LITE_ECP_DP_SECP521R1) && !defined(CE_LITE_ECP_DP_SM2P256V1))
#error "CE_LITE_ECP is defined, but not curve is enabled"
#endif

#if defined(CE_LITE_ECDSA) && (!defined(CE_LITE_ECP))
#error "CE_LITE_ECDSA requires CE_LITE_ECP"
#endif

#if defined(CE_LITE_ECDSA) &&                                                  \
    (!defined(CE_LITE_ECP_DP_SECP256R1) && !defined(CE_LITE_ECP_DP_SECP521R1))
#error "CE_LITE_ECDSA is defined, but no supported curve is enabled"
#endif

#if defined(CE_LITE_ECDSA_SIGN) && (!defined(CE_LITE_ECDSA))
#error "CE_LITE_ECDSA_SIGN requires CE_LITE_ECDSA"
#endif

#if defined(CE_LITE_SM2DSA) && (!defined(CE_LITE_ECP))
#error "CE_LITE_SM2DSA requires CE_LITE_ECP"
#endif

#if defined(CE_LITE_SM2DSA) && (!defined(CE_LITE_SM3))
#error "CE_LITE_SM2DSA requires CE_LITE_SM3"
#endif

#if defined(CE_LITE_SM2DSA) && (!defined(CE_LITE_ECP_DP_SM2P256V1))
#error "CE_LITE_SM2DSA is defined, but no supported curve is enabled"
#endif

#if defined(CE_LITE_SM2DSA_SIGN) && (!defined(CE_LITE_SM2DSA))
#error "CE_LITE_SM2DSA_SIGN requires CE_LITE_SM2DSA"
#endif

#if defined(CE_LITE_AES) || defined(CE_LITE_SM4)
#define CE_LITE_CIPHER
#endif

#if (defined(CE_LITE_AES) || defined(CE_LITE_SM4)) &&                          \
    ((!defined(CE_LITE_CIPHER_MODE_ECB)) &&                                    \
     (!defined(CE_LITE_CIPHER_MODE_CBC)) &&                                    \
     (!defined(CE_LITE_CIPHER_MODE_CTR)))
#error "Cipher algorithm requires one of cipher mode: ECB, CBC, CTR"
#endif

#if (defined(CE_LITE_CIPHER_MODE_ECB) ||                                       \
     defined(CE_LITE_CIPHER_MODE_CBC) ||                                       \
     defined(CE_LITE_CIPHER_MODE_CTR)) &&                                      \
    ((!defined(CE_LITE_AES)) && (!defined(CE_LITE_SM4)))
#error "Cipher mode requires one of cipher algorithm: AES, SM4"
#endif

#if defined(CE_LITE_OTP) &&                                                    \
    ((!defined(CE_LITE_OTP_NORMAL)) && (!defined(CE_LITE_OTP_PUF)) &&          \
     (!defined(CE_LITE_OTP_DUMMY)))
#error                                                                         \
    "CE_LITE_OTP is defined, but non of OTP types(normal, PUF, dummy) is enabled"
#endif

#if defined(CE_LITE_OTP) &&                                                    \
    ((defined(CE_LITE_OTP_NORMAL) && defined(CE_LITE_OTP_PUF)) ||              \
    (defined(CE_LITE_OTP_NORMAL) && defined(CE_LITE_OTP_DUMMY)) ||             \
    (defined(CE_LITE_OTP_DUMMY) && defined(CE_LITE_OTP_PUF)))
#error                                                                         \
    "CE_LITE_OTP is defined, but more than one OTP type is enabled"
#endif

#if defined(CE_LITE_OTP_NORMAL) && (!defined(CE_LITE_OTP))
#error "CE_LITE_OTP_NORMAL requires CE_LITE_OTP"
#endif

#if defined(CE_LITE_OTP_PUF) && (!defined(CE_LITE_OTP))
#error "CE_LITE_OTP_PUF requires CE_LITE_OTP"
#endif

#if defined(CE_LITE_OTP_DUMMY) && (!defined(CE_LITE_OTP))
#error "CE_LITE_OTP_DUMMY requires CE_LITE_OTP"
#endif

#if defined(CE_LITE_OTP_WRITE) && (!defined(CE_LITE_OTP))
#error "CE_LITE_OTP_WRITE requires CE_LITE_OTP"
#endif

#if defined(CE_LITE_OTP_PUF_ENROLL) &&                                         \
    (!defined(CE_LITE_OTP_PUF) || (!defined(CE_LITE_OTP)))
#error "CE_LITE_OTP_PUF_ENROLL requires CE_LITE_OTP and CE_LITE_OTP_PUF"
#endif

#if defined(CE_LITE_OTP_PUF_QUALITY_CHECK) &&                                  \
    (!defined(CE_LITE_OTP_PUF) || (!defined(CE_LITE_OTP)))
#error "CE_LITE_OTP_PUF_QUALITY_CHECK requires CE_LITE_OTP and CE_LITE_OTP_PUF"
#endif

#if defined(CE_LITE_OTP_PUF_INIT_MGN_READ) &&                                  \
    (!defined(CE_LITE_OTP_PUF) || (!defined(CE_LITE_OTP)))
#error "CE_LITE_OTP_PUF_INIT_MGN_READ requires CE_LITE_OTP and CE_LITE_OTP_PUF"
#endif

#if defined(CE_LITE_OTP_PUF_PGM_MGN_READ) &&                                   \
    (!defined(CE_LITE_OTP_PUF) || (!defined(CE_LITE_OTP)))
#error "CE_LITE_OTP_PUF_PGM_MGN_READ requires CE_LITE_OTP and CE_LITE_OTP_PUF"
#endif

#if defined(CE_LITE_OTP_ALERT) &&                                              \
    ((!defined(CE_LITE_OTP_PUF) && !defined(CE_LITE_OTP_NORMAL)) ||            \
    (!defined(CE_LITE_OTP)))
#error "CE_LITE_OTP_ALERT requires CE_LITE_OTP and one of CE_LITE_OTP_PUF"\
       "or CE_LITE_OTP_NORMAL"
#endif

#if !defined(CE_LITE_OTP_WRITE) && defined(CE_LITE_OTP_PUF_ENROLL)
#error "CE_LITE_OTP_PUF_ENROLL requires CE_LITE_OTP_WRITE"
#endif

#if defined(CE_LITE_TRNG_DUMP) && (!defined(CE_LITE_TRNG))
#error "CE_LITE_TRNG_DUMP requires CE_LITE_TRNG"
#endif

#endif /* CE_LITE_CHECK_CONFIG_H */
