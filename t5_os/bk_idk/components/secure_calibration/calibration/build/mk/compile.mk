# Input
#
# SRCS      	source file list
# TARGET    	target name
# TARGET_OUT	target out dir
# CFLAGS    	optional, target specific cflags
# ASFLAGS   	optional, target specific asflags
# OUT_DIR   	output directory
# conf-file 	[optional] if set, all objects will depend on $(conf-file)
#
# Output
#
# set	  OBJS
# update  CLEANFILES
#
# Generates explicit rules for all objs

OBJS		:=

# Disable all builtin rules
.SUFFIXES:

.PHONY: FORCE
FORCE:

##################################################################
# common defines                                                 #
##################################################################

COMP_CFLAGS	:= 	-ffreestanding -Wall				\
			-Werror -Wmissing-include-dirs			\
			-std=c99 -fno-short-enums			\
			-fno-pie	\
			-fno-pic -fno-strict-overflow			\
			-fno-reorder-functions -fno-defer-pop		\
			-mabi=aapcs -Os -fno-builtin -nostdlib	\
			-ffunction-sections -fdata-sections $(THUMBCFLAGS)	\
			$(CFLAGS)

ifeq ($(CONFIG_MEM_LEAK),Y)
	COMP_CFLAGS += -funwind-tables
else
	COMP_CFLAGS += -fno-asynchronous-unwind-tables
endif

COMP_ASFLAGS	:= 	-ffreestanding -Wa,--fatal-warnings		\
			-Werror -Wmissing-include-dirs			\
			-D__ASSEMBLY__ $(ASFLAGS)

COMP_CPPFLAGS	:=	$(CPPFLAGS)

$(foreach f,$(SRCS),$(eval $(call PROCESS_SRC,$(f),$(OUT_DIR)/$(TARGET_OUT)/$(subst ..,dot,$(basename $f)).o)))

$(OBJS): $(conf-file)

# Clean residues from processing
COMP_CFLAGS	:=
COMP_ASFLAGS	:=
COMP_CPPFLAGS	:=
conf-file		:=
