// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "icu_driver.h"
#include "interrupt_base.h"
#include <driver/int.h>
#include "sdkconfig.h"
#include "armstar.h"
#include "arch_interrupt.h"
#include "components/log.h"

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#if CONFIG_INTERRUPT_DEBUG_RECORDER
#include "interrupt_debug.h"
#include <string.h>
#endif

#if CONFIG_ARCH_INT_STATIS
static uint32_t s_int_statis[InterruptMAX_IRQn] = {0};
#define INT_INC_STATIS(irq) s_int_statis[(irq)] ++
#else
#define INT_INC_STATIS(irq)
#endif

#if (CONFIG_FREERTOS_TRACE)
#define IRQ_TRACE_BEGIN(irq)	xTraceISRBegin(xGetTraceISRHandle(irq))
#define IRQ_TRACE_END()			xTraceISREnd(0)
#else
#define IRQ_TRACE_BEGIN(irq)
#define IRQ_TRACE_END()
#endif


#if CONFIG_INTERRUPT_DEBUG_RECORDER
__attribute__((__used__)) static volatile interrupt_recorder_t  s_interrupt_flag[CONFIG_INTERRUPT_RECORDER_CNT] ;
__attribute__((__used__)) static volatile uint32_t s_int_cnt = 0;

#define ARCH_ISR_HANDLER(irq)                                   \
{                                                               \
	INT_INC_STATIS(irq);                                        \
	IRQ_TRACE_BEGIN(irq);                                       \
	int_group_isr_t isr_cb;                                     \
	isr_cb = arch_interrupt_get_handler(irq);                   \
    s_interrupt_flag[s_int_cnt % CONFIG_INTERRUPT_RECORDER_CNT ].int_flag = irq;           \
    s_interrupt_flag[s_int_cnt % CONFIG_INTERRUPT_RECORDER_CNT].current_cnt = s_int_cnt;   \
	if (isr_cb != NULL) {                                       \
		(*(isr_cb))();                                          \
	}                                                           \
    s_interrupt_flag[s_int_cnt % CONFIG_INTERRUPT_RECORDER_CNT].int_flag |= 0xf0000000;    \
    s_int_cnt ++;                                               \
	IRQ_TRACE_END();                                            \
}
#else
#define ARCH_ISR_HANDLER(irq)                                   \
{                                                               \
	INT_INC_STATIS(irq);                                        \
	IRQ_TRACE_BEGIN(irq);                                       \
	int_group_isr_t isr_cb;                                     \
	isr_cb = arch_interrupt_get_handler(irq);                   \
	if (isr_cb != NULL) {                                       \
		(*(isr_cb))();                                          \
	}                                                           \
	IRQ_TRACE_END();                                            \
}
#endif


void soc_isr_init(void)
{
#if CONFIG_INTERRUPT_DEBUG_RECORDER
    memset((void *)s_interrupt_flag, 0, sizeof(interrupt_recorder_t) * CONFIG_INTERRUPT_RECORDER_CNT);
#endif
	arch_isr_entry_init();
}

void soc_isr_deinit(void)
{
    /*to do something*/
}

void __attribute__ ((interrupt)) DMA0_NSEC_Handler(void)
{
	ARCH_ISR_HANDLER(0);
}

void __attribute__ ((interrupt)) ENCP_SEC_Handler(void)
{
	ARCH_ISR_HANDLER(1);
}

void __attribute__ ((interrupt)) ENCP_NSEC_Handler(void)
{
	ARCH_ISR_HANDLER(2);
}

extern uint32_t     g_wdt_handler_lr;
void __attribute__ ((interrupt)) TIMER0_Handler(void)
{
	g_wdt_handler_lr = __get_LR();
	ARCH_ISR_HANDLER(3);
}


void __attribute__ ((interrupt)) UART0_Handler(void)
{
	ARCH_ISR_HANDLER(4);
}

void __attribute__ ((interrupt)) PWM0_Handler(void)
{
	ARCH_ISR_HANDLER(5);
}

void __attribute__ ((interrupt)) I2C0_Handler(void)
{
	ARCH_ISR_HANDLER(6);
}

void __attribute__ ((interrupt)) SPI0_Handler(void)
{
	ARCH_ISR_HANDLER(7);
}

void __attribute__ ((interrupt)) SARADC_Handler(void)
{
	ARCH_ISR_HANDLER(8);
}

void __attribute__ ((interrupt)) IRDA_Handler(void)
{
	ARCH_ISR_HANDLER(9);
}

void __attribute__ ((interrupt)) SDIO_Handler(void)
{
	ARCH_ISR_HANDLER(10);
}

void __attribute__ ((interrupt)) GDMA_Handler(void)
{
	ARCH_ISR_HANDLER(11);
}

void __attribute__ ((interrupt)) LA_Handler(void)
{
	ARCH_ISR_HANDLER(12);
}

void __attribute__ ((interrupt)) TIMER1_Handler(void)
{
	ARCH_ISR_HANDLER(13);
}

void __attribute__ ((interrupt)) I2C1_Handler(void)
{
	ARCH_ISR_HANDLER(14);
}

void __attribute__ ((interrupt)) UART1_Handler(void)
{
	ARCH_ISR_HANDLER(15);
}

void __attribute__ ((interrupt)) UART2_Handler(void)
{
	ARCH_ISR_HANDLER(16);
}

void __attribute__ ((interrupt)) SPI1_Handler(void)
{
	ARCH_ISR_HANDLER(17);
}

void __attribute__ ((interrupt)) CAN_Handler(void)
{
	ARCH_ISR_HANDLER(18);
}

void __attribute__ ((interrupt)) USB_Handler(void)
{
	ARCH_ISR_HANDLER(19);
}

void __attribute__ ((interrupt)) QSPI0_Handler(void)
{
	ARCH_ISR_HANDLER(20);
}

void __attribute__ ((interrupt)) CKMN_Handler(void)
{
	ARCH_ISR_HANDLER(21);
}

void __attribute__ ((interrupt)) SBC_Handler(void)
{
	ARCH_ISR_HANDLER(22);
}

void __attribute__ ((interrupt)) AUDIO_Handler(void)
{
	ARCH_ISR_HANDLER(23);
}

void __attribute__ ((interrupt)) I2S0_Handler(void)
{
	ARCH_ISR_HANDLER(24);
}

void __attribute__ ((interrupt)) JPEG_ENC_Handler(void)
{
	ARCH_ISR_HANDLER(25);
}

void __attribute__ ((interrupt)) JPEG_DEC_Handler(void)
{
	ARCH_ISR_HANDLER(26);
}

void __attribute__ ((interrupt)) DISPLAY_Handler(void)
{
	ARCH_ISR_HANDLER(27);
}

void __attribute__ ((interrupt)) DMA2D_Handler(void)
{
	ARCH_ISR_HANDLER(28);
}

void __attribute__ ((interrupt)) PHY_MBP_Handler(void)
{
	ARCH_ISR_HANDLER(29);
}

void __attribute__ ((interrupt)) PHY_RIU_Handler(void)
{
	ARCH_ISR_HANDLER(30);
}

void __attribute__ ((interrupt)) MAC_INT_TX_RX_TIMER_Handler(void)
{
	ARCH_ISR_HANDLER(31);
}

void __attribute__ ((interrupt)) MAC_INT_TX_RX_MISC_Handler(void)
{
	ARCH_ISR_HANDLER(32);
}

void __attribute__ ((interrupt)) HSU_Handler(void)
{
	ARCH_ISR_HANDLER(32);
}

void __attribute__ ((interrupt)) MAC_INT_RX_TRIGGER_Handler(void)
{
	ARCH_ISR_HANDLER(33);
}

void __attribute__ ((interrupt)) MAC_INT_TX_TRIGGER_Handler(void)
{
	ARCH_ISR_HANDLER(34);
}

void __attribute__ ((interrupt)) MAC_INT_PORT_TRIGGER_Handler(void)
{
	ARCH_ISR_HANDLER(35);
}

void __attribute__ ((interrupt)) MAC_INT_GEN_Handler(void)
{
	ARCH_ISR_HANDLER(36);
}

void __attribute__ ((interrupt)) GPIO_NS_Handler(void)
{
	ARCH_ISR_HANDLER(37);
}

void __attribute__ ((interrupt)) INT_MAC_WAKEUP_Handler(void)
{
	ARCH_ISR_HANDLER(38);
}

void __attribute__ ((interrupt)) DM_Handler(void)
{
	ARCH_ISR_HANDLER(39);
}

void __attribute__ ((interrupt)) BLE_Handler(void)
{
	ARCH_ISR_HANDLER(40);
}

void __attribute__ ((interrupt)) BT_Handler(void)
{
	ARCH_ISR_HANDLER(41);
}

void __attribute__ ((interrupt)) QSPI1_Handler(void)
{
	ARCH_ISR_HANDLER(42);
}

void __attribute__ ((interrupt)) PWM1_Handler(void)
{
	ARCH_ISR_HANDLER(43);
}

void __attribute__ ((interrupt)) I2S1_Handler(void)
{
	ARCH_ISR_HANDLER(44);
}

void __attribute__ ((interrupt)) I2S2_Handler(void)
{
	ARCH_ISR_HANDLER(45);
}

void __attribute__ ((interrupt)) H264_Handler(void)
{
	ARCH_ISR_HANDLER(46);
}

void __attribute__ ((interrupt)) SDMADC_Handler(void)
{
	ARCH_ISR_HANDLER(47);
}

void __attribute__ ((interrupt)) ETHERNET_Handler(void)
{
	ARCH_ISR_HANDLER(48);
}

void __attribute__ ((interrupt)) SCAL0_Handler(void)
{
	ARCH_ISR_HANDLER(49);
}

void __attribute__ ((interrupt)) OTP_Handler(void)
{
	ARCH_ISR_HANDLER(50);
}

void __attribute__ ((interrupt)) DPLL_UNLOCK_Handler(void)
{
	BK_LOGD(NULL,"%s enter\r\n",__func__);
	ARCH_ISR_HANDLER(51);
}

void __attribute__ ((interrupt)) TOUCH_Handler(void)
{
	ARCH_ISR_HANDLER(52);
}

void __attribute__ ((interrupt)) USB_PLUG_Handler(void)
{
	ARCH_ISR_HANDLER(53);
}

void __attribute__ ((interrupt)) RTC_Handler(void)
{

#if CONFIG_RTC_TIMER_PRECISION_TEST_BY_GPIO
	static uint8_t gpio_up = 1;
	if (gpio_up) {
		GPIO_UP(14);
	} else {
		GPIO_DOWN(14);
	}
	gpio_up = !gpio_up;
#endif

#if CONFIG_RTC_TIMER_PRECISION_TEST
	extern uint32_t g_26m_current_cnt;
	REG_WRITE((SOC_TIMER0_REG_BASE + (8 << 2)), 1); // Read timer0
	while(REG_READ(SOC_TIMER0_REG_BASE + (8 << 2)) & 1); // Wait hardware read ready
	g_26m_current_cnt = REG_READ(SOC_TIMER0_REG_BASE + (9 << 2)); // Read timer0
#endif

	ARCH_ISR_HANDLER(54);
}

void __attribute__ ((interrupt)) GPIO_Handler(void)
{
	ARCH_ISR_HANDLER(55);
}

void __attribute__ ((interrupt)) DMA1_SEC_Handler(void)
{
	ARCH_ISR_HANDLER(56);
}

void __attribute__ ((interrupt)) DMA1_NSEC_Handler(void)
{
	ARCH_ISR_HANDLER(57);
}

void __attribute__ ((interrupt)) YUV_BUF_Handler(void)
{
	ARCH_ISR_HANDLER(58);
}

void __attribute__ ((interrupt)) ROTT_Handler(void)
{
	ARCH_ISR_HANDLER(59);
}

void __attribute__ ((interrupt)) BK7816_Handler(void)
{
	ARCH_ISR_HANDLER(60);
}

void __attribute__ ((interrupt)) LIN_Handler(void)
{
	ARCH_ISR_HANDLER(61);
}

void __attribute__ ((interrupt)) SCAL1_Handler(void)
{
	ARCH_ISR_HANDLER(62);
}

void __attribute__ ((interrupt)) MAILBOX_Handler(void)
{
	ARCH_ISR_HANDLER(63);
}

// eof

