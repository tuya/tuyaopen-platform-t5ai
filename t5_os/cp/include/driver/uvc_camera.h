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

#include <driver/uvc_camera_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     uvc power on
 *
 * This API called by user, power on all port and register connect/disconnect cb
 *
 * @param trigger 0:not wait uvc connect, 1: wait uvc connect 4 seconds
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_power_on(uint32_t format, uint32_t timeout);

/**
 * @brief     uvc power on
 *
 * This API called by user, power off all port
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_power_off(void);

/**
 * @brief     Init the uvc
 *
 * This API open uvc
 *
 * @param open handle
 * @param config The port config
 * @param cb The frame_buffer callback
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_init(camera_handle_t *handle, uvc_config_t *config, bk_uvc_callback_t *cb);

/**
 * @brief     Deinit the uvc
 *
 * This API stop and close uvc
 *
 * @param handle The uvc open handle output
 *
 * @attation: port must 1 - other
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_deinit(camera_handle_t *handle);

/**
 * @brief     Get uvc config
 *
 * This API called by user, get port info
 *
 * @param port The port user want to know
 * @param format The image format you want to know(image_format_t)
 *
 * @return
 *    - PTR: succeed
 *    - NULL: other errors.
 */
bk_usb_hub_port_info *bk_uvc_get_enum_info(uint8_t port, uint16_t format);

/**
 * @brief     Set uvc config
 *
 * This API called by user, Set uvc support fps and resolutions and start stream
 *
 * @param handle the uvc init device handle
 * @param config the uvc need support param the user set
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_set_start(camera_handle_t *handle, uvc_config_t *config);

/**
 * @brief     Set uvc config
 *
 * This API called by user, Set stop uvc stream output
 *
 * @param handle the uvc init device handle
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_set_stop(camera_handle_t *handle);

/**
 * @brief     register uvc packet analyse callback
 *
 * This API will register uvc analyse callabck
 *
 * @param cb
 *
 * @attation 1. this api called before bk_uvc_init. if not register, analyse uvc packet by default adk function
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_uvc_register_packet_cb(void *cb);

/**
 * @brief     register uvc connect state cb
 *
 * This API will register uvc connect state cb, include uvc state change cb, and other ops error
 *
 * @param cb
 *
 * @attation 1. this api called before bk_uvc_power_on.
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_uvc_register_connect_state_cb(camera_state_cb_t cb);

/**
 * @brief     register separate packet cb
 *
 * This API will register separate packet data, some uvc device a endpoint will output mjpeg and h264 data meantime, need register this callback
 *
 * @param cb
 *
 * @attation 1. this api called before bk_uvc_init, not all uvc need register
 *
 * @return
 *    - BK_OK: set success
 *    - others: other errors.
 */
bk_err_t bk_uvc_register_separate_packet_callback(uvc_separate_config_t *cb);

#if (CONFIG_STANDARD_DUALSTREAM)

bk_err_t bk_uvc_h26x_power_on(uint32_t trigger);
bk_err_t bk_uvc_h26x_power_off(void);
bk_err_t bk_uvc_h26x_init(uvc_config_t *config, bk_uvc_callback_t *cb);
bk_err_t bk_uvc_h26x_deinit(uint8_t port);
bk_usb_hub_port_info *bk_uvc_h26x_get_enum_info(uint8_t port);

#endif

#ifdef __cplusplus
}
#endif

