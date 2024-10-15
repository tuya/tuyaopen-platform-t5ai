/**
 * Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#ifdef ARM_CE_DUBHE_OTP

#include "hal.h"
#include "pal.h"
#include "mem_layout.h"
#include "utils.h"
#include "hal_src_internal.h"
#include "flash_operation.h"
#include "mbedtls/otp.h"
#include "mbedtls/aes.h"
#include "hal_dubhe_config.h"

#if CONFIG_BK_BOOT
#include "pal_log_redefine.h"
#endif

/**
 * Note:
 *  This is the HAL implementation for MPS2+ AN495/AN495+/AN511 which has:
 *  1. SRAM simulated OTP in Dubhe, no e-memory.
 *  2. External SPI flash simulated OTP.
 *
 * This HAL implementation keeps the OTP data consistency between the two media.
 *
 *      !!! DON'T use this file directly in your project !!!
 *      !!! DON'T use this file directly in your project !!!
 *      !!! DON'T use this file directly in your project !!!
 *
 * You should implement your Hardware Adaption Layer code of OTP
 * reading/writing.
 */

/**
 * In our simulated OTP, the OTP bits changing rule is:
 * Only can be changed from 0 --> 1
 * For customers has the opposite OTP logic, please also change the hal_otp
 * implementation.
 */

#define _HAL_OTP_BASE_ADDR (LAYOUT_SIMU_OTP_FLASH_OFFSET)

#ifdef DBH_OTP_PUF_SUPPORT

#define _DEVICE_ROOT_KEY_LOCK_BIT (0x1 << 3)
#define _IS_DEVICE_ROOT_KEY_LOCK(__lock_ctrl_otp_u32_ptr__)                    \
    ((__lock_ctrl_otp_u32_ptr__)[0] & _DEVICE_ROOT_KEY_LOCK_BIT)
#define _SET_DEVICE_ROOT_KEY_LOCK(__lock_ctrl_otp_u32_ptr__)                   \
    do {                                                                       \
        (__lock_ctrl_otp_u32_ptr__)[0] =                                       \
            (__lock_ctrl_otp_u32_ptr__)[0] | _DEVICE_ROOT_KEY_LOCK_BIT;        \
    } while (0)

#endif /* DBH_OTP_PUF_SUPPORT */
/**
 * This HAL is only for MPS2_AN495/AN511 FPGA with dubhe inside.
 * Don't use it in your project because it may cause unexpected result.
 *
 * In our FPGA, we don't have a real OTP file, instead we have SRAM simulated
 * OTP. Here we reset the OTP to zero for the first time calling hal_otp_init.
 * For the OTP in the engine, we can make sure they are zero because
 * arm_ce_otp_driver_init() has already reset the OTP engine.
 */
#define _HAL_DUBHE_TOP_ADDR (DUBHE_BASE_ADDR)
#define _HAL_DUBHE_OTP_MANAGEMENT_ADDR (_HAL_DUBHE_TOP_ADDR + 0x0400)
#define _HAL_DUBHE_CTRL_RESET_ADDR (_HAL_DUBHE_TOP_ADDR + 0x04)

#define _HAL_DUBHE_OTP_SRAM_SIMU_CLEAN                                         \
    do {                                                                       \
        *(volatile uint32_t *)(_HAL_DUBHE_OTP_MANAGEMENT_ADDR) |= (0x1 << 16); \
        *(volatile uint32_t *)(_HAL_DUBHE_OTP_MANAGEMENT_ADDR) &=              \
            (~(0x1 << 16));                                                    \
        *(volatile uint32_t *)(_HAL_DUBHE_CTRL_RESET_ADDR) |= (0x1 << 3);      \
        *(volatile uint32_t *)(_HAL_DUBHE_CTRL_RESET_ADDR) &= (~(0x1 << 3));   \
    } while (0)

static int _g_otp_state = 0;

/****************************************************/
/* dubhe start */
/****************************************************/
#define OTP_ASSERT_MSG(_x, fmt, ...)                                           \
    do {                                                                       \
        if (!(_x)) {                                                           \
            PAL_LOG_ERR("OTP ASSERT FAILURE:\n");                              \
            PAL_LOG_ERR("%s (%d): %s\n", __func__, __LINE__, __FUNCTION__);    \
            PAL_LOG_ERR(fmt, ##__VA_ARGS__);                                   \
            while (1) /* loop */                                               \
                ;                                                              \
        }                                                                      \
    } while (0)

/** Compute offset of a field within a structure. */
#define otp_offset_of(structure, member)                                       \
    ((hal_addr_t) & (((structure *)0)->member))

#define _GET_IPSS_LCS_TYPE(__lcs_buf__) ((__lcs_buf__)[0])

#define _SET_IPSS_LCS_TYPE(__lcs_buf__, __type__)                              \
    do {                                                                       \
        ((__lcs_buf__)[0]) = (__type__);                                       \
    } while (0)

#define _IPSS_OTP_RSVD_SIZE (16)
#define _IPSS_OTP_USER_NON_SECURE_SIZE (128)
#define _IPSS_OTP_USER_SECURE_SIZE (512)
typedef struct _ipss_otp_layout_t {
    uint32_t model_id[HAL_OTP_MODEL_ID_SIZE / 4];
    uint32_t model_key[HAL_OTP_MODEL_KEY_SIZE / 4];
    uint32_t device_id[HAL_OTP_DEVICE_ID_SIZE / 4];
    uint32_t device_root_key[HAL_OTP_DEVICE_ROOT_KEY_SIZE / 4];
    uint32_t sec_boot_pubkey_hash[HAL_OTP_SEC_BOOT_PUBKEY_HASH_SIZE / 4];
    uint32_t sec_dbg_pubkey_hash[HAL_OTP_SEC_DBG_PUBKEY_HASH_SIZE / 4];
    uint32_t lcs[HAL_OTP_LCS_SIZE / 4];
    uint32_t rsvd[_IPSS_OTP_RSVD_SIZE / 4];
    uint32_t lock_ctrl[HAL_OTP_LOCK_CTRL_SIZE / 4];
    uint32_t user_non_secure[_IPSS_OTP_USER_NON_SECURE_SIZE / 4];
    uint32_t user_secure[_IPSS_OTP_USER_SECURE_SIZE / 4];
} ipss_otp_layout_t;

typedef enum _ipss_lcs_t {
    IPSS_LCS_CM_DATA = 0,
    IPSS_LCS_DM_DATA = 1,
    IPSS_LCS_DD_DATA = 3,
    IPSS_LCS_DR_DATA = 7,
} ipss_lcs_t;

#define WORD_SIZE (4U)
static inline int __dubhe_read_otp(mbedtls_otp_info_t type,
                                   void *output,
                                   size_t size,
                                   size_t offset)
{
    int ret = 0;
    size_t word_align_size = 0;
    size_t word_align_offset = 0;
    uint8_t *buffer = NULL;

    if ((!IS_ALIGNED(size, WORD_SIZE)) ||
        (!IS_ALIGNED(offset, WORD_SIZE))) {
        word_align_offset = ROUND_DOWN(offset, WORD_SIZE);
        word_align_size = ROUND_UP(offset + size, WORD_SIZE) - word_align_offset;
        buffer = (uint8_t *)pal_malloc(word_align_size);
        HAL_CHECK_CONDITION(buffer,
                            HAL_ERR_OUT_OF_MEMORY,
                            "Malloc %d out of memory!\n",
                            word_align_size);
        ret = mbedtls_get_otp_info_ex(type, buffer, word_align_size, word_align_offset);
        HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,
                            "mbedtls_get_otp_info_ex failed: -0x%x\n", -ret);
        pal_memcpy(output, buffer + (offset & (WORD_SIZE - 1)), size);
    } else {
        ret = mbedtls_get_otp_info_ex(type, output, size, offset);
        HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,
                            "mbedtls_get_otp_info_ex failed: -0x%x\n", -ret);
    }

finish:
    if (buffer != NULL) {
        pal_memset(buffer, 0, word_align_size);
        pal_free(buffer);
    }
    return ret;
}

static inline int __dubhe_write_otp(mbedtls_otp_info_t type,
                                    const void *input,
                                    size_t size,
                                    size_t offset)
{
    int ret = 0;
    size_t word_align_size = 0;
    size_t word_align_offset = 0;
    uint8_t *buffer = NULL;

    if ((!IS_ALIGNED(size, WORD_SIZE)) ||
        (!IS_ALIGNED(offset, WORD_SIZE))) {
        word_align_offset = ROUND_DOWN(offset, WORD_SIZE);
        word_align_size = ROUND_UP(offset + size, WORD_SIZE) - word_align_offset;
        buffer = (uint8_t *)pal_malloc(word_align_size);
        HAL_CHECK_CONDITION(buffer,
                            HAL_ERR_OUT_OF_MEMORY,
                            "Malloc %d out of memory!\n",
                            word_align_size);
        ret = mbedtls_get_otp_info_ex(type, buffer, word_align_size, word_align_offset);
        HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,
                            "mbedtls_get_otp_info_ex failed: -0x%x\n", -ret);
        pal_memcpy(buffer + (offset & (WORD_SIZE - 1)), input, size);
        ret = mbedtls_set_otp_info_ex(type, buffer, word_align_size, word_align_offset);
        HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,
                            "mbedtls_set_otp_info_ex failed: -0x%x\n", -ret);
    } else {
        ret = mbedtls_set_otp_info_ex(type, input, size, offset);
        HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,
                            "mbedtls_set_otp_info_ex failed: -0x%x\n", -ret);
    }

finish:
    if (buffer != NULL) {
        pal_memset(buffer, 0, word_align_size);
        pal_free(buffer);
    }
    return ret;
}
#undef WORD_SIZE

#define __DUBHE_READ_OTP(__dubhe_otp_type__, __dubhe_otp_data__,               \
                            __otp_size__, __offset__)                          \
    ret = __dubhe_read_otp((__dubhe_otp_type__), (__dubhe_otp_data__),         \
                               (__otp_size__), (__offset__));                  \
    HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,                      \
                        "__dubhe_read_otp failed: -0x%x\n", -ret);

#define __DUBHE_WRITE_OTP(__dubhe_otp_type__, __dubhe_otp_data__,              \
                             __otp_size__, __offset__)                         \
    ret = __dubhe_write_otp((__dubhe_otp_type__),                              \
                               (const uint32_t *)(__dubhe_otp_data__),         \
                               (__otp_size__), (__offset__));                  \
    HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,                      \
                        "__dubhe_write_otp failed: -0x%x\n", -ret);

#ifdef DBH_OTP_PUF_SUPPORT

#define __DUBHE_PUF_WRITE_DEVICE_ROOT_KEY                                      \
    ret = mbedtls_otp_puf_enrollment();                                        \
    HAL_CHECK_CONDITION(0 == ret, HAL_ERR_INTERNAL_FATAL,                      \
                        "mbedtls_otp_puf_enrollment failed: -0x%x\n", -ret);
#endif /* DBH_OTP_PUF_SUPPORT */

static inline void _convert_otp_data(uint8_t *old_data,
                                     uint8_t *new_data,
                                     size_t size)
{
    for (int i = 0; i < size; i++)
        new_data[i] = ~old_data[i];
}

static hal_ret_t _dubhe_write_lcs(uint32_t *target_lcs)
{
    hal_ret_t ret                            = HAL_OK;
    uint32_t cur_lcs[HAL_OTP_LCS_SIZE / 4]   = {0};
    uint32_t write_lcs[HAL_OTP_LCS_SIZE / 4] = {0};

    pal_memset(cur_lcs, 0, HAL_OTP_LCS_SIZE);
    __DUBHE_READ_OTP(LIFE_CYCLE, cur_lcs, HAL_OTP_LCS_SIZE, 0);

    HAL_CHECK_CONDITION(
        (_GET_IPSS_LCS_TYPE(target_lcs) == IPSS_LCS_CM_DATA) ||
            (_GET_IPSS_LCS_TYPE(target_lcs) == IPSS_LCS_DM_DATA) ||
            (_GET_IPSS_LCS_TYPE(target_lcs) == IPSS_LCS_DD_DATA) ||
            (_GET_IPSS_LCS_TYPE(target_lcs) == IPSS_LCS_DR_DATA),
        HAL_ERR_GENERIC, "Write invalid LCS: 0x%x\n",
        _GET_IPSS_LCS_TYPE(target_lcs));

    if (_GET_IPSS_LCS_TYPE(cur_lcs) > _GET_IPSS_LCS_TYPE(target_lcs)) {
        PAL_LOG_ERR("Current LCS (%d) > Target LCS (%d)!\n",
                    _GET_IPSS_LCS_TYPE(cur_lcs),
                    _GET_IPSS_LCS_TYPE(target_lcs));
        ret = HAL_ERR_INTERNAL_FATAL;
        goto finish;
    }

    while (_GET_IPSS_LCS_TYPE(cur_lcs) != _GET_IPSS_LCS_TYPE(target_lcs)) {
        switch (_GET_IPSS_LCS_TYPE(cur_lcs)) {
        case IPSS_LCS_CM_DATA: {
            /* advance to DM */
            pal_memset(write_lcs, 0, HAL_OTP_LCS_SIZE);
            _SET_IPSS_LCS_TYPE(write_lcs, IPSS_LCS_DM_DATA);
            __DUBHE_WRITE_OTP(LIFE_CYCLE, write_lcs, HAL_OTP_LCS_SIZE, 0);
            ret = HAL_OK;
            break;
        }
        case IPSS_LCS_DM_DATA: {
            /* advance to DD */
            pal_memset(write_lcs, 0, HAL_OTP_LCS_SIZE);
            _SET_IPSS_LCS_TYPE(write_lcs, IPSS_LCS_DD_DATA);
            __DUBHE_WRITE_OTP(LIFE_CYCLE, write_lcs, HAL_OTP_LCS_SIZE, 0);
            ret = HAL_OK;
            break;
        }
        case IPSS_LCS_DD_DATA: {
            /* advance to DR */
            pal_memset(write_lcs, 0, HAL_OTP_LCS_SIZE);
            _SET_IPSS_LCS_TYPE(write_lcs, IPSS_LCS_DR_DATA);
            __DUBHE_WRITE_OTP(LIFE_CYCLE, write_lcs, HAL_OTP_LCS_SIZE, 0);
            break;
        }
        case IPSS_LCS_DR_DATA: {
            PAL_LOG_ERR("Invalid Current LCS : 0x%x(DR)\n",
                        _GET_IPSS_LCS_TYPE(cur_lcs));
            ret = HAL_ERR_INTERNAL_FATAL;
            goto finish;
            break;
        }
        default: {
            PAL_LOG_ERR("Invalid Current LCS data: 0x%x\n",
                        _GET_IPSS_LCS_TYPE(cur_lcs));
            ret = HAL_ERR_INTERNAL_FATAL;
            goto finish;
            break;
        }
        }
        if (ret != HAL_OK) {
            goto finish;
        }
        pal_memset(cur_lcs, 0, HAL_OTP_LCS_SIZE);
        __DUBHE_READ_OTP(LIFE_CYCLE, cur_lcs, HAL_OTP_LCS_SIZE, 0);
    }

finish:
    return ret;
}

static hal_ret_t _check_dubhe_otp_data_blank(void)
{
    hal_ret_t ret                    = HAL_OK;
    ipss_otp_layout_t dubhe_otp_data = {0};
    ipss_otp_layout_t zero_data      = {0};

    PAL_LOG_DEBUG("Check OTP data in dubhe...\n");

    pal_memset(&dubhe_otp_data, 0, sizeof(ipss_otp_layout_t));
    pal_memset(&zero_data, 0, sizeof(ipss_otp_layout_t));

    /* check if LCS is in CM */
    __DUBHE_READ_OTP(LIFE_CYCLE, dubhe_otp_data.lcs, HAL_OTP_LCS_SIZE, 0);

    /* If LCS is not in CM, we think the OTP is already configured, just
     * bypass OTP black checking to improve performance
     **/
    if (_GET_IPSS_LCS_TYPE(dubhe_otp_data.lcs) != IPSS_LCS_CM_DATA) {
        PAL_LOG_DEBUG("dubhe otp life cycle: 0x%x\n",
                    _GET_IPSS_LCS_TYPE(dubhe_otp_data.lcs));
        return HAL_OK;
    }

    /* Read out all the otp in dubhe and check if they are zero */
    pal_memset(&dubhe_otp_data, 0, sizeof(ipss_otp_layout_t));
    __DUBHE_READ_OTP(MODEL_ID, dubhe_otp_data.model_id, HAL_OTP_MODEL_ID_SIZE, 0);
    __DUBHE_READ_OTP(MODEL_KEY, dubhe_otp_data.model_key,
                     HAL_OTP_MODEL_KEY_SIZE, 0);
    __DUBHE_READ_OTP(DEV_ID, dubhe_otp_data.device_id, HAL_OTP_DEVICE_ID_SIZE, 0);
    __DUBHE_READ_OTP(DEV_ROOT_KEY,
                     dubhe_otp_data.device_root_key,
                     HAL_OTP_DEVICE_ROOT_KEY_SIZE, 0);
    __DUBHE_READ_OTP(SEC_BT_PK_HASH,
                     dubhe_otp_data.sec_boot_pubkey_hash,
                     HAL_OTP_SEC_BOOT_PUBKEY_HASH_SIZE, 0);
    __DUBHE_READ_OTP(SEC_DBG_PK_HASH,
                     dubhe_otp_data.sec_dbg_pubkey_hash,
                     HAL_OTP_SEC_DBG_PUBKEY_HASH_SIZE, 0);
    __DUBHE_READ_OTP(LIFE_CYCLE, dubhe_otp_data.lcs, HAL_OTP_LCS_SIZE, 0);
    __DUBHE_READ_OTP(LOCK_CTRL, dubhe_otp_data.lock_ctrl,
                     HAL_OTP_LOCK_CTRL_SIZE, 0);
    __DUBHE_READ_OTP(USR_NON_SEC_REGION,
                     dubhe_otp_data.user_non_secure,
                     _IPSS_OTP_USER_NON_SECURE_SIZE, 0);
    __DUBHE_READ_OTP(USR_SEC_REGION, dubhe_otp_data.user_secure,
                     _IPSS_OTP_USER_SECURE_SIZE, 0);

    /* compare with zero */
#ifdef CONFIG_SIMU_OTP_IN_FLASH
    if (0 !=
        pal_memcmp(&dubhe_otp_data, &zero_data, sizeof(ipss_otp_layout_t))) {
        PAL_LOG_ERR("OTP Data in dubhe are not 0!\n");
        PAL_LOG_DUMP_BUF("dubhe OTP:", &dubhe_otp_data,
                         sizeof(ipss_otp_layout_t));
        ret = HAL_ERR_INTERNAL_FATAL;
        goto finish;
    } 
#else
    if (0 !=
        pal_memcmp(&dubhe_otp_data, &zero_data, sizeof(ipss_otp_layout_t))) {
        PAL_LOG_DEBUG("OTP Data in dubhe are not 0!\n");
        PAL_LOG_DUMP_BUF("dubhe OTP:", &dubhe_otp_data, sizeof(ipss_otp_layout_t));
        ret = HAL_OK;
        goto finish;
    } else {
        pal_critical_err(CRITICAL_ERR_OTP_BLANK);
        PAL_LOG_ERR("OTP Data in dubhe are 0!\n");
        ret = HAL_ERR_INTERNAL_FATAL;
        goto finish;
    }
#endif

    PAL_LOG_INFO("OTP data in dubhe are blank.\n");
    ret = HAL_OK;
finish:
    return ret;
}

#ifdef CONFIG_SIMU_OTP_IN_FLASH
static hal_ret_t _update_dubhe_otp_data(void)
{
    hal_ret_t ret                     = HAL_OK;
    ipss_otp_layout_t zero_data       = {0};
    ipss_otp_layout_t flash_simu_data = {0};
#ifdef DBH_OTP_PUF_SUPPORT
    uint32_t puf_device_root_key[HAL_OTP_DEVICE_ROOT_KEY_SIZE / 4];
#endif

    PAL_LOG_INFO("Start to restore OTP from flash to dubhe...\n");

    pal_memset(&zero_data, 0, sizeof(ipss_otp_layout_t));

    /* read OTP data from flash */
    ret = flash_op_read((uint32_t)_HAL_OTP_BASE_ADDR + 0,
                        (uint8_t *)(&flash_simu_data),
                        sizeof(ipss_otp_layout_t));
    HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_INTERNAL_FATAL,
                        "flash read otp failed!\n");
    _convert_otp_data((uint8_t *)(&flash_simu_data),
                      (uint8_t *)(&flash_simu_data),
                      sizeof(ipss_otp_layout_t));

    /* check OTP in flash is valid */
    HAL_CHECK_CONDITION(((flash_simu_data.lcs[0] == IPSS_LCS_CM_DATA) ||
                         (flash_simu_data.lcs[0] == IPSS_LCS_DM_DATA) ||
                         (flash_simu_data.lcs[0] == IPSS_LCS_DD_DATA) ||
                         (flash_simu_data.lcs[0] == IPSS_LCS_DR_DATA)),
                        HAL_ERR_INTERNAL_FATAL,
                        "Invalid LCS in flash simulated OTP: 0x%x!\n",
                        flash_simu_data.lcs[0]);

    /* check if data in flash are also all zero */
    pal_memset(&zero_data, 0, sizeof(ipss_otp_layout_t));
    if (0 ==
        pal_memcmp(&zero_data, &flash_simu_data, sizeof(ipss_otp_layout_t))) {
        PAL_LOG_DEBUG("OTP in flash simulation are all 0!\n");
        PAL_LOG_DEBUG("Need provisioning!\n");
        ret = HAL_OK;
        goto finish;
    }

    /* flash in simulated otp are not 0, do update */
#ifdef DBH_OTP_PUF_SUPPORT
    /**
     * Note: When puf is enabled, the mbedtls_set_otp_info interface can't write
     * device root key. But here we MUST restore the device root key from flash.
     *
     * Note: When puf and scramble are enabled at the same time, the device ROOT
     * key MUST be written first before any other OTP area. And in the dubhe
     * driver, the device ROOT key is also locked after writing.
     *
     * From Jason we know that currently on MPS2+ FPGA board, the
     * mbedtls_otp_puf_enrollment interface will set the SAME device root key to
     * Dubhe OTP area (because of the hardcoding logic in RTL). So here we call
     * puf interface again to restore the device root key, and read and compare
     * it with in flash.
     *
     */
#ifdef DBH_OTP_SCRAMBLE
    __DUBHE_PUF_WRITE_DEVICE_ROOT_KEY;
    __DUBHE_READ_OTP(DEV_ROOT_KEY, puf_device_root_key,
                     HAL_OTP_DEVICE_ROOT_KEY_SIZE, 0);
    /* make sure OTP in dubhe sram and flash are same */
    OTP_ASSERT_MSG(0 == pal_memcmp(puf_device_root_key,
                                   flash_simu_data.device_root_key,
                                   HAL_OTP_DEVICE_ROOT_KEY_SIZE),
                   "Restore Device Root Key failed! PUF data is not constant "
                   "cross different powercycle!\n");
#else   /* END DBH_OTP_SCRAMBLE enabled */
    if (_IS_DEVICE_ROOT_KEY_LOCK(flash_simu_data.lock_ctrl)) {
        __DUBHE_PUF_WRITE_DEVICE_ROOT_KEY;
        __DUBHE_READ_OTP(DEV_ROOT_KEY, puf_device_root_key,
                        HAL_OTP_DEVICE_ROOT_KEY_SIZE, 0);
        /* make sure OTP in dubhe sram and flash are same */
        OTP_ASSERT_MSG(0 == pal_memcmp(puf_device_root_key,
                                    flash_simu_data.device_root_key,
                                    HAL_OTP_DEVICE_ROOT_KEY_SIZE),
                    "Restore Device Root Key failed! PUF data is not constant "
                    "cross different powercycle!\n");
    }
#endif /* END DBH_OTP_SCRAMBLE disabled */
#else  /* END DBH_OTP_PUF_SUPPORT */
    __DUBHE_WRITE_OTP(DEV_ROOT_KEY,
                      flash_simu_data.device_root_key,
                      HAL_OTP_DEVICE_ROOT_KEY_SIZE, 0);
#endif
    __DUBHE_WRITE_OTP(MODEL_ID, flash_simu_data.model_id,
                      HAL_OTP_MODEL_ID_SIZE, 0);
    __DUBHE_WRITE_OTP(MODEL_KEY, flash_simu_data.model_key,
                      HAL_OTP_MODEL_KEY_SIZE, 0);
    __DUBHE_WRITE_OTP(DEV_ID, flash_simu_data.device_id,
                      HAL_OTP_DEVICE_ID_SIZE, 0);

    __DUBHE_WRITE_OTP(SEC_BT_PK_HASH,
                      flash_simu_data.sec_boot_pubkey_hash,
                      HAL_OTP_SEC_BOOT_PUBKEY_HASH_SIZE, 0);
    __DUBHE_WRITE_OTP(SEC_DBG_PK_HASH,
                      flash_simu_data.sec_dbg_pubkey_hash,
                      HAL_OTP_SEC_DBG_PUBKEY_HASH_SIZE, 0);
    __DUBHE_WRITE_OTP(USR_NON_SEC_REGION,
                      flash_simu_data.user_non_secure,
                      _IPSS_OTP_USER_NON_SECURE_SIZE, 0);
    __DUBHE_WRITE_OTP(USR_SEC_REGION,
                      flash_simu_data.user_secure,
                      _IPSS_OTP_USER_SECURE_SIZE, 0);
    __DUBHE_WRITE_OTP(LOCK_CTRL, flash_simu_data.lock_ctrl,
                      HAL_OTP_LOCK_CTRL_SIZE, 0);

    /* always write lcs at the last */
    ret = _dubhe_write_lcs(flash_simu_data.lcs);
    HAL_CHECK_RET("Write dubhe LCS failed!\n");

    PAL_LOG_INFO("Restore OTP to dubhe done!\n");

    ret = HAL_OK;
finish:
    return ret;
}
#endif

static hal_ret_t dubhe_otp_init(void)
{
    hal_ret_t ret = HAL_OK;

#ifdef CONFIG_SIMU_OTP_IN_FLASH
    /* clean otp data in dubhe sram simulated */
    _HAL_DUBHE_OTP_SRAM_SIMU_CLEAN;
#endif

    /* check that otp in dubhe are all zero */
    ret = _check_dubhe_otp_data_blank();
    HAL_CHECK_RET("Check dubhe OTP failed!\n");

#ifdef CONFIG_SIMU_OTP_IN_FLASH
    /* update otp data */
    ret = _update_dubhe_otp_data();
    HAL_CHECK_RET("Update dubhe OTP data failed!\n");
#else
    PAL_LOG_DEBUG("Not restore OTP from flash\n");
#endif
    ret = HAL_OK;
finish:
    return ret;
}

#define __IS_USER_NON_SECURE(__offset__, __size__)                             \
    (((__offset__) >= otp_offset_of(ipss_otp_layout_t, user_non_secure)) &&    \
     ((__size__) <= _IPSS_OTP_USER_NON_SECURE_SIZE) &&                         \
     ((__offset__) < otp_offset_of(ipss_otp_layout_t, user_non_secure) +       \
                         _IPSS_OTP_USER_NON_SECURE_SIZE) &&                    \
     ((__offset__) + size <=                                                   \
      otp_offset_of(ipss_otp_layout_t, user_non_secure) +                      \
          _IPSS_OTP_USER_NON_SECURE_SIZE))

#define __IS_USER_SECURE(__offset__, __size__)                                 \
    (((__offset__) >= otp_offset_of(ipss_otp_layout_t, user_secure)) &&        \
     ((__size__) <= _IPSS_OTP_USER_SECURE_SIZE) &&                             \
     ((__offset__) < otp_offset_of(ipss_otp_layout_t, user_secure) +           \
                         _IPSS_OTP_USER_SECURE_SIZE) &&                        \
     ((__offset__) + size <= otp_offset_of(ipss_otp_layout_t, user_secure) +   \
                                 _IPSS_OTP_USER_SECURE_SIZE))

static inline hal_ret_t __convert_otp_offset_size_to_dubhe_type(
    hal_addr_t offset, size_t size, mbedtls_otp_info_t *type)
{
    if ((offset == otp_offset_of(ipss_otp_layout_t, model_id)) &&
        (size == HAL_OTP_MODEL_ID_SIZE)) {
        *type = MODEL_ID;
    } else if ((offset == otp_offset_of(ipss_otp_layout_t, model_key)) &&
               (size == HAL_OTP_MODEL_KEY_SIZE)) {
        *type = MODEL_KEY;
    } else if ((offset == otp_offset_of(ipss_otp_layout_t, device_id)) &&
               (size == HAL_OTP_DEVICE_ID_SIZE)) {
        *type = DEV_ID;
    } else if ((offset == otp_offset_of(ipss_otp_layout_t, device_root_key)) &&
               (size == HAL_OTP_DEVICE_ROOT_KEY_SIZE)) {
        *type = DEV_ROOT_KEY;
    } else if ((offset ==
                otp_offset_of(ipss_otp_layout_t, sec_boot_pubkey_hash)) &&
               (size == HAL_OTP_SEC_BOOT_PUBKEY_HASH_SIZE)) {
        *type = SEC_BT_PK_HASH;
    } else if ((offset ==
                otp_offset_of(ipss_otp_layout_t, sec_dbg_pubkey_hash)) &&
               (size == HAL_OTP_SEC_DBG_PUBKEY_HASH_SIZE)) {
        *type = SEC_DBG_PK_HASH;
    } else if ((offset == otp_offset_of(ipss_otp_layout_t, lcs)) &&
               (size == HAL_OTP_LCS_SIZE)) {
        *type = LIFE_CYCLE;
    } else if ((offset == otp_offset_of(ipss_otp_layout_t, lock_ctrl)) &&
               (size == HAL_OTP_LOCK_CTRL_SIZE)) {
        *type = LOCK_CTRL;
    } else {
        PAL_LOG_ERR("Invalid OTP offset: 0x%x, size: 0x%x\n", offset, size);
        return HAL_ERR_INTERNAL_FATAL;
    }
    return HAL_OK;
}

static hal_ret_t dubhe_otp_read(hal_addr_t offset, uint8_t *data, size_t size)
{
    hal_ret_t ret                         = HAL_OK;
    mbedtls_otp_info_t type               = 0;
#ifdef CONFIG_SIMU_OTP_IN_FLASH
    ipss_otp_layout_t flash_simu_otp_data = {0};
#endif

    /* 1. call dubhe to read OTP */

    /* Check if we are reading user-nonsecure */
    if (__IS_USER_NON_SECURE(offset, size)) {
        __DUBHE_READ_OTP(USR_NON_SEC_REGION,
                             data, size,
                     offset - otp_offset_of(ipss_otp_layout_t, user_non_secure));
        /* Check if we are reading user-secure */
    } else if (__IS_USER_SECURE(offset, size)) {
        __DUBHE_READ_OTP(USR_SEC_REGION,
                             data, size,
                     offset - otp_offset_of(ipss_otp_layout_t, user_secure));
    } else {
        ret = __convert_otp_offset_size_to_dubhe_type(offset, size, &type);
        HAL_CHECK_RET("OTP Read with offset/size: 0x%x/0x%x doesn't match any "
                      "OTP bits in dubhe!\n",
                      offset,
                      size);
        switch (type) {
        case MODEL_ID: {
            __DUBHE_READ_OTP(MODEL_ID,
                                data, size,
                        offset - otp_offset_of(ipss_otp_layout_t, model_id));
            break;
        }
        case MODEL_KEY: {
            __DUBHE_READ_OTP(MODEL_KEY, data, size,
                        offset - otp_offset_of(ipss_otp_layout_t, model_key));
            break;
        }
        case DEV_ID: {
            __DUBHE_READ_OTP(DEV_ID, data, size,
                        offset - otp_offset_of(ipss_otp_layout_t, device_id));
            break;
        }
        case DEV_ROOT_KEY: {
            __DUBHE_READ_OTP(DEV_ROOT_KEY, data, size,
                  offset - otp_offset_of(ipss_otp_layout_t, device_root_key));
            break;
        }
        case SEC_BT_PK_HASH: {
            __DUBHE_READ_OTP(SEC_BT_PK_HASH, data, size,
                offset - otp_offset_of(ipss_otp_layout_t, sec_boot_pubkey_hash));
            break;
        }
        case SEC_DBG_PK_HASH: {
            __DUBHE_READ_OTP(SEC_DBG_PK_HASH, data, size,
                offset - otp_offset_of(ipss_otp_layout_t, sec_dbg_pubkey_hash));
            break;
        }
        case LIFE_CYCLE: {
            __DUBHE_READ_OTP(LIFE_CYCLE, data, size,
                    offset - otp_offset_of(ipss_otp_layout_t, lcs));
            break;
        }
        case LOCK_CTRL: {
            __DUBHE_READ_OTP(LOCK_CTRL,data, size,
                    offset - otp_offset_of(ipss_otp_layout_t, lock_ctrl));
            break;
        }
        default: {
            OTP_ASSERT_MSG(0, "Bad dubhe OTP type: %d\n", type);
            break;
        }
        }
    }

	PAL_LOG_DEBUG("read otp, offset=%x\n", offset);
	PAL_LOG_DUMP_BUF("read otp: \n", data, size);

#ifdef CONFIG_SIMU_OTP_IN_FLASH
    /* 2. call flash simulation to read OTP */

    ret = flash_op_read((uint32_t)_HAL_OTP_BASE_ADDR + offset,
                        (uint8_t *)(&flash_simu_otp_data) + offset,
                        size);
    HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_INTERNAL_FATAL,
                        "flash read OTP failed!\n");
    _convert_otp_data((uint8_t *)(&flash_simu_otp_data) + offset,
                      (uint8_t *)(&flash_simu_otp_data) + offset,
                      size);

    /* compare the data */
    if (0 != pal_memcmp(data,
                        (uint8_t *)(&flash_simu_otp_data) + offset,
                        size)) {
        PAL_LOG_ERR(
            "OTP data in dubhe and flash simulation are NOT consistent!\n");
        PAL_LOG_DUMP_BUF("In dubhe:", data, size);
        PAL_LOG_DUMP_BUF("In flash:",
                         (uint8_t *)(&flash_simu_otp_data) + offset,
                         size);
        ret = HAL_ERR_INTERNAL_FATAL;
        goto finish;
    }

    /* Success, Out put to user */
    pal_memcpy(data, (uint8_t *)(&flash_simu_otp_data) + offset, size);
#endif

finish:
    return ret;
}

/* Check that we are writing from 0 to 1 */
static inline bool _check_otp_write_rule(uint8_t *old_data,
                                         uint8_t *new_data,
                                         size_t size)
{
    int i, j;

    for (i = 0; i < size; i++) {
        for (j = 0; j < 8; j++) {
            /* are we writing from 1 to 0 ? */
            if ((old_data[i] & (1 << j)) && (!(new_data[i] & (1 << j)))) {
                PAL_LOG_ERR("Invalid OTP write: 0x%x --> 0x%x\n",
                            old_data[i],
                            new_data[i]);
                return false;
            }
        }
    }
    return true;
}

#ifdef DBH_OTP_PUF_SUPPORT
static hal_ret_t dubhe_otp_puf_enroll(void)
{
    hal_ret_t ret              = HAL_OK;
    ipss_otp_layout_t otp_data = {0};

    PAL_LOG_INFO("PUF init device root key!\n");

        /* 1.1 write dubhe otp */
        __DUBHE_PUF_WRITE_DEVICE_ROOT_KEY;

        /* 1.2 read device root key for flash read */
        __DUBHE_READ_OTP(DEV_ROOT_KEY,
                         otp_data.device_root_key,
                         HAL_OTP_DEVICE_ROOT_KEY_SIZE, 0);

        /* 1.3 read lock ctrl again, for updating lock ctrl latter in flash */
        __DUBHE_READ_OTP(LOCK_CTRL, otp_data.lock_ctrl,
                            HAL_OTP_LOCK_CTRL_SIZE, 0);
        HAL_CHECK_CONDITION(_IS_DEVICE_ROOT_KEY_LOCK(otp_data.lock_ctrl),
                            HAL_ERR_GENERIC,
                            "Device Root Key is not locked !\n");

        /* 1.4. Write flash with device root key */
        _convert_otp_data((uint8_t *)otp_data.device_root_key,
                          (uint8_t *)otp_data.device_root_key,
                          HAL_OTP_DEVICE_ROOT_KEY_SIZE);
        ret = flash_op_raw_write((uint32_t)_HAL_OTP_BASE_ADDR +
                                 HAL_OTP_DEVICE_ROOT_KEY_OFFSET,
                                 (const uint8_t *)(otp_data.device_root_key),
                                 HAL_OTP_DEVICE_ROOT_KEY_SIZE);
        HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                            "flash write Device Root Key failed!\n");
        /* 1.5. Also update device root key lock ctrl bit */
        _convert_otp_data((uint8_t *)otp_data.lock_ctrl,
                          (uint8_t *)otp_data.lock_ctrl,
                          HAL_OTP_LOCK_CTRL_SIZE);
        ret = flash_op_raw_write(
            (uint32_t)_HAL_OTP_BASE_ADDR + HAL_OTP_LOCK_CTRL_OFFSET,
            (const uint8_t *)(otp_data.lock_ctrl), HAL_OTP_LOCK_CTRL_SIZE);
        HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                            "flash update Device Root Key lock ctrl failed!\n");
finish:
    return ret;
}
#endif /* END DBH_OTP_PUF_SUPPORT */

static hal_ret_t dubhe_otp_write(hal_addr_t offset,
                                 const uint8_t *data,
                                 size_t size)
{
    hal_ret_t ret              = HAL_OK;
    ipss_otp_layout_t otp_data = {0};
    mbedtls_otp_info_t type    = 0;
#if defined(DBH_OTP_PUF_SUPPORT)
    uint32_t tmp_lock_ctrl_data[HAL_OTP_LOCK_CTRL_SIZE / sizeof(uint32_t)];
#endif
    uint8_t *p    = NULL;
    uint8_t *op_p = NULL;

    pal_memset(&otp_data, 0, sizeof(ipss_otp_layout_t));

    /* 1. always write device root key first if PUF enabled */
#ifdef DBH_OTP_PUF_SUPPORT
    /* Check device root key is written and locked */
    __DUBHE_READ_OTP(LOCK_CTRL, otp_data.lock_ctrl,
                        HAL_OTP_LOCK_CTRL_SIZE, 0);
#ifdef DBH_OTP_SCRAMBLE
    if (!_IS_DEVICE_ROOT_KEY_LOCK(otp_data.lock_ctrl)) {
        /* device root key is not written, write device root key first */
        ret = dubhe_otp_puf_enroll();
        HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                            "dubhe otp puf enroll failed!\n");
        __DUBHE_READ_OTP(LOCK_CTRL, otp_data.lock_ctrl,
                         HAL_OTP_LOCK_CTRL_SIZE, 0);
    } else {
        /* device root key is written, skip */
    }
#else /* END DBH_OTP_SCRAMBLE */
#endif
    if (_IS_DEVICE_ROOT_KEY_LOCK(otp_data.lock_ctrl)) {
        /* skip write device root key */
        if ((offset == HAL_OTP_DEVICE_ROOT_KEY_OFFSET) &&
            (size == HAL_OTP_DEVICE_ROOT_KEY_SIZE)) {
            PAL_LOG_WARN("Skip write device root key!\n");
            return HAL_OK;
        } else if ((offset == HAL_OTP_LOCK_CTRL_OFFSET) &&
                   (size == HAL_OTP_LOCK_CTRL_SIZE)) {
            pal_memset(&otp_data, 0, sizeof(ipss_otp_layout_t));
            __DUBHE_READ_OTP(LOCK_CTRL, otp_data.lock_ctrl,
                                HAL_OTP_LOCK_CTRL_SIZE, 0);

            pal_memcpy(tmp_lock_ctrl_data, data, size);

            _SET_DEVICE_ROOT_KEY_LOCK(tmp_lock_ctrl_data);
            p = (uint8_t *)(tmp_lock_ctrl_data);
        } else {
            p = (uint8_t *)data;
        }
    } else {
        p = (uint8_t *)data;
    }
#else  /* END DBH_OTP_PUF_SUPPORT */
    p = (uint8_t *)data;
#endif

    /* 1. read OTP first */
    ret = dubhe_otp_read(offset, (uint8_t *)(&otp_data) + offset, size);
    HAL_CHECK_RET("dubhe read OTP failed!\n");

    /* 2. Check if we can write */
    HAL_CHECK_CONDITION(
        _check_otp_write_rule((uint8_t *)(&otp_data) + offset, p, size),
        HAL_ERR_BAD_PARAM,
        "Invalid OTP write data write: write from 1 to 0!\n");

    /* 3. Write dubhe */

    /* Check if we are writing user non-secure */
    if (__IS_USER_NON_SECURE(offset, size)) {
        __DUBHE_WRITE_OTP(USR_NON_SEC_REGION,
                             data,
                             size,
                             offset -
                               otp_offset_of(ipss_otp_layout_t, user_non_secure));

        /* Check if we are reading user-secure */
    } else if (__IS_USER_SECURE(offset, size)) {
        __DUBHE_WRITE_OTP(USR_SEC_REGION,
                             data,
                             size,
                             offset -
                               otp_offset_of(ipss_otp_layout_t, user_secure));
    } else {
        ret = __convert_otp_offset_size_to_dubhe_type(offset, size, &type);
        HAL_CHECK_RET("OTP Write with offset/size: 0x%x/0x%x doesn't match any "
                      "OTP bits in dubhe!\n",
                      offset,
                      size);

        switch (type) {
        case MODEL_ID: {
            __DUBHE_WRITE_OTP(MODEL_ID,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, model_id));
            break;
        }
        case MODEL_KEY: {
            __DUBHE_WRITE_OTP(MODEL_KEY,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, model_key));
            break;
        }
        case DEV_ID: {
            __DUBHE_WRITE_OTP(DEV_ID,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, device_id));
            break;
        }
        case DEV_ROOT_KEY: {
#ifdef DBH_OTP_PUF_SUPPORT
#ifdef DBH_OTP_SCRAMBLE
        OTP_ASSERT_MSG(0, "Can't write Device Root Key again!\n");
#else  /* END DBH_OTP_SCRAMBLE enabled */
        if (!_IS_DEVICE_ROOT_KEY_LOCK(otp_data.lock_ctrl)) {
            ret = dubhe_otp_puf_enroll();
            HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                                "dubhe otp puf enroll failed!\n");
        } else {
            OTP_ASSERT_MSG(0, "Can't write Device Root Key again!\n");
        }
        /* skip write device root key */
        if ((offset == HAL_OTP_DEVICE_ROOT_KEY_OFFSET) &&
            (size == HAL_OTP_DEVICE_ROOT_KEY_SIZE)) {
            PAL_LOG_WARN("Skip write device root key!\n");
            return HAL_OK;
        }
#endif /* END DBH_OTP_SCRAMBLE disabled */
#else  /* END DBH_OTP_PUF_SUPPORT enabled */
            __DUBHE_WRITE_OTP(DEV_ROOT_KEY,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, device_root_key));
#endif  /* END DBH_OTP_PUF_SUPPORT disabled */
            break;
        }
        case SEC_BT_PK_HASH: {
            __DUBHE_WRITE_OTP(SEC_BT_PK_HASH,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, sec_boot_pubkey_hash));
            break;
        }
        case SEC_DBG_PK_HASH: {
            __DUBHE_WRITE_OTP(SEC_DBG_PK_HASH,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, sec_dbg_pubkey_hash));
            break;
        }
        case LIFE_CYCLE: {
            pal_memcpy((uint8_t *)(&otp_data) + offset, p, size);
            ret = _dubhe_write_lcs(otp_data.lcs);
            HAL_CHECK_RET("dubhe write lcs failed!\n");
            break;
        }
        case LOCK_CTRL: {
            __DUBHE_WRITE_OTP(LOCK_CTRL,
                                 data,
                                 size,
                                 offset -
                                   otp_offset_of(ipss_otp_layout_t, lock_ctrl));
            break;
        }
        default: {
            OTP_ASSERT_MSG(0, "Bad dubhe OTP type: %d\n", type);
            break;
        }
        }
    }

#ifdef CONFIG_SIMU_OTP_IN_FLASH
    /* 4. Write flash */
    op_p = (uint8_t *)pal_malloc(size);
    HAL_CHECK_CONDITION(op_p,
                        HAL_ERR_OUT_OF_MEMORY,
                        "Malloc %zd out of memory!\n",
                        size);
    _convert_otp_data(p, op_p, size);
    ret = flash_op_raw_write((uint32_t)_HAL_OTP_BASE_ADDR + offset, op_p, size);
    HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                        "flash write OTP failed!\n");
#endif

    /* 5. read again */
    pal_memset(&otp_data, 0, sizeof(ipss_otp_layout_t));
    ret = dubhe_otp_read(offset, (uint8_t *)(&otp_data) + offset, size);
    HAL_CHECK_RET("dubhe read OTP failed!\n");

    /* 6. compare with our target data */
    if (0 != pal_memcmp((uint8_t *)(&otp_data) + offset, p, size)) {
        PAL_LOG_ERR("OTP data not match with our writen data!\n");
        PAL_LOG_DUMP_BUF("Read data:", (uint8_t *)(&otp_data) + offset, size);
        PAL_LOG_DUMP_BUF("Write data:", p, size);
        ret = HAL_ERR_INTERNAL_FATAL;
        goto finish;
    }

	PAL_LOG_DUMP_BUF("Write otp:", data, size);

finish:
    if (op_p) {
        pal_free(op_p);
    }
    return ret;
}

#define _DUBHE_GEN_PSK_PSK_SIZE (16)
#define _DUBHE_GEN_PSK_EK1_SIZE (16)
#define _DUBHE_GEN_PSK_EK2_SIZE (16)
#define _DUBHE_GEN_PSK_EK3_SIZE (16)
hal_ret_t dubhe_gen_psk(uint8_t *psk, size_t psk_size)
{
    hal_ret_t ret                        = HAL_OK;
    uint8_t ek1[_DUBHE_GEN_PSK_EK1_SIZE] = {0};
    uint8_t ek2[_DUBHE_GEN_PSK_EK2_SIZE] = {0};
    uint8_t ek3[_DUBHE_GEN_PSK_EK3_SIZE] = {0};
    mbedtls_aes_context aes_ctx          = {0};
    mbedtls_aes_sec_key_t seckey         = {0};
    int32_t i                            = 0;

    if (psk_size != _DUBHE_GEN_PSK_EK3_SIZE) {
        PAL_LOG_ERR("PSK size: %d not supported!\n", psk_size);
        return HAL_ERR_BAD_PARAM;
    }

    /* Init ek1, ek2, ek3 */
    for (i = 0; i < _DUBHE_GEN_PSK_EK1_SIZE; i++) {
        ek1[i] = (i * 123) & 0xff;
    }
    for (i = 0; i < _DUBHE_GEN_PSK_EK2_SIZE; i++) {
        ek2[i] = (i * 456) & 0xff;
    }
    for (i = 0; i < _DUBHE_GEN_PSK_EK3_SIZE; i++) {
        ek3[i] = (i * 789) & 0xff;
    }

    /* init mbedtls context */
    mbedtls_aes_init(&aes_ctx);

    /* call keyladder to derive key */
    seckey.sel = MBEDTLS_AES_KL_KEY_ROOT;
    seckey.ek1bits = _DUBHE_GEN_PSK_EK1_SIZE * 8;
    pal_memcpy(seckey.ek1, ek1, sizeof(ek1));
    pal_memcpy(seckey.ek2, ek2, sizeof(ek2));
    pal_memcpy(seckey.ek3, ek3, sizeof(ek3));
    ret = mbedtls_aes_setseckey_dec(&aes_ctx, &seckey);
    HAL_CHECK_CONDITION(0 == ret,
                        HAL_ERR_INTERNAL_FATAL,
                        "mbedtls_aes_setseckey_dec failed: -0x%x\n",
                        -ret);

    pal_memset(ek3, 0x5a, _DUBHE_GEN_PSK_EK3_SIZE);

    /* do decrypt */
    ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, ek3, psk);
    HAL_CHECK_CONDITION(0 == ret,
                        HAL_ERR_INTERNAL_FATAL,
                        "mbedtls_aes_crypt_ecb failed: -0x%x\n",
                        -ret);

    ret = HAL_OK;
finish:
    mbedtls_aes_free(&aes_ctx);
    pal_memset(ek1, 0, _DUBHE_GEN_PSK_EK1_SIZE);
    pal_memset(ek2, 0, _DUBHE_GEN_PSK_EK2_SIZE);
    pal_memset(ek3, 0, _DUBHE_GEN_PSK_EK3_SIZE);
    pal_memset(&seckey, 0, sizeof(seckey));

    return ret;
}
/****************************************************/
/* dubhe start */
/****************************************************/

/**
 * @ingroup hal_otp
 * @brief   hal_otp_init function
 */

hal_ret_t hal_otp_init(void)
{
    hal_ret_t ret = HAL_OK;

    if (0 == _g_otp_state) {
        ret = dubhe_otp_init();
        OTP_ASSERT_MSG(HAL_OK == ret, "Dubhe OTP init failed!\n");
    }

    _g_otp_state++;
    return HAL_OK;
}

/**
 * @ingroup hal_otp
 * @brief   hal_otp_cleanup function
 */

void hal_otp_cleanup(void)
{
    if (_g_otp_state > 0) {
        _g_otp_state--;
    }
    return;
}

hal_ret_t hal_otp_read(hal_addr_t offset, uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION(_g_otp_state > 0,
                        HAL_ERR_GENERIC,
                        "Invalid hal_otp calling sequence, not init!\n");

    HAL_CHECK_CONDITION(data && size, HAL_ERR_BAD_PARAM,
                        "Parameter data/size is 0!\n");

    ret = dubhe_otp_read(offset, data, size);
    if (ret != HAL_OK) {
        pal_critical_err(offset + CRITICAL_ERR_OTP_READ);
    }
    HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_INTERNAL_FATAL,
                        "dubhe read OTP failed!\n");

finish:
    return ret;
}

hal_ret_t hal_otp_write(hal_addr_t offset, const uint8_t *data, size_t size)
{
    hal_ret_t ret = HAL_OK;

    HAL_CHECK_CONDITION(_g_otp_state > 0,
                        HAL_ERR_GENERIC,
                        "Invalid hal_otp calling sequence, not init!\n");
    HAL_CHECK_CONDITION(data && size, HAL_ERR_BAD_PARAM,
                        "Parameter data/size is 0!\n");

    ret = dubhe_otp_write(offset, data, size);
    HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC,
                        "dubhe write OTP failed!\n");

    ret = HAL_OK;
finish:
    return ret;
}

hal_ret_t hal_gen_psk(uint8_t *psk, size_t psk_size)
{
    if ((!psk) || (!psk_size)) {
        PAL_LOG_ERR("Parameter is NULL!\n");
        return HAL_ERR_BAD_PARAM;
    }
    return dubhe_gen_psk(psk, psk_size);
}

hal_ret_t hal_otp_write_user_data(const uint8_t *user_data,
                                  size_t user_data_size)
{
    if ((!user_data) || (!user_data_size)) {
        PAL_LOG_ERR("Parameter is NULL!\n");
        return HAL_ERR_BAD_PARAM;
    }

    /* do nothing with user data */
    return HAL_OK;
}

#endif /* END ARM_CE_DUBHE_OTP enable */
