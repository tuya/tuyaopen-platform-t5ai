list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

set(PLATFORM_PUBINC_2 
    ${PLATFORM_PATH}/t5_os/ap/include
    ${PLATFORM_PATH}/t5_os/build/bk7258/tuya_app/config
    ${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})