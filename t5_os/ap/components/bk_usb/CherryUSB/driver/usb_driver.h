#include <common/bk_include.h>
#include <components/log.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "bk_gpio.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <components/cherryusb/usb_def.h>
#include "usb_errno.h"

#ifndef _USB_DRIVER_H_
#define _USB_DRIVER_H_


#define USB_DRIVER_TAG "usb_driver"
#define USB_DRIVER_LOGI(...) BK_LOGI(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGW(...) BK_LOGW(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGE(...) BK_LOGE(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGD(...) BK_LOGD(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGV(...) BK_LOGV(USB_DRIVER_TAG, ##__VA_ARGS__)

void bk_usb_driver_task_lock_mutex();
void bk_usb_driver_task_unlock_mutex();
bk_err_t bk_usb_adaptor_init(void);

#endif
