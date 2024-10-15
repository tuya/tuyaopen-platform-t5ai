armino_build_get_property(armino_soc ARMINO_SOC)
armino_component_register()

add_prebuilt_library(libmftest.a "$ENV{TUYA_PROJECT_DIR}/t5_os/extra_components/libmftest.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE libmftest.a)
