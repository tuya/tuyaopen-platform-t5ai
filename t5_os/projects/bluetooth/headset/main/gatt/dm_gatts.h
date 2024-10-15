#pragma once

#include "components/bluetooth/bk_dm_gatts.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"

int dm_gatts_main(void);
int32_t dm_gatts_disconnect(uint8_t *addr);
int32_t dm_gatts_start_adv(void);
int32_t dm_gatts_enable_service(uint32_t index, uint8_t enable);


