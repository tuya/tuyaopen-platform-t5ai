/*
 * Copyright (c) 2016-2018 ARM Limited
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file platform_retarget.h
 * \brief This file defines all the peripheral base addresses for MPS2/SSE-200 platform.
 */

#ifndef __ARM_LTD_AN495_RETARGET_H__
#define __ARM_LTD_AN495_RETARGET_H__

/* ======= Defines peripherals memory map addresses ======= */
/* Non-secure memory map addresses */
#define CMSDK_TIMER0_BASE_NS            (0x40000000) 
#define CMSDK_TIMER1_BASE_NS            (0x40001000)
#define APB_DUALTIMER_BASE_NS           (0x40002000)
#define UART0_BASE_NS                   (0x40004000)
#define UART1_BASE_NS                   (0x40005000)
#define UART2_BASE_NS                   (0x40006000)
#define APB_WATCHDOG_BASE_NS            (0x40008000)

#define GPIO0_BASE_NS                   (0x40010000)
#define GPIO1_BASE_NS                   (0x40011000)
#define GPIO2_BASE_NS                   (0x40012000)
#define GPIO3_BASE_NS                   (0x40013000)

#define MPS2_SSP1_BASE_NS               (0x40020000)       /* User SSP Base Address   */
#define MPS2_IO_FPGAIO_BASE_NS          (0x40028000)
#define MPS2_IO_SCC_BASE_NS             (0x4002f000)

/* Secure memory map addresses          (*/
#define CMSDK_TIMER0_BASE_S             (0x50000000)
#define CMSDK_TIMER1_BASE_S             (0x50001000)
#define APB_DUALTIMER_BASE_S            (0x50002000)
#define UART0_BASE_S                    (0x50004000)
#define UART1_BASE_S                    (0x50005000)
#define UART2_BASE_S                    (0x50004000)
#define APB_WATCHDOG_BASE_S             (0x50008000)

#define GPIO0_BASE_S                    (0x50010000)
#define GPIO1_BASE_S                    (0x50011000)
#define GPIO2_BASE_S                    (0x50012000)
#define GPIO3_BASE_S                    (0x50013000)

#define MPS2_SSP1_BASE_S                (0x50020000)       /* User SSP Base Address   */
#define MPS2_IO_FPGAIO_BASE_S           (0x50028000)
#define MPS2_IO_SCC_BASE_S              (0x5002f000)

/* Secure Privilege Control */
#define CMSDK_SPCTRL_BASE_S             (0x58006000)
#define MPC_CODE_SRAM1_BASE_S           (0x58007000)
#define MPC_CODE_SRAM2_BASE_S           (0x58008000)
#define MPC_CODE_SRAM3_BASE_S           (0x58009000)

/* External SSRAM memory */
#define MPC_CODE_SRAM1_RANGE_BASE_NS    (0x00000000)
#define MPC_CODE_SRAM1_RANGE_LIMIT_NS   (0x003FFFFF)
#define MPC_CODE_SRAM1_RANGE_BASE_S     (0x10000000)
#define MPC_CODE_SRAM1_RANGE_LIMIT_S    (0x103FFFFF)

#define MPC_CODE_SRAM2_RANGE_BASE_NS    (0x20000000)
#define MPC_CODE_SRAM2_RANGE_LIMIT_NS   (0x201FFFFF)
#define MPC_CODE_SRAM2_RANGE_BASE_S     (0x30000000)
#define MPC_CODE_SRAM2_RANGE_LIMIT_S    (0x301FFFFF)

/*------------------- Watchdog ----------------------------------------------*/
typedef struct
{

  __IO    uint32_t  LOAD;                   /* Offset: 0x000 (R/W) Watchdog Load Register */
  __I     uint32_t  VALUE;                  /* Offset: 0x004 (R/ ) Watchdog Value Register */
  __IO    uint32_t  CTRL;                   /* Offset: 0x008 (R/W) Watchdog Control Register */
  __O     uint32_t  INTCLR;                 /* Offset: 0x00C ( /W) Watchdog Clear Interrupt Register */
  __I     uint32_t  RAWINTSTAT;             /* Offset: 0x010 (R/ ) Watchdog Raw Interrupt Status Register */
  __I     uint32_t  MASKINTSTAT;            /* Offset: 0x014 (R/ ) Watchdog Interrupt Status Register */
        uint32_t  RESERVED0[762];
  __IO    uint32_t  LOCK;                   /* Offset: 0xC00 (R/W) Watchdog Lock Register */
        uint32_t  RESERVED1[191];
  __IO    uint32_t  ITCR;                   /* Offset: 0xF00 (R/W) Watchdog Integration Test Control Register */
  __O     uint32_t  ITOP;                   /* Offset: 0xF04 ( /W) Watchdog Integration Test Output Set Register */
}CMSDK_WATCHDOG_TypeDef;
/* CMSDK_WATCHDOG LOAD Register Definitions */
#define CMSDK_Watchdog_LOAD_Pos               0                                              /* CMSDK_Watchdog LOAD: LOAD Position */
#define CMSDK_Watchdog_LOAD_Msk              (0xFFFFFFFFul << CMSDK_Watchdog_LOAD_Pos)       /* CMSDK_Watchdog LOAD: LOAD Mask */

/* CMSDK_WATCHDOG VALUE Register Definitions */
#define CMSDK_Watchdog_VALUE_Pos              0                                              /* CMSDK_Watchdog VALUE: VALUE Position */
#define CMSDK_Watchdog_VALUE_Msk             (0xFFFFFFFFul << CMSDK_Watchdog_VALUE_Pos)      /* CMSDK_Watchdog VALUE: VALUE Mask */

/* CMSDK_WATCHDOG CTRL Register Definitions */
#define CMSDK_Watchdog_CTRL_RESEN_Pos         1                                              /* CMSDK_Watchdog CTRL_RESEN: Enable Reset Output Position */
#define CMSDK_Watchdog_CTRL_RESEN_Msk        (0x1ul << CMSDK_Watchdog_CTRL_RESEN_Pos)        /* CMSDK_Watchdog CTRL_RESEN: Enable Reset Output Mask */

#define CMSDK_Watchdog_CTRL_INTEN_Pos         0                                              /* CMSDK_Watchdog CTRL_INTEN: Int Enable Position */
#define CMSDK_Watchdog_CTRL_INTEN_Msk        (0x1ul << CMSDK_Watchdog_CTRL_INTEN_Pos)        /* CMSDK_Watchdog CTRL_INTEN: Int Enable Mask */

/* CMSDK_WATCHDOG INTCLR Register Definitions */
#define CMSDK_Watchdog_INTCLR_Pos             0                                              /* CMSDK_Watchdog INTCLR: Int Clear Position */
#define CMSDK_Watchdog_INTCLR_Msk            (0x1ul << CMSDK_Watchdog_INTCLR_Pos)            /* CMSDK_Watchdog INTCLR: Int Clear Mask */

/* CMSDK_WATCHDOG RAWINTSTAT Register Definitions */
#define CMSDK_Watchdog_RAWINTSTAT_Pos         0                                              /* CMSDK_Watchdog RAWINTSTAT: Raw Int Status Position */
#define CMSDK_Watchdog_RAWINTSTAT_Msk        (0x1ul << CMSDK_Watchdog_RAWINTSTAT_Pos)        /* CMSDK_Watchdog RAWINTSTAT: Raw Int Status Mask */

/* CMSDK_WATCHDOG MASKINTSTAT Register Definitions */
#define CMSDK_Watchdog_MASKINTSTAT_Pos        0                                              /* CMSDK_Watchdog MASKINTSTAT: Mask Int Status Position */
#define CMSDK_Watchdog_MASKINTSTAT_Msk       (0x1ul << CMSDK_Watchdog_MASKINTSTAT_Pos)       /* CMSDK_Watchdog MASKINTSTAT: Mask Int Status Mask */

/* CMSDK_WATCHDOG LOCK Register Definitions */
#define CMSDK_Watchdog_LOCK_Pos               0                                              /* CMSDK_Watchdog LOCK: LOCK Position */
#define CMSDK_Watchdog_LOCK_Msk              (0x1ul << CMSDK_Watchdog_LOCK_Pos)              /* CMSDK_Watchdog LOCK: LOCK Mask */

/* CMSDK_WATCHDOG INTEGTESTEN Register Definitions */
#define CMSDK_Watchdog_INTEGTESTEN_Pos        0                                              /* CMSDK_Watchdog INTEGTESTEN: Integration Test Enable Position */
#define CMSDK_Watchdog_INTEGTESTEN_Msk       (0x1ul << CMSDK_Watchdog_INTEGTESTEN_Pos)       /* CMSDK_Watchdog INTEGTESTEN: Integration Test Enable Mask */

/* CMSDK_WATCHDOG INTEGTESTOUTSET Register Definitions */
#define CMSDK_Watchdog_INTEGTESTOUTSET_Pos    1                                              /* CMSDK_Watchdog INTEGTESTOUTSET: Integration Test Output Set Position */
#define CMSDK_Watchdog_INTEGTESTOUTSET_Msk   (0x1ul << CMSDK_Watchdog_INTEGTESTOUTSET_Pos)   /* CMSDK_Watchdog INTEGTESTOUTSET: Integration Test Output Set Mask */


#define SEC_WATCHDOG            ((CMSDK_WATCHDOG_TypeDef  *)APB_WATCHDOG_BASE_S)

/* SPCTRL memory mapped register access structure */
struct spctrl_def {
    volatile uint32_t apbnspppc0_nonpriv;
    volatile uint32_t apbnsppc0_nonsec;
    volatile uint32_t apbnspppc1_nonpriv;
    volatile uint32_t apbnsppc1_nonsec;
    volatile uint32_t ahbnspppc0_nonpriv;
    volatile uint32_t ahbnsppc0_nonsec;
    volatile uint32_t version;
};

/* ARM APB PPC0 peripherals definition */
#define CMSDK_TIMER0_APB_PPC_POS    0U
#define CMSDK_TIMER1_APB_PPC_POS    1U
#define CMSDK_DTIMER_APB_PPC_POS    2U
#define CMSDK_UART0_APB_PPC_POS     4U
#define CMSDK_UART1_APB_PPC_POS     5U
#define CMSDK_UART2_APB_PPC_POS     6U
/* End ARM APB PPC0 peripherals definition */

/* ARM APB PPC1 peripherals definition */
#define CMSDK_SPI0_APB_PPC_POS      0U
#define CMSDK_SPI1_APB_PPC_POS      1U
#define CMSDK_I2C0_APB_PPC_POS      2U
#define CMSDK_I2C1_APB_PPC_POS      3U
#define CMSDK_I2S_APB_PPC_POS       4U
#define CMSDK_FPGA_IO_PPC_POS       8U
#define CMSDK_FPGA_SCC_PPC_POS      15U
/* End ARM APB PPC1 peripherals definition */

/* ARM AHB PPC0 peripherals definition */
#define CMSDK_GPIO0_PPC_POS         0U
#define CMSDK_GPIO1_PPC_POS         1U
#define CMSDK_GPIO2_PPC_POS         2U
#define CMSDK_GPIO3_PPC_POS         3U
#define CMSDK_SYS_CTRL_REG_POS      15U
/* End of ARM AHB PPCEXP0 peripherals definition */

#endif  /* __ARM_LTD_AN495_RETARGET_H__ */
