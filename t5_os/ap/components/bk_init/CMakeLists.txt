armino_build_get_property(armino_path ARMINO_PATH)
set(verify_recode_file "${armino_path}/components/bk_libs/verify_info.cmake")
function(check_verify_env_varible)
	message(STATUS "enter check_verify_env_varible")
	set(bk_verify_id)
	set(verify_record_str "")
	if(DEFINED ENV{BK_JENKINS_ID})
		message(STATUS "ENV VERIFY_ID: $ENV{BK_JENKINS_ID}")
		set(bk_verify_id $ENV{BK_JENKINS_ID})
		set(verify_record_str "${verify_record_str}set(bk_verify_id ${bk_verify_id})\r\n")
	endif()
	if (verify_record_str)
		message(STATUS "write to ${verify_recode_file}")
		message("${verify_record_str}")
		file(WRITE ${verify_recode_file} ${verify_record_str})
	endif()
endfunction(check_verify_env_varible)
macro(load_verify_info)
	if(EXISTS ${verify_recode_file})
		include(${verify_recode_file})
	endif()
	if(NOT DEFINED bk_verify_id)
		set(bk_verify_id "unknown")
	endif()
endmacro(load_verify_info)
check_verify_env_varible()
load_verify_info()
armino_build_get_property(target ARMINO_SOC)
set(incs include)
set(srcs
	bk_init.c
	components_init.c
	)

set(priv_req bk_common bk_netif driver easy_flash bk_libs bk_bluetooth bk_wifi bk_wifi_driver lwip_intf_v2_1)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

target_compile_definitions(${COMPONENT_LIB} PRIVATE
	BK_VERIFY_ID=\"${bk_verify_id}\"
)