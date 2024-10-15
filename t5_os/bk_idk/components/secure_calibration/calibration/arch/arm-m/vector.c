#include "platform.h"
#include "pal_log.h"
#include "bl_bk_reg.h"
#include "hal_misc.h"
#include "hal_hw_fih.h"
#include "hal_efuse.h"
#include "bootloader.h"
#include "soc.h"
#include "hal_platform.h"

extern uint32_t __MSPTop;
extern uint32_t __MSPLimit;
extern void bs_main(void);

typedef void(*VECTOR_TABLE_Type)(void);

void NMI_Handler            (void) __attribute__ ((weak));
void HardFault_Handler      (void) __attribute__ ((weak));
void MemManage_Handler      (void) __attribute__ ((weak));
void BusFault_Handler       (void) __attribute__ ((weak));
void UsageFault_Handler     (void) __attribute__ ((weak));
void SecureFault_Handler    (void) __attribute__ ((weak));
void UART_InterruptHandler  (void) __attribute__ ((weak));
const VECTOR_TABLE_Type __VECTOR_TABLE[];

#define EFUSE_REG2             *((volatile unsigned long *) (0x44880000 + 0x2*4))
#define EFUSE_REG4             *((volatile unsigned long *) (0x44880000 + 0x4*4))
#define EFUSE_REG5             *((volatile unsigned long *) (0x44880000 + 0x5*4))
#define EFUSE_READ_VALID       ((EFUSE_REG5 & 0x100) >> 8)

#define PLL_ENABLED    0
#define PLL_DISABLED   1
#define EFUSE_READ_ERR 2

/* Return true if PLL is enabled! */
static inline int deep_sleep_reset(void)
{
	volatile uint32_t loop_cnt = 0;
	uint32_t efuse_data;

	EFUSE_REG2 = 0x03;
	EFUSE_REG4 = 0x01;

	while((EFUSE_READ_VALID == 0)) {
		if (loop_cnt ++ > 1020000) {// about 600ms
			return EFUSE_READ_ERR;
		}
	}

	efuse_data = EFUSE_REG5;
	if( DEEP_SLEEP_RESTART )
	{
		if( (efuse_data & HAL_EFUSE_FAST_BOOT_DISABLE_BIT) == 0 ){
#if CONFIG_HW_FIH
			PRRO_REG19_CMP0_DATA_SRC = 0;
			PRRO_REG1A_CMP0_DATA_DST = 0;
			PRRO_REG1D_CMP1_DATA_SRC = 0;
			PRRO_REG1E_CMP1_DATA_DST = 0;
			PRRO_REG21_CMP2_DATA_SRC = 0;
			PRRO_REG22_CMP2_DATA_DST = 0;
#endif

			control_set_to_flash_not_enable_jtag();
		}
	}

#if CONFIG_FORCE_ENABLE_PLL
	if(efuse_data & HAL_EFUSE_PLL_ENABLE_BIT) {
		hal_enable_pll();
		return PLL_ENABLED;
	}
#else
	// UART download mode don't need to enable PLL
	if((efuse_data & HAL_EFUSE_PLL_ENABLE_BIT) && (efuse_data & HAL_EFUSE_SECURE_BOOT_SUPPORTED_BIT)) {
		hal_enable_pll();
		return PLL_ENABLED;
	}
#endif
	return PLL_DISABLED;
}

extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_size__;
extern uint32_t __bss_start__;
extern uint32_t __bss_size__;

__NO_RETURN void Reset_Handler(void)
{
#if CONFIG_PERF
	GPIO_UP_DOWN(14);
	GPIO_UP(15);
#endif

	/*jtag disable at beginning*/
	hal_jtag_disable();

#if (CONFIG_CACHE)
	SCB_EnableICache();
#endif
	/* Per-tenglong: 480M PLL need 180us to produce stable clock:
         * 50us for calibration, 40us for lockdown, 30us redundant, 60us for removing glitch.
         *
         * The BootROM load need about 124 + 274 = 398us, so enable PLL before loading,
         * then enable core/flash high frequency clock.
         **/
	int ret = deep_sleep_reset();

	__set_MSPLIM((uint32_t)(&__MSPLimit));
	SCB->VTOR = (uint32_t) &(__VECTOR_TABLE[0]);

	uint32_t const *src = (const uint32_t*)&__etext;
	uint32_t *dst = (uint32_t*)&__data_start__;

	//Need about 124us to load data to RAM
	for (int i = 0; i < (uint32_t)&__data_size__; i++) {
		dst[i] = src[i];
	}

	//Need about 274us to load BSS to RAM
	uint32_t *bss = (uint32_t*)&__bss_start__;
	for (int i = 0; i < (uint32_t)&__bss_size__; i++) {
		bss[i] = 0;
	}

	if (ret == EFUSE_READ_ERR) {
		pal_critical_err(CRITICAL_ERR_EFUSE_READ1);
	}

	if (ret == PLL_ENABLED) {
		hal_enable_high_freq();
	}

	bs_main();
	while(1);
}

const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
	(VECTOR_TABLE_Type)(&__MSPTop),           /*     Initial Stack Pointer */
	Reset_Handler,                            /*     Reset Handler */
	NMI_Handler,                              /* -14 NMI Handler */
	HardFault_Handler,                        /* -13 Hard Fault Handler */
	MemManage_Handler,                        /* -12 MPU Fault Handler */
	BusFault_Handler,                         /* -11 Bus Fault Handler */
	UsageFault_Handler,                       /* -10 Usage Fault Handler */
	SecureFault_Handler,                      /*  -9 Secure Fault Handler */
	0,                                        /*  -8 */
	0,                                        /*  -7 */
	0,                                        /*  -6 */
	0,                                        /*  -5 SVCall Handler */
	0,                                        /*  -4 Debug Monitor Handler */
	0,                                        /*  -3 */
	0,                                        /*  -2 PendSV Handler */
	0,                                        /*  -1 SysTick Handler */

	0,                                        /* Interrupt 0 */
	0,                                        /* Interrupt 1 */
	0,                                        /* Interrupt 2 */
	0,                                        /* Interrupt 3 */
	UART_InterruptHandler,                    /* Interrupt 4 */
	0,                                        /* Interrupt 5 */
	0,                                        /* Interrupt 6 */
	0,                                        /* Interrupt 7 */
	0,                                        /* Interrupt 8 */
	0,                                        /* Interrupt 9 */
	0,                                        /* Interrupt 10 */
	0,                                        /* Interrupt 11 */
	0,                                        /* Interrupt 12 */
	0,                                        /* Interrupt 13 */
	0,                                        /* Interrupt 14 */
	0,                                        /* Interrupt 15 */
	0,                                        /* Interrupt 16 */
	0,                                        /* Interrupt 17 */
	0,                                        /* Interrupt 18 */
	0,                                        /* Interrupt 19 */
	0,                                        /* Interrupt 20 */
	0,                                        /* Interrupt 21 */
	0,                                        /* Interrupt 22 */
	0,                                        /* Interrupt 23 */
	0,                                        /* Interrupt 24 */
	0,                                        /* Interrupt 25 */
	0,                                        /* Interrupt 26 */
	0,                                        /* Interrupt 27 */
	0,                                        /* Interrupt 28 */
	0,                                        /* Interrupt 29 */
	0,                                        /* Interrupt 30 */
	0,                                        /* Interrupt 31 */
	0,                                        /* Interrupt 32 */
	0,                                        /* Interrupt 33 */
	0,                                        /* Interrupt 34 */
	0,                                        /* Interrupt 35 */
	0,                                        /* Interrupt 36 */
	0,                                        /* Interrupt 37 */
	0,                                        /* Interrupt 38 */
	0,                                        /* Interrupt 39 */
	0,                                        /* Interrupt 40 */
	0,                                        /* Interrupt 41 */
	0,                                        /* Interrupt 42 */
	0,                                        /* Interrupt 43 */
	0,                                        /* Interrupt 44 */
	0,                                        /* Interrupt 45 */
	0,                                        /* Interrupt 46 */
	0,                                        /* Interrupt 47 */
	(void (*)(void))0x32374B42,
	(void (*)(void))0x00003633,
};

