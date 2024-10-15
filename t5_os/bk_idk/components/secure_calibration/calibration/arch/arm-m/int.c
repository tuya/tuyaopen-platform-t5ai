#include "platform.h"
#include "pal_log.h"

void arch_enable_uart_int(void)
{
	NVIC_EnableIRQ(4);
}

void NMI_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_NMI);
	while(1);
}

void MemManage_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_MM);
	while(1);
}

void HardFault_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_HF);
	while(1);
}

void BusFault_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_BF);
	while(1);
}

void UsageFault_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_UF);
	while(1);
}

void SecureFault_Handler(void)
{
	pal_critical_err(CRITICAL_ERR_SF);
	while(1);
}
