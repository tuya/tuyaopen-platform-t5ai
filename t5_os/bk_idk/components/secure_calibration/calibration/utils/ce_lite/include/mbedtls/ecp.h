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

#ifndef MBEDTLS_ECP_H
#define MBEDTLS_ECP_H

#include "pal_common.h"
#include "bignum.h"

#define MBEDTLS_ECP_MAX_BITS     521   /**< The maximum size of groups, in bits. */

/*
 * ECP error codes
 */
#define MBEDTLS_ERR_ECP_BAD_INPUT_DATA                    -0x4F80  /**< Bad input parameters to function. */
#define MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL                  -0x4F00  /**< The buffer is too small to write to. */
#define MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE               -0x4E80  /**< The requested feature is not available, for example, the requested curve is not supported. */
#define MBEDTLS_ERR_ECP_VERIFY_FAILED                     -0x4E00  /**< The signature is not valid. */
#define MBEDTLS_ERR_ECP_RANDOM_FAILED                     -0x4D00  /**< Generation of random value, such as ephemeral key, failed. */
#define MBEDTLS_ERR_ECP_ALLOC_FAILED                      -0x4D80  /**< Memory allocation failed. */
#define MBEDTLS_ERR_ECP_INVALID_KEY                       -0x4C80  /**< Invalid private or public key. */
#define MBEDTLS_ERR_ECP_HW_FAILED                         -0x4A80  /**< There is HW error in calling CE driver. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Domain-parameter identifiers: curve, subgroup, and generator.
 */
typedef enum {
    MBEDTLS_ECP_DP_NONE = 0,       /*!< Curve not defined. */
    MBEDTLS_ECP_DP_SECP256R1,      /*!< Domain parameters for the 256-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SECP521R1,      /*!< Domain parameters for the 521-bit curve defined by FIPS 186-4 and SEC1. */
    MBEDTLS_ECP_DP_SM2P256V1,      /*!< Domain parameters for SM2 curve. */
} mbedtls_ecp_group_id;

/**
 * \brief           The ECP point structure, in Jacobian coordinates.
 */
typedef struct mbedtls_ecp_point {
    mbedtls_mpi X;          /*!< The X coordinate of the ECP point. */
    mbedtls_mpi Y;          /*!< The Y coordinate of the ECP point. */
    mbedtls_mpi Z;          /*!< The Z coordinate of the ECP point. */
} mbedtls_ecp_point;

/**
 * \brief           The ECP group structure.
 * \note            The definition of this structure shall be in line with ECP lite driver always.
 */
typedef struct mbedtls_ecp_group {
    mbedtls_ecp_group_id id;    /*!< An internal group identifier. */
    mbedtls_mpi P;              /*!< The prime modulus of the base field. */
    mbedtls_mpi A;              /*!< For Short Weierstrass: \p A in the equation. */
    mbedtls_mpi B;              /*!< For Short Weierstrass: \p B in the equation. */
    mbedtls_ecp_point G;        /*!< The generator of the subgroup used. */
    mbedtls_mpi N;              /*!< The order of \p G. */
    size_t pbits;               /*!< The number of bits in \p P.*/
    size_t nbits;               /*!< For Short Weierstrass: The number of bits in \p N. */
} mbedtls_ecp_group;

/**
 * \brief    The ECP key-pair structure.
 */
typedef struct mbedtls_ecp_keypair {
    mbedtls_ecp_group grp;      /*!<  Elliptic curve and base point     */
    mbedtls_mpi d;              /*!<  our secret value                  */
    mbedtls_ecp_point Q;        /*!<  our public value                  */
} mbedtls_ecp_keypair;

/**
 * \brief           This function initializes a point as zero.
 * \note            This function will assert when internal ctx init failed.
 *
 * \param pt        The point to initialize.
 */
void mbedtls_ecp_point_init(mbedtls_ecp_point *pt);

/**
 * \brief           This function initializes an ECP group context
 *                  without loading any domain parameters.
 *
 * \note            After this function is called, domain parameters
 *                  for various ECP groups can be loaded through the
 *                  mbedtls_ecp_load() functions.
 *                  This function will assert when ecp group init failed.
 *
 * \param grp       The grp to initialize.
 */
void mbedtls_ecp_group_init(mbedtls_ecp_group *grp);

/**
 * \brief           This function initializes a key pair as an invalid one.
 *
 * \param key       The key pair to initialize.
 */
void mbedtls_ecp_keypair_init(mbedtls_ecp_keypair *key);

/**
 * \brief           This function frees the components of a point.
 *
 * \param pt        The point to free. This may be \c NULL, in which
 *                  case this function returns immediately.
 */
void mbedtls_ecp_point_free(mbedtls_ecp_point *pt);

/**
 * \brief           This function frees the components of an ECP group.
 *
 * \param grp       The group to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP group.
 */
void mbedtls_ecp_group_free(mbedtls_ecp_group *grp);

/**
 * \brief           This function frees the components of a key pair.
 *
 * \param key       The key pair to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP key pair.
 */
void mbedtls_ecp_keypair_free(mbedtls_ecp_keypair *key);

/**
 * \brief           This function sets up an ECP group context
 *                  from a standardized set of domain parameters.
 *
 * \note            The index should be a value of the enum defined in
 *                  mbedtls_ecp_group_id.
 *
 * \param grp       The group context to setup. This must be initialized.
 * \param id        The identifier of the domain parameter set to load.
 *
 * \return          \c 0 on success.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_ALLOC_FAILED on memory allocation failed.
 * \return          \c MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE on unsupported group id.
 * \return          \c MBEDTLS_ERR_ECP_HW_FAILED if there is hardware error.
 */
int mbedtls_ecp_group_load(mbedtls_ecp_group *grp, mbedtls_ecp_group_id id);

/**
 * \brief           This function checks that a point is a valid public key
 *                  on this curve.
 *
 *                  It only checks that the point is non-zero, has
 *                  valid coordinates and lies on the curve. It does not verify
 *                  that it is indeed a multiple of \p G. This additional
 *                  check is computationally more expensive, is not required
 *                  by standards, and should not be necessary if the group
 *                  used has a small cofactor. In particular, it is useless for
 *                  the NIST groups which all have a cofactor of 1.
 *
 * \param grp       The ECP group the point should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param pt        The point to check. This must be initialized. The Z coordinate of
 *                  the \p pt point must use affine coordinates, Z == 1.
 *
 * \return          \c 0 if the point is a valid public key.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if the point is not
 *                  a valid public key for the given curve.
 * \return          \c MBEDTLS_ERR_ECP_HW_FAILED if there is hardware error.
 */
int mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
                             const mbedtls_ecp_point *pt);

/**
 * \brief           This function checks that an \p mbedtls_mpi is a
 *                  valid private key for this curve.
 *
 *
 * \param grp       The ECP group the private key should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through mbedtls_ecp_group_load().
 * \param d         The integer to check. This must be initialized.
 *
 * \return          \c 0 if the point is a valid private key.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if the point is not a valid
 *                  private key for the given curve.
 * \return          \c MBEDTLS_ERR_ECP_HW_FAILED if there is hardware error.
 */
int mbedtls_ecp_check_privkey(const mbedtls_ecp_group *grp,
                              const mbedtls_mpi *d);

/**
 * \brief           This function checks that the keypair objects
 *                  \p pub and \p prv have the same group and the
 *                  same public point, and that the private key in
 *                  \p prv is consistent with the public key.
 *
 * \param pub       The keypair structure holding the public key. This
 *                  must be initialized. If it contains a private key, that
 *                  part is ignored.
 * \param prv       The keypair structure holding the full keypair.
 *                  This must be initialized.
 *
 * \return          \c 0 on success, meaning that the keys are valid and match.
 * \return          \c MBEDTLS_ERR_ECP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_ECP_INVALID_KEY if the keys are invalid or do not match.
 * \return          \c MBEDTLS_ERR_ECP_HW_FAILED if there is hardware error.
 */
int mbedtls_ecp_check_pub_priv(const mbedtls_ecp_keypair *pub,
                               const mbedtls_ecp_keypair *prv);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_ECP_H */
