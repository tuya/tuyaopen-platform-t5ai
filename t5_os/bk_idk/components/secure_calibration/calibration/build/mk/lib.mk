# Input
#
# SRCS			tells the source files and
# TARGET_OUT	target out dir
# LIBNAME		tells the name of the lib
# LIBDIR		tells directory of lib
# LIBINCS		tells the includes for building lib
# DEPADD		tells external dependency of lib
# DEPLIB		tells external dependency lib
#
# Output
#
# updated CLEANFILES and
# updated LIBNAMES, LIBDIRS, LIBNAMES and LIBDEPS

TARGET	:= $(LIBNAME)
CFLAGS_$(LIBNAME)	+=	$(LIBINCS)
ASFLAGS_$(LIBNAME)	+=	$(LIBINCS)

include $(mk-dir)/compile.mk

$(eval $(call PROCESS_LIB,$(LIBNAME),$(OBJS),$(LIB_OUT_FILE)))

# Clean residues from processing
LIBNAME		:=
SRCS		:=
OBJS		:=
TARGET		:=
TARGET_OUT	:=
LIB_OUT_FILE:=
DEPLIB		:=

