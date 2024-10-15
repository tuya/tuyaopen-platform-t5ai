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

#ifndef MBEDTLS_OTP_H
#define MBEDTLS_OTP_H

#include "pal_common.h"

#define MBEDTLS_ERR_OTP_BAD_INPUT_DATA                 -0x0073  /* OTP input data was malformed. */
#define MBEDTLS_ERR_OTP_OVERFLOW                       -0x0030  /* OTP overflow. */
#define MBEDTLS_ERR_OTP_ACCESS_DENIED                  -0x0031  /* OTP access denied. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The offset of user-secure region and test region
 * conf: the ptr of mbedtls_otp_conf
 */
#define MBEDTLS_OTP_USER_SEC_REGION_OFFSET(conf)    ((MBEDTLS_OTP_USER_NON_SEC_REGION_OFFSET) + \
                                                     ((conf)->otp_ns_sz))
#define MBEDTLS_OTP_TEST_REGION_OFFSET(conf)        ((MBEDTLS_OTP_USER_NON_SEC_REGION_OFFSET) + \
                                                     ((conf)->otp_ns_sz) + \
                                                     ((conf)->otp_s_sz))

/**
 * OTP LAYOUT
 */
enum {
    MBEDTLS_OTP_MODEL_ID_OFFSET = 0x00U,
    MBEDTLS_OTP_MODEL_KEY_OFFSET = 0x04U,
    MBEDTLS_OTP_DEVICE_ID_OFFSET = 0x14U,
    MBEDTLS_OTP_DEVICE_RK_OFFSET = 0x18U,
    MBEDTLS_OTP_SEC_BOOT_HASH_OFFSET = 0x28U,
    MBEDTLS_OTP_SEC_DEBUG_HASH_OFFSET = 0x48U,
    MBEDTLS_OTP_LCS_OFFSET = 0x68U,
    MBEDTLS_OTP_LOCK_CTRL_OFFSET = 0x7CU,
    MBEDTLS_OTP_USER_NON_SEC_REGION_OFFSET = 0x80U,
};

enum {
    MBEDTLS_OTP_MODEL_ID_SIZE = 0x04U,
    MBEDTLS_OTP_MODEL_KEY_SIZE = 0x10U,
    MBEDTLS_OTP_DEVICE_ID_SIZE = 0x04U,
    MBEDTLS_OTP_DEVICE_RK_SIZE = 0x10U,
    MBEDTLS_OTP_SEC_BOOT_HASH_SIZE = 0x20U,
    MBEDTLS_OTP_SEC_DEBUG_HASH_SIZE = 0x20U,
    MBEDTLS_OTP_LCS_SIZE = 0x04U,
    MBEDTLS_OTP_LOCK_CTRL_SIZE = 0x04U,
};

typedef enum {
    MODEL_ID,
    MODEL_KEY,
    DEV_ID,
    DEV_ROOT_KEY,
    SEC_BT_PK_HASH,
    SEC_DBG_PK_HASH,
    LIFE_CYCLE,
    LOCK_CTRL,
    USR_NON_SEC_REGION,
    USR_SEC_REGION,
    TEST_REGION,
} mbedtls_otp_info_t;

typedef struct mbedtls_otp_conf {
    bool otp_exist;
    uint16_t otp_tst_sz;          /* test region size in byte */
    uint16_t otp_s_sz;            /* sec region size in byte */
    uint16_t otp_ns_sz;           /* ns region size in byte */
} mbedtls_otp_conf;

typedef void* mbedtls_otp_context;

/**
 * \brief          This function reads OTP data according to region type and offset.
 * \note           If the configuration is CE_LITE_OTP_PUF, and PUF OTP scramble is enabled in hardware,
 *                 before OTP read, please make sure the PUF OTP enrollment has been finished. It is not
 *                 recommended to use the OTP data read before the enrollment as the OTP data would
 *                 vary after then.
 *                 For the PUF OTP scramble, please check with PUF OTP vendor for more detail.
 *                 If the configuration is CE_LITE_OTP_DUMMY, only 4 OTP regions can read:
 *                 LCS, MODEL_KEY, DEVICE_RK, LOCK_CTRL. other regions will return
 *                 \c MBEDTLS_ERR_OTP_ACCESS_DENIED. Please refer to TE-200 TRM for more DUMMY OTP
 *                 details.
 *
 * \param type     The region type. Must be the enum in the mbedtls_otp_info_t.
 * \param output   The buffer to hold data.
 * \param size     The length that the caller want to read, refer to above region size enum.
 *                 For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/SECDEBUG_PK_HASH/
 *                 LCS/LOCK_CTRL regions, the \p size should strictly follow the region size
 *                 defined in above enum, other length will cause unpredictable result.
 *                 For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p size
 *                 should be WORD(4 bytes) aligned, and the sum of \p size and \p offset should
 *                 be never bigger than the size of the region, \p size should equal or less than
 *                 the \p buf size, or it will cause unpredictable result.
 * \param offset   The offset from the start address of the otp region relative to \p type.
 *                 For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/SECDEBUG_PK_HASH/
 *                 LCS/LOCK_CTRL regions, the \p offset should must be 0,
 *                 other offset will cause unpredictable result.
 *                 For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p offset should
 *                 be WORD(4 bytes) aligned, and the sum of \p size and \p offset
 *                 should not exceed the upper region boundary, otherwise the result is unpredictable.
 *
 * \return         \c 0 if successful
 * \return         \c MBEDTLS_ERR_OTP_BAD_INPUT_DATA on detecting bad parameters.
 * \return         \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_get_otp_info_ex(mbedtls_otp_info_t type,
                            void *output,
                            size_t size,
                            size_t offset);

/**
 * \brief          This function gets OTP's configuration in engine.
 *
 * \param conf     The config obj to hold the config data.
 *
 * \return         \c 0 if successful
 * \return         \c MBEDTLS_ERR_OTP_BAD_INPUT_DATA on detecting bad parameters.
 */
int mbedtls_otp_get_conf(mbedtls_otp_conf *conf);

/**
 * \brief           This function writes data to OTP hardware according to region type and offset.
 * \note            If the configuration is CE_LITE_OTP_PUF, and PUF OTP scramble is enabled in hardware,
 *                  before OTP write, please make sure the PUF OTP enrollment has been finished. It is not
 *                  recommended to use the OTP data write before the enrollment, otherwise the result
 *                  is unpredictable.
 *                  For the PUF OTP scramble, please check with PUF OTP vendor for more detail.
 *                  If the configuration is CE_LITE_OTP_DUMMY:
 *                  1. Only 4 OTP regions can write:
 *                  LCS, MODEL_KEY, DEVICE_RK, LOCK_CTRL. other regions will return
 *                  \c MBEDTLS_ERR_OTP_ACCESS_DENIED
 *                  2. LOCK_CTRL should write at last, and LOCK_CTRL must write, or the
 *                  MODEL_KEY/DEVICE_RK will not be active.
 *
 * \param type      The region type. Must be the enum in the mbedtls_otp_info_t.
 * \param input     The buffer to hold data.
 * \param ilen      The length that the caller want to read, refer to above region size enum.
 *                  For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/SECDEBUG_PK_HASH/
 *                  LCS/LOCK_CTRL regions, the \p ilen should strictly follow the region size
 *                  defined in above enum, other length will cause unpredictable result.
 *                  For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p ilen
 *                  should be WORD(4 bytes) aligned, and the sum of \p ilen and \p offset should
 *                  be never bigger than the size of the region.
 * \param offset    The offset from the start address of the otp region relative to \p type.
 *                  For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/SECDEBUG_PK_HASH/
 *                  LCS/LOCK_CTRL regions, the \p offset should must be 0,
 *                  other offset will cause unpredictable result.
 *                  For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p offset
 *                  should be WORD(4 bytes) aligned, and the sum of \p ilen and \p offset should
 *                  not exceed the upper region boundary, otherwise the result is unpredictable.
 *
 * \return          \c 0 if successful
 * \return          \c MBEDTLS_ERR_OTP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_set_otp_info_ex(mbedtls_otp_info_t type,
                            const void *input,
                            size_t ilen,
                            size_t offset);

/**
 * \brief           This function generates the device root key. When PUF enable
 *                  it's the only way to derive device root key.
 * \note            It should be called before the first time for OTP read or write.
 *                  This function is available when CE_LITE_OTP_PUF is enabled.
 *                  For more information about PUF OTP enrollment,
 *                  please check with PUF OTP vendor for more detail.
 *
 * \return          \c 0 if successful
 * \return          \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_otp_puf_enrollment(void);

/**
 * \brief           This function checks the physical status of PUF.
 * \note            This function is available when CE_LITE_OTP_PUF is enabled.
 *                  For more information about PUF OTP quality check,
 *                  please check with PUF OTP vendor for more detail.
 *
 * \return          \c 0 if in good status.
 * \return          \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_otp_puf_quality_check(void);

/**
 * \brief           This function does initial margin reading when PUF enable.
 * \note            This function is available when CE_LITE_OTP_PUF is enabled.
 *                  For more information about PUF OTP initial margin read,
 *                  please check with PUF OTP vendor for more detail.
 *
 * \param area      The region area. Must be the enum in the mbedtls_otp_info_t.
 * \param output    The buffer to hold data.
 * \param size      Length expected to read. For the restrictions of \p size,
 *                  please refer to mbedtls_get_otp_info_ex or mbedtls_set_otp_info_ex.
 * \param offset    The offset from the start address of the otp region relative to \p area.
 *                  For the restrictions of \p offset, please refer to mbedtls_get_otp_info_ex
 *                  or mbedtls_set_otp_info_ex.
 *
 * \return          \c 0 if successsful.
 * \return          \c MBEDTLS_ERR_OTP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_otp_puf_initial_margin_read(mbedtls_otp_info_t area,
                                        void *output,
                                        size_t *size,
                                        size_t offset);

/**
 * \brief           This function does program margin reading when PUF enable.
 * \note            This function is available when CE_LITE_OTP_PUF is enabled.
 *                  For more information about PUF OTP program margin read,
 *                  please check with PUF OTP vendor for more detail.
 *
 * \param area      The region area. Must be the enum in the mbedtls_otp_info_t.
 * \param output    The buffer to hold data.
 * \param size      Length expected to read. For the restrictions of \p size,
 *                  please refer to mbedtls_get_otp_info_ex or mbedtls_set_otp_info_ex.
 * \param offset    The offset from the start address of the otp region relative to \p area.
 *                  For the restrictions of \p offset, please refer to mbedtls_get_otp_info_ex
 *                  or mbedtls_set_otp_info_ex.
 *
 * \return          \c 0 if successsful.
 * \return          \c MBEDTLS_ERR_OTP_BAD_INPUT_DATA on detecting bad parameters.
 * \return          \c MBEDTLS_ERR_OTP_ACCESS_DENIED if there is hardware error.
 */
int mbedtls_otp_puf_pgm_margin_read(mbedtls_otp_info_t area,
                                    void *output,
                                    size_t *size,
                                    size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_OTP_H */
