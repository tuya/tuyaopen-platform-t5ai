PRE_BUILD_TARGET :=
ifeq ($(SUPPORT_DUAL_CORE),true)
	PRE_BUILD_TARGET += $(ARMINO_SOC)_cp1
endif

ifeq ($(SUPPORT_TRIPLE_CORE),true)
	PRE_BUILD_TARGET += $(ARMINO_SOC)_cp1
	PRE_BUILD_TARGET += $(ARMINO_SOC)_cp2
endif

ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
	ARMINO_BOOTLOADER = $(ARMINO_DIR)/properties/modules/bootloader/aboot/arm_bootloader_ab
endif

BOOTLOADER_JSON_INSEQ := --smode-inseq=1,1,4,0,0,0
BOOTLOADER_JSON := $(ARMINO_DIR)/tools/build_tools/part_table_tools/tempFiles/partition_ota_new.json
BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
#$(error "BOOTLOADER_JSON_OLD = $(BOOTLOADER_JSON_OLD)")