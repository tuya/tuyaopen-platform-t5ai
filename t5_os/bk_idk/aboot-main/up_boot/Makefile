#
# Copyright (c) 2012-2021 Andes Technology Corporation
# All rights reserved.
#
PROJECT_BUILD_DIR   = ./out
PROJECT_BUILD_DIR_TEMP   = $(PROJECT_BUILD_DIR)/temp
PROG	?= $(PROJECT_BUILD_DIR)/u_bootloader
PROJECT_BUILD_OUT_DIR = ../PACK/bootloader_u/u_bootloader

CROSS_COMPILE = /opt/risc-v/nds32le-elf-mculib-v5/bin/riscv32-elf-

ifndef CROSS_COMPILE
	ifneq (,$(filter $(COMPILER),riscv32 riscv32-llvm))
		CROSS_COMPILE = riscv32-elf-
	endif
endif
ifndef CROSS_COMPILE
	ifneq (,$(filter $(COMPILER),riscv64 riscv64-llvm))
		CROSS_COMPILE = riscv64-elf-
	endif
endif
ifndef CROSS_COMPILE
$(error You have to specify 'COMPILER' to build ! Ex: make COMPILER=[riscv32|riscv64|riscv32-llvm|riscv64-llvm])
endif

ifneq (,$(filter $(COMPILER),riscv32-llvm riscv64-llvm))
CC	:= $(CROSS_COMPILE)clang
C++	:= $(CROSS_COMPILE)clang++
else
CC	:= $(CROSS_COMPILE)gcc
C++     := $(CROSS_COMPILE)g++
endif
OBJDUMP	:= $(CROSS_COMPILE)objdump
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
AR	:= $(CROSS_COMPILE)ar
AS	:= $(CROSS_COMPILE)as

OPTIM   := -Os


# Platform: AE250, AE350, CF1-AE250
PLAT	?= AE250

# Mode: LOAD, BURN, XIP
MODE	?= LOAD

ifneq ($(filter $(PLAT), AE250 AE350 CF1-AE250),$(PLAT))
$(error Unknown PLAT "$(PLAT)" is not supported!)
endif

ifeq (AE350, $(PLAT))
ifneq ($(USE_CACHE), 1)
$(error Please specify "USE_CACHE=1" for AE350 platform!)
endif
endif

# Define BOARD
ifeq ($(PLAT),CF1-AE250)
	BOARD	:= CF1
else
	BOARD	:= ORCA
endif

PLATNAME = $(shell echo $(patsubst CF1-%,%,$(PLAT)) | tr A-Z a-z)

# Define the directories
APP_SOURCE_DIR		= ./applications
COMPONENT_DIR       = ./components
PACKAGES_DIR        = ./packages
SYSTEM_SOURCE_DIR	= ./system
RISCV_SOURCE_DIR	= ../platform/riscv
BSP_SOURCE_DIR		= ../platform/drivers
DIFF2YA_SOURCE_DIR		= ../platform/diff2ya
U_BOOTLOADER_LIB_NAME   = lib_ubl.a
U_BOOTLOADER_ZLIB_NAME   = libz.a
U_BOOTLOADER_AES_NAME   = libaes.a

# Define the source files we have
# BSP source files
#STARTUP_SRCS = $(RISCV_SOURCE_DIR)/boot_vectors_base.S
STARTUP_SRCS = $(RISCV_SOURCE_DIR)/start.S

BSP_SRCS = \
	$(BSP_SOURCE_DIR)/board.c \
	$(BSP_SOURCE_DIR)/bk_timer.c \
	$(BSP_SOURCE_DIR)/driver_gpio.c \
	$(BSP_SOURCE_DIR)/driver_uart0.c \
	$(BSP_SOURCE_DIR)/driver_uart1.c \
	$(BSP_SOURCE_DIR)/driver_uart2.c \
	$(BSP_SOURCE_DIR)/drv_uart.c \
	$(BSP_SOURCE_DIR)/driver_flash.c \
	$(BSP_SOURCE_DIR)/drv_timer.c \
	$(BSP_SOURCE_DIR)/driver_system.c \
	$(BSP_SOURCE_DIR)/wdt.c \
	$(SYSTEM_SOURCE_DIR)/BK_System.c


	#$(BSP_SOURCE_DIR)/driver_sys_ctrl.c \
	

ifeq ($(MODE), BURN)
	BSP_SRCS += $(BSP_SOURCE_DIR)/$(PLATNAME)/loader.c
endif

# App source files
APP_SRCS = \
	$(APP_SOURCE_DIR)/app_jump.c  \
	$(APP_SOURCE_DIR)/main.c \
	$(APP_SOURCE_DIR)/ota.c  \
	$(APP_SOURCE_DIR)/uart_dl.c   \
	$(APP_SOURCE_DIR)/bootx.c \
	$(APP_SOURCE_DIR)/abc.c   \
	$(APP_SOURCE_DIR)/encrypt.c  \
	$(APP_SOURCE_DIR)/light_upgrade.c  \
	$(COMPONENT_DIR)/rt_ota/rt_ota_key_port.c \
	$(COMPONENT_DIR)/fal/fal_flash_port.c  \
	$(COMPONENT_DIR)/fal/fal_flash_crc_port.c  \
	
OTA_SRCS = \
	$(PACKAGES_DIR)/fal/src/fal.c        \
	$(PACKAGES_DIR)/fal/src/fal_flash.c       \
	$(PACKAGES_DIR)/fal/src/fal_partition.c   \
	#$(PACKAGES_DIR)/bk_ota/data_move.c  

RISCV_SRCS = \
	$(RISCV_SOURCE_DIR)/reset.c \
	$(RISCV_SOURCE_DIR)/ae350.c \
	$(RISCV_SOURCE_DIR)/initfini.c \
	$(RISCV_SOURCE_DIR)/trap.c \
	$(RISCV_SOURCE_DIR)/interrupt.c
	
TUYA_OTA_SRCS = \
	$(DIFF2YA_SOURCE_DIR)/src/bsdiff_flash.c \
	$(DIFF2YA_SOURCE_DIR)/src/bspatch.c \
	$(DIFF2YA_SOURCE_DIR)/src/diff2ya.c \
	$(DIFF2YA_SOURCE_DIR)/src/diff2ya_hal.c \
	$(DIFF2YA_SOURCE_DIR)/src/tuya_heap_4.c \
	$(DIFF2YA_SOURCE_DIR)/src/ty_bsdiff_adapt.c \
	$(DIFF2YA_SOURCE_DIR)/src/ty_bsdiff_adapt_fs.c \
	$(DIFF2YA_SOURCE_DIR)/src/utils_crc32.c \
	$(DIFF2YA_SOURCE_DIR)/src/xz_read.c \
	$(DIFF2YA_SOURCE_DIR)/xz/xz_crc32.c \
	$(DIFF2YA_SOURCE_DIR)/xz/xz_crc64.c \
	$(DIFF2YA_SOURCE_DIR)/xz/xz_dec_bcj.c \
	$(DIFF2YA_SOURCE_DIR)/xz/xz_dec_lzma2.c \
	$(DIFF2YA_SOURCE_DIR)/xz/xz_dec_stream.c



# Define all object files.
SRCS = $(STARTUP_SRCS)  $(BSP_SRCS) $(APP_SRCS) $(RISCV_SRCS) $(OTA_SRCS) $(TUYA_OTA_SRCS)
OBJS = $(patsubst %.S,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,${SRCS})))
LIBS += $(PACKAGES_DIR)/lib/$(U_BOOTLOADER_ZLIB_NAME)
LIBS += $(PACKAGES_DIR)/lib/$(U_BOOTLOADER_AES_NAME)

### Verbosity control. Use 'make V=1' to get verbose builds.
ifeq ($(V),1)
TRACE_CC  =
TRACE_C++ =
TRACE_LD  =
TRACE_AR  =
TRACE_AS  =
Q=
else
TRACE_CC  = @echo "  CC       " $<
TRACE_C++ = @echo "  C++      " $<
TRACE_LD  = @echo "  LD       " $@
TRACE_AR  = @echo "  AR       " $@
TRACE_AS  = @echo "  AS       " $<
Q=@
endif

# Compilation options

#LDSCRIPT = $(RISCV_SOURCE_DIR)/$(PLATNAME)/$(shell echo $(PLAT) | tr A-Z a-z).ld
LDSCRIPT = ./ld/bootloader.ld

CMODEL = -mcmodel=medany

#
# FreeRTOS needs to define an "portasmHANDLE_INTERRUPT" assembler macro to tell
# the FreeRTOS kernel which external interrupt handler to call.
# Besides, it also needs to specify the linker variable called "__freertos_irq_stack_top"
# that holds the highest address of the interrupt stack. We set __freertos_irq_stack_top
# to equal the value of linker variable "_stack".
#
DEFINES = -DCFG_$(BOARD) -DCFG_$(MODE) -DportasmHANDLE_INTERRUPT=mext_interrupt -D__freertos_irq_stack_top=_stack

INCLUDES = \
	-I. \
	-I$(SYSTEM_SOURCE_DIR) \
	-I$(RISCV_SOURCE_DIR)/include \
	-I$(APP_SOURCE_DIR) \
	-I$(BSP_SOURCE_DIR) \
	-I$(BSP_SOURCE_DIR)/$(PLATNAME) \
	-I$(PACKAGES_DIR)/aes        \
	-I$(PACKAGES_DIR)/fal/inc    \
	-I$(PACKAGES_DIR)/bk_ota     \
	-I$(PACKAGES_DIR)/zlib   \
	-I$(COMPONENT_DIR)/fal \
	-I$(SYSTEM_SOURCE_DIR) \
	-I$(DIFF2YA_SOURCE_DIR)/include \
	-I$(DIFF2YA_SOURCE_DIR)/xz
	

CFLAGS = \
	-Wall $(OPTIM) $(INCLUDES) $(DEFINES) \
	-fomit-frame-pointer -fno-strict-aliasing -fno-builtin -funroll-loops \
	-ffunction-sections -fdata-sections \
	$(CMODEL)

CXXFLAGS = $(CFLAGS)

ASFLAGS = -D__ASSEMBLY__ $(CFLAGS) -c

LDFLAGS = -T$(LDSCRIPT) $(OPTIM) -static -nostartfiles -Wl,--gc-sections,--defsym=__rtos_signature_freertos_v10_3=0x0 \
	-Wl,--defsym=__rtos_tracer_target_api_v1_0_0_freertos=0x0 \
	$(CMODEL)

### Make variables

ifeq ($(USE_CACHE), 1)
	CFLAGS += -DCFG_CACHE_ENABLE
endif

ifeq ($(USE_HWDSP), 1)
	CFLAGS += -mext-dsp
	LDFLAGS += -mext-dsp
endif

# Compilation rules
.SUFFIXES : %.o %.c %.cpp %.S

%.o: %.c
	$(TRACE_CC)
	$(Q)$(CC) -c -MMD $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(TRACE_C++)
	$(Q)$(C++) -c -MMD $(CXXFLAGS) -o $@ $<

%.o: %.S
	$(TRACE_CC)
	$(Q)$(CC) -c -MMD $(CFLAGS) -o $@ $<

all: $(PROG).elf

$(PROG).elf  : $(OBJS) Makefile
	$(TRACE_LD)
	$(Q)$(CC) -Wl,-Map -Wl,$(PROG).map -o $@ $(LDFLAGS) $(OBJS) $(LIBS)
	$(OBJCOPY) -O binary -S $(PROG).elf $(PROG).bin
	$(OBJCOPY) -O binary -S $(PROG).elf $(PROJECT_BUILD_OUT_DIR).bin
	$(OBJDUMP) -d $(PROG).elf > $(PROG).asm
	$(SIZE) $(PROG).elf > $(PROJECT_BUILD_DIR_TEMP)/all_size.txt
	$(AR) -cr $(PROJECT_BUILD_DIR_TEMP)/$(U_BOOTLOADER_LIB_NAME) $(OBJS)
	$(SIZE) $(PROJECT_BUILD_DIR_TEMP)/$(U_BOOTLOADER_LIB_NAME) > $(PROJECT_BUILD_DIR_TEMP)/detail_size.txt
	@echo Completed

clean :
	@rm -f $(OBJS) $(OBJS:.o=.d)
	@rm -f $(PROG).elf
	@rm -f $(PROG).bin
	@rm -f $(PROG).asm
	@rm -f $(PROG).map
	@rm -rf $(PROJECT_BUILD_DIR_TEMP)/*

# Automatic dependency generation
ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif


