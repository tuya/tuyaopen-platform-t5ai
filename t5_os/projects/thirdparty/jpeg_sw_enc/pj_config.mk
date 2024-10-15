PRE_BUILD_TARGET :=

	PRE_BUILD_TARGET += $(ARMINO_SOC)_cp1


ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
endif