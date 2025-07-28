set(TP_DEVICE_FILES "")
set(TP_PATH src/tp)

if (CONFIG_TP)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_sensor_devices.c)
endif()

if (CONFIG_TP_FT6336)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_ft6336.c)
endif()

if (CONFIG_TP_GT911)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_gt911.c)
endif()

if (CONFIG_TP_GT1151)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_gt1151.c)
endif()

if (CONFIG_TP_HY4633)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_hy4633.c)
endif()

if (CONFIG_TP_CST816D)
	list(APPEND TP_DEVICE_FILES ${TP_PATH}/tp_cst816d.c)
endif()
