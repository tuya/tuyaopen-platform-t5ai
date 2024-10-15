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

#include "mem_layout.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

MEMORY
{
  FLASH (rx)            : ORIGIN = CONFIG_ROM_ADDR, LENGTH = CONFIG_ROM_SIZE
  RAM (rwx)             : ORIGIN = CONFIG_RAM_ADDR, LENGTH = CONFIG_RAM_SIZE
}

ENTRY(Reset_Handler)

SECTIONS
{
    ASSERT((. == ALIGN(128)), "vector table address align fault.")
    .vectors :
    {
        __vector_table = .;
        KEEP(*(.vectors))
        . = ALIGN(32);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(".text.*")

        . = ALIGN(4);
        *(.rodata*)
        *(".rodata.*")

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

    .data :
    {
        . = ALIGN(4);
        PROVIDE(__etext = LOADADDR(.data));
        __data_start__ = .;
        KEEP(*(.iram))
        KEEP(*(".iram*"))
        
        *(.data)
        *(".data.*")
        
        . = ALIGN(4);
        __device_init_start = .;
        KEEP(*(SORT_BY_NAME(.init_level_[0-9]_[0-9])));
        KEEP(*(SORT_BY_NAME(.init_level_[0-9]_[1-9][0-9])));
        . = ALIGN(4);
        __device_init_end = .;

        __commands_table_start = .;
        KEEP(*(SORT_BY_NAME(.commands)));
        KEEP(*(SORT_BY_NAME(.commands.*)));
        . = ALIGN(4);
        __commands_table_end = .;

        /* All data end */
        __data_end__ = .;

    } > RAM AT > FLASH

    __data_size__ = (__data_end__ - __data_start__) / 4;

    /* set the global rom end */
    g_rom_end = LOADADDR(.data) + SIZEOF(.data);

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
        __START_BSS = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        __END_BSS = .;

    } > RAM

    __bss_size__ = (__bss_end__ - __bss_start__) / 4;

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - CONFIG_MSP_STACK_SIZE;
    PROVIDE(__stack = __MSPTop);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__MSPLimit >= __bss_end__, "region RAM overflowed with stack")
    ASSERT((g_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT((__bss_end__  < ORIGIN(RAM)  + LENGTH(RAM)), "RAM overflow!!!")

}   /* End of sections */
