/**
 * @if copyright_display
 *      Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
 * @endif
 */

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "system_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	AES_OPERATION_DECRYPT = 0,
	AES_OPERATION_ENCRYPT = 1,
}key_ladder_mode_t;

typedef struct{
	uint32_t          key_id;
	key_ladder_mode_t mode;
	unsigned char     iv[16];
}key_ladder_context_t;

int dubhe_key_ladder_crypt_aes_cbc( key_ladder_context_t *ex_ctx, size_t length,
                         const unsigned char *input,
                         unsigned char *output );

int dubhe_key_ladder_crypt_aes_ecb( key_ladder_context_t *ex_ctx, size_t length,
                         const unsigned char *input,
                         unsigned char *output );

#if 0
void dubhe_aes_ladder_init( void );
void dubhe_aes_ladder_free( void );
int dubhe_aes_ladder_set_model_key( unsigned int ek1bits );
int dubhe_aes_ladder_set_iv( unsigned char iv[16] );
int dubhe_aes_ladder_encrypt_cbc( size_t length,
                         const unsigned char *input,
                         unsigned char *output );
int dubhe_aes_ladder_decrypt_cbc( size_t length,
                         const unsigned char *input,
                         unsigned char *output );
#endif

#ifdef __cplusplus
}
#endif