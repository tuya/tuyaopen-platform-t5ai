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

#include "asm_macros.h"

   .text
    .syntax unified
    .thumb

    .section .vector_table,"a",%progbits
    .align 2
    .globl __isr_vector
__isr_vector:
    .long    __MSPTop              /* Top of Stack */
    .long    Reset_Handler         /* Reset Handler */
    .long    NMI_Handler           /* NMI Handler */
    .long    HardFault_Handler     /* Hard Fault Handler */
    .long    MemManage_Handler     /* MPU Fault Handler */
    .long    BusFault_Handler      /* Bus Fault Handler */
    .long    UsageFault_Handler    /* Usage Fault Handler */
    .long    SecureFault_Handler  /* Secure Fault Handler */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    SVC_Handler           /* SVCall Handler */
    .long    DebugMon_Handler      /* Debug Monitor Handler */
    .long    0                     /* Reserved */
    .long    PendSV_Handler        /* PendSV Handler */
    .long    SysTick_Handler       /* SysTick Handler */

    .long    0                     /* Interrupt 0 */
    .long    0                     /* Interrupt 1 */
    .long    0                     /* Interrupt 2 */
    .long    0                     /* Interrupt 3 */
    .long    UART_InterruptHandler /* Interrupt 4 */
    .rept    43
    .long    0
    .endr
    .long    0x32374B42
    .long    0x00003633

    .text
    .section .text.Reset_Handler
    .thumb
    .thumb_func
    .align  2
    .globl   Reset_Handler
    .globl   deep_sleep_reset_if_required
    .type    Reset_Handler, %function
    .type    deep_sleep_reset_if_required, %function

Reset_Handler:
/*
 * Loop to copy data from read only memory to RAM. The ranges
 * of copy from/to are specified by following symbols evaluated in
 * linker script.
 * _etext: End of code section, i.e., begin of data sections to copy from.
 * __data_start__/__data_end__: RAM address range that data should be
 * copied to. Both must be aligned to 4 bytes boundary.
 */

#if (CONFIG_EFUSE)
    push   {R0-R7, LR}
    ldr    r3, = deep_sleep_reset_if_required
    blx    r3
    pop   {R0-R7, LR}
#endif

.normal_reset:
    ldr    r1, =__etext
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

    subs   r3, r2
    ble    .Lflash_to_ram_loop_end

    movs    r4, 0
.Lflash_to_ram_loop:
    ldr    r0, [r1,r4]
    str    r0, [r2,r4]
    adds   r4, 4
    cmp    r4, r3
    blt    .Lflash_to_ram_loop
.Lflash_to_ram_loop_end:

/* Initialize .bss */
init_bss:
    ldr   r1, =__bss_start__
    ldr   r2, =__bss_end__
    ldr   r3, =bss_size

    cmp   r3, #0
    beq   system_startup

    mov   r4, #0
zero:
    strb  r4, [r1], #1
    subs  r3, r3, #1
    bne   zero

system_startup:
    ldr    r0, =bs_main
    bx     r0

/* Register definition */

/* Reset as soon as possible if we found it's deep sleep reset:
 *
 * 1> Check whether PMU fast boot bit is set:
 *    PMU reg (0x4400 0000 + 3*4) fast_boot bit (bit0)
 *    means it's deep sleep reset, 0 normal reset.
 *
 * 2> Enable Efuse read operate from byte offeset 0
 *    Set Efuse reg (0x4488 0000 + 4*4) efuse_opr_en bit (bit0)
 *    to 1, and efuse_oprt_addr bit[8:12] to 0 enable read op
 *    from efuse offset 0
 *
 * 3> Wait Efuse read data valid bit becomes to 1
 *    Check Efuse reg (0x4488 0000 + 5*4) efuse_read_data_valid (bit8)
 *    becomes to 1
 *
 * 4> Check fast_boot support flag
 *    Read Efuse reg (0x4488 0000 + 5*4) efuse_read_data bit[0:7],
 *    if bit2 (fast boot bit) is 1, means fast boot supported.
 * 
 * 5> Do deep sleep reset
 *    Set system reg (0x4401 0000 + 2*4), flash_sel bit(bit9) to 0,
 *    to disable SPI download mode
 *    Set system reg (0x4401 0000 + 2*4), boot_mode bit(bit0) to 0,
 *    to enable ROM boot.
 */

.align   4
.equ     AON_PMU_REG0x0, 0x44000000
.equ     EFUSE_REG0x2,   0x44880008
.equ     EFUSE_REG0x4,   0x44880010
.equ     SYSTEM_REG0x2,  0x44010008
.equ     PRRO_REG19_CMP0_DATA_SRC,  0x41040064
.equ     PRRO_REG1A_CMP0_DATA_DST,  0x41040068
.equ     PRRO_REG1D_CMP1_DATA_SRC,  0x41040074
.equ     PRRO_REG1E_CMP1_DATA_DST,  0x41040078
.equ     PRRO_REG21_CMP2_DATA_SRC,  0x41040084
.equ     PRRO_REG22_CMP2_DATA_DST,  0x41040088

deep_sleep_reset_if_required:

.check_deep_sleep:
    ldr         r3, =AON_PMU_REG0x0

    ldr	        r3, [r3, #0]
    lsls	r1, r3, #30
    bpl.n	.local_return

.start_efuse_byte0_read:
    mov 	r2, #3
    ldr         r3, =EFUSE_REG0x2
    str	        r2, [r3, #0]

    mov 	r2, #1

    ldr         r3, =EFUSE_REG0x4

    str	        r2, [r3, #0]
    adds	r3, #4

.wait_efuse_valid_bit:
    ldr	        r2, [r3, #0]
    lsls	r2, r2, #23
    bpl.n	.wait_efuse_valid_bit
    ldr	        r3, [r3, #0]
    lsls	r3, r3, #29
    bmi.n	.local_return

.disable_hw_fih:
    mov 	r2, #0
    ldr         r3, =PRRO_REG19_CMP0_DATA_SRC
    str	        r2, [r3, #0]
    ldr         r3, =PRRO_REG1A_CMP0_DATA_DST
    str	        r2, [r3, #0]
    ldr         r3, =PRRO_REG1D_CMP1_DATA_SRC
    str	        r2, [r3, #0]
    ldr         r3, =PRRO_REG1E_CMP1_DATA_DST
    str	        r2, [r3, #0]
    ldr         r3, =PRRO_REG21_CMP2_DATA_SRC
    str	        r2, [r3, #0]
    ldr         r3, =PRRO_REG22_CMP2_DATA_DST
    str	        r2, [r3, #0]

.deep_sleep_reset:
.disable_flash_spi_download:
    ldr         r3, =SYSTEM_REG0x2

    ldr     r2, [r3, #0]
    bic.w   r2, r2, #512 /* 0x200 */
    str     r2, [r3, #0]

.set_boot_mode_to_rom_boot:
    ldr     r2, [r3, #0]
    bic.w   r2, r2, #1
    str     r2, [r3, #0]
.dead_loop:
    b       .dead_loop

.local_return:
    bx          lr

    .pool
    .size Reset_Handler, . - Reset_Handler

    .text
    def_default_handler    NMI_Handler
    def_default_handler    HardFault_Handler
    def_default_handler    MemManage_Handler
    def_default_handler    BusFault_Handler
    def_default_handler    UsageFault_Handler
    def_default_handler    SecureFault_Handler
    def_default_handler    SVC_Handler
    def_default_handler    DebugMon_Handler
    def_default_handler    PendSV_Handler
    def_default_handler    SysTick_Handler
    def_default_handler    Default_Handler

    .end

