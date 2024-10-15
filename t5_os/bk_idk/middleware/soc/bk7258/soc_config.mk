SUPPORT_TRIPLE_CORE := true
SUPPORT_BOOTLOADER := true
ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
	ARMINO_BOOTLOADER := $(ARMINO_DIR)/properties/modules/bootloader/aboot/arm_bootloader/
endif