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

#ifndef MBEDTLS_ECDSA_H
#define MBEDTLS_ECDSA_H

#include "ecp.h"
#include "md.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The maximal size of an ECDSA signature in Bytes. */
#define MBEDTLS_ECDSA_MAX_LEN  ( 3 + 2 * ( 3 + MBEDTLS_ECP_MAX_BYTES ) )

/**
 * \brief           The ECDSA context structure.
 */
typedef mbedtls_ecp_keypair mbedtls_ecdsa_context;

/**
 * \brief           This function initializes an ECDSA context.
 * \note            This function will assert when internal ctx init failed.
 *
 * \param ctx       The ECDSA context to initialize.
 *                  This must not be \c NULL, otherwise this function will return directly.
 */
void mbedtls_ecdsa_init(mbedtls_ecdsa_context *ctx);

/**
 * \brief           This function frees an ECDSA context.
 *
 * \param ctx       The ECDSA context to free. This may be \c NULL,
 *                  in which case this function does nothing. If it
 *                  is not \c NULL, it must be initialized.
 */
void mbedtls_ecdsa_free(mbedtls_ecdsa_context *ctx);

/**
 * \brief           This function sets up an ECDSA context from an EC key pair.
 *
 * \note            Currently only support MBEDTLS_ECP_DP_SECP256R1 and
 *                  MBEDTLS_ECP_DP_SECP521R1.
 *
 * \param ctx       The ECDSA context to setup. This must be initialized.
 * \param key       The EC key to use. This must be initialized and hold
 *                  a private-public key pair or a public key. In the former
 *                  case, the ECDSA context may be used for signature creation
 *                  and verification after this call. In the latter case, it
 *                  may be used for signature verification.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 */
int mbedtls_ecdsa_from_keypair(mbedtls_ecdsa_context *ctx,
                               const mbedtls_ecp_keypair *key);

/**
 * \brief           This function computes the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            Currently only support MBEDTLS_ECP_DP_SECP256R1 and
 *                  MBEDTLS_ECP_DP_SECP521R1.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param r         The MPI context in which to store the first part
 *                  the signature. This must be initialized.
 * \param s         The MPI context in which to store the second part
 *                  the signature. This must be initialized.
 * \param d         The private signing key. This must be initialized.
 * \param buf       The content to be signed. This is usually the hash of
 *                  the original data to be signed. This must be a readable
 *                  buffer of length \p blen Bytes.
 * \param blen      The length of \p buf in Bytes.
                    The length must be 32, 28 or 20 bytes for SHA256,SHA224 and
                    SHA1 respectively.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context parameter.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if private key \p d is invalid.
 * \return          \c MBEDTLS_ERR_ECP_RANDOM_FAILED if generating random failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                       const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

/**
 * \brief           This function verifies the ECDSA signature of a
 *                  previously-hashed message.
 *
 * \note            Currently only support MBEDTLS_ECP_DP_SECP256R1 and
 *                  MBEDTLS_ECP_DP_SECP521R1.
 *
 * \param grp       The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param buf       The hashed content that was signed. This must be a readable
 *                  buffer of length \p blen Bytes. It may be \c NULL if
 *                  \p blen is zero.
 * \param blen      The length of \p buf in Bytes.
                    The length must be 32, 28 or 20 bytes for SHA256,SHA224 and
                    SHA1 respectively.
 * \param Q         The public key to use for verification. This must be
 *                  initialized and setup.
 * \param r         The first integer of the signature.
 *                  This must be initialized.
 * \param s         The second integer of the signature.
 *                  This must be initialized.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if public key \p Q is invalid.
 * \return          \c MBEDTLS_ERR_ECP_VERIFY_FAILED if signature is invalid.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_ecdsa_verify(mbedtls_ecp_group *grp,
                         const unsigned char *buf, size_t blen,
                         const mbedtls_ecp_point *Q, const mbedtls_mpi *r,
                         const mbedtls_mpi *s);

/**
 * \brief           This function computes the ECDSA signature and writes it
 *                  to a buffer, serialized as defined in <em>RFC-4492:
 *                  Elliptic Curve Cryptography (ECC) Cipher Suites for
 *                  Transport Layer Security (TLS)</em>.
 *
 * \note            Currently only support MBEDTLS_ECP_DP_SECP256R1 and
 *                  MBEDTLS_ECP_DP_SECP521R1.
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and private key bound to it.
 * \param md_alg    This parameter is ignored.
 * \param hash      The message hash to be signed. This must be a readable
 *                  buffer of length \p blen Bytes.
 * \param hlen      The length of the hash \p hash in Bytes.
                    The length must be 32, 28 or 20 bytes for SHA256,SHA224 and
                    SHA1 respectively.
 * \param sig       The buffer to which to write the signature. This must be a
 *                  writable buffer of length at least twice as large as the
 *                  size of the curve used, plus 9. For example, 73 Bytes if
 *                  a 256-bit curve is used. A buffer length of
 *                  #MBEDTLS_ECDSA_MAX_LEN is always safe.
 * \param slen      The address at which to store the actual length of
 *                  the signature written. Must not be \c NULL.
 * \param f_rng     The RNG function. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't use a context.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if private key is invalid in \p ctx.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_RANDOM_FAILED if generating random failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */

int mbedtls_ecdsa_write_signature(mbedtls_ecdsa_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hlen,
                                  unsigned char *sig, size_t *slen,
                                  int (*f_rng)(void *, unsigned char *, size_t),
                                  void *p_rng);

/**
 * \brief           This function reads and verifies an ECDSA signature.
 *
 * \note            Currently only support MBEDTLS_ECP_DP_SECP256R1 and
 *                  MBEDTLS_ECP_DP_SECP521R1.
 *
 * \param ctx       The ECDSA context to use. This must be initialized
 *                  and have a group and public key bound to it.
 * \param hash      The message hash that was signed. This must be a readable
 *                  buffer of length \p hlen Bytes.
 * \param hlen      The size of the hash \p hash.
                    The length must be 32, 28 or 20 bytes for SHA256,SHA224 and
                    SHA1 respectively.
 * \param sig       The signature to read and verify. This must be a readable
 *                  buffer of length \p slen Bytes.
 * \param slen      The size of \p sig in Bytes.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if public key is invalid in \p ctx.
 * \return          \c MBEDTLS_ERR_ECP_VERIFY_FAILED if signature is invalid.
 * \return          \c MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig, but its length is less than \p slen.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_ecdsa_read_signature(mbedtls_ecdsa_context *ctx,
                                 const unsigned char *hash, size_t hlen,
                                 const unsigned char *sig, size_t slen);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_ECDSA_H */