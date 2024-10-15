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

#ifndef MBEDTLS_SM3_H
#define MBEDTLS_SM3_H

#include "pal_common.h"

#define MBEDTLS_ERR_SM3_BAD_INPUT_DATA                    -0x0075  /**< SM3 input data was malformed. */
#define MBEDTLS_ERR_SM3_FEATURE_UNAVAILABLE               -0x0040  /**< SM3 feature not available */
#define MBEDTLS_ERR_SM3_ALLOC_FAILED                      -0x0010  /**< Failed to allocate memory. */
#define MBEDTLS_ERR_SM3_HW_ACCEL_FAILED                   -0x0039  /**< SM3 hardware accelerator failed */

#ifdef __cplusplus
extern "C" {
#endif

struct ce_dgst_ctx_t;

/**
 * \brief          SM3 context structure
 * \note           The SM3 algorithm leverages CE driver, and the internal CE
 *                 context is maintained by CE driver.
 */
typedef struct mbedtls_sm3_context {
    uint32_t magic;               /*!<  The magic value to identify SM3 context */
    struct ce_dgst_ctx_t *ce_ctx; /*!<  pointer to CE SM3 internal context */
} mbedtls_sm3_context;

/**
 * \brief          This function initializes a SM3 context.
 *
 * \param ctx      The SM3 context to initialize. This must
 *                 not be \c NULL. If ctx is \c NULL, this function will return directly.
 */
void mbedtls_sm3_init(mbedtls_sm3_context *ctx);

/**
 * \brief          This function clears a SM3 context.
 *
 * \param ctx      The SM3 context to clear. This may be \c NULL,
 *                 in which case this function does nothing. If it
 *                 is not \c NULL, it must point to an initialized
 *                 SM3 context. Otherwise, it will return directly.
 */
void mbedtls_sm3_free(mbedtls_sm3_context *ctx);

/**
 * \brief          This function starts a SM3 checksum calculation.
 * \note           This function will assert when malloc internal ctx failed.
 *
 * \param ctx      The SM3 context to use. This must be initialized.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM3_BAD_INPUT_DATA if \p ctx is \c NULL, or
 *                 \p ctx is invalid.
 * \return         \c MBEDTLS_ERR_SM3_HW_ACCEL_FAILED if there is hardware error or busy.
 */
int mbedtls_sm3_starts_ret(mbedtls_sm3_context *ctx);

/**
 * \brief           This function feeds an input buffer into an ongoing
 *                  SM3 checksum calculation.
 * \note            The maxmum supported input size for this function is 4G – 1 (0xFFFFFFFF).
 *                  If the target size >= 4G, the caller should split it into
 *                  several pieces with each size < 4G.
 *
 * \param ctx       The SM3 context. This must be initialized
 *                  and have a hash operation started.
 * \param input     The buffer holding the input data. This must
 *                  be a readable buffer of length \p ilen Bytes.
 * \param ilen      The length of the input data in Bytes.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_SM3_BAD_INPUT_DATA if \p ctx is \c NULL, or
 *                  \p ctx is invalid.
 * \return          \c MBEDTLS_ERR_SM3_HW_ACCEL_FAILED if there is hardware error or busy.
 */
int mbedtls_sm3_update_ret(mbedtls_sm3_context *ctx,
                           const unsigned char *input,
                           size_t ilen);

/**
 * \brief          This function finishes the SM3 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SM3 context. This must be initialized
 *                 and have a hash operation started.
 * \param output   The SM3 checksum result.
 *                 This must be a writable buffer of length \c 32 Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM3_BAD_INPUT_DATA if \p ctx is \c NULL, or
 *                 \p ctx is invalid.
 * \return         \c MBEDTLS_ERR_SM3_HW_ACCEL_FAILED if there is hardware error or busy.
 */
int mbedtls_sm3_finish_ret(mbedtls_sm3_context *ctx,
                           unsigned char output[32]);

/**
 * \brief          This function calculates the SM3 checksum of a buffer.
 *
 *                 The SM3 result is calculated as
 *                 output = SM3(input buffer).
 * \note           The maxmum supported input size for this function is 4G – 1 (0xFFFFFFFF).
 *                 If the target size >= 4G, the caller can use mbedtls_sm3_update_ret and
 *                 should split it into several pieces with each size < 4G.
 *
 * \param input    The buffer holding the input data. This must be
 *                 a readable buffer of length \p ilen Bytes.
 * \param ilen     The length of the input data in Bytes.
 * \param output   The SM3 checksum result.
 *                 This must be a writable buffer of length \c 32 Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM3_BAD_INPUT_DATA if \p ilen > 0xFFFFFFFF,
 *                 or \p ilen > 0 and \p input is \c NULL, or \p output is \c NULL.
 * \return         \c MBEDTLS_ERR_SM3_HW_ACCEL_FAILED if there is hardware error or busy.
 */
int mbedtls_sm3_ret(const unsigned char *input,
                    size_t ilen,
                    unsigned char output[32]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SM3_H */
