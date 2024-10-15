#
# Platform specific make configure file
# For mps2_an495
#

# Platform path
ARM_CPU 	:= cortex-m33
PLAT_PATH	:=	hal/$(PLAT)

PLAT_CPU_ARCH	:=	-mcpu=$(ARM_CPU)+nodsp
LIBC    = ${shell ${CC} -mthumb $(PLAT_CPU_ARCH) -print-file-name=libc.a}
LIBGCC  = ${shell ${CC} -mthumb $(PLAT_CPU_ARCH) -print-libgcc-file-name}

# TODO fix me, optimize the Makefile
PLAT_INCS	:=	\
			-I$(PLAT_PATH)			\
			-I$(PLAT_PATH)/devices			\
			-I$(PLAT_PATH)/devices/flash		\
			-Idrvs/$(PLAT)/driver/flash		\
			-Idrvs/$(PLAT)/driver/efuse		\
			-Idrvs/$(PLAT)/driver/otp		\
			-Idrvs/$(PLAT)/driver/trng		\
			-Idrvs/$(PLAT)/hal/uart			\
			-Idrvs/$(PLAT)/hal/wdt			\
			-Idrvs/$(PLAT)/hal/flash		\
			-Idrvs/$(PLAT)/hal/sys			\
			-Idrvs/$(PLAT)/hal/gpio			\
			-Idrvs/$(PLAT)/hal/efuse		\
			-Idrvs/$(PLAT)/hal/otp			\
			-Idrvs/$(PLAT)/hal/trng			\
			-Idrvs/$(PLAT)/soc/			\
			-Idrvs/$(PLAT)/soc/uart/		\
			-Idrvs/$(PLAT)/soc/wdt/		\
			-Idrvs/$(PLAT)/soc/flash/		\
			-Idrvs/$(PLAT)/soc/sys/			\
			-Idrvs/$(PLAT)/soc/gpio/		\
			-Idrvs/$(PLAT)/soc/efuse		\
			-Idrvs/$(PLAT)/soc/otp			\
			-Idrvs/$(PLAT)/soc/trng			\
			-Idrvs/$(PLAT)/port/			\
			-Iboard/$(PLAT)	\
			-Iboard/$(PLAT)/layout	\

PLATFROM_SRCS	:=	\
			$(PLAT_PATH)/hal_platform.c		\
			$(PLAT_PATH)/devices/flash/flash_device.c	\
			$(PLAT_PATH)/devices/uart/uart_device.c	\
			drvs/$(PLAT)/driver/flash/flash_driver.c	\
			drvs/$(PLAT)/driver/efuse/efuse_driver.c	\
			drvs/$(PLAT)/driver/otp/otp_driver_v1_1.c	\
			drvs/$(PLAT)/driver/trng/trng_driver.c	\
			drvs/$(PLAT)/hal/uart/uart_hal.c	\
			drvs/$(PLAT)/hal/wdt/wdt_hal.c	\
			drvs/$(PLAT)/hal/gpio/gpio_hal.c	\
			drvs/$(PLAT)/hal/sys/sys_hal.c	\
			drvs/$(PLAT)/hal/flash/flash_hal.c	\
			drvs/$(PLAT)/hal/efuse/efuse_hal.c	\
			drvs/$(PLAT)/hal/otp/otp_hal.c	\
			drvs/$(PLAT)/hal/trng/trng_hal.c	\

LINKSCRIPT	:=	$(PLAT_PATH)/link/link.ld.S
