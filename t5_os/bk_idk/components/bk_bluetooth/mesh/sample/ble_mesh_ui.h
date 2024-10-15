#pragma once

#include <stdint.h>

#define BLE_ZEPHRY_MESH_IS_PROVISIONER 1


#ifdef __GNUC__
#define OUR_COMPLIER GCC
#elif define __clang__
#define OUR_COMPLIER clang
#endif

typedef struct
{
    uint32_t msg_id;
    uint8_t type;
    uint16_t len;
    void *data;
    void *data2;
} BLE_MESH_MSG_T;

typedef enum
{
    BLE_MESH_MSG_FUNC,
}BLE_MESH_MSG_ENUM;

typedef int32_t (*ble_mesh_msg_func)(void *arg);



void zephyr_ble_mesh_init(void *arg);
uint32_t zephyr_ble_mesh_push_msg(uint32_t msg_id, void *data, uint32_t len, void *data2, uint32_t len2);

