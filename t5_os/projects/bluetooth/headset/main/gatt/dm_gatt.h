#pragma once

#include "dm_gatt_connection.h"
#include <stdint.h>

#define BLE_USE_STORAGE 1

#define GAP_IS_OLD_API 0

#define GATT_MAX_BOND_COUNT 7
enum
{
    GATT_DEBUG_LEVEL_ERROR,
    GATT_DEBUG_LEVEL_WARNING,
    GATT_DEBUG_LEVEL_INFO,
    GATT_DEBUG_LEVEL_DEBUG,
    GATT_DEBUG_LEVEL_VERBOSE,
};

#define SYNC_CMD_TIMEOUT_MS 4000
#define GATT_DEBUG_LEVEL GATT_DEBUG_LEVEL_INFO

#define gatt_loge(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_ERROR)   BK_LOGE("app_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logw(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_WARNING) BK_LOGW("app_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logi(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_INFO)    BK_LOGI("app_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logd(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_DEBUG)   BK_LOGI("app_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logv(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_VERBOSE) BK_LOGI("app_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


enum
{
    DM_BLE_GAP_APP_CB_RET_PROCESSED,
    DM_BLE_GAP_APP_CB_RET_NO_INTERESTING,
};





int dm_gatt_main(void);
int dm_gatt_add_gap_callback(void * cb);
int32_t dm_gatt_get_authen_status(uint8_t *nominal_addr, uint8_t *nominal_addr_type, uint8_t *identity_addr, uint8_t *identity_addr_type);
int dm_gatt_passkey_reply(uint8_t accept, uint32_t passkey);
int dm_gatt_set_security_method(uint8_t iocap, uint8_t auth_req, uint8_t key_distr);
bool dm_gatt_is_linkkey_distr_from_ltk(void);
int dm_ble_gap_create_bond(uint8_t *addr);
int dm_ble_gap_remove_bond(uint8_t *addr);
int32_t dm_ble_gap_clean_bond(void);
int dm_ble_gap_update_param(uint8_t *addr, uint16_t interval, uint16_t tout);

