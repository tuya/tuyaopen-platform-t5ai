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

#include <common/bk_include.h>
#include "sys_driver.h"
#include "clock_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
#include <modules/pm.h>
#include "bk_misc.h"
#include <driver/uac.h>

#include <components/usbh_hub_multiple_classes_api.h>


#define AUD_UAC_DRV_TAG "aud_uac_drv"

#define LOGD(...) BK_LOGD(AUD_UAC_DRV_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_UAC_DRV_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_UAC_DRV_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(AUD_UAC_DRV_TAG, ##__VA_ARGS__)


bk_err_t bk_aud_uac_register_disconnect_cb(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *disconnect_cb)
{
#if CONFIG_USBH_UAC
	return bk_usbh_hub_port_register_disconnect_callback(port_index, class_dev_index, disconnect_cb, NULL);
#else
	return BK_OK;
#endif
}

bk_err_t bk_aud_uac_register_connect_cb(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index, void *connect_cb)
{
#if CONFIG_USBH_UAC
	return bk_usbh_hub_port_register_connect_callback(port_index, class_dev_index, connect_cb, NULL);
#else
	return BK_OK;
#endif
}

bk_err_t bk_aud_uac_power_on(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_multiple_devices_power_on(mode, port_index, class_dev_index);

#endif

	return BK_OK;
}

bk_err_t bk_aud_uac_power_down(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_multiple_devices_power_down(mode, port_index, class_dev_index);

#else

	return BK_OK;
#endif
}

bk_err_t bk_aud_uac_hub_port_check_device(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info **port_dev_info)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_port_check_device(port_index, device_index, port_dev_info);

#else

	return BK_OK;
#endif
}


bk_err_t bk_aud_uac_hub_port_dev_open(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_port_dev_open(port_index, device_index, port_dev_info);

#else

	return BK_OK;
#endif
}

bk_err_t bk_aud_uac_hub_port_dev_close(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_port_dev_close(port_index, device_index, port_dev_info);

#else

	return BK_OK;
#endif
}

bk_err_t bk_aud_uac_hub_dev_request_data(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, struct usbh_urb *urb)
{
#if CONFIG_USBH_UAC

	return bk_usbh_hub_dev_request_data(port_index, device_index, urb);

#else

	return BK_OK;
#endif
}

