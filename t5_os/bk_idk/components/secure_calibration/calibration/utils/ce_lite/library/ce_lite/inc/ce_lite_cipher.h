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

#ifndef __CE_LITE_CIPHER_H__
#define __CE_LITE_CIPHER_H__

#include "ce_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                            Cipher APIs                                     */
/******************************************************************************/

#define MAX_EK1_SIZE        (32U)
#define MAX_EK2_SIZE        (16U)
#define MAX_EK3_SIZE        (16U)
#define MAX_EKS_SIZE        (MAX_EK1_SIZE + MAX_EK2_SIZE + MAX_EK3_SIZE)

/**
 * Crypto engine key ladder root key selection enumeration
 */
typedef enum ce_klad_key_sel {
    CE_KLAD_KEY_MODEL = 0,         /**< model key */
    CE_KLAD_KEY_ROOT               /**< device root key */
} ce_klad_key_sel_t;

/**
 * Crypto engine operation mode
 */
typedef enum ce_operation {
    CE_ENCRYPT = 0,         /**< encryption mode */
    CE_DECRYPT,             /**< decryption mode */
} ce_operation_t;

/**
 * secure key structure
 */
typedef struct ce_sec_key {
    ce_klad_key_sel_t sel;                 /**< key ladder root key selection */
    uint32_t ek1bits;                      /**< ek1 length in bits, 128 or 256 */
    union {
        struct {
            uint8_t ek3[MAX_EK3_SIZE];     /**< encrypted key3 (fixed to 128-bit) */
            uint8_t ek2[MAX_EK2_SIZE];     /**< encrypted key2 (fixed to 128-bit) */
            uint8_t ek1[MAX_EK1_SIZE];     /**< encrypted key1 */
        };
        uint8_t eks[MAX_EKS_SIZE];         /**< ek3 || ek2 || ek1 */
    };
} ce_sec_key_t;

/**
 * Trust engine cipher context structure
 */
struct ce_sca_crypt;
typedef struct ce_cipher_ctx {
    struct ce_sca_crypt *crypt;
} ce_cipher_ctx_t;

/**
 * \brief           This function initializes the cipher context \p ctx.
 *
 * \warning         - Multiple times call this function without free on same
 *                  context will cause memory and hardware slot resources be
 *                  exhausted.
 *                  - Illegal cipher algorithm identifier will cause
 *                  unpredictable behavior.
 *
 * \param[out] ctx  The cipher context.
 * \param[in]  alg  The cipher algorithm.
 *
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_OOM on out-of-memory.
 *                  \c CE_ERROR_BUSY on no free slots.
 *                  \c CE_ERROR_BAD_PARAMS on ctx is an invalid pointer.
 */
int32_t ce_cipher_init(ce_cipher_ctx_t *ctx, ce_algo_t alg);

/**
 * \brief           This function withdraws the cipher context \p ctx.
 *
 * \warning         - Pass corrupted cipher context to this function
 *                  will cause unpredictable behavior.
 *
 * \param[in] ctx   The cipher context.
 * \return          \c N/A.
 */
void ce_cipher_free(ce_cipher_ctx_t *ctx);

/**
 * \brief           This function sets up the iv for the specified
 *                  cipher context \p ctx.
 *
 * \warning         - Pass corrupted cipher context to this function
 *                  will cause unpredictable behavior.
 *                  - Set iv on an in-use cipher context doesn't take any
 *                  effects.
 *                  - Multiple call on this function, only the last time's
 *                  iv takes effect.
 *
 * \param[in] ctx   The cipher context.
 * \param[in] iv    The cipher iv. The iv buffer length must be no less than
 *                  the cipher's block size.
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_BAD_PARAMS on ctx or iv is an invalid pointer.
 */
int32_t ce_cipher_set_iv(ce_cipher_ctx_t *ctx, const uint8_t *iv);

/**
 * \brief           This function gets the iv from the specified
 *                  cipher context \p ctx.
 *
 * \warning         - Pass corrupted cipher context to this function
 *                  will cause unpredictable behavior.
 *                  - Call this function before set iv, the output iv buffer
 *                  will be filled by zero.
 *                  - Call this function always return the last set iv call's
 *                  iv value.
 *
 * \param[in] ctx   The cipher context.
 * \param[out] iv   The cipher iv. The iv buffer length must be no less than
 *                  the cipher's block size.
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_BAD_PARAMS on ctx or iv is an invalid pointer.
 */
int32_t ce_cipher_get_iv(ce_cipher_ctx_t *ctx, uint8_t *iv);

/**
 * \brief           This function sets up the operation mode for the specified
 *                  cipher context \p ctx.
 *
 * \warning         - Pass corrupted cipher context to this function
 *                  will cause unpredictable behavior.
 *                  - Set operation mode on an in-use cipher context doesn't
 *                  take any effects.
 *                  - Multiple call on this function, only the last time's
 *                  operation modes takes effect.
 *
 * \param[in] ctx   The cipher context.
 * \param[in] op    The cipher operation mode.
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_BAD_PARAMS on ctx is an invalid pointer
 *                  or op is invalid.
 */
int32_t ce_cipher_set_op_mode(ce_cipher_ctx_t *ctx, ce_operation_t op);

/**
 * \brief               This function sets up the user key for the specified
 *                      cipher context \p ctx
 *
 * \warning             - Pass corrupted cipher context to this function
 *                      will cause unpredictable behavior.
 *                      - Set user key on an in-use cipher context doesn't
 *                      take any effects.
 *                      - Multiple call on this function, only the last time's
 *                      user key takes effect.
 *                      - Cross call this function with ce_cipher_setseckey,
 *                      only the last call and its input-key takes effect.
 *
 * \param[in] ctx       The cipher context.
 * \param[in] key       The buffer holding the user key.
 * \param[in] keybits   The cipher key length in bit.
 * \return              \c CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on ctx or key is an invalid
 *                      pointer.
 *                      \c CE_ERROR_BAD_KEY_LENGTH on one of below conditions.
 *                          - keybits doesn't equal to 128 or 256 when
 *                          main algorithm is AES.
 *                          - keybits doesn't equal to 128 when main algorithm
 *                          is SM4.
 */
int32_t ce_cipher_setkey(ce_cipher_ctx_t *ctx,
                         const uint8_t *key, uint32_t keybits);

/**
 * \brief           This function sets up the secure key for the specified
 *                  cipher context \p ctx.
 *
 * \warning         - Pass corrupted cipher context to this function
 *                  will cause unpredictable behavior.
 *                  - Set secure key on an in-use cipher context doesn't
 *                  take any effects.
 *                  - Multiple call on this function, only the last time's
 *                  secure key takes effect.
 *                  - Cross call this function with ce_cipher_setkey,
 *                  only the last call and its input-key takes effect.
 *
 * \param[in] ctx   The cipher context.
 * \param[in] key   The secure key.
 * \return          \c CE_SUCCESS on success.
 *                  \c CE_ERROR_BAD_PARAMS on ctx or key is an invalid
 *                    pointer.
 *                  \c CE_ERROR_BAD_PARAMS on the one of below conditions.
 *                      - key->sel is neither CE_KL_KEY_MODEL nor
 *                      CE_KL_KEY_ROOT.
 *                  \c CE_ERROR_BAD_KEY_LENGTH on one of below conditions.
 *                      - key->ek1bits doesn't equal to 128 or 256 when
 *                      main algorithm is AES.
 *                      - key->ek1bits doesn't equal to 128 when main
 *                      algorithm is SM4.
 */
int32_t ce_cipher_setseckey(ce_cipher_ctx_t *ctx, const ce_sec_key_t *key);

/**
 * \brief               This function performs an encryption or
 *                      decryption operation.
 *
 * \warning             - Pass corrupted cipher context to this function
 *                      will cause unpredictable behavior.
 *                      - Once this function be called, the cipher context
 *                      transition into in-use state, the set iv, set key
 *                      and set operation mode APIs doesn't take effects.
 *                      - The input buffer length limited to 0xFFFFFFEFU.
 *
 * \param[in]   ctx     The cipher context.
 * \param[in]   is_last Specifies to this is the last en/decryption operation.
 * \param[in]   ilen    The length of input data.
 * \param[in]   in      The buffer holding the input data.
 * \param[out]  olen    The practical length of output.
 *                      The output length's calculation follows rules as below:
 *                      For the encryption case, by add the padding length.
 *                      For the decryption case, by sub the padding length.
 * \param[out]  out     The buffer holds the output data.
 *                      The buffer length must be no less than
 *                        ROUNDUP(ilen+1, 16) when use PKCS7 padding.
 * \return              \c CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_STATE on iv, Key or operation
 *                      mode doesn't set.
 *                      \c CE_ERROR_BAD_PARAMS on the one of below conditions.
 *                          - Cipher context is invalid.
 *                          - ilen larger than 0xFFFFFFEFU.
 *                          - ilen is 0, is_last is true, decryption operation
 *                        with PKCS7 padding.
 *                          - ilen is not multiple of 16, is_last is false and
 *                        algorithm is not CTR mode.
 *                          - in is an invalid pointer when ilen != 0.
 *                          - out is an invalid pointer when the ilen != 0.
 *                          - out is an invalid pointer when the ilen = 0,
 *                          is_last = true, encryption operation with PKCS7
 *                          padding.
 *                      \c CE_ERROR_BAD_PADDING on the condition of padding data
 *                      can't be identified.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 */
int32_t ce_cipher_crypt(ce_cipher_ctx_t *ctx, bool is_last, size_t ilen,
                        const uint8_t *in, size_t *olen, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_CIPHER_H__ */
