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
#ifndef __CE_HASH_H__
#define __CE_HASH_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"
#include "ce_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The hash info */
typedef struct hash_info {
    const char *name;
    ce_algo_t algo;
    uint32_t block_size;
    uint32_t dgst_size;
} hash_info_t;

/*
 * HASH registers
 */
/* The offset of hash_queue of target host */
#define HASH_QUEUE_REG   (HASH_OFS + 0x00U)
#define HASH_QUEUE_REG_OP_SHIFT     (0x18U)
#define HASH_QUEUE_REG_OP_WIDTH     (0x08U)
#define HASH_QUEUE_REG_MODE_SHIFT   (0x00U)
#define HASH_QUEUE_REG_MODE_WIDTH   (0x08U)

/* The offset of hash_ctrl of target host */
#define HASH_CTRL_REG        (HASH_OFS + 0x04U)
#define HASH_CTRL_REG_HASH_RUN_SHIFT    (0x00U)
#define HASH_CTRL_REG_HASH_RUN_WIDTH    (0x01U)

/* The offset of hash_stat of target host */
#define HASH_STAT_REG                (HASH_OFS + 0x08U)
#define HASH_STAT_REG_ENGINE_STAT_SHIFT         (0x00U)
#define HASH_STAT_REG_ENGINE_STAT_WIDTH         (0x02U)
#define HASH_STAT_REG_QUEUE_AVAIL_SLOTS_SHIFT   (0x02U)
#define HASH_STAT_REG_QUEUE_AVAIL_SLOTS_WIDTH   (0x07U)

/* The offset of hash_intr_stat of target host */
#define HASH_INTR_STAT_REG      (HASH_OFS + 0x0C)

/* The offset of hash_intr_msk of target host */
#define HASH_INTR_MSK_REG       (HASH_OFS + 0x10)

/* The offset of hash_suspend_msk of target host */
#define HASH_SUSPEND_MSK_REGS   (HASH_OFS + 0x14)

#define CTL_RUN (1U << 0)

/* Queue depth */
#define HASH_QUEUE_DEPTH    (0x2U)

/* maximum block size */
#define MAX_BLOCK_SIZE (64U)

/* The hash state */
enum {
    HASH_STATE_INVALID = 0,
    HASH_STATE_IDLE    = 1,
    HASH_STATE_BUSY    = 2,
    HASH_STATE_ERR     = 3,
};

typedef struct ce_hash_t {
    ce_algo_t algo;
    const hash_info_t *info;
    uint8_t block_buf[MAX_BLOCK_SIZE];
    size_t block_idx;
    int32_t state;
} ce_hash_t;

typedef struct ce_hash_t hash_ctx_t;

extern int32_t hash_engine_init(void);
extern void hash_engine_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* !__CE_HASH_H__ */
