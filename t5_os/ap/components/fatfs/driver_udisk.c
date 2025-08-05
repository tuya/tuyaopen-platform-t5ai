#include <stdio.h>
#include <string.h>
#include <common/bk_include.h>
#include "driver_udisk.h"
#include <os/os.h>
#include "bk_uart.h"
#include "diskio.h"
#include <components/usb.h>

#if CONFIG_USBH_MSC
#if CONFIG_USB_HOST
#include <components/cherryusb/usbh_msc.h>
#endif

uint8 udisk_is_attached(void)
{
    return bk_usb_get_device_connect_status();
}

uint8 udisk_init(void)
{
	uint32 ret = USB_RET_ERROR;

	FATFS_LOGD("udisk_init\r\n");

#if CONFIG_USB_HOST
	if (usbh_ms_media_get_status()) {
		ret = USB_RET_OK;
	}
#elif CONFIG_USB_DEVICE
	{
		//MUSB_Host_init();

		while (1)
		{
			ret = MUSB_NoneRunBackground();
			FATFS_LOGD("udisk_init: ret = 0x%lx\r\n", ret);
			if (usbd_ms_media_get_status()) {
				ret = USB_RET_OK;
				break;
			} else {
				if((USB_RET_DISCONNECT == ret) || (USB_RET_ERROR == ret)) {
					rtos_delay_milliseconds(100);
					FATFS_LOGD("need plug in usb device\r\n");
					break;
				}
			}
		}
	}
#endif
	return ret;
}

int udisk_rd_blk_sync(uint32 first_block, uint32 block_num, uint8 *dest )
{
    int ret = USB_RET_ERROR;

    FATFS_LOGV("disk_rd:%d:%d\r\n", first_block, block_num);
#if CONFIG_USB_HOST
    if (!usbh_ms_media_get_status())
    {
        FATFS_LOGD("disk_rd_failed\r\n");
        return ret;
    }

    ret = usbh_device_read(first_block, dest, block_num);
#endif
    FATFS_LOGV("%s: ret=%d\r\n", __func__, ret);

    return ret;
}

int udisk_wr_blk_sync(uint32 first_block, uint32 block_num, uint8 *dest)
{
    int ret = USB_RET_ERROR;

    FATFS_LOGV("disk_wr:%d:%d\r\n", first_block, block_num);
#if CONFIG_USB_HOST
    if (!usbh_ms_media_get_status())
    {
        FATFS_LOGD("disk_wr_failed\r\n");
        return ret;
    }
    ret = usbh_device_write(first_block, dest, block_num);
#endif
    FATFS_LOGV("%s: ret=%d\r\n", __func__, ret);

    return ret;
}

uint32 udisk_get_size(void)
{
    return 0;//driver_udisk.total_block;
}

#endif

