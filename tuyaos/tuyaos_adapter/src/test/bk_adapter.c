/****************************************************************************
 * @file bk_adapter.c
 * @brief this module is used to bk_adapter
 * @version 0.0.1
 * @date 2023-06-28
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include "sdkconfig.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Security Check Interface : To check the contents of hostids
 **/
uint32_t mem_sanity_check(void *mem)
{
    return 1;
}

#if CONFIG_USB
#if CONFIG_USB_HOST

#include "usbh_core.h"
#include "usbh_hub.h"
static uint32_t usbdev_detect_flag = 0;
static uint32_t usbdev_idVendor  = 0xFFFFFFFF;
static uint32_t usbdev_idProduct = 0xFFFFFFFF;
static void usbh_enumerate_callback(void *arg)
{
    bk_printf("%s\r\n",__func__);
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *)arg;

    if (desc->bLength != USB_SIZEOF_DEVICE_DESC) {
        bk_printf("invalid device bLength 0x%02x\r\n", desc->bLength);
        return;
    } else if (desc->bDescriptorType != USB_DESCRIPTOR_TYPE_DEVICE) {
        bk_printf("unexpected device descriptor 0x%02x\r\n", desc->bDescriptorType);
        return;
    }

    os_printf("===============================================\r\n");
    os_printf("  Device Descriptor:\r\n");
    os_printf("  bLength:             0x%02x\r\n", desc->bLength);
    os_printf("  bDescriptorType:     0x%02x\r\n", desc->bDescriptorType);
    os_printf("  bcdUSB:              0x%04x\r\n", desc->bcdUSB);
    os_printf("  bDeviceClass:        0x%02x\r\n", desc->bDeviceClass);
    os_printf("  bDeviceSubClass:     0x%02x\r\n", desc->bDeviceSubClass);
    os_printf("  bDeviceProtocol:     0x%02x\r\n", desc->bDeviceProtocol);
    os_printf("  bMaxPacketSize0:     0x%02x\r\n", desc->bMaxPacketSize0);
    os_printf("  idVendor:            0x%04x\r\n", desc->idVendor);
    os_printf("  idProduct:           0x%04x\r\n", desc->idProduct);
    os_printf("  bcdDevice:           0x%04x\r\n", desc->bcdDevice);
    os_printf("  iManufacturer:       0x%02x\r\n", desc->iManufacturer);
    os_printf("  iProduct:            0x%02x\r\n", desc->iProduct);
    os_printf("  iSerialNumber:       0x%02x\r\n", desc->iSerialNumber);
    os_printf("  bNumConfigurations:  0x%02x\r\n", desc->bNumConfigurations);
    os_printf("===============================================\r\n");

    usbdev_idVendor = desc->idVendor;
    usbdev_idProduct = desc->idProduct;

    usbdev_detect_flag = 1;
}

void user_reg_usbenum_cb(void)
{
    usbh_enumerate_register_cb(usbh_enumerate_callback);
}

void tuya_get_usb_dev(uint32_t *vid, uint32_t *pid)
{
    int cnt = 10;
    if (vid == NULL || pid == NULL)
        return;

    usbdev_detect_flag = 0;
    bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
    bk_usb_open(0);

    do {
        tkl_system_sleep(100);
        *vid = usbdev_idVendor;
        *pid = usbdev_idProduct;
        cnt--;
    } while((cnt > 0) && (usbdev_detect_flag == 0));
}
#endif // CONFIG_USB
#endif // CONFIG_USB_HOST



