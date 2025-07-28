export ARMINO_AVDK_DIR := $(CURDIR)
export ARMINO_AP_DIR := $(ARMINO_AVDK_DIR)/ap
export ARMINO_CP_DIR := $(ARMINO_AVDK_DIR)/cp

export ARMINO_TOOLS_PATH :=  $(ARMINO_AVDK_DIR)/tools
export ARMINO_TOOL := @$(ARMINO_TOOLS_PATH)/build_tools/armino
export ARMINO_TOOL_WRAPPER := @$(ARMINO_TOOLS_PATH)/build_tools/build.sh


# 1. soc_targets contains all supported SoCs
# 2. cmake_supported_targets contains all targets that can directly
#    passed to armino cmake build system
# 3. cmake_not_supported_targets contains all targets:
#    3.1> armino cmake doesn't support it, only implemented in this
#         Makefile
#    3.2> armino cmake supports it, but has different target name
soc_targets_ap := $(shell find  ap/middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")
soc_targets_cp := $(shell find  cp/middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")

soc_targets = soc_targets_ap soc_targets_cp

cmake_not_supported_targets = help clean doc ap_doc cp_doc
all_targets = cmake_not_supported_targets soc_targets_cp soc_targets_ap cmake_supported_targets
export SOC_SUPPORTED_TARGETS_AP := ${soc_targets_ap}
export SOC_SUPPORTED_TARGETS_CP := ${soc_targets_cp}

export ARMINO_SOC := $(findstring $(MAKECMDGOALS), $(soc_targets))
export CMD_TARGET := $(MAKECMDGOALS)


ifeq ("$(APP_VERSION)", "")
	export APP_VERSION := unknown
else
	export APP_VERSION := $(APP_VERSION)
endif

ifeq ("$(PROJECT)", "")
	export PROJECT := app
else
	export PROJECT := $(PROJECT)
endif


ifeq ("$(PROJECT_DIR)", "")
	export PROJECT_DIR := ${CURDIR}/projects/$(PROJECT)
else
	export PROJECT_DIR := $(PROJECT_DIR)
endif

ifeq ("$(ARMINO_SOC)", "")
ifeq ("$(ARMINO_SOC_LIB)", "")
	ARMINO_SOC := bk7258
	ARMINO_TARGET := $(MAKECMDGOALS)
endif
else
	ARMINO_TARGET := build
endif

export ARMINO_SOC_NAME ?= $(ARMINO_SOC)

export PROJECT_NAME := $(notdir $(PROJECT_DIR))
ifneq ("$(BUILD_DIR)", "")
	export PROJECT_BUILD_DIR := $(BUILD_DIR)/$(ARMINO_SOC_NAME)/$(PROJECT_NAME)
else
	export PROJECT_BUILD_DIR := $(CURDIR)/build/$(ARMINO_SOC_NAME)/$(PROJECT_NAME)
endif

ifdef USE_LIBS_DETERMINED_MODE
	export ARMINO_USE_WRAPPER := 1
	export ARMINO_WRAPPER_PATH := $(ARMINO_AVDK_DIR)
	export ARMINO_WRAPPER_NEW_PATH := /armino_avdk_smp
endif

# verify enable multithread build.
ifdef BK_JENKINS_ID
    MAKEFLAGS += -j2
endif

ifndef PRINT_SUMMARY
	PRINT_SUMMARY := 1
endif

.PHONY: all_targets

help:
	@echo ""
	@echo " make bkxxx - build soc bkxxx"
	@echo " make bkxxx_ap - build soc bkxxx ap"
	@echo " make bkxxx_cp - build soc bkxxx cp"
	@echo " make all - build all soc"
	@echo " make clean - clean build"
	@echo " make help - display this help info"
	@echo " make doc - generate smp doc and cp doc and ap doc"
	@echo " make ap_doc - generate ap doc"
	@echo " make cp_doc - generate cp doc"
	@echo " make smp_doc - generate smp doc"
	@echo ""

common:
	@echo "ARMINO_SOC is set to $(ARMINO_SOC)"
	@echo "ARMINO_TARGET is set to $(ARMINO_TARGET)"
	@echo "armino project path=$(PROJECT_DIR)"
	@echo "armino ap path=$(ARMINO_AP_DIR)"
	@echo "armino cp path=$(ARMINO_CP_DIR)"
	@echo "armino build path=$(PROJECT_BUILD_DIR)"


all: $(soc_targets) $(ARMINO_SOC)_cp

$(soc_targets_ap): common build_prepare
	@make $(ARMINO_SOC)_ap ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) -C $(ARMINO_AP_DIR)

$(ARMINO_SOC)_cp: common build_prepare
	@make $(ARMINO_SOC) ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) -C $(ARMINO_CP_DIR)

$(soc_targets_cp): $(ARMINO_SOC)_ap $(ARMINO_SOC)_cp package

DOCS_PARAMTERS:=
ifneq ("$(DOCS_TARGET)", "")
	DOCS_PARAMTERS += --target $(DOCS_TARGET)
endif
ifneq ("$(DOCS_TYPE)", "")
	DOCS_PARAMTERS += --type $(DOCS_TYPE)
endif
ifneq ("$(DOCS_VERSION)", "")
	DOCS_PARAMTERS += --version $(DOCS_VERSION)
endif

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
	export WIN32 := 1
	PRINT_SUMMARY := 0
	export PYTHONPATH := $(ARMINO_TOOLS_PATH)/env_tools/bk_py_libs;$(PYTHONPATH)
else
	export WIN32 := 0
	export PYTHONPATH := $(ARMINO_TOOLS_PATH)/env_tools/bk_py_libs:$(PYTHONPATH)
endif

AUTO_PARTITION_TABLE := $(PROJECT_DIR)/partitions/$(ARMINO_SOC_NAME)/auto_partitions.csv
export PARTITIONS_DIR := $(PROJECT_BUILD_DIR)/partitions
auto_partition_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_auto_partition.py
auto_partition_out := $(PARTITIONS_DIR)/partitions.txt

$(auto_partition_out): $(auto_partition_script) $(AUTO_PARTITION_TABLE)
	@mkdir -p $(PARTITIONS_DIR)
	@python3 $(auto_partition_script)

print_partitions: $(auto_partition_out)
	@echo ===================== Partitions Table =====================
	@cat $(auto_partition_out)
	@echo ============================================================

ram_partition_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_ram_regions.py
RAM_REGIONS_TABLE := $(PROJECT_DIR)/partitions/$(ARMINO_SOC_NAME)/ram_regions.csv
ram_regions_out := $(PARTITIONS_DIR)/ram_regions.h
$(ram_regions_out): $(RAM_REGIONS_TABLE)
	@mkdir -p $(PARTITIONS_DIR)
	@python3 $(ram_partition_script)

build_prepare: $(auto_partition_out) print_partitions $(ram_regions_out)

package_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_package.py
package_dir := $(PROJECT_BUILD_DIR)/package
package_json := $(PARTITIONS_DIR)/bk_package.json
build_summary := $(package_dir)/build_summary.txt
package: $(package_script) $(ARMINO_SOC)_cp $(soc_targets_ap)
	@mkdir -p $(package_dir)
	@python3 $(package_script) $(PROJECT_BUILD_DIR) $(package_json) $(build_summary)
ifneq ($(PRINT_SUMMARY), 0)
	@cat $(build_summary)
endif

ap_doc:
	@make doc ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) -C $(ARMINO_AP_DIR)

cp_doc:
	@make doc ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) -C $(ARMINO_CP_DIR)

smp_doc:
	@python3 ./tools/armino_doc.py $(DOCS_PARAMTERS)

doc: smp_doc ap_doc cp_doc

# only build bootloader
bootloader_build_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_sdk/bl_build.py
bl:
	@python $(bootloader_build_script) $(PROJECT_DIR) $(CURDIR)/build bk7258

clean:
	@echo "rm -rf ./build"
	@python3 ./tools/armino_doc.py --clean True
	@rm -rf ./build
	@rm -rf $(ARMINO_AP_DIR)/build
	@rm -rf $(ARMINO_CP_DIR)/build

