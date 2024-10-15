
#TODO should move all public include dir to modules

set(incs .)
set(srcs )

if (CONFIG_LITTLEFS)
	list(APPEND srcs
		littlefs_demo/lfs_example_onchip_flash.c
	)
endif()

if (CONFIG_HOST_NONE)
	list(APPEND incs)
endif()

if (CONFIG_NTP_SYNC_RTC)
	list(APPEND srcs
	time/time.c
	time/ntp.c
	time/app_time_intf.c
	)
endif()

#TODO fix main
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_wifi bk_cli
	PRIV_REQUIRES bk_common littlefs lwip_intf_v2_1
	)
