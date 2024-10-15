/*
 * Copyright (c) 2012-2021 Andes Technology Corporation
 * All rights reserved.
 *
 */

#include <stdio.h>
#include "platform.h"

extern void mext_interrupt(unsigned int irq_source);

__attribute__((weak)) void mtime_handler(void)
{
	clear_csr(NDS_MIE, MIP_MTIP);
}

__attribute__((weak)) void mswi_handler(void)
{
	clear_csr(NDS_MIE, MIP_MSIP);
}

__attribute__((weak)) void syscall_handler(long n, long a0, long a1, long a2, long a3)
{
	//printf("syscall #%ld (a0:0x%lx,a1:0x%lx, a2:0x%lx, a3:0x%lx)\n", n, a0, a1, a2, a3);
}

__attribute__((weak)) long except_handler(long cause, long epc)
{
	/* Unhandled Trap */
	//printf("Unhandled Trap : mcause = 0x%x, mepc = 0x%x\n", (unsigned int)cause, (unsigned int)epc);

	return epc;
}

void trap_entry(void) __attribute__ ((interrupt ("machine") , aligned(4)));
void trap_entry(void)
{
	long mcause = read_csr(NDS_MCAUSE);
	long mepc = read_csr(NDS_MEPC);
	long mstatus = read_csr(NDS_MSTATUS);
#if SUPPORT_PFT_ARCH
	long mxstatus = read_csr(NDS_MXSTATUS);
#endif
#ifdef __riscv_dsp
	int ucode = read_csr(NDS_UCODE);
#endif
#ifdef __riscv_flen
	int fcsr = read_fcsr();
#endif

	/* clobbers list for ecall */
#ifdef __riscv_32e
	__asm volatile("" : : :"t0", "a0", "a1", "a2", "a3");
#else
	__asm volatile("" : : :"a7", "a0", "a1", "a2", "a3");
#endif

	/* Do your trap handling */
if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_EXT)) {
		/* Machine-level interrupt from PLIC */
		//while(1);
		mext_interrupt(__nds__plic_claim_interrupt());
	} else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_TIMER)) {
		/* Machine timer interrupt */
		mtime_handler();
        } else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_SOFT)) {
		/* Machine SWI interrupt */
		mswi_handler();
		/* Machine SWI is connected to PLIC_SW source 1 */
		__nds__plic_sw_complete_interrupt((__nds__mfsr(NDS_MHARTID) + 1));
	} else if (!(mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == TRAP_M_ECALL)) {
		/* Machine Syscal call */
		__asm volatile(
				"mv a4, a3\n"
				"mv a3, a2\n"
				"mv a2, a1\n"
				"mv a1, a0\n"
#ifdef __riscv_32e
				"mv a0, t0\n"
#else
				"mv a0, a7\n"
#endif
				"call syscall_handler\n"
				: : : "a4"
		);
		mepc += 4;
	} else {
		/* Unhandled Trap */
		mepc = except_handler(mcause, mepc);
	}

	/* Restore CSR */
	write_csr(NDS_MSTATUS, mstatus);
	write_csr(NDS_MEPC, mepc);
#if SUPPORT_PFT_ARCH
	write_csr(NDS_MXSTATUS, mxstatus);
#endif
#ifdef __riscv_dsp
	write_csr(NDS_UCODE, ucode);
#endif
#ifdef __riscv_flen
	write_fcsr(fcsr);
#endif
}
