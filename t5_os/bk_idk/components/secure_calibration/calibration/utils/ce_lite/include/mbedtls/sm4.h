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

#ifndef MBEDTLS_SM4_H
#define MBEDTLS_SM4_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef __ASSEMBLY__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MBEDTLS_SM4_ENCRYPT                         (1)             /**< SM4 encryption. */
#define MBEDTLS_SM4_DECRYPT                         (0)             /**< SM4 decryption. */

#define MBEDTLS_ERR_SM4_ALLOC_FAILED                (-0x0010)       /**< Failed to allocate memory. */
#define MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH          (-0x0020)       /**< Invalid key length. */
#define MBEDTLS_ERR_SM4_INVALID_INPUT_LENGTH        (-0x0022)       /**< Invalid data input length. */
#define MBEDTLS_ERR_SM4_BAD_INPUT_DATA              (-0x0021)       /**< Invalid input data. */
#define MBEDTLS_ERR_SM4_HW_ACCEL_FAILED             (-0x0025)       /**< SM4 hardware accelerator failed. */

/**
* Trust engine key ladder root key selection enumeration
*/
typedef enum mbedtls_sm4_key_sel {
    MBEDTLS_SM4_KL_KEY_MODEL = 0,         /**< model key */
    MBEDTLS_SM4_KL_KEY_ROOT               /**< device root key */
} mbedtls_sm4_key_sel_t;

/**
 * Secure key structure
 */
typedef struct mbedtls_sm4_sec_key {
    mbedtls_sm4_key_sel_t sel;   /**< key ladder root key selection */
    uint32_t ek1bits;            /**< ek1 length in bits, 128-bit */
    union {
        struct {
            uint8_t ek3[16];     /**< encrypted key3 (fixed to 128-bit) */
            uint8_t ek2[16];     /**< encrypted key2 (fixed to 128-bit) */
            uint8_t ek1[32];     /**< encrypted key1 */
        };
        uint8_t eks[64];         /**< ek3 || ek2 || ek1 */
    };
} mbedtls_sm4_sec_key_t;

/**
 * mbedtls sm4 alt context
 */
struct mbedtls_ce_sm4_data;
typedef struct mbedtls_sm4_context_t {
    uint32_t magic;
    struct mbedtls_ce_sm4_data *acd;
} mbedtls_sm4_context;

/**
 * \brief          This function initializes the specified SM4 context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \warning        This function will assert when malloc internal ctx failed.
 *                 This function may have memory leak, if it is initialized multiple times
 *                 without corresponding free.
 *
 * \param ctx      The SM4 context to initialize. This must not be \c NULL.
 */
void mbedtls_sm4_init(mbedtls_sm4_context *ctx);

/**
 * \brief          This function releases and clears the specified SM4 context.
 *
 * \param ctx      The SM4 context to clear. This may be \c NULL, in which
 *                 case this function returns immediately. If it is not \c NULL,
 *                 it must point to an initialized SM4 context.
 *                 Otherwise, it will return directly.
 */
void mbedtls_sm4_free(mbedtls_sm4_context *ctx);

/**
 * \brief          This function sets the encryption key.
 *
 * \warning        Multiple calling are supported, the latest key setting is utilized.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed in bits. Valid options is:
 *                 <ul><li>128 bits</li>/ul>
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p ctx or \p key is
 *                  an invalid pointer.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on \p keybits
 *                  doesn't equal to 128.
 */
int mbedtls_sm4_setkey_enc(mbedtls_sm4_context *ctx,
                           const unsigned char *key,
                           unsigned int keybits);

/**
 * \brief          This function sets the decryption key.
 *
 * \warning        Multiple calling are supported, the latest key setting is utilized.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed. Valid options is:
 *                 <ul><li>128 bits</li></ul>
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p ctx or \p key is
 *                  an invalid pointer.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on \p keybits
 *                  doesn't equal to 128.
 */
int mbedtls_sm4_setkey_dec(mbedtls_sm4_context *ctx,
                           const unsigned char *key,
                           unsigned int keybits);

/**
 * \brief          This function sets the encryption secure key.
 *
 * \warning        Multiple calling are supported, the latest key setting is utilized.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption secure key.
 *                 including ek1 ek2 ek3. Valid options is:
 *                 <ul><li>ek1bits is 128 bits</li></ul>
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p ctx or \p key is
 *                  an invalid pointer.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p key->sel is neither
 *                  MBEDTLS_SM4_KL_KEY_MODEL nor MBEDTLS_SM4_KL_KEY_ROOT.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on \p key->ek1bits
 *                  doesn't equal to 128.
 */
int mbedtls_sm4_setseckey_enc(mbedtls_sm4_context *ctx,
                              mbedtls_sm4_sec_key_t *key);

/**
 * \brief          This function sets the decryption secure key.
 *
 * \warning        Multiple calling are supported, the latest key setting is utilized.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption secure key.
 *                 including ek1 ek2 ek3. Valid options is:
 *                 <ul><li>ek1bits is 128 bits</li></ul>
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p ctx or \p key is an
 *                  invalid pointer.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on \p key->sel is neither
 *                  MBEDTLS_SM4_KL_KEY_MODEL nor MBEDTLS_SM4_KL_KEY_ROOT.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on \p key->ek1bits
 *                  doesn't equal to 128.
 */
int mbedtls_sm4_setseckey_dec(mbedtls_sm4_context *ctx,
                              mbedtls_sm4_sec_key_t *key);

/**
 * \brief          This function performs an SM4 single-block encryption or
 *                 decryption operation.
 *
 *                 It performs the operation defined in the \p mode parameter
 *                 (encrypt or decrypt), on the input data buffer defined in
 *                 the \p input parameter.
 *
 *                 mbedtls_sm4_init(), and either mbedtls_sm4_setkey_enc/dec() or
 *                 mbedtls_sm4_setseckey_enc/dec() must be called before the first
 *                 call to this API with the same context.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \c 16 Bytes long.
 * \param output   The buffer where the output data will be written.
 *                 It must be readable/writeable and at least \c 16 Bytes long.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                      - one of \c ctx, \c input and \c output is invalid pointer.
 *                      - \c mode is invalid.
 *                      - \c ctx is not initialized.
 * \return         \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                    or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return         \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware or driver failure.
 */
int mbedtls_sm4_crypt_ecb(mbedtls_sm4_context *ctx,
                           int mode,
                           const unsigned char input[16],
                           unsigned char output[16]);

/**
 * \brief          This function performs an SM4 multiple-block encryption or
 *                 decryption extension of operation.
 *
 *                 It performs the operation defined in the \p mode parameter
 *                 (encrypt or decrypt), on the input data buffer defined in
 *                 the \p input parameter.
 *
 *                 mbedtls_sm4_init(), and either mbedtls_sm4_setkey_enc/dec() or
 *                 mbedtls_sm4_setseckey_enc/dec() must be called before the first
 *                 call to this API with the same context.
 *
 *                 The maximum input data length support is 0xFFFFFFEF.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param inlen    The length of the input data in Bytes. This must be a
 *                 multiple of the block size (\c 16 Bytes).
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \p inlen Bytes long.
 * \param output   The buffer where the output data will be written.
 *                 It must be readable/writeable and at least \p inlen Bytes long.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                      - one of \p ctx, \p input and \p output is invalid pointer.
 *                      - \p mode is invalid.
 *                      - \p ctx is not initialized.
 *                      - \p inlen is not multiple of 16.
 *                      - \p inlen is greater than \c 0xFFFFFFEF.
 * \return         \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                    or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return         \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware failure.
 */
int mbedtls_sm4_crypt_ecb_ext(mbedtls_sm4_context *ctx,
                              int mode,
                              size_t inlen,
                              const unsigned char *input,
                              unsigned char *output);

/**
 * \brief          This function performs an SM4 multiple-block encryption or
 *                 decryption by PKCS7 padding.
 *
 *                 It performs the operation defined in the \p mode parameter
 *                 (encrypt or decrypt), on the input data buffer defined in
 *                 the \p input parameter.
 *
 *                 mbedtls_sm4_init(), and either mbedtls_sm4_setkey_enc/dec() or
 *                 mbedtls_sm4_setseckey_enc/dec() must be called before the first
 *                 call to this API with the same context.
 *
 *                 The maximum input length support is 0xFFFFFFEF.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param is_last  Set it to true on the last data blocks. Or false otherwise.
 * \param inlen    Specifies the \p input data length in Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \p inlen Bytes long.
 * \param outlen   Optional. NULL to omit. Or loaded with the \p output data length on output.
 * \param output   The caller should ensure the \p output buffer be more
 *                 enough to load the output data. Otherwise, the behavior
 *                 is not defined. It must be readable/writeable.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                      - one of \p ctx, \p input and \p output is invalid pointer.
 *                      - \p mode is invalid.
 *                      - \p ctx is not initialized.
 *                      - \p inlen is 0, \p is_last is ture, \p mode is MBEDTLS_SM4_DECRYPT.
 *                      - \p inlen is not multiple of 16, \p is_last is false.
 *                      - \p inlen is greater than \c 0xFFFFFFEF.
 *                      - \p output is an invalid pointer when the \p inlen is 0, \p is_last
 *                        is true, \p mode is \c MBEDTLS_SM4_ENCRYPT with PKCS7 padding.
 *                      - padding data can't be identified.
 * \return         \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                    or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return         \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware failure.
 */
int mbedtls_sm4_crypt_ecb_pkcs7(mbedtls_sm4_context *ctx,
                                int mode,
                                bool is_last,
                                size_t inlen,
                                const unsigned char *input,
                                size_t *outlen,
                                unsigned char *output);

/**
 * \brief  This function performs an SM4-CBC encryption or decryption operation
 *         on full blocks.
 *
 *         It performs the operation defined in the \p mode
 *         parameter (encrypt/decrypt), on the input data buffer defined in
 *         the \p input parameter.
 *
 *         It can be called as many times as needed, until all the input
 *         data is processed. mbedtls_sm4_init(), and either
 *         mbedtls_sm4_setkey_enc/dec() or mbedtls_sm4_setseckey_enc/dec()
 *         must be called before the first call to this API with the same
 *         context.
 *
 *         The maximum input data length support is 0xFFFFFFEF.
 *
 * \note   This function operates on full blocks, that is, the input size
 *         must be a multiple of the SM4 block size of \c 16 Bytes.
 *
 * \note   Upon exit, the content of the IV is updated so that you can
 *         call the same function again on the next
 *         block(s) of data and get the same result as if it was
 *         encrypted in one call. This allows a "streaming" usage.
 *         If you need to retain the contents of the IV, you should
 *         either save it manually or use the cipher module instead.
 *
 * \note   Enable \c CE_LITE_CIPHER_MULTI_BLOCKS cipher call updating
 *         IV/stream block after each cipher operation.
 *         For CBC cipher mode, calling the crypt APIs will update the IV value in
 *         parameter. When the macro \c CE_LITE_CIPHER_MULTI_BLOCKS is not defined,
 *         IV array is always treated as input parameter.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param length   The length of the input data in Bytes. This must be a
 *                 multiple of the block size (\c 16 Bytes).
 * \param iv       Initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and of size \p length Bytes.
 * \param output   The buffer holding the output data.
 *                 It must be writeable and of size \p length Bytes.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                      - one of \p ctx, \p iv, \p input and \p output is invalid pointer.
 *                      - \p mode is invalid.
 *                      - \p ctx is not initialized.
 *                      - \p length is not multiple of 16.
 *                      - \p length is greater than \c 0xFFFFFFEF.
 * \return         \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                    or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return         \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware failure.
 */
int mbedtls_sm4_crypt_cbc(mbedtls_sm4_context *ctx,
                          int mode,
                          size_t length,
                          unsigned char iv[16],
                          const unsigned char *input,
                          unsigned char *output);

/**
 * \brief  This function performs an SM4-CBC encryption or decryption operation
 *         on full blocks by PKCS7 padding.
 *
 *         It performs the operation defined in the \p mode
 *         parameter (encrypt/decrypt), on the input data buffer defined in
 *         the \p input parameter.
 *
 *         The maximum input length support is 0xFFFFFFEF.
 *
 *         It can be called as many times as needed, until all the input
 *         data is processed. mbedtls_sm4_init(), and either
 *         mbedtls_sm4_setkey_enc/dec() or mbedtls_sm4_setseckey_enc/dec()
 *         must be called before the first call to this API with the same
 *         context.
 *
 * \note   Upon exit, the content of the IV is updated so that you can
 *         call the same function again on the next
 *         block(s) of data and get the same result as if it was
 *         encrypted in one call. This allows a "streaming" usage.
 *         If you need to retain the contents of the IV, you should
 *         either save it manually or use the cipher module instead.
 *
 * \note   If is_last is true, the returned IV should not be used.
 *
 * \note   Enable \c CE_LITE_CIPHER_MULTI_BLOCKS cipher call updating
 *         IV/stream block after each cipher operation.
 *         For CBC cipher mode, calling the crypt APIs will update the IV value in
 *         parameter. When the macro \c CE_LITE_CIPHER_MULTI_BLOCKS is not defined,
 *         IV array is always treated as input parameter.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param is_last  Set it to true on the last data blocks. Or false otherwise.
 * \param iv       Initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param inlen    Specifies the \p input data length in Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \p inlen Bytes long.
 * \param outlen   Optional. NULL to omit. Or loaded with the \p output data length on output.
 * \param output   The caller should ensure the \p output buffer be more
 *                 enough to load the output data. Otherwise, the behavior
 *                 is not defined. It must be readable/writeable.
 *
 * \return         \c 0 on success.
 * \return         \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                      - one of \p ctx , \p input and \p output
                          is invalid pointer.
 *                      - \p mode is invalid.
 *                      - \p ctx is not initialized.
 *                      - \p inlen is 0, \p is_last is ture, \p mode is MBEDTLS_SM4_DECRYPT.
 *                      - \p inlen is not multiple of 16, \p is_last is false.
 *                      - \p inlen is greater than \c 0xFFFFFFEF.
 *                      - \p output is an invalid pointer when the \p inlen is 0, \p is_last
 *                        is true, \p mode is \c MBEDTLS_SM4_ENCRYPT with PKCS7 padding.
 *                      - padding data can't be identified.
 * \return         \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return         \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                    or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return         \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware failure.
 */
int mbedtls_sm4_crypt_cbc_pkcs7(mbedtls_sm4_context *ctx,
                                int mode,
                                bool is_last,
                                unsigned char iv[16],
                                size_t inlen,
                                const unsigned char *input,
                                size_t *outlen,
                                unsigned char *output);

/**
 * \brief      This function performs an SM4-CTR encryption or decryption
 *             operation.
 *
 *             This function performs the operation defined in the \p mode
 *             parameter (encrypt/decrypt), on the input data buffer
 *             defined in the \p input parameter.
 *
 *             Due to the nature of CTR, you must use the same key schedule
 *             for both encryption and decryption operations. Therefore, you
 *             must use the context initialized with
 *             mbedtls_sm4_setkey_enc/dec() or mbedtls_sm4_setseckey_enc/dec()
 *             for both #MBEDTLS_SM4_ENCRYPT and #MBEDTLS_SM4_DECRYPT.
 *
 *             The maximum input data length support is 0xFFFFFFEF.
 *
 * \note       Enable \c CE_LITE_CIPHER_MULTI_BLOCKS cipher call updating
 *             IV/stream block after each cipher operation.
 *             For CTR cipher mode, calling the crypt APIs will update nonce_counter and
 *             stream block, which are reused in the next crypt session. When the macro
 *             \c CE_LITE_CIPHER_MULTI_BLOCKS is not defined, nonce_counter is always
 *             treated as input parameter, the nc_off and stream block are ignored.
 *
 * \warning    Upon return, \p stream_block contains sensitive data. Its
 *             content must not be written to insecure storage and should be
 *             securely discarded as soon as it's no longer needed.
 *
 * \param ctx              The SM4 context to use for encryption or decryption.
 *                         It must be initialized and bound to a key.
 * \param length           The length of the input data.
 * \param nc_off           The offset in the current \p stream_block, for
 *                         resuming within the current cipher stream. The
 *                         offset pointer should be 0 at the start of a stream.
 *                         It must point to a valid \c size_t.
 * \param nonce_counter    The 128-bit nonce and counter.
 *                         It must be a readable-writeable buffer of \c 16 Bytes.
 * \param stream_block     The saved stream block for resuming. This is
 *                         overwritten by the function.
 *                         It must be a readable-writeable buffer of \c 16 Bytes.
 * \param input            The buffer holding the input data.
 *                         It must be readable and of size \p length Bytes.
 * \param output           The buffer holding the output data.
 *                         It must be writeable and of size \p length Bytes.
 *
 * \return                 \c 0 on success.
 * \return                 \c MBEDTLS_ERR_SM4_BAD_INPUT_DATA on one of below conditions:
 *                              - one of \p ctx, \p nc_off, \p nonce_counter, \p stream_block,
 *                                and \p output is invalid pointer.
 *                              - \p length is non-zero, \p input is invalid pointer.
 *                              - \p inlen is greater than \c 0xFFFFFFEF.
 *                              - \p ctx is not initialized.
 *                              - \p *nc_off is greater than \c 15.
 * \return                 \c MBEDTLS_ERR_SM4_ALLOC_FAILED on init CE driver out-of-memory.
 * \return                 \c MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on mbedtls_sm4_setkey_enc/dec()
 *                            or mbedtls_sm4_setseckey_enc/dec() has not been called.
 * \return                 \c MBEDTLS_ERR_SM4_HW_ACCEL_FAILED on CE hardware failure.
 */
int mbedtls_sm4_crypt_ctr(mbedtls_sm4_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output);

#endif  /* __ASSEMBLY__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* MBEDTLS_SM4_H */