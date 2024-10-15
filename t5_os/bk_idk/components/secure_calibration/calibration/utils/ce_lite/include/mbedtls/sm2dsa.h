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

#ifndef MBEDTLS_SM2DSA_H
#define MBEDTLS_SM2DSA_H

#include "pal_common.h"
#include "ecp.h"
#include "md.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The maximal size of an SM2DSA signature in Bytes. */
#define MBEDTLS_SM2DSA_MAX_LEN ( 3 + 2 * ( 3 + 32 ) )

/**
 * \brief    The SM2DSA context structure.
 */

typedef mbedtls_ecp_keypair mbedtls_sm2dsa_context;

/**
 * \brief           This function initializes an SM2DSA context.
 * \note            This function will assert when SM2DSA ctx init failed.
 *
 * \param ctx       The SM2DSA context to initialize.
 *                  This must not be \c NULL, otherwise will return directly.
 */
void mbedtls_sm2dsa_init(mbedtls_sm2dsa_context *ctx);

/**
 * \brief           This function frees an SM2DSA context.
 *
 * \param ctx       The SM2DSA context to free.This may be \c NULL,
 *                  in which case this function does nothing. If it
 *                  is not \c NULL, it must be initialized.
 */
void mbedtls_sm2dsa_free(mbedtls_sm2dsa_context *ctx);

/**
 * \brief           This function sets an SM2DSA context from an EC key pair.
 *
 *
 * \param ctx       The SM2DSA context to setup. This must be initialized.
 * \param key       The EC key to use. This must be initialized and hold
 *                  a private-public key pair or a public key. In the former
 *                  case, the SM2DSA context may be used for signature creation
 *                  and verification after this call. In the latter case, it
 *                  may be used for signature verification.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 */
int mbedtls_sm2dsa_from_keypair(mbedtls_sm2dsa_context *ctx,
                                const mbedtls_ecp_keypair *key);

/**
 * \brief           This function computes the SM2DSA signature of a
 *                  previously-hashed message.
 *
 * \param r         The first output integer. This must be initialized.
 * \param s         The second output integer. This must be initialized.
 * \param d         The private signing key. This must be initialized.
 * \param buf       The content to be signed. This is usually the hash of
 *                  the original data to be signed. This must be a readable
 *                  buffer of length \p size Bytes. It may be \c NULL if
 *                  \p size is zero.
 * \param blen      The length of \p buf in Bytes.
 *                  Currently the supported value is 32 Bytes.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context parameter.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if private key \p d is invalid.
 * \return          \c MBEDTLS_ERR_ECP_RANDOM_FAILED if generating random failed.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sm2dsa_sign(mbedtls_mpi *r,
                        mbedtls_mpi *s,
                        const mbedtls_mpi *d,
                        const unsigned char *buf,
                        size_t blen,
                        int (*f_rng)(void *, unsigned char *, size_t),
                        void *p_rng);

/**
 * \brief           This function verifies the SM2DSA signature of a
 *                  previously-hashed message.
 *
 * \param buf       The hashed content that was signed. This must be a readable
 *                  buffer of length \p blen Bytes.
 * \param blen      The length of \p buf. Currently the supported value is 32 Bytes.
 * \param Q         The public key to use for verification. This must be
 *                  initialized and setup.
 * \param r         The first integer of the signature. This must be initialized.
 * \param s         The second integer of the signature. This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          \c CE_SUCCESS on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if public key \p Q is invalid.
 * \return          \c MBEDTLS_ERR_ECP_VERIFY_FAILED if signature is invalid.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sm2dsa_verify(const unsigned char *buf,
                          size_t blen,
                          const mbedtls_ecp_point *Q,
                          const mbedtls_mpi *r,
                          const mbedtls_mpi *s);

/**
 * \brief           This function computes the ZA value from user's ID and
 *                  pubkey.
 * \param[in] md_type   The message digest type used to compute ZA. Currently
 *                      only support MBEDTLS_MD_SM3.
 * \param[in] Q         User's public key Q.
 * \param[in] id        User's ID.
 * \param[in] idlen     User's ID size in bytes.
 * \param[out] za       Output buffer, length must be 256 bits (32 Bytes).
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sm2_compute_id_digest(mbedtls_md_type_t md_type,
                                  const mbedtls_ecp_point *Q,
                                  const char *id,
                                  size_t idlen,
                                  unsigned char *za);

/**
 * \brief           This function computes the SM2DSA signature and writes it
 *                  to a buffer, serialized as defined in <em>RFC-4492:
 *                  Elliptic Curve Cryptography (ECC) Cipher Suites for
 *                  Transport Layer Security (TLS)</em>.
 *
 * \param ctx       The SM2DSA context, must contains valid private key
 *                  \c d and group \c grp.
 * \param md_alg    Currently only support MBEDTLS_MD_SM3.
 * \param hash      The message hash to be signed. This must be a readable
 *                  buffer of length \p blen Bytes.
 * \param hlen      The length of the hash in Bytes. Currently the supported value is 32 Bytes.
 * \param sig       The buffer that holds the signature. The \p sig buffer must be
 *                  at least twice as large as the size of the SM2 curve used, plus 9.
 *                  For example, 73 Bytes if a SM2P256V1 curve is used.
 *                  A buffer length of #MBEDTLS_SM2DSA_MAX_LEN is always safe.
 * \param slen      The address at which to store the actual length of
 *                  the signature written. Must not be \c NULL.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't use a context.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if private key is invalid in \p ctx.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_RANDOM_FAILED if generating random failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sm2dsa_write_signature(mbedtls_sm2dsa_context *ctx,
                                   mbedtls_md_type_t md_alg,
                                   const unsigned char *hash,
                                   size_t hlen,
                                   unsigned char *sig,
                                   size_t *slen,
                                   int (*f_rng)(void *,
                                                unsigned char *,
                                                size_t),
                                   void *p_rng);

/**
 * \brief           This function reads and verifies an SM2DSA signature.
 *
 * \param ctx       The SM2DSA context, must contains valid public key
 *                  \c Q and group \c grp.
 * \param hash      The message hash that was signed. This must be a readable
 *                  buffer of length \p size Bytes.
 * \param hlen      The size of the hash. Currently the supported value is 32 Bytes.
 * \param sig       The signature to read and verify. This must be a readable
 *                  buffer of length \p slen Bytes.
 * \param slen      The size of \p sig.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA if the signature contains bad
 *                  asn.1 data.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if public key is invalid in \p ctx.
 * \return          \c MBEDTLS_ERR_ECP_VERIFY_FAILED if signature is invalid.
 * \return          \c MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig, but its length is less than \p slen.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_sm2dsa_read_signature(mbedtls_sm2dsa_context *ctx,
                                  const unsigned char *hash,
                                  size_t hlen,
                                  const unsigned char *sig,
                                  size_t slen);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SM2DSA_H */
