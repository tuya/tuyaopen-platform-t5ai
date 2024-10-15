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

#ifndef __CE_LITE_OTP_H__
#define __CE_LITE_OTP_H__

#include "ce_lite_common.h"
#include "ce_lite_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                            OTP APIs                                       */
/******************************************************************************/
/**
 * \brief   The enumeration of OTP region initial offset.
 */
enum {
    CE_OTP_MODEL_ID_OFFSET = 0x00U,
    CE_OTP_MODEL_KEY_OFFSET = 0x04U,
    CE_OTP_DEVICE_ID_OFFSET = 0x14U,
    CE_OTP_DEVICE_RK_OFFSET = 0x18U,
    CE_OTP_SEC_BOOT_HASH_OFFSET = 0x28U,
    CE_OTP_SEC_DEBUG_HASH_OFFSET = 0x48U,
    CE_OTP_LCS_OFFSET = 0x68U,
    CE_OTP_LOCK_CTRL_OFFSET = 0x7CU,
    CE_OTP_USER_NON_SEC_REGION_OFFSET = 0x80U,
};

/**
 * \brief   The enumeration of OTP region size.
 */
enum {
    CE_OTP_MODEL_ID_SIZE = 0x04U,
    CE_OTP_MODEL_KEY_SIZE = 0x10U,
    CE_OTP_DEVICE_ID_SIZE = 0x04U,
    CE_OTP_DEVICE_RK_SIZE = 0x10U,
    CE_OTP_SEC_BOOT_HASH_SIZE = 0x20U,
    CE_OTP_SEC_DEBUG_HASH_SIZE = 0x20U,
    CE_OTP_LCS_SIZE = 0x04U,
    CE_OTP_LOCK_CTRL_SIZE = 0x04U,
};

#if defined(CE_LITE_OTP_PUF)
/**
 * \brief           The ememory puf operation structure.
 * This structure defines puf operations, the ce lite driver has implemented these hook
 * functions as default, but user could define their own implementations.
 *
 */
typedef struct ce_emem_puf_ops {
    int32_t (*enroll)(void); /*!< This function will trigger puf
                               enrollment. */
    int32_t (*quality_check)(void); /*!< This function will do puf
                                      quality check, just for test purpose. */
    int32_t (*init_mgn_read)(size_t off,
                        uint8_t *buf, size_t len); /*!< This function will do
                                                     init margin read test, just
                                                     for test purpose. */

    int32_t (*pgm_mgn_read)(size_t off,
                        uint8_t *buf, size_t len); /*!< This function will do
                                                     programmed margin read test,
                                                     just for test purpose. */
} ce_emem_puf_ops_t;
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief           The OTP hardware config info  structure.
 * This structure defines the configurations info of OTP hardware, these configurations are
 * read from hardware.
 *
 */
typedef struct ce_otp_cfg_info {
    bool otp_exist;      /*!< TRUE means use hardware otp, FALSE means use dummy otp. */
    uint16_t otp_tst_sz; /*!< Test region size in byte. */
    uint16_t otp_s_sz;    /*!< Secure user region size in byte. */
    uint16_t otp_ns_sz;   /*!< Non secure user region size in byte. */
} ce_otp_cfg_info_t;

/**
 * \brief   This function initializes otp module.
 *
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_SECURITY on LCS is DR and hardware invalidate not
 *                      finished. this error code is not invalid for CE_LITE_OTP_DUMMY.
 */
extern int32_t ce_otp_drv_init(void);

/**
 * \brief   This function exit otp module, in this lite driver, will do nothing.
 */
extern void ce_otp_drv_exit(void);

/**
 * \brief   This function gets the otp user config info from hardware.
 *
 * \note    You must call ce_otp_drv_init() before calling this function.
 *
 * \param[out] conf    user config info structure.
 * \return             CE_SUCCESS on success.
 */
extern int32_t ce_otp_get_cfg_info(ce_otp_cfg_info_t *conf);

#if defined(CE_LITE_OTP_PUF)
/**
 * \brief   This function gets the PUF OTP operations.
 *
 * \note    You must call ce_otp_drv_init() before calling this function.
 * \note    This function is used when the OTP configuration is CE_LITE_OTP_PUF,
 *          if the OTP configuration is CE_LITE_OTP_DUMMY or CE_LITE_OTP_NORMAL,
 *          then \p vpos will be set to \c NULL.
 *
 * \param[out] vops    The pointer for the PUF OTP operation, please refer to
 *                     \struct ce_emem_puf_ops_t.
 * \return             CE_SUCCESS on success.
 */
extern int32_t ce_otp_get_vops(void **vops);

#if defined(CE_LITE_OTP_PUF_ENROLL)
/**
 * \brief   This function will do PUF enrollment, it should be called before the first time
 *          for OTP read or write.
 *
 *          Call this function after a call to ce_otp_drv_init().
 * \param[in] offset    The OTP offset, refer to the OTP layout.
 * \param[in] len       The length that the caller want to read.
 * \param[out] buf      The buffer for reading OTP data.
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 * \note                \p len should equal or less than the \p buf size, or it will
 *                      cause unpredictable result.
 */

static inline int32_t ce_otp_puf_enroll(void)
{
    int32_t ret = CE_SUCCESS;
    ce_emem_puf_ops_t *ops = NULL;

    ret = ce_otp_get_vops((void **)&ops);
    if (ret != CE_SUCCESS){
        goto error;
    }

    if(!ops || !ops->enroll) {
        ret = CE_ERROR_NOT_SUPPORTED;
        goto error;
    }
    ret = ops->enroll();
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_ENROLL */

#if defined(CE_LITE_OTP_PUF_QUALITY_CHECK)
/**
 * \brief   This function will do PUF quality check test, just for test purpose.
 *
 *          Call this function after a call to ce_otp_drv_init().
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 * \note                \p len should equal or less than the \p buf size, or it will
 *                      cause unpredictable result.
 */

static inline int32_t ce_otp_puf_quality_check(void)
{
    int32_t ret = CE_SUCCESS;
    ce_emem_puf_ops_t *ops = NULL;

    ret = ce_otp_get_vops((void **)&ops);
    if (ret != CE_SUCCESS){
        goto error;
    }

    if(!ops || !ops->quality_check) {
        ret = CE_ERROR_NOT_SUPPORTED;
        goto error;
    }
    ret = ops->quality_check();
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_QUALITY_CHECK */

#if defined(CE_LITE_OTP_PUF_INIT_MGN_READ)
/**
 * \brief   This function will do PUF initial margin read test, just for test purpose.
 *
 *          Call this function after a call to ce_otp_drv_init().
 * \param[in] offset    The OTP offset, refer to the OTP layout.
 * \param[in] len       The length that the caller want to read.
 * \param[out] buf      The buffer for reading OTP data.
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 * \note                \p len should equal or less than the \p buf size, or it will
 *                      cause unpredictable result.
 */

static inline int32_t ce_otp_puf_init_mgn_read(size_t offset,
                                               uint8_t *buf, size_t len)
{
    int32_t ret = CE_SUCCESS;
    ce_emem_puf_ops_t *ops = NULL;

    ret = ce_otp_get_vops((void **)&ops);
    if (ret != CE_SUCCESS){
        goto error;
    }

    if(!ops || !ops->init_mgn_read) {
        ret = CE_ERROR_NOT_SUPPORTED;
        goto error;
    }
    ret = ops->init_mgn_read(offset, buf, len);
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_INIT_MGN_READ */

#if defined(CE_LITE_OTP_PUF_PGM_MGN_READ)
/**
 * \brief   This function will do PUF programmed margin read test, just for test purpose.
 *
 *          Call this function after a call to ce_otp_drv_init().
 * \param[in] offset    The OTP offset, refer to the OTP layout.
 * \param[in] len       The lenth that the caller want to read.
 * \param[out] buf      The buffer for reading OTP data.
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 * \note                \p len should equal or less than the \p buf size, or it will
 *                      cause unpredictable result.
 */
static inline int32_t ce_otp_puf_pgm_mgn_read(size_t offset,
                                               uint8_t *buf, size_t len)
{
    int32_t ret = CE_SUCCESS;
    ce_emem_puf_ops_t *ops = NULL;

    ret = ce_otp_get_vops((void **)&ops);
    if (ret != CE_SUCCESS){
        goto error;
    }

    if(!ops || !ops->pgm_mgn_read) {
        ret = CE_ERROR_NOT_SUPPORTED;
        goto error;
    }
    ret = ops->pgm_mgn_read(offset, buf, len);
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_PGM_MGN_READ */
#endif /* CE_LITE_OTP_PUF */

/**
 * \brief   This function reads OTP data according to offset.
 *
 *          Call this function after a call to ce_otp_drv_init(), if the configuration
 *          is CE_LITE_OTP_PUF, and PUF OTP scramble is enabled in hardware, before
 *          OTP read, please make sure the PUF OTP enrollment has been finished. For
 *          the PUF OTP scramble, please check with PUF OTP vendor for more detail.
 * \note    If the configuration is CE_LITE_OTP_DUMMY, only 4 OTP regions can be read:
 *          LCS, MODEL_KEY, DEVICE_RK, LOCK_CTRL. other regions will return \c
 *          CE_ERROR_NOT_SUPPORTED.
 * \param[in] offset    The OTP offset, refer to the above otp intial offset enum.
 *                      For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/
 *                      SECDEBUG_PK_HASH/LCS/LOCK_CTRL regions, the \p offset should strictly
 *                      follow the offsets defined in above enum, other offset will cause
 *                      unpredictable result.
 *                      For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p offset
 *                      should be WORD(4 bytes) aligned, and should be never bigger than start
 *                      address of the region + the size of the region.
 * \param[in] len       The lenth that the caller want to read, refer to above region size enum.
 *                      For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/
 *                      SECDEBUG_PK_HASH/LCS/LOCK_CTRL regions, the \p len should strictly
 *                      follow the region size defined in above enum, other length will cause
 *                      unpredictable result.
 *                      For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p len
 *                      should be WORD(4 bytes) aligned, and should be never bigger than the size
 *                      of the region.
 * \param[out] buf      The buffer for reading OTP data.
 * \return              \c CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 * \note                \p len should equal or less than the \p buf size, or it will
 *                      cause unpredictable result.
 */
extern int32_t ce_otp_read(size_t offset, uint8_t *buf, size_t len);

/**
 * \brief   This function writes data to OTP hardware according to different offset.
 *
 *          Call this function after a call to ce_otp_drv_init(), if the configuration
 *          is CE_LITE_OTP_PUF, and PUF OTP scramble is enabled in hardware, before
 *          OTP write, please make sure the PUF OTP enrollment has been finished. For
 *          the PUF OTP scramble, please check with PUF OTP vendor for more detail.
 * \note    If the configuration is CE_LITE_OTP_DUMMY,
 *          1. Only 4 OTP regions can be written:
 *             LCS, MODEL_KEY, DEVICE_RK, LOCK_CTRL. other regions will retunr \c
 *             CE_ERROR_NOT_SUPPORTED.
 *          2. LOCK_CTRL should write at last, and LOCK_CTRL must write, or the
 *             MODEL_KEY/DEVICE_RK will not be actived.
 *
 * \param[in] offset    The OTP offset, refer to the above otp intial offset enum.
 *                      For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/
 *                      SECDEBUG_PK_HASH/LCS/LOCK_CTRL regions, the \p offset should strictly
 *                      follow the offsets defined in above enum, other offset will cause
 *                      unpredictable result.
 *                      For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p offset
 *                      should be WORD(4 bytes) aligned, and should be never bigger than start
 *                      address of the region + the size of the region.
 * \param[in] len       The lenth that the caller want to read, refer to above region size enum.
 *                      For MODEL_ID/MODEL_KEY/DEVICE_ID/DEVICE_RK/SECBOOT_PUBKEY_HASH/
 *                      SECDEBUG_PK_HASH/LCS/LOCK_CTRL regions, the \p len should strictly
 *                      follow the region size defined in above enum, other length will cause
 *                      unpredictable result.
 *                      For USER_NON_SEC_REGION/USER_SEC_REGION/TEST_REGION, the \p len
 *                      should be WORD(4 bytes) aligned, and should be never bigger than the size
 *                      of the region.
 * \param[in] buf       The buffer stores the data which need to write to OTP.
 * \return              CE_SUCCESS on success.
 *                      \c CE_ERROR_BAD_PARAMS on parameter-verification
 *                      failure.
 *                      \c CE_ERROR_GENERIC on CE hardware failure.
 */
extern int32_t ce_otp_write(size_t offset, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CE_LITE_OTP_H__ */
