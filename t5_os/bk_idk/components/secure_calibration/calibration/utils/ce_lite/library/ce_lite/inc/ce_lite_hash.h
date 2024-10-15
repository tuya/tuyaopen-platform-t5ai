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

#ifndef __CE_LITE_HASH_H__
#define __CE_LITE_HASH_H__

#include "ce_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                            HASH APIs                                       */
/******************************************************************************/

struct ce_hash_t;
/**
 * Trust engine digest context structure
 */
typedef struct ce_dgst_ctx_t {
    struct ce_hash_t *ctx;
} ce_dgst_ctx_t;

/**
 * \brief               This function calculates the message-digest of a memory
 *                      buffer, with respect to a configurable message-digest
 *                      algorithm in a single call.
 *
 * \param[in] algo      The message-digest algorithm to use.
 * \param[in] in        The buffer holding the data.
 * \param[in] len       The byte length of the input data.
 * \param[out] hash     The generated message-digest result.
 * \return              \c CE_SUCCESS on success.
 * \return              \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return              \c CE_ERROR_GENERIC if there is hardware error.
 * \return              \c CE_ERROR_BUSY if hardware is busy.
 */
int32_t ce_dgst(ce_algo_t algo, const uint8_t *in, size_t len, uint8_t *hash);

/**
 * \brief           This function initializes the digest context \p ctx.
 *
 * \param[out] ctx  The digest context.
 * \param[in] algo  The digest algorithm identifier.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_dgst_init(ce_dgst_ctx_t *ctx, ce_algo_t algo);

/**
 * \brief           This function withdraws the digest context \p ctx.
 *
 * \param[in] ctx   The digest context. The \p ctx or the \c ctx in
 *                  ctx->ctx may be \c NULL, in which case this function is
 *                  a no-op.
 */
void ce_dgst_free(ce_dgst_ctx_t *ctx);

/**
 * \brief           This function starts a digest operation.
 *
 * \param[in] ctx   The digest context.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 * \return          \c CE_ERROR_BUSY if hardware is busy.
 */
int32_t ce_dgst_start(ce_dgst_ctx_t *ctx);

/**
 * \brief           This function feeds an input buffer into an ongoing digest
 *                  operation.
 *
 *                  It is called between ce_dgst_start() or ce_dgst_reset(),
 *                  and ce_dgst_finish(). Can be called repeatedly.
 *
 * \param[in] ctx   The digest context.
 * \param[in] in    The buffer holding the input data.
 * \param[in] len   The length of the input data.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_dgst_update(ce_dgst_ctx_t *ctx, const uint8_t *in, size_t len);

/**
 * \brief           This function finishes the digest operation and writes
 *                  the result to the \p hash buffer.
 *
 *                  The \p hash buffer must be of enough length to load the
 *                  result.
 *
 *                  It is called after one or several ce_dgst_update(), or after
 *                  ce_dgst_start(). Afterwards, you must not using the context
 *                  for any operation.
 * \note            If \p dgst pointer is NULL, this function will directly
 *                  clear the internal hash context and return CE_ERROR_NO_DATA.
 *
 * \note            Now the HASH context is always been cleared if not NULL.
 *
 * \param[in] ctx   The digest context.
 * \param[out] hash The buffer holding the hash data.
 *                  If NULL, will clear the hash context.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 * \return          \c CE_ERROR_NO_DATA when dgst is NULL for clearing the
 *                  hash context.
 */
int32_t ce_dgst_finish(ce_dgst_ctx_t *ctx, uint8_t *hash);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_HASH_H__ */
