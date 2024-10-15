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

#ifndef __CE_LITE_ECDSA_H__
#define __CE_LITE_ECDSA_H__

#include "ce_lite_common.h"
#include "ce_lite_bn.h"
#include "ce_lite_ecp.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                               ECDSA APIs */
/******************************************************************************/

/** The maximal size of an ECDSA signature in Bytes. */
#define CE_ECDSA_MAX_LEN (3 + 2 * (3 + ((CE_ECP_MAX_BITS + 7) / 8)))

typedef ce_ecp_keypair_t ce_ecdsa_context_t;

/**
 * \brief           This function verifies the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.4, step 3.
 *
 * \see             ce_ecp.h
 *
 * \param[in] grp   The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[in] buf   The hashed content that was signed. This must be a readable
 *                  buffer of length \p size Bytes. It may be \c NULL if
 *                  \p size is zero.
 * \param[in] size  The length of \p buf in Bytes.
 * \param[in] Q     The public key to use for verification. This must be
 *                  initialized and setup.
 * \param[in] r     The first integer of the signature.
 *                  This must be initialized.
 * \param[in] s     The second integer of the signature.
 *                  This must be initialized.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key \p Q is invalid.
 * \return          \c CE_ERROR_VERIFY_SIG if signature is invalid.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecdsa_verify(const ce_ecp_group_t *grp,
                        const uint8_t *buf,
                        size_t size,
                        const ce_ecp_point_t *Q,
                        const ce_bn_t *r,
                        const ce_bn_t *s);
/**
 * \brief           This function computes the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated
 *                  as defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.3, step 5.
 *
 * \see             ce_ecp.h
 *
 * \param[in] grp   The context for the elliptic curve to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[in] d     The private signing key. This must be initialized.
 * \param[in] buf   The content to be signed. This is usually the hash of
 *                  the original data to be signed. This must be a readable
 *                  buffer of length \p size Bytes. It may be \c NULL if
 *                  \p size is zero.
 * \param[in] size  The length of \p buf in Bytes.
 * \param[out] r    The bignumber context in which to store the first part
 *                  the signature. This must be initialized.
 * \param[out] s    The bignumber context in which to store the second part
 *                  the signature. This must be initialized.
 * \param[in] f_rng The RNG function. This must not be \c NULL.
 * \param[in] p_rng The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context parameter.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key \p Q is invalid.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecdsa_sign(const ce_ecp_group_t *grp,
                      const ce_bn_t *d,
                      const uint8_t *buf,
                      size_t size,
                      ce_bn_t *r,
                      ce_bn_t *s,
                      int32_t (*f_rng)(void *, uint8_t *, size_t),
                      void *p_rng);

/**
 * \brief           This function initializes an ECDSA context.
 *
 * \param[in] ctx   The ECDSA context to initialize.
 *                  This must not be \c NULL.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_ecdsa_init(ce_ecdsa_context_t *ctx);

/**
 * \brief           This function frees an ECDSA context.
 *
 * \param[in] ctx   The ECDSA context to free. This may be \c NULL,
 *                  in which case this function does nothing. If it
 *                  is not \c NULL, it must be initialized.
 */
void ce_ecdsa_free(ce_ecdsa_context_t *ctx);

/**
 * \brief           This function sets up an ECDSA context from an EC key pair.
 *
 * \see             ce_lite_ecp.h
 *
 * \param[out] ctx  The ECDSA context to setup. This must be initialized.
 * \param[in] key   The EC key to use. This must be initialized and hold
 *                  a private-public key pair or a public key. In the former
 *                  case, the ECDSA context may be used for signature creation
 *                  and verification after this call. In the latter case, it
 *                  may be used for signature verification.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_ecdsa_from_keypair(ce_ecdsa_context_t *ctx,
                              const ce_ecp_keypair_t *key);

/**
 * \brief           This function reads and verifies an ECDSA signature.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.4, step 3.
 *
 * \see             ce_lite_ecp.h
 *
 * \param[in] ctx   The ECDSA context to use. This must be initialized
 *                  and have a group and public key bound to it.
 * \param[in] Q     The public key to use for verification. This must be
 *                  initialized and setup.
 * \param[in] hash  The message hash that was signed. This must be a readable
 *                  buffer of length \p size Bytes.
 * \param[in] hlen  The size of the hash \p hash.
 * \param[in] sig   The signature to read and verify. This must be a readable
 *                  buffer of length \p slen Bytes.
 * \param[in] slen  The size of \p sig in Bytes.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key is invalid in \p ctx.
 * \return          \c CE_ERROR_VERIFY_SIG if signature is invalid.
 * \return          \c CE_ERROR_SIG_LENGTH if there is a valid
 *                  signature in \p sig, but its length is less than \p slen.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecdsa_read_signature(const ce_ecdsa_context_t *ctx,
                                const uint8_t *hash,
                                size_t hlen,
                                const uint8_t *sig,
                                size_t slen);

/**
 * \brief           This function computes the ECDSA signature and writes it
 *                  to a buffer, serialized as defined in <em>RFC-4492:
 *                  Elliptic Curve Cryptography (ECC) Cipher Suites for
 *                  Transport Layer Security (TLS)</em>.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the group order, then the hash is truncated as
 *                  defined in <em>Standards for Efficient Cryptography Group
 *                  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *                  4.1.3, step 5.
 *
 * \see             ce_lite_ecp.h
 *
 * \param[in] ctx   The ECDSA context to use. This must be initialized
 *                  and have a group and private key bound to it.
 * \param[in] hash  The message hash to be signed. This must be a readable
 *                  buffer of length \p blen Bytes.
 * \param[in] hlen  The length of the hash \p hash in Bytes.
 * \param[out] sig  The buffer to which to write the signature. This must be a
 *                  writable buffer of length at least twice as large as the
 *                  size of the curve used, plus 9. For example, 73 Bytes if
 *                  a 256-bit curve is used. A buffer length of
 *                  #CE_ECDSA_MAX_LEN is always safe.
 * \param[inout] slen
 *                  The address at which to store the actual length of
 *                  the signature written. Must not be \c NULL.
 * \param[in] f_rng The RNG function. This must not be \c NULL.
 * \param[in] p_rng The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't use a context.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if private key is invalid in \p ctx.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecdsa_write_signature(const ce_ecdsa_context_t *ctx,
                                 const uint8_t *hash,
                                 size_t hlen,
                                 uint8_t *sig,
                                 size_t *slen,
                                 int32_t (*f_rng)(void *, uint8_t *, size_t),
                                 void *p_rng);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_ECDSA_H__ */
