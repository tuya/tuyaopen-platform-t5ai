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

#pragma once

#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern"C" {
#endif

/**
 * @defgroup dm_bluetooth BLUETOOTH API
 * @{
 */

/**
 * @brief     Get bluetooth status
 *
 * @return    Bluetooth status
 *
 */
bk_bluetooth_status_t bk_bluetooth_get_status(void);

/**
 *
 * @brief           init bluetooth.
 *
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bluetooth_init(void);

/**
 *
 * @brief           deinit bluetooth.
 *
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bluetooth_deinit(void);


/**
 *
 * @brief      Get bluetooth device address.  Must use after "bk_bluetooth_init".
 *
 * @param[out]      addr - bluetooth device address
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 */
bt_err_t bk_bluetooth_get_address(uint8_t *addr);

/**
 * @brief  register hci callback for dual mode host only
 *
 * @param
 *    - cb: hci callback function used to recv hci data from host
 *
 * @attention used for dual mode host only
 *
 * @return
 *    - BT_OK: succeed
 *    - others: other errors.
 */
bt_err_t bk_dual_host_register_hci_callback(dual_hci_to_cp_cb cb);

/**
 * @brief send hci data to host.
 *
 * @param
 * - buf: payload
 * - len: buf's len
 *
 * @attention used for dual mode host only
 *
 * @return
 * - BT_OK: succeed
**/
bt_err_t bk_dual_hci_send_to_host(uint8_t *buf, uint32_t len);
///@}

#ifdef __cplusplus
}
#endif

