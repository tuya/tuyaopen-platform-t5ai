
# can override this in current mk
ENABLE_THUMB?=true

ifeq ($(ARM_CPU),cortex-m3)
ENABLE_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-m33)
ENABLE_THUMB := true
SUBARCH := arm-m
endif

THUMBCFLAGS :=
THUMBINTERWORK :=
ifeq ($(ENABLE_THUMB),true)
THUMBCFLAGS := -mthumb -D__thumb__
THUMBINTERWORK := -mthumb-interwork
endif

ifeq ($(SUBARCH),arm)
ARCH_SRCS :=
endif


ifeq ($(SUBARCH),arm-m)
ARCH_SRCS := \
	arch/arm-m/vector.c \
	arch/arm-m/jump.c \
	arch/arm-m/int.c \

endif

