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

#ifndef __CE_CIPHER_H__
#define __CE_CIPHER_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_lite_cipher.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CE_CHAIN_MODE_ECB_NOPAD         (0x00U)
#define CE_CHAIN_MODE_CBC_NOPAD         (0x01U)
#define CE_CHAIN_MODE_CTR               (0x02U)
#define CE_CHAIN_MODE_ECB_PKCS7         (0x0bU)
#define CE_CHAIN_MODE_CBC_PKCS7         (0x0cU)

#define SCA_QUE_REG_OFS                 (SCA_OFS + 0x00U)
#define SCA_CTL_REG_OFS                 (SCA_OFS + 0x04U)
#define SCA_STAT_REG_OFS                (SCA_OFS + 0x08U)
#define SCA_INTR_STAT_REG_OFS           (SCA_OFS + 0x0CU)

#define SCA_CTL_RUN                     (0x01U)
#define SCA_CLK_EN                      (0x01U << TOP_CTL_CLOCK_CTL_SCA_CLK_EN_SHIFT)
#define SCA_RESET                       (0x01U << TOP_CTL_RESET_CTL_SCA_SRESET_SHIFT)

#define CIPHER_KEY_READY_FLAG           (0x01U)
#define CIPHER_IV_READY_FLAG            (0x02U)
#define CIPHER_OP_READY_FLAG            (0x04U)

#define CIPHER_CTX_IDLE_STATE           (0x00U)
#define CIPHER_CTX_READY_STATE          (0x01U)

#define CIPHER_MAX_KEY_SIZE             (32U)

#define SCA_INIT                        (0x80U << 24)
#define SCA_PROC                        (0x40U << 24)
#define SCA_FINISH                      (0x20U << 24)

#define SCA_ENG_IDLE                    (0x00U)
#define SCA_ENG_WAIT_COMM               (0x01U)
#define SCA_ENG_NOR_OPT                 (0X02U)
#define SCA_ENG_STATE                   (0x03U)

#define SCA_QUE_AVAIL_SLOTS_OFS         (0x02U)
#define SCA_QUEUE_DEPTH                 (0x02U)

#define SCA_AES_ALG                     (0x00U << 19)
#define SCA_SM4_ALG                     (0x01U << 19)

#define SCA_ECB_MODE                    (0x00U << 4)
#define SCA_CTR_MODE                    (0x01U << 4)
#define SCA_CBC_MODE                    (0x02U << 4)

#define SCA_KEY_128                     (0x00U << 20)
#define SCA_KEY_256                     (0x02U << 20)

#define SCA_MODEL_KEY                   (0x00U << 22)
#define SCA_ROOT_KEY                    (0x01U << 22)
#define SCA_EXT_KEY                     (0x02U << 22)

#define SCA_KEY_ADDR_FORMAT             (0x01U << 18)
#define SCA_IV_ADDR_FORMAT              (0x01U << 14)

#define SCA_LOADIV                      (0x01U << 15)

#define SCA_ENCRYTION                   (0x01U << 5)
#define SCA_DECRYTION                   (0x00U << 5)

typedef enum cipher_key_type {
    CIPHER_KEY_TYPE_SEC = 0,
    CIPHER_KEY_TYPE_USER
} cipher_key_type_t;

typedef struct cipher_user_key {
    uint8_t key[CIPHER_MAX_KEY_SIZE];   /**< key data */
    uint32_t keybits;                   /**< key length in bits */
} cipher_user_key_t;

typedef struct cipher_key {
    cipher_key_type_t type;
    union {
        ce_sec_key_t sec;
        cipher_user_key_t user;
    };
} cipher_key_t;

typedef struct ce_sca_crypt {
    uint32_t flags;
    uint32_t state;
    ce_algo_t alg;
    uint32_t blksz;
    uint8_t iv[CE_MAX_SCA_BLOCK];
    uint8_t stream[CE_MAX_SCA_BLOCK];
    uint32_t strpos;
    ce_operation_t op;
    cipher_key_t key;
} ce_sca_crypt_t;

extern int32_t sca_engine_init(void);
extern void sca_engine_exit(void);
#ifdef __cplusplus
}
#endif

#endif /* __CE_CIPHER_H__ */