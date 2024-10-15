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
#if CONFIG_TE200_UT
#ifndef MBEDTLS_SHA1_H
#define MBEDTLS_SHA1_H

#include "pal_common.h"

#define MBEDTLS_ERR_SHA1_ALLOC_FAILED                     -0x0010  /**< Failed to allocate memory. */
#define MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED                  -0x0035  /**< SHA-1 hardware accelerator failed */
#define MBEDTLS_ERR_SHA1_BAD_INPUT_DATA                   -0x0073  /**< SHA-1 input data was malformed. */

#ifdef __cplusplus
extern "C" {
#endif

struct ce_dgst_ctx_t;

#define MBEDTLS_SHA1_MAGIC       (0x53484131U)         /* SHA1 */

/**
 * \brief          The SHA-1 context structure.
 *
 *                 The SHA-1 algorithm leverages CE driver, and the internal CE
 *                 context is maintained by CE driver.
 */
typedef struct mbedtls_sha1_context {
    uint32_t magic;                 /*!<  The magic value to identify SHA1 context */
    struct ce_dgst_ctx_t *ce_ctx;   /*!<  pointer to CE SHA1 internal context */
} mbedtls_sha1_context;

/**
 * \brief          This function initializes a SHA-1 context.
 *
 * \param ctx      The SHA-1 context to initialize. This must not be \c NULL.
 *                 If ctx is \c NULL, this function will return directly.
 */
void mbedtls_sha1_init(mbedtls_sha1_context *ctx);

/**
 * \brief          This function clears a SHA-1 context.
 *
 * \param ctx      The SHA-1 context to clear. This may be \c NULL, in which
 *                 case this function returns immediately. If it is not \c NULL,
 *                 it must point to an initialized SHA-1 context.
 *                 Otherwise, it will return directly.
 */
void mbedtls_sha1_free(mbedtls_sha1_context *ctx);

/**
 * \brief          This function starts a SHA-1 checksum
 *                 calculation.
 * \note           This function will assert when malloc internal ctx failed.
 *
 * \param ctx      The context to use. This must be initialized.
 *                 Otherwise, will return \c MBEDTLS_ERR_SHA1_BAD_INPUT_DATA.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA1_BAD_INPUT_DATA if \p ctx is \c NULL or
 *                 \p ctx->magic is not \c MBEDTLS_SHA1_MAGIC.
 * \return         \c MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED if there is hardware error or hardware busy.
 */
int mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx);

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-1 checksum calculation.
 * \note           The maxmum supported input size for this function is 4G - 1 (0xFFFFFFFF).
 *                 If the target size >= 4G, the caller should split it into
 *                 several pieces with each size < 4G.
 *
 * \param ctx      The SHA-1 context. This must be initialized
 *                 and have a hash operation started.
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA1_BAD_INPUT_DATA if \p ctx is \c NULL, or \p ilen > 0xFFFFFFFF, or
 *                 \p ilen > 0 and \p input is \c NULL, or \p ctx->magic is not \c MBEDTLS_SHA1_MAGIC.
 * \return         \c MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED if there is hardware error or hardware busy.
 */
int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx,
                            const unsigned char *input,
                            size_t ilen);

/**
 * \brief          This function finishes the SHA-1 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SHA-1 context. This must be initialized
 *                 and have a hash operation started.
 * \param output   The SHA-1 checksum result.
 *                 This must be a writable buffer of length \c 20 Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA1_BAD_INPUT_DATA if \p ctx is \c NULL or
 *                 \p ctx->magic is not \c MBEDTLS_SHA1_MAGIC.
 * \return         \c MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx,
                            unsigned char output[20]);

/**
 * \brief          This function calculates the SHA-1
 *                 checksum of a buffer.
 *
 * \note           The maxmum supported input size for this function is 4G - 1 (0xFFFFFFFF).
 *                 If the target size >= 4G, the caller can use mbedtls_sha1_update_ret and
 *                 should split it into several pieces with each size < 4G.
 *
 *                 The SHA-1 result is calculated as
 *                 output = SHA-1(input buffer).
 *
 * \param input    The buffer holding the data. This must be a readable
 *                 buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 * \param output   The SHA-1 checksum result. This must
 *                 be a writable buffer of length \c 20 Bytes.
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SHA1_BAD_INPUT_DATA if \p ilen > 0xFFFFFFFF,
 *                 or \p ilen > 0 and \p input is \c NULL, or \p output is \c NULL
 * \return         \c MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sha1_ret(const unsigned char *input,
                     size_t ilen,
                     unsigned char output[20]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA1_H */
#endif
