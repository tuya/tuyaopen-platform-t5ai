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

#include <stdint.h>
#include "boot.h"
#include "sdkconfig.h"
#include "reset_reason.h"
#include "aon_pmu_driver.h"
#include <os/os.h>
#include "bk_arch.h"
#include "bk_rtos_debug.h"
#include <components/system.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include "arch_interrupt.h"
#include "stack_base.h"
#include <driver/wdt.h>
#include <driver/pwr_clk.h>
#include "wdt_driver.h"
#include "bk_aon_wdt.h"
#include "armstar.h"
#include "sys_hal.h"
#include "cpu_id.h"
#include "mb_ipc_cmd.h"

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if CONFIG_DUMP_INTO_FLASH
#include "flash_dump.h"
#endif

#include "_ppc.h"
#include <soc/reg_base.h>
#include "spinlock.h"

#define MAX_DUMP_SYS_MEM_COUNT       (8)
#define SOC_DTCM_DATA_SIZE           (0x4000)
#define SOC_ITCM_DATA_SIZE           (0x4000)
#define SOC_SRAM_TOTAL_SIZE          (0xA0000)
#define SOC_SRAM_DATA_END            (SOC_SRAM0_DATA_BASE + SOC_SRAM_TOTAL_SIZE)
#define SOC_PSRAM_DATA_END           (SOC_PSRAM_DATA_BASE + SOC_PSRAM_DATA_SIZE)

#define SOC_SYS_CPU0_INT_0_31_EN_VALUE  (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x20 << 2)))
#define SOC_SYS_CPU0_INT_32_63_EN_VALUE (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x21 << 2)))
#define SOC_SYS_CPU1_INT_0_31_EN_VALUE  (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x22 << 2)))
#define SOC_SYS_CPU1_INT_32_63_EN_VALUE (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x23 << 2)))
#define SOC_SYS_CPU2_INT_0_31_EN_VALUE  (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x24 << 2)))
#define SOC_SYS_CPU2_INT_32_63_EN_VALUE (*(volatile uint32_t *)(SOC_SYS_REG_BASE + (0x25 << 2)))

#define TRAP_DEFAULT_MAGIC_HEAD       (0xabcd9527)  // Default initialization value
#define TRAP_DEFAULT_MAGIC_TAIL       (0x9527abcd)  // Default initialization value
#define TRAP_DEFAULT_DUMPER           (0xffffffff)  // Default dump value

#if (CONFIG_SOC_SMP)
extern unsigned char __itcm_cpu0_end__;
extern unsigned char __dtcm_cpu0_start__;
extern unsigned char _estack_cpu0;
extern unsigned char _sstack_cpu0;

#define __dtcm_end__  __itcm_cpu0_end__
#define __dtcm_start__ __dtcm_cpu0_start__
#define _estack _estack_cpu0
#define _sstack _sstack_cpu0
#endif

typedef struct sys_mem_info
{
    uint32_t mem_base_addr;
    uint32_t mem_size;
} sys_mem_info_t;

typedef struct sys_dump_lock
{
    volatile uint32_t magic_head;
    volatile spinlock_t dump_spin_lock;
    volatile uint32_t magic_tail;
} sys_dump_lock_t;

static unsigned int s_mem_count = 0;
static sys_mem_info_t s_dump_sys_mem_info[MAX_DUMP_SYS_MEM_COUNT] = {0};

static hook_func s_wifi_dump_func = NULL;
static hook_func s_ble_dump_func = NULL;



#define CPU_ID     rtos_get_core_id()


static const char * const fault_type[] =
{
	[0]  = NULL,
	[1]  = NULL,
	[2]  = "Watchdog\r\n",
	[3]  = "HardFault\r\n",
	[4]  = "MemFault\r\n",
	[5]  = "BusFault\r\n",
	[6]  = "UsageFault\r\n",
	[7]  = "SecureFault\r\n",
	[8]  = NULL,
	[9]  = NULL,
	[10] = NULL,
	[11] = "SVC\r\n",
	[12] = "DebugFault\r\n",
	[13] = NULL,
	[14] = "PendSV\r\n",
	[15] = "SysTick\r\n",
};


static inline int is_cp_in_dump_mode(void);
static inline int is_ap_in_dump_mode(void);

static SPINLOCK_SECTION volatile sys_dump_lock_t s_dump_spin_lock = {TRAP_DEFAULT_MAGIC_HEAD, SPIN_LOCK_ACQUIRE_INIT, TRAP_DEFAULT_MAGIC_TAIL};

extern void stop_cpu1_core(void);
extern void stop_cpu2_core(void);


volatile unsigned int s_cpu0_int_0_31_en_value = 0;
volatile unsigned int s_cpu0_int_32_63_en_value = 0;
volatile unsigned int s_cpu1_int_0_31_en_value = 0;
volatile unsigned int s_cpu1_int_32_63_en_value = 0;
volatile unsigned int s_cpu2_int_0_31_en_value = 0;
volatile unsigned int s_cpu2_int_32_63_en_value = 0;

int code_addr_is_valid(uint32_t addr);

int data_addr_is_valid(uint32_t start_addr, uint32_t end_addr) {

    if (start_addr >= end_addr) {
        return 0;
    }

    if (start_addr > SOC_SRAM0_DATA_BASE && start_addr < SOC_SRAM_DATA_END
        && end_addr > SOC_SRAM0_DATA_BASE && end_addr < SOC_SRAM_DATA_END) {
        return 1;
    }

    if (start_addr > SOC_PSRAM_DATA_BASE && start_addr < SOC_PSRAM_DATA_END
        && end_addr > SOC_PSRAM_DATA_BASE && end_addr < SOC_PSRAM_DATA_END) {
        return 1;
    }

    return 0;
}
static uint32_t emergency_get_uart_base(uint32_t uart_id)
{
    switch (uart_id) {
        case 0: return SOC_UART0_REG_BASE;
        case 1: return SOC_UART1_REG_BASE;
        case 2: return SOC_UART2_REG_BASE;
        default: return SOC_UART0_REG_BASE;
    }
}

/**
 * @brief Direct UART character output (independent of any system functions)
 */
 static void emergency_uart_putc(uint32_t uart_id, char c)
 {
    // FIFO status bits
#define SOC_UART_TX_FIFO_EMPTY   (1 << 17)
#define SOC_UART_TX_FIFO_FULL    (1 << 16)
#define SOC_UART_FIFO_STATUS     0x18
#define SOC_UART_FIFO_PORT       0x1C

    uint32_t uart_base = emergency_get_uart_base(uart_id);

    volatile uint32_t *uart_fifo_status = (volatile uint32_t *)((uintptr_t)(uart_base + SOC_UART_FIFO_STATUS));
    volatile uint32_t *uart_fifo_port = (volatile uint32_t *)((uintptr_t)(uart_base + SOC_UART_FIFO_PORT));
     
     // Wait for TX FIFO to have space
    while ((*uart_fifo_status & SOC_UART_TX_FIFO_FULL) != 0) {
        // Simple delay to avoid infinite loop
        for (volatile int i = 0; i < 100; i++);
    }
     
    // Send character
    *uart_fifo_port = (uint32_t)c;
 }
 

 bk_err_t emergency_uart_write_string(uint32_t uart_id, const char *string)
{
	const char *p = string;

	while (*string) {
		if (*string == '\n') {
			if (p == string || *(string - 1) != '\r')
				emergency_uart_putc(uart_id, '\r'); /* append '\r' */
		}
		emergency_uart_putc(uart_id, *string++);
	}

	return BK_OK;
}

bk_err_t emergency_uart_write_buf(uint32_t uart_id, const char *data_buff, uint16_t data_len)
{
	uint16_t i = 0;

	while (i < data_len) {
		emergency_uart_putc(uart_id, data_buff[i]);
		i++;
	}

	return BK_OK;
}

/**
 * @brief Peterson algorithm (shared memory) implementation for spinlock protection between AP and CP
 * 
 * Since AP side is an SMP system, when AP subsystem enters exception, it first acquires SMP system spinlock,
 * then shuts down the other core. AP and CP are AMP system, this solution mainly describes using Peterson
 * algorithm (shared memory) to implement spinlock protection between AP and CP.
    ________________________________________
    1. Objectives
    • SMP CP/AP share a section of SRAM (fixed address), when any subsystem enters exception:
    o First arrival monopolizes UART → DUMP system info → direct reboot (without releasing lock);
    o Later arrival waits permanently (WFE()), avoiding UART conflicts.
    • Ensure reliable fallback solution even when shared area is corrupted.

    ________________________________________
    2. Shared memory layout (fixed address macros)
    • MAGIC_BEGIN / MAGIC_END: Header/tail magic numbers for validating shared area legitimacy.
    • CP_EXCEPTION_STATUS / AP_EXCEPTION_STATUS: Peterson's want[0]/want[1].
    • EXCEPTION_TURN: turn in Peterson algorithm.
    • EXCEPTION_DUMPER: marks who holds the lock (0=CP, 1=AP, -1=none); also used for "first come first serve DUMP" in fallback.
    ________________________________________

 */

/* ================= Utility functions ================= */
static inline void dmb_ish(void){ __asm volatile("dmb ish" ::: "memory"); }
static inline void cpu_wfe(void){ __asm volatile("wfe" ::: "memory"); }
static inline void cpu_sev(void){ __asm volatile("sev" ::: "memory"); }

/* ================= Validation / Repair ================= */
static inline int values_sane(void){
    uint32_t w_cp = FIXED_ADDR_CP_EXCEPTION_STATUS;
    uint32_t w_ap = FIXED_ADDR_AP_EXCEPTION_STATUS;
    uint32_t turn = FIXED_ADDR_EXCEPTION_TURN;
    uint32_t  dump = FIXED_ADDR_EXCEPTION_DUMPER;
    if (!((w_cp==0u || w_cp==1u) && (w_ap==0u || w_ap==1u))) return 0;
    if (!(turn==0u || turn==1u)) return 0;
    if (!(dump== TRAP_DEFAULT_DUMPER || dump==0 || dump==1)) return 0;
    return 1;
}


static inline int shared_valid(void){
    if (FIXED_ADDR_EXCEPTION_MAGIC_BEGIN != TRAP_DEFAULT_MAGIC_HEAD) return 0;
    if (FIXED_ADDR_EXCEPTION_MAGIC_END   != TRAP_DEFAULT_MAGIC_TAIL) return 0;
    if (!values_sane()) return 0;
    return 1;
}
static inline int shared_repair(void){
    /* Re-seal header/tail + clear key fields (write header→clear values→write tail), with memory barrier for visibility */
    FIXED_ADDR_EXCEPTION_MAGIC_BEGIN = TRAP_DEFAULT_MAGIC_HEAD;
    dmb_ish();
    FIXED_ADDR_CP_EXCEPTION_STATUS = 0u;
    FIXED_ADDR_AP_EXCEPTION_STATUS = 0u;
    FIXED_ADDR_EXCEPTION_TURN      = 0u;
    FIXED_ADDR_EXCEPTION_DUMPER    = TRAP_DEFAULT_DUMPER; /* -1 */
    dmb_ish();
    FIXED_ADDR_EXCEPTION_MAGIC_END = TRAP_DEFAULT_MAGIC_TAIL;
    dmb_ish();
    return shared_valid();
}


/* Is current Dump owner:
 * Returns: 1 = CP is current dump owner, 0 = CP is not current dump owner
 * Note: Even if magic area is corrupted, as long as DUMPER is correctly written to 0/1 (including fallback logic),
 *       it can accurately return the owner.
 */
 static inline int is_cp_in_dump_mode(void){
    __asm volatile("dmb ish" ::: "memory");
    uint32_t dumper = FIXED_ADDR_EXCEPTION_DUMPER;
    __asm volatile("dmb ish" ::: "memory");
    if (dumper == TRAP_DEFAULT_DUMPER) return 0;          /* Clearly not held */
    if (FIXED_ADDR_EXCEPTION_MAGIC_BEGIN != TRAP_DEFAULT_MAGIC_HEAD) return 0;
    if (FIXED_ADDR_EXCEPTION_MAGIC_END   != TRAP_DEFAULT_MAGIC_TAIL) return 0;

    if (dumper == 0) return 1;

    return 0;
}

/* Is current Dump owner:
 * Returns: 1 = AP is current dump owner, 0 = AP is not current dump owner
 * Note: Even if magic area is corrupted, as long as DUMPER is correctly written to 0/1 (including fallback logic),
 *       it can accurately return the owner.
 */
static inline int is_ap_in_dump_mode(void){
    __asm volatile("dmb ish" ::: "memory");
    uint32_t dumper = FIXED_ADDR_EXCEPTION_DUMPER;
    __asm volatile("dmb ish" ::: "memory");
    if (dumper == TRAP_DEFAULT_DUMPER) return 0;          /* Clearly not held */
    if (FIXED_ADDR_EXCEPTION_MAGIC_BEGIN != TRAP_DEFAULT_MAGIC_HEAD) return 0;
    if (FIXED_ADDR_EXCEPTION_MAGIC_END   != TRAP_DEFAULT_MAGIC_TAIL) return 0;

    if (dumper == 1) return 1;

    return 0;
}

/* ================= Power-on/Reset early initialization ================= */
void exception_sync_init_early(void){
    (void)shared_repair();
    dmb_ish();
}
/* ================= Peterson lock (volatile + DMB, symmetric) ================= */
static inline void peterson_lock(unsigned me){
    unsigned other = (me ^ 1u);
    FIXED_ADDR_AP_EXCEPTION_STATUS = 1u;               /* I want to enter */
    FIXED_ADDR_EXCEPTION_TURN = other;/* Give priority to the other */
    dmb_ish();                        /* Flush write buffer + establish ordering */
    while ((FIXED_ADDR_CP_EXCEPTION_STATUS == 1u) && (FIXED_ADDR_EXCEPTION_TURN == other)){
        cpu_wfe();                    /* Wait for the other to yield/exit */
    }
    /* Enter critical section */
}
/* ================= Fallback (current CPU directly DUMP; other core waits permanently) ================= */
static inline void fallback_take_and_dump(unsigned me){
    /* Grab fallback ownership: write DUMPER as me; if read back is not me, other side got it first → wait permanently */
    FIXED_ADDR_EXCEPTION_DUMPER = me;
    dmb_ish();
    if (FIXED_ADDR_EXCEPTION_DUMPER != me){
        for(;;) cpu_wfe();            /* Avoid UART conflicts */
    }
}
/* ================= Unified exception handling (shared by both cores) ================= */
static void exception_get_peterson_lock(unsigned me /*CP_ID/AP_ID*/){
    __disable_irq();
    dmb_ish();
    /* Shared area corrupted/uninitialized → try to repair; if repair fails → fallback (current core directly DUMP, other core waits) */
    if (!shared_valid()){
        if (!shared_repair()){
            fallback_take_and_dump(me);
        } else {
            /* Normal path: Peterson mutual exclusion enter exception critical section */
            peterson_lock(me);
            /* Declare me as critical section holder (also used as fallback placeholder reuse, no additional field) */
            FIXED_ADDR_EXCEPTION_DUMPER = me;
        }
    } else {
        /* Normal path: Peterson mutual exclusion enter exception critical section */
        peterson_lock(me);
        /* Declare me as critical section holder (also used as fallback placeholder reuse, no additional field) */
        FIXED_ADDR_EXCEPTION_DUMPER = me;
    }

    dmb_ish();
    cpu_sev(); /* Wake up the other side to make it quickly stuck in the waiting loop */
}


static void rtos_dump_plat_memory(void) {
#if CONFIG_SOC_BK7236XX
    // Dump DTCM
    stack_mem_dump((uint32_t)SOC_DTCM_DATA_BASE, (uint32_t)(SOC_DTCM_DATA_BASE + SOC_DTCM_DATA_SIZE));

    // Dump ITCM
    stack_mem_dump((uint32_t)(SOC_ITCM_DATA_BASE + 0x20) , (uint32_t)(SOC_ITCM_DATA_BASE + SOC_ITCM_DATA_SIZE));
#endif

    // Dump All SRAM
    stack_mem_dump((uint32_t)SOC_SRAM3_DATA_BASE, (uint32_t)SOC_SRAM4_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM4_DATA_BASE, (uint32_t)SOC_SRAM_DATA_END);

    stack_mem_dump((uint32_t)SOC_SRAM0_DATA_BASE, (uint32_t)SOC_SRAM1_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM1_DATA_BASE, (uint32_t)SOC_SRAM2_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM2_DATA_BASE, (uint32_t)SOC_SRAM3_DATA_BASE);

#if CONFIG_SOC_BK7236XX
    stack_mem_dump((uint32_t)SOC_MEM_CHECK_REG_BASE, (uint32_t)(SOC_MEM_CHECK_REG_BASE + 0x81 * 4));
#endif
}

static void dump_peri_regs(void) {
#if CONFIG_SOC_BK7236XX
    stack_mem_dump((uint32_t)SOC_SYS_REG_BASE, (uint32_t)SOC_SYS_REG_BASE + (0x5c*4));
    stack_mem_dump((uint32_t)SOC_FLASH_REG_BASE, (uint32_t)SOC_FLASH_REG_BASE + (0x20*4));
    stack_mem_dump((uint32_t)SOC_AON_PMU_REG_BASE, (uint32_t)SOC_AON_PMU_REG_BASE + (0x7f*4));
#if (GEN_SECURITY_DEV_UART1_IS_SECURE && !CONFIG_SPE) // if UART1 is used for TFM debug, NSPE cannot access gpio0/1 reg
    stack_mem_dump((uint32_t)SOC_AON_GPIO_REG_BASE+ (0x2*4), (uint32_t)SOC_AON_GPIO_REG_BASE + (0x30*4));
#else
    stack_mem_dump((uint32_t)SOC_AON_GPIO_REG_BASE, (uint32_t)SOC_AON_GPIO_REG_BASE + (0x30*4));
#endif

#if CONFIG_GENERAL_DMA
    stack_mem_dump((uint32_t)SOC_GENER_DMA_REG_BASE, (uint32_t)SOC_GENER_DMA_REG_BASE + (0x44*4));
#if (SOC_DMA_UNIT_NUM > 1)
    stack_mem_dump((uint32_t)SOC_GENER_DMA1_REG_BASE, (uint32_t)SOC_GENER_DMA1_REG_BASE + (0x44*4));
#endif
#endif
#if CONFIG_MAILBOX
    stack_mem_dump((uint32_t)SOC_MBOX0_REG_BASE, (uint32_t)SOC_MBOX0_REG_BASE + (0x38*4));
    stack_mem_dump((uint32_t)SOC_MBOX1_REG_BASE, (uint32_t)SOC_MBOX1_REG_BASE + (0x38*4));
#endif
#if CONFIG_AON_RTC
    stack_mem_dump((uint32_t)SOC_AON_RTC_REG_BASE, (uint32_t)SOC_AON_RTC_REG_BASE + (0x0a*4));
#endif
#if CONFIG_PSRAM
    stack_mem_dump((uint32_t)SOC_PSRAM_REG_BASE, (uint32_t)SOC_PSRAM_REG_BASE + (0x17*4));
#endif
#endif

}

unsigned int arch_is_enter_exception(void) {
    if (is_ap_in_dump_mode()) return 1;
    return 0;
}

unsigned int arch_is_cp_in_dump_mode(void) {
    if (is_cp_in_dump_mode()) return 1;
    return 0;
}

void arch_set_enter_exception(void) {
    if( TRAP_DEFAULT_MAGIC_HEAD != s_dump_spin_lock.magic_head 
        || TRAP_DEFAULT_MAGIC_TAIL != s_dump_spin_lock.magic_tail)
    {
        spinlock_init((spinlock_t *)&s_dump_spin_lock.dump_spin_lock);
        s_dump_spin_lock.magic_head = TRAP_DEFAULT_MAGIC_HEAD;
        s_dump_spin_lock.magic_tail = TRAP_DEFAULT_MAGIC_TAIL;
    }

    spinlock_acquire(&s_dump_spin_lock.dump_spin_lock, 0xffffffff);  // get smp spin lock before get peterson lock.

    /* if core1 dump */
    if (rtos_get_core_id() == CPU1_CORE_ID) {
        stop_cpu2_core();
    } else {
        stop_cpu1_core();
    }

    /* get peterson lock */
    exception_get_peterson_lock(1);
}

void arch_init_exception_magic_status(void) {
    exception_sync_init_early();
    spinlock_init((spinlock_t *)&s_dump_spin_lock.dump_spin_lock);
    s_dump_spin_lock.magic_head = TRAP_DEFAULT_MAGIC_HEAD;
    s_dump_spin_lock.magic_tail = TRAP_DEFAULT_MAGIC_TAIL;
}

void rtos_regist_wifi_dump_hook(hook_func wifi_func)
{
    s_wifi_dump_func = wifi_func;
}

void rtos_regist_ble_dump_hook(hook_func ble_func)
{
    s_ble_dump_func = ble_func;
}

void rtos_regist_plat_dump_hook(uint32_t mem_base_addr, uint32_t mem_size)
{
    if (mem_base_addr >= SOC_SRAM0_DATA_BASE
        && (mem_base_addr + mem_size) < SOC_SRAM_DATA_END) {
        return;
    }

    for (int i = 0; i < s_mem_count; i++) {
        if(mem_base_addr == s_dump_sys_mem_info[i].mem_base_addr
         && mem_size == s_dump_sys_mem_info[i].mem_size) {
            //BK_DUMP_OUT("rtos_regist_plat_dump_hook memory(0x%x) already register.\r\n", mem_base_addr);
            return;
         }
    }

    if (s_mem_count < MAX_DUMP_SYS_MEM_COUNT) {
        s_dump_sys_mem_info[s_mem_count].mem_base_addr = mem_base_addr;
        s_dump_sys_mem_info[s_mem_count].mem_size = mem_size;
        s_mem_count++;
    } else {
        //BK_DUMP_OUT("rtos_regist_plat_dump_hook failed:s_mem_count(%d).\r\n", s_mem_count);
    }
}

void rtos_dump_plat_sys_mems(void) {
#if CONFIG_MEMDUMP_ALL
    dump_peri_regs();	//move before PSRAM, avoid psram not power-on cause it can't dump out

    rtos_dump_plat_memory();

    BK_DUMP_OUT("rtos_dump_plat_sys_mems:s_mem_count(%d).\r\n", s_mem_count);
    for (int i = 0; i < MAX_DUMP_SYS_MEM_COUNT; i++) {
        uint32_t begin = s_dump_sys_mem_info[i].mem_base_addr;
        uint32_t end = begin + s_dump_sys_mem_info[i].mem_size;
        if (data_addr_is_valid(begin, end)) {
            stack_mem_dump(begin, end);
        }
    }
#endif
}


static void dump_prologue(void)
{

    bk_wdt_force_feed();

    s_cpu0_int_0_31_en_value = SOC_SYS_CPU0_INT_0_31_EN_VALUE;
    s_cpu0_int_32_63_en_value = SOC_SYS_CPU0_INT_32_63_EN_VALUE;
    s_cpu1_int_0_31_en_value = SOC_SYS_CPU1_INT_0_31_EN_VALUE;
    s_cpu1_int_32_63_en_value = SOC_SYS_CPU1_INT_32_63_EN_VALUE;
    s_cpu2_int_0_31_en_value = SOC_SYS_CPU2_INT_0_31_EN_VALUE;
    s_cpu2_int_32_63_en_value = SOC_SYS_CPU2_INT_32_63_EN_VALUE;
    SOC_SYS_CPU0_INT_0_31_EN_VALUE = 0;
    SOC_SYS_CPU0_INT_32_63_EN_VALUE = 0;
    SOC_SYS_CPU1_INT_0_31_EN_VALUE = 0;
    SOC_SYS_CPU1_INT_32_63_EN_VALUE = 0;
    SOC_SYS_CPU2_INT_0_31_EN_VALUE = 0;
    SOC_SYS_CPU2_INT_32_63_EN_VALUE = 0;
    dmb_ish();
}

static void dump_epilogue(void)
{
    /* flush log buffer */
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
	BK_LOG_FLUSH();
#endif

}

/**
 * this function will show registers of CPU
 *
 * @param mcause
 * @param context
 */
static void arch_dump_cpu_registers(uint32_t mcause, SAVED_CONTEXT *context) {

    BK_DUMP_OUT("CPU%d Current regs:\r\n", CPU_ID);

    // Context of task.
    BK_DUMP_OUT("0 r0 x 0x%lx\r\n", context->r0);
    BK_DUMP_OUT("1 r1 x 0x%lx\r\n", context->r1);
    BK_DUMP_OUT("2 r2 x 0x%lx\r\n", context->r2);
    BK_DUMP_OUT("3 r3 x 0x%lx\r\n", context->r3);
    BK_DUMP_OUT("4 r4 x 0x%lx\r\n", context->r4);
    BK_DUMP_OUT("5 r5 x 0x%lx\r\n", context->r5);
    BK_DUMP_OUT("6 r6 x 0x%lx\r\n", context->r6);
    BK_DUMP_OUT("7 r7 x 0x%lx\r\n", context->r7);
    BK_DUMP_OUT("8 r8 x 0x%lx\r\n", context->r8);
    BK_DUMP_OUT("9 r9 x 0x%lx\r\n", context->r9);
    BK_DUMP_OUT("10 r10 x 0x%lx\r\n", context->r10);
    BK_DUMP_OUT("11 r11 x 0x%lx\r\n", context->r11);
    BK_DUMP_OUT("12 r12 x 0x%lx\r\n", context->r12);
    BK_DUMP_OUT("14 sp x 0x%lx\r\n", context->sp);
    BK_DUMP_OUT("15 lr x 0x%lx\r\n", context->lr);
    BK_DUMP_OUT("16 pc x 0x%lx\r\n", context->pc);
    BK_DUMP_OUT("17 xpsr x 0x%lx\r\n", context->xpsr);
    BK_DUMP_OUT("18 msp x 0x%lx\r\n", context->msp);
    BK_DUMP_OUT("19 psp x 0x%lx\r\n", context->psp);
    BK_DUMP_OUT("20 primask x 0x%lx\r\n", context->primask);
    BK_DUMP_OUT("21 basepri x 0x%lx\r\n", context->basepri);
    BK_DUMP_OUT("22 faultmask x 0x%lx\r\n", context->faultmask);
    BK_DUMP_OUT("23 fpscr x 0x%lx\r\n", context->fpscr);

    // Context of ISR.
    mcause = __get_xPSR();
    BK_DUMP_OUT("30 CPU%d xPSR x 0x%lx\r\n", CPU_ID, mcause);
    BK_DUMP_OUT("31 LR x 0x%lx\r\n", context->control);       // Exception LR.
    BK_DUMP_OUT("32 control x 0x%lx\r\n", __get_CONTROL());

    BK_DUMP_OUT("40 MMFAR x 0x%lx\r\n", SCB->MMFAR);
    BK_DUMP_OUT("41 BFAR x 0x%lx\r\n", SCB->BFAR);
    BK_DUMP_OUT("42 CFSR x 0x%lx\r\n", SCB->CFSR);
    BK_DUMP_OUT("43 HFSR x 0x%lx\r\n", SCB->HFSR);

	mcause = mcause & 0x1FF;

	if ( (mcause <= 0x0F) && (fault_type[mcause] != NULL) )
	{
		BK_DUMP_OUT((char *)fault_type[mcause]);
	}

}

static void dump_context(uint32_t lr, uint32_t msp)
{
    SAVED_CONTEXT regs;

    regs.r4 = ((uint32_t *)msp)[-8];
    regs.r5 = ((uint32_t *)msp)[-7];
    regs.r6 = ((uint32_t *)msp)[-6];
    regs.r7 = ((uint32_t *)msp)[-5];
    regs.r8 = ((uint32_t *)msp)[-4];
    regs.r9 = ((uint32_t *)msp)[-3];
    regs.r10 = ((uint32_t *)msp)[-2];
    regs.r11 = ((uint32_t *)msp)[-1];
    regs.control = lr;       // Exception LR.

    regs.msp = __get_MSP();
    regs.psp = __get_PSP();

    regs.primask = __get_PRIMASK();
    regs.basepri = __get_BASEPRI();
    regs.faultmask = __get_FAULTMASK();

    regs.fpscr = __get_FPSCR();

	uint32_t stack_pointer = msp;
	uint32_t stack_adj = 8 * sizeof(uint32_t);

	if(lr & (1UL << 2))
	{
		stack_pointer = __get_PSP();
	}

	if((lr & (1UL << 4)) == 0)
	{
		stack_adj += 18 * sizeof(uint32_t);  // 18 FPU registers
	}

	regs.r0 = ((uint32_t *)stack_pointer)[0];
	regs.r1 = ((uint32_t *)stack_pointer)[1];
	regs.r2 = ((uint32_t *)stack_pointer)[2];
	regs.r3 = ((uint32_t *)stack_pointer)[3];
	regs.r12 = ((uint32_t *)stack_pointer)[4];
	regs.lr = ((uint32_t *)stack_pointer)[5];
	regs.pc = ((uint32_t *)stack_pointer)[6];
	regs.xpsr = ((uint32_t *)stack_pointer)[7];

	if(regs.xpsr & (1UL << 9))
	{
		stack_adj += 1 * sizeof(uint32_t);
	}
	regs.sp = stack_pointer + stack_adj;

    arch_dump_cpu_registers(0, &regs);

#if CONFIG_DUMP_INTO_FLASH
    fdump_cpu_registers(0, &regs);
#endif
}

static void rtos_dump_system(void)
{
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    BK_DUMP_OUT("***********************************************************************************************\r\n");
    BK_DUMP_OUT("***********************************user except handler begin***********************************\r\n");
    BK_DUMP_OUT("***********************************************************************************************\r\n");

    bk_wdt_force_feed();

    rtos_dump_plat_sys_mems();

    BK_DUMP_OUT("rtos_dump_system:bk_psram_heap_dump_data.\r\n");
    bk_psram_heap_dump_data();
	
    BK_DUMP_OUT("rtos_dump_system:s_wifi_dump_func.\r\n");
    if(NULL != s_wifi_dump_func && code_addr_is_valid((uint32_t)s_wifi_dump_func)) {
        s_wifi_dump_func();
    }

    BK_DUMP_OUT("rtos_dump_system:s_ble_dump_func.\r\n");
    if(NULL != s_ble_dump_func && code_addr_is_valid((uint32_t)s_ble_dump_func)) {
        s_ble_dump_func();
    }

    BK_DUMP_OUT("rtos_dump_system:os_dump_memory_stats.\r\n");
#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
    os_dump_memory_stats(0, 0, NULL);
#endif

    BK_DUMP_OUT("rtos_dump_system:rtos_dump_backtrace.\r\n");
    rtos_dump_backtrace();
    BK_DUMP_OUT("rtos_dump_system:rtos_dump_task_list.\r\n");
    rtos_dump_task_list();
    BK_DUMP_OUT("rtos_dump_system:rtos_dump_task_runtime_stats.\r\n");
#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif

    BK_DUMP_OUT("***********************************************************************************************\r\n");
    BK_DUMP_OUT("************************************user except handler end************************************\r\n");
    BK_DUMP_OUT("***********************************************************************************************\r\n");
#endif //CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
}

#define CHECK_TASK_WDT_INTERRUPT (0x13)

uint32_t g_wdt_handler_lr = 0;

static void user_except_handler(uint32_t reset_reason, SAVED_CONTEXT *regs)
{

    if (0 == arch_is_enter_exception()) {
        // Make sure the interrupt is disable
        uint32_t int_level = port_disable_interrupts_flag();
        
        /* Handled Trap */
        arch_set_enter_exception();
        dump_prologue();

        /* set reset reson fisrt to avoid the loss of information due to
        abnormalities in the dump process */
        bk_misc_set_reset_reason(reset_reason);
    #if CONFIG_AON_PMU_REG0_REFACTOR_DEV
        aon_pmu_drv_r0_latch_to_r7b();
    #endif

        // If it is a task WDT assert!
        if((regs->xpsr & 0x1FF) == CHECK_TASK_WDT_INTERRUPT)  // It can be used for any interrupts if LR is saved at entrance of ISR.
        {
            if(g_wdt_handler_lr & (1UL << 2))
            {
                uint32_t stack_pointer = __get_PSP();
                uint32_t stack_adj = 8 * sizeof(uint32_t);

                if((g_wdt_handler_lr & (1UL << 4)) == 0)
                {
                    stack_adj += 18 * sizeof(uint32_t);  // 18 FPU registers
                }

                regs->r0 = ((uint32_t *)stack_pointer)[0];
                regs->r1 = ((uint32_t *)stack_pointer)[1];
                regs->r2 = ((uint32_t *)stack_pointer)[2];
                regs->r3 = ((uint32_t *)stack_pointer)[3];
                regs->r12 = ((uint32_t *)stack_pointer)[4];
                regs->lr = ((uint32_t *)stack_pointer)[5];
                regs->pc = ((uint32_t *)stack_pointer)[6];
                regs->xpsr = ((uint32_t *)stack_pointer)[7];

                if(regs->xpsr & (1UL << 9))
                {
                    stack_adj += 1 * sizeof(uint32_t);
                }
                regs->sp = stack_pointer + stack_adj;
            }
        }

        arch_dump_cpu_registers(ECAUSE_ASSERT, regs);

        rtos_dump_system();

        dump_epilogue();

        bk_wdt_force_reboot();

        while(1);

        (void)port_enable_interrupts_flag(int_level);
    } else {
        BK_DUMP_OUT("AP Secondary crash happend !!!!\r\n");
		dump_epilogue();
        bk_wdt_force_reboot();
    }

}

///1. Save to stack is better
///2. Some registers already saved in stack
static void store_cpu_regs(uint32_t mcause, SAVED_CONTEXT *regs) {
    regs->r0 = __get_R0();
    regs->r1 = __get_R1();
    regs->r2 = __get_R2();
    regs->r3 = __get_R3();
    regs->r4 = __get_R4();
    regs->r5 = __get_R5();
    regs->r6 = __get_R6();
    regs->r7 = __get_R7();
    regs->r8 = __get_R8();
    regs->r9 = __get_R9();
    regs->r10 = __get_R10();
    regs->r11 = __get_R11();
    regs->r12 = __get_R12();
    regs->sp = __get_SP();
    regs->lr = __get_LR();
    regs->pc = __get_PC();

    regs->xpsr = __get_xPSR();
    regs->msp = __get_MSP();
    regs->psp = __get_PSP();

    regs->primask = __get_PRIMASK();
    regs->basepri = __get_BASEPRI();
    regs->faultmask = __get_FAULTMASK();

    regs->control = __get_CONTROL();
    regs->fpscr = __get_FPSCR();
}

extern volatile const uint8_t build_version[];

void bk_system_dump(const char *func, const int line)
{
    uint32_t int_level = port_disable_interrupts_flag();
    SAVED_CONTEXT regs = {0};
    store_cpu_regs(ECAUSE_ASSERT, &regs);

#if CONFIG_DUMP_INTO_FLASH
    fdump_cpu_registers(0, &regs);
    fdump_save();
#endif

	BK_DUMP_OUT("(%d)Assert at: %s:%d\r\n", rtos_get_time(), func, line);
    BK_DUMP_OUT("SMP MODE CPU_ID=%d:build time => %s !\r\n", CPU_ID, build_version);
    user_except_handler(RESET_SOURCE_CRASH_ASSERT, &regs);

    port_enable_interrupts_flag(int_level);
}

/*when a cpu hangs,use this variable to judge whether the cpu has entered an exception*/
#if CONFIG_INTERRUPT_DEBUG_RECORDER
__attribute__((__used__)) static volatile uint32_t CPUx_crash_recorder = 0;
#endif
void user_except_handler_ex(uint32_t reset_reason, uint32_t lr, uint32_t sp)
{
    if (0 == arch_is_enter_exception()) {
        // Make sure the interrupt is disable
        uint32_t int_level = port_disable_interrupts_flag();

        /* Handled Trap */
        arch_set_enter_exception();
        dump_prologue();

        /* set reset reson fisrt to avoid the loss of information due to
        abnormalities in the dump process */
        bk_misc_set_reset_reason(reset_reason);
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
        aon_pmu_drv_r0_latch_to_r7b();
#endif

#if CONFIG_INTERRUPT_DEBUG_RECORDER
        CPUx_crash_recorder = sp;
#endif
        BK_DUMP_OUT("AP crash happend, rr: 0x%x\r\n", reset_reason);
        BK_DUMP_OUT("build time => %s !\r\n", build_version); 
        dump_context(lr, sp);

        rtos_dump_system();

#if CONFIG_DUMP_INTO_FLASH
        fdump_save();
#endif

#if CONFIG_CM_BACKTRACE
        cm_backtrace_fault(lr, sp);
#endif


        dump_epilogue();
        bk_wdt_force_reboot();

        (void)port_enable_interrupts_flag(int_level);
    } else {
#if CONFIG_INTERRUPT_DEBUG_RECORDER
        CPUx_crash_recorder = 0;
#endif
        BK_DUMP_OUT("AP Secondary crash happend, rr: 0x%x\r\n", reset_reason);
		dump_epilogue();
        bk_wdt_force_reboot();
        while(1);
    }
}


/* If MPU is enabled, accessing ITCM zero pointer violates the MPU rule, please refer to mpu_cfg
 * If MPU is disabled, null pointer/zero pointer maybe is a software fault. So bk_null_trap_handler
 *    can catch the issue;
 */
__attribute__((used, section(".null_trap_handler"))) \
void bk_null_trap_handler(void)
{
        __asm volatile(
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
        );
        BK_ASSERT(0);
}

// eof

