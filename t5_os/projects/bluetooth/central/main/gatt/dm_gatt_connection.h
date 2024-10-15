#pragma once

#define GATT_MAX_CONNECTION_COUNT 7

enum
{
    GAP_CONNECT_STATUS_IDLE,
    GAP_CONNECT_STATUS_CONNECTING,
    GAP_CONNECT_STATUS_CONNECTED,
    GAP_CONNECT_STATUS_DISCONNECTING,
};

typedef struct
{
    bk_bd_addr_t addr;
    bk_ble_addr_type_t addr_type;
    uint16_t conn_id;
    uint8_t status; //see GAP_CONNECT_STATUS_IDLE
    uint8_t local_is_master;
    uint8_t is_authen;

    uint32_t data_len;
    uint8_t *data;
} dm_gatt_app_env_t;

typedef struct
{
    //for server
    uint8_t notify_status; //0 disable; 1 notify; 2 indicate

    //for client
    uint8_t job_status; //see GATTC_STATUS_IDLE
    uint8_t noti_indica_switch;
    uint8_t noti_indicate_recv_count;
} dm_gatt_addition_app_env_t;

dm_gatt_app_env_t *dm_ble_alloc_app_env_by_addr(uint8_t *addr, uint32_t data_len);
dm_gatt_app_env_t *dm_ble_find_app_env_by_addr(uint8_t *addr);
dm_gatt_app_env_t *dm_ble_find_app_env_by_conn_id(uint16_t conn_id);
uint8_t dm_ble_del_app_env_by_addr(uint8_t *addr);
uint8_t dm_ble_app_env_foreach( int32_t (*func) (dm_gatt_app_env_t *env, void *arg), void *arg );
