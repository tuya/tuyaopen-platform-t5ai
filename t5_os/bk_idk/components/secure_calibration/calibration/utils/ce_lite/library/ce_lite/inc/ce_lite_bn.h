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

#ifndef __CE_LITE_BN_H__
#define __CE_LITE_BN_H__

#include "ce_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                            Bignumebr APIs                                  */
/******************************************************************************/
struct ce_aca_bn_t;
/**
 * The bignumber context.
 */
typedef struct ce_bn_t {
    struct ce_aca_bn_t *ctx;
} ce_bn_t;

/**
 * \brief This function initializes one bignumber.
 *
 * This makes the bignumber ready to be set or reed, but NOT defines a
 * value for the bignumber.
 *
 * \param[in] bn    The bignumber context.
 *                  This must not be \c NULL.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_bn_init(ce_bn_t *bn);

/**
 * \brief This function frees the components of an bignumber and sets the \c ctx
 *        in \c ce_bn_t to NULL.
 *
 * \param[in] bn    The bignumber to be cleared. The \p bn or the \c ctx in
 *                  bn->ctx may be \c NULL, in which case this function is
 *                  a no-op.
 */
void ce_bn_free(ce_bn_t *bn);

/**
 * \brief Import an BN from unsigned big endian binary data.
 *
 * \param[in] bn    The destination BN. This must point to an initialized BN.
 * \param[in] data  The input buffer. This must be a readable buffer of length
 *                  \p size Bytes.
 * \param[in] size  The length of the input buffer \p data in Bytes.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_bn_import(ce_bn_t *bn, const uint8_t *buf, size_t size);

/**
 * \brief Export an BN into unsigned big endian binary data of fixed size.
 *
 * \param[in] bn    The source BN. This must point to an initialized BN.
 * \param[out] buf  The output buffer. This must be a writable buffer of
 *                  length \p size Bytes.
 * \param[in] size  The output buffer size pointer,
 *                  updated to the BN byte length.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_SHORT_BUFFER on buffer size too short to save
 *                  BN data.
 */
int32_t ce_bn_export(const ce_bn_t *bn, uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_BN_H__ */
