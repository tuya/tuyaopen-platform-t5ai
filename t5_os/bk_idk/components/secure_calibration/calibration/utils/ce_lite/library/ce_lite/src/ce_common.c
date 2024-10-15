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
#include "ce_common.h"
#include "ce_aca.h"
#include "ce_hash.h"
#include "ce_trng.h"
#include "ce_cipher.h"
#include "ce_otp.h"

#ifdef CONFIG_BK_BOOT
#include "pal_log_redefine.h"
#endif

ce_hw_cfg_t ce_hw_cfg = {0};

int32_t ce_drv_init(void)
{
    int32_t ret   = CE_SUCCESS;
#ifdef CE_DBG_DUMP_INIT_REGS
    uint32_t conf = 0;
#endif

    /* init CE config structure for merak_lite */
    pal_memset(&ce_hw_cfg, 0, sizeof(ce_hw_cfg));
    ce_hw_cfg.value = IO_READ32(TOP_STAT_CFG1);

#ifdef CONFIG_BK_BOOT
    PAL_LOG_DEBUG("%s (HostID: %d) - ce_hw_cfg.value = 0x%X\r\n", __FUNCTION__, CE_CFG_HOST_ID, ce_hw_cfg.value);
    PAL_LOG_DEBUG("  rn_pool_host_num = 0x%X\r\n", ce_hw_cfg.cfg.rn_pool_host_num);
    PAL_LOG_DEBUG("  aca_host_num = 0x%X\r\n", ce_hw_cfg.cfg.aca_host_num);
    PAL_LOG_DEBUG("  sca_host_num = 0x%X\r\n", ce_hw_cfg.cfg.sca_host_num);
    PAL_LOG_DEBUG("  hash_host_num = 0x%X\r\n", ce_hw_cfg.cfg.hash_host_num);	
#endif

    /* Check configured HOST ID */
    if (CE_CFG_HOST_ID + 1 > ce_hw_cfg.cfg.rn_pool_host_num) {
        PAL_LOG_ERR("Bad host id: %d, no such host for TRNG Pool!\n");
        return CE_ERROR_GENERIC;
    }
    if (CE_CFG_HOST_ID + 1 > ce_hw_cfg.cfg.aca_host_num) {
        PAL_LOG_ERR("Bad host id: %d, no such host for ACA!\n");
        return CE_ERROR_GENERIC;
    }
    if (CE_CFG_HOST_ID + 1 > ce_hw_cfg.cfg.sca_host_num) {
        PAL_LOG_ERR("Bad host id: %d, no such host for SCA!\n");
        return CE_ERROR_GENERIC;
    }
    if (CE_CFG_HOST_ID + 1 > ce_hw_cfg.cfg.hash_host_num) {
        PAL_LOG_ERR("Bad host id: %d, no such host for HASH!\n");
        return CE_ERROR_GENERIC;
    }

    /* customer configurations */
    ce_config_hw();

    /* sub-engine init */
#if defined(CE_LITE_BN)
    ret = aca_engine_init();
    PAL_ASSERT(CE_SUCCESS == ret);
#endif

#if defined(CE_LITE_HASH)
    ret = hash_engine_init();
    PAL_ASSERT(CE_SUCCESS == ret);
#endif

#if defined(CE_LITE_OTP)
    ret = ce_otp_drv_init();
    PAL_ASSERT(CE_SUCCESS == ret);
#endif

#if defined(CE_LITE_TRNG)
    ret = trng_engine_init();
    PAL_ASSERT(CE_SUCCESS == ret);
#endif

#if defined(CE_LITE_CIPHER)
    ret = sca_engine_init();
    PAL_ASSERT(CE_SUCCESS == ret);
#endif

    PAL_LOG_DEBUG("Current host ID: %d\n", CE_CFG_HOST_ID);

#ifdef CE_DBG_DUMP_INIT_REGS
    /**
     * The following dumps all the top registers of CE for debug purpose.
     */

    /* Dump TOP_CTL registers */
    conf = IO_READ32(TOP_CTL_CLOCK_CTL);
    PAL_LOG_DEBUG("TOP clock_ctrl: 0x%x\n", conf);
    PAL_LOG_DEBUG("TOP clock_ctrl:  dma_ahb_clk_en: %s\n",
                   (((conf >> 5) & 1U) == 1U) ? "Enabled" : "Disabled");
    PAL_LOG_DEBUG("TOP clock_ctrl:  trng_clk_en: %s\n",
                   (((conf >> 4) & 1U) == 1U) ? "Enabled" : "Disabled");
    PAL_LOG_DEBUG("TOP clock_ctrl:  otp_clk_en: %s\n",
                   (((conf >> 3) & 1U) == 1U) ? "Enabled" : "Disabled");
    PAL_LOG_DEBUG("TOP clock_ctrl:  aca_clk_en: %s\n",
                   (((conf >> 2) & 1U) == 1U) ? "Enabled" : "Disabled");
    PAL_LOG_DEBUG("TOP clock_ctrl:  sca_clk_en: %s\n",
                   (((conf >> 1) & 1U) == 1U) ? "Enabled" : "Disabled");
    PAL_LOG_DEBUG("TOP clock_ctrl:  hash_clk_en: %s\n",
                   (((conf >> 0) & 1U) == 1U) ? "Enabled" : "Disabled");


    conf = IO_READ32(TOP_CTL_RESET_CTL);
    PAL_LOG_DEBUG("TOP reset_ctrl: 0x%x\n", conf);

    conf = IO_READ32(TOP_CTL_TOP_CFG);
    PAL_LOG_DEBUG("TOP top_cfg: 0x%x\n", conf);
    PAL_LOG_DEBUG("TOP top_cfg:        aca_arb_algo: %s\n",
                   (((conf >> 7) & 1U) == 1U) ? "Round-robin" : "Fixed");
    PAL_LOG_DEBUG("TOP top_cfg:        aca_arb_gran: %s\n",
                   (((conf >> 6) & 1U) == 1U) ? "Command queue" : "Command");
    PAL_LOG_DEBUG("TOP top_cfg:        sca_arb_algo: %s\n",
                   (((conf >> 5) & 1U) == 1U) ? "Round-robin" : "Fixed");
    PAL_LOG_DEBUG("TOP top_cfg:        sca_arb_gran: %s\n",
                   (((conf >> 4) & 1U) == 1U) ? "Command queue" : "Command");
    PAL_LOG_DEBUG("TOP top_cfg:        hash_arb_algo: %s\n",
                   (((conf >> 3) & 1U) == 1U) ? "Round-robin" : "Fixed");
    PAL_LOG_DEBUG("TOP top_cfg:        hash_arb_gran: %s\n",
                   (((conf >> 2) & 1U) == 1U) ? "Command queue" : "Command");

    conf = IO_READ32(TOP_STAT_VER);
    PAL_LOG_DEBUG("TOP dubhe_ver: 0x%x\n", conf);

    PAL_LOG_DEBUG("TOP top_cfg1: 0x%x\n", ce_hw_cfg.value);
    PAL_LOG_DEBUG("TOP top_cfg1:  sca_host_num: %d\n",
                   ce_hw_cfg.cfg.sca_host_num);
    PAL_LOG_DEBUG("TOP top_cfg1:  sca_host1_attr: %s\n",
                   ce_hw_cfg.cfg.sca_host1_attr ? "Secure" : "Non-secure");
    PAL_LOG_DEBUG("TOP top_cfg1:  hash_host_num: %d\n",
                   ce_hw_cfg.cfg.hash_host_num);
    PAL_LOG_DEBUG("TOP top_cfg1:  hash_host1_attr: %s\n",
                   ce_hw_cfg.cfg.hash_host1_attr ? "Secure" : "Non-secure");
    PAL_LOG_DEBUG("TOP top_cfg1:  aca_host_num: %d\n",
                   ce_hw_cfg.cfg.aca_host_num);
    PAL_LOG_DEBUG("TOP top_cfg1:  aca_host1_attr: %s\n",
                   ce_hw_cfg.cfg.aca_host1_attr ? "Secure" : "Non-secure");
    PAL_LOG_DEBUG("TOP top_cfg1:  aca_sram_size: %dK bytes\n",
                   (ce_hw_cfg.cfg.aca_sram_size == 0)
                   ? 4 : ((ce_hw_cfg.cfg.aca_sram_size == 1)
                   ? 8 : ((ce_hw_cfg.cfg.aca_sram_size == 2)
                   ? 16: -1)));
    PAL_LOG_DEBUG("TOP top_cfg1:  otp_exist: %s\n",
                   ce_hw_cfg.cfg.otp_exist ? "Exist" : "Not exist");
    PAL_LOG_DEBUG("TOP top_cfg1:  otp_init_value: %d\n",
                   ce_hw_cfg.cfg.otp_init_value);
    PAL_LOG_DEBUG("TOP top_cfg1:  trng_inter_src_exist: %s\n",
                   ce_hw_cfg.cfg.trng_inter_src_exist ? "Exist" : "Not exist");
    PAL_LOG_DEBUG("TOP top_cfg1:  rn_pool_host_num: %d\n",
                   ce_hw_cfg.cfg.rn_pool_host_num);
    PAL_LOG_DEBUG("TOP top_cfg1:  rn_pool_host1_attr: %s\n",
                   ce_hw_cfg.cfg.rn_pool_host1_attr ? "Secure" : "Non-secure");
    PAL_LOG_DEBUG("TOP top_cfg1:   otp_test_word_size: 0x%x\n",
                   ce_hw_cfg.cfg.otp_test_word_size);

    conf = IO_READ32(TOP_STAT_CFG2);
    PAL_LOG_DEBUG("TOP top_cfg2: 0x%x\n", conf);
    PAL_LOG_DEBUG("TOP top_cfg2:   otp_nsec_word_size: 0x%x\n",
                   ((conf >> 16) & 0x7FFU));
    PAL_LOG_DEBUG("TOP top_cfg2:   otp_sec_word_size: 0x%x\n",
                   ((conf >> 0) & 0x7FFU));

    conf = IO_READ32(TOP_STAT_INTR_HOST0);
    PAL_LOG_DEBUG("STATUS intr_host0_stat: 0x%x\n", conf);
    PAL_LOG_DEBUG("STATUS intr_host0_stat:     intr_trng: %d\n",
                   ((conf >> 4) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host0_stat:     intr_rng_pool0: %d\n",
                   ((conf >> 3) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host0_stat:     intr_sca0: %d\n",
                   ((conf >> 2) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host0_stat:     intr_hash0: %d\n",
                   ((conf >> 1) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host0_stat:     intr_aca0: %d\n",
                   ((conf >> 0) & 1U));

    conf = IO_READ32(TOP_STAT_INTR_HOST1);
    PAL_LOG_DEBUG("STATUS intr_host1_stat: 0x%x\n", conf);
    PAL_LOG_DEBUG("STATUS intr_host1_stat:     intr_rng_pool1: %d\n",
                   ((conf >> 3) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host1_stat:     intr_sca11: %d\n",
                   ((conf >> 2) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host1_stat:     intr_hash1: %d\n",
                   ((conf >> 1) & 1U));
    PAL_LOG_DEBUG("STATUS intr_host1_stat:     intr_aca1: %d\n",
                   ((conf >> 0) & 1U));

#endif /* CE_DBG_DUMP_INIT_REGS */

    return ret;
}

void ce_drv_exit(void)
{
#if defined(CE_LITE_BN)
    aca_engine_exit();
#endif

#if defined(CE_LITE_HASH)
    hash_engine_exit();
#endif

#if defined(CE_LITE_OTP)
    ce_otp_drv_exit();
#endif

#if defined(CE_LITE_CIPHER)
    sca_engine_exit();
#endif

#if defined(CE_LITE_TRNG)
    trng_engine_exit();
#endif

    return;
}
