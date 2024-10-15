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

#ifndef __CE_LITE_COMMON_H__
#define __CE_LITE_COMMON_H__

#include "pal_common.h"
#include "pal_log.h"
#include "pal_heap.h"
#include "pal_string.h"
#include "pal_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                              Error Code                                    */
/******************************************************************************/
/* API Error Codes (GP TEE compliant) */
#define CE_SUCCESS                       0x00000000
#define CE_ERROR_GENERIC                 0xFFFF0000
#define CE_ERROR_ACCESS_DENIED           0xFFFF0001
#define CE_ERROR_CANCEL                  0xFFFF0002
#define CE_ERROR_ACCESS_CONFLICT         0xFFFF0003
#define CE_ERROR_EXCESS_DATA             0xFFFF0004
#define CE_ERROR_BAD_FORMAT              0xFFFF0005
#define CE_ERROR_BAD_PARAMS              0xFFFF0006
#define CE_ERROR_BAD_STATE               0xFFFF0007
#define CE_ERROR_ITEM_NOT_FOUND          0xFFFF0008
#define CE_ERROR_NOT_IMPLEMENTED         0xFFFF0009
#define CE_ERROR_NOT_SUPPORTED           0xFFFF000A
#define CE_ERROR_NO_DATA                 0xFFFF000B
#define CE_ERROR_OOM                     0xFFFF000C
#define CE_ERROR_BUSY                    0xFFFF000D
#define CE_ERROR_COMMUNICATION           0xFFFF000E
#define CE_ERROR_SECURITY                0xFFFF000F
#define CE_ERROR_SHORT_BUFFER            0xFFFF0010
#define CE_ERROR_EXTERNAL_CANCEL         0xFFFF0011
#define CE_ERROR_TIMEOUT                 0xFFFF3001
#define CE_ERROR_OVERFLOW                0xFFFF300F

/**
 * Extended Error Codes
 *
 * +------+----------+
 * |Type  |Range     |
 * +------+----------+
 * |Common|0x800000xx|
 * +------+----------+
 * |Cipher|0x800001xx|
 * +------+----------+
 * |MPI   |0x800002xx|
 * +------+----------+
 * |DHM   |0x800003xx|
 * +------+----------+
 * |PK    |0x800004xx|
 * +------+----------+
 */
#define CE_ERROR_AGAIN                   0x80000000
#define CE_ERROR_FEATURE_UNAVAIL         0x80000001
#define CE_ERROR_BAD_KEY_LENGTH          0x80000002
#define CE_ERROR_INVAL_KEY               0x80000003
#define CE_ERROR_BAD_INPUT_LENGTH        0x80000004
#define CE_ERROR_BAD_INPUT_DATA          0x80000005
#define CE_ERROR_AUTH_FAILED             0x80000006
#define CE_ERROR_INVAL_CTX               0x80000007
#define CE_ERROR_UNKNOWN_ALG             0x80000008
#define CE_ERROR_INVAL_ALG               0x80000009

#define CE_ERROR_INVAL_PADDING           0x80000100
#define CE_ERROR_INCOMPLETE_BLOCK        0x80000101

#define CE_ERROR_INVAL_CHAR              0x80000200
#define CE_ERROR_NEGATIVE_VALUE          0x80000201
#define CE_ERROR_DIV_BY_ZERO             0x80000202
#define CE_ERROR_NOT_ACCEPTABLE          0x80000203
#define CE_ERROR_NO_SRAM_SPACE           0x80000204
#define CE_ERROR_NO_AVAIL_GR             0x80000205
#define CE_ERROR_NO_AVAIL_LEN_TYPE       0x80000206
#define CE_ERROR_INVAL_MOD               0x80000207
#define CE_ERROR_NOT_PRIME               0x80000208
#define CE_ERROR_OP_TOO_LONG             0x80000209

#define CE_ERROR_READ_PARAMS             0x80000300
#define CE_ERROR_MAKE_PARAMS             0x80000301
#define CE_ERROR_READ_PUBLIC             0x80000302
#define CE_ERROR_MAKE_PUBLIC             0x80000303
#define CE_ERROR_CALC_SECRET             0x80000304
#define CE_ERROR_SET_GROUP               0x80000305

#define CE_ERROR_GEN_RANDOM              0x80000400
#define CE_ERROR_TYPE_MISMATCH           0x80000401
#define CE_ERROR_KEY_VERSION             0x80000402
#define CE_ERROR_KEY_FORMAT              0x80000403
#define CE_ERROR_INVAL_PUBKEY            0x80000404
#define CE_ERROR_UNKNOWN_CURVE           0x80000405
#define CE_ERROR_SIG_LENGTH              0x80000406
#define CE_ERROR_GEN_KEY                 0x80000407
#define CE_ERROR_CHECK_KEY               0x80000408
#define CE_ERROR_PUBLIC_OP               0x80000409
#define CE_ERROR_PRIVATE_OP              0x8000040A
#define CE_ERROR_VERIFY_SIG              0x8000040B
#define CE_ERROR_OUT_TOO_LARGE           0x8000040C
#define CE_ERROR_BAD_PADDING             0x8000040D

/******************************************************************************/
/*                           Algorithem identifier                            */
/******************************************************************************/
/**
 * Algorithem identifier type
 */
typedef uint32_t ce_algo_t;

/* Main Algorithm */
#define CE_MAIN_ALGO_SHA1                 0x02
#define CE_MAIN_ALGO_SHA224               0x03
#define CE_MAIN_ALGO_SHA256               0x04
#define CE_MAIN_ALGO_SM3                  0x07
#define CE_MAIN_ALGO_AES                  0x10
#define CE_MAIN_ALGO_SM4                  0x14
#define CE_MAIN_ALGO_RSA                  0x30
#define CE_MAIN_ALGO_SM2                  0x45

/*
 * Algorithm Identifiers (compliant with GP Core API v1.2.1)
 * Bitwise value with assignments
 *    [31:28]    class (operation)
 *    [23:20]    internal hash
 *    [15:12]    digest hash
 *    [11:8]     chain mode
 *    [7:0]      main algo
 */
#define CE_ALG_AES_ECB_NOPAD                   0x10000010
#define CE_ALG_AES_CBC_NOPAD                   0x10000110
#define CE_ALG_AES_CTR                         0x10000210
#define CE_ALG_AES_ECB_PKCS7                   0x10000B10
#define CE_ALG_AES_CBC_PKCS7                   0x10000C10
#define CE_ALG_SM4_ECB_NOPAD                   0x10000014
#define CE_ALG_SM4_CBC_NOPAD                   0x10000114
#define CE_ALG_SM4_CTR                         0x10000214
#define CE_ALG_SM4_ECB_PKCS7                   0x10000B14
#define CE_ALG_SM4_CBC_PKCS7                   0x10000C14
#define CE_ALG_RSASSA_PKCS1_V1_5_SHA1          0x70002830
#define CE_ALG_RSASSA_PKCS1_V1_5_SHA224        0x70003830
#define CE_ALG_RSASSA_PKCS1_V1_5_SHA256        0x70004830
#define CE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA1      0x70212930
#define CE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA224    0x70313930
#define CE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA256    0x70414930
#define CE_ALG_RSA_NOPAD                       0x60000030
#define CE_ALG_SHA1                            0x50000002
#define CE_ALG_SHA224                          0x50000003
#define CE_ALG_SHA256                          0x50000004
#define CE_ALG_SM3                             0x50000007
#define CE_ALG_ECDSA_P256                      0x70003041
#define CE_ALG_ECDSA_P521                      0x70005041
#define CE_ALG_SM2_DSA_SM3                     0x70007045

/******************************************************************************/
/*                                 UTIL Size                                  */
/******************************************************************************/
/**
 * SCA block size
 */
#define CE_AES_BLOCK_SIZE      16UL
#define CE_SM4_BLOCK_SIZE      16UL
#define CE_MAX_SCA_BLOCK       16UL

/**
 * SCA key size
 */
#define CE_SM4_KEY_SIZE        16UL
#define CE_MAX_AES_KEY         32UL
#define CE_MAX_SCA_KEY         32UL

/**
 * HASH block size
 */
#define CE_SHA1_BLK_SIZE       64UL
#define CE_SHA224_BLK_SIZE     64UL
#define CE_SHA256_BLK_SIZE     64UL
#define CE_SM3_BLK_SIZE        64UL
#define CE_MAX_HASH_BLOCK      CE_SHA256_BLK_SIZE

/**
 * HASH digest size
 */
#define CE_SHA1_HASH_SIZE      20UL
#define CE_SHA224_HASH_SIZE    28UL
#define CE_SHA256_HASH_SIZE    32UL
#define CE_SM3_HASH_SIZE       32UL
#define CE_MAX_HASH_SIZE       CE_SHA256_HASH_SIZE

/******************************************************************************/
/*                                   APIs                                     */
/******************************************************************************/

/**
 * \brief           This function initializes the CE driver.
 * \return          \c TE_SUCCESS on success.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_drv_init(void);

/**
 * \brief           This function deinitializes the CE driver.
 */
void ce_drv_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_COMMON_H__ */
