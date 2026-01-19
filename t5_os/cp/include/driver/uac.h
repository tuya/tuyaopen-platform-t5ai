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
//
#pragma once
#include <components/usbh_hub_multiple_classes_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/* uac 1.0 version */
typedef enum {
	AUD_UAC_DATA_FORMAT_TYPE_UNDEFINED = 0,
	AUD_UAC_DATA_FORMAT_TYPE_PCM,
	AUD_UAC_DATA_FORMAT_TYPE_PCM8,
	AUD_UAC_DATA_FORMAT_TYPE_IEEE_FLOAT,
	AUD_UAC_DATA_FORMAT_TYPE_ALAW,
	AUD_UAC_DATA_FORMAT_TYPE_MULAW,
	AUD_UAC_DATA_FORMAT_TYPE_MAX
} aud_uac_data_format_type_t;

typedef struct {
    aud_uac_data_format_type_t mic_format_tag;
    uint32_t mic_samp_rate;
} aud_uac_mic_config_t;

typedef struct {
    aud_uac_data_format_type_t spk_format_tag;
    uint32_t spk_samp_rate;
    uint16_t spk_volume;
} aud_uac_spk_config_t;

typedef struct {
    aud_uac_mic_config_t mic_config;
    aud_uac_spk_config_t spk_config;
} aud_uac_config_t;

/**
 * @brief AUD API
 * @defgroup bk_api_aud AUD API group
 * @{
 */

bk_err_t bk_aud_uac_register_disconnect_cb(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *disconnect_cb);

bk_err_t bk_aud_uac_register_connect_cb(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *connect_cb);

bk_err_t bk_aud_uac_power_on(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index);

bk_err_t bk_aud_uac_power_down(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index);

bk_err_t bk_aud_uac_hub_port_check_device(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info **port_dev_info);

bk_err_t bk_aud_uac_hub_port_dev_open(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info);

bk_err_t bk_aud_uac_hub_port_dev_close(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info);

bk_err_t bk_aud_uac_hub_dev_request_data(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, struct usbh_urb *urb);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
