export ARMINO_AVDK_DIR := $(shell pwd)

ARMINO_IDK_DIR := $(shell pwd)/bk_idk
ARMINO_TOOL := @$(ARMINO_IDK_DIR)/tools/build_tools/armino
ARMINO_TOOL_WRAPPER := @$(ARMINO_IDK_DIR)/tools/build_tools/build.sh

# 1. soc_targets contains all supported SoCs
# 2. cmake_supported_targets contains all targets that can directly
#    passed to armino cmake build system
# 3. cmake_not_supported_targets contains all targets:
#    3.1> armino cmake doesn't support it, only implemented in this
#         Makefile
#    3.2> armino cmake supports it, but has different target name
soc_targets := $(shell find  bk_idk/middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")

cmake_not_supported_targets = help clean doc
all_targets = cmake_not_supported_targets soc_targets cmake_supported_targets
export SOC_SUPPORTED_TARGETS := ${soc_targets}

export ARMINO_SOC := $(findstring $(MAKECMDGOALS), $(soc_targets))
export CMD_TARGET := $(MAKECMDGOALS)

ifeq ("$(APP_VERSION)", "")
	export APP_VERSION := unknownn
else
	export APP_VERSION := test
endif

ifeq ("$(PROJECT)", "")
	export PROJECT := media/doorbell
else
	export PROJECT := $(PROJECT)
endif


ifeq ("$(PROJECT_DIR)", "")
	PROJECT_DIR := ../projects/$(PROJECT)
else
	PROJECT_DIR := $(PROJECT_DIR)
endif

ifeq ("$(ARMINO_SOC)", "")
ifeq ("$(ARMINO_SOC_LIB)", "")
	ARMINO_SOC := bk7256
	ARMINO_TARGET := $(MAKECMDGOALS)
endif
else
	ARMINO_TARGET := build
endif

PROJECT_NAME := $(notdir $(PROJECT_DIR))
ifdef BUILD_DIR
	PROJECT_BUILD_DIR := $(BUILD_DIR)
else
	PROJECT_BUILD_DIR := ../build/$(PROJECT_NAME)
endif


.PHONY: all_targets

help:
	@echo ""
	@echo " make bkxxx - build soc bkxxx"
	@echo " make all - build all soc"
	@echo " make clean - clean build"
	@echo " make help - display this help info"
	@echo " make doc - generate avdk doc"
	@echo ""

common:
	@echo "ARMINO_SOC is set to $(ARMINO_SOC)"
	@echo "ARMINO_TARGET is set to $(ARMINO_TARGET)"
	@echo "armino project path=$(PROJECT_DIR)"
	@echo "armino path=$(ARMINO_IDK_DIR)"
	@echo "armino build path=$(PROJECT_BUILD_DIR)"


all: $(soc_targets)

$(soc_targets): common
	@make $(ARMINO_SOC) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) -C $(ARMINO_IDK_DIR)

doc:
	@python3 ./tools/armino_doc.py

clean:
	@echo "rm -rf ./build"
	@python3 ./tools/armino_doc.py clean
	@rm -rf ./build ./bk_idk/build

