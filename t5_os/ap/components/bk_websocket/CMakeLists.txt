set(incs include include)
set(srcs)

if(CONFIG_WEBSOCKET)
	list(APPEND srcs
		bk_websocket_client.c
		http_parser.c)
		if(CONFIG_WEBSOCKET_TLS)
			list(APPEND srcs
				bk_ssl.c
				http_header.c
				http_utils.c
				bk_rom_md5.c
			)
		endif()
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES psa_mbedtls bk_wifi bk_common lwip_intf_v2_1
	)
