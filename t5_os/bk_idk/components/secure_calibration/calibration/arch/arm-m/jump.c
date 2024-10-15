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
#include "platform.h"
#define DBG_PRE_PROMPT ""
#include "dbg.h"
#include "pal_log.h"
#include "hal_misc.h"

void jump_to_next(uint32_t addr)
{
    uint32_t msp = 0;
    uint32_t pc  = 0;

    PAL_LOG_DEBUG("jump to %x\r\n", addr);
    PAL_LOG_DEBUG("Next Image Addr: 0x%x\n", addr);
    msp = *(uint32_t *)addr;
    pc  = *(uint32_t *)(addr + 4);

    PAL_LOG_DEBUG("Set MSP: 0x%x\n", (unsigned int)msp);
    PAL_LOG_DEBUG("Jump to: 0x%x\n", (unsigned int)pc);

    __set_MSP(msp);
    __DSB();
    __ISB();

    /* relocate vecotr table */
    SCB->VTOR = (addr);
    __DSB();
    __ISB();

    /* save next entry to r9 */
    __asm__ volatile("mov r9, %0" : : "r"(pc) : "memory");

    /* clear all general registers */
    __asm__ volatile(
        "mov r0, #0;\n\t"
        "mov r1, r0;\n\t"
        "mov r2, r0;\n\t"
        "mov r3, r0;\n\t"
        "mov r4, r0;\n\t"
        "mov r5, r0;\n\t"
        "mov r6, r0;\n\t"
        "mov r7, r0;\n\t"
        "mov r8, r0;\n\t"
        "mov r10, r0;\n\t"
        "mov r11, r0;\n\t"
        "mov r12, r0;\n\t"
        "dsb;\n\t"
        "isb;\n\t");

    addSYSTEM_Reg0x10 |= BIT(15); //Power off BootROM 

    __asm__ volatile(
        "bx r9;\n\t"
        "b .;\n\t"      /* should not be here */
        );
}
