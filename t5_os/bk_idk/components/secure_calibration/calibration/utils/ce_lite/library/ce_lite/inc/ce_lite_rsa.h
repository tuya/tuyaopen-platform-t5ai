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

#ifndef __CE_LITE_RSA_H__
#define __CE_LITE_RSA_H__

#include "ce_lite_common.h"
#include "ce_lite_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                               RSA APIs                                     */
/******************************************************************************/
#define CE_RSA_SALT_LEN_ANY -1
#define CE_RSA_SALT_LEN_MAX -2

#define CE_RSA_MIN_KEY_BITS 1024
#define CE_RSA_MAX_KEY_BITS 4096

/**
 * \brief   The RSA context structure.
 *
 * \note    Direct manipulation of the members of this structure
 *          is deprecated. All manipulation should instead be done through
 *          the public interface functions.
 */
typedef struct ce_rsa_context {
    size_t len; /*!<  The size of \p N in Bytes. */

    ce_bn_t N; /*!<  The public modulus. */
    ce_bn_t E; /*!<  The public exponent. */

    ce_bn_t D; /*!<  The private exponent. */
    ce_bn_t P; /*!<  The first prime factor. */
    ce_bn_t Q; /*!<  The second prime factor. */

    ce_bn_t Vi; /*!<  The cached blinding value. */
    ce_bn_t Vf; /*!<  The cached un-blinding value. */
} ce_rsa_context_t;

/**
 * \brief           This function initializes an RSA context.
 *
 * \param[in] ctx   The RSA context to initialize. This must not be \c NULL.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_rsa_init(ce_rsa_context_t *ctx);

/**
 * \brief           This function frees the components of an RSA key.
 *
 * \param[in] ctx   The RSA context to free. May be \c NULL, in which case
 *                  this function is a no-op. If it is not \c NULL, it must
 *                  point to an initialized RSA context.
 */
void ce_rsa_free(ce_rsa_context_t *ctx);

/**
 * \brief           This function imports a set of core parameters into an
 *                  RSA context.
 *
 * \note            This function can be called multiple times for successive
 *                  imports, if the parameters are not simultaneously present.
 *
 * \note            The imported parameters are copied and need not be preserved
 *                  for the lifetime of the RSA context being set up.
 *
 * \param[in] ctx   The initialized RSA context to store the parameters in.
 * \param[in] N     The RSA modulus. This may be \c NULL.
 * \param[in] P     The first prime factor. This may be \c NULL.
 * \param[in] Q     The second prime factor. This may be \c NULL.
 * \param[in] D     The private exponent. This may be \c NULL.
 * \param[in] E     The public exponent. This may be \c NULL.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_rsa_import(ce_rsa_context_t *ctx,
                      const ce_bn_t *N,
                      const ce_bn_t *P,
                      const ce_bn_t *Q,
                      const ce_bn_t *D,
                      const ce_bn_t *E);

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
 * \param[in] ctx  The initialized RSA context to store the parameters in.
 * \param[in] N    The RSA modulus. This may be \c NULL.
 * \param[in] N_len
 *                 The Byte length of \p N; it is ignored if \p N == NULL.
 * \param[in] P    The The first prime factor. This may be \c NULL.
 * \param[in] P_len
 *                 The Byte length of \p P; it is ignored if \p P == NULL.
 * \param[in] Q    The The second prime factor. This may be \c NULL.
 * \param[in] Q_len
 *                 The Byte length of \p Q; it is ignored if \p Q == NULL.
 * \param[in] D    The private exponent. This may be \c NULL.
 * \param[in] D_len
 *                 The Byte length of \p D; it is ignored if \p D == NULL.
 * \param[in] E    The public exponent. This may be \c NULL.
 * \param[in] E_len
 *                 The Byte length of \p E; it is ignored if \p E == NULL.
 *
 * \return         \c CE_SUCCESS on success.
 * \return         \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return         \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_rsa_import_raw(ce_rsa_context_t *ctx,
                          const uint8_t *N,
                          size_t N_len,
                          const uint8_t *P,
                          size_t P_len,
                          const uint8_t *Q,
                          size_t Q_len,
                          const uint8_t *D,
                          size_t D_len,
                          const uint8_t *E,
                          size_t E_len);

/**
 * \brief          This function checks if a context contains at least an RSA
 *                 public key.
 *
 *                 If the function runs successfully, it is guaranteed that
 *                 enough information is present to perform an RSA public key
 *                 operation using ce_rsa_public().
 *
 * \param[in] ctx  The initialized RSA context to check.
 *
 * \return         \c CE_SUCCESS on success.
 * \return         \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return         \c CE_ERROR_INVAL_KEY if public key in \p ctx is invalid.
 */
int32_t ce_rsa_check_pubkey(const ce_rsa_context_t *ctx);

/**
 * \brief           This function checks if a context contains an RSA private
 *                  key and perform basic consistency checks.
 *
 * \note            The consistency checks performed by this function not only
 *                  ensure that ce_rsa_private() can be called successfully
 *                  on the given context, but that the various parameters are
 *                  mutually consistent with high probability, in the sense that
 *                  ce_rsa_public() and ce_rsa_private() are inverses.
 *
 * \param[in] ctx   The initialized RSA context to check.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key in \p ctx is invalid.
 */
int32_t ce_rsa_check_privkey(const ce_rsa_context_t *ctx);

/**
 * \brief           This function checks a public-private RSA key pair.
 *
 *                  It checks each of the contexts, and makes sure they match.
 *
 * \param[in] pub   The initialized RSA context holding the public key.
 * \param[in] prv   The initialized RSA context holding the private key.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key in \p pub is invalid, or
 *                  private key \p prv is invalid, or the public key doesn't
 *                  match with the private key.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_rsa_check_pub_priv(const ce_rsa_context_t *pub,
                              const ce_rsa_context_t *prv);

/**
 * \brief           This function performs an RSA public key operation.
 *
 * \param[in] ctx   The initialized RSA context to use.
 * \param[in] input The input buffer. This must be a readable buffer
 *                  of length key Bytes. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 * \param[in] output
 *                  The output buffer. This must be a writable buffer
 *                  of length key Bytes. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 *
 * \note            This function does not handle message padding.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key in \p ctx is invalid.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_rsa_public(ce_rsa_context_t *ctx,
                      const uint8_t *input,
                      uint8_t *output);

/**
 * \brief           This function performs an RSA private key operation.
 *
 * \note            This function does not handle message padding.
 *
 * \param[in] ctx   The initialized RSA context to use.
 * \param[in] f_rng The RNG function, used for blinding. It is discouraged
 *                  and deprecated to pass \c NULL here, in which case
 *                  blinding will be omitted.
 * \param[in] p_rng The RNG context to pass to \p f_rng. This may be \c NULL
 *                  if \p f_rng is \c NULL or if \p f_rng doesn't need a
 *                  context.
 * \param[in] input The input buffer. This must be a readable buffer
 *                  of length key Bytes. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 * \param[out] output
 *                  The output buffer. This must be a writable buffer
 *                  of length key Bytes. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 *
 * \note            This function does not handle message padding.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if private key in \p ctx is invalid.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_rsa_private(ce_rsa_context_t *ctx,
                       int32_t (*f_rng)(void *, uint8_t *, size_t),
                       void *p_rng,
                       uint8_t *input,
                       uint8_t *output);

/**
 * \brief           This function performs a PKCS#1 v1.5 verification
 *                  operation (RSASSA-PKCS1-v1_5-VERIFY).
 *
 * \param[in] ctx   The initialized RSA public key context to use.
 * \param[in] md_alg
 *                  The message-digest algorithm used to hash the original
 *                  data. Use #CE_HASH_ALG_INVALID for verifying raw data.
 * \param[in] hashlen
 *                  The length of the message digest. Ths is only used if
 *                  \p md_alg is #CE_HASH_ALG_INVALID.
 * \param[in] hash  The buffer holding the message digest or raw data.
 *                  If \p md_alg is #CE_HASH_ALG_INVALID, This buffer holds the
 *                  raw data to verify, and it must be a readable buffer of
 *                  length \p hashlen Bytes.
 *                  If \p md_alg is not #CE_HASH_ALG_INVALID, this buffer holds
 *                  the message digest to verify, and it must be a readable
 *                  buffer of length the size of the hash corresponding to
 *                  \p md_alg.
 * \param[in] sig   The buffer to hold the signature. This must be a writable
 *                  buffer of rsa modulus size. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key in \p ctx is invalid.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 * \return          \c CE_ERROR_VERIFY_SIG if signature is invalid.
 */
int32_t ce_rsa_rsassa_pkcs1_v15_verify(ce_rsa_context_t *ctx,
                                       ce_algo_t md_alg,
                                       size_t hashlen,
                                       const uint8_t *hash,
                                       const uint8_t *sig);
/**
 * \brief           This function performs a PKCS#1 v2.1 PSS verification
 *                  operation (RSASSA-PSS-VERIFY).
 *
 *                  The hash function for the MGF mask generating function
 *                  is that specified in \p mgf1_hash_id.
 *
 * \note            The \p sig buffer must be as large as the size
 *                  of \p N. For example, 128 Bytes if RSA-1024 is used.
 *
 * \param[in] ctx   The initialized RSA public key context to use.
 * \param[in] hashlen
 *                  The length of the message digest.
 * \param[in] hash  The buffer holding the message digest or raw data.
 *                  This buffer holds the raw data to verify, and it must be a
 *                  readable buffer of length \p hashlen Bytes.
 * \param[in] mgf1_hash_id
 *                  The message digest used for mask generation.
 * \param[in] expected_salt_len
 *                  The length of the salt used in padding.
 *                  #CE_RSA_SALT_LEN_ANY to accept any salt length.
 *                  #CE_RSA_SALT_LEN_MAX to accept maximum possible salt size,
 *                  up to the length of the payload hash or raw data. This
 *                  choice of salt size complies with FIPS 186-4 §5.5 (e) and
 *                  RFC 8017 (PKCS#1 v2.2) §9.1.1 step 3.
 * \param[in] sig   The buffer holding the signature. This must be a readable
 *                  buffer of length rsa modulus size. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key in \p ctx is invalid.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 * \return          \c CE_ERROR_VERIFY_SIG if signature is invalid.
 */
int32_t ce_rsa_rsassa_pss_verify(ce_rsa_context_t *ctx,
                                 const uint8_t *hash,
                                 size_t hashlen,
                                 ce_algo_t mgf1_hash_id,
                                 int32_t expected_salt_len,
                                 const uint8_t *sig);

/**
 * \brief           This function performs a PKCS#1 v1.5 signature
 *                  operation (RSASSA-PKCS1-v1_5-SIGN).
 *
 * \param[in] ctx   The initialized RSA context to use.
 * \param[in] f_rng The RNG function, used for blinding. It is discouraged
 *                  and deprecated to pass \c NULL here, in which case
 *                  blinding will be omitted.
 * \param[in] p_rng The RNG context to pass to \p f_rng. This may be \c NULL
 *                  if \p f_rng is \c NULL or if \p f_rng doesn't need a
 *                  context.
 * \param[in] md_alg
 *                  The message-digest algorithm used to hash the original
 *                  data. Use #CE_HASH_ALG_INVALID for signing raw data.
 * \param[in] hashlen
 *                  The length of the message digest. Ths is only used if
 *                  \p md_alg is #CE_HASH_ALG_INVALID.
 * \param[in] hash  The buffer holding the message digest or raw data.
 *                  If \p md_alg is #CE_HASH_ALG_INVALID, This buffer holds the
 *                  raw data to sign, and it must be a readable buffer of
 *                  length \p hashlen Bytes.
 *                  If \p md_alg is not #CE_HASH_ALG_INVALID, this buffer holds
 *                  the message digest to sign, and it must be a readable
 *                  buffer of length the size of the hash corresponding to
 *                  \p md_alg.
 * \param[out] sig  The buffer to hold the signature. This must be a writable
 *                  buffer of rsa modulus size. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if private key in \p ctx is invalid.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t
ce_rsa_rsassa_pkcs1_v15_sign(ce_rsa_context_t *ctx,
                             int32_t (*f_rng)(void *, uint8_t *, size_t),
                             void *p_rng,
                             ce_algo_t md_alg,
                             size_t hashlen,
                             const uint8_t *hash,
                             uint8_t *sig);

/**
 * \brief           This function performs a PKCS#1 v2.1 PSS signature
 *                  operation (RSASSA-PSS-SIGN).
 *
 * \note            This function enforces a minimum salt size which is the hash
 *                  size minus 2 bytes. If this minimum size is too large given
 *                  the key size the salt size, plus the hash size, plus 2 bytes
 *                  must be no more than the key size in bytes), this function
 *                  returns \c CE_ERROR_BAD_INPUT_DATA.
 *
 * \param[in] ctx   The initialized RSA context to use.
 * \param[in] f_rng The RNG function. It must not be \c NULL.
 * \param[in] p_rng The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng doesn't need a context argument.
 * \param[in] hashlen
 *                  The length of the message digest.
 * \param[in] hash  The buffer holding the message digest or raw data.
 *                  This buffer holds the raw data to sign, and it must be a
 *                  readable buffer of length \p hashlen Bytes.
 * \param[in] mgf1_hash_id
 *                  The message digest used for mask generation.
 * \param[in] salt_len
 *                  The length of the salt used in padding. it can have the
 *                  following types of input:
 *                  #CE_RSA_SALT_LEN_MAX to use maximum possible salt size,
 *                  up to the length of the payload hash or raw data. This
 *                  choice of salt size complies with FIPS 186-4 §5.5 (e) and
 *                  RFC 8017 (PKCS#1 v2.2) §9.1.1 step 3.
 * \param[out] sig  The buffer to hold the signature. This must be a writable
 *                  buffer of rsa modulus size. For example, \c 256 Bytes
 *                  for an 2048-bit RSA modulus.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if private key in \p ctx is invalid.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_rsa_rsassa_pss_sign(ce_rsa_context_t *ctx,
                               int32_t (*f_rng)(void *, uint8_t *, size_t),
                               void *p_rng,
                               const uint8_t *hash,
                               size_t hashlen,
                               ce_algo_t mgf1_hash_id,
                               int32_t salt_len,
                               uint8_t *sig);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_RSA_H__ */
