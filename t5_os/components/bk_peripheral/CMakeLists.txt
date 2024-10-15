set(BK_PERIPHERAL_PATH $ENV{ARMINO_AVDK_DIR}/components/bk_peripheral)

include(${BK_PERIPHERAL_PATH}/src/lcd/rgb/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/mcu/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/qspi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/spi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/dvp/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/tp/config.cmake)

set(incs include)
set(srcs
	src/bk_peripheral.c
	src/lcd/lcd_panel_devices.c
)

list(APPEND srcs
	${RGB_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${MCU_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${QSPI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${SPI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${DVP_CAMERA_DEVICE_FILES}
)

list(APPEND srcs
	${TP_DEVICE_FILES}
)


set(priv_req bk_common)


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" unity include)
