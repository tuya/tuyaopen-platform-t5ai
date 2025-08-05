COMPILER_TOOLCHAIN_PATH := $(CURDIR)/../../../tools/gcc-arm-none-eabi-10.3-2021.10/bin

PRE_BUILD_TARGET :=

ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
endif

