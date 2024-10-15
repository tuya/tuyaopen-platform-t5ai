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
#include "ce_cipher.h"

#if defined(CE_LITE_CIPHER)

#if defined(CE_LITE_CIPHER_MODE_CTR)
static void xor_bytes(const uint8_t *in1, const uint8_t *in2,
                      uint8_t *out, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        out[i] = in1[i] ^ in2[i];
    }

    return;
}
#endif /* CE_LITE_CIPHER_MODE_CTR */

static void sca_engine_reset(void)
{
    uint32_t ctl = 0;

    /* reset sca module */
    ctl = IO_READ32(TOP_CTL_RESET_CTL);
    ctl |= SCA_RESET;
    IO_WRITE32(TOP_CTL_RESET_CTL, ctl);
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
    ctl &= ~SCA_RESET;
    IO_WRITE32(TOP_CTL_RESET_CTL, ctl);

    /* sca run */
    ctl = IO_READ32(SCA_CTL_REG_OFS);
    ctl |= SCA_CTL_RUN;
    IO_WRITE32(SCA_CTL_REG_OFS, ctl);

    return;
}

static int32_t sca_wait_done(void)
{
    int32_t ret = CE_SUCCESS;
    int32_t stat = 0;

    while (1) {
        /* check intr stat */
        stat = (IO_READ32(SCA_INTR_STAT_REG_OFS)) & (~0x11);
        if (stat) {
            PAL_LOG_ERR("sca INIR STATE returns failure INTR status: 0x%x\n", stat);
            IO_WRITE32(SCA_INTR_STAT_REG_OFS, stat);
            ret = CE_ERROR_GENERIC;
            break;
        }
        /* check engine stat */
        stat = IO_READ32(SCA_STAT_REG_OFS);
        if (((stat & SCA_ENG_STATE) <= SCA_ENG_WAIT_COMM) &&
            ((stat >> SCA_QUE_AVAIL_SLOTS_OFS) == SCA_QUEUE_DEPTH)) {
            ret = CE_SUCCESS;
            break;
        }
    }

    return ret;
}

static int32_t sca_cmd_exec_init(ce_sca_crypt_t *sctx)
{
    int32_t ret = CE_SUCCESS;
    uint8_t load_iv = 0;
    uint32_t t, klen = 0;
    uint32_t chain_mode = CE_ALG_GET_CHAIN_MODE(sctx->alg);
    ce_algo_t algo = CE_ALG_GET_MAIN_ALG(sctx->alg);

    if (CIPHER_KEY_TYPE_USER == sctx->key.type) {
        t = SCA_EXT_KEY;
        klen = sctx->key.user.keybits;
    } else {
        if (CE_KLAD_KEY_MODEL == sctx->key.sec.sel) {
            t = SCA_MODEL_KEY;
        } else {
            t = SCA_ROOT_KEY;
        }
        klen = sctx->key.sec.ek1bits;
    }

#if defined(CE_LITE_AES) || defined(CE_LITE_SM4)
    if (128 == klen) {
        t |= SCA_KEY_128;
    }
#endif

#if defined(CE_LITE_AES)
    if (256 == klen) {
        t |= SCA_KEY_256;
    }

    if (CE_MAIN_ALGO_AES == algo) {
        t |= SCA_AES_ALG;
    }
#endif

#if defined(CE_LITE_SM4)
    if (CE_MAIN_ALGO_SM4 == algo) {
        t |= SCA_SM4_ALG;
    }
#endif

    switch (chain_mode) {
#if defined(CE_LITE_CIPHER_MODE_ECB)
    case CE_CHAIN_MODE_ECB_NOPAD:
#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
    case CE_CHAIN_MODE_ECB_PKCS7:
#endif
        t |= SCA_ECB_MODE;
        break;
#endif
#if defined(CE_LITE_CIPHER_MODE_CBC)
    case CE_CHAIN_MODE_CBC_NOPAD:
#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
    case CE_CHAIN_MODE_CBC_PKCS7:
#endif
        load_iv = 1;
        t |= SCA_CBC_MODE | SCA_LOADIV | SCA_IV_ADDR_FORMAT;
        break;
#endif
#if defined(CE_LITE_CIPHER_MODE_CTR)
    case CE_CHAIN_MODE_CTR:
        load_iv = 1;
        t |= SCA_CTR_MODE | SCA_LOADIV | SCA_IV_ADDR_FORMAT;
        break;
#endif
    default:
        PAL_ASSERT(0);
    }

    t |= SCA_KEY_ADDR_FORMAT | SCA_INIT;
    IO_WRITE32(SCA_QUE_REG_OFS, t);

    if (CIPHER_KEY_TYPE_USER == sctx->key.type) {
        t = (uint32_t)((uintptr_t)sctx->key.user.key);
        IO_WRITE32(SCA_QUE_REG_OFS, t);
    } else {
        t = (uint32_t)((uintptr_t)sctx->key.sec.ek3);
        IO_WRITE32(SCA_QUE_REG_OFS, t);
        t = (uint32_t)((uintptr_t)sctx->key.sec.ek2);
        IO_WRITE32(SCA_QUE_REG_OFS, t);
        t = (uint32_t)((uintptr_t)sctx->key.sec.ek1);
        IO_WRITE32(SCA_QUE_REG_OFS, t);
    }

    if (load_iv) {
        t = (uint32_t)((uintptr_t)sctx->iv);
        IO_WRITE32(SCA_QUE_REG_OFS, t);
    }

    ret = sca_wait_done();
    return ret;
}

static int32_t sca_cmd_exec_proc(ce_sca_crypt_t *sctx, size_t len,
                                 const uint8_t *in, uint8_t *out)
{
    int32_t ret = CE_SUCCESS;
    uint32_t t = 0;

    t = (SCA_PROC | (CE_ENCRYPT == sctx->op ? SCA_ENCRYTION : SCA_DECRYTION));
    IO_WRITE32(SCA_QUE_REG_OFS, t);

    t = (uint32_t)((uintptr_t)in);
    IO_WRITE32(SCA_QUE_REG_OFS, t);

    t = (uint32_t)(len);
    IO_WRITE32(SCA_QUE_REG_OFS, t);

    t = (uint32_t)((uintptr_t)out);
    IO_WRITE32(SCA_QUE_REG_OFS, t);

    ret = sca_wait_done();
    return ret;
}

static int32_t sca_cmd_exec_finish(ce_sca_crypt_t *sctx)
{
    int32_t ret = CE_SUCCESS;
    uint32_t t = 0;

    t = SCA_FINISH;
    IO_WRITE32(SCA_QUE_REG_OFS, t);
    ret = sca_wait_done();

    return ret;
}

#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
static int32_t ce_cipher_pkcs7_check(uint8_t *in, uint32_t *plen)
{
    int32_t ret = CE_SUCCESS;
    uint8_t pchar = *((uint8_t *)((uintptr_t)in - 1));
    uint8_t i;

    if (pchar > CE_MAX_SCA_BLOCK) {
        ret = CE_ERROR_BAD_PADDING;
        goto end;
    }

    for (i = 0; i < pchar; i++) {
        if (*((uint8_t *)((uintptr_t)in - i - 1)) != pchar) {
            ret = CE_ERROR_BAD_PADDING;
            goto end;
        }
    }

    *plen = (uint32_t)pchar;
end:
    return ret;
}
#endif /* CE_LITE_CIPHER_PADDING_PKCS7 */

static int32_t ce_cipher_prepare_to_ready(ce_sca_crypt_t *sctx)
{
    int32_t ret = CE_SUCCESS;
    uint32_t expected = CIPHER_KEY_READY_FLAG | CIPHER_OP_READY_FLAG;

    if (CE_ALG_GET_CHAIN_MODE(sctx->alg) != CE_CHAIN_MODE_ECB_NOPAD &&
        CE_ALG_GET_CHAIN_MODE(sctx->alg) != CE_CHAIN_MODE_ECB_PKCS7) {
        expected |= CIPHER_IV_READY_FLAG;
    }

    if (CIPHER_CTX_READY_STATE == sctx->state) {
        goto end;
    }

    if ((sctx->flags & expected) != expected) {
        ret = CE_ERROR_BAD_STATE;
        goto end;
    }

    ret = sca_cmd_exec_init(sctx);
    if (ret) {
        goto end;
    }

    sctx->state = CIPHER_CTX_READY_STATE;

end:
    return ret;
}

static int32_t proc_nopad(ce_sca_crypt_t *sctx, size_t ilen,
                          const uint8_t *in, uint8_t *out)
{
    int32_t ret = CE_SUCCESS;

    PAL_ASSERT(CE_IS_ALIGNED(ilen, sctx->blksz));
    ret = sca_cmd_exec_proc(sctx, ilen, in, out);
    if (ret) {
        goto end;
    }

end:
    return ret;
}

#if defined(CE_LITE_CIPHER_MODE_CTR)
static int32_t proc_stream(ce_sca_crypt_t *sctx, size_t ilen,
                           const uint8_t *in, uint8_t *out)
{
    int32_t ret = CE_SUCCESS;
    const uint8_t *ipos = in;
    uint8_t *opos = out;
    size_t remainder = 0, process = 0;
    uint8_t temp[CE_MAX_SCA_BLOCK] = {0};

    /* CTR: handle stream block */
    if (sctx->strpos != 0) {
        process = sctx->blksz - sctx->strpos;
        if (process > ilen) {
            process = ilen;
        }
        xor_bytes(ipos, &sctx->stream[sctx->strpos], opos, process);

        /* Update stream block pos */
        sctx->strpos += process;
        sctx->strpos &= (sctx->blksz - 1);

        /* Update input and output pointer */
        ipos = (const uint8_t *)((uintptr_t)ipos + process);
        opos = (uint8_t *)((uintptr_t)opos + process);
    }

    remainder = (ilen - process) & (sctx->blksz - 1ULL);
    process = CE_ROUND_DOWN(ilen - process, (size_t)sctx->blksz);

    if (process) {
        ret = sca_cmd_exec_proc(sctx, process, ipos, opos);
        if (ret) {
            goto end;
        }

        /* Update input and output pointer */
        ipos = (const uint8_t *)((uintptr_t)ipos + process);
        opos = (uint8_t *)((uintptr_t)opos + process);
    }

    if (remainder) {
        /* Update stream buffer */
        pal_memset(temp, 0x0, sizeof(temp));
        ret = sca_cmd_exec_proc(sctx, sctx->blksz, temp, sctx->stream);
        if (ret) {
            goto end;
        }
        sctx->strpos = 0;

        /* Handle the remainder of data */
        xor_bytes(ipos, &sctx->stream[sctx->strpos], opos, remainder);

        /* Update stream block pos */
        sctx->strpos = remainder;
    }
end:
    return ret;
}
#endif /* CE_LITE_CIPHER_MODE_CTR */

#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
static int32_t proc_pkcs7(ce_sca_crypt_t *sctx, bool is_last, size_t ilen,
                          const uint8_t *in, size_t *olen, uint8_t *out)
{
    int32_t ret = CE_SUCCESS;
    uint32_t plen = 0;
    const uint8_t *ipos = in;
    uint8_t *opos = out;
    size_t total = ilen, remainder = 0, process = 0;
    uint8_t temp[CE_MAX_SCA_BLOCK] = {0};
    bool encrypt = (CE_ENCRYPT == sctx->op ? true : false);

    remainder = total & (sctx->blksz - 1ULL);
    process = CE_ROUND_DOWN(total, sctx->blksz);

    if (process) {
        ret = sca_cmd_exec_proc(sctx, process, ipos, opos);
        if (ret) {
            goto end;
        }

        /* Update input and output pointer */
        ipos = (const uint8_t *)((uintptr_t)ipos + process);
        opos = (uint8_t *)((uintptr_t)opos + process);
    }

    if (encrypt && is_last) {
        plen = sctx->blksz - remainder;
        process = sctx->blksz;

        pal_memset(temp, 0x0, sizeof(temp));
        pal_memcpy(temp, ipos, remainder);
        /* Add PKCS7 padding, if plen != 0 */
        pal_memset((uint8_t *)((uintptr_t)temp + remainder), plen & 0xFFU, plen);

        ret = sca_cmd_exec_proc(sctx, process, temp, opos);
        if (ret) {
            goto end;
        }

        /* Adds padding length */
        total += plen;
    }

    /*
     * For the PKCS7 decryption case, data length must be block size aligned.
     * Thus, the remainder must be 0 here.
     */
    if ((!encrypt) && is_last) {
        PAL_ASSERT(0 == remainder);

        ret = ce_cipher_pkcs7_check(opos, &plen);

        /* Subtracts padding length */
        total -= plen;
    }

    if (olen && CE_SUCCESS == ret) {
        *olen = (size_t)total;
    }

end:
    return ret;
}
#endif /* CE_LITE_CIPHER_PADDING_PKCS7 */

int32_t ce_cipher_init(ce_cipher_ctx_t *ctx, ce_algo_t alg)
{
    int32_t ret = CE_SUCCESS;

    if (NULL == ctx) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    ctx->crypt = pal_calloc(1, sizeof(ce_sca_crypt_t));
    if (NULL == ctx->crypt) {
         ret = CE_ERROR_OOM;
         goto end;
    }

    ctx->crypt->state = CIPHER_CTX_IDLE_STATE;
    ctx->crypt->alg = alg;
    ctx->crypt->blksz = CE_MAX_SCA_BLOCK;  //aes block plaintext data size is 16 bytes

end:
    return ret;
}

void ce_cipher_free(ce_cipher_ctx_t *ctx)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt) {
        goto end;
    }

    sctx = ctx->crypt;

    if (CIPHER_CTX_IDLE_STATE == sctx->state) {
        goto free_ctx;
    }

    ret = sca_cmd_exec_finish(sctx);
    if (ret != 0) {
        /* reset if engine error */
        sca_engine_reset();
    }

free_ctx:
    pal_memset((void *)sctx, 0x0, sizeof(ce_sca_crypt_t));
    pal_free(sctx);
    ctx->crypt = NULL;
end:
    return;
}

int32_t ce_cipher_set_iv(ce_cipher_ctx_t *ctx, const uint8_t *iv)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt || NULL == iv) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

    pal_memcpy(sctx->iv, iv, sctx->blksz);
    sctx->flags |= CIPHER_IV_READY_FLAG;
end:
    return ret;
}

int32_t ce_cipher_get_iv(ce_cipher_ctx_t *ctx, uint8_t *iv)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt || NULL == iv) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

    pal_memcpy(iv, sctx->iv, sctx->blksz);
end:
    return ret;
}

int32_t ce_cipher_set_op_mode(ce_cipher_ctx_t *ctx, ce_operation_t op)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

    if (op != CE_ENCRYPT && op != CE_DECRYPT) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /* CTR: overwrites the operation mode to encryption */
    if (CE_CHAIN_MODE_CTR == CE_ALG_GET_CHAIN_MODE(sctx->alg)) {
        sctx->op = CE_ENCRYPT;
    } else {
        sctx->op = op;
    }

    sctx->flags |= CIPHER_OP_READY_FLAG;

end:
    return ret;
}

int32_t ce_cipher_setkey(ce_cipher_ctx_t *ctx,
                         const uint8_t *key, uint32_t keybits)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt || NULL == key) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

    if (keybits != 128 && keybits != 256) {
        ret = CE_ERROR_BAD_KEY_LENGTH;
        goto end;
    }

    if (CE_ALG_GET_MAIN_ALG(sctx->alg) !=
        CE_MAIN_ALGO_AES && keybits != 128) {
        ret = CE_ERROR_BAD_KEY_LENGTH;
        goto end;
    }

    sctx->key.type = CIPHER_KEY_TYPE_USER;
    pal_memcpy(sctx->key.user.key, key, keybits>>3);
    sctx->key.user.keybits = keybits;

    sctx->flags |= CIPHER_KEY_READY_FLAG;

end:
    return ret;
}

int32_t ce_cipher_setseckey(ce_cipher_ctx_t *ctx, const ce_sec_key_t *key)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;

    if (NULL == ctx || NULL == ctx->crypt || NULL == key) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

    if (CE_KLAD_KEY_MODEL != key->sel && CE_KLAD_KEY_ROOT != key->sel) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    if (key->ek1bits != 128 && key->ek1bits != 256) {
        ret = CE_ERROR_BAD_KEY_LENGTH;
        goto end;
    }

    if (CE_ALG_GET_MAIN_ALG(sctx->alg) !=
        CE_MAIN_ALGO_AES && key->ek1bits != 128) {
        ret = CE_ERROR_BAD_KEY_LENGTH;
        goto end;
    }

    sctx->key.type = CIPHER_KEY_TYPE_SEC;
    pal_memcpy((void *)&sctx->key.sec, key, sizeof(ce_sec_key_t));

    sctx->flags |= CIPHER_KEY_READY_FLAG;

end:
    return ret;
}

int32_t ce_cipher_crypt(ce_cipher_ctx_t *ctx, bool is_last, size_t ilen,
                        const uint8_t *in, size_t *olen, uint8_t *out)
{
    int32_t ret = CE_SUCCESS;
    ce_sca_crypt_t *sctx = NULL;
    bool padding = false;
    bool ctr = false;
    bool encrypt = false;

    if (NULL == ctx || NULL == ctx->crypt || ilen > (size_t)0xffffffefU) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    sctx = ctx->crypt;

#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
    if (CE_CHAIN_MODE_ECB_PKCS7 == CE_ALG_GET_CHAIN_MODE(sctx->alg) ||
        CE_CHAIN_MODE_CBC_PKCS7 == CE_ALG_GET_CHAIN_MODE(sctx->alg)) {
        padding = true;
    }
#endif

#if defined(CE_LITE_CIPHER_MODE_CTR)
    if (CE_CHAIN_MODE_CTR == CE_ALG_GET_CHAIN_MODE(sctx->alg)) {
        ctr = true;
    }
#endif

    encrypt = (CE_ENCRYPT == sctx->op ? true : false);

    /*
     * Decryption with PKCS7 padding the ilen must be
     * multiple of 16 and doesn't equal to 0.
     */
    if ((0 == ilen || (ilen & (CE_MAX_SCA_BLOCK - 1)) != 0) &&
        is_last && padding && !encrypt) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    /*
     * CBC & ECB NOPAD or PKCS7 but isn't the last process.
     * The ilen must be multiple of 16.
     */
    if ((ilen & (CE_MAX_SCA_BLOCK - 1)) != 0 &&
        (!is_last || !padding) && !ctr) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    if (NULL == in && ilen != 0) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    if (NULL == out && ilen != 0) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    if (NULL == out && is_last && padding && encrypt) {
        ret = CE_ERROR_BAD_PARAMS;
        goto end;
    }

    if (0 == ilen && (!padding || !is_last)) {
        goto end;
    }

    ret = ce_cipher_prepare_to_ready(sctx);
    if (ret) {
        goto end;
    }

#if defined(CE_LITE_CIPHER_MODE_CTR)
    if (ctr) {
        ret = proc_stream(sctx, ilen, in, out);
        if (CE_SUCCESS == ret && olen) {
            *olen = ilen;
        }
    }
#endif

#if defined(CE_LITE_CIPHER_PADDING_PKCS7)
    if (padding) {
        ret = proc_pkcs7(sctx, is_last, ilen, in, olen, out);
    }
#endif

    if (!ctr && !padding) {
        ret = proc_nopad(sctx, ilen, in, out);
        if (CE_SUCCESS == ret && olen) {
            *olen = ilen;
        }
    }

end:
    return ret;
}

int32_t sca_engine_init(void)
{
    uint32_t ctl = 0;

    /* enable sca clk */
    ctl = IO_READ32(TOP_CTL_CLOCK_CTL);
    ctl |= SCA_CLK_EN;
    IO_WRITE32(TOP_CTL_CLOCK_CTL, ctl);

    /* sca run */
    ctl = IO_READ32(SCA_CTL_REG_OFS);
    ctl |= SCA_CTL_RUN;
    IO_WRITE32(SCA_CTL_REG_OFS, ctl);

    return CE_SUCCESS;
}

void sca_engine_exit(void)
{
    uint32_t ctl = IO_READ32(TOP_CTL_CLOCK_CTL);
    ctl &= ~SCA_CLK_EN;
    IO_WRITE32(TOP_CTL_CLOCK_CTL, ctl);

    return;
}

#endif /* CE_LITE_CIPHER */