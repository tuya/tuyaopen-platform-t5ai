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
#if CONFIG_TE200_UT
#ifndef MBEDTLS_BIGNUM_H
#define MBEDTLS_BIGNUM_H

#include "pal_common.h"
#include "../../library/ce_lite/inc/ce_lite_bn.h"

#define MBEDTLS_ERR_MPI_BAD_INPUT_DATA                    -0x0004  /**< Bad input parameters to function. */
#define MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL                  -0x0008  /**< The buffer is too small to write to. */
#define MBEDTLS_ERR_MPI_ALLOC_FAILED                      -0x0010  /**< Memory allocation failed. */
#define MBEDTLS_ERR_MPI_HW_FAILED                         -0x0011  /**< There is HW error in calling CE driver. */

#ifdef __cplusplus
extern "C" {
#endif

#define MPI2BN(x) (&((x)->p))

/**
 * \brief          MPI structure
 * \note           For MPI based on CE driver, the mbedtls_mpi structure is one
 *                 ce_bn_t instance.
 *                 Please make sure that the MPI ctx has been imported before used,
 *                 otherwise, aca driver might assert.
 */
typedef struct mbedtls_mpi {
    struct ce_bn_t p; /*!<   CE bignum structure  */
} mbedtls_mpi;

/**
 * \brief           Initialize an MPI context.
 *
 *                  This makes the MPI ready to be set or freed,
 *                  but does not define a value for the MPI.
 *
 * \note            The initialized MPI couldn't be treated as 0, it must be
 *                  imported by mbedtls_mpi_read_binary to set value.
 *                  This function will assert when init internal ctx failed.
 *
 * \param X         The MPI context to initialize. This must not be \c NULL.
 */
void mbedtls_mpi_init(mbedtls_mpi *X);

/**
 * \brief          This function frees the components of an MPI context.
 *
 * \param X        The MPI context to be cleared. This may be \c NULL,
 *                 in which case this function is a no-op. If it is
 *                 not \c NULL, it must point to an initialized MPI.
 */
void mbedtls_mpi_free(mbedtls_mpi *X);

/**
 * \brief          Import an MPI from unsigned big endian binary data.
 *
 * \param X        The destination MPI. This must point to an initialized MPI.
 * \param buf      The input buffer. This must be a readable buffer of length
 *                 \p buflen Bytes.
 * \param buflen   The length of the input buffer \p buf in Bytes.
 *
 * \return         \c 0 if successful.
 * \return         \c MBEDTLS_ERR_MPI_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_MPI_ALLOC_FAILED on memory allocation failed.
 */
int mbedtls_mpi_read_binary(mbedtls_mpi *X,
                            const unsigned char *buf,
                            size_t buflen);

/**
 * \brief          Export an MPI into unsigned big endian binary data
 *                 of fixed size.
 *
 * \param X        The source MPI. This must point to an initialized MPI.
 * \param buf      The output buffer. This must be a writable buffer of length
 *                 \p buflen Bytes.
 * \param buflen   The size of the output buffer \p buf in Bytes.
 *
 * \return         \c 0 if successful.
 * \return         \c MBEDTLS_ERR_MPI_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL on buffer size too short to save
 *                 BN data.
 */
int mbedtls_mpi_write_binary(const mbedtls_mpi *X,
                             unsigned char *buf,
                             size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_BIGNUM_H */
#endif
