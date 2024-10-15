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
#include "ce_lite_common.h"
#include "mbedtls/platform.h"

int (*mbedtls_printf)( const char *, ... ) = NULL;

int mbedtls_platform_set_printf( int (*printf_func)( const char *, ... ) )
{
    mbedtls_printf = printf_func;
    return( 0 );
}

void * (*mbedtls_calloc)( size_t, size_t ) = NULL;
void (*mbedtls_free)( void * )     = NULL;

int mbedtls_platform_set_calloc_free( void * (*calloc_func)( size_t, size_t ),
                              void (*free_func)( void * ) )
{
    mbedtls_calloc = calloc_func;
    mbedtls_free = free_func;
    return( 0 );
}
static int _convert_retval_to_mbedtls(int errno)
{
    switch (errno) {
        case CE_SUCCESS:
            errno = 0;
            break;
        case CE_ERROR_BAD_PARAMS:
        case CE_ERROR_BAD_FORMAT:
            errno = MBEDTLS_ERR_PLTF_BAD_INPUT;
            break;
        default:
            errno = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            break;
    }

    return errno;
}

int mbedtls_platform_setup(mbedtls_platform_context *ctx)
{
    if (NULL == ctx) {
        return MBEDTLS_ERR_PLTF_BAD_INPUT;
    }
    pal_memset(ctx, 0, sizeof(mbedtls_platform_context));

    return _convert_retval_to_mbedtls(
            ce_drv_init());
}

void mbedtls_platform_teardown(mbedtls_platform_context *ctx)
{
    if (NULL == ctx) {
        return;
    }

    ce_drv_exit();
}
#endif