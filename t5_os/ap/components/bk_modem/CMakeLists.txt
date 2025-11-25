set(incs)
set(srcs)

if (CONFIG_BK_MODEM)
list(APPEND incs
	include/
	)

	list(APPEND srcs
	src/bk_modem_at_cmd.c
	src/bk_modem_dce.c
	src/bk_modem_dte.c
	src/bk_modem_main.c
	src/bk_modem_netif.c
	src/bk_modem_usbh_if.c
	src/bk_modem_uart.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES driver
	PRIV_REQUIRES bk_common bk_wifi bk_rtos os_source lwip_intf_v2_1 bk_usb
	)
