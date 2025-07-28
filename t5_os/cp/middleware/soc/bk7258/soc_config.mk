# SUPPORT_TRIPLE_CORE := true
SUPPORT_BOOTLOADER := true

ifeq ($(WIN32), 1)
	COMPILER_TOOLCHAIN_PATH := $(ARMINO_BASH_TOOLS_PATH)/gcc-arm-none-eabi-10.3-2021.10/bin
else
	COMPILER_TOOLCHAIN_PATH := /opt/gcc-arm-none-eabi-10.3-2021.10/bin
endif
