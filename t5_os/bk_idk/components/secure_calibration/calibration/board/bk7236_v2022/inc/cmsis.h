/**
 * @file	test_doxygen.h
 * @brief	This file contains examples for doxygen.
 * @author 	xxx (xxx@armchina.com)
 *
 * @date    	2018/04/09
 *
 * @if copyright_display
 *		Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 *		All rights reserved
 *
 *	 	The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *		to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *		License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *		restricting among other things, the use, reproduction, distribution
 *		and transfer.  Each of the embodiments, including this information and
 *		any derivative work shall retain this copyright notice.
 * @endif
 *
 *
 * @if modification_History
 *
 *		<b>Modification History:\n</b>
 *		Date				Name			Comment \n\n
 *		2018/04/09               	xxx			initial verison
 *
 * @endif
 */

#ifndef __CMSIS_H__
#define __CMSIS_H__

#if 0
/* CP10 Access Bits */
#define CPACR_CP10_Pos          20U
#define CPACR_CP10_Msk          (3UL << CPACR_CP10_Pos)
#define CPACR_CP10_NO_ACCESS    (0UL << CPACR_CP10_Pos)
#define CPACR_CP10_PRIV_ACCESS  (1UL << CPACR_CP10_Pos)
#define CPACR_CP10_RESERVED     (2UL << CPACR_CP10_Pos)
#define CPACR_CP10_FULL_ACCESS  (3UL << CPACR_CP10_Pos)

/* CP11 Access Bits */
#define CPACR_CP11_Pos          22U
#define CPACR_CP11_Msk          (3UL << CPACR_CP11_Pos)
#define CPACR_CP11_NO_ACCESS    (0UL << CPACR_CP11_Pos)
#define CPACR_CP11_PRIV_ACCESS  (1UL << CPACR_CP11_Pos)
#define CPACR_CP11_RESERVED     (2UL << CPACR_CP11_Pos)
#define CPACR_CP11_FULL_ACCESS  (3UL << CPACR_CP11_Pos)

#define SCB_UFSR  (*((__IOM u16_t *) &SCB->CFSR + 1))
#define SCB_BFSR  (*((__IOM u8_t *) &SCB->CFSR + 1))
#define SCB_MMFSR (*((__IOM u8_t *) &SCB->CFSR))

/* CFSR[UFSR] */
#define CFSR_DIVBYZERO_Pos		(25U)
#define CFSR_DIVBYZERO_Msk		(0x1U << CFSR_DIVBYZERO_Pos)
#define CFSR_UNALIGNED_Pos		(24U)
#define CFSR_UNALIGNED_Msk		(0x1U << CFSR_UNALIGNED_Pos)
#define CFSR_NOCP_Pos			(19U)
#define CFSR_NOCP_Msk			(0x1U << CFSR_NOCP_Pos)
#define CFSR_INVPC_Pos			(18U)
#define CFSR_INVPC_Msk			(0x1U << CFSR_INVPC_Pos)
#define CFSR_INVSTATE_Pos		(17U)
#define CFSR_INVSTATE_Msk		(0x1U << CFSR_INVSTATE_Pos)
#define CFSR_UNDEFINSTR_Pos		(16U)
#define CFSR_UNDEFINSTR_Msk		(0x1U << CFSR_UNDEFINSTR_Pos)

/* CFSR[BFSR] */
#define CFSR_BFARVALID_Pos		(15U)
#define CFSR_BFARVALID_Msk		(0x1U << CFSR_BFARVALID_Pos)
#define CFSR_LSPERR_Pos			(13U)
#define CFSR_LSPERR_Msk			(0x1U << CFSR_LSPERR_Pos)
#define CFSR_STKERR_Pos			(12U)
#define CFSR_STKERR_Msk			(0x1U << CFSR_STKERR_Pos)
#define CFSR_UNSTKERR_Pos		(11U)
#define CFSR_UNSTKERR_Msk		(0x1U << CFSR_UNSTKERR_Pos)
#define CFSR_IMPRECISERR_Pos		(10U)
#define CFSR_IMPRECISERR_Msk		(0x1U << CFSR_IMPRECISERR_Pos)
#define CFSR_PRECISERR_Pos		(9U)
#define CFSR_PRECISERR_Msk		(0x1U << CFSR_PRECISERR_Pos)
#define CFSR_IBUSERR_Pos		(8U)
#define CFSR_IBUSERR_Msk		(0x1U << CFSR_IBUSERR_Pos)

/* CFSR[MMFSR] */
#define CFSR_MMARVALID_Pos		(7U)
#define CFSR_MMARVALID_Msk		(0x1U << CFSR_MMARVALID_Pos)
#define CFSR_MLSPERR_Pos		(5U)
#define CFSR_MLSPERR_Msk		(0x1U << CFSR_MLSPERR_Pos)
#define CFSR_MSTKERR_Pos		(4U)
#define CFSR_MSTKERR_Msk		(0x1U << CFSR_MSTKERR_Pos)
#define CFSR_MUNSTKERR_Pos		(3U)
#define CFSR_MUNSTKERR_Msk		(0x1U << CFSR_MUNSTKERR_Pos)
#define CFSR_DACCVIOL_Pos		(1U)
#define CFSR_DACCVIOL_Msk		(0x1U << CFSR_DACCVIOL_Pos)
#define CFSR_IACCVIOL_Pos		(0U)
#define CFSR_IACCVIOL_Msk		(0x1U << CFSR_IACCVIOL_Pos)

#endif

#ifndef __ASSEMBLY__
/* --------  Configuration of the Cortex-M33 Processor and Core Peripherals  ------ */
#define __CM33_REV                0x0000U   /* Core revision r0p1 */
#define __MPU_PRESENT             1U        /* MPU present */
#define __SAUREGION_PRESENT       1U        /* SAU regions present */
#define __VTOR_PRESENT            1U        /* VTOR present */
#define __NVIC_PRIO_BITS          3U        /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT             1U        /* no FPU present */
#define __DSP_PRESENT             1U        /* no DSP extension present */

#include "cmsis_platform_irq.h"
#include "cmsis/Include/core_star.h"

#endif /* __ASSEMBLY__ */
#endif /* __CMSIS_H__ */
