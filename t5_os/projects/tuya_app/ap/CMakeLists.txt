set(incs .)
set(srcs ap_main.c)

list(APPEND incs
    $ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/utilities/include
    $ENV{ARMINO_PATH}/../../../../include/base/include
)

set(priv_req
    media_service
    multimedia
    wifi_transfer
    avdk_utils
    lwip_intf_v2_1
    bk_wifi_driver
    bk_vfs
)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
    list(APPEND priv_req)
endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES "${priv_req}"
    REQUIRES tuyaos_adapter
)
