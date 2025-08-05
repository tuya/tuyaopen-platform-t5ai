#pragma once

#include <stdint.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"


#define BT_STORAGE_KEY "bluetooth_user_info"

#define BT_LINKKEY_MAX_SAVE_COUNT 3

typedef struct __attribute__((packed))
{
    //    char addr[6 * 2 + 5 + 1];
    //    char link_key[16 * 2 + 1];
    uint8_t addr[6];
    uint8_t link_key[16];
    uint8_t a2dp_volume;
} bt_user_storage_elem_linkkey_t;


typedef struct __attribute__((packed))
{
    bt_user_storage_elem_linkkey_t linkkey[BT_LINKKEY_MAX_SAVE_COUNT];
#if CONFIG_BLE
    bk_ble_bond_dev_t ble_key[BT_LINKKEY_MAX_SAVE_COUNT];
    bk_ble_local_keys_t local_keys;
#endif
} bt_user_storage_t;


int32_t bluetooth_storage_init(void);
int32_t bluetooth_storage_deinit(void);

int32_t bluetooth_storage_find_linkkey_info_index(uint8_t *addr, uint8_t *key);
int32_t bluetooth_storage_save_linkkey_info(uint8_t *addr, uint8_t *key);
int32_t bluetooth_storage_del_linkkey_info(uint8_t *addr);
int32_t bluetooth_storage_update_to_newest(uint8_t *addr);
int32_t bluetooth_storage_get_newest_linkkey_info(uint8_t *addr, uint8_t *key);
int32_t bluetooth_storage_clean_linkkey_info(void);
int32_t bluetooth_storage_sync_to_flash(void);

int32_t bluetooth_storage_linkkey_debug(void);
int32_t bluetooth_storage_find_volume_by_addr(uint8_t *addr, uint8_t *volume);
int32_t bluetooth_storage_save_volume(uint8_t *addr, uint8_t volume);

#if CONFIG_BLE
int32_t bluetooth_storage_save_ble_key_info(bk_ble_bond_dev_t *list, uint32_t count);
int32_t bluetooth_storage_clean_ble_key_info(void);
int32_t bluetooth_storage_read_ble_key_info(bk_ble_bond_dev_t *list, uint32_t *count);

int32_t bluetooth_storage_save_local_key(bk_ble_local_keys_t *key);
int32_t bluetooth_storage_clean_local_key(void);
int32_t bluetooth_storage_read_local_key(bk_ble_local_keys_t *key);
#endif
