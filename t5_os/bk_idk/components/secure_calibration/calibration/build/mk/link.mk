# Input
#
# SRCS			tells the source files and
# BINARY		tells the name of the binary
# TARGET_OUT	target out dir
# LDSCRIPT		tells the link script source file
# DEPADD		tells external dependency of binary
#
# CFLAGS		optional
# ASFLAGS		optional
# LDFLAGS		optional
#
# Output
#
# updated CLEANFILES and
# updated LIBNAMES, LIBDIRS, LIBNAMES and LIBDEPS

TARGET	:= $(BINARY)
include $(mk-dir)/compile.mk

BIN_OUT	:= $(OUT_DIR)/$(TARGET_OUT)

LNK_LDFLAGS	:=	-nostartfiles -nodefaultlibs -nostdlib	\
			-Os --gc-sections $(LDFLAGS)

$(eval $(call PROCESS_BIN,$(BINARY),$(OBJS),$(LDSCRIPT)))

# Clean residues from processing
SRCS		:=
OBJS		:=
TARGET		:=
TARGET_OUT	:=
DEPAPP 		:=
LNK_LDFLAGS	:=
DEPLIB 		:=
