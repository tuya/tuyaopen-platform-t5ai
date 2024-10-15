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
#include "platform.h"
#include "uart_hal.h"
#include "uart.h"
#include "hal_boot_control.h"

#include "mbedtls/platform.h"
#include "hal_efuse.h"
#include "efuse.h"

#include "../../utils/ce_lite/port_mbedtls/mbedtls/include/mbedtls/platform.h"


#if (ARM_CE_DUBHE)
#include "ce_lite_common.h"
#endif

extern int32_t platform_init(void);
extern int32_t device_init(void);
extern void global_heap_init(void);
extern int mbedtls_platform_set_calloc_free(void *(*calloc_func)(size_t, size_t), void (*free_func)(void *));
extern int mbedtls_platform_set_printf(int (*printf_func)(const char *, ...));
int32_t hal_platform_early_init(void)
{
    global_heap_init();
    return 0;
}

void mbedtls_init(void)
{
#if CONFIG_TE200_UT
    mbedtls_platform_set_printf(printf);
#endif
   mbedtls_platform_set_calloc_free(kcalloc, kfree);
    
#if (ARM_CE_DUBHE)
    ce_drv_init();
#endif

#if CONFIG_HW_FIH
    extern bool s_is_crypto_inited;
    s_is_crypto_inited = true;
    PAL_LOG_DEBUG("crypto inited\r\n");
#endif
    return;
}

int32_t hal_platform_init(void)
{
#ifdef CONFIG_BK_BOOT
    mbedtls_init();
#else
    mbedtls_init();
#endif
    return 0;
}

int32_t hal_platform_post_init(void)
{
    return 0;
}

void hal_udelay(uint32_t usecs)
{
    arch_sleep_us(usecs);
}

void hal_mdelay(uint32_t msecs)
{
    arch_sleep_us(msecs * 1000);
}

uint32_t hal_get_timestamp_second()
{
    return platform_get_timestamp_second();
}

uint32_t hal_get_timestamp_counter()
{
    return platform_get_timestamp_counter();
}

uint32_t hal_get_rand()
{
    return platform_get_rand_seed();
}

void hal_reset(void)
{
    SEC_WATCHDOG->CTRL = CMSDK_Watchdog_CTRL_RESEN_Msk;  // enable reset
    SEC_WATCHDOG->LOAD = 0x1ul;  // the minimum value of load
    SEC_WATCHDOG->ITOP = CMSDK_Watchdog_INTEGTESTEN_Msk;
    SEC_WATCHDOG->ITCR = CMSDK_Watchdog_INTEGTESTEN_Msk;
	
    while (1);  // wait for watchdog reset
}

extern struct device *uart_0_dev;
void hal_uart_putc(uint8_t c)
{
    if ('\n' == c) {
        uart_putc(uart_0_dev, '\r');
    }
    return uart_putc(uart_0_dev, c);
}

void hal_uart_putc_strict(uint8_t c)
{
    return uart_putc(uart_0_dev, c);
}

size_t hal_uart_puts(const char *s)
{
    int i      = 0;
    size_t len = strlen(s);

    for (i = 0; i < len; i++) {
        hal_uart_putc(s[i]);
    }
    return len;
}

int32_t hal_uart_getc(uint8_t *c)
{
    return uart_getc(uart_0_dev, c);
}

/*
 * no data input return 0
 * other return !0
 *
 */
int32_t hal_uart_rx_ready()
{
    return uart_rx_ready(uart_0_dev);
}


void hal_led_on_off(int32_t led_id, bool is_on)
{
    return;
}

void hal_mcc_led_op(int32_t led_id, bool is_on)
{
    return;
}

void hal_jtag_enable(void)
{
    *((volatile uint32_t*)CORTEX_M33_REG_DAUTHCTRL) =
        CORTEX_M33_CPU_DEBUG_ENABLE_VALUE;/*DAUTHCTRL register, jtag debug  enable*/
}

void hal_jtag_disable(void)
{
    *((volatile uint32_t*)CORTEX_M33_REG_DAUTHCTRL) =
        CORTEX_M33_CPU_DEBUG_DISABLE_VALUE;/*DAUTHCTRL register, jtag  disable*/
}
