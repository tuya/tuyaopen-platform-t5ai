/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "base.h"
#include "hal_platform.h"

#define DBG_PRE_PROMPT "BS"
#include "dbg.h"
#include "pal_log.h"
#include "bootloader.h"
#include "hal_wdt.h"
#include "hal_boot_control.h"
#include "hal_misc.h"
#include "hal_hw_fih.h"
#include "hal.h"
#include "soc.h"
#include "platform.h"
#include "sys_hal.h"
#include "trng.h"
#include "otp.h"
#include "boot_protocol.h"
#include "../utils/ce_lite/port_mbedtls/mbedtls/library/rsa_demo_test.h"

extern int main(void);

#define OPEN_DOWNLOAD_FUNCTION    (0)

void bk_original_boot_init(void)
{
    hal_perf_debug(P_MAIN_START);
    	//Init efuse firstly since deep sleep to flash depends on efuse.
	hal_efuse_init();
}


void bk_secure_otp_efuse_init()
{

}

extern int32_t hal_platform_init(void);

void bs_main(void)
{
	int32_t ret = 0;

	bk_original_boot_init(); //can ignore it .

#ifdef CONFIG_BK_BOOT

	/* Feed the watchdog at very beginning in case memory copy cost too much time */
	hal_wdt_close();//BK3000_start_wdt(0xA000); // 24s

	sys_hal_init();

	// Important notice:
	// In bootrom, the only supported LEVEL0 device is uart, uart initialization will not call any heap malloc API, it's safe to put LEVEL0 initializtion
	// before hal_platform_early init which will init the heap.

	ret = sys_device_do_config_level(DEVICE_LEVEL0); // uart1_init
	CHECK_RET("device level0 init fail 0x%08x\n", ret);
#endif

#if CONFIG_XTAL_40M
	PAL_LOG_INFO("xtal:%d,%d\r\n", AON_PMU_IS_XTAL_40M, AON_PMU_IS_XTAL_SEL_40M);
#else
	PAL_LOG_INFO("xtal: 26m\r\n");
#endif

	ret = hal_platform_early_init();
	CHECK_RET("platform_early_init failed!\n");

	ret = sys_device_do_config_level(DEVICE_LEVEL1);  //flash_init
	CHECK_RET("device level2 init fail 0x%08x\n", ret);

	PAL_LOG_INFO("version:%s\n", VERSION_STRING);

	ret = hal_platform_init();
	CHECK_RET("platform_init failed!\n");

	//bk_secure_otp_efuse_init(); //can ignore it .
	bk_otp_init();
	bk_trng_driver_init();

#if OPEN_DOWNLOAD_FUNCTION
	PAL_LOG_INFO("enter test \n");
	extern int mbedtls_aes_self_test(int verbose);
	mbedtls_aes_self_test(1);

	extern int mbedtls_aes_port_test(int verbose,int mode);
	mbedtls_aes_port_test(1,1);		//do aes encrypt.
	mbedtls_aes_port_test(1,0);		//do aes decrypt.

	bl_test_otp_read_write_function();

	PAL_LOG_INFO("enter rsa test \n");
	extern int mbedtls_rsa_self_test(int verbose);
	mbedtls_rsa_self_test(1);

	PAL_LOG_INFO("read rsa test \n");
	bl_read_encrypted_aes_from_flash();
	bl_read_rsa_pubkey_from_flash();

	PAL_LOG_INFO("enter rsa self related test \n");
	//demo1_run_rsa_encryption_decryption_separate();
	demo2_mbedtls_rsa_test_with_public_private_key();
	demo2_with_private_key_encrypt_public_key_decrypt();
	demo3_auto_generate_keypair_do_rsa_test();
#endif

	legacy_boot_main_adapt();  //interacte with the pc(BKFIL)

finish:
	/* should not be here */
	while (1);
}

__WEAK int main(void)
{
	while (1);
}
