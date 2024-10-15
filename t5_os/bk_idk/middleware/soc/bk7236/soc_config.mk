SUPPORT_BOOTLOADER := true
ifeq ($(SUPPORT_BOOTLOADER),true)
	ARMINO_BOOTLOADER := $(ARMINO_DIR)/properties/modules/bootloader/aboot/arm_bootloader/
endif