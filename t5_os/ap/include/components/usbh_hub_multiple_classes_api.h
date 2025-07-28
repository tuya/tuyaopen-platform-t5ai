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

#include "usb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief     UVC connect the callback function registration function registration
 *
 * This API connect the callback:
 *   - parameter is the registered callback function.
 *   - The callback function needs to be defined.
 *
 *
 *  Notify the registered callback function when the device is connected.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_hub_port_register_connect_callback(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *connect_cb, void *arg);

/**
 * @brief     UVC disconnect the callback function registration function registration
 *
 * This API disconnect the callback:
 *   - parameter is the registered callback function.
 *   - The callback function needs to be defined.
 *
 *
 *  Notify the registered callback function when the device is disconnected.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usbh_hub_port_register_disconnect_callback(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *connect_cb, void *arg);

bk_err_t bk_usbh_hub_multiple_devices_power_on(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index);
bk_err_t bk_usbh_hub_multiple_devices_power_down(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index);

bk_err_t bk_usbh_hub_port_check_device(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index,bk_usb_hub_port_info **port_dev_info);

bk_err_t bk_usbh_hub_port_dev_open(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info);
bk_err_t bk_usbh_hub_port_dev_close(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info);

bk_err_t bk_usbh_hub_dev_request_data(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, struct usbh_urb *urb);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

