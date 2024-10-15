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

#if defined(CE_LITE_CONFIG_FILE)
#include CE_LITE_CONFIG_FILE
#else /* CE_LITE_CONFIG_FILE */

#ifndef CE_LITE_CONFIG_H
#define CE_LITE_CONFIG_H

/**
 * The following configurations enable or disable certain algorithm.
 */

/******************************************************************************/
/*                            HASH configurations                             */
/******************************************************************************/
/**
 * \brief Enable digest algorithm of sha224.
 */
#define CE_LITE_SHA224

/**
 * \brief Enable digest algorithm of sha256.
 */
#define CE_LITE_SHA256

/**
 * \brief Enable digest algorithm of sha1.
 */
#define CE_LITE_SHA1

/**
 * \brief Enable digest algorithm of SM3.
 */
#define CE_LITE_SM3

/******************************************************************************/
/*                             ACA configurations                             */
/******************************************************************************/
/**
 * \brief Enable bignumber for asymmetric crypto algorithms(ACA) such as RSA,
 *        ECDSA.
 *        This must be enabled if one of the following is enabled:
 *        \c CE_LITE_RSASSA
 *        \c CE_LITE_ECP
 *        \c CE_LITE_ECDSA
 *        \c CE_LITE_SM2DSA
 */
#define CE_LITE_BN

/**
 * \brief Enable blinding in ACA operations. This controls:
 *        1. The exponent blinding and message blinding in RSA sign.
 *        2. The point multiplication in ECP.
 *        3. The ECDSA blinding in sign.
 *        4. The SM2DSA blinding in sign.
 */
#define CE_LITE_BLINDING

/******************************************************************************/
/*                             RSA configurations                             */
/******************************************************************************/
/**
 * \brief Enable RSA Signature Scheme with Appendix.
 *        Details about RSASSA please see Chapter 8 of PKCS #1: RSA Cryptography
 *        Specifications Version 2.2
 *
 * Requires: CE_LITE_BN
 */
#define CE_LITE_RSASSA

/**
 * \brief Enable the sign method of RSASSA.
 *
 * Requires: CE_LITE_RSASSA
 */
#define CE_LITE_RSASSA_SIGN

/**
 * \brief Enable PKCS1-v1_5 padding for RSA algorithm.
 *
 * Requires: CE_LITE_RSASSA
 */
#define CE_LITE_RSA_PKCS_V15

/**
 * \brief Enable RSASSA-PSS padding for RSA algorithm.
 *
 * Requires: CE_LITE_RSASSA and one of SHA256, SHA1.
 */
#define CE_LITE_RSA_PKCS_V21

/******************************************************************************/
/*                          ECP && SM2 configurations                         */
/******************************************************************************/
/**
 * \brief Enable Elliptic curves over GF(p) related algorithms, which includes
 *        ECDSA, SM2DSA.
 *
 * Requires: CE_LITE_BN
 */
#define CE_LITE_ECP

/**
 * \brief Enable specific curves within the Elliptic Curve module.
 *
 */
#define CE_LITE_ECP_DP_SECP256R1
#define CE_LITE_ECP_DP_SECP521R1
#define CE_LITE_ECP_DP_SM2P256V1

/**
 * \brief Enable the elliptic curve DSA algorithm.
 *
 * Requires: CE_LITE_ECP, one of ECP curves: SECP256R1, SECP521R1
 */
#define CE_LITE_ECDSA

/**
 * \brief Enable the sign method of ECDSA.
 *
 * Requires: CE_LITE_ECDSA
 */
#define CE_LITE_ECDSA_SIGN

/**
 * \brief Enable signature verification algorithm of SM2DSA.
 *
 *        Details about SM2DSA please see SM2椭圆曲线公钥密码算法
 *        第2部分：数字签名算法
 *
 * Requires: CE_LITE_SM3, CE_LITE_ECP, ECP curve: SM2P256V1
 */
#define CE_LITE_SM2DSA

/**
 * \brief Enable the sign method of SM2DSA.
 *
 * Requires: CE_LITE_SM2DSA
 */
#define CE_LITE_SM2DSA_SIGN

/******************************************************************************/
/*                                  Cipher                                    */
/******************************************************************************/
/**
 * \brief Enable cipher scheme AES.
 */
#define CE_LITE_AES

/**
 * \brief Enable cipher scheme SM4
 */
#define CE_LITE_SM4

/**
 * \brief Enable specific modes within the cipher algorithms.
 *        One of them must be enabled to use cipher algorithms.
 *
 * Requires: one of the cipher algorithms: AES, SM4
 */
#define CE_LITE_CIPHER_MODE_ECB
#define CE_LITE_CIPHER_MODE_CBC
#define CE_LITE_CIPHER_MODE_CTR

/**
 * \brief Enable PKCS #7 padding with using block-based cipher mode(ECB or CBC).
 *        Details about PKCS #7 padding see <PKCS #7: Cryptographic Message
 *        Syntax Version 1.5>
 */
#define CE_LITE_CIPHER_PADDING_PKCS7

/**
 * \brief Enable cipher call updating IV/stream block after each cipher operation.
 *
 *        For CBC cipher mode, calling the crypt APIs will update the IV value in
 *        parameter. When the macro \c CE_LITE_CIPHER_MULTI_BLOCKS is not defined,
 *        IV array is always treated as input parameter.
 *
 *        For CTR cipher mode, calling the crypt APIs will update nonce_counter and
 *        stream block, which are reused in the next crypt session. When the macro
 *        \c CE_LITE_CIPHER_MULTI_BLOCKS is not defined, nonce_counter is always
 *        treated as input parameter, the nc_off and stream block are ignored.
 *
 *        It affects the following 6 functions:
 *        mbedtls_aes_crypt_cbc
 *        mbedtls_aes_crypt_cbc_pkcs7
 *        mbedtls_aes_crypt_ctr
 *        mbedtls_sm4_crypt_cbc
 *        mbedtls_sm4_crypt_cbc_pkcs7
 *        mbedtls_sm4_crypt_ctr
 */
#define CE_LITE_CIPHER_MULTI_BLOCKS

/**
 * \brief Enable OTP driver.
 *        One and only one of the otp type: normal, PUF, dummy must be selected.
 */
#define CE_LITE_OTP

/**
 * \brief Select the OTP type.
 *
 * \note The default OTP type is normal otp. If the dummy or PUF OTP is used,
 *       please define \c CE_LITE_OTP_PUF or \c CE_LITE_OTP_DUMMY after \c CE_LITE_OTP .
 *       Pay attention to \c CE_LITE_OTP_PUF or \c CE_LITE_OTP_DUMMY definition in your
 *       compile system to avoid duplicated definitions or conflicts.
 *
 * Requires: CE_LITE_OTP
 */
#if defined(CE_LITE_OTP) && !defined(CE_LITE_OTP_PUF) && !defined(CE_LITE_OTP_DUMMY)
#define CE_LITE_OTP_NORMAL
#endif /* CE_LITE_OPT && !CE_LITE_OTP_PUF && !CE_LITE_OTP_DUMMY */

/**
 * \brief Enable PUF OTP enrollment.
 *        Default enabled in case of PUF OTP.
 *
 * Requires: CE_LITE_OTP && CE_LITE_OTP_PUF
 * */
#if defined(CE_LITE_OTP_PUF)
#define CE_LITE_OTP_PUF_ENROLL
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief Enable PUF OTP quality check test.
 *        Default enabled in case of PUF OTP.
 *
 * Requires: CE_LITE_OTP && CE_LITE_OTP_PUF
 */
#if defined(CE_LITE_OTP_PUF)
#define CE_LITE_OTP_PUF_QUALITY_CHECK
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief Enable PUF OTP init margin read test.
 *        Default enabled in case of PUF OTP.
 *
 * Requires: CE_LITE_OTP && CE_LITE_OTP_PUF
 */
#if defined(CE_LITE_OTP_PUF)
#define CE_LITE_OTP_PUF_INIT_MGN_READ
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief Enable PUF OTP program margin read test.
 *        Default enabled in case of PUF OTP.
 *
 * Requires: CE_LITE_OTP && CE_LITE_OTP_PUF
 */
#if defined(CE_LITE_OTP_PUF)
#define CE_LITE_OTP_PUF_PGM_MGN_READ
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief Enable alert message for DR revocation
 *        actived and invalid.
 *
 * Requires: CE_LITE_OTP, CE_LITE_OTP_NORMAL or
 *           CE_LITE_OTP_PUF
 */
#undef CE_LITE_OTP_ALERT

/**
 * \brief Enable OTP write operations in OTP driver.
 *
 * Requires: CE_LITE_OTP
 */
#define CE_LITE_OTP_WRITE

/**
 * \brief Enable TRNG.
 *
 */
#define CE_LITE_TRNG

/**
 * \brief Enable TRNG DUMP for TRNG calibration.
 *
 */
#define CE_LITE_TRNG_DUMP

#endif /* CE_LITE_CONFIG_H */

#endif /* !CE_LITE_CONFIG_FILE */

/* Check configurations immediately */
#include "ce_lite_check_config.h"
