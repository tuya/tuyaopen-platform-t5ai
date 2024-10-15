export CMD_ARMINO_SOC := $(findstring $(MAKECMDGOALS), $(soc_targets))


############################################################
#        Create supported projects list for bk7235         #
############################################################
bk7235xx_supported_targets := bk7235
bk7235xx_supported_projects := app
bk7235xx_supported_projects := $(bk7235xx_supported_projects)at


############################################################
#        Create supported projects list for bk7256         #
############################################################
bk7256xx_supported_targets := bk7256
bk7256xx_supported_projects := app
bk7256xx_supported_projects := $(bk7256xx_supported_projects) ate_mini_code
bk7256xx_supported_projects := $(bk7256xx_supported_projects) bluetooth/central bluetooth/hci bluetooth/headset bluetooth/mesh
bk7256xx_supported_projects := $(bk7256xx_supported_projects) lvgl/86box lvgl/benchmark lvgl/camera lvgl/keypad_encoder lvgl/lcd_qspi_meter lvgl/meter lvgl/music lvgl/psram_4MB lvgl/soundbar lvgl/stress lvgl/widgets
bk7256xx_supported_projects := $(bk7256xx_supported_projects) media/audio_player media/av_client media/av_server media/doorbell media/audio_play_sdcard_mp3_music media/audio_record_to_sdcard media/dual_device_voice_call
bk7256xx_supported_projects := $(bk7256xx_supported_projects) thirdparty/agora thirdparty/p2p_client thirdparty/p2p_server thirdparty/wanson_asr
bk7256xx_supported_projects := $(bk7256xx_supported_projects) lvgl/86box_smart_panel
bk7256xx_supported_projects := $(bk7256xx_supported_projects) wifi/repeater
bk7256xx_supported_projects := $(bk7256xx_supported_projects) at

############################################################
#        Create supported projects list for bk7236         #
############################################################
bk7236xx_supported_targets := bk7236
bk7236xx_supported_projects := app clip
bk7236xx_supported_projects := $(bk7236xx_supported_projects) wifi/repeater
bk7236xx_supported_projects := $(bk7236xx_supported_projects) at
bk7236xx_supported_projects := $(bk7236xx_supported_projects) customization/config_ab

############################################################
#        Create supported projects list for bk7258         #
############################################################
bk7258xx_supported_targets := bk7258
bk7258xx_supported_projects := app smp
bk7258xx_supported_projects := $(bk7258xx_supported_projects) bluetooth/central bluetooth/headset bluetooth/spp bluetooth/mesh bluetooth/bt_hidd bluetooth/bt_hidh bluetooth/gatt_server bluetooth/gatt_client
bk7258xx_supported_projects := $(bk7258xx_supported_projects) media/audio_play_sdcard_mp3_music media/doorbell_8M media/doorviewer media/doorbell_pro media/audio_record_to_sdcard 
bk7258xx_supported_projects := $(bk7258xx_supported_projects) media/doorbell media/doorviewer_8M media/lcd_mcu_8080 media/media_transfer media/dual_device_voice_call media/doorbell_720p
bk7258xx_supported_projects := $(bk7258xx_supported_projects) lvgl/86box lvgl/86box_smart_panel lvgl/stress lvgl/benchmark lvgl/meter_rgb_16M lvgl/soundbar lvgl/widgets lvgl/camera lvgl/meter lvgl/meter_spi lvgl/music lvgl/keypad_encoder
bk7258xx_supported_projects := $(bk7258xx_supported_projects) thirdparty/doorbell_cs2 thirdparty/doorbell_cs2_8M thirdparty/wanson_asr thirdparty/doorbell_cs2_720p
bk7258xx_supported_projects := $(bk7258xx_supported_projects) phy/cert_test
bk7258xx_supported_projects := $(bk7258xx_supported_projects) wifi/repeater
bk7258xx_supported_projects := $(bk7258xx_supported_projects) at
bk7258xx_supported_projects := $(bk7258xx_supported_projects) customization/config_ab
# Modified by TUYA Start
bk7258xx_supported_projects := $(bk7258xx_supported_projects) tuya_app
# Modified by TUYA End

############################################################
#        Create supported projects list for bk7234         #
############################################################
bk7234xx_supported_targets := bk7234
bk7234xx_supported_projects := app
bk7234xx_supported_projects := $(bk7234xx_supported_projects) customization/config_ab

############################################################
#        Create supported projects list for bk7239         #
############################################################
bk7239xx_supported_targets := bk7239
bk7239xx_supported_projects := app


############################################################
#        Create supported projects list for bk723L         #
############################################################
bk723Lxx_supported_targets := bk723L
bk723Lxx_supported_projects := app
bk723Lxx_supported_projects := $(bk723Lxx_supported_projects) customization/config_ab

PART_TABLE_SUPPORTED_TARGETS := $(bk7235xx_supported_targets) $(bk7256xx_supported_targets) $(bk7236xx_supported_targets) $(bk7258xx_supported_targets) $(bk7234xx_supported_targets) $(bk7239xx_supported_targets) $(bk723Lxx_supported_targets)
PART_TABLE_SUPPORTED_PROJECTS := app
ifneq ($(findstring $(ARMINO_SOC), $(bk7235xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7235xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk7256xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7256xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk7236xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7236xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk7258xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7258xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk7234xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7234xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk7239xx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk7239xx_supported_projects)
endif
ifneq ($(findstring $(ARMINO_SOC), $(bk723Lxx_supported_targets)),)
	PART_TABLE_SUPPORTED_PROJECTS := $(bk723Lxx_supported_projects)
endif

ARMINO_TOOL_PART_TABLE := $(ARMINO_DIR)/tools/build_tools/part_table_tools/gen_bk7256partitions.py
PARTITIONS_ARGS := --flash-size=16MB --smode
BOOTLOADER_JSON_INSEQ := --smode-inseq=1,1,2,0,0,0
BOOTLOADER_JSON := $(ARMINO_DIR)/tools/build_tools/part_table_tools/tempFiles/partition_bk7256_ota_a_new.json
BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)tools/partition_bk7256_ota_a.json
BOOTLOADER_JSON_PACK := $(ARMINO_DIR)/tools/env_tools/beken_packager/partition_bootloader.json
ifeq ("$(ARMINO_SOC)", "bk7258")
	BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
endif
ifeq ("$(ARMINO_SOC)", "bk7236")
	BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
endif
ifeq ("$(ARMINO_SOC)", "bk7234")
	BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
endif
ifeq ("$(ARMINO_SOC)", "bk7239")
	BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
endif
ifeq ("$(ARMINO_SOC)", "bk723L")
	BOOTLOADER_JSON_OLD := $(ARMINO_BOOTLOADER)/tools/partition_ota.json
endif
CLEAN_ALLFILE_INSEQ := --smode-inseq=3,0
SHOW_APPS_INSEQ := --smode-inseq=4,0
DEFAULT_CSV_FILE := $(ARMINO_DIR)/tools/build_tools/part_table_tools/bk7256Partitions.csv

ifneq ($(findstring $(ARMINO_SOC), $(PART_TABLE_SUPPORTED_TARGETS)),)
ifneq ($(findstring $(PROJECT), $(PART_TABLE_SUPPORTED_PROJECTS)),)
	main_target_config := $(ARMINO_DIR)/$(PROJECT_DIR)/config/$(ARMINO_SOC).config
	ifneq ($(wildcard $(ARMINO_DIR)/$(PROJECT_DIR)/config/$(ARMINO_SOC)/config),)
		main_target_config := $(ARMINO_DIR)/$(PROJECT_DIR)/config/$(ARMINO_SOC)/config
	endif
	target_config_tool := $(ARMINO_DIR)/tools/build_tools/part_table_tools/otherScript/get_target_config_val.py
	target_config_tool_args := --config=$(main_target_config)
	config_value := $(shell python3 $(target_config_tool) $(target_config_tool_args))
	ifeq ("$(config_value)", "y")
		PARTITIONS_CSV_FILE := $(ARMINO_DIR)/$(PROJECT_DIR)/csv/$(ARMINO_SOC).csv
		ifneq ($(wildcard $(ARMINO_DIR)/$(PROJECT_DIR)/config/$(ARMINO_SOC)/$(ARMINO_SOC)_partitions.csv),)
			PARTITIONS_CSV_FILE := $(ARMINO_DIR)/$(PROJECT_DIR)/config/$(ARMINO_SOC)/$(ARMINO_SOC)_partitions.csv
		endif
	else
		PARTITIONS_CSV_FILE := $(ARMINO_DIR)/middleware/boards/$(ARMINO_SOC)/partitions.csv
	endif

	app_names := $(shell python3 $(ARMINO_TOOL_PART_TABLE) $(PARTITIONS_CSV_FILE) $(PARTITIONS_ARGS) $(SHOW_APPS_INSEQ))
	app1_search := app1
	ifneq ($(findstring $(app1_search), $(app_names)),)
		SUPPORT_DUAL_CORE := true
	else
		SUPPORT_DUAL_CORE := false
	endif
else
	PARTITIONS_CSV_FILE := $(DEFAULT_CSV_FILE)
endif
else
	PARTITIONS_CSV_FILE := $(DEFAULT_CSV_FILE)
endif

PART_TABLE_SUPPORTED_PROJECTS :=$(PART_TABLE_SUPPORTED_PROJECTS) customization/bk7256_configa customization/bk7256_configb

PT_VERBOSE ?= 0
ifeq ("$(PT_VERBOSE)", "1")
	export PT_VERBOSE := 1
else
	export PT_VERBOSE := 0
endif
ifeq ("$(PT_VERBOSE)", "1")
$(info "<========================part_table_tools info start========================>")
$(info "PT_VERBOSE: $(PT_VERBOSE)")
$(info "CMD_ARMINO_SOC: $(CMD_ARMINO_SOC)")
$(info "CONFIG_OVERRIDE_FLASH_PARTITION: $(config_value)")
$(info "PARTITIONS_CSV_FILE: $(PARTITIONS_CSV_FILE)")
$(info "BOOTLOADER_JSON: $(BOOTLOADER_JSON)")
$(info "BOOTLOADER_JSON_INSEQ: $(BOOTLOADER_JSON_INSEQ)")
$(info "SHOW_APPS_INSEQ: $(SHOW_APPS_INSEQ)")
$(info "app_names: $(app_names)")
$(info "SUPPORT_DUAL_CORE: $(SUPPORT_DUAL_CORE)")
$(info "PROJECT: $(PROJECT)")
$(info "<========================part_table_tools info stop ========================>")
endif
