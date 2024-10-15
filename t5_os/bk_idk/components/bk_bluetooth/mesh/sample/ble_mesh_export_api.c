#include "ble_mesh_ui.h"
#include <stdint.h>

uint8_t bk_ble_mesh_is_support(void)
{
#if CONFIG_BLE_MESH_ZEPHYR
    return 1;
#else
    return 0;
#endif
}

