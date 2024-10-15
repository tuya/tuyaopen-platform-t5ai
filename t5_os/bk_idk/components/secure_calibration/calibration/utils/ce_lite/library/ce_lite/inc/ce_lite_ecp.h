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

#ifndef __CE_LITE_ECP_H__
#define __CE_LITE_ECP_H__

#include "ce_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                               ECP APIs                                     */
/******************************************************************************/

/**
 * The maximum size of the groups, that is, of \c N and \c P.
 */
#define CE_ECP_MAX_BITS 521 /**< The maximum size of groups, in bits. */

/**
 * \brief Domain-parameter identifiers: curve, subgroup, and generator.
 */
typedef enum ce_ecp_group_id {
    CE_ECP_DP_NONE = 0,  /*!< Curve not defined. */
    CE_ECP_DP_SECP256R1, /*!< Domain parameters for the 256-bit curve defined
                             by FIPS 186-4 and SEC1. */
    CE_ECP_DP_SECP521R1, /*!< Domain parameters for the 521-bit curve defined
                             by FIPS 186-4 and SEC1. */
    CE_ECP_DP_SM2P256V1, /*!< Domain parameters for SM2 curve. */
} ce_ecp_group_id_t;

/**
 * \brief           The ECP point structure, in Jacobian coordinates.
 */
typedef struct ce_ecp_point {
    ce_bn_t X; /*!< The X coordinate of the ECP point. */
    ce_bn_t Y; /*!< The Y coordinate of the ECP point. */
    ce_bn_t Z; /*!< The Z coordinate of the ECP point. */
} ce_ecp_point_t;

/**
 * \brief           The ECP group structure.
 */
typedef struct ce_ecp_group {
    ce_ecp_group_id_t id; /*!< The internal group identifier. */
    ce_bn_t P;            /*!< The prime modulus of the base field. */
    ce_bn_t A;            /*!< A in the equation. */
    ce_bn_t B;            /*!< B in the equation. */
    ce_ecp_point_t G;     /*!< The generator of the subgroup used. */
    ce_bn_t N;            /*!< The order of G. */
    size_t pbits;         /*!< The number of bits in P. */
    size_t nbits;         /*!< The number of bits in N. */
} ce_ecp_group_t;

/**
 * \brief    The ECP key-pair structure.
 */
typedef struct ce_ecp_keypair {
    ce_ecp_group_t grp; /*!<  Elliptic curve and base point     */
    ce_bn_t d;          /*!<  The secret value                  */
    ce_ecp_point_t Q;   /*!<  The public value                  */
} ce_ecp_keypair_t;

/**
 * \brief           This function initializes a point.
 *
 * \param[in] pt    The point to initialize.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_ecp_point_init(ce_ecp_point_t *pt);

/**
 * \brief           This function initializes an ECP group context
 *                  without loading any domain parameters.
 * \note            After this function is called, domain parameters
 *                  for various ECP groups can be loaded through the
 *                  ce_ecp_group_load() function.
 *
 * \param[in] grp   The group to initialize.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_ecp_group_init(ce_ecp_group_t *grp);

/**
 * \brief           This function frees the components of a point.
 *
 * \param[in] pt    The point to free. This may be \c NULL, in which
 *                  case this function returns immediately.
 */
void ce_ecp_point_free(ce_ecp_point_t *pt);

/**
 * \brief           This function frees the components of an ECP group.
 *
 * \param[in] grp   The group to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP group.
 */
void ce_ecp_group_free(ce_ecp_group_t *grp);

/**
 * \brief           This function initializes a key pair as an invalid one.
 *
 * \param[in] key   The key pair to initialize.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 */
int32_t ce_ecp_keypair_init(ce_ecp_keypair_t *key);

/**
 * \brief           This function frees the components of a key pair.
 *
 * \param[in] key   The key pair to free. This may be \c NULL, in which
 *                  case this function returns immediately. If it is not
 *                  \c NULL, it must point to an initialized ECP key pair.
 */
void ce_ecp_keypair_free(ce_ecp_keypair_t *key);

/**
 * \brief           This function sets up an ECP group context
 *                  from a standardized set of domain parameters.
 *
 * \note            The index should be a value of the enum defined in
 *                  ce_ecp_group_id_t.
 *
 * \param[in] grp   The group context to setup. This must be initialized.
 * \param[in] id    The identifier of the domain parameter set to load.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_group_load(ce_ecp_group_t *grp, ce_ecp_group_id_t id);

/**
 * \brief           This function performs a scalar multiplication of a point
 *                  by an integer: \p R = \p m * \p P.
 *
 * \note            To prevent timing attacks, this function
 *                  executes the exact same sequence of base-field
 *                  operations for any valid \p m. It avoids any if-branch or
 *                  array index depending on the value of \p m.
 *
 * \note            If \p f_rng is not NULL, it is used to randomize
 *                  intermediate results to prevent potential timing attacks
 *                  targeting these results. We recommend always providing
 *                  a non-NULL \p f_rng. The overhead is negligible.
 *
 * \param[in] grp   The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[out] R    The point in which to store the result of the calculation.
 *                  This must be initialized.
 * \param[in] m     The integer by which to multiply. This must be initialized.
 * \param[in] P     The point to multiply. This must be initialized.
 * \param[in] f_rng The RNG function. This may be \c NULL if randomization
 *                  of intermediate results isn't desired (discouraged).
 * \param[in] p_rng The RNG context to be passed to \p p_rng.
 *
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key \p P is invalid,
 *                  or private key \p m is invalid.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_mul(const ce_ecp_group_t *grp,
                   ce_ecp_point_t *R,
                   const ce_bn_t *m,
                   const ce_ecp_point_t *P,
                   int32_t (*f_rng)(void *, uint8_t *, size_t),
                   void *p_rng);
/**
 * \brief           This function performs multiplication and addition of two
 *                  points by integers: \p R = \p m * \p P + \p n * \p Q
 *
 * \note            Only support Short Weierstrass curve:
 *                  y^2 = x^3 + A x + B mod P
 *
 * \note            The algorithm uses several common algorithms in ECP:
 *                  1. Jacobian corrdiante and modified jacobian.
 *                  2. NAF.
 *
 * \param[in] grp   The ECP group to use.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[out] R    The point in which to store the result of the calculation.
 *                  This must be initialized.
 * \param[in] m     The integer by which to multiply \p P.
 *                  This must be initialized.
 * \param[in] P     The point to multiply by \p m. This must be initialized.
 * \param[in] n     The integer by which to multiply \p Q.
 *                  This must be initialized.
 * \param[in] Q     The point to be multiplied by \p n.
 *                  This must be initialized.
 * \return          \c CE_SUCCESS on success.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if public key \p P or \p Q is invalid,
 *                  or private key \p m or \p n is invalid.
 * \return          \c CE_ERROR_OOM on memory allocation failed.
 * \return          \c CE_ERROR_GEN_RANDOM if generating random failed.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_muladd(const ce_ecp_group_t *grp,
                      ce_ecp_point_t *R,
                      const ce_bn_t *m,
                      const ce_ecp_point_t *P,
                      const ce_bn_t *n,
                      const ce_ecp_point_t *Q);

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
 * \param[in] grp   The ECP group the point should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[in] pt    The point to check. This must be initialized.
 *
 * \return          \c CE_SUCCESS if the point is a valid public key.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if the point is not
 *                  a valid public key for the given curve.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_check_pubkey(const ce_ecp_group_t *grp,
                            const ce_ecp_point_t *pt);

/**
 * \brief           This function checks that an \p ce_mpi is a
 *                  valid private key for this curve.
 *
 * \param[in] grp   The ECP group the private key should belong to.
 *                  This must be initialized and have group parameters
 *                  set, for example through ce_ecp_group_load().
 * \param[in] d     The integer to check. This must be initialized.
 *
 * \return          \c CE_SUCCESS if the point is a valid private key.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if the point is not a valid
 *                  private key for the given curve.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_check_privkey(const ce_ecp_group_t *grp, const ce_bn_t *d);

/**
 * \brief           This function checks that the keypair objects
 *                  \p pub and \p prv have the same group and the
 *                  same public point, and that the private key in
 *                  \p prv is consistent with the public key.
 *
 * \param[in] pub   The keypair structure holding the public key. This
 *                  must be initialized. If it contains a private key, that
 *                  part is ignored.
 * \param[in] prv   The keypair structure holding the full keypair.
 *                  This must be initialized.
 *
 * \return          \c CE_SUCCESS on success, meaning that the keys are valid
 *                  and match.
 * \return          \c CE_ERROR_BAD_PARAMS on detecting bad parameters.
 * \return          \c CE_ERROR_INVAL_KEY if the keys are invalid or do not
 *                  match.
 * \return          \c CE_ERROR_GENERIC if there is hardware error.
 */
int32_t ce_ecp_check_pub_priv(const ce_ecp_keypair_t *pub,
                              const ce_ecp_keypair_t *prv);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_ECP_H__ */
