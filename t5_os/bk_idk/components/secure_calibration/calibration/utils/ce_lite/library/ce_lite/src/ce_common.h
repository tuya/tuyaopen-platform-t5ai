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

#ifndef __CE_COMMON_H__
#define __CE_COMMON_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"

/*
 * Top memory map
 */
#define TOP_CTL_OFS (0x0000U)
#define TOP_STAT_OFS (0x0100U)
#define TRNG_CTL_OFS (0x0200U)
#define DBG_CTL_OFS (0x0300U)
#define OTP_CTL_OFS (0x0400U)
#define OTP_SPACE_OFS (0x1000U)


#define HOST_IO_SIZE (0x400U)
#define ACA_OFS (0x3000U + HOST_IO_SIZE * CE_CFG_HOST_ID)
#define SCA_OFS (0x3200U + HOST_IO_SIZE * CE_CFG_HOST_ID)
#define HASH_OFS (0x3280U + HOST_IO_SIZE * CE_CFG_HOST_ID)
#if CE_CFG_HOST_ID == 0
#define RNP_OFS (0x0280U)
#elif CE_CFG_HOST_ID == 1
#define RNP_OFS (0x3800U)
#else
#error "SHANHAI Dubhe only support host 0 and host 1"
#endif

/*
 * Top control registers
 */
#define TOP_CTL_CLOCK_CTL (TOP_CTL_OFS + 0x00)
#define TOP_CTL_CLOCK_CTL_HASH_CLK_EN_SHIFT    (0x00U)
#define TOP_CTL_CLOCK_CTL_HASH_CLK_EN_WIDTH    (0x01U)
#define TOP_CTL_CLOCK_CTL_SCA_CLK_EN_SHIFT     (0x01U)
#define TOP_CTL_CLOCK_CTL_SCA_CLK_EN_WIDTH     (0x01U)
#define TOP_CTL_CLOCK_CTL_ACA_CLK_EN_SHIFT     (0x02U)
#define TOP_CTL_CLOCK_CTL_ACA_CLK_EN_WIDTH     (0x01U)
#define TOP_CTL_CLOCK_CTL_OTP_CLK_EN_SHIFT     (0x03U)
#define TOP_CTL_CLOCK_CTL_OTP_CLK_EN_WIDTH     (0x01U)
#define TOP_CTL_CLOCK_CTL_TRNG_CLK_EN_SHIFT    (0x04U)
#define TOP_CTL_CLOCK_CTL_TRNG_CLK_EN_WIDTH    (0x01U)
#define TOP_CTL_CLOCK_CTL_DMA_CLK_EN_SHIFT     (0x05U)
#define TOP_CTL_CLOCK_CTL_DMA_CLK_EN_WIDTH     (0x01U)


#define TOP_CTL_RESET_CTL (TOP_CTL_OFS + 0x04)
#define TOP_CTL_RESET_CTL_HASH_SRESET_SHIFT    (0x00U)
#define TOP_CTL_RESET_CTL_HASH_SRESET_WIDTH    (0x01U)
#define TOP_CTL_RESET_CTL_SCA_SRESET_SHIFT     (0x01U)
#define TOP_CTL_RESET_CTL_SCA_SRESET_WIDTH     (0x01U)
#define TOP_CTL_RESET_CTL_ACA_SRESET_SHIFT     (0x02U)
#define TOP_CTL_RESET_CTL_ACA_SRESET_WIDTH     (0x01U)
#define TOP_CTL_RESET_CTL_OTP_SRESET_SHIFT     (0x03U)
#define TOP_CTL_RESET_CTL_OTP_SRESET_WIDTH     (0x01U)
#define TOP_CTL_RESET_CTL_TRNG_SRESET_SHIFT    (0x04U)
#define TOP_CTL_RESET_CTL_TRNG_SRESET_WIDTH    (0x01U)

#define TOP_CTL_TOP_CFG (TOP_CTL_OFS + 0x08)

/*
 * Top status registers
 */
#define TOP_STAT_VER (TOP_STAT_OFS + 0x00)

#define TOP_STAT_CFG1 (TOP_STAT_OFS + 0x04)
#define TOP_STAT_CFG1_OTP_TST_WORD_SIZE_SHIFT    (0x00U)
#define TOP_STAT_CFG1_OTP_TST_WORD_SIZE_WIDTH    (0x0BU)
#define TOP_STAT_CFG1_RN_POOL_EXT_HOST_SEC_SHIFT (0x0FU)
#define TOP_STAT_CFG1_RN_POOL_EXT_HOST_SEC_WIDTH (0x01U)
#define TOP_STAT_CFG1_RN_POOL_HOST_NUM_SHIFT     (0x10U)
#define TOP_STAT_CFG1_RN_POOL_HOST_NUM_WIDTH     (0x02U)
#define TOP_STAT_CFG1_TRNG_INTER_SRC_EXIST_SHIFT (0x12U)
#define TOP_STAT_CFG1_TRNG_INTER_SRC_EXIST_WIDTH (0x01U)
#define TOP_STAT_CFG1_OTP_INIT_VALUE_SHIFT       (0x13U)
#define TOP_STAT_CFG1_OTP_INIT_VALUE_WIDTH       (0x01U)
#define TOP_STAT_CFG1_OTP_EXIST_SHIFT            (0x14U)
#define TOP_STAT_CFG1_OTP_EXIST_WIDTH            (0x01U)
#define TOP_STAT_CFG1_ACA_SRAM_SIZE_SHIFT        (0x15U)
#define TOP_STAT_CFG1_ACA_SRAM_SIZE_WIDTH        (0x02U)
#define TOP_STAT_CFG1_ACA_EXT_HOST_SEC_SHIFT     (0x17U)
#define TOP_STAT_CFG1_ACA_EXT_HOST_SEC_WIDTH     (0x01U)
#define TOP_STAT_CFG1_ACA_HOST_NUM_SHIFT         (0x18U)
#define TOP_STAT_CFG1_ACA_HOST_NUM_WIDTH         (0x02U)
#define TOP_STAT_CFG1_HASH_EXT_HOST_SEC_SHIFT    (0x1AU)
#define TOP_STAT_CFG1_HASH_EXT_HOST_SEC_WIDTH    (0x01U)
#define TOP_STAT_CFG1_HASH_HOST_NUM_SHIFT        (0x1BU)
#define TOP_STAT_CFG1_HASH_HOST_NUM_WIDTH        (0x02U)
#define TOP_STAT_CFG1_SCA_EXT_HOST_SEC_SHIFT     (0x1DU)
#define TOP_STAT_CFG1_SCA_EXT_HOST_SEC_WIDTH     (0x01U)
#define TOP_STAT_CFG1_SCA_HOST_NUM_SHIFT         (0x1EU)
#define TOP_STAT_CFG1_SCA_HOST_NUM_WIDTH         (0x02U)

#define TOP_STAT_CFG2 (TOP_STAT_OFS + 0x08)
#define TOP_STAT_CFG2_OTP_SEC_WORD_SIZE_SHIFT    (0x00U)
#define TOP_STAT_CFG2_OTP_SEC_WORD_SIZE_WIDTH    (0x0BU)
#define TOP_STAT_CFG2_OTP_NSEC_WORD_SIZE_SHIFT   (0x10U)
#define TOP_STAT_CFG2_OTP_NSEC_WORD_SIZE_WIDTH   (0x0BU)
#define TOP_STAT_INTR_HOST0 (TOP_STAT_OFS + 0x10)
#define TOP_STAT_INTR_HOST1 (TOP_STAT_OFS + 0x14)


/*
 * Top Deubug Control registers
 */
#define TOP_DBG_CTL (DBG_CTL_OFS + 0x00U)
#define TOP_DBG_CTRL_VALUE_SHIFT   (0x00U)
#define TOP_DBG_CTRL_VALUE_WIDTH    (0x20U)

#define TOP_DBG_CTL_LOCK (DBG_CTL_OFS + 0x04U)
#define TOP_DBG_CTL_LOCK_VALUE_SHIFT   (0x00U)
#define TOP_DBG_CTL_LOCK_VALUE_WIDTH    (0x20U)





#ifdef __cplusplus
extern "C" {
#endif

/* The CE HW config structure */
typedef union _ce_hw_cfg_t {
    uint32_t value;
	struct {
	    uint16_t otp_test_word_size : 11;
	    uint8_t reserved : 4;
	    uint8_t rn_pool_host1_attr : 1; /* 0:normal 1:secure */
	    uint8_t rn_pool_host_num : 2; /* can be 0, 1, 2 */
	    uint8_t trng_inter_src_exist : 1; /* 0:not exist 1: exists*/
	    uint8_t otp_init_value : 1;
	    uint8_t otp_exist : 1; /* 0:not exist 1: exists*/
	    uint8_t aca_sram_size : 2; /* 0:4K 1:8K 2:16K 3:reserved*/
	    uint8_t aca_host1_attr : 1; /* 0:normal 1:secure */
	    uint8_t aca_host_num : 2; /* can be 0, 1, 2 */
	    uint8_t hash_host1_attr : 1; /* 0:normal 1:secure */
	    uint8_t hash_host_num : 2; /* can be 0, 1, 2 */
	    uint8_t sca_host1_attr : 1; /* 0:normal 1:secure */
	    uint8_t sca_host_num : 2; /* can be 0, 1, 2 */
	} cfg;
} ce_hw_cfg_t;

/* The global CE hardware configurations */
extern ce_hw_cfg_t ce_hw_cfg;

/*
 * Make the CE IO address based on the specified offset
 */
#define CE_ADDR(ofs) ((uintptr_t)CE_CFG_BASE_ADDR + (ofs))

#ifdef IO_LOG_ENABLE

#define IO_LOG(...)                                                            \
    do {                                                                       \
        PAL_LOG_DEBUG(__VA_ARGS__);                                           \
    } while (0)

#else /* IO_LOG_ENABLE */

#define IO_LOG(...)                                                            \
    do {                                                                       \
    } while (0)

#endif /* !IO_LOG_ENABLE */

/*
 * Read register
 */
#define IO_READ32(ofs)                                                         \
    __extension__({                                                            \
        uint32_t _v = LE32TOH(*(volatile uint32_t *)CE_ADDR(ofs));             \
        IO_LOG(                                                                \
            "READ_REG_WORD (0x%08x)                              ;0x%08x\n",   \
            CE_ADDR(ofs), _v);                                                 \
        _v;                                                                    \
    })

/*
 * Write register
 */
#define IO_WRITE32(ofs, val)                                                   \
    do {                                                                       \
        IO_LOG("WRITE_REG_WORD(0x%08x, 0x%08x)\n", CE_ADDR(ofs), (val));       \
        *((volatile uint32_t *)CE_ADDR(ofs)) = HTOLE32(val);                   \
    } while (0)

/*
 * Check register
 */
#define IO_CHECK32(ofs, msk, want)                                             \
    do {                                                                       \
        uint32_t _t = 0;                                                       \
        IO_LOG("CHECK_REG_WORD(0x%08x, 0x%08x, 0x%08x)", CE_ADDR(ofs), (msk),  \
               (want));                                                        \
        do {                                                                   \
            _t = LE32TOH(*(volatile uint32_t *)CE_ADDR(ofs));                  \
        } while ((_t & (msk)) != ((want) & (msk)));                            \
        IO_LOG("      ;0x%08x\n", _t);                                         \
    } while (0)

/*
 * Endianess
 */
#define ___ce_swap32(x)                                                        \
    ((uint32_t)((((uint32_t)(x) & (uint32_t)0x000000ffU) << 24) |             \
                (((uint32_t)(x) & (uint32_t)0x0000ff00U) << 8) |              \
                (((uint32_t)(x) & (uint32_t)0x00ff0000U) >> 8) |              \
                (((uint32_t)(x) & (uint32_t)0xff000000U) >> 24)))


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define HTOLE32(x) ((uint32_t)(x))
#define LE32TOH(x) ((uint32_t)(x))

#define HTOBE32(x) (___ce_swap32(x))
#define BE32TOH(x) (___ce_swap32(x))

#else /* __ORDER_LITTLE_ENDIAN__ */

#define HTOLE32(x) (___ce_swap32(x))
#define LE32TOH(x) (___ce_swap32(x))

#define HTOBE32(x) ((uint32_t)(x))
#define BE32TOH(x) ((uint32_t)(x))

#endif /* !__ORDER_LITTLE_ENDIAN__ */

/**
 * Other helper macros
 */
#ifndef CE_ARRAY_SIZE
#define CE_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define CE_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CE_MAX(a, b) ((a) >= (b) ? (a) : (b))

#define CE_CONTAINER_OF(ptr, TYPE, FIELD)                                      \
    ((TYPE *)(((char *)(ptr)) - offsetof(TYPE, FIELD)))

/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
#define CE_ROUND_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define CE_ROUND_DOWN(x, a) ((x) & ~((a)-1))
/* check if "x" is align with "align"(must be a power of 2) */
#define CE_IS_ALIGNED(x, a) (!(((x)) & ((a)-1)))

#ifndef __ce_dma_aligned
/**
 * __ce_dma_aligned - Mark variable keep aligned.
 */
#define CE_DMA_ALIGNED OSAL_CACHE_LINE_SIZE
#define __ce_dma_aligned __attribute__((aligned(CE_DMA_ALIGNED)))
#endif

/* The maximum retry count when calling random fuction */
#define CE_RANDOM_RETRY_CNT (30)

#define CHECK_SUCCESS_GOTO(_ret_, _label_)                                     \
    do {                                                                       \
        if (CE_SUCCESS != _ret_) {                                             \
            goto _label_;                                                      \
        }                                                                      \
    } while (0)

#ifndef SET_BITS
/**
 * SET_BITS - set bit[\p s, \p s + \p w] to \p nv for value \p v.
 * @v:          output value.
 * @s:          the field shift value.
 * @w:          the field width.
 * @nv:         new value.
 */
#define SET_BITS(v, s, w, nv) do {                                              \
    (v) &= ~(((0x1 << (w)) - 1) << (s));                                        \
    (v) |= ((nv) & ((0x1 << (w)) - 1)) << (s);                                  \
} while(0)
#endif

#ifndef GET_BITS
/**
 * GET_BITS - get bit[\p s,\p s + \p w] from value \p v.
 * @v:          input value.
 * @s:          the field shift value.
 * @w:          the field width.
 */
#define GET_BITS(v, s, w) (((v) >> (s)) & ((1 << (w)) - 1))
#endif

#ifndef FIELD_GET
/**
 * Get the field \p fn value from the specified register \p rn
 * value \p val.
 *
 * \val             Register value, 32-bit.
 * \rn              Register name.
 * \fn              Register field name.
 */
#define FIELD_GET(val, rn, fn) __extension__({         \
     GET_BITS((val),rn ##_## fn ##_SHIFT,                  \
              rn ##_## fn ##_WIDTH);                       \
})
#endif

#ifndef FIELD_SET
/**
 * Set the field \p fn value \p fv in the specified register \p rn
 * value \p val.
 *
 * \val             Register value, 32-bit.
 * \rn              Register name.
 * \fn              Register field name.
 * \fv              Register field value.
 */
#define FIELD_SET(val, rn, fn, fv) __extension__({    \
     SET_BITS((val),rn ##_## fn ##_SHIFT,                 \
              rn ##_## fn ##_WIDTH,(fv));                 \
})
#endif


/**
 * Functions to parse main algorithm and chain mode
 */
#define CE_ALG_GET_MAIN_ALG(algo) ((algo) & 0xFF)
#define CE_ALG_GET_CHAIN_MODE(algo) (((algo) >> 8) & 0xF)

#ifdef __cplusplus
}
#endif

#endif /*__CE_COMMON_H__*/
