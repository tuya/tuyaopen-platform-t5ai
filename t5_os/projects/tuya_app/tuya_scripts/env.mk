#
# Makefile
# cc, 2023-06-29 20:38
#

include ${TUYA_PROJECT_DIR}/apps/${APP_BIN_NAME}/components.mk

VENDOR_DIR := $(dir $(lastword $(MAKEFILE_LIST)))../../../..

TUYA_COMPONENTS_DESC=${VENDOR_DIR}/t5_os/projects/tuya_app/.tmp_component_desc
tuya_components:
	@ echo "Automatically generated file. DO NOT EDIT." > ${TUYA_COMPONENTS_DESC};
	@ echo "COMPONENT apps/$${APP}" >> ${TUYA_COMPONENTS_DESC};
	@ for c in ${COMPONENTS}; do \
		echo "COMPONENT ${COMPONENTS_PATH}/$${c}" >> ${TUYA_COMPONENTS_DESC}; \
		done
	@ for c in ${COMPONENTS_LIB}; do \
		echo "COMPONENT_LIB ${COMPONENTS_PATH}/$${c}" >> ${TUYA_COMPONENTS_DESC}; \
		done
	@ cat ${TUYA_COMPONENTS_DESC}

tuya_lwip_dir = ${VENDOR_DIR}/tuyaos/tuya_os_adapter/lwip_intf_v2_1
tuya_lwip_cmake = CMakeLists.txt
tuya_lwip:
	@ if [ ! -d ${tuya_lwip_dir} ]; then mkdir -p ${tuya_lwip_dir}; echo "mkdir ${tuya_lwip_dir}"; fi
	@ echo "" > ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "set(incs)" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "set(srcs)" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "list(APPEND incs" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/base/include" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/lwip" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/netif" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/compat" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/vendor/bk7236/tuyaos/tuya_os_adapter/include/lwip" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/vendor/bk7236/tuyaos/tuya_os_adapter/bk_extension/include" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/lwip_intf_v2_1/dhcpd" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/bk_netif/include" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo ")" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "list(APPEND srcs" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../src/driver/net.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/bk_netif/bk_netif.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/lwip_intf_v2_1/dhcpd/dhcp-server.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/lwip_intf_v2_1/dhcpd/dhcp-server-main.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/lwip_intf_v2_1/lwip-2.1.2/port/ethernetif.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    ../../../bk7236_os/armino/components/lwip_intf_v2_1/lwip-2.1.2/src/apps/mdns/mdns.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/vendor/bk7236/tuyaos/tuya_os_adapter/bk_extension/src/ping.c" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    )" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "armino_component_register(" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    SRCS \"\$${srcs}\"" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    INCLUDE_DIRS \"\$${incs}\"" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    REQUIRES driver" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo "    PRIV_REQUIRES bk_common bk_wifi tuya_os_adapter" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ echo ")" >> ${tuya_lwip_dir}/${tuya_lwip_cmake}
	@ cd -

# vim:ft=make
#
