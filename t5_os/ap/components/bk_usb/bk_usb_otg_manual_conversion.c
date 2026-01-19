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

#include "os/os.h"
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/system.h>
#include <components/usb.h>
#include <components/usb_types.h>

#define USB_OTG_TAG "usb_otg"

#define USB_OTG_LOGI(...) BK_LOGI(USB_OTG_TAG, ##__VA_ARGS__)
#define USB_OTG_LOGW(...) BK_LOGW(USB_OTG_TAG, ##__VA_ARGS__)
#define USB_OTG_LOGE(...) BK_LOGE(USB_OTG_TAG, ##__VA_ARGS__)
#define USB_OTG_LOGD(...) BK_LOGD(USB_OTG_TAG, ##__VA_ARGS__)

extern int msc_storage_init(void);
extern int msc_storage_deinit(void);

static bk_err_t bk_usb_otg_close_mod(E_USB_MODE close_mod)
{
	int ret = BK_OK;
	
	switch (close_mod) {
		case USB_HOST_MODE:
			ret = bk_usb_close();
			if(ret == BK_ERR_USB_NOT_OPEN){
				ret = BK_OK;
			}
			break;
		case USB_DEVICE_MODE:
#if CONFIG_USBD_MSC
			ret = msc_storage_deinit();
#endif
			break;
		default:
			ret = BK_FAIL;
			break;
	}

	return ret;
}

static bk_err_t bk_usb_otg_open_mod(E_USB_MODE open_mod)
{
	int ret = BK_OK;

	switch (open_mod) {
		case USB_HOST_MODE:
			ret =  bk_usb_open(USB_HOST_MODE);
			if(ret == BK_ERR_USB_NOT_CLOSE){
				ret = BK_OK;
			}
			break;
		case USB_DEVICE_MODE:
#if CONFIG_USBD_MSC
			ret = msc_storage_init();
#endif
			break;
		default:
			ret = BK_FAIL;
			break;
	}

	return ret;
}

bk_err_t bk_usb_otg_manual_convers_mod(E_USB_MODE close_mod, E_USB_MODE open_mod)
{
	if(close_mod == open_mod){
		USB_OTG_LOGE("Please Check Param!\r\n");
		return BK_FAIL;
	}

	int ret_close = bk_usb_otg_close_mod(close_mod);
	int ret_open = bk_usb_otg_open_mod(open_mod);
	
	if((ret_close != BK_OK) || (ret_open != BK_OK)) {
		USB_OTG_LOGE("%s Fail! close_mod:%d open_mod:%d\r\n", __func__, close_mod, open_mod);
		return BK_FAIL;
	}

	return BK_OK;
}

