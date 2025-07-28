// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common.h"
#include <driver/aon_rtc.h>
#include <driver/trng.h>

#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>
#include <os/os.h>

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_printf    printf
#define mbedtls_calloc    calloc
#define mbedtls_free      free
#endif /* MBEDTLS_PLATFORM_C */

#define MAX_RANDOM_DELAY  (5)
static void random_delay(void)
{
	unsigned char delay;

	myrand( NULL, &delay, sizeof(unsigned char) );
	rtos_delay_milliseconds( delay%MAX_RANDOM_DELAY );
}

static const uint8_t Vector_P384_Message[] =
{
    0xAB, 0xE1, 0x0A, 0xCE, 0x13, 0xE7, 0xE1, 0xD9, 0x18, 0x6C, 0x48, 0xF7, 0x88, 0x9D, 0x51, 0x47,
    0x3D, 0x3A, 0x09, 0x61, 0x98, 0x4B, 0xC8, 0x72, 0xDF, 0x70, 0x8E, 0xCC, 0x3E, 0xD3, 0xB8, 0x16,
    0x9D, 0x01, 0xE3, 0xD9, 0x6F, 0xC4, 0xF1, 0xD5, 0xEA, 0x00, 0xA0, 0x36, 0x92, 0xBC, 0xC5, 0xCF,
    0xFD, 0x53, 0x78, 0x7C, 0x88, 0xB9, 0x34, 0xAF, 0x40, 0x4C, 0x03, 0x9D, 0x32, 0x89, 0xB5, 0xBA,
    0xC5, 0xAE, 0x7D, 0xB1, 0x49, 0x68, 0x75, 0xB5, 0xDC, 0x73, 0xC3, 0x09, 0xF9, 0x25, 0xC1, 0x3D,
    0x1C, 0x01, 0xAB, 0xDA, 0xAF, 0xEB, 0xCD, 0xAC, 0x2C, 0xEE, 0x43, 0x39, 0x39, 0xCE, 0x8D, 0x4A,
    0x0A, 0x5D, 0x57, 0xBB, 0x70, 0x5F, 0x3B, 0xF6, 0xEC, 0x08, 0x47, 0x95, 0x11, 0xD4, 0xB4, 0xA3,
    0x21, 0x1F, 0x61, 0x64, 0x9A, 0xD6, 0x27, 0x43, 0x14, 0xBF, 0x0D, 0x43, 0x8A, 0x81, 0xE0, 0x60
};

static int symmetric_algo_test(void)
{
	unsigned char key[32];
	unsigned char in_buf[32];
	unsigned char out_buf[32];
	unsigned char iv[16];
	mbedtls_aes_context ctx;
	int i;

	memset( key, 0xa5, 32 );
	memset( iv,  0x5a, 16 );
	for( i = 0; i < sizeof(in_buf); i++ )
	{
		in_buf[i] = i%0xff;
	}

	mbedtls_aes_init( &ctx );
	mbedtls_aes_setkey_enc( &ctx, key, 256 );
	random_delay();
	mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, sizeof(in_buf), iv, in_buf, out_buf );
	mbedtls_aes_free( &ctx );

	memset( iv,  0x5a, 16 );
	memset( in_buf, 0x0, sizeof(in_buf) );

	mbedtls_aes_init( &ctx );
	mbedtls_aes_setkey_dec( &ctx, key, 256 );
	mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, sizeof(in_buf), iv, out_buf, in_buf );
	random_delay();
	mbedtls_aes_free( &ctx );

	for( i = 0; i < sizeof(in_buf); i++ )
	{
		if( in_buf[i] != i%0xff )
		{
			BK_LOGD(NULL,"Err:sym algo test fail\r\n");
			return -1;
		}
	}

	return 0;
}

static int asymmetric_algo_test(void)
{
	mbedtls_ecdsa_context ctx_sign, ctx_verify;
    unsigned char sig[MBEDTLS_ECDSA_MAX_LEN] = {0};
    size_t sig_len;
	int ret = -1;

    uint8_t digest[32] = {0};
    memset(&digest[0], 0, sizeof(digest));

	mbedtls_sha256(Vector_P384_Message, sizeof(Vector_P384_Message), &digest[0], 0);

    mbedtls_ecdsa_init( &ctx_sign );
    mbedtls_ecdsa_init( &ctx_verify );
	random_delay();

	mbedtls_ecdsa_genkey( &ctx_sign, MBEDTLS_ECP_DP_SECP256R1, myrand, NULL );
	mbedtls_ecdsa_write_signature( &ctx_sign, MBEDTLS_MD_SHA256, &digest[0], sizeof( digest ),
                                            sig, sizeof(sig), &sig_len, myrand, NULL );

	mbedtls_ecp_group_copy( &ctx_verify.grp, &ctx_sign.grp );
	mbedtls_ecp_copy( &ctx_verify.Q, &ctx_sign.Q );
	random_delay();

	ret = mbedtls_ecdsa_read_signature( &ctx_verify, &digest[0], sizeof(digest),sig, sig_len );
	if(ret != 0)
	{
		BK_LOGD(NULL,"Err:asym algo test fail\r\n");
	}

	mbedtls_ecdsa_free( &ctx_verify );
    mbedtls_ecdsa_free( &ctx_sign );
	return ret;
}

void te200_muti_task_test( void *para )
{
	uint32_t count = 0;
	uint32_t max_count = *(uint32_t *)para;

	while(1)
	{
		if( symmetric_algo_test() != 0 )
		{
			goto exit;
		}

		if( asymmetric_algo_test() != 0 )
		{
			goto exit;
		}

		if( ++count > max_count )
		{
			BK_LOGD(NULL,"Suc: mbedtls multi test pass \r\n");
			goto exit;
		}
	}
exit:
	rtos_delete_thread(NULL);
}
