set(incs include)
set(srcs)

if(CONFIG_HTTPS)
	list(APPEND srcs
		bk_https.c
		http_header.c
		http_parser.c
		http_utils.c
		bk_rom_md5.c
		bk_ssl.c
	)
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_wifi bk_common lwip_intf_v2_1
	PRIV_REQUIRES psa_mbedtls
	)
