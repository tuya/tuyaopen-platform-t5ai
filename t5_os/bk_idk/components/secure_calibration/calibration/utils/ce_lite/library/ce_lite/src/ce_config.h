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

#ifndef __CE_CONFIG_H__
#define __CE_CONFIG_H__

/*
 * CE base address
 */
#ifndef CE_CFG_BASE_ADDR
#define CE_CFG_BASE_ADDR    0x4b110000
#endif

/**
 * CE host number
 */
#ifndef CE_CFG_HOST_ID
#define CE_CFG_HOST_ID      0
#endif

/**
 * Indicates CE Lite driver's running environment is cache enabled.
 */
#ifndef CE_CFG_WITH_DCACHE
#define CE_CFG_WITH_DCACHE  0
#endif

/**
 * The following are debug configurations, only for debug purpose, disable them
 * by default
 */

/* Dump the TOP config/status and other registers at te_init */
#undef CE_DBG_DUMP_INIT_REGS

/* Dump detailed information in ECP related operation */
#undef ECP_DBG_PRINT_DETAIL_EN

/* Dump simple information in ECP related operation */
#undef ECP_DBG_PRINT_SIMPLE_EN

/**
 * Whether use hex string format or binary data format when dumping OP context.
 */
#define OP_DUMP_HEX_STR_FORMAT

/* Dump every read/write of ACA registers */
#undef IO_LOG_ENABLE

extern void ce_config_hw(void);

#endif /*__CE_CONFIG_H__*/
