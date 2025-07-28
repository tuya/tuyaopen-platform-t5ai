#pragma once

#include <stdint.h>
#include <stdbool.h>


#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_gatt_common_v1 GATT COMMON API
 * @{
 */


/**
 * @defgroup dm_gatt_common_types_v1 Defines
 * @{
 */



/// @}




/**
 * @defgroup dm_gatt_common_functions_v1 Functions
 * @{
 */

/**
 * @brief           This function is called to set local MTU,
 *                  the function is called before BLE connection.
 *
 * @param[in]       mtu: the size of MTU. Max is 517, min is 23
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gatt_set_local_mtu (uint16_t mtu);

/**
 * @brief           This function is use to get hci_handle from gatt conn id,
 *
 * @param[in]       gatt_conn_id: id from gatts_connect_evt_param/gattc_connect_evt_param
 * @param[in|out]   hci_handle: output hci handle
 * @attention       1. This function only could be used when connection exist.
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 */
ble_err_t bk_ble_get_hci_handle_from_gatt_conn_id (uint16_t gatt_conn_id, uint16_t *hci_handle);

/**
 * @brief           This function is use to get gatt conn id from hci_handle,
 *
 * @param[in]       hci_handle: hci handle from BK_BLE_GAP_CONNECT_COMPLETE_EVT
 * @param[in|out]   gatt_conn_id: output gatt conn id
 * @attention       1. This function only could be used when connection exist.
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 */
ble_err_t bk_ble_get_gatt_conn_id_from_hci_handle (uint16_t hci_handle, uint16_t *gatt_conn_id);


/// @}

/// @}

#ifdef __cplusplus
}
#endif

