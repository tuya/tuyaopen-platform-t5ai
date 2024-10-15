/*
 * Copyright (c) 2020-2021, Arm Technology (China) Co., Ltd.
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
#include "ce_lite_bn.h"
#include "ce_lite_common.h"
#include "ce_lite_config.h"
#include "mbedtls/bignum.h"
#include "mbedtls/platform.h"

#if defined(CE_LITE_BN)
static int _convert_retval_to_mbedtls(int errno)
{
    switch (errno) {
        case CE_SUCCESS:
            errno = 0;
            break;
        case CE_ERROR_BAD_PARAMS:
            errno = MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
            break;
        case CE_ERROR_OOM:
            errno = MBEDTLS_ERR_MPI_ALLOC_FAILED;
            break;
        case CE_ERROR_SHORT_BUFFER:
            errno = MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL;
            break;
        default:
            errno = MBEDTLS_ERR_MPI_HW_FAILED;
            break;
    }

    return errno;
}

void mbedtls_mpi_init(mbedtls_mpi *X)
{
    int ret = CE_SUCCESS;

    if (NULL == X) {
        return;
    }

    (void)ret;
    pal_memset(X, 0, sizeof(mbedtls_mpi));

    ret = ce_bn_init(MPI2BN(X));
    PAL_ASSERT(CE_SUCCESS == ret);
}

void mbedtls_mpi_free(mbedtls_mpi *X)
{
    if (NULL == X) {
        return;
    }

    ce_bn_free(MPI2BN(X));
    pal_memset(X, 0, sizeof(mbedtls_mpi));
}

int mbedtls_mpi_read_binary(mbedtls_mpi *X,
                            const unsigned char *buf,
                            size_t buflen)
{
    if (NULL == X) {
        return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
    }

    return _convert_retval_to_mbedtls(
            ce_bn_import(MPI2BN(X),
                         buf,
                         buflen));

}

int mbedtls_mpi_write_binary(const mbedtls_mpi *X,
                             unsigned char *buf,
                             size_t buflen)
{
    if (NULL == X) {
        return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
    }

    return _convert_retval_to_mbedtls(
            ce_bn_export(MPI2BN(X),
                         buf,
                         buflen));
}
#endif /* CE_LITE_BN */
#endif