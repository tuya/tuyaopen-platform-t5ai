/**
 * \file platform.h
 *
 * \brief This file contains the definitions and functions of the
 *        Mbed TLS platform abstraction layer.
 *
 *        The platform abstraction layer removes the need for the library
 *        to directly link to standard C library functions or operating
 *        system services, making the library easier to port and embed.
 *        Application developers and users of the library can provide their own
 *        implementations of these functions, or implementations specific to
 *        their platform, which can be statically linked to the library or 
 *        dynamically configured at runtime.
 */
/*
 *  Copyright (C) 2006-2018, Arm Limited (or its affiliates), All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */
#if CONFIG_TE200_UT
#ifndef MBEDTLS_PLATFORM_H
#define MBEDTLS_PLATFORM_H

#include "pal_common.h"
#include "pal_heap.h"
#include "pal_log.h"
#include "pal_string.h"

#define MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED     -0x0070 /**< Hardware accelerator failed */
#define MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED -0x0072 /**< The requested feature is not supported by the platform */
#define MBEDTLS_ERR_PLTF_BAD_INPUT               -0x000D /**< Bad input parameters to the function. */

#ifdef __cplusplus
extern "C" {
#endif

extern void * (*mbedtls_calloc)( size_t n, size_t size );
extern void (*mbedtls_free)( void *ptr );

/**
 * \brief               This function dynamically sets the memory-management
 *                      functions used by the library, during runtime.
 *
 * \param calloc_func   The \c calloc function implementation.
 * \param free_func     The \c free function implementation.
 *
 * \return              \c 0.
 */
int mbedtls_platform_set_calloc_free( void * (*calloc_func)( size_t, size_t ),
		                              void (*free_func)( void * ) );
extern int (*mbedtls_printf)( const char *format, ... );

/**
 * \brief               This function dynamically configures the snprintf
 *                      function that is called when the mbedtls_snprintf()
 *                      function is invoked by the library.
 *
 * \param printf_func   The \c printf function implementation.
 *
 * \return              \c 0 on success.
 */
int mbedtls_platform_set_printf( int (*printf_func)( const char *, ... ) );


/**
 * \brief   The platform context structure.
 *
 * \note    This structure may be used to assist platform-specific
 *          setup or teardown operations.
 */
typedef struct {
    char dummy; /**< A placeholder member, as empty structs are not portable. */
}
mbedtls_platform_context;


/**
 * \brief   This function performs any platform-specific initialization
 *          operations.
 *
 * \note    This function should be called before any other library functions.
 *
 *          Its implementation is platform-specific, and unless
 *          platform-specific code is provided, it does nothing.
 *
 * \note    The usage and necessity of this function is dependent on the platform.
 *
 * \param   ctx     The platform context.
 *
 * \return  \c 0 on success.
 */
int mbedtls_platform_setup( mbedtls_platform_context *ctx );
/**
 * \brief   This function performs any platform teardown operations.
 *
 * \note    This function should be called after every other Mbed TLS module
 *          has been correctly freed using the appropriate free function.
 *
 *          Its implementation is platform-specific, and unless
 *          platform-specific code is provided, it does nothing.
 *
 * \note    The usage and necessity of this function is dependent on the platform.
 *
 * \param   ctx     The platform context.
 *
 */
void mbedtls_platform_teardown( mbedtls_platform_context *ctx );

#ifdef __cplusplus
}
#endif

#endif /* platform.h */

#endif
