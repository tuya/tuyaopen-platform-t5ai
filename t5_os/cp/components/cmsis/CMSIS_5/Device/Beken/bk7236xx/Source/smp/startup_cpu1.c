/*
 * Copyright (c) 2009-2019 Arm Limited.
 * Copyright (c) 2019-2020 ArmChina.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "os/os.h"
#include "components/log.h"
#include "components/system.h"
#include "bk7236xx.h"
#include "aon_pmu_driver.h"
#include "sys_driver.h"
#include "driver/uart.h"
#include "wdt_driver.h"
#include "bk_pm_internal_api.h"
#include "smp.h"

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#if CONFIG_MPU
#include "mpu.h"
#endif

#define TAG "arch"

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __StackTopCpu1;
extern uint32_t __StackLimitCpu1;
extern uint32_t __vector_core1_table;

extern __NO_RETURN void __PROGRAM_START(void);
extern void SystemInitCpu1(void);

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler_Cpu1  (void);
            void Default_Handler(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Exceptions */
void NMI_Handler            (void) __attribute__ ((weak));
void HardFault_Handler      (void) __attribute__ ((weak));
void MemManage_Handler      (void);
void BusFault_Handler       (void);
void UsageFault_Handler     (void);
void SecureFault_Handler    (void);
void SVC_Handler            (void);
void DebugMon_Handler       (void);
void PendSV_Handler         (void);
void SysTick_Handler        (void);

/* Interrupt Handler */
void DMA0_NSEC_Handler               (void) __attribute__ ((weak));
void ENCP_SEC_Handler                (void) __attribute__ ((weak));
void ENCP_NSEC_Handler               (void) __attribute__ ((weak));
void TIMER0_Handler                  (void) __attribute__ ((weak));
void UART0_Handler                   (void) __attribute__ ((weak));
void PWM0_Handler                    (void) __attribute__ ((weak));
void I2C0_Handler                    (void) __attribute__ ((weak));
void SPI0_Handler                    (void) __attribute__ ((weak));
void SARADC_Handler                  (void) __attribute__ ((weak));
void IRDA_Handler                    (void) __attribute__ ((weak));
void SDIO_Handler                    (void) __attribute__ ((weak));
void GDMA_Handler                    (void) __attribute__ ((weak));
void LA_Handler                      (void) __attribute__ ((weak));
void TIMER1_Handler                  (void) __attribute__ ((weak));
void I2C1_Handler                    (void) __attribute__ ((weak));
void UART1_Handler                   (void) __attribute__ ((weak));
void UART2_Handler                   (void) __attribute__ ((weak));
void SPI1_Handler                    (void) __attribute__ ((weak));
void CAN_Handler                     (void) __attribute__ ((weak));
void USB_Handler                     (void) __attribute__ ((weak));
void QSPI0_Handler                   (void) __attribute__ ((weak));
void CKMN_Handler                    (void) __attribute__ ((weak));
void SBC_Handler                     (void) __attribute__ ((weak));
void AUDIO_Handler                   (void) __attribute__ ((weak));
void I2S0_Handler                    (void) __attribute__ ((weak));
void JPEG_ENC_Handler                (void) __attribute__ ((weak));
void JPEG_DEC_Handler                (void) __attribute__ ((weak));
void DISPLAY_Handler                 (void) __attribute__ ((weak));
void DMA2D_Handler                   (void) __attribute__ ((weak));
void PHY_MBP_Handler                 (void) __attribute__ ((weak));
void PHY_RIU_Handler                 (void) __attribute__ ((weak));
void MAC_INT_TX_RX_TIMER_Handler     (void) __attribute__ ((weak));
void MAC_INT_TX_RX_MISC_Handler      (void) __attribute__ ((weak));
void MAC_INT_RX_TRIGGER_Handler      (void) __attribute__ ((weak));
void MAC_INT_TX_TRIGGER_Handler      (void) __attribute__ ((weak));
void MAC_INT_PORT_TRIGGER_Handler    (void) __attribute__ ((weak));
void MAC_INT_GEN_Handler             (void) __attribute__ ((weak));
void GPIO_NS_Handler                 (void) __attribute__ ((weak));
void INT_MAC_WAKEUP_Handler          (void) __attribute__ ((weak));
void DM_Handler                      (void) __attribute__ ((weak));
void BLE_Handler                     (void) __attribute__ ((weak));
void BT_Handler                      (void) __attribute__ ((weak));
void QSPI1_Handler                   (void) __attribute__ ((weak));
void PWM1_Handler                    (void) __attribute__ ((weak));
void I2S1_Handler                    (void) __attribute__ ((weak));
void I2S2_Handler                    (void) __attribute__ ((weak));
void H264_Handler                    (void) __attribute__ ((weak));
void SDMADC_Handler                  (void) __attribute__ ((weak));
void ETHERNET_Handler                (void) __attribute__ ((weak));
void SCAL0_Handler                   (void) __attribute__ ((weak));
void OTP_Handler                     (void) __attribute__ ((weak));
void DPLL_UNLOCK_Handler             (void) __attribute__ ((weak));
void TOUCH_Handler                   (void) __attribute__ ((weak));
void USB_PLUG_Handler                (void) __attribute__ ((weak));
void RTC_Handler                     (void) __attribute__ ((weak));
void GPIO_Handler                    (void) __attribute__ ((weak));
void DMA1_SEC_Handler                (void) __attribute__ ((weak));
void DMA1_NSEC_Handler               (void) __attribute__ ((weak));
void YUV_BUF_Handler                 (void) __attribute__ ((weak));
void ROTT_Handler                    (void) __attribute__ ((weak));
void SCR7816_Handler                 (void) __attribute__ ((weak));
void LIN_Handler                     (void) __attribute__ ((weak));
void SCAL1_Handler                   (void) __attribute__ ((weak));
void MAILBOX_Handler                 (void) __attribute__ ((weak));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

__attribute__((used, section(".vectors_core1")))
const VECTOR_TABLE_Type __VECTOR_TABLE_CPU1[] = {
  (VECTOR_TABLE_Type)(&__StackTopCpu1),       /*     Initial Stack Pointer */
  Reset_Handler_Cpu1,                            /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*  -5 SVCall Handler */
  DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*  -2 PendSV Handler */
  SysTick_Handler,                          /*  -1 SysTick Handler */

  /* Interrupts */
  DMA0_NSEC_Handler,
  ENCP_SEC_Handler,
  ENCP_NSEC_Handler,
  TIMER0_Handler,
  UART0_Handler,
  PWM0_Handler,
  I2C0_Handler,
  SPI0_Handler,
  SARADC_Handler,
  IRDA_Handler,
  SDIO_Handler,
  GDMA_Handler,
  LA_Handler,
  TIMER1_Handler,
  I2C1_Handler,
  UART1_Handler,
  UART2_Handler,
  SPI1_Handler,
  CAN_Handler,
  USB_Handler,
  QSPI0_Handler,
  CKMN_Handler,
  SBC_Handler,
  AUDIO_Handler,
  I2S0_Handler,
  JPEG_ENC_Handler,
  JPEG_DEC_Handler,
  DISPLAY_Handler,
  DMA2D_Handler,
  PHY_MBP_Handler,
  PHY_RIU_Handler,
  MAC_INT_TX_RX_TIMER_Handler,
  MAC_INT_TX_RX_MISC_Handler,
  MAC_INT_RX_TRIGGER_Handler,
  MAC_INT_TX_TRIGGER_Handler,
  MAC_INT_PORT_TRIGGER_Handler,
  MAC_INT_GEN_Handler,
  GPIO_NS_Handler,
  INT_MAC_WAKEUP_Handler,
  DM_Handler,
  BLE_Handler,
  BT_Handler,
  QSPI1_Handler,
  PWM1_Handler,
  I2S1_Handler,
  I2S2_Handler,
  H264_Handler,
  SDMADC_Handler,
  ETHERNET_Handler,
  SCAL0_Handler,
  OTP_Handler,
  DPLL_UNLOCK_Handler,
  TOUCH_Handler,
  USB_PLUG_Handler,
  RTC_Handler,
  GPIO_Handler,
  DMA1_SEC_Handler,
  DMA1_NSEC_Handler,
  YUV_BUF_Handler,
  ROTT_Handler,
  SCR7816_Handler,
  LIN_Handler,
  SCAL1_Handler,
  MAILBOX_Handler
/* Interrupts 64 .. 480 are left out */
};

#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#define FIREWARE_NAME    "app@cpu1"
#define HARDWARE_VERSION "V1.0.0"
#define SOFTWARE_VERSION "V1.0.0"

#ifndef __STACK_LIMIT1
#define __STACK_LIMIT1             __StackLimitCpu1
#endif

void (*multicore_cpu1_func)(void) = NULL;
void reset_cpu1_core(uint32 offset, uint32_t start_flag);
void reset_cpu2_core(uint32 offset, uint32_t start_flag);
#if CONFIG_FORCE_PROTECT_CHANNEL
bk_err_t bk_mailbox_cc_init_on_current_core(int id);
#endif

void multicore_launch_core1(void (*func)(void))
{
    multicore_cpu1_func = func;
    //smp(cpu0,cpu1) + cpu2
    bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_ON);
    reset_cpu1_core((uint32_t)&__vector_core1_table, 1);

    BK_LOGW(TAG, "%s end\n", __func__);
}

void _othercore_start(void)
{
#if CONFIG_MPU
    mpu_enable();
#if CONFIG_DCACHE
	if (SCB->CLIDR & SCB_CLIDR_DC_Msk)
		SCB_EnableDCache();
		
    SCB_CleanInvalidateDCache();
#endif
#endif // #if CONFIG_MPU

  /* prvDisableInterruptsAndPortStartSchedulerOnCore*/
  if (multicore_cpu1_func) {
  	multicore_cpu1_func();
  }

  while(1) {
  	BK_LOGW(TAG, "@\r\n");
  }
}

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler_Cpu1(void)
{
  rtos_disable_int();
  cpu1_set_core_id();

  __set_MSPLIM((uint32_t)(&__STACK_LIMIT1));

  SystemInitCpu1();                             /* CMSIS System Initialization */

  __TCM_LOADER_START();

#if CONFIG_CM_BACKTRACE
  cm_backtrace_init(FIREWARE_NAME, HARDWARE_VERSION, SOFTWARE_VERSION);
#endif

#if CONFIG_FORCE_PROTECT_CHANNEL
  bk_mailbox_cc_init_on_current_core(rtos_get_core_id());
#endif

  _othercore_start();

  while(1){
  	;
  }
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#endif
// eof

