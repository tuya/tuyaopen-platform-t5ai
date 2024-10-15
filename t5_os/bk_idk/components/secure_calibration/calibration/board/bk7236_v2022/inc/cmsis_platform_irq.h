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

#ifndef __PLATFORM_IRQ_H__
#define __PLATFORM_IRQ_H__

#ifndef __ASSEMBLY__
typedef enum _IRQn_Type {
    NonMaskableInt_IRQn         = -14,  /* Non Maskable Interrupt */
    HardFault_IRQn              = -13,  /* HardFault Interrupt */
    MemoryManagement_IRQn       = -12,  /* Memory Management Interrupt */
    BusFault_IRQn               = -11,  /* Bus Fault Interrupt */
    UsageFault_IRQn             = -10,  /* Usage Fault Interrupt */
    SecureFault_IRQn            = -9,   /* Secure Fault Interrupt */
    SVCall_IRQn                 = -5,   /* SV Call Interrupt */
    DebugMonitor_IRQn           = -4,   /* Debug Monitor Interrupt */
    PendSV_IRQn                 = -2,   /* Pend SV Interrupt */
    SysTick_IRQn                = -1,   /* System Tick Interrupt */

    UARTRX0_IRQn                = 0,    /* UART 0 RX Interrupt */
    UARTTX0_IRQn                = 1,    /* UART 0 TX Interrupt */
    UARTRX1_IRQn                = 2,    /* UART 1 RX Interrupt */
    UARTTX1_IRQn                = 3,    /* UART 1 TX Interrupt */
    UARTRX2_IRQn                = 4,    /* UART 2 RX Interrupt */
    UARTTX2_IRQn                = 5,    /* UART 2 TX Interrupt */
    GPIO0_IRQn                  = 6,    /* GPIO 0 Combined Interrupt */
    GPIO1_IRQn                  = 7,    /* GPIO 1 Combined Interrupt */
    TIMER0_IRQn                 = 8,    /* TIMER 0 Interrupt */
    TIMER1_IRQn                 = 9,    /* TIMER 1 Interrupt */
    DUALTIMER_IRQn              = 10,   /* Dual Timer Interrupt */
    SPI0_IRQn                   = 11,   /* SPI 0 Interrupt */
    UARTOVF_IRQn                = 12,   /* UART Overflow (0, 1, 2, 3 & 4) */
    ETHERNET_IRQn               = 13,   /* ethernet Interrupt*/
    I2S_IRQn                    = 14,   /* Audio I2S Interrupt */
    DMA_DONE_IRQn               = 15,   /* DMA done Interrupt */
    GPIO0_0_IRQn                = 16,   /* GPIO 2 Combined Interrupt */
    GPIO0_1_IRQn                = 17,
    GPIO0_2_IRQn                = 18,
    GPIO0_3_IRQn                = 19,
    GPIO0_4_IRQn                = 20,
    GPIO0_5_IRQn                = 21,
    GPIO0_6_IRQn                = 22,
    GPIO0_7_IRQn                = 23,
    GPIO0_8_IRQn                = 24,
    GPIO0_9_IRQn                = 25,
    GPIO0_10_IRQn               = 26,
    GPIO0_11_IRQn               = 27,
    GPIO0_12_IRQn               = 28,
    GPIO0_13_IRQn               = 29,
    GPIO0_14_IRQn               = 30,
    GPIO0_15_IRQn               = 31,
    DUBHE_S_IRQn                = 32,
    DUBHE_NS_IRQn               = 33,
}IRQn_Type;
#endif /* __ASSEMBLY__ */

#endif  /* __PLATFORM_IRQ_H__ */
