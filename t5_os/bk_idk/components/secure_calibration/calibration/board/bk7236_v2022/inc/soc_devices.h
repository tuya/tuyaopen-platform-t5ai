/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is copied from
    zephyr/arch/arm/soc/arm/mps2/soc_devices.h
*/
#ifndef __SOC_DEVICES_H__
#define __SOC_DEVICES_H__

#include "platform_retarget.h"
#include "soc_registers.h"

/* FPGA system control block (FPGAIO) */
#define FPGAIO_BASE_ADDR	 (MPS2_IO_FPGAIO_BASE_NS)
#define __MPS2_FPGAIO ((volatile struct mps2_fpgaio *)FPGAIO_BASE_ADDR)

/* Names of GPIO drivers used to provide access to some FPGAIO registers */
#define FPGAIO_LED0_GPIO_NAME		"FPGAIO_LED0"
#define FPGAIO_BUTTON_GPIO_NAME		"FPGAIO_BUTTON"
#define FPGAIO_MISC_GPIO_NAME		"FPGAIO_MISC"

static inline uint32_t fpga_scb_io_clk1hz_read(void)
{
    return (__MPS2_FPGAIO->clk1hz);
}


static inline uint32_t fpga_scb_io_counter_read(void)
{
    return (__MPS2_FPGAIO->counter);
}

#define _FPGA_SCB_IO_COUNTERS_CYCLES_PER_SEC    (20000000)
#define _FPGA_SCB_IO_1US_COUNTER        (_FPGA_SCB_IO_COUNTERS_CYCLES_PER_SEC/1000000)
/* Sleep function to delay n*uS
 */
static inline void arch_sleep_us(uint32_t usec)
{
#if 1	////Pochin workaround
	uint32_t dlyCnt = 100;
	dlyCnt *= usec;
	while(dlyCnt--);
#else	
	register unsigned int end;
	register unsigned int start;

    start = fpga_scb_io_counter_read();
    end   = start + (_FPGA_SCB_IO_1US_COUNTER * usec);

    if (end >= start) {
        while (fpga_scb_io_counter_read() >= start && fpga_scb_io_counter_read() < end);
    } else {
        while (fpga_scb_io_counter_read() >= start);
        while (fpga_scb_io_counter_read() < end);
    }
#endif	
}

static inline uint32_t platform_get_rand_seed(void)
{
    volatile uint32_t i = 0;
    uint32_t tmp = fpga_scb_io_counter_read();
    for (i = 0; i < (tmp & 0xfff); i++);
    tmp = tmp * fpga_scb_io_counter_read();
    return tmp;
}

static inline uint32_t platform_get_timestamp_second(void)
{
    uint32_t cnt = fpga_scb_io_clk1hz_read();

    return cnt;
}

static inline uint32_t platform_get_timestamp_counter(void)
{
    uint32_t cnt = fpga_scb_io_counter_read();

    return cnt;
}

#endif /* _SOC_DEVICES_H_ */
