COMPILER_TOOLCHAIN_PATH := $(CURDIR)/../../../tools/gcc-arm-none-eabi-10.3-2021.10/bin
ifneq ($(TUYA_TOOLCHAIN_PATH),)
	# tuya export TUYA_TOOLCHAIN_PATH=xxx
	COMPILER_TOOLCHAIN_PATH := $(TUYA_TOOLCHAIN_PATH)/bin
endif

PRE_BUILD_TARGET :=

ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
endif

