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

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"
#include "ce_common.h"
#include "ce_aca.h"

#ifdef CONFIG_BK_BOOT
#include "pal_log_redefine.h"
#endif

#if defined(CE_LITE_BN)

static void op_exec_cmd(uint32_t op_code,
                        int32_t len_type_id,
                        aca_op_t *A,
                        aca_op_t *B,
                        int32_t imme_B,
                        aca_op_t *C,
                        aca_op_t *R);

static void op_submit_cmd(uint32_t op_code,
                          int32_t len_type_id,
                          aca_op_t *A,
                          aca_op_t *B,
                          int32_t imme_B,
                          aca_op_t *C,
                          aca_op_t *R);
static void op_submit_shift_cmd(uint32_t op_code,
                                int32_t len_type_id,
                                aca_op_t *A,
                                int32_t shift_B,
                                aca_op_t *R);
static void op_wait_cmd(void);

struct last_op_status {
    uint32_t intr_status;
    uint32_t aca_status;
};

static volatile uint32_t g_sram_allocated_addr             = ACA_SRAM_BASE;
static volatile int32_t g_aca_gr_allocated_id              = ACA_GR_USED_START;
static volatile struct last_op_status g_aca_last_op_status = {0};

static inline void reset_last_op_status(void)
{
    g_aca_last_op_status.intr_status = 0;
    g_aca_last_op_status.aca_status  = 0;
}

#define OP_EXEC_ONE_CMD(_op_a_, _op_b_, _op_c_, _op_r_, _len_type_id_,         \
                        _op_code_)                                             \
    do {                                                                       \
        reset_last_op_status();                                                \
        op_exec_cmd(_op_code_, _len_type_id_, _op_a_, _op_b_, -1, _op_c_,      \
                    _op_r_);                                                   \
    } while (0)

#define OP_EXEC_ONE_CMD_IMME_B(_op_a_, _imme_b_, _op_c_, _op_r_,               \
                               _len_type_id_, _op_code_)                       \
    do {                                                                       \
        reset_last_op_status();                                                \
        op_exec_cmd(_op_code_, _len_type_id_, _op_a_, NULL, _imme_b_, _op_c_,  \
                    _op_r_);                                                   \
    } while (0)

#define OP_EXEC_ONE_CMD_SHIFT(_op_a_, _shift_b_, _op_r_, _len_type_id_,        \
                              _op_code_)                                       \
    do {                                                                       \
        reset_last_op_status();                                                \
        op_submit_shift_cmd(_op_code_, _len_type_id_, _op_a_, _shift_b_,       \
                            _op_r_);                                           \
        op_wait_cmd();                                                         \
    } while (0)

#ifdef ECP_DBG_PRINT_DETAIL_EN

#define OP_ECP_PREPARE(_empty_)                                                \
    do {                                                                       \
        reset_last_op_status();                                                \
    } while (0)
#define OP_ECP_EXEC(_code_, _len_type_id_, _op_a_, _is_b_imme_, _b_, _op_c_,   \
                    _op_r_)                                                    \
    do {                                                                       \
        if (_is_b_imme_) {                                                     \
            op_exec_cmd(ACA_OP_##_code_, _len_type_id_, _op_a_, NULL,          \
                        (int32_t)(uintptr_t)_b_, _op_c_, _op_r_);              \
        } else {                                                               \
            op_exec_cmd(ACA_OP_##_code_, _len_type_id_, _op_a_,                \
                        (aca_op_t *)_b_, -1, _op_c_, _op_r_);                  \
        }                                                                      \
    } while (0)
#define OP_ECP_WAIT(_empty_)                                                   \
    do {                                                                       \
    } while (0)

#else /* !ECP_DBG_PRINT_DETAIL_EN */

#define OP_ECP_PREPARE(_empty_)                                                \
    do {                                                                       \
        reset_last_op_status();                                                \
    } while (0)
#define OP_ECP_EXEC(_code_, _len_type_id_, _op_a_, _is_b_imme_, _b_, _op_c_,   \
                    _op_r_)                                                    \
    do {                                                                       \
        if (_is_b_imme_) {                                                     \
            op_submit_cmd(ACA_OP_##_code_, _len_type_id_, _op_a_, NULL,        \
                          (int32_t)(uintptr_t)_b_, _op_c_, _op_r_);            \
        } else {                                                               \
            op_submit_cmd(ACA_OP_##_code_, _len_type_id_, _op_a_,              \
                          (aca_op_t *)_b_, -1, _op_c_, _op_r_);                \
        }                                                                      \
    } while (0)
#define OP_ECP_WAIT(_empty_)                                                   \
    do {                                                                       \
        op_wait_cmd();                                                         \
    } while (0)

#endif /* ECP_DBG_PRINT_DETAIL_EN */

int32_t aca_engine_init(void)
{
    uint32_t reg = 0;

    /**
     * Enable aca_clock, SET FIFO Watermark to 0. We don't use FIFO Wartermark.
     */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    FIELD_SET(reg, TOP_CTL_CLOCK_CTL, ACA_CLK_EN, 1);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg);

    /**
     * Mask all interrupts.
     */
    IO_WRITE32(ACA_ACA_INTR_MSK, 0x7FU);

    return 0;
}

void aca_engine_exit(void)
{
    uint32_t reg = 0;

    /* disable clock */
    reg = IO_READ32(TOP_CTL_CLOCK_CTL);
    FIELD_SET(reg, TOP_CTL_CLOCK_CTL, ACA_CLK_EN, 0);
    IO_WRITE32(TOP_CTL_CLOCK_CTL, reg);
}

/**
 * write some data to ACA SRAM
 */
static void sram_write_data(uint32_t sram_addr,
                            uint32_t block_num,
                            const uint8_t *data)
{
    int32_t i = 0, byte_idx = 0;
    uint32_t val     = 0;
    int32_t word_num = ACA_BLOCK_NUM_TO_BITS(block_num) / 32;

    /* When read/write sram, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    /* write sram_waddr with word offset */
    IO_WRITE32(ACA_SRAM_WADDR, sram_addr >> 2);

    for (i = 0; i < word_num; i++) {
        byte_idx = (word_num - i) * 4 - 1;
        val      = ((((uint32_t)data[byte_idx]) << 0) |
               (((uint32_t)data[byte_idx - 1]) << 8) |
               (((uint32_t)data[byte_idx - 2]) << 16) |
               (((uint32_t)data[byte_idx - 3]) << 24));
        IO_WRITE32(ACA_SRAM_WDATA, val);
    }
}

/**
 * read some data from ACA SRAM
 */
static void sram_read_data(uint32_t sram_addr,
                           uint32_t block_num,
                           uint8_t *data)
{
    int32_t i = 0, byte_idx = 0;
    uint32_t val     = 0;
    int32_t word_num = ACA_BLOCK_NUM_TO_BITS(block_num) / 32;

    /* When read/write sram, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    /* write sram_raddr with word offset */
    IO_WRITE32(ACA_SRAM_RADDR, sram_addr >> 2);

    for (i = 0; i < word_num; i++) {
        byte_idx           = (word_num - i) * 4 - 1;
        val                = IO_READ32(ACA_SRAM_RDATA);
        data[byte_idx]     = (val >> 0) & 0xFF;
        data[byte_idx - 1] = (val >> 8) & 0xFF;
        data[byte_idx - 2] = (val >> 16) & 0xFF;
        data[byte_idx - 3] = (val >> 24) & 0xFF;
    }
}

/**
 * write one word
 */
static void sram_write_word(uint32_t sram_addr, uint32_t data)
{
    /* When read/write sram, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    /* write sram_waddr with word offset */
    IO_WRITE32(ACA_SRAM_WADDR, sram_addr >> 2);
    IO_WRITE32(ACA_SRAM_WDATA, data);
}

#if 0
/**
 * read one word
 */
static void sram_read_word(uint32_t sram_addr, uint32_t *data)
{
    /* When read/write sram, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    /* write sram_raddr with word offset */
    IO_WRITE32(ACA_SRAM_RADDR, sram_addr >> 2);
    *data = IO_READ32(ACA_SRAM_RDATA);
}
#endif

/* allocate a sram address */
static uint32_t sram_alloc_one(uint32_t blk_num)
{
    uint32_t addr = g_sram_allocated_addr;
    g_sram_allocated_addr += ACA_BLOCK_NUM_TO_BYTES(blk_num);

    PAL_ASSERT(g_sram_allocated_addr <= ACA_SRAM_BASE + ACA_SRAM_SIZE);
    return addr;
}

/* free one address */
static void sram_free_one(uint32_t blk_num)
{
    PAL_ASSERT(g_sram_allocated_addr >=
                ACA_SRAM_BASE + ACA_BLOCK_NUM_TO_BYTES(blk_num));
    g_sram_allocated_addr -= ACA_BLOCK_NUM_TO_BYTES(blk_num);
}

/* allocate one GR */
static int32_t gr_alloc_one(void)
{
    int32_t gr = g_aca_gr_allocated_id;
    g_aca_gr_allocated_id++;

    PAL_ASSERT(g_aca_gr_allocated_id <= ACA_GR_NUMBER);

    return gr;
}

/* free one GR */
static void gr_free_one(void)
{
    PAL_ASSERT(g_aca_gr_allocated_id > ACA_GR_USED_START);
    g_aca_gr_allocated_id--;
}

/* init one op ctx, excluding T0 T1 */
static void op_prepare(aca_op_t *op, gr_usage_t usage)
{
    uint32_t reg_off  = 0;
    uint32_t reg_data = 0;

    op->sram_addr = sram_alloc_one(op->blk_num);
    op->gr_id     = gr_alloc_one();

    /* write sram */
    if ((usage == GR_USAGE_IN) || (usage == GR_USAGE_N) ||
        (usage == GR_USAGE_P)) {
        PAL_ASSERT(op->data);
        sram_write_data(op->sram_addr, op->blk_num, op->data);
    }
    /* write sram if have for INOUT */
    if (usage == GR_USAGE_INOUT) {
        if (op->data) {
            sram_write_data(op->sram_addr, op->blk_num, op->data);
        }
    }

    /* config GR */
    reg_off  = ACA_GRX_SRAM_ADDR_OFS + 4 * op->gr_id;
    reg_data = ((op->sram_addr >> 2) & (0xFFF));
    IO_WRITE32(reg_off, reg_data);

    /* config special GRs */
    if ((usage == GR_USAGE_N) || (usage == GR_USAGE_P)) {
        reg_data = IO_READ32(ACA_N_P_T0_T1_USE_GRID);
        if (usage == GR_USAGE_N) {
            reg_data &= (~(0x1FU << 0));
            reg_data |= ((((uint32_t)(op->gr_id)) & 0x1FU) << 0);
        } else {
            reg_data &= (~(0x1FU << 5));
            reg_data |= ((((uint32_t)(op->gr_id)) & 0x1FU) << 5);
        }
        IO_WRITE32(ACA_N_P_T0_T1_USE_GRID, reg_data);
    }
}

/* deinit one op ctx, excluding T0 T1 */
static void op_release(aca_op_t *op)
{
    sram_free_one(op->blk_num);
    gr_free_one();
    op->sram_addr = ACA_SRAM_ADDR_INVALID;
    op->gr_id     = ACA_GR_INVALID;
}

/* init T0 or T1 */
static void op_prepare_tmp(int32_t t0_t1, uint32_t blk_num)
{
    uint32_t reg_off   = 0;
    uint32_t reg_data  = 0;
    uint32_t sram_addr = 0;
    int32_t gr_id      = 0;

    PAL_ASSERT((0 == t0_t1) || (1 == t0_t1));

    sram_addr = sram_alloc_one(blk_num);
    gr_id     = gr_alloc_one();

    /* config GR */
    reg_off  = ACA_GRX_SRAM_ADDR_OFS + 4 * gr_id;
    reg_data = ((sram_addr >> 2) & (0xFFF));
    IO_WRITE32(reg_off, reg_data);

    /* config special GRs */
    reg_data = IO_READ32(ACA_N_P_T0_T1_USE_GRID);
    if (0 == t0_t1) {
        reg_data &= (~(0x1FU << 10));
        reg_data |= ((((uint32_t)(gr_id)) & 0x1FU) << 10);
    } else if (1 == t0_t1) {
        reg_data &= (~(0x1FU << 15));
        reg_data |= ((((uint32_t)(gr_id)) & 0x1FU) << 15);
    } else {
        PAL_ASSERT(0);
    }
    IO_WRITE32(ACA_N_P_T0_T1_USE_GRID, reg_data);
}

/* deinit T0 or T1 */
static void op_release_tmp(int32_t t0_t1, uint32_t blk_num)
{
    sram_free_one(blk_num);
    gr_free_one();
}

/**
 * check that the SRAM and GR resource are in unoccupied state.
 * Here use assert because occupied resource is unexpected
 */
static void op_check_res(void)
{
    PAL_ASSERT(g_aca_gr_allocated_id == ACA_GR_USED_START);
    PAL_ASSERT(g_sram_allocated_addr == ACA_SRAM_BASE);
}

/* config length type */
static void op_cfg_len_type(int32_t len_type_id, int32_t op_bits)
{
    PAL_ASSERT(op_bits <= ACA_MAX_OP_BITS);
    IO_WRITE32(ACA_GR_LEN_TYPEX_OFS + 4 * len_type_id, (op_bits & 0x1FFF));
}

/* submit one command (exclude shift related command) to command queue */
static void op_submit_cmd(uint32_t op_code,
                          int32_t len_type_id,
                          aca_op_t *A,
                          aca_op_t *B,
                          int32_t imme_B,
                          aca_op_t *C,
                          aca_op_t *R)
{
    uint32_t reg = 0;
    uint32_t cmd = 0;

    /* When submitting command, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    cmd = ((op_code & 0x1F) << 27);
    cmd |= ((len_type_id & 0x7) << 24);

    PAL_ASSERT(A);
    PAL_ASSERT(A->gr_id != ACA_GR_INVALID);
    cmd |= ((A->gr_id & 0x1F) << 18);
    if (B) {
        PAL_ASSERT(B->gr_id != ACA_GR_INVALID);
        cmd |= ((B->gr_id & 0x1F) << 12);
    } else if (imme_B != -1) {
        cmd |= (0x1U << 17);
        cmd |= ((imme_B & 0x1F) << 12);
    } else {
        /* This command doesn't need B(for example, MODINV), do nothing */
    }

    if (R) {
        PAL_ASSERT(R->gr_id != ACA_GR_INVALID);
        cmd |= ((R->gr_id & 0x1F) << 7);
    } else {
        /* not save to R */
        cmd |= (0x1U << 6);
    }

    if (C) {
        PAL_ASSERT(C->gr_id != ACA_GR_INVALID);
        cmd |= ((C->gr_id & 0x1F) << 1);
    }

    /* Wait until there is free space in command queue. */
    do {
        reg = IO_READ32(ACA_ACA_STATUS);
        if (reg & 0xFU) {
            break;
        }
    } while (1);

    /* Write command to ACA command queue */
    IO_WRITE32(ACA_ACA_ENTRY, cmd);

    /* Trigger op_run in ACA control incase engine in in IDLE */
    reg = IO_READ32(ACA_ACA_CTRL);
    reg |= 1;
    IO_WRITE32(ACA_ACA_CTRL, reg);
}

/* submit one shift related command to command queue */
static void op_submit_shift_cmd(uint32_t op_code,
                                int32_t len_type_id,
                                aca_op_t *A,
                                int32_t shift_B,
                                aca_op_t *R)
{
    uint32_t reg = 0;
    uint32_t cmd = 0;

    /* When submitting command, make sure clock is enabled */
    PAL_ASSERT(IO_READ32(TOP_CTL_CLOCK_CTL) & (1 << 2));

    cmd = ((op_code & 0x1F) << 27);
    cmd |= ((len_type_id & 0x7) << 24);

    PAL_ASSERT(A);
    PAL_ASSERT(A->gr_id != ACA_GR_INVALID);
    PAL_ASSERT(R);
    PAL_ASSERT(R->gr_id != ACA_GR_INVALID);

    cmd |= ((A->gr_id & 0x1F) << 18);
    cmd |= ((shift_B & 0x3F) << 12);
    cmd |= ((R->gr_id & 0x1F) << 7);

    /* Wait until there is free space in command queue. */
    do {
        reg = IO_READ32(ACA_ACA_STATUS);
        if (reg & 0xFU) {
            break;
        }
    } while (1);

    /* Write command to ACA command queue */
    IO_WRITE32(ACA_ACA_ENTRY, cmd);

    /* Trigger op_run in ACA control incase engine in in IDLE */
    reg = IO_READ32(ACA_ACA_CTRL);
    reg |= 1;
    IO_WRITE32(ACA_ACA_CTRL, reg);
}

/* Wait command finish */
static void op_wait_cmd(void)
{
    uint32_t reg = 0;
    /* Wait FIFO empty and engine done signal */
    do {
        reg = IO_READ32(ACA_ACA_INTR_STAT);
        if ((reg & (1 << 5)) != 0) {
            /* intr_opfifo_empty_engine_done status is set */
            break;
        }
    } while (1);

    /* Write clear interrupt status */
    IO_WRITE32(ACA_ACA_INTR_STAT, reg);

    /**
     * Update last op status, use |= so that we are in batch commands.
     * intr_status is RW1M, and we should write back to clean the status.
     */
    g_aca_last_op_status.intr_status |= reg;
    g_aca_last_op_status.aca_status |= IO_READ32(ACA_ACA_STATUS);
}

static void op_exec_cmd(uint32_t op_code,
                        int32_t len_type_id,
                        aca_op_t *A,
                        aca_op_t *B,
                        int32_t imme_B,
                        aca_op_t *C,
                        aca_op_t *R)
{
    op_submit_cmd(op_code, len_type_id, A, B, imme_B, C, R);
    op_wait_cmd();
}

static inline uint32_t op_read_last_status(void)
{
    return g_aca_last_op_status.aca_status;
}
static inline int32_t op_is_last_op_final_aul_carry(void)
{
    return ((op_read_last_status() & (0x1U << 9)) ? (1) : (0));
}
static inline int32_t op_is_last_op_xor_zero(void)
{
    return ((op_read_last_status() & (0x1U << 8)) ? (1) : (0));
}

static inline uint32_t op_read_last_intr_status(void)
{
    return g_aca_last_op_status.intr_status;
}
static inline int32_t op_intr_is_mod_n_zero(void)
{
    return ((op_read_last_intr_status() & (0x1U << 8)) ? (1) : (0));
}
static inline int32_t op_intr_is_red_time_byd63(void)
{
    return ((op_read_last_intr_status() & (0x1U << 7)) ? (1) : (0));
}
static inline int32_t op_intr_is_mult_red_err(void)
{
    return ((op_read_last_intr_status() & (0x1U << 6)) ? (1) : (0));
}
static inline int32_t op_intr_is_modinv_zero(void)
{
    return ((op_read_last_intr_status() & (0x1U << 4)) ? (1) : (0));
}
static inline int32_t op_intr_is_div_zero(void)
{
    return ((op_read_last_intr_status() & (0x1U << 3)) ? (1) : (0));
}

/* The following is the aca_op interfaces. */

/**
 * Change op's size with new block number, and copy data if have
 * Note: target_bl_num should > 0
 */
static int32_t op_change_size(aca_op_t *op, uint32_t target_blk_num)
{
    int32_t ret  = CE_SUCCESS;
    uint8_t *tmp = NULL;

    PAL_ASSERT(target_blk_num > 0);

    if (op->blk_num != target_blk_num) {
        tmp = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
        if (NULL == tmp) {
            PAL_LOG_ERR("Calloc %d failed!\n",
                         ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
            ret = CE_ERROR_OOM;
            goto end;
        }
        if (op->data) {
            /* copy original data only when op->data is not NULL */
            if (target_blk_num > op->blk_num) {
                pal_memcpy(
                    tmp + ACA_BLOCK_NUM_TO_BYTES(target_blk_num - op->blk_num),
                    op->data,
                    ACA_BLOCK_NUM_TO_BYTES(op->blk_num));
            } else {
                pal_memcpy(tmp,
                            op->data + ACA_BLOCK_NUM_TO_BYTES(op->blk_num -
                                                              target_blk_num),
                            ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
            }
            pal_free(op->data);
            op->data = NULL;
        }
        op->data    = tmp;
        op->blk_num = target_blk_num;
    } else {
        /* do nothing */
    }

end:
    return ret;
}

/* init one aca operation */
void aca_op_init(aca_op_t *op)
{
    PAL_ASSERT(op);
    pal_memset(op, 0, sizeof(aca_op_t));
    op->sram_addr = ACA_SRAM_ADDR_INVALID;
    op->gr_id     = ACA_GR_INVALID;
}

/* free one aca operation */
void aca_op_free(aca_op_t *op)
{
    PAL_ASSERT(op);
    if (op->data) {
        pal_free(op->data);
    }
    pal_memset(op, 0, sizeof(aca_op_t));
    op->sram_addr = ACA_SRAM_ADDR_INVALID;
    op->gr_id     = ACA_GR_INVALID;
}

static int32_t _aca_op_init_np(aca_op_t *np)
{
    aca_op_init(np);
    np->blk_num = ACA_NP_BLK_NUM;
    np->data    = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(np->blk_num));
    if (NULL == np->data) {
        PAL_LOG_ERR("Calloc %d failed!\n",
                     ACA_BLOCK_NUM_TO_BYTES(np->blk_num));
        return CE_ERROR_OOM;
    } else {
        return CE_SUCCESS;
    }
}

/* dump one aca operation's data in hex string format */
#define __UINT8_GET_LOW(__d__) ((__d__) & (0x0F))
#define __UINT8_GET_HIGH(__d__) (((__d__) >> 4) & (0x0F))
#define _INT_TO_CHAR(__d__)                                                    \
    (((__d__) >= (0x0A)) ? ((__d__) - (0x0A) + 'A') : ((__d__) + '0'))
static void dbg_dump_data(const char *msg, uint8_t *data, size_t size)
{
    uint8_t *str_buf    = NULL;
    size_t str_buf_size = 0;
    uint8_t *p          = NULL;
    size_t i            = 0;
    size_t start        = 0;

    str_buf_size = 2 * size + 1;
    str_buf      = pal_calloc(1, str_buf_size);
    if (NULL == str_buf) {
        PAL_LOG_ERR("Calloc %d failed!\n", str_buf_size);
        return;
    }

    /* data is in big endian */
    for (i = 0; i < size; i++) {
        if (data[i] != 0) {
            start = i;
            break;
        }
    }
    p = str_buf;
    if (i != size) {
        for (i = start; i < size; i++) {
            *p = _INT_TO_CHAR(__UINT8_GET_HIGH(data[i]));
            p++;
            *p = _INT_TO_CHAR(__UINT8_GET_LOW(data[i]));
            p++;
        }
        *p = '\0';
    } else {
        *p = '0';
        p++;
        *p = '0';
        p++;
        *p = '\0';
    }
    PAL_LOG_DEBUG("%s: %s\n", msg, str_buf);

    if (NULL != str_buf) {
        pal_free(str_buf);
    }
    return;
}

void aca_op_dump(const char *name, aca_op_t *op_ctx)
{
    uint8_t *tmp_buf = NULL;

    if (!op_ctx) {
        return;
    }
    PAL_LOG_DEBUG("########## Start Dump %s ##########\n", name);

    if (op_ctx->sram_addr != ACA_SRAM_ADDR_INVALID) {
        PAL_ASSERT(0 != op_ctx->blk_num);
        tmp_buf = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(op_ctx->blk_num));
        if (NULL == tmp_buf) {
            PAL_LOG_ERR("Calloc %d failed!\n",
                         ACA_BLOCK_NUM_TO_BYTES(op_ctx->blk_num));
            goto end;
        }
        sram_read_data(op_ctx->sram_addr, op_ctx->blk_num, tmp_buf);
        PAL_LOG_DEBUG("CE SRAM Addr: 0x%x, Block num: %d\n", op_ctx->sram_addr,
                       op_ctx->blk_num);
        dbg_dump_data("CE SRAM Data (Big Endian)", tmp_buf,
                      ACA_BLOCK_NUM_TO_BYTES(op_ctx->blk_num));
    } else if (op_ctx->data) {
        PAL_ASSERT(0 != op_ctx->blk_num);
        PAL_LOG_DEBUG(
            "CE SWAPPED Addr(64 bits): H(32): 0x%x, L(32): 0x%x, Block num: "
            "%d\n",
            (uint32_t)((((uint64_t)((uintptr_t)(op_ctx->data))) >> 32) &
                       0xFFFFFFFFU),
            (uint32_t)((((uint64_t)((uintptr_t)(op_ctx->data)))) & 0xFFFFFFFFU),
            op_ctx->blk_num);
        dbg_dump_data("SWAPPED Data (Big Endian)", op_ctx->data,
                      ACA_BLOCK_NUM_TO_BYTES(op_ctx->blk_num));
    } else {
        PAL_LOG_DEBUG("No Data!\n");
    }

    if (op_ctx->gr_id != ACA_GR_INVALID) {
        PAL_LOG_DEBUG("CE GR ID: %d\n", op_ctx->gr_id);
    }

end:
    if (NULL != tmp_buf) {
        pal_free(tmp_buf);
    }
    PAL_LOG_DEBUG("========== End Dump %s ==========\n", name);
    return;
}

/**
 * copy src to dst, with dst->blk_num == target_blk_num. src must contain data
 */
static int32_t aca_op_copy_change_size(aca_op_t *dst,
                                       aca_op_t *src,
                                       uint32_t target_blk_num)
{
    int32_t ret      = CE_SUCCESS;
    uint8_t *tmp_buf = NULL;

    CHECK_OP_CTX(src);
    PAL_ASSERT(dst);

    PAL_ASSERT(src->blk_num);

    tmp_buf = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
    if (NULL == tmp_buf) {
        PAL_LOG_ERR("Calloc %d failed!\n",
                     ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
        ret = CE_ERROR_OOM;
        goto end;
    }

    /* same logic from op_change_size */
    if (target_blk_num > src->blk_num) {
        pal_memcpy(tmp_buf +
                        ACA_BLOCK_NUM_TO_BYTES(target_blk_num - src->blk_num),
                    src->data,
                    ACA_BLOCK_NUM_TO_BYTES(src->blk_num));
    } else {
        pal_memcpy(tmp_buf,
                    src->data +
                        ACA_BLOCK_NUM_TO_BYTES(src->blk_num - target_blk_num),
                    ACA_BLOCK_NUM_TO_BYTES(target_blk_num));
    }

    if (dst->blk_num) {
        PAL_ASSERT(dst->data);
        pal_free(dst->data);
        dst->data    = NULL;
        dst->blk_num = 0;
    } else {
        PAL_ASSERT(NULL == dst->data);
    }
    dst->data    = tmp_buf;
    dst->blk_num = target_blk_num;

end:
    return ret;
}

int32_t aca_op_copy(aca_op_t *dst, aca_op_t *src)
{
    return aca_op_copy_change_size(dst, src, src->blk_num);
}

#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECP)
/* this function is also used in ECP point mul in this file */
/* write u32 */
int32_t aca_op_import_u32(aca_op_t *op, uint32_t val)
{
    int32_t ret  = CE_SUCCESS;
    uint8_t *tmp = NULL;

    PAL_ASSERT(op);

    /* prepare new data first */
    tmp = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(1));
    if (NULL == tmp) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    /* set as big endian */
    tmp[ACA_BLOCK_NUM_TO_BYTES(1) - 4 + 0] = (val >> 24) & 0xFFU;
    tmp[ACA_BLOCK_NUM_TO_BYTES(1) - 4 + 1] = (val >> 16) & 0xFFU;
    tmp[ACA_BLOCK_NUM_TO_BYTES(1) - 4 + 2] = (val >> 8) & 0xFFU;
    tmp[ACA_BLOCK_NUM_TO_BYTES(1) - 4 + 3] = (val >> 0) & 0xFFU;

    if (op->data) {
        /* free old data if have */
        PAL_ASSERT(op->blk_num > 0);
        pal_free(op->data);
        op->data    = NULL;
        op->blk_num = 0;
    }

    op->data    = tmp;
    op->blk_num = 1;
end:
    return ret;
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING) || (CE_LITE_ECP) */

/* import bignumber data in big endian */
int32_t aca_op_import_bin(aca_op_t *op, const uint8_t *data, size_t size)
{
    int32_t ret          = CE_SUCCESS;
    uint32_t req_blk_num = 0;
    uint8_t *tmp         = NULL;

    PAL_ASSERT(op);
    PAL_ASSERT((0 == size) || (data != NULL));

    /* not exceed max OP bits */
    if (size > (ACA_MAX_OP_BITS >> 3)) {
        PAL_LOG_ERR("size: %d\n", size);
        return CE_ERROR_BAD_PARAMS;
    }

    req_blk_num = ACA_BYTES_TO_BLOCK_NUM(size);

    if (0 == req_blk_num) {
        /**
         * if required blk num is 0, change to 1 so that after calling this
         * function, the op->data and op->blk_num always meet:
         * op->data != NULL and op->blk_num > 0
         */
        req_blk_num = 1;
    }

    /* prepare new data first */
    tmp = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(req_blk_num));
    if (NULL == tmp) {
        ret = CE_ERROR_OOM;
        goto end;
    }

    /* import with bigendain, copy directly */
    if (size) {
        pal_memcpy(tmp + ACA_BLOCK_NUM_TO_BYTES(req_blk_num) - size, data,
                    size);
    }

    if (op->data) {
        /* free old data if have */
        PAL_ASSERT(op->blk_num > 0);
        pal_free(op->data);
        op->data    = NULL;
        op->blk_num = 0;
    }

    op->data    = tmp;
    op->blk_num = req_blk_num;
end:
    return ret;
}

/* export bignumber data to bigendian */
int32_t aca_op_export_bin(aca_op_t *op, uint8_t *buf, size_t size)
{
    int32_t ret           = CE_SUCCESS;
    int32_t bitlen        = 0;
    uint32_t req_buf_size = 0;

    CHECK_OP_CTX(op);
    PAL_ASSERT((0 == size) || (buf != NULL));

    bitlen = aca_op_bitlen((aca_op_t *)op);

    req_buf_size = (bitlen + 7) / 8;
    if (req_buf_size > size) {
        ret = CE_ERROR_SHORT_BUFFER;
        goto end;
    }

    pal_memset(buf, 0, size - req_buf_size);
    pal_memcpy(buf + size - req_buf_size,
                op->data + ACA_BLOCK_NUM_TO_BYTES(op->blk_num) - req_buf_size,
                req_buf_size);
end:
    return ret;
}

/*
 * Count leading zero bits in a given integer
 */
static size_t _sram_clz(const uint32_t x)
{
    size_t j;
    uint32_t mask = (uint32_t)1 << (32 - 1);

    for (j = 0; j < 32; j++) {
        if (x & mask) {
            break;
        }
        mask >>= 1;
    }

    return j;
}
/**
 * Get bitlength.
 *
 * Note: this function is special, it supports aca_op_t which has not imported
 * data.
 * Note: don't add defined(CE_LITE_XXX) to strict this function, because this
 * function is also used in other aca_op_xxx in this file.
 */
uint32_t aca_op_bitlen(aca_op_t *op)
{
    int32_t i    = 0;
    uint32_t val = 0;
    int32_t size = 0;

    PAL_ASSERT(op);

    /* return 0 if this aca_op doesn't have data */
    if (op->blk_num == 0) {
        PAL_ASSERT(NULL == op->data);
        return 0;
    }

    size = ACA_BLOCK_NUM_TO_BYTES(op->blk_num);

    for (i = 0; i < size; i += 4) {
        /* get one u32 value */
        val = (((uint32_t)(op->data[i + 0]) << 24) |
               ((uint32_t)(op->data[i + 1]) << 16) |
               ((uint32_t)(op->data[i + 2]) << 8) |
               ((uint32_t)(op->data[i + 3]) << 0));
        if (val != 0) {
            break;
        }
    }
    if (i == size) {
        return 0;
    } else {
        return ((size - i) * 8 - _sram_clz(val));
    }
}

/**
 * Get bit value from 0 started position
 * Note: don't add defined(CE_LITE_XXX) to strict this function, because this
 * function is also used in other aca_op_xxx in this file.
 */
int32_t aca_op_get_bit_value(aca_op_t *op, size_t pos)
{
    uint32_t tmp         = 0;
    uint32_t byte_offset = 0;
    uint32_t bit_offset  = 0;

    CHECK_OP_CTX(op);

    byte_offset = (uint32_t)(pos) / 8;
    bit_offset  = (uint32_t)(pos) % 8;
    if (byte_offset >= ACA_BLOCK_NUM_TO_BYTES(op->blk_num)) {
        return 0;
    }

    tmp = op->data[ACA_BLOCK_NUM_TO_BYTES(op->blk_num) - byte_offset - 1];
    tmp &= (0x1U << bit_offset);
    return ((tmp == 0) ? (0) : (1));
}

/**
 * Calculate np.
 * The output np block number is always ACA_NP_BLK_NUM(2), and the data buffer
 * should be prepared by caller.
 *
 * Update NP. Algorithm:
 *
 * Only support: k ≤ 144, P = floor (2^(k+71)/N);
 *
 */
static int32_t op_cal_np_case1(aca_op_t *np, aca_op_t *N, uint32_t kbits)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t value      = 0;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    /* high = 2^(k + 135) */
    aca_op_t high = ACA_OP_INIT_DATA;
    /* tmp to save np */
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t tmp_n = ACA_OP_INIT_DATA;

    /* internal used, skip check ctx */
    PAL_ASSERT(np->blk_num == ACA_NP_BLK_NUM);
    PAL_ASSERT(kbits <= 144);

    op_bit_len = CE_ROUND_UP(kbits + 71 + 1, ACA_BLOCK_BITS);
    op_blk_num = ACA_BITS_TO_BLOCK_NUM(op_bit_len);
    /* init high */
    high.blk_num = op_blk_num;
    high.data    = NULL;

    /* init tmp */
    tmp.blk_num = op_blk_num;
    tmp.data    = NULL;

    ret = aca_op_copy_change_size(&tmp_n, N, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_n, GR_USAGE_IN);
    op_prepare(&tmp, GR_USAGE_OUT);
    op_prepare(&high, GR_USAGE_INOUT);
    op_prepare_tmp(0, op_blk_num);
    op_prepare_tmp(1, op_blk_num);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* zero high */
    OP_EXEC_ONE_CMD_IMME_B(&high, 0, NULL, &high, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_AND);

    /* set 2^(k + 71) bits to 1 */
    value = (0x1U << ((kbits + 71) % 32));
    sram_write_word(high.sram_addr + 4 * ((kbits + 71) / 32), value);

    /* P = 2^(k + 71) / N */
    OP_EXEC_ONE_CMD(&high, &tmp_n, NULL, &tmp, ACA_LENTYPE_ID_BLK_OP, ACA_OP_DIV);
    /* Check div by 0 */
    if (op_intr_is_div_zero()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /* read NP from sram */
    sram_read_data(tmp.sram_addr, np->blk_num, np->data);

end:
    op_release(&tmp_n);
    op_release(&tmp);
    op_release(&high);
    op_release_tmp(0, op_blk_num);
    op_release_tmp(1, op_blk_num);
    op_check_res();
    aca_op_free(&tmp_n);
error:
    return ret;
}

/**
 * Calculate np.
 * The output np block number is always ACA_NP_BLK_NUM(2), and the data buffer
 * should be prepared by caller.
 *
 * Update NP. Algorithm:
 *
 * Only support: k > 144, P = floor(top/bottom);
 *                        top= 2^215; bottom=ceil(N/(2^(k-144)));
 *
 */
static int32_t op_cal_np_case2(aca_op_t *np, aca_op_t *N, uint32_t kbits)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t value      = 0;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    uint32_t shift_size = 0, cur_shift_size = 0;
    aca_op_t top    = ACA_OP_INIT_DATA;
    aca_op_t bottom = ACA_OP_INIT_DATA;
    aca_op_t tmp    = ACA_OP_INIT_DATA;
    aca_op_t tmp_n  = ACA_OP_INIT_DATA;

    /* internal used, skip check ctx */
    PAL_ASSERT(np->blk_num == ACA_NP_BLK_NUM);
    PAL_ASSERT(kbits > 144);

    op_blk_num = CE_MAX(ACA_BITS_TO_BLOCK_NUM(215 + 1), N->blk_num);
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_blk_num);

    /* init top, bottom and tmp */
    top.blk_num    = op_blk_num;
    top.data       = NULL;
    bottom.blk_num = op_blk_num;
    bottom.data    = NULL;
    tmp.blk_num    = op_blk_num;
    tmp.data       = NULL;

    ret = aca_op_copy_change_size(&tmp_n, N, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_n, GR_USAGE_IN);
    op_prepare(&top, GR_USAGE_INOUT);
    op_prepare(&bottom, GR_USAGE_INOUT);
    op_prepare(&tmp, GR_USAGE_OUT);
    op_prepare_tmp(0, op_blk_num);
    op_prepare_tmp(1, op_blk_num);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* Init top = 2^215 */
    OP_EXEC_ONE_CMD_IMME_B(&top, 0, NULL, &top, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_AND);
    /* set 2^(215) bits to 1 */
    value = (0x1U << (215 % 32));
    sram_write_word(top.sram_addr + 4 * (215 / 32), value);

    /* calculating N/(2^(k-144)) */

    /* set bottom == N - 1, prevent further ceiling increment */
    OP_EXEC_ONE_CMD_IMME_B(&tmp_n, 1, NULL, &bottom, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_SUB);

    /* start to shift right */
    shift_size = kbits - 144;
    while (shift_size) {
        cur_shift_size = CE_MIN(shift_size, 0x40);
        /* bottom == bottom >> cur_shift_size */
        OP_EXEC_ONE_CMD_SHIFT(&bottom, cur_shift_size - 1, &bottom,
                              ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHR0);
        shift_size -= cur_shift_size;
    }

    /* Ceiling by bottom == bottom + 1 */
    OP_EXEC_ONE_CMD_IMME_B(&bottom, 1, NULL, &bottom, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_ADD);

    /* P = (2^215)/ceil(N/(2^(k-144))) */
    OP_EXEC_ONE_CMD(&top, &bottom, NULL, &tmp, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_DIV);
    /* Check div by 0 */
    if (op_intr_is_div_zero()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /* read NP from sram */
    sram_read_data(tmp.sram_addr, np->blk_num, np->data);

end:
    op_release(&tmp_n);
    op_release(&tmp);
    op_release(&top);
    op_release(&bottom);
    op_release_tmp(0, op_blk_num);
    op_release_tmp(1, op_blk_num);
    op_check_res();
    aca_op_free(&tmp_n);
error:
    return ret;
}

/**
 * The function uses physical data pointers to calculate and output
 * the Barrett tag Np.
 *
 *  For N bitsize > 2*A+2*X it uses truncated sizes:
 *      Np = truncated(2^(3*A+3*X-1) / ceiling(n/(2^(N-2*A-2*X)));
 *  For  N bitsize <= 2*A+2*X:
 *      Np = truncated(2^(N+A+X-1) / n);
 *  Here A means ACA word size in bits(64), X means ACA extra bits(8)
 *  N means N bit size(kbits)
 */
static int32_t op_cal_np(aca_op_t *np, aca_op_t *N, uint32_t kbits)
{
    if (kbits > 144) {
        return op_cal_np_case2(np, N, kbits);
    } else {
        return op_cal_np_case1(np, N, kbits);
    }
}

#if defined(CE_LITE_RSASSA) || defined(CE_LITE_ECP)
int32_t aca_op_cmp_bn(aca_op_t *op_a, aca_op_t *op_b, int32_t *result)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t *tmp_ptr1 = op_a;
    aca_op_t *tmp_ptr2 = op_b;

    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);
    PAL_ASSERT(result);

    /* init tmp */
    aca_op_init(&tmp);

    op_bit_len = ACA_BLOCK_NUM_TO_BITS(CE_MAX(op_a->blk_num, op_b->blk_num));

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    if (op_a->blk_num < op_b->blk_num) {
        /* copy op_a to tmp */
        ret = aca_op_copy_change_size(&tmp, op_a, op_b->blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr1 = ( aca_op_t *)&tmp;
    } else if (op_a->blk_num > op_b->blk_num) {
        /* copy op_b to tmp */
        ret = aca_op_copy_change_size(&tmp, op_b, op_a->blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr2 = ( aca_op_t *)&tmp;
    }

    op_prepare(tmp_ptr1, GR_USAGE_IN);
    op_prepare(tmp_ptr2, GR_USAGE_IN);

    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, NULL, ACA_LENTYPE_ID_BLK_OP, ACA_OP_XOR);
    if (op_is_last_op_xor_zero()) {
        /* XOR result is 0, a == b */
        *result = 0;
        ret     = CE_SUCCESS;
        goto end;
    }

    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, NULL, ACA_LENTYPE_ID_BLK_OP, ACA_OP_SUB);
    if (op_is_last_op_final_aul_carry()) {
        /* ALU carry, a < b */
        *result = -1;
    } else {
        *result = 1;
    }

end:
    op_release(tmp_ptr1);
    op_release(tmp_ptr2);
    op_check_res();
error:
    aca_op_free(&tmp);
    return ret;
}
#endif /* CE_LITE_RSASSA || CE_LITE_ECP */

#if defined(CE_LITE_ECP)
int32_t aca_op_cmp_u32(aca_op_t *op_a, uint32_t b, int32_t *result)
{
    int32_t ret          = CE_SUCCESS;
    uint8_t tmp_a_buf[4] = {0};
    uint32_t tmp_a       = 0;

    PAL_ASSERT(result);

    if (op_a->blk_num != 0) {
        PAL_ASSERT(NULL != op_a->data);

        ret = aca_op_export_bin(op_a, tmp_a_buf, sizeof(tmp_a_buf));
        if (CE_ERROR_SHORT_BUFFER == ret) {
            /* op_a can't fill into 32bits buffer, a > b */
            *result = 1;
            ret     = CE_SUCCESS;
            goto end;
        }
        PAL_ASSERT(CE_SUCCESS == ret);

        tmp_a = ((tmp_a_buf[3]) | (tmp_a_buf[2] << 8) | (tmp_a_buf[1] << 16) |
                 (tmp_a_buf[0] << 24));
    } else {
        tmp_a = 0;
    }

    if (tmp_a == b) {
        *result = 0;
    } else if (tmp_a > b) {
        *result = 1;
    } else {
        *result = -1;
    }

end:
    return ret;
}
#endif /* CE_LITE_ECP */

#if defined(CE_LITE_ECP) || defined(CE_LITE_RSASSA_SIGN)
int32_t aca_op_cmp_bn_equal(aca_op_t *op_a, aca_op_t *op_b, int32_t *result)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t *tmp_ptr1 = op_a;
    aca_op_t *tmp_ptr2 = op_b;

    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);
    PAL_ASSERT(result);

    /* init tmp */
    aca_op_init(&tmp);
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(CE_MAX(op_a->blk_num, op_b->blk_num));

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    if (op_a->blk_num < op_b->blk_num) {
        /* copy op_a to tmp */
        ret = aca_op_copy_change_size(&tmp, op_a, op_b->blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr1 = ( aca_op_t *)&tmp;
    } else if (op_a->blk_num > op_b->blk_num) {
        /* copy op_b to tmp */
        ret = aca_op_copy_change_size(&tmp, op_b, op_a->blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr2 = ( aca_op_t *)&tmp;
    }

    op_prepare(tmp_ptr1, GR_USAGE_IN);
    op_prepare(tmp_ptr2, GR_USAGE_IN);

    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, NULL, ACA_LENTYPE_ID_BLK_OP, ACA_OP_XOR);
    if (op_is_last_op_xor_zero()) {
        /* XOR result is 0, a == b */
        *result = 0;
    } else {
        *result = 1;
    }

    op_release(tmp_ptr1);
    op_release(tmp_ptr2);
    op_check_res();
error:
    aca_op_free(&tmp);
    return ret;
}
#endif /* CE_LITE_ECP || CE_LITE_RSASSA_SIGN */

#if defined(CE_LITE_ECP)

/**
 * This function covers add_mod and sub_mod, because they are both ALU OPs.
 * Requirement: op_a and op_b bit length < op_n's bit length. Because there is
 * MOD_RED to reduction op_a/op_b, and if op_a/op_b bit length > op_n's bit
 * length, the MOD_RED may take very long time.
 */
#if defined(CE_LITE_ECP) || defined(CE_LITE_SM2DSA_SIGN)
static int32_t _aca_op_add_sub_mod(aca_op_t *op_r,
                                   aca_op_t *op_a,
                                   aca_op_t *op_b,
                                   aca_op_t *op_n,
                                   int32_t is_mod_add)
{
    int32_t ret    = CE_SUCCESS;
    aca_op_t tmp_a = ACA_OP_INIT_DATA;
    aca_op_t tmp_b = ACA_OP_INIT_DATA;
    /* use tmp_r to support IO same */
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);
    CHECK_OP_CTX(op_n);

    op_bit_len = aca_op_bitlen(op_n);
    /**
     * checks that op_a and op_b bit length <= op_n bitlen.
     */
    PAL_ASSERT(aca_op_bitlen(op_a) <= op_bit_len);
    PAL_ASSERT(aca_op_bitlen(op_b) <= op_bit_len);

    op_blk_num = ACA_BITS_TO_BLOCK_NUM(op_bit_len);

    /* init tmp_a and tmp_b */
    aca_op_init(&tmp_a);
    aca_op_init(&tmp_b);
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;

    /* reset op_r size if necessary */
    ret = op_change_size(op_r, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_a, op_a, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_b, op_b, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(op_n, GR_USAGE_N);
    op_prepare_tmp(0, op_blk_num);
    op_prepare(&tmp_a, GR_USAGE_IN);
    op_prepare(&tmp_b, GR_USAGE_IN);
    op_prepare(&tmp_r, GR_USAGE_OUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BIT_OP, op_bit_len);

    /* Call MODRED first */
    OP_EXEC_ONE_CMD(&tmp_a, op_n, NULL, &tmp_a, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODRED);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    OP_EXEC_ONE_CMD(&tmp_b, op_n, NULL, &tmp_b, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODRED);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /* call mod_add or mod_sub */
    OP_EXEC_ONE_CMD(&tmp_a, &tmp_b, NULL, &tmp_r, ACA_LENTYPE_ID_BIT_OP,
                    (is_mod_add ? ACA_OP_MODADD : ACA_OP_MODSUB));
    if (op_intr_is_mod_n_zero() || op_intr_is_red_time_byd63()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /* read result */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num, op_r->data);

end:
    op_release(op_n);
    op_release_tmp(0, op_blk_num);
    op_release(&tmp_a);
    op_release(&tmp_b);
    op_release(&tmp_r);
    op_check_res();
error:
    aca_op_free(&tmp_a);
    aca_op_free(&tmp_b);
    aca_op_free(&tmp_r);
    return ret;
}
#endif /* CE_LITE_ECP || CE_LITE_SM2DSA_SIGN */

int32_t
aca_op_add_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n)
{
    return _aca_op_add_sub_mod(op_r, op_a, op_b, op_n, 1);
}
#endif /* CE_LITE_ECP */

#if defined(CE_LITE_SM2DSA_SIGN)
int32_t
aca_op_sub_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n)
{
    return _aca_op_add_sub_mod(op_r, op_a, op_b, op_n, 0);
}
#endif /* CE_LITE_SM2DSA_SIGN */

/**
 * MOD_INV is valid only:
 * 1. N is odd
 * 2. GCD(A, N) == 1
 * Although HW doesn't require A <= N, for easy use here we requires that A
 * bitlength <= N block bits.
 * Besides, only support N is odd.
 * Requirement:
 * 1. A bitlen <= N block bits
 * 2. N is odd.
 */
#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECDSA) || defined(CE_LITE_SM2DSA_SIGN)
int32_t aca_op_inv_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_n)
{
    int32_t ret         = CE_SUCCESS;
    aca_op_t tmp_a      = ACA_OP_INIT_DATA;
    aca_op_t tmp_n      = ACA_OP_INIT_DATA;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_n);

    PAL_ASSERT(aca_op_bitlen(op_a) <= ACA_BLOCK_NUM_TO_BITS(op_n->blk_num));

    if (aca_op_get_bit_value(op_n, 0) != 1) {
        /* return invalid mode if N is not odd. */
        ret = CE_ERROR_INVAL_MOD;
        goto error;
    }

    /* for modular N of modINV, use op_n's block bits as op bits */
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_n->blk_num);

    /* use op_n's ceil_128(N_bit_len + 16) as op block number. */
    op_blk_num = ACA_BITS_TO_BLOCK_NUM(op_bit_len + 16);

    /* init tmp_a, tmp_n, tmp_r */
    aca_op_init(&tmp_a);
    aca_op_init(&tmp_n);
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;

    /* change op_r size to op_n's block size */
    ret = op_change_size(op_r, op_n->blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    /* copy op_a to tmp_a, op_n to tmp_n */
    ret = aca_op_copy_change_size(&tmp_a, op_a, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_n, op_n, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_r, GR_USAGE_OUT);
    op_prepare(&tmp_a, GR_USAGE_IN);
    op_prepare(&tmp_n, GR_USAGE_N);
    op_prepare_tmp(0, op_blk_num);
    op_prepare_tmp(1, op_blk_num);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* call mod_inv */
    OP_EXEC_ONE_CMD(&tmp_a, NULL, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_MODINV);
    if (op_intr_is_modinv_zero()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /**
     * mod_inv is valid only if GCD(A, N) == 1, check tmp_a == 1. use 1 ^ 1 = 0
     */
    OP_EXEC_ONE_CMD_IMME_B(&tmp_a, 1, NULL, NULL, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_XOR);
    if (!op_is_last_op_xor_zero()) {
        /* XOR result != 0, tmp_a != 1 */
        ret = CE_ERROR_NOT_ACCEPTABLE;
        goto end;
    }

    /**
     * Every thing is OK, read result.
     * Here we read only tmp_r.blk_num data, skip the high 0 data.
     */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num, op_r->data);

end:
    op_release(&tmp_r);
    op_release(&tmp_a);
    op_release(&tmp_n);
    op_release_tmp(0, op_blk_num);
    op_release_tmp(1, op_blk_num);
    op_check_res();
error:
    aca_op_free(&tmp_a);
    aca_op_free(&tmp_n);
    aca_op_free(&tmp_r);
    return ret;
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING)) || (CE_LITE_ECDSA) ||   \
          (CE_LITE_SM2DSA_SIGN) */

/**
 * This function covers mul_mod and exp_mod, because they are both MULT OPs.
 * Requirement: both op_a bit length and op_b bit length <= op_n bit length
 */
static int32_t _aca_op_mul_exp_mod(aca_op_t *op_r,
                                   aca_op_t *op_a,
                                   aca_op_t *op_b,
                                   aca_op_t *op_n,
                                   int32_t is_exp_mod)
{
    int32_t ret         = CE_SUCCESS;
    aca_op_t tmp_a      = ACA_OP_INIT_DATA;
    aca_op_t tmp_b      = ACA_OP_INIT_DATA;
    aca_op_t tmp_n      = ACA_OP_INIT_DATA;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    aca_op_t tmp_np     = ACA_OP_INIT_DATA;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;

#define np (&(tmp_np))

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);
    CHECK_OP_CTX(op_n);

    op_bit_len = aca_op_bitlen(op_n);

    /* Check op_a and op_b's bit length <= op_n's bit length */
    PAL_ASSERT(aca_op_bitlen(op_a) <= op_bit_len);
    PAL_ASSERT(aca_op_bitlen(op_b) <= op_bit_len);

    /* use op_n's ceil_128(N_bit_len + 2) as op block number. This apply to
     * exp_mod and mul_mod */
    op_blk_num = ACA_BITS_TO_BLOCK_NUM(op_bit_len + 2);

    /* init tmp_a, tmp_n, tmp_r */
    aca_op_init(&tmp_a);
    aca_op_init(&tmp_b);
    aca_op_init(&tmp_n);
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;

    /* init np */
    ret = _aca_op_init_np(np);
    CHECK_SUCCESS_GOTO(ret, error);

    /* change op_r size to op_n's block size */
    ret = op_change_size(op_r, op_n->blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* copy op_a to tmp_a, op_b to tmp_b, op_n to tmp_n */
    ret = aca_op_copy_change_size(&tmp_a, op_a, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_b, op_b, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_n, op_n, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* calculate np */
    ret = op_cal_np(np, op_n, op_bit_len);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_r, GR_USAGE_OUT);
    op_prepare(&tmp_a, GR_USAGE_IN);
    op_prepare(&tmp_b, GR_USAGE_IN);
    op_prepare(&tmp_n, GR_USAGE_N);
    op_prepare(np, GR_USAGE_P);
    op_prepare_tmp(0, op_blk_num);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BIT_OP, op_bit_len);

    OP_EXEC_ONE_CMD(&tmp_a, &tmp_b, NULL, &tmp_r, ACA_LENTYPE_ID_BIT_OP,
                    is_exp_mod ? ACA_OP_MODEXP : ACA_OP_MODMUL);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }
    /* read result */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num, op_r->data);

end:
    op_release(&tmp_r);
    op_release(&tmp_a);
    op_release(&tmp_b);
    op_release(&tmp_n);
    op_release(np);
    op_release_tmp(0, op_blk_num);
    op_check_res();
error:
    aca_op_free(&tmp_a);
    aca_op_free(&tmp_b);
    aca_op_free(&tmp_n);
    aca_op_free(&tmp_r);
    aca_op_free(np);
#undef np
    return ret;
}

#if defined(CE_LITE_RSASSA)
int32_t
aca_op_exp_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_e, aca_op_t *op_n)
{
    return _aca_op_mul_exp_mod(op_r, op_a, op_e, op_n, 1);
}
#endif /* CE_LITE_RSASSA */

#if (defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)) ||             \
    defined(CE_LITE_ECP)
int32_t
aca_op_mul_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n)
{
    return _aca_op_mul_exp_mod(op_r, op_a, op_b, op_n, 0);
}
#endif /* (CE_LITE_RSASSA_SIGN) && (CE_LITE_BLINDING) || (CE_LITE_ECP) */

/**
 * Call SHIFT_D directly.
 * Requirements: shift_num <= 64
 */
#if defined(CE_LITE_ECDSA)
int32_t aca_op_shift_r(aca_op_t *op_a, int32_t shift_num)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;

    CHECK_OP_CTX(op_a);
    /* supports max of 64bit shift */
    PAL_ASSERT(shift_num <= 64);

    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_a->blk_num);

    /* prepare ops */
    op_prepare(op_a, GR_USAGE_INOUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    OP_EXEC_ONE_CMD_SHIFT(op_a, shift_num - 1, op_a, ACA_LENTYPE_ID_BLK_OP,
                          ACA_OP_SHR0);
    /* read result */
    sram_read_data(op_a->sram_addr, op_a->blk_num, op_a->data);

    op_release(op_a);
    op_check_res();
    return ret;
}
#endif /* CE_LITE_ECDSA */

/**
 * Call MOD_RED directly.
 *
 * Requirements: op_a bit length < op_n's bit length.
 */
#if defined(CE_LITE_ECDSA)
int32_t aca_op_mod_red(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_n)
{
    int32_t ret         = CE_SUCCESS;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    aca_op_t tmp_a      = ACA_OP_INIT_DATA;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_n);

    /**
     * checks op_a bit length <= op_n blk bits.
     */
    PAL_ASSERT(aca_op_bitlen(op_a) <= aca_op_bitlen(op_n));

    /* use op_n's block bits as op bits */
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_n->blk_num);
    op_blk_num = op_n->blk_num;

    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;
    aca_op_init(&tmp_a);

    /* reset op_r size if necessary */
    ret = op_change_size(op_r, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    ret = aca_op_copy_change_size(&tmp_a, op_a, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_a, GR_USAGE_IN);
    op_prepare(op_n, GR_USAGE_IN);
    op_prepare_tmp(0, op_blk_num);
    op_prepare(&tmp_r, GR_USAGE_OUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* Call MODRED */
    OP_EXEC_ONE_CMD(&tmp_a, op_n, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_MODRED);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* read result */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num, op_r->data);

end:
    op_release(&tmp_r);
    op_release(&tmp_a);
    op_release(op_n);
    op_release_tmp(0, op_blk_num);
    op_check_res();
error:
    aca_op_free(&tmp_r);
    aca_op_free(&tmp_a);
    return ret;
}
#endif /* CE_LITE_ECDSA */

/************************* Used by RSA signing blinding ***********************/
#if defined(CE_LITE_RSASSA_SIGN) && defined(CE_LITE_BLINDING)
int32_t aca_op_mul(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t *tmp_ptr1 = op_a;
    aca_op_t *tmp_ptr2 = op_b;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);

    /* calculate op block number */
    op_blk_num = CE_MAX(op_a->blk_num, op_b->blk_num);
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_blk_num);

    /* init tmp_r and tmp */
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;
    aca_op_init(&tmp);

    if (op_a->blk_num < op_b->blk_num) {
        /* copy op_a to tmp */
        ret = aca_op_copy_change_size(&tmp, op_a, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, end);
        tmp_ptr1 = ( aca_op_t *)&tmp;
    } else if (op_a->blk_num > op_b->blk_num) {
        /* copy op_b to tmp */
        ret = aca_op_copy_change_size(&tmp, op_b, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, end);
        tmp_ptr2 = ( aca_op_t *)&tmp;
    }

    /* reset op_r size */
    ret = op_change_size(op_r, op_a->blk_num + op_b->blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    PAL_ASSERT((op_r->blk_num > op_blk_num) &&
                (op_r->blk_num <= 2 * op_blk_num));

    /* prepare ops */
    op_prepare(tmp_ptr1, GR_USAGE_IN);
    op_prepare(tmp_ptr2, GR_USAGE_IN);
    op_prepare(&tmp_r, GR_USAGE_OUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* MUL_LOW */
    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_MUL_LOW);

    /* read low */
    sram_read_data(tmp_r.sram_addr, op_blk_num,
                   op_r->data +
                       ACA_BLOCK_NUM_TO_BYTES(op_r->blk_num - op_blk_num));

    /* MUL_HIGH */
    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_MUL_HIGH);
    /* read high */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num - op_blk_num, op_r->data);

    op_release(tmp_ptr1);
    op_release(tmp_ptr2);
    op_release(&tmp_r);
    op_check_res();
end:
    aca_op_free(&tmp_r);
    aca_op_free(&tmp);
    return ret;
}

int32_t aca_op_add(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    int32_t is_carry    = 0;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t *tmp_ptr1 = op_a;
    aca_op_t *tmp_ptr2 = op_b;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);

    /* calculate op block number */
    op_blk_num = CE_MAX(op_a->blk_num, op_b->blk_num);
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_blk_num);

    /* init tmp_r and tmp */
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;
    aca_op_init(&tmp);

    /* reset op_r size to op_blk_num. handle carryout latter */
    ret = op_change_size(op_r, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);

    if (op_a->blk_num < op_b->blk_num) {
        /* copy op_a to tmp */
        ret = aca_op_copy_change_size(&tmp, op_a, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, end);
        tmp_ptr1 = ( aca_op_t *)&tmp;
    }

    if (op_a->blk_num > op_b->blk_num) {
        /* copy op_b to tmp */
        ret = aca_op_copy_change_size(&tmp, op_b, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, end);
        tmp_ptr2 = ( aca_op_t *)&tmp;
    }

    /* prepare ops */
    op_prepare(tmp_ptr1, GR_USAGE_IN);
    op_prepare(tmp_ptr2, GR_USAGE_IN);
    op_prepare(&tmp_r, GR_USAGE_OUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* ADD */
    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_ADD);
    is_carry = op_is_last_op_final_aul_carry();

    sram_read_data(tmp_r.sram_addr, op_blk_num, op_r->data);

    op_release(tmp_ptr1);
    op_release(tmp_ptr2);
    op_release(&tmp_r);
    op_check_res();

    /**
     * handle carry bit here, don't change size before op_release, because
     * op_a/op_b may equal to op_r
     */
    if ((CE_SUCCESS == ret) && (is_carry)) {
        ret = op_change_size(op_r, op_r->blk_num + 1);
        CHECK_SUCCESS_GOTO(ret, end);
        /* set 1 << op_bit_len to 1 */
        op_r->data[ACA_BLOCK_NUM_TO_BYTES(1) - 1] = 1;
    }
end:
    aca_op_free(&tmp_r);
    aca_op_free(&tmp);
    return ret;
}

/* return CE_ERROR_NEGATIVE_VALUE if a < b */
int32_t aca_op_sub(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b)
{
    int32_t ret         = CE_SUCCESS;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    aca_op_t tmp_r      = ACA_OP_INIT_DATA;
    aca_op_t tmp = ACA_OP_INIT_DATA;
    aca_op_t *tmp_ptr1 = op_a;
    aca_op_t *tmp_ptr2 = op_b;

    PAL_ASSERT(op_r);
    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_b);

    /* calculate op block number */
    op_blk_num = CE_MAX(op_a->blk_num, op_b->blk_num);
    op_bit_len = ACA_BLOCK_NUM_TO_BITS(op_blk_num);

    /* init tmp_r and tmp */
    aca_op_init(&tmp_r);
    tmp_r.blk_num = op_blk_num;
    tmp_r.data    = NULL;
    aca_op_init(&tmp);

    /* reset op_r size to op_blk_num. Note the overflow is handled latter */
    ret = op_change_size(op_r, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    if (op_a->blk_num < op_b->blk_num) {
        /* copy op_a to tmp */
        ret = aca_op_copy_change_size(&tmp, op_a, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr1 = ( aca_op_t *)&tmp;
    }

    if (op_a->blk_num > op_b->blk_num) {
        /* copy op_b to tmp */
        ret = aca_op_copy_change_size(&tmp, op_b, op_blk_num);
        CHECK_SUCCESS_GOTO(ret, error);
        tmp_ptr2 = ( aca_op_t *)&tmp;
    }

    /* prepare ops */
    op_prepare(tmp_ptr1, GR_USAGE_IN);
    op_prepare(tmp_ptr2, GR_USAGE_IN);
    op_prepare(&tmp_r, GR_USAGE_OUT);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, op_bit_len);

    /* SUB */
    OP_EXEC_ONE_CMD(tmp_ptr1, tmp_ptr2, NULL, &tmp_r, ACA_LENTYPE_ID_BLK_OP,
                    ACA_OP_SUB);
    if (op_is_last_op_final_aul_carry()) {
        ret = CE_ERROR_NEGATIVE_VALUE;
        goto end;
    }

    sram_read_data(tmp_r.sram_addr, op_blk_num, op_r->data);
end:
    op_release(tmp_ptr1);
    op_release(tmp_ptr2);
    op_release(&tmp_r);
    op_check_res();
error:
    aca_op_free(&tmp_r);
    aca_op_free(&tmp);
    return ret;
}

/**
 * Calculate MODEXP when E > N.
 * Algorithm:
 * R = A^E mod N
 *     A ^ E = A ^ (EM * EQ + ER) = A^(EM * EQ) * A^ER = (A^EM)^EQ * A^ER
 *
 * E = EM * EQ + ER, where:
 * EM is fixed to: 1 << (n_bit_len - 1). That is: 2^(n_bit_len - 1). This will
 * easier the E / EM by shift right.
 * By mathematical analyse, we got:
 * EQ <= (2^n_bit_len) - 1
 * ER <= (2^n_bit_len) - 1
 * so E <= 2^(n_bit_len - 1) * ((2^n_bit_len) - 1) + (2^n_bit_len) - 1
 *      <= 2^(2*n_bit_len - 1) + 2^(n_bit_len - 1) - 1
 * But actually in our code, note that when calculating EQ and ER, we use
 * sram_write_data then shift, and when write to SRAM, we assume the maximum
 * written block number < eq.block_number, and from the condition:
 *
 *     tmp_blk_num = op_e->blk_num - ACA_BITS_TO_BLOCK_NUM(CE_ROUND_DOWN(
 *                          (op_n_bit_len - 1), ACA_BLOCK_BITS));
 *     PAL_ASSERT(tmp_blk_num <= tmp_eq.blk_num);
 * We got, the maximum E bit length is:
 *
 *          2 * ROUND_DOWN((N_bit_len - 1), 128) + 128
 * Note: also require that E bit length > N bit length
 **/

int32_t aca_op_exp_mod_large_e(aca_op_t *op_r,
                               aca_op_t *op_a,
                               aca_op_t *op_e,
                               aca_op_t *op_n)
{
    int32_t ret           = CE_SUCCESS;
    aca_op_t tmp_a        = ACA_OP_INIT_DATA;
    aca_op_t tmp_er       = ACA_OP_INIT_DATA;
    aca_op_t tmp_eq       = ACA_OP_INIT_DATA;
    aca_op_t tmp_em       = ACA_OP_INIT_DATA;
    aca_op_t tmp_n        = ACA_OP_INIT_DATA;
    aca_op_t tmp_r        = ACA_OP_INIT_DATA;
    aca_op_t tmp_np       = ACA_OP_INIT_DATA;
    uint32_t op_n_bit_len = 0;
    uint32_t op_blk_num   = 0;
    uint32_t tmp_blk_num  = 0;
    uint32_t value        = 0;

#define np (&(tmp_np))

    CHECK_OP_CTX(op_a);
    CHECK_OP_CTX(op_e);
    CHECK_OP_CTX(op_n);

    op_n_bit_len = aca_op_bitlen(op_n);

    /* Check op_a's bit length <= op_n's bit length */
    PAL_ASSERT(aca_op_bitlen(op_a) <= op_n_bit_len);
    PAL_ASSERT(aca_op_bitlen(op_e) > op_n_bit_len);
    PAL_ASSERT(aca_op_bitlen(op_e) <=
                (2 * CE_ROUND_DOWN((op_n_bit_len - 1), ACA_BLOCK_BITS) +
                 ACA_BLOCK_BITS));

    /* use op_n's ceil_128(N_bit_len + 2) as op block number. This apply to
     * exp_mod and mul_mod */
    op_blk_num = ACA_BITS_TO_BLOCK_NUM(op_n_bit_len + 2);

    /* init tmp_a, tmp_n, tmp_r */
    aca_op_init(&tmp_a);
    aca_op_init(&tmp_er);
    aca_op_init(&tmp_em);
    aca_op_init(&tmp_eq);
    aca_op_init(&tmp_n);
    aca_op_init(&tmp_r);
    tmp_r.blk_num  = op_blk_num;
    tmp_r.data     = NULL;
    tmp_er.blk_num = op_blk_num;
    tmp_er.data    = NULL;
    tmp_em.blk_num = op_blk_num;
    tmp_em.data    = NULL;
    tmp_eq.blk_num = op_blk_num;
    tmp_eq.data    = NULL;

    /* init np */
    ret = _aca_op_init_np(np);
    CHECK_SUCCESS_GOTO(ret, error);

    /* copy op_a to tmp_a, op_e to tmp_e, op_n to tmp_n */
    ret = aca_op_copy_change_size(&tmp_a, op_a, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    ret = aca_op_copy_change_size(&tmp_n, op_n, op_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* change op_r size to op_n's block size */
    ret = op_change_size(op_r, op_n->blk_num);
    CHECK_SUCCESS_GOTO(ret, error);

    /* calculate np */
    ret = op_cal_np(np, op_n, op_n_bit_len);
    CHECK_SUCCESS_GOTO(ret, error);

    /* prepare ops */
    op_prepare(&tmp_r, GR_USAGE_OUT);
    op_prepare(&tmp_a, GR_USAGE_IN);
    op_prepare(&tmp_n, GR_USAGE_N);
    op_prepare(&tmp_er, GR_USAGE_INOUT);
    op_prepare(&tmp_em, GR_USAGE_INOUT);
    op_prepare(&tmp_eq, GR_USAGE_INOUT);
    op_prepare(np, GR_USAGE_P);
    op_prepare_tmp(0, op_blk_num);

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BIT_OP, op_n_bit_len);
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, ACA_BLOCK_NUM_TO_BITS(op_blk_num));

    /* Zero EM, ER, EQ */
    OP_EXEC_ONE_CMD_IMME_B(&tmp_em, 0, NULL, &tmp_em, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_AND);
    OP_EXEC_ONE_CMD_IMME_B(&tmp_er, 0, NULL, &tmp_er, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_AND);
    OP_EXEC_ONE_CMD_IMME_B(&tmp_eq, 0, NULL, &tmp_eq, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_AND);

    /* Init EM = 1 << (n_bit_len - 1), that is Set bit: n_bit_len - 1 to 1 */
    value = (0x1U << ((op_n_bit_len - 1) % 32));
    sram_write_word(tmp_em.sram_addr + 4 * ((op_n_bit_len - 1) / 32), value);

    /* EQ = E >> (n_bit_len - 1) */
    PAL_ASSERT(op_e->blk_num > ACA_BITS_TO_BLOCK_NUM(CE_ROUND_DOWN(
                                    (op_n_bit_len - 1), ACA_BLOCK_BITS)));
    tmp_blk_num = op_e->blk_num - ACA_BITS_TO_BLOCK_NUM(CE_ROUND_DOWN(
                                      (op_n_bit_len - 1), ACA_BLOCK_BITS));
    PAL_ASSERT(tmp_blk_num <= tmp_eq.blk_num);
    /* write high blocks */
    sram_write_data(tmp_eq.sram_addr, tmp_blk_num,
                    (const uint8_t *)(op_e->data));
    /* shift right if necessary */
    if (((op_n_bit_len - 1) & (ACA_BLOCK_BITS - 1)) != 0) {
        OP_EXEC_ONE_CMD_SHIFT(&tmp_eq,
                              ((op_n_bit_len - 1) & (ACA_BLOCK_BITS - 1)) - 1,
                              &tmp_eq, ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHR0);
    }

    /* ER = E & ((1 << (n_bit_len - 1)) - 1), that is read low n_bit_len - 1
     * data from E
     */
    tmp_blk_num =
        ACA_BITS_TO_BLOCK_NUM(CE_ROUND_UP(op_n_bit_len - 1, ACA_BLOCK_BITS));
    PAL_ASSERT(tmp_blk_num <= tmp_er.blk_num);
    /* write low blocks */
    sram_write_data(
        tmp_er.sram_addr, tmp_blk_num,
        (const uint8_t *)(op_e->data +
                          ACA_BLOCK_NUM_TO_BYTES(op_e->blk_num - tmp_blk_num)));
    /* adjust if necessary */
    if (((op_n_bit_len - 1) & (ACA_BLOCK_BITS - 1)) != 0) {
        value = op_blk_num - tmp_blk_num;
        while (value) {
            OP_EXEC_ONE_CMD_SHIFT(&tmp_er, ACA_BLOCK_BITS - 1, &tmp_er,
                                  ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHL0);
            value--;
        }
        OP_EXEC_ONE_CMD_SHIFT(
            &tmp_er,
            ACA_BLOCK_BITS - ((op_n_bit_len - 1) & (ACA_BLOCK_BITS - 1)) - 1,
            &tmp_er, ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHL0);
        OP_EXEC_ONE_CMD_SHIFT(
            &tmp_er,
            ACA_BLOCK_BITS - ((op_n_bit_len - 1) & (ACA_BLOCK_BITS - 1)) - 1,
            &tmp_er, ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHR0);
        value = op_blk_num - tmp_blk_num;
        while (value) {
            OP_EXEC_ONE_CMD_SHIFT(&tmp_er, ACA_BLOCK_BITS - 1, &tmp_er,
                                  ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHR0);
            value--;
        }
    }

    /* tmp_r = A^em mod N */
    OP_EXEC_ONE_CMD(&tmp_a, &tmp_em, NULL, &tmp_r, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODEXP);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* em is free, em = (A^em)^eq mod N */
    OP_EXEC_ONE_CMD(&tmp_r, &tmp_eq, NULL, &tmp_em, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODEXP);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* eq is free, eq = A^er mod N */
    OP_EXEC_ONE_CMD(&tmp_a, &tmp_er, NULL, &tmp_eq, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODEXP);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* tmp_r = eq * em mod N */
    OP_EXEC_ONE_CMD(&tmp_eq, &tmp_em, NULL, &tmp_r, ACA_LENTYPE_ID_BIT_OP,
                    ACA_OP_MODMUL);
    if (op_intr_is_mod_n_zero() || op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* read result */
    sram_read_data(tmp_r.sram_addr, op_r->blk_num, op_r->data);

end:
    op_release(&tmp_r);
    op_release(&tmp_a);
    op_release(&tmp_n);
    op_release(&tmp_em);
    op_release(&tmp_er);
    op_release(&tmp_eq);
    op_release(np);
    op_release_tmp(0, op_blk_num);
    op_check_res();
error:
    aca_op_free(&tmp_r);
    aca_op_free(&tmp_a);
    aca_op_free(&tmp_n);
    aca_op_free(&tmp_em);
    aca_op_free(&tmp_er);
    aca_op_free(&tmp_eq);
    aca_op_free(np);
#undef np
    return ret;
}
#endif /* CE_LITE_RSASSA_SIGN && CE_LITE_BLINDING */
/******************************************************************************/

#if defined(CE_LITE_ECP)

static int32_t cal_ecp_op_blk_num(int32_t op_bit_len)
{
    return ACA_BITS_TO_BLOCK_NUM(CE_ROUND_UP(op_bit_len + 2, ACA_BLOCK_BITS));
}

/**
 * Convert jacobian jx, jy, jz to affine X, Y
 * Note: jz is modified after calling this function.
 */
static int32_t jaco_to_affi(aca_op_t *P,
                            aca_op_t *np,
                            aca_op_t *jx,
                            aca_op_t *jy,
                            aca_op_t *jz,
                            aca_op_t *X,
                            aca_op_t *Y)
{
    int32_t ret         = CE_SUCCESS;
    int32_t i           = 0;
    uint32_t op_bit_len = 0;
    uint32_t op_blk_num = 0;
    aca_op_t tmp[5]     = {0};

    CHECK_OP_CTX(P);
    CHECK_OP_CTX(np);
    CHECK_OP_CTX(jx);
    CHECK_OP_CTX(jy);
    CHECK_OP_CTX(jz);
    CHECK_OP_CTX(X);
    CHECK_OP_CTX(Y);
    PAL_ASSERT(np->blk_num == ACA_NP_BLK_NUM);

    op_bit_len = aca_op_bitlen(P);
    op_blk_num = cal_ecp_op_blk_num(op_bit_len);

    /* Check block number */
    PAL_ASSERT(P->blk_num == op_blk_num);
    PAL_ASSERT(jx->blk_num == op_blk_num);
    PAL_ASSERT(jy->blk_num == op_blk_num);
    PAL_ASSERT(jz->blk_num == op_blk_num);

    op_prepare(P, GR_USAGE_N);
    op_prepare(np, GR_USAGE_P);
    op_prepare(jx, GR_USAGE_IN);
    op_prepare(jy, GR_USAGE_IN);
    op_prepare(jz, GR_USAGE_INOUT);
    op_prepare_tmp(0, op_blk_num);
    op_prepare_tmp(1, op_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_init(&(tmp[i]));
        tmp[i].blk_num = op_blk_num;
        tmp[i].data    = NULL;
        op_prepare(&tmp[i], GR_USAGE_INOUT);
    }

    /* config length type */
    op_cfg_len_type(ACA_LENTYPE_ID_BIT_OP, op_bit_len);

    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODINV, ACA_LENTYPE_ID_BIT_OP, jz, 0, 0, 0, &tmp[0]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &tmp[0], 0, &tmp[0], 0, &tmp[1]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &tmp[0], 0, &tmp[1], 0, &tmp[2]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, jx, 0, &tmp[1], 0, &tmp[3]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, jy, 0, &tmp[2], 0, &tmp[4]);
    OP_ECP_WAIT();

    if (op_intr_is_mod_n_zero() || op_intr_is_modinv_zero() ||
        op_intr_is_mult_red_err()) {
        ret = CE_ERROR_INVAL_MOD;
        goto end;
    }

    /* copy to X, Y */
    sram_read_data(tmp[3].sram_addr, X->blk_num, X->data);
    sram_read_data(tmp[4].sram_addr, Y->blk_num, Y->data);

end:
    op_release(P);
    op_release(np);
    op_release(jx);
    op_release(jy);
    op_release(jz);
    op_release_tmp(0, op_blk_num);
    op_release_tmp(1, op_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        op_release(&tmp[i]);
    }
    op_check_res();

    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_free(&(tmp[i]));
    }
    return ret;
}

static void _double_point_jj(aca_op_t *A,
                             aca_op_t *Tx,
                             aca_op_t *Ty,
                             aca_op_t *Tz,
                             aca_op_t *W,
                             bool i_mj_en,
                             bool o_mj_en,
                             aca_op_t u[])
{
    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Tx, 0, Tx, 0, &u[0]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, &u[0], 0, &u[1]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Ty, 0, Ty, 0, &u[2]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Tx, 0, &u[2], 0, &u[3]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[3], 0, &u[3], 0, &u[4]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[2], 0, &u[2], 0, &u[5]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[5], 0, &u[5], 0, &u[6]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[6], 0, &u[6], 0, &u[7]);

    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, &u[1], 0, &u[8]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[4], 0, &u[9]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[7], 0, &u[7], 0, &u[10]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Ty, 0, Tz, 0, &u[11]);
    OP_ECP_WAIT();
    OP_ECP_PREPARE();
    if (!i_mj_en) {
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Tz, 0, Tz, 0, W);
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, W, 0, W, 0, &u[12]);
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, A, 0, &u[12], 0, W);
    }
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[8], 0, W, 0, &u[0]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, &u[0], 0, &u[1]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[9], 0, &u[9], 0, &u[2]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[1], 0, &u[2], 0, Tx);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[9], 0, Tx, 0, &u[3]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, &u[3], 0, &u[4]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[10], 0, Ty);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[11], 0, &u[11], 0, Tz);
    if (o_mj_en) {
        OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[10], 0, &u[10], 0, &u[5]);
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, W, 0, &u[5], 0, &u[6]);
        OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[6], 1, 0, 0, W);
    }
    OP_ECP_WAIT();

    return;
}

static void _add_point_ajj(aca_op_t *A,
                           aca_op_t *X,
                           aca_op_t *Y,
                           aca_op_t *Z,
                           aca_op_t *X1,
                           aca_op_t *Y1,
                           aca_op_t *W,
                           bool o_mj_en,
                           aca_op_t u[])
{
    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Z, 0, Z, 0, &u[0]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, Z, 0, &u[1]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, X1, 0, &u[2]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[1], 0, Y1, 0, &u[3]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[2], 0, X, 0, &u[4]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[4], 0, &u[5]);

    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, Z, 0, &u[7]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[3], 0, Y, 0, &u[8]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[8], 0, &u[8], 0, &u[9]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[5], 0, &u[10]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[5], 0, X, 0, &u[11]);
    OP_ECP_WAIT();
    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[11], 0, &u[11], 0, &u[0]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[10], 0, &u[0], 0, &u[1]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[9], 0, &u[1], 0, X);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[11], 0, X, 0, &u[2]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[8], 0, &u[2], 0, &u[3]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[10], 0, Y, 0, &u[4]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[3], 0, &u[4], 0, Y);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[7], 1, 0, 0, Z);

    if (o_mj_en) {
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Z, 0, Z, 0, &u[5]);
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[5], 0, &u[5], 0, &u[6]);
        OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[6], 0, A, 0, W);
    }
    OP_ECP_WAIT();

    return;
}

static void _add_point_jjj(aca_op_t *P,
                           aca_op_t *X,
                           aca_op_t *Y,
                           aca_op_t *Z,
                           aca_op_t *X1,
                           aca_op_t *Y1,
                           aca_op_t *Z1,
                           aca_op_t *X2,
                           aca_op_t *Y2,
                           aca_op_t *Z2,
                           aca_op_t u[])
{
    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Z2, 0, Z2, 0, &u[0]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Z1, 0, Z1, 0, &u[1]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, Z2, 0, &u[2]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[1], 0, Z1, 0, &u[3]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, X1, 0, &u[4]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[1], 0, X2, 0, &u[5]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[2], 0, Y1, 0, &u[6]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[3], 0, Y2, 0, &u[7]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, Z1, 0, Z2, 0, &u[8]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[5], 0, &u[9]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[6], 0, &u[7], 0, &u[10]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[5], 0, &u[11]);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, &u[6], 0, &u[7], 0, &u[12]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[8], 0, &u[9], 0, Z);
    OP_ECP_WAIT();
    OP_ECP_PREPARE();
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[10], 0, &u[10], 0, &u[0]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[9], 0, &u[9], 0, &u[1]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[11], 0, &u[1], 0, &u[2]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[0], 0, &u[2], 0, X);
    OP_ECP_EXEC(MODADD, ACA_LENTYPE_ID_BIT_OP, X, 0, X, 0, &u[3]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[2], 0, &u[3], 0, &u[4]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[4], 0, &u[10], 0, &u[5]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[1], 0, &u[9], 0, &u[6]);
    OP_ECP_EXEC(MODMUL, ACA_LENTYPE_ID_BIT_OP, &u[12], 0, &u[6], 0, &u[7]);
    OP_ECP_EXEC(MODSUB, ACA_LENTYPE_ID_BIT_OP, &u[5], 0, &u[7], 0, &u[8]);
    OP_EXEC_ONE_CMD_SHIFT(P, 0,  &u[9], ACA_LENTYPE_ID_BLK_OP, ACA_OP_SHR0);
    OP_ECP_EXEC(MODMULACC, ACA_LENTYPE_ID_BIT_OP, &u[8], 0, &u[9], &u[8], Y);

    OP_ECP_WAIT();

    return;
}

/**
 * calculate ecp point mul: R = k * G
 */
static int32_t ecp_mul(aca_op_t *P,
                       aca_op_t *np,
                       aca_op_t *A,
                       aca_op_t *G_X,
                       aca_op_t *G_Y,
                       aca_op_t *G_Z,
                       aca_op_t *k,
                       aca_op_t *R_X,
                       aca_op_t *R_Y,
                       aca_op_t *R_Z)
{
    int32_t ret           = CE_SUCCESS;
    aca_op_t neg_Y        = ACA_OP_INIT_DATA;
    aca_op_t *used_y      = NULL;
    aca_op_t tmp_k        = ACA_OP_INIT_DATA;
    aca_op_t k3           = ACA_OP_INIT_DATA;
    aca_op_t Tx           = ACA_OP_INIT_DATA;
    aca_op_t Ty           = ACA_OP_INIT_DATA;
    aca_op_t Tz           = ACA_OP_INIT_DATA;
    aca_op_t W            = ACA_OP_INIT_DATA;
    aca_op_t u[13]         = {0};
    uint32_t mul_bit_len  = 0;
    uint32_t full_bit_len = 0;
    uint32_t full_blk_num = 0;
    uint32_t k_bit_len = 0, k3_bit_len = 0;
    uint32_t k_bit_val = 0, k3_bit_val = 0;
    int32_t i = 0;

#ifdef ECP_DBG_PRINT_SIMPLE_EN
    PAL_LOG_INFO(
        "ECP MUL START ++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    aca_op_dump("P", P);
    aca_op_dump("np", np);
    aca_op_dump("A", A);
    aca_op_dump("G_X", G_X);
    aca_op_dump("G_Y", G_Y);
    aca_op_dump("G_Z", G_Z);
    aca_op_dump("k", k);
#endif

    CHECK_OP_CTX(P);
    CHECK_OP_CTX(np);
    CHECK_OP_CTX(A);
    CHECK_OP_CTX(G_X);
    CHECK_OP_CTX(G_Y);
    CHECK_OP_CTX(G_Z);
    CHECK_OP_CTX(k);
    CHECK_OP_CTX(R_X);
    CHECK_OP_CTX(R_Y);
    CHECK_OP_CTX(R_Z);

    mul_bit_len = aca_op_bitlen(P);
    k_bit_len   = aca_op_bitlen(k);
    PAL_ASSERT(k_bit_len <= mul_bit_len);
    full_blk_num = cal_ecp_op_blk_num(mul_bit_len);
    full_bit_len = ACA_BLOCK_NUM_TO_BITS(full_blk_num);
    /* Check that all input meet required size */
    PAL_ASSERT(np->blk_num == ACA_NP_BLK_NUM);
    PAL_ASSERT(P->blk_num == full_blk_num);
    PAL_ASSERT(A->blk_num == full_blk_num);
    PAL_ASSERT(G_X->blk_num == full_blk_num);
    PAL_ASSERT(G_Y->blk_num == full_blk_num);
    PAL_ASSERT(G_Z->blk_num == full_blk_num);

    /* init tmp_op_ctx u */
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        aca_op_init(&(u[i]));
    }

    /* init k3 */
    k3.blk_num = full_blk_num;
    k3.data    = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(k3.blk_num));
    if (!k3.data) {
        PAL_LOG_ERR("Calloc %d failed!\n", ACA_BLOCK_NUM_TO_BYTES(k3.blk_num));
        ret = CE_ERROR_OOM;
        goto error;
    }

    ret = aca_op_copy_change_size(&tmp_k, k, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, error);
    /* calculate k3 */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, full_bit_len);
    op_prepare(&tmp_k, GR_USAGE_IN);
    op_prepare(&k3, GR_USAGE_OUT);
#ifdef ECP_DBG_PRINT_SIMPLE_EN
        aca_op_dump("tmp_k:", &tmp_k);
#endif

    OP_EXEC_ONE_CMD_IMME_B(&tmp_k, 3, NULL, &k3, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_MUL_LOW);

    sram_read_data(k3.sram_addr, k3.blk_num, k3.data);
#ifdef ECP_DBG_PRINT_SIMPLE_EN
    aca_op_dump("k3:", &k3);
#endif

    /* here we can safely release k and k3, because they have been swapped */
    op_release(&tmp_k);
    op_release(&k3);
    op_check_res();
    /* calc k3 bitlen */
    k3_bit_len = aca_op_bitlen(&k3);

    /* check k3 bitlen vs k bitlen */
    PAL_ASSERT((k3_bit_len >= k_bit_len + 1) && (k3_bit_len <= k_bit_len + 2));
    /* ECP MUL(m=1,G) makes k3_bit_len=2 */
    PAL_ASSERT(k3_bit_len >= 2);

    /* init internal used GRs' block number */
    neg_Y.blk_num = full_blk_num;
    Tx.blk_num    = full_blk_num;
    Ty.blk_num    = full_blk_num;
    Tz.blk_num    = full_blk_num;
    W.blk_num     = full_blk_num;

    /* init u block number */
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        u[i].blk_num = full_blk_num;
    }

    /* prepare two length types */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, full_bit_len);
    op_cfg_len_type(ACA_LENTYPE_ID_BIT_OP, mul_bit_len);

    /**
     * Init Tx, Ty, Tz, W, neg_Y. Set P usage to N,
     * because calculating W may use MODMUL.
     *
     * Note: Here we don't allocate cached buffer
     * (swapped memory) for saving the internal used GRs (such as neg_Y),
     * so we MUST NOT release these resources. For easy using, here we
     * make all common used GRs as INOUT.
     **/
    op_prepare(G_X, GR_USAGE_INOUT);
    op_prepare(&Tx, GR_USAGE_INOUT);
    op_prepare(G_Y, GR_USAGE_INOUT);
    op_prepare(&Ty, GR_USAGE_INOUT);
    op_prepare(G_Z, GR_USAGE_INOUT);
    op_prepare(&Tz, GR_USAGE_INOUT);
    op_prepare(A, GR_USAGE_INOUT);
    op_prepare(&W, GR_USAGE_INOUT);
    op_prepare(P, GR_USAGE_N);
    op_prepare(&neg_Y, GR_USAGE_INOUT);
    op_prepare(np, GR_USAGE_P);
    op_prepare_tmp(0, full_blk_num);
    op_prepare_tmp(1, full_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        op_prepare(&u[i], GR_USAGE_INOUT);
    }

    /* copy G_X-->Tx, G_Y-->Ty, G_Z-->Tz */
    OP_EXEC_ONE_CMD_IMME_B(G_X, 0, NULL, &Tx, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_ADD);
    OP_EXEC_ONE_CMD_IMME_B(G_Y, 0, NULL, &Ty, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_ADD);
    OP_EXEC_ONE_CMD_IMME_B(G_Z, 0, NULL, &Tz, ACA_LENTYPE_ID_BLK_OP,
                           ACA_OP_ADD);
    /* W = A * Z^4. Here the caller makes sure Z ==1, so W = A */
    OP_EXEC_ONE_CMD_IMME_B(A, 0, NULL, &W, ACA_LENTYPE_ID_BLK_OP, ACA_OP_ADD);

    /* neg_Y = P - G_Y */
    OP_EXEC_ONE_CMD(P, G_Y, NULL, &neg_Y, ACA_LENTYPE_ID_BLK_OP, ACA_OP_SUB);

#ifdef ECP_DBG_PRINT_SIMPLE_EN
    aca_op_dump("neg_Y:", &neg_Y);
#endif
    /* parse binary NAF */
    for (i = k3_bit_len - 2; i >= 1; i--) {
        k3_bit_val = aca_op_get_bit_value(&k3, i);
        if (i >= k_bit_len) {
            k_bit_val = 0;
        } else {
            k_bit_val = aca_op_get_bit_value(k, i);
        }

        if (k3_bit_val == k_bit_val) {
            /* 0 */
            _double_point_jj(A, &Tx, &Ty, &Tz, &W, true, true, u);
        } else {
            /* +- */
            _double_point_jj(A, &Tx, &Ty, &Tz, &W, true, false, u);
        }

        if ((k3_bit_val == 1) && (k_bit_val == 0)) {
            /* + */
            used_y = G_Y;
        } else if ((k3_bit_val == 0) && (k_bit_val == 1)) {
            /* - */
            used_y = &neg_Y;
        } else {
            used_y = NULL;
        }
        if (used_y) {
            _add_point_ajj(A, &Tx, &Ty, &Tz, G_X, used_y, &W, true, u);
        } else {
            /*do not support blinding here, no do nothing*/
        }
    }

    /* Copy to R_X, R_Y, R_Z */
    sram_read_data(Tx.sram_addr, R_X->blk_num, R_X->data);
    sram_read_data(Ty.sram_addr, R_Y->blk_num, R_Y->data);
    sram_read_data(Tz.sram_addr, R_Z->blk_num, R_Z->data);

    op_release(G_X);
    op_release(&Tx);
    op_release(G_Y);
    op_release(&Ty);
    op_release(G_Z);
    op_release(&Tz);
    op_release(A);
    op_release(&W);
    op_release(P);
    op_release(&neg_Y);
    op_release(np);
    op_release_tmp(0, full_blk_num);
    op_release_tmp(1, full_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        op_release(&u[i]);
    }
    op_check_res();
error:
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        aca_op_free(&(u[i]));
    }
    aca_op_free(&tmp_k);
    aca_op_free(&k3);
#ifdef ECP_DBG_PRINT_SIMPLE_EN
    aca_op_dump("R_X", R_X);
    aca_op_dump("R_Y", R_Y);
    aca_op_dump("R_Z", R_Z);
    PAL_LOG_INFO(
        "ECP MUL END ----------------------------------------------------\n");
#endif
    return ret;
}

/**
 * Calculate ecp point add: R = G1 + G2
 */
static int32_t ecp_add(aca_op_t *P,
                       aca_op_t *np,
                       aca_op_t *G1_X,
                       aca_op_t *G1_Y,
                       aca_op_t *G1_Z,
                       aca_op_t *G2_X,
                       aca_op_t *G2_Y,
                       aca_op_t *G2_Z,
                       aca_op_t *R_X,
                       aca_op_t *R_Y,
                       aca_op_t *R_Z)
{
    int32_t ret           = CE_SUCCESS;
    aca_op_t Tx           = ACA_OP_INIT_DATA;
    aca_op_t Ty           = ACA_OP_INIT_DATA;
    aca_op_t Tz           = ACA_OP_INIT_DATA;
    aca_op_t u[13]         = {0};
    uint32_t mul_bit_len  = 0;
    uint32_t full_bit_len = 0;
    uint32_t full_blk_num = 0;
    size_t i              = 0;

#ifdef ECP_DBG_PRINT_SIMPLE_EN
    PAL_LOG_INFO(
        "ECP ADD START ++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    aca_op_dump("P", P);
    aca_op_dump("np", np);
    aca_op_dump("G1_X", G1_X);
    aca_op_dump("G1_Y", G1_Y);
    aca_op_dump("G1_Z", G1_Z);
    aca_op_dump("G2_X", G2_X);
    aca_op_dump("G2_Y", G2_Y);
    aca_op_dump("G2_Z", G2_Z);
#endif

    CHECK_OP_CTX(P);
    CHECK_OP_CTX(np);
    CHECK_OP_CTX(G1_X);
    CHECK_OP_CTX(G1_Y);
    CHECK_OP_CTX(G1_Z);
    CHECK_OP_CTX(G2_X);
    CHECK_OP_CTX(G2_Y);
    CHECK_OP_CTX(G2_Z);
    CHECK_OP_CTX(R_X);
    CHECK_OP_CTX(R_Y);
    CHECK_OP_CTX(R_Z);

    mul_bit_len = aca_op_bitlen(P);

    full_blk_num = cal_ecp_op_blk_num(mul_bit_len);
    full_bit_len = ACA_BLOCK_NUM_TO_BITS(full_blk_num);

    /* Check that all input meet required size */
    PAL_ASSERT(np->blk_num == ACA_NP_BLK_NUM);
    PAL_ASSERT(P->blk_num == full_blk_num);
    PAL_ASSERT(G1_X->blk_num == full_blk_num);
    PAL_ASSERT(G1_Y->blk_num == full_blk_num);
    PAL_ASSERT(G1_Z->blk_num == full_blk_num);
    PAL_ASSERT(G2_X->blk_num == full_blk_num);
    PAL_ASSERT(G2_Y->blk_num == full_blk_num);
    PAL_ASSERT(G2_Z->blk_num == full_blk_num);

    /* init tmp_op_ctx u */
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        aca_op_init(&(u[i]));
    }

    /* init internal used GRs' block number */
    Tx.blk_num  = full_blk_num;
    Ty.blk_num  = full_blk_num;
    Tz.blk_num  = full_blk_num;

    /* init u block number */
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        u[i].blk_num = full_blk_num;
    }

    /* prepare two length types */
    op_cfg_len_type(ACA_LENTYPE_ID_BLK_OP, full_bit_len);

    /* Same as ECP MUL, prepare all used GRs */
    op_prepare(G1_X, GR_USAGE_INOUT);
    op_prepare(&Tx, GR_USAGE_INOUT);
    op_prepare(G1_Y, GR_USAGE_INOUT);
    op_prepare(&Ty, GR_USAGE_INOUT);
    op_prepare(G1_Z, GR_USAGE_INOUT);
    op_prepare(&Tz, GR_USAGE_INOUT);
    op_prepare(P, GR_USAGE_N);
    op_prepare(G2_X, GR_USAGE_INOUT);
    op_prepare(G2_Y, GR_USAGE_INOUT);
    op_prepare(G2_Z, GR_USAGE_INOUT);
    op_prepare(np, GR_USAGE_P);
    op_prepare_tmp(0, full_blk_num);
    op_prepare_tmp(1, full_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        op_prepare(&u[i], GR_USAGE_INOUT);
    }

    /* call jacobin point addition */
    _add_point_jjj(P, &Tx, &Ty, &Tz, G1_X, G1_Y, G1_Z, G2_X, G2_Y, G2_Z, u);

    /* Copy to R_X, R_Y, R_Z */
    sram_read_data(Tx.sram_addr, R_X->blk_num, R_X->data);
    sram_read_data(Ty.sram_addr, R_Y->blk_num, R_Y->data);
    sram_read_data(Tz.sram_addr, R_Z->blk_num, R_Z->data);

    op_release(G1_X);
    op_release(&Tx);
    op_release(G1_Y);
    op_release(&Ty);
    op_release(G1_Z);
    op_release(&Tz);
    op_release(P);
    op_release(G2_X);
    op_release(G2_Y);
    op_release(G2_Z);
    op_release(np);
    op_release_tmp(0, full_blk_num);
    op_release_tmp(1, full_blk_num);
    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        op_release(&u[i]);
    }
    op_check_res();

    for (i = 0; i < CE_ARRAY_SIZE(u); i++) {
        aca_op_free(&(u[i]));
    }
#ifdef ECP_DBG_PRINT_SIMPLE_EN
    aca_op_dump("R_X", R_X);
    aca_op_dump("R_Y", R_Y);
    aca_op_dump("R_Z", R_Z);
    PAL_LOG_INFO(
        "ECP ADD END ----------------------------------------------------\n");
#endif
    return ret;
}

#if defined(CE_LITE_ECP_PRIVATE)
/* Calculate ecp point mul: R = G * k*/
int32_t aca_op_ecp_mul(aca_op_t *P,
                       aca_op_t *A,
                       aca_op_t *G_X,
                       aca_op_t *G_Y,
                       aca_op_t *G_Z,
                       aca_op_t *k,
                       aca_op_t *R_X,
                       aca_op_t *R_Y,
                       aca_op_t *R_Z)
{
    int32_t ret           = CE_SUCCESS;
    aca_op_t tmp_P        = ACA_OP_INIT_DATA;
    aca_op_t tmp_A        = ACA_OP_INIT_DATA;
    aca_op_t tmp_G_X      = ACA_OP_INIT_DATA;
    aca_op_t tmp_G_Y      = ACA_OP_INIT_DATA;
    aca_op_t tmp_G_Z      = ACA_OP_INIT_DATA;
    aca_op_t tmp_np       = ACA_OP_INIT_DATA;
    aca_op_t tmp[3]       = {0};
    uint32_t p_bit_len    = 0;
    uint32_t full_blk_num = 0;
    int32_t i             = 0;

#define np (&(tmp_np))
#define RR_X (&(tmp[0]))
#define RR_Y (&(tmp[1]))
#define RR_Z (&(tmp[2]))

    CHECK_OP_CTX(P);
    CHECK_OP_CTX(A);
    CHECK_OP_CTX(G_X);
    CHECK_OP_CTX(G_Y);
    CHECK_OP_CTX(G_Z);
    CHECK_OP_CTX(k);

    /* get p length */
    p_bit_len    = aca_op_bitlen(P);
    full_blk_num = cal_ecp_op_blk_num(p_bit_len);
    /* check G_Z equals to 1 */
    ret = aca_op_bitlen(G_Z);
    PAL_ASSERT(ret == 1);
    /* init tmp_P, tmp_A, tmp_G_X, tmp_G_Y, tmp_G_Z */
    aca_op_init(&tmp_P);
    aca_op_init(&tmp_A);
    aca_op_init(&tmp_G_X);
    aca_op_init(&tmp_G_Y);
    aca_op_init(&tmp_G_Z);

    /* init np */
    ret = _aca_op_init_np(np);
    CHECK_SUCCESS_GOTO(ret, end);
    /* init tmp ops */
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_init(&(tmp[i]));
        tmp[i].blk_num = full_blk_num;
        tmp[i].data    = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(full_blk_num));
        if (NULL == tmp[i].data) {
            PAL_LOG_ERR("Calloc %d failed!\n",
                         ACA_BLOCK_NUM_TO_BYTES(full_blk_num));
            ret = CE_ERROR_OOM;
            goto end;
        }
    }
    /* copy P, A, G_X, G_Y, G_Z. */
    ret = aca_op_copy_change_size(&tmp_P, P, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_A, A, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G_X, G_X, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G_Y, G_Y, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G_Z, G_Z, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    /* adjust R_X, R_Y to P block_size, not include R_Z */
    ret = op_change_size(R_X, P->blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = op_change_size(R_Y, P->blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    /* calculate np */
    ret = op_cal_np(np, &tmp_P, p_bit_len);
    CHECK_SUCCESS_GOTO(ret, end);
    /* calculate RR = k * G */
    ret = ecp_mul(&tmp_P, np, &tmp_A, &tmp_G_X, &tmp_G_Y, &tmp_G_Z, k, RR_X,
                  RR_Y, RR_Z);
    CHECK_SUCCESS_GOTO(ret, end);
    /* convert jacobian to affine */
    ret = jaco_to_affi(&tmp_P, np, RR_X, RR_Y, RR_Z, R_X, R_Y);
    CHECK_SUCCESS_GOTO(ret, end);
    /* set R_Z to 1 */
    ret = aca_op_import_u32(R_Z, 1);
    CHECK_SUCCESS_GOTO(ret, end);
end:
    aca_op_free(np);
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_free(&(tmp[i]));
    }
    aca_op_free(&tmp_P);
    aca_op_free(&tmp_A);
    aca_op_free(&tmp_G_X);
    aca_op_free(&tmp_G_Y);
    aca_op_free(&tmp_G_Z);
    return ret;
#undef np
#undef RR_X
#undef RR_Y
#undef RR_Z
}
#endif /* CE_LITE_ECP_PRIVATE */

/* Calculate ecp point mul add: R = G1 * k1 + G2 * k2 */
int32_t aca_op_ecp_muladd(aca_op_t *P,
                          aca_op_t *A,
                          aca_op_t *G1_X,
                          aca_op_t *G1_Y,
                          aca_op_t *G1_Z,
                          aca_op_t *k1,
                          aca_op_t *G2_X,
                          aca_op_t *G2_Y,
                          aca_op_t *G2_Z,
                          aca_op_t *k2,
                          aca_op_t *R_X,
                          aca_op_t *R_Y,
                          aca_op_t *R_Z)
{
    int32_t ret           = CE_SUCCESS;
    aca_op_t tmp_P        = ACA_OP_INIT_DATA;
    aca_op_t tmp_A        = ACA_OP_INIT_DATA;
    aca_op_t tmp_G1_X     = ACA_OP_INIT_DATA;
    aca_op_t tmp_G1_Y     = ACA_OP_INIT_DATA;
    aca_op_t tmp_G1_Z     = ACA_OP_INIT_DATA;
    aca_op_t tmp_G2_X     = ACA_OP_INIT_DATA;
    aca_op_t tmp_G2_Y     = ACA_OP_INIT_DATA;
    aca_op_t tmp_G2_Z     = ACA_OP_INIT_DATA;
    aca_op_t tmp_np       = ACA_OP_INIT_DATA;
    aca_op_t tmp[9]       = {0};
    uint32_t p_bit_len    = 0;
    uint32_t full_blk_num = 0;
    int32_t i             = 0;

#define np (&(tmp_np))
#define R1_X (&(tmp[0]))
#define R1_Y (&(tmp[1]))
#define R1_Z (&(tmp[2]))
#define R2_X (&(tmp[3]))
#define R2_Y (&(tmp[4]))
#define R2_Z (&(tmp[5]))
#define RR_X (&(tmp[6]))
#define RR_Y (&(tmp[7]))
#define RR_Z (&(tmp[8]))

    CHECK_OP_CTX(P);
    CHECK_OP_CTX(A);
    CHECK_OP_CTX(G1_X);
    CHECK_OP_CTX(G1_Y);
    CHECK_OP_CTX(G1_Z);
    CHECK_OP_CTX(k1);
    CHECK_OP_CTX(G2_X);
    CHECK_OP_CTX(G2_Y);
    CHECK_OP_CTX(G2_Z);
    CHECK_OP_CTX(k2);

    /* get p length */
    p_bit_len    = aca_op_bitlen(P);
    full_blk_num = cal_ecp_op_blk_num(p_bit_len);

    /* Check G1_Z and G2_Z, must be 1 */
    ret = aca_op_bitlen(G1_Z);
    PAL_ASSERT(ret == 1);
    ret = aca_op_bitlen(G2_Z);
    PAL_ASSERT(ret == 1);

    /* init tmp_P, tmp_A, tmp_G_X, tmp_G_Y, tmp_G_Z */
    aca_op_init(&tmp_P);
    aca_op_init(&tmp_A);
    aca_op_init(&tmp_G1_X);
    aca_op_init(&tmp_G1_Y);
    aca_op_init(&tmp_G1_Z);
    aca_op_init(&tmp_G2_X);
    aca_op_init(&tmp_G2_Y);
    aca_op_init(&tmp_G2_Z);

    /* init np */
    ret = _aca_op_init_np(np);
    CHECK_SUCCESS_GOTO(ret, end);

    /* init tmp ops */
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_init(&(tmp[i]));
        tmp[i].blk_num = full_blk_num;
        tmp[i].data    = pal_calloc(1, ACA_BLOCK_NUM_TO_BYTES(full_blk_num));
        if (NULL == tmp[i].data) {
            PAL_LOG_ERR("Calloc %d failed!\n",
                         ACA_BLOCK_NUM_TO_BYTES(full_blk_num));
            ret = CE_ERROR_OOM;
            goto end;
        }
    }

    /* copy P, A, G1_X, G1_Y, G1_Z, G2_X, G2_Y, G2_Z. */
    ret = aca_op_copy_change_size(&tmp_P, P, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_A, A, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G1_X, G1_X, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G1_Y, G1_Y, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G1_Z, G1_Z, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G2_X, G2_X, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G2_Y, G2_Y, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = aca_op_copy_change_size(&tmp_G2_Z, G2_Z, full_blk_num);
    CHECK_SUCCESS_GOTO(ret, end);

    /* adjust R_X, R_Y to P block_size, not include R_Z */
    ret = op_change_size(R_X, P->blk_num);
    CHECK_SUCCESS_GOTO(ret, end);
    ret = op_change_size(R_Y, P->blk_num);
    CHECK_SUCCESS_GOTO(ret, end);

    /* calculate np */
    ret = op_cal_np(np, &tmp_P, p_bit_len);
    CHECK_SUCCESS_GOTO(ret, end);

    /* calculate R1 = k1 * G1 */
    ret = ecp_mul(&tmp_P, np, &tmp_A, &tmp_G1_X, &tmp_G1_Y, &tmp_G1_Z, k1, R1_X,
                  R1_Y, R1_Z);
    CHECK_SUCCESS_GOTO(ret, end);

    /* calculate R2 = k2 * G2 */
    ret = ecp_mul(&tmp_P, np, &tmp_A, &tmp_G2_X, &tmp_G2_Y, &tmp_G2_Z, k2, R2_X,
                  R2_Y, R2_Z);
    CHECK_SUCCESS_GOTO(ret, end);

    /* calculate RR = R1 + R2 */
    ret = ecp_add(&tmp_P, np, R1_X, R1_Y, R1_Z, R2_X, R2_Y, R2_Z, RR_X, RR_Y,
                  RR_Z);
    CHECK_SUCCESS_GOTO(ret, end);

    /* convert jacobian to affine */
    ret = jaco_to_affi(&tmp_P, np, RR_X, RR_Y, RR_Z, R_X, R_Y);
    CHECK_SUCCESS_GOTO(ret, end);
    /* set R_Z to 1 */
    ret = aca_op_import_u32(R_Z, 1);
    CHECK_SUCCESS_GOTO(ret, end);
end:
    aca_op_free(np);
    for (i = 0; i < CE_ARRAY_SIZE(tmp); i++) {
        aca_op_free(&(tmp[i]));
    }
    aca_op_free(&tmp_P);
    aca_op_free(&tmp_A);
    aca_op_free(&tmp_G1_X);
    aca_op_free(&tmp_G1_Y);
    aca_op_free(&tmp_G1_Z);
    aca_op_free(&tmp_G2_X);
    aca_op_free(&tmp_G2_Y);
    aca_op_free(&tmp_G2_Z);
    return ret;
#undef np
#undef R1_X
#undef R1_Y
#undef R1_Z
#undef R2_X
#undef R2_Y
#undef R2_Z
#undef RR_X
#undef RR_Y
#undef RR_Z
}
#endif /* CE_LITE_ECP */

#endif /* CE_LITE_BN */
