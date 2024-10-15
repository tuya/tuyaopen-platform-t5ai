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

#ifndef MBEDTLS_SHA256_H
#define MBEDTLS_SHA256_H

#include "pal_common.h"

#define MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED                -0x0037  /**< SHA-256 hardware accelerator failed */
#define MBEDTLS_ERR_SHA256_BAD_INPUT_DATA                 -0x0074  /**< SHA-256 input data was malformed. */
#define MBEDTLS_ERR_SHA256_ALLOC_FAILED                   -0x0010  /**< Failed to allocate memory. */

#ifdef __cplusplus
extern "C" {
#endif

struct ce_dgst_ctx_t;

/**
 * \brief          The SHA-256 context structure.
 *
 *                 The SHA-256 algorithm leverages CE driver, and the internal CE
 *                 context is maintained by CE driver.
 */
typedef struct mbedtls_sha256_context {
    uint32_t magic;                 /*!<  The magic value to identify SHA256 context */
    struct ce_dgst_ctx_t *ce_ctx;   /*!<  pointer to CE SHA256 internal context */
} mbedtls_sha256_context;

/**
 * \brief          This function initializes a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to initialize. This must not be \c NULL.
 *                 If ctx is \c NULL, this function will return directly.
 */
void mbedtls_sha256_init(mbedtls_sha256_context *ctx);

/**
 * \brief          This function clears a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to clear. This may be \c NULL, in which
 *                 case this function returns immediately. If it is not \c NULL,
 *                 it must point to an initialized SHA-256 context.
 *                 Otherwise, it will return directly.
 */
void mbedtls_sha256_free(mbedtls_sha256_context *ctx);

/**
 * \brief          This function starts a SHA224 or SHA-256 checksum
 *                 calculation.
 * \note           This function will assert when malloc internal ctx failed.
 *
 * \param ctx      The context to use. This must be initialized.
 * \param is224    Determines which function to use:
 *                 0: Use SHA-256, otherwise: Use SHA-224.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA256_BAD_INPUT_DATA if \p ctx is \c NULL.
 * \return         \c MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED if there is hardware error or hardware busy.
 */
int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224);

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-256 checksum calculation.
 * \note           The maxmum supported input size for this function is 4G - 1 (0xFFFFFFFF).
 *                 If the target size >= 4G, the caller should split it into
 *                 several pieces with each size < 4G.
 *
 * \param ctx      The SHA-256 context. This must be initialized
 *                 and have a hash operation started.
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA256_BAD_INPUT_DATA if \p ctx is \c NULL,
 *                 or \p ilen > 0xFFFFFFFF, or \p ilen > 0 and \p input is \c NULL.
 * \return         \c MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED if there is hardware error or hardware busy.
 */
int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
                              const unsigned char *input,
                              size_t ilen);

/**
 * \brief          This function finishes the SHA-256 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SHA-256 context. This must be initialized
 *                 and have a hash operation started.
 * \param output   The SHA224 or SHA-256 checksum result.
 *                 This must be a writable buffer of length \c 32 Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA256_BAD_INPUT_DATA if \p ctx is \c NULL.
 * \return         \c MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx,
                              unsigned char output[32]);

/**
 * \brief          This function calculates the SHA-256
 *                 checksum of a buffer.
 *
 * \note           The maxmum supported input size for this function is 4G - 1 (0xFFFFFFFF).
 *                 If the target size >= 4G, the caller can use mbedtls_sha256_update_ret and
 *                 should split it into several pieces with each size < 4G.
 *
 *                 The SHA-256 result is calculated as
 *                 output = SHA-256(input buffer).
 *
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 * \param output   The SHA-256 checksum result. This must
 *                 be a writable buffer of length \c 32 Bytes.
 * \param is224    Determines which function to use:
 *                 0: Use SHA-256, otherwise: Use SHA-224.
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA256_BAD_INPUT_DATA if \p ilen > 0xFFFFFFFF,
 *                 or \p ilen > 0 and \p input is \c NULL, or \p output is \c NULL
 * \return         \c MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sha256_ret(const unsigned char *input,
                       size_t ilen,
                       unsigned char output[32],
                       int is224);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA256_H */
