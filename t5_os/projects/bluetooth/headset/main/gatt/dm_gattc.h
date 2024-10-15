#pragma once

#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"

int dm_gattc_main(void);
int32_t dm_gattc_connect(uint8_t *addr);
int32_t dm_gattc_disconnect(uint8_t *addr);
int32_t dm_gattc_discover(uint16_t conn_id);
int32_t dm_gattc_write(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len);
