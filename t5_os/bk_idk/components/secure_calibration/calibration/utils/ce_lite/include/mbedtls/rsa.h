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

#ifndef MBEDTLS_RSA_H
#define MBEDTLS_RSA_H

#include "pal_common.h"
#include "bignum.h"
#include "md.h"

/*
 * RSA Error codes
 */
#define MBEDTLS_ERR_RSA_BAD_INPUT_DATA                    -0x4080  /**< Bad input parameters to function. */
#define MBEDTLS_ERR_RSA_INVALID_PADDING                   -0x4100  /**< Input data contains invalid padding and is rejected. */
#define MBEDTLS_ERR_RSA_VERIFY_FAILED                     -0x4380  /**< The PKCS#1 verification failed. */
#define MBEDTLS_ERR_RSA_KEY_CHECK_FAILED                  -0x4200  /**< Key failed to pass the validity check of the library. */
#define MBEDTLS_ERR_RSA_HW_ACCEL_FAILED                   -0x4580  /**< RSA hardware accelerator failed. */
#define MBEDTLS_ERR_RSA_RNG_FAILED                        -0x4480  /**< The random generator failed to generate non-zeros. */

/*
 * RSA constants
 */
#define MBEDTLS_RSA_PUBLIC      0 /**< Request private key operation. */
#define MBEDTLS_RSA_PRIVATE     1 /**< Request public key operation. */

#define MBEDTLS_RSA_PKCS_V15    0 /**< Use PKCS#1 v1.5 encoding. */
#define MBEDTLS_RSA_PKCS_V21    1 /**< Use PKCS#1 v2.1 encoding. */

#define MBEDTLS_RSA_SALT_LEN_ANY    -1
#define MBEDTLS_RSA_SALT_LEN_MAX    -2

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief   The RSA context structure.
 *
 * \note    Direct manipulation of the members of this structure
 *          is deprecated. All manipulation should instead be done through
 *          the public interface functions.
 */
typedef struct mbedtls_rsa_context {
    size_t len;                 /*!<  The size of \p N in Bytes. */

    mbedtls_mpi N;              /*!<  The public modulus. */
    mbedtls_mpi E;              /*!<  The public exponent. */

    mbedtls_mpi D;              /*!<  The private exponent. */
    mbedtls_mpi P;              /*!<  The first prime factor. */
    mbedtls_mpi Q;              /*!<  The second prime factor. */

    mbedtls_mpi Vi;             /*!<  The cached blinding value. */
    mbedtls_mpi Vf;             /*!<  The cached un-blinding value. */

    int padding;                /*!< Selects padding mode:
                                     #MBEDTLS_RSA_PKCS_V15 for 1.5 padding and
                                     #MBEDTLS_RSA_PKCS_V21 for PSS. */
    int hash_id;                /*!< Hash identifier of mbedtls_md_type_t type,
                                     as specified in md.h for use in the MGF
                                     mask generating function used in the
                                     EMSA-PSS encodings. */
} mbedtls_rsa_context;

/**
 * \brief          This function initializes an RSA context.
 *
 * \note           Set padding to #MBEDTLS_RSA_PKCS_V21 for the RSASSA-PSS
 *                 signature scheme.
 *
 * \note           This function will assert when internal mpi init failed or hash_id is not
 *                 #MBEDTLS_MD_SHA256, #MBEDTLS_MD_SHA224, or #MBEDTLS_MD_SHA1.
 *
 * \note           The \p hash_id parameter is ignored when using
 *                 #MBEDTLS_RSA_PKCS_V15 padding.
 *
 * \note           The choice of padding mode for publickey verifying signature.
 *
 * \note           The hash selected in \p hash_id is used for verifying
 *                 signatures, can be overriden for verifying them.
 *                 If set to #MBEDTLS_MD_NONE, it is always overriden.
 *
 * \param ctx      The RSA context to initialize. This must not be \c NULL.
 * \param padding  The padding mode to use. This must be either
 *                 #MBEDTLS_RSA_PKCS_V15 or #MBEDTLS_RSA_PKCS_V21.
 * \param hash_id  The \p hash_id must be either #MBEDTLS_MD_SHA256,
 *                 or #MBEDTLS_MD_SHA224, or #MBEDTLS_MD_SHA1.
 *                 if \p padding is #MBEDTLS_RSA_PKCS_V21. It is unused
 *                 otherwise.
 */
void mbedtls_rsa_init(mbedtls_rsa_context *ctx,
                      int padding,
                      int hash_id);

/**
 * \brief          This function frees the components of an RSA key.
 *
 * \param ctx      The RSA context to free. May be \c NULL, in which case
 *                 this function is a no-op. If it is not \c NULL, it must
 *                 point to an initialized RSA context.
 */
void mbedtls_rsa_free(mbedtls_rsa_context *ctx);

/**
 * \brief          This function imports a set of core parameters into an
 *                 RSA context.
 *
 * \note           This function can be called multiple times for successive
 *                 imports, if the parameters are not simultaneously present.
 *
 * \note           The imported parameters are copied and need not be preserved
 *                 for the lifetime of the RSA context being set up.
 *
 * \param ctx      The initialized RSA context to store the parameters in.
 * \param N        The RSA modulus. This may be \c NULL.
 * \param P        The first prime factor of \p N. This may be \c NULL.
 * \param Q        The second prime factor of \p N. This may be \c NULL.
 * \param D        The private exponent. This may be \c NULL.
 * \param E        The public exponent. This may be \c NULL.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_MPI_ALLOC_FAILED on memory allocation failed.
 */
int mbedtls_rsa_import(mbedtls_rsa_context *ctx,
                       const mbedtls_mpi *N,
                       const mbedtls_mpi *P, const mbedtls_mpi *Q,
                       const mbedtls_mpi *D, const mbedtls_mpi *E);

/**
 * \brief          This function imports core RSA parameters, in raw big-endian
 *                 binary format, into an RSA context.
 *
 * \note           This function can be called multiple times for successive
 *                 imports, if the parameters are not simultaneously present.
 *
 * \note           The imported parameters are copied and need not be preserved
 *                 for the lifetime of the RSA context being set up.
 *
 * \param ctx      The initialized RSA context to store the parameters in.
 * \param N        The RSA modulus. This may be \c NULL.
 * \param N_len    The Byte length of \p N; it is ignored if \p N == NULL.
 * \param P        The first prime factor of \p N. This may be \c NULL.
 * \param P_len    The Byte length of \p P; it ns ignored if \p P == NULL.
 * \param Q        The second prime factor of \p N. This may be \c NULL.
 * \param Q_len    The Byte length of \p Q; it is ignored if \p Q == NULL.
 * \param D        The private exponent. This may be \c NULL.
 * \param D_len    The Byte length of \p D; it is ignored if \p D == NULL.
 * \param E        The public exponent. This may be \c NULL.
 * \param E_len    The Byte length of \p E; it is ignored if \p E == NULL.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_MPI_ALLOC_FAILED on memory allocation failed.
 */
int mbedtls_rsa_import_raw(mbedtls_rsa_context *ctx,
                           unsigned char const *N, size_t N_len,
                           unsigned char const *P, size_t P_len,
                           unsigned char const *Q, size_t Q_len,
                           unsigned char const *D, size_t D_len,
                           unsigned char const *E, size_t E_len);

/**
 * \brief          This function retrieves the length of RSA modulus in Bytes.
 *
 * \param ctx      The initialized RSA context.
 *
 * \return         The length of the RSA modulus in Bytes.
 *
 */
size_t mbedtls_rsa_get_len(const mbedtls_rsa_context *ctx);

/**
 * \brief          This function performs a private RSA operation to sign
 *                 a message digest using PKCS#1.
 *
 *                 It is the generic wrapper for performing a PKCS#1
 *                 signature using the \p mode from the context.
 *
 * \note           The \p sig buffer must be as large as the size
 *                 of \p ctx->N. For example, 128 Bytes if RSA-1024 is used.
 *
 * \param ctx      The initialized RSA context to use.
 * \param f_rng    The RNG function, used for blinding. It is discouraged
 *                 and deprecated to pass \c NULL here, in which case
 *                 blinding will be omitted.
 * \param p_rng    The RNG context to pass to \p f_rng. This may be \c NULL
 *                 if \p f_rng is \c NULL or if \p f_rng doesn't need a
 *                 context.
 * \param mode     The mode of operation. This must be #MBEDTLS_RSA_PRIVATE.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Currently support #MBEDTLS_MD_SHA256, #MBEDTLS_MD_SHA224 or
 *                 #MBEDTLS_MD_SHA1.
 * \param hashlen  This parameter is ignored.
 * \param hash     The buffer holding the message digest.
 *                 It must be a readable buffer of length,
 *                 the size of the hash corresponding to \p md_alg.
 * \param sig      The buffer to hold the signature. This must be a writable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if private key in \p ctx is invalid.
 * \return         \c MBEDTLS_ERR_RSA_RNG_FAILED if generating random failed.
 * \return         \c MBEDTLS_ERR_MPI_ALLOC_FAILED on memory allocation failed.
 * \return         \c MBEDTLS_ERR_RSA_HW_ACCEL_FAILED if there is hardware error.
 */
int mbedtls_rsa_pkcs1_sign(mbedtls_rsa_context *ctx,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng,
                           int mode,
                           mbedtls_md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           unsigned char *sig);

/**
 * \brief          This function performs a public RSA operation and checks
 *                 the message digest.
 *
 *                 This is the generic wrapper for performing a PKCS#1
 *                 verification using the mode from the context.
 *
 * \note           For PKCS#1 v2.1 encoding:
 *                 The hash function for the MGF mask generating function is that
 *                 specified in the RSA context.
 *
 * \note           Currently the \p mode argument MUST be #MBEDTLS_RSA_PUBLIC.
 *
 * \param ctx      The initialized RSA public key context to use.
 * \param f_rng    This parameter is ignored.
 * \param p_rng    This parameter is ignored.
 * \param mode     The mode of operation. Currently MUST be #MBEDTLS_RSA_PUBLIC.
 * \param md_alg   The message-digest algorithm used to hash the original data.
 *                 Currently support #MBEDTLS_MD_SHA256, #MBEDTLS_MD_SHA224 or
 *                 #MBEDTLS_MD_SHA1.
 * \param hashlen  This parameter is ignored.
 * \param hash     The buffer holding the message digest or raw data.
 *                 It must be a readable buffer with a length of the corresponding \p md_alg.
 *
 * \param sig      The buffer holding the signature. This must be a readable
 *                 buffer of length \c ctx->len Bytes. For example, \c 256 Bytes
 *                 for an 2048-bit RSA modulus.
 *
 * \return         \c 0 if the verify operation was successful.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if public key in \p ctx is invalid.
 * \return         \c MBEDTLS_ERR_MPI_ALLOC_FAILED on memory allocation failed.
 * \return         \c MBEDTLS_ERR_RSA_HW_ACCEL_FAILED if there is hardware error.
 * \return         \c MBEDTLS_ERR_RSA_VERIFY_FAILED if signature is invalid.
 */
int mbedtls_rsa_pkcs1_verify(mbedtls_rsa_context *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            mbedtls_md_type_t md_alg,
                            unsigned int hashlen,
                            const unsigned char *hash,
                            const unsigned char *sig);

/**
 * \brief          This function checks if a context contains at least an RSA
 *                 public key.
 *
 *                 If the function runs successfully, it is guaranteed that
 *                 enough information is present to perform an RSA public key
 *                 operation.
 *
 * \param ctx      The initialized RSA context to check.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if public key in \p ctx is invalid.
 */
int mbedtls_rsa_check_pubkey(const mbedtls_rsa_context *ctx);

/**
 * \brief      This function checks if a context contains an RSA private key
 *             and perform basic consistency checks.
 *
 * \param ctx  The initialized RSA context to check.
 *
 * \return     \c 0 on success.
 * \return     \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return     \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if private key in \p ctx is invalid.
 */
int mbedtls_rsa_check_privkey(const mbedtls_rsa_context *ctx);

/**
 * \brief          This function checks a public-private RSA key pair.
 *
 *                 It checks each of the contexts, and makes sure they match.
 *
 * \param pub      The initialized RSA context holding the public key.
 * \param prv      The initialized RSA context holding the private key.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_RSA_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if public key in \p pub is invalid, or
 *                 private key \p prv is invalid, or the public key doesn't
 *                 match with the private key.
 * \return         \c MBEDTLS_ERR_MPI_HW_FAILED if there is hardware error.
 */
int mbedtls_rsa_check_pub_priv(const mbedtls_rsa_context *pub,
                               const mbedtls_rsa_context *prv);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_RSA_H */
