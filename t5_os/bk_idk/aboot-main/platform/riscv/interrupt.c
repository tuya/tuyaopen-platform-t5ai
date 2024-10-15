/*
 * Copyright (c) 2012-2021 Andes Technology Corporation
 * All rights reserved.
 *
 */

#include <stdio.h>
#include "platform.h"
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_uart2.h"

#define INT_NUMBER_MAX              (64)
typedef void (*isr_func)(void);

void default_irq_handler(void)
{
	//printf("Default interrupt handler\n");
}

#if 0
void wdt_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void rtc_period_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void rtc_alarm_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void pit_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void spi1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void spi2_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void i2c_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void gpio_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void uart1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void uart2_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void dma_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void swint_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void ac97_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void sdc_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void lcd_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void touch_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void standby_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void wakeup_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));

const isr_func irq_handler[] = {
	wdt_irq_handler,
	rtc_period_irq_handler,
	rtc_alarm_irq_handler,
	pit_irq_handler,
	spi1_irq_handler,
	spi2_irq_handler,
	i2c_irq_handler,
	gpio_irq_handler,
	uart1_irq_handler,
	uart2_irq_handler,
	dma_irq_handler,
	default_irq_handler,
	swint_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	ac97_irq_handler,
	sdc_irq_handler,
	mac_irq_handler,
	lcd_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	touch_irq_handler,
	standby_irq_handler,
	wakeup_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler
};
#endif

void arch_interrupt_ctrl(unsigned int ctrl)
{
        volatile unsigned int i = 0;
        if(ctrl == 0x1)
        { 
                //clear_csr(NDS_MIE, MIP_MEIP | MIP_MTIP | MIP_MSIP); 
                for (i = 1; i < INT_NUMBER_MAX; i++)
                {
                        HAL_INTERRUPT_SET_LEVEL(i, 0x01);   //set default Priority.  Priority level must be set > 0
                        HAL_INTERRUPT_ENABLE(i);         // Enable PLIC interrupt  source
                }
                set_csr(NDS_MIE, MIP_MEIP);			/* Enable the Machine External/Timer/Sofware interrupt bit in MIE. */
                set_csr(NDS_MSTATUS, MSTATUS_MIE);	/* Enable interrupts in general. */
         }
        else
        {
                //clear_csr(NDS_MIE, MIP_MEIP);			/* Disable the Machine External/Timer/Sofware interrupt bit in MIE. */
                clear_csr(NDS_MIE, MIP_MEIP | MIP_MTIP | MIP_MSIP); 
                clear_csr(NDS_MSTATUS, MSTATUS_MIE);	/* Disable interrupts in general. */
                for (i = 1; i < INT_NUMBER_MAX; i++)
                {
                        HAL_INTERRUPT_DISABLE(i);         /* Disable PLIC interrupt  source */
                }
                write_csr(NDS_MCAUSE,0x0);
        }
}

void uart_irq_handler(void)
{
        UART0_InterruptHandler();
}

void uart1_irq_handler(void)
{
        UART1_InterruptHandler();
}

void uart2_irq_handler(void)
{
        UART2_InterruptHandler();
}


void bmc32_int_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void host_0_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void host_0_sec_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void timer_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void pwm_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void i2c_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void spi_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void sadc_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void irda_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void sdio_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void gdma_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void la_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void timer1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void i2c1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void spi1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void can_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void usb_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void qspi_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void fft_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void sbc_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void aud_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void i2s_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void jpegenc_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void jpegdec_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void lcd_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void intn_phy_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_tx_rx_timer_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_tx_rx_misc_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_rx_trigger_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_tx_trigger_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_port_trigger_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mac_int_gen_n_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void hsu_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void int_mac_wakeup_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void dm_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void ble_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void bt_irq_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mboxdefault_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void mbox1_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void bmc64_int_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void touched_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void usbplug_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void rtc_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));
void gpio_irq_handler(void) __attribute__((weak, alias("default_irq_handler")));

const isr_func irq_handler[] = {
	bmc32_int_irq_handler,
	host_0_irq_irq_handler,
	host_0_sec_irq_irq_handler,
	timer_irq_handler,
	uart_irq_handler,
	pwm_irq_handler,
	i2c_irq_handler,
	spi_irq_handler,
	sadc_irq_handler,
	irda_irq_handler,
	sdio_irq_handler,
	gdma_irq_handler,
	la_irq_handler,
	timer1_irq_handler,
	i2c1_irq_handler,
	uart1_irq_handler,
	uart2_irq_handler,
	spi1_irq_handler,
	can_irq_handler,
	usb_irq_handler,
	qspi_irq_handler,
	fft_irq_handler,
	sbc_irq_handler,
	aud_irq_handler,
	i2s_irq_handler,
	jpegenc_irq_handler,
	jpegdec_irq_handler,
	lcd_irq_handler,
	default_irq_handler,
	intn_phy_irq_handler,
	mac_int_tx_rx_timer_n_irq_handler,
	mac_int_tx_rx_misc_n_irq_handler,
	mac_int_rx_trigger_n_irq_handler,
	mac_int_tx_trigger_n_irq_handler,
	mac_int_port_trigger_n_irq_handler,
	mac_int_gen_n_irq_handler,
	hsu_irq_irq_handler,
	int_mac_wakeup_irq_handler,
	default_irq_handler,
	dm_irq_irq_handler,
	ble_irq_irq_handler,
	bt_irq_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	default_irq_handler,
	mboxdefault_irq_handler,
	mbox1_irq_handler,
	bmc64_int_irq_handler,
	default_irq_handler,
	touched_irq_handler,
	usbplug_irq_handler,
	rtc_irq_handler,
	gpio_irq_handler
};
unsigned int g_irq_source = 0;
void mext_interrupt(unsigned int irq_source)
{
	/* Enable interrupts in general to allow nested */
	//set_csr(NDS_MSTATUS, MSTATUS_MIE);
	g_irq_source = irq_source;
	/* Do interrupt handler */
	irq_handler[irq_source]();
	__nds__plic_complete_interrupt(irq_source);

	/* Disable interrupt in general to restore context */
	//clear_csr(NDS_MSTATUS, MSTATUS_MIE);
}
