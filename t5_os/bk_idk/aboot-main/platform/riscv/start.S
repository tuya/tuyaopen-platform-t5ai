/*
 * Copyright (c) 2012-2021 Andes Technology Corporation
 * All rights reserved.
 *
 */
#include "core_v5.h"

	.section .nds_vector, "ax"

.option push
.option norelax
	.global reset_vector

reset_vector:
	/* Decide whether this is an NMI or cold reset */
	csrr t0, mcause
	bnez t0, nmi_handler
	//csrc mcause

	.global _start
	.type _start,@function

_start:
#ifndef CACHE_DISABLE
   /* enable icache   */
    csrr	a5,mcache_ctl
     ori	a5,a5,1
    csrw	mcache_ctl,a5
   /* enable dcache */
    csrr	a5,mcache_ctl
     ori	a5,a5,2
    csrw	mcache_ctl,a5
#endif
	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	/* Initialize stack pointer */
	la t0, _stack
	mv sp, t0

#ifdef __nds_execit
	/* Initialize EXEC.IT table */
	la t0, _ITB_BASE_
	csrw uitb, t0
#endif

#ifdef __riscv_flen
	/* Enable FPU */
	li t0, MSTATUS_FS
	csrrs t0, mstatus, t0

	/* Initialize FCSR */
	fscsr zero
#endif

	/* Initial machine trap-vector Base */
	la t0, trap_entry
	csrw mtvec, t0

	/* Do system low level setup. It must be a leaf function */
	call __platform_init

	/* System reset handler */
	call reset_handler

	/* Infinite loop, if returned accidently */
1:	j 1b

	.weak __platform_init
__platform_init:
	ret

	.weak nmi_handler
nmi_handler:
	j 0x00010000
1:	j 1b
.org  0x100
.word 0x32374B42
.word 0x00003635
.word 0x00000000
.word 0x00000000
.org  0x110
.word 0x64616568    /*head: sign head address*/
.word 0x00000000
.word 0x00000000
.word 0x00000000
.org  0x120
.word 0x00000000    /*version: provide to version number setting*/
.word 0x00000000
.word 0x00000000
.word 0x00000000
.option pop
