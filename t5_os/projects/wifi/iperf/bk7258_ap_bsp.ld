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
#include "ram_regions.h"
#include "partitions.h"
#include "sdkconfig.h"

#if (CONFIG_SOC_SMP)
#include "cpu_id.h"
#endif

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (32 << 10);
#if CONFIG_MPU
__MPU_PROTECT_SIZE = 0x20;
#else
__MPU_PROTECT_SIZE = 0x0;
#endif


/*****************************************************************************
The follow four shared memory address area mapping to the same physical memory
    0x08000000 ~ 0x080a0000  ---- instruction area, recommend put sram code
    0x18000000 ~ 0x180a0000  ---- instruction area, recommend put sram code
    0x28000000 ~ 0x280a0000  ---- data area, recommend put sram data
    0x38000000 ~ 0x380a0000  ---- data area, recommend put sram data
******************************************************************************/

__AP_APP_IRAM_OFFSET = 0x20000000;

__AP_APP_IRAM_BASE = CONFIG_AP_RAM_ADDR - __AP_APP_IRAM_OFFSET;  /*0x08000000 = 0x28000000 - __CP_APP_IRAM_OFFSET*/
__AP_APP_IRAM_SIZE = CONFIG_AP_RAM_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET), LENGTH = CONFIG_AP_VIRTUAL_PARTITION_SIZE
  IRAM (rx)             : ORIGIN = __AP_APP_IRAM_BASE, LENGTH = __AP_APP_IRAM_SIZE
#if CONFIG_SPINLOCK_SECTION
  SPINLOCK (rwx)        : ORIGIN = CONFIG_AP_SPINLOCK_ADDR, LENGTH = CONFIG_AP_SPINLOCK_SIZE
#endif

  RAM (rwx)             : ORIGIN = CONFIG_AP_RAM_ADDR, LENGTH = CONFIG_AP_RAM_SIZE
  WIFI_DATA(rwx)        : ORIGIN = CONFIG_AP_WIFI_DATA_ADDR, LENGTH = CONFIG_AP_WIFI_DATA_SIZE

  SWAP (rwx)            : ORIGIN = CONFIG_SWAP_ADDR, LENGTH = CONFIG_SWAP_SIZE
#if (CONFIG_BT_REUSE_MEDIA_MEMORY)
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

#if CONFIG_PSRAM
  PSRAM_SLAB_USER (rwx)        : ORIGIN = CONFIG_PSRAM_MEM_SLAB_USER_ADDR, LENGTH = CONFIG_PSRAM_MEM_SLAB_USER_SIZE
  PSRAM_SLAB_AUDIO (rwx)       : ORIGIN = CONFIG_PSRAM_MEM_SLAB_AUDIO_ADDR, LENGTH = CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE
  PSRAM_SLAB_ENCODE (rwx)      : ORIGIN = CONFIG_PSRAM_MEM_SLAB_ENCODE_ADDR, LENGTH = CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE
  PSRAM_SLAB_DISPLAY (rwx)     : ORIGIN = CONFIG_PSRAM_MEM_SLAB_DISPLAY_ADDR, LENGTH = CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
  PSRAM_HEAP (rwx)      : ORIGIN = CONFIG_AP_PSRAM_HEAP_ADDR, LENGTH = CONFIG_AP_PSRAM_HEAP_SIZE
#endif
  PSRAM_SECTION (rwx)   : ORIGIN = CONFIG_AP_PSRAM_SECTION_ADDR, LENGTH = CONFIG_AP_PSRAM_SECTION_SIZE
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
#if 0
        EXCLUDE_FILE
            (
            *wdrv_main.c.obj *wdrv_tx.c.obj *wdrv_rx.c.obj *wdrv_co_list.c.obj  
            *tasks.c.obj *queue.c.obj *list.c.obj *timers.c.obj *port.c.obj *heap_4.c.obj *mem_arch.c.obj *arch_interrupt.c.obj  *rtos_pub_smp.c.obj
            *sys_arch.c.obj *sys.c.obj *wlanif.c.obj *udp.c.obj *tcpip.c.obj *tcp_out.c.obj *tcp_in.c.obj *tcp.c.obj *sockets.c.obj 
            *mem.c.obj *memp.c.obj *ethernet.c.obj *api_msg.c.obj *pbuf.c.obj
            *mailbox_channel.c.obj *mbox0_adapter.c.obj  *mbox0_drv.c.obj
            *iperf.c.obj
            )
        *(.text.* *.rodata *.rodata.*)
#else
        EXCLUDE_FILE
            (
            *wdrv_main.c.obj *wdrv_tx.c.obj *wdrv_rx.c.obj *wdrv_co_list.c.obj 
            *mailbox_channel.c.obj *mbox0_adapter.c.obj  *mbox0_drv.c.obj
            )
        *(.text.* *.rodata *.rodata.*)
#endif
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
        #if 1
        *libiperf.a:*(.text*)
        //*liblwip_intf_v2_1.a:*(.text*)
        //*libbk_wifi_driver.a:*(.text*)
        *libbk_wifi.a:*(.text*)
        #endif
        __wifi_dp_code_end = .;

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

        LONG (__psram_text)
        LONG (__psram_data_start__)
        LONG ((__psram_data_end__ - __psram_data_start__) / 4)

#if ((!defined(CONFIG_SOC_SMP)) || (0 == CONFIG_SOC_SMP))
        LONG (__itcm_text)
        LONG (__itcm_start__)
        LONG ((__itcm_end__ - __itcm_start__) / 4)

        LONG (__dtcm_content)
        LONG (__dtcm_start__)
        LONG ((__dtcm_end__ - __dtcm_start__) / 4)
#endif

#if (CONFIG_SPINLOCK_SECTION && CONFIG_SOC_SMP)
        LONG (__spinlock_begin)
        LONG (_spinlock_start)
        LONG ((_spinlock_end - _spinlock_start) / 4)
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
        LONG (__psram_bss_start__)
        LONG ((__psram_bss_end__ - __psram_bss_start__) / 4)
        __zero_table_end__ = .;
    } > FLASH

#if (CONFIG_BT_REUSE_MEDIA_MEMORY)
    .hardware_acc (NOLOAD):
    {
        . = ALIGN(32);
        _bt_data_start = .;
        KEEP(*(.bt_spec_data ))
        KEEP(*(.ble_bss_data ))
        . = ALIGN(4);
        _bt_data_end = .;
    } > HARDWARE_ACC AT > FLASH
#endif

    .wifi.data (NOLOAD) :
    {
        . = ALIGN(8);
        _ram_heap_start = .;

        *(.ram_heap)
        *(.ram_heap*)
        *(.bss.ram_heap)
        *(.bss.ram_heap*)
        *mem.c.obj(.bss .bss.* .bss*)
        . = ALIGN(8);
        _ram_heap_end = .;
    } > WIFI_DATA

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
        LONG (__itcm_cpu1_text)
        LONG (__itcm_cpu1_start__)
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


        *(.itcm)
        *(.itcm_section*)
        *(.itcm_sec_code*)
        *(.interrupt)
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
#if 0//CONFIG_SPE
        *wdrv_main.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_tx.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_rx.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_co_list.c.obj(.text.* *.rodata *.rodata.*)

        *tasks.c.obj(.text.* *.rodata *.rodata.*)
        *queue.c.obj(.text.* *.rodata *.rodata.*)
        *list.c.obj(.text.* *.rodata *.rodata.*)
        *timers.c.obj(.text.* *.rodata *.rodata.*)
        *port.c.obj(.text.* *.rodata *.rodata.*)
        *heap_4.c.obj(.text.* *.rodata *.rodata.*)
        *mem_arch.c.obj(.text.* *.rodata *.rodata.*) 
        *arch_interrupt.c.obj(.text.* *.rodata *.rodata.*)
        *rtos_pub_smp.c.obj(.text.* *.rodata *.rodata.*)

        *sys_arch.c.obj(.text.* *.rodata *.rodata.*) 
        *sys.c.obj(.text.* *.rodata *.rodata.*) 
        *wlanif.c.obj(.text.* *.rodata *.rodata.*) 
        *udp.c.obj(.text.* *.rodata *.rodata.*) 
        *tcpip.c.obj(.text.* *.rodata *.rodata.*) 
        *tcp_out.c.obj(.text.* *.rodata *.rodata.*) 
        *tcp_in.c.obj(.text.* *.rodata *.rodata.*) 
        *tcp.c.obj(.text.* *.rodata *.rodata.*) 
        *sockets.c.obj(.text.* *.rodata *.rodata.*) 
        *mem.c.obj(.text.* *.rodata *.rodata.*) 
        *memp.c.obj(.text.* *.rodata *.rodata.*) 
        *ethernet.c.obj(.text.* *.rodata *.rodata.*) 
        *api_msg.c.obj(.text.* *.rodata *.rodata.*)
        *pbuf.c.obj(.text.* *.rodata *.rodata.*)

        *mailbox_channel.c.obj(.text.* *.rodata *.rodata.*) 
        *mbox0_adapter.c.obj(.text.* *.rodata *.rodata.*) 
        *mbox0_drv.c.obj(.text.* *.rodata *.rodata.*)

        *iperf.c.obj(.text.* *.rodata *.rodata.*)
#else
        *wdrv_tx.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_main.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_rx.c.obj(.text.* *.rodata *.rodata.*) 
        *wdrv_co_list.c.obj(.text.* *.rodata *.rodata.*)

        *mailbox_channel.c.obj(.text.* *.rodata *.rodata.*) 
        *mbox0_adapter.c.obj(.text.* *.rodata *.rodata.*) 
        *mbox0_drv.c.obj(.text.* *.rodata *.rodata.*)
#endif
        __iram_end__ = .;

    } > IRAM AT > FLASH

#if (CONFIG_SPINLOCK_SECTION && CONFIG_SOC_SMP)
    .spinlock :
    {
        . = ALIGN(4);
        PROVIDE(__spinlock_begin = LOADADDR(.spinlock));
        _spinlock_start = .;
        *(.sram_spinlock_section)
        . = ALIGN(4);
         _spinlock_end = .;
    } > SPINLOCK AT > FLASH
#endif

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
        KEEP(*(.bss_last))
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    bss_size = _bss_end - _bss_start;

#if (CONFIG_SOC_SMP)
	_msp_total_size = __MSP_STACK_SIZE * 2;
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
    /* .stack_cpu2 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2 - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu2 = .;
        __StackLimitCpu2 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu2 = .;
        _estack_cpu2 = .;
    } > RAM
    */

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




#if CONFIG_PSRAM
    .psram.data :
    {
        . = ALIGN(4);
        PROVIDE(__psram_text = LOADADDR(.psram.data));
        __psram_data_start__ = .;

        *(.psram.data)
        *(.psram.data*)

        . = ALIGN(4);
        __psram_data_end__ = .;
    } > PSRAM_SECTION AT > FLASH

    .psram.bss (NOLOAD):
    {
        . = ALIGN(4);
        __psram_bss_start__ = .;
        *(.psram.bss)
        *(.psram.bss*)
        . = ALIGN(4);
        __psram_bss_end__ = .;
    } > PSRAM_SECTION
#endif

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 32K!!!")
}
//eof

