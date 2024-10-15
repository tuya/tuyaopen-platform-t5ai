
CONFIG_MSP_STACK_SIZE		:=	10240

GLOBAL_INCLUDES	:=	\
			$(COMMON_INCS)	\
			$(PLAT_INCS)	\
			-Iboard/$(PLAT)/layout/bootrom


#EXT_IMG_SOURCES	:=	\
			#$(EXT_IMG_TOP)/main.c


BINARY		:=	ext_img
TARGET_OUT	:=	ext_img
#SRCS		:=	$(sort $(EXT_IMG_SOURCES))
#LDSCRIPT	:=  $(EXT_IMG_TOP)/linker.ld.S

CFLAGS_$(BINARY)	:=	$(PLAT_CPU_ARCH) $(GLOBAL_INCLUDES)  -DBUILD_TYPE=\"$(BUILD_TYPE)\"
ASFLAGS_$(BINARY)	:=	$(PLAT_CPU_ARCH) $(GLOBAL_INCLUDES) -DCONFIG_MSP_STACK_SIZE=$(CONFIG_MSP_STACK_SIZE) $(SEC_BOOT_ASFLAGS) $(COMMON_DBH_ASFLAGS)
LDFLAGS_$(BINARY)	:=	-static	--print-memory-usage
ifeq ($(CONFIG_MEM_LEAK),Y)
DEPLIB 				:= $(LIBGCC)
else
DEPLIB 				:=
endif

include ${mk-dir}/link.mk


