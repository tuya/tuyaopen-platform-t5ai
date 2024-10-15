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

#include "ce_lite_hash.h"
#include "ce_hash.h"

#if defined(CE_LITE_HASH)

static void hash_reset(void)
{
    uint32_t reg;

    reg = IO_READ32(TOP_CTL_RESET_CTL);
    IO_WRITE32(TOP_CTL_RESET_CTL, reg |
                        (1U << TOP_CTL_RESET_CTL_HASH_SRESET_SHIFT));
    /**
     *  From TRM:
     *  Before using the sub-module, wait more than three cycles after
     *  releasing the reset of the sub-module.
     *  Here,
     *  1/25M * 3 = 0.12us.
     *  So delay 1us here is with much margin.
     *
     *  In other words, udelay(1) can support CE clock down to
     *  1/(1/3)=3MHz.
     **/
    pal_udelay(1);
    reg = IO_READ32(TOP_CTL_RESET_CTL);
    IO_WRITE32(TOP_CTL_RESET_CTL, reg &
                        ~(1U << TOP_CTL_RESET_CTL_HASH_SRESET_SHIFT));
    /**
     * Run
     */
    reg = IO_READ32(HASH_CTRL_REG);
    IO_WRITE32(HASH_CTRL_REG, reg | (1 << HASH_CTRL_REG_HASH_RUN_SHIFT));
}

/**
 * Init hash engine, enable clock, reset engine and run.
 */
int32_t hash_engine_init(void)
{
    uint32_t reg = 0;
    /**
     * Enable hash_clock.
     */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg |
                        (1U << TOP_CTL_CLOCK_CTL_HASH_CLK_EN_SHIFT));
    /**
     * Run
     */
    reg = IO_READ32(HASH_CTRL_REG);
    IO_WRITE32(HASH_CTRL_REG, reg | (1 << HASH_CTRL_REG_HASH_RUN_SHIFT));

    return CE_SUCCESS;
}

/**
 * Deinit hash engine, disable clock.
 */
void hash_engine_exit(void)
{
    uint32_t reg = 0;

    /* disable clock */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg & ~(1 << HASH_CTRL_REG_HASH_RUN_SHIFT));
}

const static hash_info_t hash_lists[] = {
#if defined(CE_LITE_SM3)
    {
        "SM3",
        CE_ALG_SM3,
        64,
        CE_SM3_HASH_SIZE,
    },
#endif /* CE_LITE_SM3 */

#if defined(CE_LITE_SHA256)
    {
        "SHA256",
        CE_ALG_SHA256,
        64,
        CE_SHA256_HASH_SIZE,
    },
#endif /* CE_LITE_SHA256 */

#if defined(CE_LITE_SHA224)
    {
        "SHA224",
        CE_ALG_SHA224,
        64,
        CE_SHA224_HASH_SIZE,
    },
#endif /* CE_LITE_SHA224 */

#if defined(CE_LITE_SHA1)
    {
        "SHA1",
        CE_ALG_SHA1,
        64,
        CE_SHA1_HASH_SIZE,
    },
#endif /* CE_LITE_SHA1 */

};

static const hash_info_t *hash_get_info(ce_algo_t algo)
{
    size_t i = 0;
    for (i = 0; i < CE_ARRAY_SIZE(hash_lists); i++) {
        if (algo == hash_lists[i].algo) {
            return &hash_lists[i];
        }
    }
    return NULL;
}

static uint32_t hash_algo_to_te_algo(ce_algo_t algo)
{
    uint32_t ret = 0;

    switch (algo) {
#if defined(CE_LITE_SHA1)
    case CE_ALG_SHA1: {
        ret = 0;
        break;
    }
#endif
#if defined(CE_LITE_SHA224)
    case CE_ALG_SHA224: {
        ret = 1;
        break;
    }
#endif
#if defined(CE_LITE_SHA256)
    case CE_ALG_SHA256: {
        ret = 2;
        break;
    }
#endif
#if defined(CE_LITE_SM3)
    case CE_ALG_SM3: {
        ret = 3;
        break;
    }
#endif
    default: {
        PAL_ASSERT(0);
    }
    }

    return ret;
}

/**
 * hash wait command done.
 * If returns CE_ERROR_GENERIC, it means a hardware error.
 */
static int32_t hash_wait_done(void)
{
    int32_t ret;
    uint32_t t;

    do {
        t = IO_READ32(HASH_INTR_STAT_REG);
        IO_WRITE32(HASH_INTR_STAT_REG, t);
        if (t & (~0x11U)) {
            PAL_LOG_ERR("hash INIR STATE returns failure INTR status: 0x%x\n", t);
            ret = CE_ERROR_GENERIC;
            break;
        } else {
            t = IO_READ32(HASH_STAT_REG);
            if ((HASH_QUEUE_DEPTH ==
                    FIELD_GET(t, HASH_STAT_REG, QUEUE_AVAIL_SLOTS)) &&
                (FIELD_GET(t, HASH_STAT_REG, ENGINE_STAT) <= 0x1U)) {
                ret = CE_SUCCESS;
                break;
            }
        }
    }while (1);

    return ret;
}

/**
 * HASH INIT flow
 */
static void hash_op_init(ce_algo_t algo)
{
    /* make sure engine is running */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) &
                    (1U << TOP_CTL_CLOCK_CTL_HASH_CLK_EN_SHIFT));
    IO_WRITE32(HASH_QUEUE_REG, (0x80U << HASH_QUEUE_REG_OP_SHIFT) |
                (uint32_t)(((hash_algo_to_te_algo(algo)) & 0x7U) << 5));
}
/**
 * HASH PROC flow.
 */
static void hash_op_proc(const uint8_t *msg,
                         size_t msg_size)
{
    /* make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) &
                    (1U << TOP_CTL_CLOCK_CTL_HASH_CLK_EN_SHIFT));
    if (0 == msg_size) {
        return;
    }

    IO_WRITE32(HASH_QUEUE_REG, 0x40U << HASH_QUEUE_REG_OP_SHIFT);
    IO_WRITE32(HASH_QUEUE_REG, (uint32_t)((uintptr_t)msg));
    IO_WRITE32(HASH_QUEUE_REG, (uint32_t)msg_size);
}

/**
 * HASH FINAL flow
 */
static void hash_op_final(const uint8_t *out,
                                 size_t out_size)
{
    (void) out_size;
    /* make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) &
                    (1U << TOP_CTL_CLOCK_CTL_HASH_CLK_EN_SHIFT));

    IO_WRITE32(HASH_QUEUE_REG, (0x20U << HASH_QUEUE_REG_OP_SHIFT) |
                                (uint32_t)(0x1U << 7));
    IO_WRITE32(HASH_QUEUE_REG, (uint32_t)((uintptr_t)out));
}

int32_t ce_dgst(ce_algo_t algo, const uint8_t *in, size_t len, uint8_t *hash)
{
    int32_t ret             = CE_SUCCESS;
    const hash_info_t *info = NULL;
    uint8_t tmp_dgst[CE_MAX_HASH_SIZE];

    /* check algorithm */
    if ((algo != CE_ALG_SM3 ) && (algo != CE_ALG_SHA256) &&
        (algo != CE_ALG_SHA1) && (algo != CE_ALG_SHA224)) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }
    /* when len != 0, check in and dgst */
    if ((len != (size_t)0) && ((NULL == in) || (NULL == hash))) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* get hash info */
    info = hash_get_info(algo);
    PAL_ASSERT(info);

    /* make sure tmp_dgst buffer is large enough to save digest */
    PAL_ASSERT(info->dgst_size <= sizeof(tmp_dgst));
    /* send INIT command */
    hash_op_init(algo);
    ret = hash_wait_done();
    if (ret != CE_SUCCESS) {
        goto end;
    }

    /* send PROC command, with last process = 1 */
    hash_op_proc(in, len);
    ret = hash_wait_done();
    if (ret != CE_SUCCESS) {
        goto cleanup;
    }

    hash_op_final(tmp_dgst, info->dgst_size);
    ret = hash_wait_done();
    if (ret != CE_SUCCESS) {
        goto cleanup;
    }

    /* copy tmp dgst to user's output buffer */
    pal_memcpy(hash, tmp_dgst, info->dgst_size);

cleanup:
    if (CE_SUCCESS != ret) {
        hash_reset();
    }
end:
    /* reset tmp digest on stack */
    pal_memset(tmp_dgst, 0, sizeof(tmp_dgst));
    return ret;
}

int32_t ce_dgst_init(ce_dgst_ctx_t *ctx, ce_algo_t algo)
{
    int32_t ret          = CE_SUCCESS;
    hash_ctx_t *hash_ctx = NULL;

    /* check ctx ptr */
    if (NULL == ctx) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* check algorithm */
    if ((algo != CE_ALG_SM3 ) && (algo != CE_ALG_SHA256) &&
        (algo != CE_ALG_SHA1) && (algo != CE_ALG_SHA224)) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* allocate one internal hash context */
    hash_ctx = pal_calloc(1, sizeof(hash_ctx_t));
    if (NULL == hash_ctx) {
        PAL_LOG_ERR("Calloc %d failed!\n", (uint32_t)sizeof(hash_ctx_t));
        ret = CE_ERROR_OOM;
        goto end;
    }
    hash_ctx->algo = algo;
    /* get hash info */
    hash_ctx->info = hash_get_info(algo);
    PAL_ASSERT(hash_ctx->info);
    hash_ctx->state = HASH_STATE_IDLE;

    /* set to ce_dgst_ctx_t */
    ctx->ctx = hash_ctx;

end:
    return ret;
}

void ce_dgst_free(ce_dgst_ctx_t *ctx)
{
    if (NULL == ctx) {
        return;
    }
    /* state compensation */
    if (ctx->ctx) {
        if ((HASH_STATE_BUSY == ctx->ctx->state) ||
            (HASH_STATE_ERR == ctx->ctx->state)) {
            /* reset to clear hw fsm */
            hash_reset();
        }
        pal_memset(ctx->ctx, 0, sizeof(hash_ctx_t));
        pal_free(ctx->ctx);
        ctx->ctx = NULL;
    }
    return;
}

int32_t ce_dgst_start(ce_dgst_ctx_t *ctx)
{
    int32_t ret          = CE_SUCCESS;
    hash_ctx_t *hash_ctx = NULL;

    /* check ctx ptr */
    if ((NULL == ctx) || (NULL == ctx->ctx)) {
        return CE_ERROR_BAD_PARAMS;
    }

    hash_ctx = ctx->ctx;

    /* check state */
    if (hash_ctx->state != HASH_STATE_IDLE) {
        ret = CE_ERROR_BAD_STATE;
        goto end;
    }
    /* send INIT command */
    hash_op_init(hash_ctx->algo);
    ret = hash_wait_done();
    if (ret != CE_SUCCESS) {
        goto end;
    }

    hash_ctx->block_idx = 0;
    hash_ctx->state     = HASH_STATE_BUSY;

end:
    if (ret != CE_SUCCESS) {
        hash_ctx->state = HASH_STATE_INVALID;
    }
    return ret;
}

int32_t ce_dgst_update(ce_dgst_ctx_t *ctx, const uint8_t *in, size_t len)
{
    int32_t ret          = CE_SUCCESS;
    hash_ctx_t *hash_ctx = NULL;
    size_t cur_sz        = 0;
    uint8_t *in_ptr = (uint8_t *)in;
    size_t left = len;

    /* check ctx ptr */
    if ((NULL == ctx) || (NULL == ctx->ctx)) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* when len != 0, check in */
    if ((len != (size_t)0) && (NULL == in)) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    hash_ctx = ctx->ctx;

    /* check state */
    if (hash_ctx->state != HASH_STATE_BUSY) {
        ret = CE_ERROR_BAD_STATE;
        goto end;
    }

    /* when len is 0, do nothing */
    if (0UL == len) {
        ret = CE_SUCCESS;
        goto end;
    }

    /* fill saved block data if have */
    if (hash_ctx->block_idx != 0) {
        cur_sz = CE_MIN(hash_ctx->info->block_size - hash_ctx->block_idx, left);
        /* copy to block buffer */
        pal_memcpy(hash_ctx->block_buf + hash_ctx->block_idx, in_ptr, cur_sz);
        hash_ctx->block_idx += cur_sz;
        in_ptr += cur_sz;
        left -= cur_sz;
    }

    /* process saved block data if necessary */
    if (hash_ctx->info->block_size == hash_ctx->block_idx) {
        /* send PROC command */
        hash_op_proc(hash_ctx->block_buf, hash_ctx->info->block_size);
        ret = hash_wait_done();
        if (ret != CE_SUCCESS) {
            goto end;
        }
        hash_ctx->block_idx = 0;
    }

    /* process block aligned data if have */
    cur_sz = CE_ROUND_DOWN(left, hash_ctx->info->block_size);
    if (cur_sz) {
        /* send PROC command */
        hash_op_proc(in_ptr, cur_sz);
        ret = hash_wait_done();
        if (ret != CE_SUCCESS) {
            goto end;
        }
        in_ptr += cur_sz;
        left -= cur_sz;
    }

    /* save tail if have */
    if (left) {
        cur_sz = left;
        PAL_ASSERT(cur_sz < hash_ctx->info->block_size);
        /* copy to block buffer */
        pal_memcpy(hash_ctx->block_buf + hash_ctx->block_idx, in_ptr, cur_sz);
        hash_ctx->block_idx += cur_sz;
        left -= cur_sz;
    }

    PAL_ASSERT(0 == left);
    PAL_ASSERT(hash_ctx->block_idx < hash_ctx->info->block_size);

end:
    /* make sure that only when hardware failure, return CE_ERROR_GENERIC */
    if (CE_ERROR_GENERIC == ret) {
        /**
         * Only change state to ERR, and depends on hash_finish to reset the state.
         */
        hash_ctx->state = HASH_STATE_ERR;
    }
    return ret;
}

int32_t ce_dgst_finish(ce_dgst_ctx_t *ctx, uint8_t *hash)
{
    int32_t ret            = CE_SUCCESS;
    hash_ctx_t *hash_ctx   = NULL;
    uint8_t tmp_dgst[CE_MAX_HASH_SIZE];

    if ((NULL == ctx) || (NULL == ctx->ctx)) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* get internal hash context */
    hash_ctx = ctx->ctx;

    /* if dgst is NULL, clear hash context */
    if (NULL == hash) {
        PAL_LOG_DEBUG("Reset hash context!\n");
        ret = CE_ERROR_NO_DATA;
        goto cleanup;
    }

    /* if not in BUSY or ERR state, clear hash context and return bad state */
    if ((hash_ctx->state != HASH_STATE_BUSY) &&
        (hash_ctx->state != HASH_STATE_ERR)) {
        PAL_LOG_DEBUG("Bad hash state: %d!\n", hash_ctx->state);
        ret = CE_ERROR_BAD_STATE;
        goto cleanup;
    }

    /* if in ERR state, clear hash context */
    if (HASH_STATE_ERR == hash_ctx->state) {
        PAL_LOG_DEBUG("Reset bad hash context!\n");
        ret = CE_ERROR_GENERIC;
        goto cleanup;
    }

    /* make sure tmp_dgst buffer is large enough to save digest */
    PAL_ASSERT(hash_ctx->info->dgst_size <= sizeof(tmp_dgst));

    /* send PROC command, if any */
    if (hash_ctx->block_idx > 0) {
        hash_op_proc(hash_ctx->block_buf, hash_ctx->block_idx);
        ret = hash_wait_done();
        if (ret != CE_SUCCESS) {
            goto cleanup;
        }
        hash_ctx->block_idx = 0;
    }
    hash_op_final(tmp_dgst, hash_ctx->info->dgst_size);
    ret = hash_wait_done();
    if (ret != CE_SUCCESS) {
        goto cleanup;
    }

    /* copy back to user's dgst buffer */
    pal_memcpy(hash, tmp_dgst, hash_ctx->info->dgst_size);
    hash_ctx->state = HASH_STATE_IDLE;
cleanup:
    if (ret != CE_SUCCESS) {
        hash_reset();
    }
end:
    /* reset tmp digest on stack */
    pal_memset(tmp_dgst, 0, sizeof(tmp_dgst));
    return ret;
}

#endif /* CE_LITE_HASH */
