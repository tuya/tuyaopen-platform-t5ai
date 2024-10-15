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

#ifndef __CE_ACA_H__
#define __CE_ACA_H__

#include "ce_lite_config.h"
#include "ce_lite_common.h"
#include "ce_config.h"
#include "ce_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The base block size in bit of ACA engine. */
#define ACA_BLOCK_BITS (64)

/* The maximum operation size in bit of ACA engine. */
#define ACA_MAX_OP_BITS (8000)

/* The start address of ACA SRAM */
#define ACA_SRAM_BASE (0x00000000)

/* The size of total ACA SRAM, get from ce_hw_cfg */
#define ACA_SRAM_SIZE ((ce_hw_cfg.cfg.aca_sram_size == 0)            \
                      ? 4096 : ((ce_hw_cfg.cfg.aca_sram_size == 1)  \
                      ? 8192 : ((ce_hw_cfg.cfg.aca_sram_size == 2)  \
                      ? 16384: 0)))

/* The total GR number */
#define ACA_GR_NUMBER (32)

/* The total length type number */
#define ACA_LEN_TYPE_NUMBER (8)

/*
 * ACA registers
 */
/* The offset of grx_sram_addr(x is 0 - 31) of target host */
#define ACA_GRX_SRAM_ADDR_OFS (ACA_OFS + 0x00)

/* The offset of gr_len_typex(x is 0 - 7) of target host */
#define ACA_GR_LEN_TYPEX_OFS (ACA_OFS + 0x100)

/* The offset of n_p_t0_t1_use_grid of target host */
#define ACA_N_P_T0_T1_USE_GRID (ACA_OFS + 0x120)

/* The offset of aca_ctrl of target host */
#define ACA_ACA_CTRL (ACA_OFS + 0x124)

/* The offset of aca_entry of target host */
#define ACA_ACA_ENTRY (ACA_OFS + 0x128)

/* The offset of aca_status of target host */
#define ACA_ACA_STATUS (ACA_OFS + 0x12C)

/* The offset of sram_waddr of target host */
#define ACA_SRAM_WADDR (ACA_OFS + 0x130)

/* The offset of sram_wdata of target host */
#define ACA_SRAM_WDATA (ACA_OFS + 0x134)

/* The offset of sram_raddr of target host */
#define ACA_SRAM_RADDR (ACA_OFS + 0x138)

/* The offset of sram_rdata of target host */
#define ACA_SRAM_RDATA (ACA_OFS + 0x13C)

/* The offset of aca_intr_stat of target host */
#define ACA_ACA_INTR_STAT (ACA_OFS + 0x140)

/* The offset of aca_intr_msk of target host */
#define ACA_ACA_INTR_MSK (ACA_OFS + 0x144)

/* Start of used GR ID. always reserve GRID 0 */
#define ACA_GR_USED_START (1)

/* Invalid GR ID value */
#define ACA_GR_INVALID (-1)

/* Invalid SRAM address value */
#define ACA_SRAM_ADDR_INVALID (0xFFFFFFFFU)

/* Convert bit length to block number */
#define ACA_BITS_TO_BLOCK_NUM(bits)                                            \
    (((bits) + (ACA_BLOCK_BITS - 1)) / ACA_BLOCK_BITS)
/* Convert byte length to block number */
#define ACA_BYTES_TO_BLOCK_NUM(bytes)                                          \
    ((((bytes)*8) + (ACA_BLOCK_BITS - 1)) / ACA_BLOCK_BITS)

/* Convert block number to bit length */
#define ACA_BLOCK_NUM_TO_BITS(blk_nm) ((blk_nm)*ACA_BLOCK_BITS)

/* Convert block number to byte length */
#define ACA_BLOCK_NUM_TO_BYTES(blk_nm) (((blk_nm)*ACA_BLOCK_BITS) / 8)

/**
 * The usage of lengthtype ID is fixed to two type:
 *  1. Block based OP bits. In this usage, the OP bits is multiple of block
 *     bits. Such as ADD, SUB, MUL.
 *  2. Modulus N based OP bits. In this usage, the OP bits is the real bit
 *     length of modulus N. Such as modMUL, modEXP, modMUL7NR.
 */
/* The lengthtype ID for block based OP bits. */
#define ACA_LENTYPE_ID_BLK_OP (1)

/* The lengthtype ID for modulus N based OP bits */
#define ACA_LENTYPE_ID_BIT_OP (2)

/* The const block number for NP, always 2*/
#define ACA_NP_BLK_NUM (2)

/* The ACA OP command enumeration */
enum {
    ACA_OP_ADD          = 0x01,
    ACA_OP_SUB          = 0x02,
    ACA_OP_MUL_LOW      = 0x03,
    ACA_OP_DIV          = 0x04,
    ACA_OP_AND          = 0x05,
    ACA_OP_OR           = 0x06,
    ACA_OP_XOR          = 0x07,
    ACA_OP_SHR0         = 0x08,
    ACA_OP_SHL0         = 0x0A,
    ACA_OP_SHL1         = 0x0B,
    ACA_OP_MUL_HIGH     = 0x0C,
    ACA_OP_MODRED       = 0x10,
    ACA_OP_MODADD       = 0x11,
    ACA_OP_MODSUB       = 0x12,
    ACA_OP_MODMUL       = 0x13,
    ACA_OP_MODINV       = 0x14,
    ACA_OP_MODEXP       = 0x15,
    ACA_OP_MODMULNR     = 0x16,
    ACA_OP_MODMULACC    = 0x17,
    ACA_OP_MODMULACCNR  = 0x18,
};

/* The GR usage */
typedef enum gr_usage {
    GR_USAGE_NULL  = 0,
    GR_USAGE_IN    = 1,
    GR_USAGE_OUT   = 2,
    GR_USAGE_INOUT = 3,
    GR_USAGE_N     = 4,
    GR_USAGE_P     = 5,
    GR_USAGE_T0    = 6,
    GR_USAGE_T1    = 7,
} gr_usage_t;

/* The OP context structure. */
typedef struct ce_aca_bn_t {
    uint32_t blk_num;
    /* data in bigendian */
    uint8_t *data;
    uint32_t sram_addr;
    int32_t gr_id;
} ce_aca_bn_t;

typedef struct ce_aca_bn_t aca_op_t;

/* The init value for one OP context */
#define ACA_OP_INIT_DATA                                                       \
    {                                                                          \
        .blk_num = 0, .data = NULL, .sram_addr = ACA_SRAM_ADDR_INVALID,        \
        .gr_id = ACA_GR_INVALID,                                               \
    }

/* assert if op context is invalid. */
#define CHECK_OP_CTX(op_ctx)                                                   \
    do {                                                                       \
        PAL_ASSERT((op_ctx != NULL) && (op_ctx->data != NULL) &&              \
                    (op_ctx->blk_num != 0));                                   \
    } while (0)

int32_t aca_engine_init(void);
void aca_engine_exit(void);
void aca_op_init(aca_op_t *op);
void aca_op_free(aca_op_t *op);
int32_t aca_op_copy(aca_op_t *dst, aca_op_t *src);
int32_t aca_op_import_u32(aca_op_t *op, uint32_t val);
int32_t aca_op_import_bin(aca_op_t *op, const uint8_t *data, size_t size);
int32_t aca_op_export_bin(aca_op_t *op, uint8_t *buf, size_t size);
uint32_t aca_op_bitlen(aca_op_t *op);
int32_t aca_op_get_bit_value(aca_op_t *op, size_t bit_num);
int32_t aca_op_set_bit_value(aca_op_t *op, size_t bit_num, int32_t value);
int32_t aca_op_cmp_bn(aca_op_t *op_a, aca_op_t *op_b, int32_t *result);
int32_t aca_op_cmp_u32(aca_op_t *op_a, uint32_t b, int32_t *result);
int32_t aca_op_cmp_bn_equal(aca_op_t *op_a, aca_op_t *op_b, int32_t *result);
int32_t
aca_op_add_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n);
int32_t
aca_op_sub_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n);
int32_t aca_op_inv_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_n);
int32_t
aca_op_exp_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_e, aca_op_t *op_n);
int32_t
aca_op_mul_mod(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b, aca_op_t *op_n);
int32_t aca_op_shift_r(aca_op_t *op_a, int32_t shift_num);
int32_t aca_op_mod_red(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_n);
int32_t aca_op_mul(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b);
int32_t aca_op_add(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b);
int32_t aca_op_sub(aca_op_t *op_r, aca_op_t *op_a, aca_op_t *op_b);
int32_t aca_op_exp_mod_large_e(aca_op_t *op_r,
                               aca_op_t *op_a,
                               aca_op_t *op_e,
                               aca_op_t *op_n);
int32_t aca_op_ecp_mul(aca_op_t *P,
                       aca_op_t *A,
                       aca_op_t *G_X,
                       aca_op_t *G_Y,
                       aca_op_t *G_Z,
                       aca_op_t *k,
                       aca_op_t *R_X,
                       aca_op_t *R_Y,
                       aca_op_t *R_Z);
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
                          aca_op_t *R_Z);
void aca_op_dump(const char *name, aca_op_t *op_ctx);

#ifdef __cplusplus
}
#endif

#endif /*__CE_ACA_H__*/
