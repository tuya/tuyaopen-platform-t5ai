#pragma once

#define BLE_ZEPHYR_MESH_TMALL_TYPE_BEKEN 1
#define BLE_ZEPHYR_MESH_TMALL_TYPE_ESPRESSIF 2

#define BLE_ZEPHYR_MESH_TMALL_TYPE BLE_ZEPHYR_MESH_TMALL_TYPE_BEKEN

// pin25
#if CONFIG_SOC_BK7235
#define LED_DEV_ID PWM_ID_3
#elif CONFIG_SOC_BK7256
#define LED_DEV_ID PWM_ID_2
#else
#define LED_DEV_ID PWM_ID_2
#endif


void ble_mesh_tmall_spirit_sample_shell(int32_t argc, char **argv);

