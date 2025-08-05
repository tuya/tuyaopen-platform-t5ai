#include <common/bk_include.h>
#include <driver/mailbox_channel.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <components/cherryusb/usbh_hub.h>
#include <components/cherryusb/usbh_audio.h>
#include <components/cherryusb/usbh_video.h>
#include "usb_driver.h"

#if (CONFIG_USB_CDC)
#include <components/cherryusb/usbh_cdc_acm.h>
#endif

#include "os/os.h"
#include "os/mem.h"

#define TAG "USB_HUB_MC"
#define USB_HUB_MD_LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define USB_HUB_MD_LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define USB_HUB_MD_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define USB_HUB_MD_LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define USB_HUB_MD_LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


