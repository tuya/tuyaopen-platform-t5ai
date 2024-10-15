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

#include "ce_lite_common.h"
#include "ce_lite_otp.h"
#include "ce_otp.h"

#if defined(CE_LITE_OTP)

/**
 * OTP word size in byte
 */
#define OTP_WORD_SIZE           (4U)

/**
 * LCS(Life Cycle State) definition
 */
#define LCS_CM                  (0x00U)
#define LCS_DM                  (0x01U)
#define LCS_DD                  (0x03U)
#define LCS_DR                  (0x07U)

#define OTP_LCS_MASK            (0x07U)
#define OTP_INVALID_MASK        (0x08U)

/**
 * OTP READ/WRITE operation define
 */
#define OTP_OP_READ             (0x00U)
#define OTP_OP_WRITE            (0x01U)

/**
 * \brief   Read one word from the OTP at the specified \p _ofs. The OTP
 *          works as being set with an initial value of 0 always in s/w
 *          perspective.
 * \_ofs    offset into the OTP table
 * \return  OTP value
 */
#define OTP_READ(ofs) (IO_READ32((OTP_SPACE_OFS + (ofs))))

/**
 * \brief   Write one word to the OTP at the specified \p _ofs. The OTP
 *          works as being set with an intial value of 0 always in s/w
 *          perspective. Indirect write allowed only.
 * \_ofs    offset of the OTP table
 * \_val    value to write
 * \return  void
 */
#define OTP_WRITE(_ofs, _val) do {                                     \
    IO_WRITE32(OTP_WR_ADDR, (_ofs));                                   \
    IO_WRITE32(OTP_WR_DATA, (_val));                                   \
    IO_WRITE32(OTP_WR_TRIGGER, 1);                                     \
} while(0)

#define OTP_SET_CTRL(_ctrl) do {                                       \
    uint32_t _e = 0;                                                   \
    _e = IO_READ32(OTP_SETTING);                                       \
    FIELD_SET(_e, OTP_SETTING, DIRECT_RD, (_ctrl).direct_rd);          \
    FIELD_SET(_e, OTP_SETTING, GENERAL_CTRL, (_ctrl).general_ctrl);    \
    IO_WRITE32(OTP_SETTING, _e);                                       \
} while(0)

#define OTP_GET_CTRL(_ctrl) do {                                       \
    uint32_t _e = 0;                                                   \
    _e = IO_READ32(OTP_SETTING);                                       \
    (_ctrl).direct_rd = FIELD_GET(_e, OTP_SETTING, DIRECT_RD);         \
    (_ctrl).general_ctrl = FIELD_GET(_e, OTP_SETTING, GENERAL_CTRL);   \
} while(0)

#if defined(CE_LITE_OTP_PUF_ENROLL)
typedef union ce_otp_lock {
    uint32_t value;
    struct {
        uint32_t model_id : 1;
        uint32_t model_key : 1;
        uint32_t device_id : 1;
        uint32_t device_root_key : 1;
        uint32_t secure_boot_pk_hash : 1;
        uint32_t secure_debug_pk_hash : 1;
        uint32_t reserve : 2;
        uint32_t usr_nonsec_region : 8;
        uint32_t usr_sec_region : 16;
    } locks;
} ce_otp_lock_t;
#endif /* CE_LITE_OTP_PUF_ENROLL */

typedef struct ce_otp_ctrl {
    uint32_t direct_rd:1;
    uint32_t general_ctrl:8;
} ce_otp_ctrl_t;

static  ce_otp_cfg_info_t g_otp_cfg = {0};
#if defined(CE_LITE_OTP_PUF) && (CE_CFG_HOST_ID == 0)
static ce_emem_puf_ops_t g_puf_ops = {0};
#endif /* CE_LITE_OTP_PUF && (CE_CFG_HOST_ID == 0) */

#if defined(CE_LITE_OTP_DUMMY)
#if CE_CFG_HOST_ID == 0
static int32_t otp_read(uint32_t off, uint8_t *buf, uint32_t len)
{
    int32_t ret = CE_SUCCESS;
    uint32_t value = 0;
    int32_t i = 0;

	PAL_LOG_DEBUG("otp read off=%x len=%x\r\n", off, len);
    switch (off) {
        case CE_OTP_LCS_OFFSET:
            value = IO_READ32(OTP_DUMMY_CFG);
            pal_memset(buf, 0, len);
            switch (value & 0x0FU) {
                /** 0 means the initial value
                 *  of the reg, we should treat
                 *  it as CM.
                 * */
                case 0:
                    buf[0] = LCS_CM;
                    break;
                case 1:
                    buf[0] = LCS_DR;
                    break;
                case 2:
                    buf[0] = LCS_DD;
                    break;
                case 4:
                    buf[0] = LCS_DM;
                    break;
                case 8:
                    buf[0] = LCS_CM;
                    break;
                default:
                    ret = CE_ERROR_NOT_SUPPORTED;
                    break;
            }
            break;
        case CE_OTP_DEVICE_RK_OFFSET:
            for(i = 0;i < len/OTP_WORD_SIZE; i++) {
                value = IO_READ32(OTP_DUMMY_DEVICE_RK0 + OTP_WORD_SIZE*i);
                pal_memcpy(&buf[OTP_WORD_SIZE*i], &value, OTP_WORD_SIZE);
            }
            break;
        case CE_OTP_MODEL_KEY_OFFSET:

            for(i = 0;i < len/OTP_WORD_SIZE; i++) {
                value = IO_READ32(OTP_DUMMY_MODEL_KEY0 + OTP_WORD_SIZE*i);
                pal_memcpy(&buf[OTP_WORD_SIZE*i], &value, OTP_WORD_SIZE);
            }
            break;
        /** For dummy otp, the lock ctrl is different with normal OTP,
         *  there is no lock ctrl region in dummy otp, we just need to
         *  set the lock_en bit of OTP_DUMMY_CFG register.
         * */
        case CE_OTP_LOCK_CTRL_OFFSET:
            value = IO_READ32(OTP_DUMMY_CFG);
            pal_memset(buf, 0, len);
            buf[0] = FIELD_GET(value, OTP_DUMMY_CFG, LOCK_EN);
            break;
        default:
            ret = CE_ERROR_NOT_SUPPORTED;
            break;
    }

    return ret;
}

#if defined(CE_LITE_OTP_WRITE)
static int32_t otp_write(uint32_t off, const uint8_t *buf, uint32_t len)
{
    int32_t ret = CE_SUCCESS;
    uint32_t lcs = 0;
    uint32_t value = 0;
    int32_t i = 0;

    value = IO_READ32(OTP_DUMMY_CFG);
    if (FIELD_GET(value, OTP_DUMMY_CFG, LOCK_EN) == 1) {
        ret = CE_ERROR_ACCESS_DENIED;
        goto error;
    }
    switch (off) {
        case CE_OTP_LCS_OFFSET:
            /* for dummy otp, the lcs only keep the last write value*/
            value = 0;
            pal_memcpy(&lcs, buf, len);
            switch (lcs) {
                case LCS_CM:
                    FIELD_SET(value, OTP_DUMMY_CFG, LCS_CM, 1);
                    break;
                case LCS_DM:
                    FIELD_SET(value, OTP_DUMMY_CFG, LCS_DM, 1);
                    break;
                case LCS_DD:
                    FIELD_SET(value, OTP_DUMMY_CFG, LCS_DD, 1);
                    break;
                case LCS_DR:
                    FIELD_SET(value, OTP_DUMMY_CFG, LCS_DR, 1);
                    break;
                default:
                    ret = CE_ERROR_NOT_SUPPORTED;
                    goto error;
            }
            IO_WRITE32(OTP_DUMMY_CFG, value);
            break;
        case CE_OTP_DEVICE_RK_OFFSET:
            for(i = 0;i < len/OTP_WORD_SIZE; i++) {
                pal_memcpy(&value, &buf[OTP_WORD_SIZE*i], OTP_WORD_SIZE);
                IO_WRITE32(OTP_DUMMY_DEVICE_RK0 + OTP_WORD_SIZE*i, value);
            }
            break;
        case CE_OTP_MODEL_KEY_OFFSET:
            for(i = 0;i < len/OTP_WORD_SIZE; i++) {
                pal_memcpy(&value, &buf[OTP_WORD_SIZE*i], OTP_WORD_SIZE);
                IO_WRITE32(OTP_DUMMY_MODEL_KEY0 + OTP_WORD_SIZE*i, value);
            }
            break;
        /** For dummy otp, the lock ctrl is different with normal OTP,
         *  there is no lock ctrl region in dummy otp, we just need to
         *  set the lock_en bit of OTP_DUMMY_CFG register.
         * */
        case CE_OTP_LOCK_CTRL_OFFSET:
            value = IO_READ32(OTP_DUMMY_CFG);
            FIELD_SET(value, OTP_DUMMY_CFG, LOCK_EN, 1);
            IO_WRITE32(OTP_DUMMY_CFG, value);
            break;
        default:
            ret = CE_ERROR_NOT_SUPPORTED;
            break;
    }
error:
    return ret;
}
#endif /* CE_LITE_OTP_WRITE */
#endif /* CE_CFG_HOST_ID == 0 */

#else /* !CE_LITE_OTP_DUMMY */
static int32_t otp_user_region_check_len(size_t offset,
                                         size_t len)
{
    int32_t ret = CE_SUCCESS;

    if (offset >= CE_OTP_TEST_REGION_OFFSET(g_otp_cfg)) {
        if ((offset + len) >
           ((size_t)CE_OTP_TEST_REGION_OFFSET(g_otp_cfg) +
            g_otp_cfg.otp_tst_sz)) {
           ret = CE_ERROR_OVERFLOW;
           goto error;
        }
    } else if (offset >= CE_OTP_USER_SEC_REGION_OFFSET(g_otp_cfg)) {
        if ((offset + len) >
           ((size_t)CE_OTP_USER_SEC_REGION_OFFSET(g_otp_cfg) +
            g_otp_cfg.otp_s_sz)) {
           ret = CE_ERROR_OVERFLOW;
           goto error;
        }
    } else if (offset >= CE_OTP_USER_NON_SEC_REGION_OFFSET) {
        if ((offset + len) >
           ((size_t)CE_OTP_USER_NON_SEC_REGION_OFFSET +
            g_otp_cfg.otp_ns_sz)) {
           ret = CE_ERROR_OVERFLOW;
           goto error;
        }
    } else {
        /* Fixed region will not check */
    }

error:
    return ret;
}

static int32_t otp_read(uint32_t off, uint8_t *buf, uint32_t len)
{
    int32_t ret = CE_SUCCESS;
    uint32_t val = 0;
    uint32_t pos = off;

    PAL_ASSERT((off & (OTP_WORD_SIZE - 1)) == 0 &&
            (len & (OTP_WORD_SIZE - 1)) == 0);
    if (0 == len) {
        ret = CE_SUCCESS;
        goto error;
    }

    for (; off + len - pos >= OTP_WORD_SIZE; pos += OTP_WORD_SIZE) {
        val = OTP_READ(pos);
        pal_memcpy(buf + pos - off, &val, OTP_WORD_SIZE);
    }

error:
    return ret;
}

static inline int32_t otp_read_with_lcs_mask(uint32_t off, uint8_t *buf,
                                             uint32_t len)
{
    int32_t ret = otp_read(off, buf, len);

    if ((CE_SUCCESS == ret) && (CE_OTP_LCS_OFFSET == off) && (len != 0)) {
        *buf &= OTP_LCS_MASK;
    }
    return ret;
}

#if defined(CE_LITE_OTP_WRITE) && (CE_CFG_HOST_ID == 0)
static int32_t otp_write_word(uint32_t ofs, uint32_t val)
{
    uint32_t stat = 0;

    /* write a word */
    OTP_WRITE(ofs, val);

    /* wait otp write done */
    while (1) {
        stat = IO_READ32(OTP_UPDATE_STAT);
        if ((stat & OTP_UPDATE_BUSY) == 0) {
            break;
        }
    }

    /* check result */
    return ((stat & OTP_UPDATE_FAIL) ?
           CE_ERROR_ACCESS_DENIED : CE_SUCCESS);
}

static int32_t otp_write(uint32_t off, const uint8_t *buf, uint32_t len)
{
    int32_t ret = CE_SUCCESS;
    uint32_t val = 0;
    uint32_t pos = off;

    PAL_ASSERT((off & (OTP_WORD_SIZE - 1)) == 0 &&
            (len & (OTP_WORD_SIZE - 1)) == 0);

    if (0 == len) {
        ret = CE_SUCCESS;
        goto error;
    }

    /* complete words */
    for (; off + len - pos >= OTP_WORD_SIZE; pos += OTP_WORD_SIZE) {
        pal_memcpy(&val, buf + pos - off, OTP_WORD_SIZE);
        if ((ret = otp_write_word(pos, val)) != CE_SUCCESS) {
            break;
        }
    }

error:
    return ret;
}
#endif /* CE_LITE_OTP_WRITE && (CE_CFG_HOST_ID == 0) */

#if defined(CE_LITE_OTP_ALERT) && defined(CE_LITE_OTP_WRITE) && (CE_CFG_HOST_ID == 0)
static int32_t otp_sanity_check(void)
{
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t old_ctrl = {0};
    ce_otp_ctrl_t tmp = {0};
    uint32_t lcs = 0;

    OTP_GET_CTRL(old_ctrl);

    pal_memcpy(&tmp, &old_ctrl, sizeof(tmp));
    tmp.direct_rd = 0x1;
    OTP_SET_CTRL(tmp);
    ret = otp_read(CE_OTP_LCS_OFFSET, (uint8_t *)&lcs, sizeof(lcs));
    if (ret != CE_SUCCESS) {
        goto err_read;
    }
    if (LCS_DR == (lcs & OTP_LCS_MASK)) {
        /** check inv done flag */
        if (!(lcs & OTP_INVALID_MASK)) {
            PAL_LOG_ERR("[Alert] Revoked failed, Erase failed!!!\n");
            ret = CE_ERROR_SECURITY;
        }
    }
err_read:
    OTP_SET_CTRL(old_ctrl);
    return ret;
}
#endif /* CE_LITE_OTP_ALERT && CE_LITE_OTP_WRITE && (CE_CFG_HOST_ID == 0) */
#endif /* CE_LITE_OTP_DUMMY */

#if (CE_CFG_HOST_ID == 0) && \
    (defined(CE_LITE_OTP_PUF_ENROLL) || \
        defined(CE_LITE_OTP_PUF_QUALITY_CHECK) || \
        defined(CE_LITE_OTP_WRITE) || \
        defined(CE_LITE_OTP_DUMMY))
static int32_t otp_get_lcs(void)
{
    int32_t lcs = -1;
    int32_t ret = CE_SUCCESS;

    ret = otp_read(CE_OTP_LCS_OFFSET, (uint8_t *)&lcs, sizeof(lcs));
    if (ret != CE_SUCCESS) {
        goto error;
    }
    lcs &= OTP_LCS_MASK;

    ret = lcs;
error:
    return ret;
}
#endif /* (CE_CFG_HOST_ID == 0) && \
          (defined(CE_LITE_OTP_PUF_ENROLL) || \
            defined(CE_LITE_OTP_PUF_QUALITY_CHECK) || \
            defined(CE_LITE_OTP_WRITE) || \
            defined(CE_LITE_OTP_DUMMY)) */

#if defined(CE_LITE_OTP_PUF) && (CE_CFG_HOST_ID == 0)
/**
 * PUF Gerneral control
 */
#define PUF_GNRCTL_PTA_SHIFT          (0x00U)
#define PUF_GNRCTL_PTA_SIZE           (0x02U)
#define PUF_GNRCTL_PTA_MASK           (0x03U)

#define PUF_GNRCTL_PAPUF_SHIFT        (PUF_GNRCTL_PTA_SHIFT +       \
                                            PUF_GNRCTL_PTA_SIZE)
#define PUF_GNRCTL_PAPUF_SIZE         (0x01U)
#define PUF_GNRCTL_PAPUF_MASK         (0x01U)


#define TPENS           (2U)
#define TPENH           (3U)
#define TASH            (11U)
#define TSAS            (3U)

static int32_t otp_puf_ready(void)
{
    uint32_t stat = 0;

    stat = IO_READ32(OTP_UPDATE_STAT);

    /* check result */
    return ((stat & OTP_READY) ?
           CE_SUCCESS:CE_ERROR_BUSY);
}

static void puf_power_on(void)
{
    uint32_t value = 0;

    value = IO_READ32(OTP_SETTING);

    FIELD_SET(value, OTP_SETTING, GNRCTL_PUF_PENVDD2, 1);
    IO_WRITE32(OTP_SETTING,  value);
    pal_udelay(TPENS);
    FIELD_SET(value, OTP_SETTING, GNRCTL_PUF_PDSTB, 1);
    IO_WRITE32(OTP_SETTING,  value);
    pal_udelay(TPENH);
    FIELD_SET(value, OTP_SETTING, GNRCTL_PUF_PRESETN, 1);
    IO_WRITE32(OTP_SETTING,  value);

    return;
}

static inline bool otp_is_region_support_shadow(size_t off)
{
    bool ret = false;
    switch (off) {
        case CE_OTP_MODEL_ID_OFFSET:
        case CE_OTP_MODEL_KEY_OFFSET:
        case CE_OTP_DEVICE_ID_OFFSET:
        case CE_OTP_DEVICE_RK_OFFSET:
        case CE_OTP_LCS_OFFSET:
        case CE_OTP_LOCK_CTRL_OFFSET:
            ret = true;
            break;
        default:
            break;
    }

    return ret;
}

#if defined(CE_LITE_OTP_PUF_ENROLL)
static inline void  otp_get_lock_state(ce_otp_lock_t *lock)
{
    /* TO BE UPDATED: endianess need to process before return back */
    lock->value = OTP_READ(CE_OTP_LOCK_CTRL_OFFSET);

    return;
}

static int32_t otp_puf_enroll(void)
{
#define PUF_ENROLL_SEED     (uint8_t *)"\x12\x34\x56\x78"
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t ctrl_action = {0};
    ce_otp_ctrl_t old_ctrl = {0};
    ce_otp_lock_t lock = {0};
    uint8_t device_rk[CE_OTP_DEVICE_RK_SIZE] = {0};
    int32_t lcs = 0;

    lcs = otp_get_lcs();
    if (lcs < 0) {
        ret = lcs;
        goto error;
    } else {
        if (LCS_DD == lcs || LCS_DR == lcs) {
            ret = CE_ERROR_SECURITY;
            goto error;
        }
    }

    otp_get_lock_state(&lock);

    if (lock.locks.device_root_key) {
        ret = CE_ERROR_ACCESS_DENIED;
        goto error;
    }

    OTP_GET_CTRL(old_ctrl);

    ctrl_action.direct_rd = 1;
    ctrl_action.general_ctrl = old_ctrl.general_ctrl;
    ctrl_action.general_ctrl &= ~(PUF_GNRCTL_PTA_MASK << PUF_GNRCTL_PTA_SHIFT);
    ctrl_action.general_ctrl &= ~(PUF_GNRCTL_PAPUF_MASK <<
                           PUF_GNRCTL_PAPUF_SHIFT);
    OTP_SET_CTRL(ctrl_action);

    /*write non all zero non all F to last word of
      device root key to trigger enroll*/
    ret = otp_write(CE_OTP_DEVICE_RK_OFFSET + 0x0C, PUF_ENROLL_SEED,
            OTP_WORD_SIZE);
    if (ret != CE_SUCCESS) {
        goto err_write;
    }
    /*hw require direct read back */
    ret = otp_read(CE_OTP_DEVICE_RK_OFFSET,
                    device_rk,
                    CE_OTP_DEVICE_RK_SIZE);
    if (ret != CE_SUCCESS) {
        goto err_read;
    }
    /**< lock device root key */
    lock.locks.device_root_key = 1;
    ret = otp_write(CE_OTP_LOCK_CTRL_OFFSET, (uint8_t *)&lock, sizeof(lock) );
err_read:
err_write:
    OTP_SET_CTRL(old_ctrl);
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_ENROLL */

#if defined(CE_LITE_OTP_PUF_QUALITY_CHECK)
static int32_t otp_puf_quality_check(void)
{
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t ctrl_action = {0};
    ce_otp_ctrl_t old_ctrl = {0};
    uint8_t device_rk[CE_OTP_DEVICE_RK_SIZE] = {0};
    int32_t lcs = 0;

    lcs = otp_get_lcs();
    if (lcs < 0) {
        ret = lcs;
        goto error;
    } else {
        if (LCS_DD == lcs || LCS_DR == lcs) {
            ret = CE_ERROR_SECURITY;
            goto error;
        }
    }
    OTP_GET_CTRL(old_ctrl);
    ctrl_action.general_ctrl = old_ctrl.general_ctrl;
    ctrl_action.direct_rd = 1; /* set direct read to flush device root key */
    ctrl_action.general_ctrl |= 0x02 << PUF_GNRCTL_PTA_SHIFT;
    ctrl_action.general_ctrl |= 1 << PUF_GNRCTL_PAPUF_SHIFT;
    OTP_SET_CTRL(ctrl_action);
    /* read root key to check quality check result */
    ret = otp_read(CE_OTP_DEVICE_RK_OFFSET,
                    device_rk,
                    CE_OTP_DEVICE_RK_SIZE);
    if (ret != CE_SUCCESS) {
        goto err_read;
    }
    if ((device_rk[3] & 0x01) != 0x01) {
        ret = CE_ERROR_BUSY;
        goto err_qc;
    }
    /*hw require direct read back to flush device root key */
    ctrl_action.general_ctrl = old_ctrl.general_ctrl;
    OTP_SET_CTRL(ctrl_action);

    ret = otp_read(CE_OTP_DEVICE_RK_OFFSET,
                    device_rk,
                    CE_OTP_DEVICE_RK_SIZE);

err_qc:
err_read:
    OTP_SET_CTRL(old_ctrl);
error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_QUALITY_CHECK */

#define PUF_RD_WAIT (4U)
#if defined(CE_LITE_OTP_PUF_INIT_MGN_READ)
static int32_t otp_puf_init_margin_read(size_t off, uint8_t *buf, size_t len)
{
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t new_ctrl = {0};
    ce_otp_ctrl_t old_ctrl = {0};

    if ((!buf) || (off & (OTP_WORD_SIZE - 1))
        || (len & (OTP_WORD_SIZE - 1))) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }

    if (CE_OTP_DEVICE_RK_OFFSET == off) {
        ret = CE_ERROR_ACCESS_DENIED;
        goto error;
    }
    ret = otp_user_region_check_len(off, len);
    if (CE_SUCCESS == ret) {
        OTP_GET_CTRL(old_ctrl);
        new_ctrl.general_ctrl = old_ctrl.general_ctrl;
        new_ctrl.direct_rd = 1;
        new_ctrl.general_ctrl &= ~(PUF_GNRCTL_PTA_MASK << PUF_GNRCTL_PTA_SHIFT |
                                PUF_GNRCTL_PTA_MASK << PUF_GNRCTL_PTA_SHIFT );
        new_ctrl.general_ctrl |= 0x02 << PUF_GNRCTL_PTA_SHIFT;
        new_ctrl.general_ctrl &= ~(1 << PUF_GNRCTL_PTA_SHIFT);
        OTP_SET_CTRL(new_ctrl);

        /**< proper delay is requred, otherwise will read failed*/
        pal_udelay(PUF_RD_WAIT);
        ret = otp_read_with_lcs_mask(off, buf, len);
        OTP_SET_CTRL(old_ctrl);
    }

error:
    return ret;
}
#endif /* CE_LITE_OTP_PUF_INIT_MGN_READ */

#if defined(CE_LITE_OTP_PUF_PGM_MGN_READ)
static int32_t otp_puf_pgm_margin_read(size_t off,
                           uint8_t *buf, size_t len)
{
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t new_ctrl = {0};
    ce_otp_ctrl_t old_ctrl = {0};

    if ((!buf) || (off & (OTP_WORD_SIZE - 1))
        || (len & (OTP_WORD_SIZE - 1))) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }
    if (CE_OTP_DEVICE_RK_OFFSET == off) {
        ret = CE_ERROR_ACCESS_DENIED;
        goto error;
    }
    ret = otp_user_region_check_len(off, len);
    if (CE_SUCCESS == ret) {
        OTP_GET_CTRL(old_ctrl);
        new_ctrl.general_ctrl = old_ctrl.general_ctrl;
        new_ctrl.direct_rd = 1;
        new_ctrl.general_ctrl &= ~0x7;
        new_ctrl.general_ctrl |= 0x03 << PUF_GNRCTL_PTA_SHIFT;
        new_ctrl.general_ctrl &= ~(1 << PUF_GNRCTL_PAPUF_SHIFT);
        OTP_SET_CTRL(new_ctrl);
        /**< proper delay is requred, otherwise will read failed*/
        pal_udelay(PUF_RD_WAIT);
        ret = otp_read_with_lcs_mask(off, buf, len);
        OTP_SET_CTRL(old_ctrl);
    }

error:
    return ret;
}
#endif/* CE_LITE_OTP_PUF_PGM_MGN_READ */
#endif /* CE_LITE_OTP_PUF && (CE_CFG_HOST_ID == 0) */

int32_t ce_otp_get_vops(void **vops)
{
#if (CE_CFG_HOST_ID == 0) && defined(CE_LITE_OTP_PUF)
    pal_memset(&g_puf_ops, 0, sizeof(g_puf_ops));
#if defined(CE_LITE_OTP_PUF_ENROLL)
    g_puf_ops.enroll = otp_puf_enroll;
#endif /* CE_LITE_OTP_PUF_ENROLL */
#if defined(CE_LITE_OTP_PUF_QUALITY_CHECK)
    g_puf_ops.quality_check = otp_puf_quality_check;
#endif /* CE_LITE_OTP_PUF_QUALITY_CHECK */
#if defined(CE_LITE_OTP_PUF_INIT_MGN_READ)
    g_puf_ops.init_mgn_read = otp_puf_init_margin_read;
#endif /* CE_LITE_OTP_PUF_INIT_MGN_READ */
#if defined(CE_LITE_OTP_PUF_PGM_MGN_READ)
    g_puf_ops.pgm_mgn_read = otp_puf_pgm_margin_read;
#endif /* CE_LITE_OTP_PUF_PGM_MGN_READ */
    *vops = (void *)&g_puf_ops;
    return CE_SUCCESS;
#else /* !((CE_CFG_HOST_ID == 0) && defined(CE_LITE_OTP_PUF)) */
    return CE_ERROR_ACCESS_DENIED;
#endif /*(CE_CFG_HOST_ID == 0) && defined(CE_LITE_OTP_PUF) */
}

static void otp_clk_enable(void)
{
    uint32_t reg;

    /* enable clock */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg |
                        (1U << TOP_CTL_CLOCK_CTL_OTP_CLK_EN_SHIFT));
}

int32_t ce_otp_drv_init(void)
{
#define DELAY_US (10)
    int32_t  ret  = CE_SUCCESS;
    uint32_t conf = 0;

    /* read the otp config from top control regs */
    conf = IO_READ32(TOP_STAT_CFG1);
    g_otp_cfg.otp_exist = ((conf >> TOP_STAT_CFG1_OTP_EXIST_SHIFT) & 1U);
    g_otp_cfg.otp_tst_sz = (conf & 0x7FF)*4; /*WORD to Byte*/
    conf = IO_READ32(TOP_STAT_CFG2);
    g_otp_cfg.otp_s_sz  = (conf & 0x7FF)*4; /*WORD to Byte*/
    g_otp_cfg.otp_ns_sz = ((conf >> TOP_STAT_CFG2_OTP_NSEC_WORD_SIZE_SHIFT) & 0x7FF)*4;

#if CONFIG_BK_BOOT
    /* In BK7236, otp_ns_sz read from hardware is 256, but it should be 128.*/
    g_otp_cfg.otp_ns_sz -= CE_OTP_USER_NON_SEC_REGION_OFFSET;
#endif

    PAL_LOG_DEBUG("otp exist: %d\r\n", g_otp_cfg.otp_exist);
    PAL_LOG_DEBUG("otp tst_sz: %d\r\n", g_otp_cfg.otp_tst_sz);
    PAL_LOG_DEBUG("otp s_sz: %d\r\n", g_otp_cfg.otp_s_sz);
    PAL_LOG_DEBUG("otp ns_sz: %d\r\n", g_otp_cfg.otp_ns_sz);

#if defined(CE_LITE_OTP_DUMMY)
    if (g_otp_cfg.otp_exist) {
#else /* !CE_LITE_OTP_DUMMY */
    if (!g_otp_cfg.otp_exist) {
#endif /* CE_LITE_OTP_DUMMY */
        ret = CE_ERROR_FEATURE_UNAVAIL;
        goto error;
    }
#if CE_CFG_HOST_ID == 0
    otp_clk_enable();
#if defined(CE_LITE_OTP_PUF)
    puf_power_on();
#endif /* CE_LITE_OTP_PUF */
#if !defined(CE_LITE_OTP_DUMMY)
    /**
     * Wait otp init done (only host is 0).
     *  - otp_init_done flag on OTP management Register[0]
     *  - only host 0 can access it.
     */
    for ( ;; ) {
        conf = IO_READ32(OTP_SETTING) & 0x01U;
        if (conf != 0x01U) {
            pal_udelay(2000);
        } else {
            break;
        }
    }
#if defined(CE_LITE_OTP_ALERT) && defined(CE_LITE_OTP_WRITE)
    (void)otp_sanity_check();
#endif /* CE_LITE_OTP_ALERT && defined(CE_LITE_OTP_WRITE) */
#endif /* !CE_LITE_OTP_DUMMY */
#endif /* CE_CFG_HOST_ID == 0 */
error:
    return ret;
}

void ce_otp_drv_exit(void)
{
    return;
}

int32_t ce_otp_get_cfg_info(ce_otp_cfg_info_t *conf)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == conf) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }

    pal_memcpy(conf, (void *)&g_otp_cfg, sizeof(ce_otp_cfg_info_t));
error:
    return ret;
}

int32_t ce_otp_read(size_t offset, uint8_t *buf, size_t len)
{
    int32_t ret = CE_SUCCESS;

    PAL_LOG_INFO("otp read, o=%u l=%u\n", offset, len);
    if ((!buf) || (offset & (OTP_WORD_SIZE - 1))
        || (len & (OTP_WORD_SIZE - 1))) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }

#if defined(CE_LITE_OTP_DUMMY)
#if CE_CFG_HOST_ID == 0
    ret = otp_read(offset, buf, len);
#else /* CE_CFG_HOST_ID == 0 */
    ret = CE_ERROR_ACCESS_DENIED;
#endif /* CE_CFG_HOST_ID != 0 */
#else /* CE_LITE_OTP_DUMMY */
    ret = otp_user_region_check_len(offset, len);
    if (CE_SUCCESS == ret) {
#if defined(CE_LITE_OTP_PUF) && (CE_CFG_HOST_ID == 0)
        if (!otp_is_region_support_shadow(offset)) {
            if (CE_SUCCESS != otp_puf_ready()) {
                ret = CE_ERROR_ACCESS_DENIED;
                goto error;
            }
        }
#endif /* CE_LITE_OTP_PUF && (CE_CFG_HOST_ID == 0) */
        ret = otp_read_with_lcs_mask(offset, buf, len);
    }
#endif /* !CE_LITE_OTP_DUMMY */
error:
    return ret;
}

#if !defined(CE_LITE_OTP_DUMMY) && \
    defined(CE_LITE_OTP_ALERT) && \
    defined(CE_LITE_OTP_WRITE) && \
    (CE_CFG_HOST_ID == 0)
static int32_t otp_alert_revocation_failed(void)
{
    int32_t ret = CE_SUCCESS;
    ce_otp_ctrl_t old_ctrl = {0};
    ce_otp_ctrl_t tmp = {0};
    uint32_t lcs = 0;

    OTP_GET_CTRL(old_ctrl);
    pal_memcpy(&tmp, &old_ctrl, sizeof(tmp));
    tmp.direct_rd = 0x1;
    OTP_SET_CTRL(tmp);
    /** sync otp data */
    ret = otp_read(CE_OTP_LCS_OFFSET, (uint8_t *)&lcs, sizeof(lcs));
    if (ret != CE_SUCCESS) {
        goto error;
    }
    if ((lcs & OTP_LCS_MASK) != LCS_DR) {
        PAL_LOG_ERR("[Alert] Revoked failed, LCS invalid!!!\n");
    }
    if (!(lcs & OTP_INVALID_MASK)) {
        PAL_LOG_ERR("[Alert] Revoked failed, Erase failed!!!\n");
    }
error:
    OTP_SET_CTRL(old_ctrl);
    return ret;
}
#endif /* !defined(CE_LITE_OTP_DUMMY) && \
          defined(CE_LITE_OTP_ALERT) && \
          defined(CE_LITE_OTP_WRITE) && \
          (CE_CFG_HOST_ID == 0) */

#if defined(CE_LITE_OTP_WRITE) || defined(CE_LITE_OTP_DUMMY)
int32_t ce_otp_write(size_t off, const uint8_t *buf, size_t len)
{
#if CE_CFG_HOST_ID == 0
    int32_t ret = CE_SUCCESS;
    int32_t lcs = 0;

    if ((!buf) || (off & (OTP_WORD_SIZE - 1))
        || (len & (OTP_WORD_SIZE - 1))) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }
    lcs = otp_get_lcs();
    if (lcs < 0){
        ret = lcs;
        goto error;
    }
    /**< check lcs contraint */
    if ( (CE_OTP_LCS_OFFSET == off) && (len > 0)
            && ((*buf & OTP_INVALID_MASK)
                ||(((*buf & OTP_LCS_MASK) != LCS_CM)
                    && ((*buf & OTP_LCS_MASK) != LCS_DM)
                    && ((*buf & OTP_LCS_MASK) != LCS_DD)
                    && ((*buf & OTP_LCS_MASK) != LCS_DR))) ) {
        ret = CE_ERROR_BAD_PARAMS;
        goto error;
    }
#if defined(CE_LITE_OTP_PUF)
    if (CE_OTP_DEVICE_RK_OFFSET == off) {
        ret = CE_ERROR_ACCESS_DENIED;
        goto error;
    }
#endif /* CE_LITE_OTP_PUF */

#if defined(CE_LITE_OTP_DUMMY)
#if defined(CE_LITE_OTP_WRITE)
    ret = otp_write(off, (uint8_t *)buf, len);
#endif /* CE_LITE_OTP_WRITE */
#else /* !CE_LITE_OTP_DUMMY */
    ret = otp_user_region_check_len(off, len);
    if (CE_SUCCESS == ret) {
#if defined(CE_LITE_OTP_WRITE)
        ret = otp_write(off, (uint8_t *)buf, len);
#if defined(CE_LITE_OTP_ALERT)
        if (ret != CE_SUCCESS) {
            /** special error handling for Revocation */
            if ((CE_OTP_LCS_OFFSET == off) &&
                (LCS_DR == (*buf & OTP_LCS_MASK))) {
                (void)otp_alert_revocation_failed();
            }
        }
#endif /* CE_LITE_OTP_ALERT */
#endif /* CE_LITE_OTP_WRITE */
    }
#endif /* CE_LITE_OTP_DUMMY */
error:
    return ret;
#else /* CE_CFG_HOST_ID == 0 */
    (void)off;
    (void)buf;
    (void)len;
    return CE_ERROR_ACCESS_DENIED;
#endif /* CE_CFG_HOST_ID != 0 */
}
#endif /* CE_LITE_OTP_WRITE || CE_LITE_OTP_DUMMY */

#endif /* CE_LITE_OTP */
