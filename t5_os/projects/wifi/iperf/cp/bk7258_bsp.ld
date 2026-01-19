/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "soc/bk7258/reg_base.h"
#include "partitions.h"
#include "sdkconfig.h"
#include "ram_regions.h"

#if (CONFIG_SOC_SMP)
#include "cpu_id.h"
#endif

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (64 << 10);
#if CONFIG_MPU
__MPU_PROTECT_SIZE = 0x20;
#else
__MPU_PROTECT_SIZE = 0x0;
#endif

__SWAP_SIZE = (2048);


/*****************************************************************************
The follow four shared memory address area mapping to the same physical memory
    0x08000000 ~ 0x080a0000  ---- instruction area, recommend put sram code
    0x18000000 ~ 0x180a0000  ---- instruction area, recommend put sram code
    0x28000000 ~ 0x280a0000  ---- data area, recommend put sram data
    0x38000000 ~ 0x380a0000  ---- data area, recommend put sram data
******************************************************************************/

__CP_APP_IRAM_OFFSET = 0x20000000;

__CP_APP_IRAM_BASE = CONFIG_CP_RAM_ADDR - __CP_APP_IRAM_OFFSET;  /*0x08000000 = 0x28000000 - __CP_APP_IRAM_OFFSET*/
__CP_APP_IRAM_SIZE = CONFIG_CP_RAM_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + CONFIG_CP_VIRTUAL_PARTITION_OFFSET), LENGTH = CONFIG_CP_VIRTUAL_PARTITION_SIZE
  IRAM (rx)             : ORIGIN = __CP_APP_IRAM_BASE, LENGTH = __CP_APP_IRAM_SIZE
  RAM (rwx)             : ORIGIN = CONFIG_CP_RAM_ADDR, LENGTH = CONFIG_CP_RAM_SIZE
  SWAP (rwx)            : ORIGIN = CONFIG_SWAP_ADDR, LENGTH = CONFIG_SWAP_SIZE

#if CONFIG_BT_REUSE_MEDIA_MEMORY
  HARDWARE_ACC (rwx)         : ORIGIN = CONFIG_HARDWARE_ACC_ADDR, LENGTH = CONFIG_HARDWARE_ACC_SIZE
#endif

#if (CONFIG_SOC_SMP)
  ITCM0 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM0 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE

  ITCM1 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM1 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE

  ITCM2 (rwx)           : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM2 (rwx)           : ORIGIN = SOC_DTCM_DATA_BASE + CPU_ID_SPACE_SIZE, LENGTH = CONFIG_DTCM_SIZE - CPU_ID_SPACE_SIZE
#else
  ITCM (rwx)            : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM (rwx)            : ORIGIN = SOC_DTCM_DATA_BASE + 0, LENGTH = CONFIG_DTCM_SIZE
#endif
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
  PSRAM_HEAP (rwx)      : ORIGIN = CONFIG_CP_PSRAM_HEAP_ADDR, LENGTH = CONFIG_CP_PSRAM_HEAP_SIZE
#endif
}

#if (CONFIG_SOC_SMP)
ENTRY(Reset_Handler_Cpu0)
#else
ENTRY(Reset_Handler)
#endif

SECTIONS
{
    ASSERT((. == ALIGN(512)), "vector table address align fault.")
    .vectors :
    {
#if (CONFIG_SOC_SMP)
        __vector_core0_table = .;
        KEEP(*(.vectors_core0))
        . = ALIGN(512);

        __vector_core1_table = .;
        KEEP(*(.vectors_core1))
        . = ALIGN(512);

        __vector_core2_table = .;
        KEEP(*(.vectors_core2))
        . = ALIGN(512);
#else
        __vector_table = .;
        KEEP(*(.vectors))
        . = ALIGN(512);
#endif
    } > FLASH


    .gnu.sgstubs ALIGN(32) : ALIGN(32)
    {
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        _stext = .;

        . = ALIGN(4);
        __devconfig_start = .;
        *(".devconfig.*")
        KEEP(*(SORT_BY_NAME(".devconfig*")))
        __devconfig_end = .;

        . = ALIGN(4);
        __apps_start = .;
        KEEP (*(.apps_data))
        __apps_end = .;

        . = ALIGN(4);
        __wifi_dp_code_start = .;
        *libcontroller_if.a:*(.text*)
        *libbk_wifi.a:*(.text*)
        *libwifi.a:*(.text*)
        *liblwip_intf_v2_1.a:*(.text*)
        __wifi_dp_code_end = .;

	EXCLUDE_FILE
	(
	*tasks.c.obj *queue.c.obj *list.c.obj *timers.c.obj *port.c.obj *rtos_pub.c.obj *heap_4.c.obj *mem_arch.c.obj *gpio_driver_base.c.obj *flash_driver.c.obj *ckmn_driver.c.obj *arch_interrupt.c.obj *hal_machw.c.obj *interrupts.c.obj 
	*rw_task.c.obj  *me_strategy.c.obj *chan.c.obj *vif_mgmt.c.obj *rxl_cntrl.c.obj *rxl_hwdesc.c.obj *mm_timer.c.obj *ke_event.c.obj *rwnx.c.obj *rwnx_rx.c.obj *ps.c.obj *mm.c.obj *rf_cntrl.c.obj *mcc.c.obj 
	*phy_karst_bk7236.c.obj *crm.c.obj *bk7236_cal.c.obj *manual_cal_bk7236.c.obj *bk_cal_common.c.obj *nv_parameters.c.obj *macif_fhost.c.obj *rc_driver.c.obj *sys_wifi_driver.c.obj *wifi_v2.c.obj *bk_feature.c.obj 
	*pm.c.obj *sys_ps_driver.c.obj *sys_pm_hal.c.obj *bk_wifi_adapter.c.obj *net_param.c.obj *wifi_api_wrapper.c.obj *bk_rf_adapter.c.obj *bk_phy_adapter.c.obj
	*shell_task.c.obj *shell_uart.c.obj *uart_driver.c.obj *uart_hal.c.obj
	)
        _etext = .;
        . = ALIGN(4);
    } > FLASH

    .a_device_null :
    {
      KEEP(*(.a_deviceobj_null))
    } > FLASH

    .a_devices :
    {
      __device_start = .;
      KEEP(*(.a_deviceobj_*))
      __device_end = .;
    } > FLASH

    .a_init_entries :
    {
      __a_init_start = .;
      KEEP(*(.a_init_entry_*))
      __a_init_end = .;
    } > FLASH

    .avdk_ipc :
    {
      __start_ipc_chan_reg = .;
      KEEP(*(.ipc_chan_reg*))
      __stop_ipc_chan_reg = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__iram_flash_begin)
        LONG (__iram_start__)
        LONG ((__iram_end__ - __iram_start__) / 4)

        LONG (__data_flash_begin)
        LONG (__data_start__)
        LONG ((__data_end__ - __data_start__) / 4)

#if ((!defined(CONFIG_SOC_SMP)) || (0 == CONFIG_SOC_SMP))
        LONG (__itcm_text)
        LONG (__itcm_start__)
        LONG ((__itcm_end__ - __itcm_start__) / 4)

        LONG (__dtcm_content)
        LONG (__dtcm_start__)
        LONG ((__dtcm_end__ - __dtcm_start__) / 4)
#endif
        __copy_table_end__ = .;
    } > FLASH

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (_bss_start)
        LONG ((_bss_end - _bss_start) / 4)
        LONG (_heap_start)
        LONG ((_heap_end - _heap_start) / 4)
        /* Add each additional bss section here */
        LONG (_bt_data_start)
        LONG ((_bt_data_end - _bt_data_start) / 4)
        __zero_table_end__ = .;
    } > FLASH


#if CONFIG_BT_REUSE_MEDIA_MEMORY
    .hardware_acc (NOLOAD):
    {
        . = ALIGN(32);
        _bt_data_start = .;
        KEEP(*(.bt_spec_data ))
        KEEP(*(.ble_bss_data ))

        *rwip.c.obj(*.bss *.bss.*)
        *rwip_driver.c.obj(*.bss *.bss.*)
        *uart_ble.c.obj(*.bss *.bss.*)
        *hci_packet_parser.c.obj(*.bss *.bss.*)
        *sdp_common.c.obj(*.bss *.bss.*)
        *app_ble.c.obj(*.bss *.bss.*)
        *prf.c.obj(*.bss *.bss.*)
        *gapm.c.obj(*.bss *.bss.*)
        *ble_util_buf.c.obj(*.bss *.bss.*)
        *sch_prog.c.obj(*.bss *.bss.*)
        *sch_slice.c.obj(*.bss *.bss.*)
        *llm.c.obj(*.bss *.bss.*)
        . = ALIGN(4);
        _bt_data_end = .;
    } > HARDWARE_ACC AT > FLASH
#endif

#if (CONFIG_SOC_SMP)
    .tcm.copy.table :
    {
        . = ALIGN(4);
        __tcm_copy_table_start__ = .;
        LONG (tcm_cpu0_core_id)
        LONG (__itcm_cpu0_text)
        LONG (__itcm_cpu0_start__)
        LONG ((__itcm_cpu0_end__ - __itcm_cpu0_start__) / 4)

        LONG (tcm_cpu0_core_id)
        LONG (__dtcm_cpu0_content)
        LONG (__dtcm_cpu0_start__)
        LONG ((__dtcm_cpu0_end__ - __dtcm_cpu0_start__) / 4)

        LONG (tcm_cpu1_core_id)
        LONG (__itcm_cpu1_text)
        LONG (__itcm_cpu1_start__)
        LONG ((__itcm_cpu1_end__ - __itcm_cpu1_start__) / 4)

        LONG (tcm_cpu1_core_id)
        LONG (__dtcm_cpu1_content)
        LONG (__dtcm_cpu1_start__)
        LONG ((__dtcm_cpu1_end__ - __dtcm_cpu1_start__) / 4)

        LONG (tcm_cpu2_core_id)
        LONG (__itcm_cpu2_text)
        LONG (__itcm_cpu2_start__)
        LONG ((__itcm_cpu2_end__ - __itcm_cpu2_start__) / 4)

        LONG (tcm_cpu2_core_id)
        LONG (__dtcm_cpu2_content)
        LONG (__dtcm_cpu2_start__)
        LONG ((__dtcm_cpu2_end__ - __dtcm_cpu2_start__) / 4)
        __tcm_copy_table_end__ = .;
    } > FLASH

    tcm_cpu0_core_id = CPU0_CORE_ID;
    .itcm_cpu0 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu0_text = LOADADDR(.itcm_cpu0));
        __itcm_cpu0_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu0)
        . = ALIGN(4);

        __itcm_cpu0_end__ = .;
    } > ITCM0 AT > FLASH

    .dtcm_cpu0 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu0_content = LOADADDR(.dtcm_cpu0));
        __dtcm_cpu0_start__ = .;
        *(.dtcm_cpu0)
        . = ALIGN(4);
        __dtcm_cpu0_end__ = .;
    } > DTCM0 AT > FLASH

    tcm_cpu1_core_id = CPU1_CORE_ID;
    .itcm_cpu1 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu1_text = LOADADDR(.itcm_cpu1));
        __itcm_cpu1_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu1)
        . = ALIGN(4);

        __itcm_cpu1_end__ = .;
    } > ITCM1 AT > FLASH

    .dtcm_cpu1 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu1_content = LOADADDR(.dtcm_cpu1));
        __dtcm_cpu1_start__ = .;
        *(.dtcm_cpu1)
        . = ALIGN(4);
        __dtcm_cpu1_end__ = .;
    } > DTCM1 AT > FLASH

    tcm_cpu2_core_id = CPU2_CORE_ID;
    .itcm_cpu2 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu2_text = LOADADDR(.itcm_cpu2));
        __itcm_cpu2_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu2)
        . = ALIGN(4);

        __itcm_cpu2_end__ = .;
    } > ITCM2 AT > FLASH

    .dtcm_cpu2 :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_cpu2_content = LOADADDR(.dtcm_cpu2));
        __dtcm_cpu2_start__ = .;
        *(.dtcm_cpu2)
        . = ALIGN(4);
        __dtcm_cpu2_end__ = .;
    } > DTCM2 AT > FLASH
#else

    .itcm :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_text = LOADADDR(.itcm));
        __itcm_start__ = .;

#if CONFIG_MPU
        /*mpu_region about itcm will catch null pointer, for zero address is out of mpu setting*/
#else
        KEEP(*(.null_trap_handler))
#endif

        *(.itcm_section*)
        *(.itcm_sec_code*)
        . = ALIGN(4);

        __itcm_end__ = .;
    } > ITCM AT > FLASH

    .dtcm :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_content = LOADADDR(.dtcm));
        __dtcm_start__ = .;
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        . = ALIGN(4);
        __dtcm_end__ = .;
    } > DTCM AT > FLASH
#endif

    .iram :
    {
        . = ALIGN(512);
        PROVIDE(__iram_flash_begin = LOADADDR(.iram));
        __iram_start__ = .;

        . = ALIGN(512);
        __vector_iram_table = .;
        KEEP(*(.vectors_iram))
        . = ALIGN(512);

        *(.itcm)
        *(.iram)
        *(.interrupt)

        . = ALIGN(4);

#if CONFIG_SPE
        *tasks.c.obj(.text.* *.rodata *.rodata.*)
        *queue.c.obj(.text.* *.rodata *.rodata.*)
        *list.c.obj(.text.* *.rodata *.rodata.*)
        *timers.c.obj(.text.* *.rodata *.rodata.*)
        *port.c.obj(.text.* *.rodata *.rodata.*)
        *rtos_pub.c.obj(.text.* *.rodata *.rodata.*)
        *heap_4.c.obj(.text.* *.rodata *.rodata.*)
        *mem_arch.c.obj(.text.* *.rodata *.rodata.*)
        *gpio_driver_base.c.obj(.text.* *.rodata *.rodata.*)
        *flash_driver.c.obj(.text.* *.rodata *.rodata.*)
        *ckmn_driver.c.obj(.text.* *.rodata *.rodata.*)
        *arch_interrupt.c.obj(.text.* *.rodata *.rodata.*)
        *hal_machw.c.obj(.text.* *.rodata *.rodata.*)
        *interrupts.c.obj(.text.* *.rodata *.rodata.*)
        *rw_task.c.obj(.text.* *.rodata *.rodata.*)
        *ke_event.c.obj(.text.* *.rodata *.rodata.*)
        *me_strategy.c.obj(.text.* *.rodata *.rodata.*)
        *chan.c.obj(.text.* *.rodata *.rodata.*)
        *vif_mgmt.c.obj(.text.* *.rodata *.rodata.*)
        *rxl_cntrl.c.obj(.text.* *.rodata *.rodata.*)
        *rxl_hwdesc.c.obj(.text.* *.rodata *.rodata.*)
        *mm_time.c.obj(.text.* *.rodata *.rodata.*)
        *rwnx.c.obj(.text.* *.rodata *.rodata.*)
        *rwnx_rx.c.obj(.text.* *.rodata *.rodata.*)
        *ps.c.obj(.text.* *.rodata *.rodata.*)
        *mm.c.obj(.text.* *.rodata *.rodata.*)
        *rf_cntrl.c.obj(.text.* *.rodata *.rodata.*)
        *mcc.c.obj(.text.* *.rodata *.rodata.*)
        *phy_karst_bk7236.c.obj(.text.* *.rodata *.rodata.*)
        *crm.c.obj (.text.* *.rodata *.rodata.*)
        *bk7236_cal.c.obj(.text.* *.rodata *.rodata.*)
        *manual_cal_bk7236.c.obj(.text.* *.rodata *.rodata.*)
        *bk_cal_common.c.obj(.text.* *.rodata *.rodata.*)
        *nv_parameters.c.obj(.text.* *.rodata *.rodata.*)
        *macif_fhost.c.obj(.text.* *.rodata *.rodata.*)
        *rc_driver.c.obj(.text.* *.rodata *.rodata.*)
        *sys_wifi_driver.c.obj(.text.* *.rodata *.rodata.*)
        *wifi_v2.c.obj(.text.* *.rodata *.rodata.*)
        *bk_feature.c.obj(.text.* *.rodata *.rodata.*)
        *pm.c.obj(.text.* *.rodata *.rodata.*)
        *sys_ps_driver.c.obj(.text.* *.rodata *.rodata.*)
        *sys_pm_hal.c.obj(.text.* *.rodata *.rodata.*)
        *bk_wifi_adapter.c.obj(.text.* *.rodata *.rodata.*)
        *net_param.c.obj(.text.* *.rodata *.rodata.*)
        *wifi_api_wrapper.c.obj(.text.* *.rodata *.rodata.*)
        *bk_rf_adapter.c.obj(.text.* *.rodata *.rodata.*)
        *bk_phy_adapter.c.obj(.text.* *.rodata *.rodata.*)
        *shell_task.c.obj(.text.* *.rodata *.rodata.*)
        *shell_uart.c.obj(.text.* *.rodata *.rodata.*)
        *uart_driver.c.obj(.text.* *.rodata *.rodata.*)
        *uart_hal.c.obj(.text.* *.rodata *.rodata.*)
#endif
        __iram_end__ = .;

    } > IRAM AT > FLASH

    .ramcode :
    {
        . = . +  SIZEOF(.iram);
        . = ALIGN(512);
    } > RAM AT > FLASH


    .data :
    {
        PROVIDE(__etext = LOADADDR(.data));
        PROVIDE(__data_flash_begin = LOADADDR(.data));

        . = ALIGN(4);
        __data_start__ = .;

#ifdef CONFIG_GCOV
        /* added in template for gcov: */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
#endif

#if (CONFIG_SOC_SMP)
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
#endif
        *(.data)
        *(".data.*")
        *(.sdata)

        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    _data_flash_begin = __data_flash_begin;
    _data_ram_begin = __data_start__;
    _data_ram_end = .;

    s_rom_end = LOADADDR(.data) + SIZEOF(.data);

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        *(".uninitialized.*")
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)

#if (!CONFIG_BT_REUSE_MEDIA_MEMORY)
        . = ALIGN(32);
        _bt_data_start = .;
        KEEP(*(.bt_spec_data ))
        KEEP(*(.ble_bss_data ))

        *rwip.c.obj(*.bss *.bss.*)
        *rwip_driver.c.obj(*.bss *.bss.*)
        *uart_ble.c.obj(*.bss *.bss.*)
        *hci_packet_parser.c.obj(*.bss *.bss.*)
        *sdp_common.c.obj(*.bss *.bss.*)
        *app_ble.c.obj(*.bss *.bss.*)
        *prf.c.obj(*.bss *.bss.*)
        *gapm.c.obj(*.bss *.bss.*)
        *ble_util_buf.c.obj(*.bss *.bss.*)
        *sch_prog.c.obj(*.bss *.bss.*)
        *sch_slice.c.obj(*.bss *.bss.*)
        *llm.c.obj(*.bss *.bss.*)

        _bt_data_end = .;
#endif

        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    bss_size = _bss_end - _bss_start;

#if (CONFIG_SOC_SMP)
	_msp_total_size = __MSP_STACK_SIZE * CONFIG_CPU_CNT;
#else
	_msp_total_size = __MSP_STACK_SIZE;
#endif

    .heap (COPY) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + (ORIGIN(RAM) + LENGTH(RAM) - _msp_total_size - _heap_start - 8); /* 16 bytes for boundary protection */
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

/*multiple cores require independent stack spaces*/
#if (CONFIG_SOC_SMP)
    /* ASSERT((CONFIG_CPU_CNT == 3), "cpu count exceptional!!!") */
    .stack_cpu2 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2 - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu2 = .;
        __StackLimitCpu2 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu2 = .;
        _estack_cpu2 = .;
    } > RAM

    .stack_cpu1 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu1 = .;
        __StackLimitCpu1 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu1 = .;
        _estack_cpu1 = .;
    } > RAM

    .stack_cpu0 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu0 = .;
        __StackLimitCpu0 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu0 = .;
        _estack_cpu0 = .;
    } > RAM
#else
    .stack (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTop = .;
        _estack = .;
    } > RAM
#endif

    .swap ORIGIN(SWAP):
    {
        . = ALIGN(8);
        _swap_start = .;

        * (.swap_data)
        * (.swap_data*)
        . = ALIGN(4);
        _swap_end = .;
    } > SWAP AT > FLASH

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 64K!!!")
}
//eof

