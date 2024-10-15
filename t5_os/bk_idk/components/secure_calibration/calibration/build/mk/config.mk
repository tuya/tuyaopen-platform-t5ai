################################################################################
# Config values for build configurations
################################################################################

# Build verbosity
V				:= 0
# Debug build
# Build platform
DEFAULT_PLAT			:= bk7236_v2022
PLAT				:= ${DEFAULT_PLAT}
# Testbed
TESTBED                 	:= 0
# Cross compile
DFT_CROSS_COMPILE		:= arm-none-eabi-
CROSS_COMPILE			:= ${TOOLCHAIN_PATH}/${DFT_CROSS_COMPILE}
# TEE SDK PATH
TEE_SDK_PATH			:= $(top-dir)/TEE_M


################################################################################
# spe config feature
################################################################################
CONFIG_SST_SUPPORT			:= Y
CONFIG_MBEDTLS_SUPPORT			:= Y
MBEDTLS_WITH_DUBHE			:= Y
CONFIG_FLASH_SP_SUPPORT			:= Y
CONFIG_CB_SUPPORT			:= Y
CONFIG_CB_SOCKET			:= N
CONFIG_STLS				:= Y

SECURE_BOOT				:= Y
SECURE_DBG				:= Y
PROVISIONING				:= Y
TRNG_CALIBRATION			:= Y
GPIO_ENA                                := N

# Beken specific options
BK_BOOT					:= Y
SKIP_FLASH_WRITE			:= Y
BOOT_V2					:= N
SW_SHA384                               := N
LOAD_IMAGE_VIA_FLASH_CBUS               := Y
LOAD_MANIFEST_VIA_FLASH_CBUS            := N
FPGA                                    := Y
HW_FIH                                  := Y
FIH       			        := Y
FIH_MULTI_FIELDS       			:= Y

FLASH_CONTROL_PARTITION			:= Y
EFUSE                                   := Y
REVERT_OTP       			:= N
WDT                                     := Y
DUMP_BUF                                := N
TE200_UT       			        := N
PRNG       			        := N
MBEDTLS_PORT_TEST_CODE_SWITCH           := Y

ifeq ($(FPGA), Y)
# Log level:
#   -1 - disabled
#   0 - error
#   1 - warning
#   2 - info
#   3 - debug
#   4 - trace
#   5 - data
CFLAGS		                        += -g -DCONFIG_RUNTIME_LOG_LEVEL=3 -DCONFIG_COMPILE_LOG_LEVEL=3
ASFLAGS		                        += -g -DCONFIG_RUNTIME_LOG_LEVEL=3 -DCONFIG_COMPILE_LOG_LEVEL=3
DEBUG                                   := Y
REDUCE_CODE_SIZE                        := N
SIMU_OTP_IN_FLASH			:= Y
UART_1M					:= N
LEGACY_BOOT                             := N
PERF					:= Y
CACHE					:= Y
FORCE_ENABLE_PLL                        := Y
else
CFLAGS		                        += -g -DCONFIG_RUNTIME_LOG_LEVEL=3 -DCONFIG_COMPILE_LOG_LEVEL=2
ASFLAGS		                        += -g -DCONFIG_RUNTIME_LOG_LEVEL=3 -DCONFIG_COMPILE_LOG_LEVEL=2
DEBUG                                   := N
REDUCE_CODE_SIZE                        := Y
SIMU_OTP_IN_FLASH			:= N
UART_1M					:= Y
LEGACY_BOOT                             := Y
PERF					:= N
CACHE					:= N
FORCE_ENABLE_PLL                        := N
endif

# Beken debug/verification only options
FORCE_RECOVERY_BOOT			:= N
C_DEEP_SLEEP_RST                        := N

################################################################################
# Assert config
################################################################################
CONFIG_ASSERT_EN			:= Y

################################################################################
# PUF config
################################################################################
PUF             := N
PUF_SCRAMBLE    := N

################################################################################
# Process build options
################################################################################

# Verbose
ifneq ($V,1)
	Q := @
	CMD_ECHO := true
	CMD_ECHO_SILENT := echo
else
	Q :=
	CMD_ECHO := echo
	CMD_ECHO_SILENT := true
endif

# Debug
ifeq ($(DEBUG), Y)
	BUILD_TYPE	:= debug
else
	BUILD_TYPE	:= release
endif

# Assert Enable Config
ifeq ($(CONFIG_ASSERT_EN),Y)
	CFLAGS		+=    -DPAL_ASSERT_ENABLE
	ASFLAGS		+=    -DPAL_ASSERT_ENABLE
endif

# Output directory
ifeq ($O,)
	TOP_OUT	:= $(top-dir)/out/$(BUILD_TYPE)/$(PLAT)
	OUT_DIR	:= $(TOP_OUT)/$(COMPONENT_OUT)
else
	TOP_OUT	:= $(O)/tmp
	OUT_DIR	:= $(O)
endif

# Callback
ifeq ($(CONFIG_CB_SUPPORT),Y)
	CFLAGS		+= -DCONFIG_CB_SUPPORT=1
	ASFLAGS		+= -DCONFIG_CB_SUPPORT=1
else
	CFLAGS		+= -DCONFIG_CB_SUPPORT=0
	ASFLAGS		+= -DCONFIG_CB_SUPPORT=0
endif

# Callback
ifeq ($(CONFIG_CB_SUPPORT),Y)
ifeq ($(CONFIG_CB_SOCKET),Y)
	CFLAGS		+= -DCONFIG_CB_SOCKET=1
	ASFLAGS		+= -DCONFIG_CB_SOCKET=1
else
	CFLAGS		+= -DCONFIG_CB_SOCKET=0
	ASFLAGS		+= -DCONFIG_CB_SOCKET=0
endif
else
ifeq ($(CONFIG_CB_SOCKET),Y)
$(error CONFIG_CB_SOCKET depend on CONFIG_CB_SUPPORT)
endif
endif

ifeq ($(CONFIG_CB_SUPPORT),Y)
ifeq ($(CONFIG_STLS),Y)
	CFLAGS		+= -DCONFIG_STLS=1
	ASFLAGS		+= -DCONFIG_STLS=1
else
	CFLAGS		+= -DCONFIG_STLS=0
	ASFLAGS		+= -DCONFIG_STLS=0
endif
else
ifeq ($(CONFIG_STLS),Y)
$(error CONFIG_STLS depend on CONFIG_CB_SUPPORT)
endif
endif

# Secure Boot
ifeq ($(SECURE_BOOT),Y)
	CFLAGS		+= -DSECURE_BOOT=1
	ASFLAGS		+= -DSECURE_BOOT=1
else
	CFLAGS		+= -DSECURE_BOOT=0
	ASFLAGS		+= -DSECURE_BOOT=0
endif

# Secure Debug
ifeq ($(SECURE_DBG),Y)
	CFLAGS		+= -DSECURE_DBG=1
	ASFLAGS		+= -DSECURE_DBG=1
else
	CFLAGS		+= -DSECURE_DBG=0
	ASFLAGS		+= -DSECURE_DBG=0
endif

# Provisioning
ifeq ($(PROVISIONING),Y)
	CFLAGS		+= -DPROVISIONING=1
	ASFLAGS		+= -DPROVISIONING=1
else
	CFLAGS		+= -DPROVISIONING=0
	ASFLAGS		+= -DPROVISIONING=0
endif

# TRNG_CALIBRATION
ifeq ($(TRNG_CALIBRATION),Y)
ifneq ($(MBEDTLS_WITH_DUBHE), Y)
$(error TRNG_CALIBRATION depend on MBEDTLS_WITH_DUBHE)
endif
	CFLAGS		+= -DTRNG_CALIBRATION=1
	ASFLAGS		+= -DTRNG_CALIBRATION=1
else
	CFLAGS		+= -DTRNG_CALIBRATION=0
	ASFLAGS		+= -DTRNG_CALIBRATION=0
endif

# MBEDTLS_WITH_DUBHE
ifeq ($(MBEDTLS_WITH_DUBHE),Y)
	CFLAGS		+= -DMBEDTLS_WITH_DUBHE=1
	ASFLAGS		+= -DMBEDTLS_WITH_DUBHE=1
else
	CFLAGS		+= -DMBEDTLS_WITH_DUBHE=0
	ASFLAGS		+= -DMBEDTLS_WITH_DUBHE=0
endif

# PUF
ifeq ($(PUF),Y)
	CFLAGS		+= -DDBH_OTP_PUF_SUPPORT
	CFLAGS		+= -DCE_LITE_OTP_PUF
	ASFLAGS		+= -DDBH_OTP_PUF_SUPPORT
	ASFLAGS		+= -DCE_LITE_OTP_PUF
ifeq ($(PUF_SCRAMBLE),Y)
	CFLAGS		+= -DDBH_OTP_SCRAMBLE
	ASFLAGS		+= -DDBH_OTP_SCRAMBLE
endif ### end PUF_SCRAMBLE
endif ### end PUF

ifeq ($(BK_BOOT),Y)
	CFLAGS		+= -DCONFIG_BK_BOOT
	ASFLAGS		+= -DCONFIG_BK_BOOT
endif

ifeq ($(UART_1M),Y)
	CFLAGS		+= -DCONFIG_UART_1M
	ASFLAGS		+= -DCONFIG_UART_1M
endif

ifeq ($(SIMU_OTP_IN_FLASH),Y)
	CFLAGS		+= -DCONFIG_SIMU_OTP_IN_FLASH
	ASFLAGS		+= -DCONFIG_SIMU_OTP_IN_FLASH
endif

ifeq ($(SKIP_FLASH_WRITE),Y)
	CFLAGS		+= -DCONFIG_SKIP_FLASH_WRITE
	ASFLAGS		+= -DCONFIG_SKIP_FLASH_WRITE
endif

ifeq ($(BOOT_V2),Y)
	CFLAGS		+= -DCONFIG_BOOT_V2
	ASFLAGS		+= -DCONFIG_BOOT_V2
endif

ifeq ($(FORCE_RECOVERY_BOOT),Y)
	CFLAGS		+= -DCONFIG_FORCE_RECOVERY_BOOT
	ASFLAGS		+= -DCONFIG_FORCE_RECOVERY_BOOT
endif

ifeq ($(FLASH_CONTROL_PARTITION),Y)
	CFLAGS		+= -DCONFIG_FLASH_CONTROL_PARTITION
	ASFLAGS		+= -DCONFIG_FLASH_CONTROL_PARTITION
endif

ifeq ($(SW_SHA384),Y)
	CFLAGS		+= -DCONFIG_SW_SHA384
	ASFLAGS		+= -DCONFIG_SW_SHA384
endif

ifeq ($(C_DEEP_SLEEP_RST),Y)
	CFLAGS		+= -DCONFIG_C_DEEP_SLEEP_RST
	ASFLAGS		+= -DCONFIG_C_DEEP_SLEEP_RST
endif

ifeq ($(REDUCE_CODE_SIZE),Y)
	CFLAGS		+= -DCONFIG_REDUCE_CODE_SIZE
	ASFLAGS		+= -DCONFIG_REDUCE_CODE_SIZE
endif

ifeq ($(FPGA),Y)
	CFLAGS		+= -DCONFIG_FPGA
	ASFLAGS		+= -DCONFIG_FPGA
endif

ifeq ($(EFUSE),Y)
	CFLAGS		+= -DCONFIG_EFUSE
	ASFLAGS		+= -DCONFIG_EFUSE
endif

ifeq ($(WDT),Y)
	CFLAGS		+= -DCONFIG_WDT
	ASFLAGS		+= -DCONFIG_WDT
endif

ifeq ($(PERF),Y)
	CFLAGS		+= -DCONFIG_PERF
	ASFLAGS		+= -DCONFIG_PERF
endif

ifeq ($(REVERT_OTP),Y)
	CFLAGS		+= -DCONFIG_REVERT_OTP
	ASFLAGS		+= -DCONFIG_REVERT_OTP
endif

ifeq ($(LOAD_IMAGE_VIA_FLASH_CBUS),Y)
	CFLAGS		+= -DCONFIG_LOAD_IMAGE_VIA_FLASH_CBUS
	ASFLAGS		+= -DCONFIG_LOAD_IMAGE_VIA_FLASH_CBUS
endif

ifeq ($(LOAD_MANIFEST_VIA_FLASH_CBUS),Y)
	CFLAGS		+= -DCONFIG_LOAD_MANIFEST_VIA_FLASH_CBUS
	ASFLAGS		+= -DCONFIG_LOAD_MANIFEST_VIA_FLASH_CBUS
endif

ifeq ($(PRNG),Y)
	CFLAGS		+= -DCONFIG_PRNG
	ASFLAGS		+= -DCONFIG_PRNG
endif

ifeq ($(TE200_UT),Y)
	CFLAGS		+= -DCONFIG_TE200_UT
	ASFLAGS		+= -DCONFIG_TE200_UT
endif

ifeq ($(HW_FIH),Y)
	CFLAGS		+= -DCONFIG_HW_FIH
	ASFLAGS		+= -DCONFIG_HW_FIH
endif

ifeq ($(LEGACY_BOOT),Y)
	CFLAGS		+= -DCONFIG_LEGACY_BOOT
	ASFLAGS		+= -DCONFIG_LEGACY_BOOT
endif

ifeq ($(DUMP_BUF),Y)
	CFLAGS		+= -DCONFIG_DUMP_BUF
	ASFLAGS		+= -DCONFIG_DUMP_BUF
endif

ifeq ($(FIH),Y)
	CFLAGS		+= -DCONFIG_FIH
	CFLAGS		+= -DTFM_FIH_PROFILE_ON
	CFLAGS		+= -DTFM_FIH_PROFILE_LOW
	CFLAGS		+= -DFIH_ENABLE_DELAY
	ASFLAGS		+= -DCONFIG_FIH
endif

ifeq ($(FIH_MULTI_FIELDS),Y)
	CFLAGS		+= -DCONFIG_FIH_MULTI_FIELDS
	ASFLAGS		+= -DCONFIG_FIH_MULTI_FIELDS
endif

ifeq ($(FIH_MULTI_FIELDS),Y)
	CFLAGS		+= -DCONFIG_FPGA_XTAL_CLOCK=26000000
endif

ifeq ($(CACHE),Y)
	CFLAGS		+= -D__ICACHE_PRESENT=1
	CFLAGS		+= -D__DCACHE_PRESENT=1
	CFLAGS		+= -DCONFIG_CACHE
endif

ifeq ($(FORCE_ENABLE_PLL),Y)
	CFLAGS		+= -DCONFIG_FORCE_ENABLE_PLL
endif

ifeq ($(GPIO_ENA),Y)
	CFLAGS		+= -DCONFIG_GPIO_ENA
endif


ifeq ($(MBEDTLS_PORT_TEST_CODE_SWITCH),Y)
	CFLAGS		+= -DCONFIG_MBEDTLS_PORT_TEST_CODE_SWITCH
	ASFLAGS		+= -DCONFIG_MBEDTLS_PORT_TEST_CODE_SWITCH
endif
################################################################################
# nspe lib config
################################################################################

TEE_CLIENT_LIB			:=
ifneq ($(TEE_SDK_PATH),)
	TEE_CLIENT_LIB		:=	$(TEE_SDK_PATH)/nspe/lib/libclient.a
else
	TEE_CLIENT_LIB		:=	$(TOP_OUT)/tee/libclient.a
endif

################################################################################
# Toolchain
################################################################################

CC			:=	${CROSS_COMPILE}gcc
CPP			:=	${CROSS_COMPILE}cpp
AS			:=	${CROSS_COMPILE}gcc
AR			:=	${CROSS_COMPILE}ar
LD			:=	${CROSS_COMPILE}ld
OC			:=	${CROSS_COMPILE}objcopy
OD			:=	${CROSS_COMPILE}objdump
NM			:=	${CROSS_COMPILE}nm
PP			:=	${CROSS_COMPILE}gcc -E

################################################################################
# Global helpers
################################################################################

#
# remove directory helper
#
RMDIR := rmdir --ignore-fail-on-non-empty


