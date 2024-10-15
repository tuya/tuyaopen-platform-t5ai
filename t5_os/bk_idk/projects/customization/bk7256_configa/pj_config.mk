PRE_BUILD_TARGET :=
ifeq ($(SUPPORT_DUAL_CORE),true)
	PRE_BUILD_TARGET += $(ARMINO_SOC)_cp1
endif

ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
	ARMINO_BOOTLOADER = $(ARMINO_DIR)/properties/modules/bootloader/aboot/bootloader-a-b
endif

PARTITIONS_CSV_FILE := $(ARMINO_DIR)/$(PROJECT_DIR)/csv/$(ARMINO_SOC).csv
PARTITIONS_ARGS := --flash-size=16MB --smode
BOOTLOADER_JSON_INSEQ := --smode-inseq=1,1,3,0,0,0
BOOTLOADER_JSON := $(ARMINO_DIR)/tools/build_tools/part_table_tools/tempFiles/partition_bk7256_ota_r_new.json
BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_bk7256_ota_r.json
app_names := $(shell python3 $(ARMINO_TOOL_PART_TABLE) $(PARTITIONS_CSV_FILE) $(PARTITIONS_ARGS) $(SHOW_APPS_INSEQ))