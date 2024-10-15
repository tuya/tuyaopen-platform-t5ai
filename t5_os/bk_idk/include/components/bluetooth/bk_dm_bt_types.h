// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDE_MODULES_BK_DM_BT_TYPES_H_
#define INCLUDE_MODULES_BK_DM_BT_TYPES_H_


/// Discoverability and Connectability mode
typedef enum {
    BK_BT_NON_CONNECTABLE,             /*!< Non-connectable */
    BK_BT_CONNECTABLE,                 /*!< Connectable */
} bk_bt_conn_mode_t;

typedef enum {
    BK_BT_NON_DISCOVERABLE,            /*!< Non-discoverable */
    BK_BT_DISCOVERABLE,                /*!< Discoverable */
} bk_bt_disc_mode_t;

/*
 * @brief common class of device
 */
typedef enum
{
    COD_PHONE    = 0x40020C,
    COD_SOUNDBAR = 0x240414,
    COD_HEADSET  = 0x240404,
} common_cod_t;

/*
 * @brief used in bk_bt_gap_set_event_callback, this enum show as "event", you must analyse param in same time
 */
typedef enum
{
    /// inquiry result, not used now
    BK_DM_BT_EVENT_INQUIRY_RESULT,

    /// disconnect completed, not used now
    BK_DM_BT_EVENT_DISCONNECT,

    //BK_DM_BT_EVENT_CMD_COMPLETE,

    /// connection completed, not used now
    BK_DM_BT_EVENT_CONNECTION_COMPLETE,

    /// recv cb when pair success and get link key, param bk_bt_linkkey_storage_t
    BK_DM_BT_EVENT_LINKKEY_NOTIFY,

    /// recv cb when peer need user to input link key, param bd_addr_t
    BK_DM_BT_EVENT_LINKKEY_REQ,

} bt_event_enum_t;





/**
 * @brief bt link key storage
 */
typedef struct
{
    /// this struct size
    uint16_t size;

    /// bt addr
    //bd_addr_t addr;
    uint8_t addr[6];

    /// bt link key
    uint8_t link_key[16];//BT_LINK_KEY_SIZE];

}__attribute__((packed)) bk_bt_linkkey_storage_t;//SM_DEVICE_ENTITY



/**
 * @brief for async dm_bt api event.
 *
 * bt event report.
 *
 * @param
 * - event: event id. see bt_event_enum_t
 * - param: param
 *
**/
typedef uint32_t (*bt_event_cb_t)(bt_event_enum_t event, void *param);

#endif
