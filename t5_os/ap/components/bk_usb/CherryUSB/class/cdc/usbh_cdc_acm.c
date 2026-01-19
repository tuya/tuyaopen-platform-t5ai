/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <components/cherryusb/usbh_core.h>
#include <components/cherryusb/usbh_cdc_acm.h>
#include "usb_errno.h"
/* interface descriptor field offsets */
#define DESC_bLength                 0 /** Length offset */
#define DESC_bDescriptorType         1 /** Descriptor type offset */
#define INTF_DESC_bInterfaceNumber   2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting  3 /** Alternate setting offset */
#define INTF_DESC_bNumEndpoints      4
#define INTF_DESC_bInterfaceClass    5 /** Interface class offset */
#define INTF_DESC_bInterfaceSubClass 6 /** Interface subclass offset */
#define INTF_DESC_iInterface         8
#define INTF_ASSOC_iFUNCTION         7 /** IAD iFunction offset */
#define DEV_FORMAT "/dev/ttyACM%d"
static uint32_t g_devinuse = 0;
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct cdc_line_coding g_cdc_line_coding;
static int usbh_cdc_acm_devno_alloc(struct usbh_cdc_acm *cdc_acm_class)
{
    int devno;
    for (devno = 0; devno < 32; devno++) {
        uint32_t bitno = 1 << devno;
        if ((g_devinuse & bitno) == 0) {
            g_devinuse |= bitno;
            cdc_acm_class->minor = devno;
            return 0;
        }
    }
    return -EMFILE;
}
static void usbh_cdc_acm_devno_free(struct usbh_cdc_acm *cdc_acm_class)
{
    int devno = cdc_acm_class->minor;
    if (devno >= 0 && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
}
int usbh_cdc_acm_set_line_coding(struct usbh_cdc_acm *cdc_acm_class, struct cdc_line_coding *line_coding)
{
    struct usb_setup_packet *setup = &cdc_acm_class->hport->setup;
    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = CDC_REQUEST_SET_LINE_CODING;
    setup->wValue = 0;
    setup->wIndex = cdc_acm_class->intf;
    setup->wLength = 7;
    memcpy((uint8_t *)&g_cdc_line_coding, line_coding, sizeof(struct cdc_line_coding));
    return usbh_control_transfer(cdc_acm_class->hport->ep0, setup, (uint8_t *)&g_cdc_line_coding);
}
int usbh_cdc_acm_get_line_coding(struct usbh_cdc_acm *cdc_acm_class, struct cdc_line_coding *line_coding)
{
    struct usb_setup_packet *setup = &cdc_acm_class->hport->setup;
    int ret;
    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = CDC_REQUEST_GET_LINE_CODING;
    setup->wValue = 0;
    setup->wIndex = cdc_acm_class->intf;
    setup->wLength = 7;
    ret = usbh_control_transfer(cdc_acm_class->hport->ep0, setup, (uint8_t *)&g_cdc_line_coding);
    if (ret < 0) {
        return ret;
    }
    memcpy(line_coding, (uint8_t *)&g_cdc_line_coding, sizeof(struct cdc_line_coding));
    return ret;
}
int usbh_cdc_acm_set_line_state(struct usbh_cdc_acm *cdc_acm_class, bool dtr, bool rts)
{
    struct usb_setup_packet *setup = &cdc_acm_class->hport->setup;
    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = CDC_REQUEST_SET_CONTROL_LINE_STATE;
    setup->wValue = (dtr << 0) | (rts << 1);
    setup->wIndex = cdc_acm_class->intf;
    setup->wLength = 0;
    cdc_acm_class->dtr = dtr;
    cdc_acm_class->rts = rts;
    return usbh_control_transfer(cdc_acm_class->hport->ep0, setup, NULL);
}
static void usbh_cdc_get_string(uint8_t *str, char *p)
{
	uint8_t string[64 + 1] = { 0 };
	int len, i = 2, j = 0;
	len = str[0];
//	USB_LOG_RAW("++++Len : %d\r\n", len);
	while (i < len) {
		string[j] = str[i];
		i += 2;
		j++;
	}
	os_memcpy(p, &string[0], os_strlen((char *)&string[0]));
//	USB_LOG_RAW("%s---%d\r\n", string, os_strlen((char *)&string[0]));
}
static void usbh_cdc_acm_match_funtion(struct usbh_cdc_acm *cdc_acm_class, char *p)
{
	if (os_strcasecmp("ppp", p) == 0)    //case-insensitive
		cdc_acm_class->function = USBH_CDC_FUNCTION_PPP;
	else if (os_strcasecmp("at", p) == 0)
		cdc_acm_class->function = USBH_CDC_FUNCTION_AT;
	else
		USB_LOG_WRN("Need Match New Function Pattern!\r\n");
}
static int usbh_cdc_acm_connect(struct usbh_hubport *hport, uint8_t intf)
{
	uint8_t __maybe_unused cur_ifaceNum    = 0xFF;
	uint8_t __maybe_unused cur_alt_setting = 0xFF;
	uint8_t __maybe_unused cur_iClass      = 0xFF;
	uint8_t __maybe_unused cur_nep         = 0;
	uint8_t __maybe_unused cur_iInterface  = 0;
	uint8_t idx = 0;
    int ret = 0;
    struct usbh_cdc_acm *cdc_acm_class = usb_malloc(sizeof(struct usbh_cdc_acm));
    if (cdc_acm_class == NULL) {
        USB_LOG_ERR("Fail to alloc cdc_acm_class\r\n");
        return -ENOMEM;
    }
    memset(cdc_acm_class, 0x00, sizeof(struct usbh_cdc_acm));
    usbh_cdc_acm_devno_alloc(cdc_acm_class);
    cdc_acm_class->hport = hport;
    cdc_acm_class->intf = intf;
    hport->config.intf[intf].priv = cdc_acm_class;
#if 1
    cdc_acm_class->linecoding.dwDTERate = 115200;
    cdc_acm_class->linecoding.bDataBits = 8;
    cdc_acm_class->linecoding.bParityType = 0;
    cdc_acm_class->linecoding.bCharFormat = 0;
    ret = usbh_cdc_acm_set_line_coding(cdc_acm_class, &cdc_acm_class->linecoding);
    if (ret < 0) {
        USB_LOG_ERR("Fail to set linecoding\r\n");
        return ret;
    }
    ret = usbh_cdc_acm_set_line_state(cdc_acm_class, true, true);
    if (ret < 0) {
        USB_LOG_ERR("Fail to set line state\r\n");
        return ret;
    }
#endif
#ifdef CONFIG_USBHOST_CDC_ACM_NOTIFY
    ep_desc = &hport->config.intf[intf].altsetting[0].ep[0].ep_desc;
    ep_cfg.ep_addr = ep_desc->bEndpointAddress;
    ep_cfg.ep_type = ep_desc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
    ep_cfg.ep_mps = ep_desc->wMaxPacketSize;
    ep_cfg.ep_interval = ep_desc->bInterval;
    ep_cfg.hport = hport;
    usbh_pipe_alloc(&cdc_acm_class->intin, &ep_cfg);
#endif
	uint8_t *p = hport->raw_config_desc;
	while (p[DESC_bLength]) {
		switch (p[DESC_bDescriptorType]) {
			case USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION:
				break;
			case USB_DESCRIPTOR_TYPE_INTERFACE:
				cur_ifaceNum    = p[INTF_DESC_bInterfaceNumber];   // current interface number
				cur_alt_setting = p[INTF_DESC_bAlternateSetting];
				cur_iClass      = p[INTF_DESC_bInterfaceClass];    // current interface class
				cur_nep         = p[INTF_DESC_bNumEndpoints];
				cur_iInterface  = p[INTF_DESC_iInterface];
				break;
			case USB_DESCRIPTOR_TYPE_ENDPOINT:
				if (cur_ifaceNum == cdc_acm_class->intf)
				{
					os_memcpy(&hport->config.intf[cur_ifaceNum].altsetting[0].ep[idx].ep_desc, &p[DESC_bLength], p[DESC_bLength]);
					idx++;
				}
				break;
			default:
				break;
		}
		p += p[DESC_bLength]; /* skip to next descriptor */
	}
#if 0
    struct usb_endpoint_descriptor * ep_desc;
    for (uint8_t i = 0; i < hport->config.intf[intf].altsetting[0].intf_desc.bNumEndpoints; i++) {
        ep_desc = &hport->config.intf[intf].altsetting[0].ep[i].ep_desc;
        if (ep_desc->bEndpointAddress & 0x80) {
            usbh_hport_activate_epx(&cdc_acm_class->bulkin, hport, ep_desc);
        } else {
            usbh_hport_activate_epx(&cdc_acm_class->bulkout, hport, ep_desc);
        }
    }
    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, cdc_acm_class->minor);
    USB_LOG_INFO("Register CDC ACM Class:%s\r\n", hport->config.intf[intf].devname);
#endif
#if CONFIG_USB_CDC_MODEM
	extern void bk_usb_cdc_connect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class);
	bk_usb_cdc_connect_notify(hport, intf, USB_DEVICE_CLASS_CDC);
#endif
	return ret;
}
static int usbh_cdc_acm_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
#if CONFIG_USB_CDC_MODEM
	extern void bk_usb_cdc_disconnect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class);
	bk_usb_cdc_disconnect_notify(hport, intf, USB_DEVICE_CLASS_CDC);
#endif
    int ret = 0;
    struct usbh_cdc_acm *cdc_acm_class = (struct usbh_cdc_acm *)hport->config.intf[intf].priv;
    if (cdc_acm_class) {
        usbh_cdc_acm_devno_free(cdc_acm_class);
        if (cdc_acm_class->bulkin) {
            usbh_pipe_free(cdc_acm_class->bulkin);
        }
        if (cdc_acm_class->bulkout) {
            usbh_pipe_free(cdc_acm_class->bulkout);
        }
        memset(cdc_acm_class, 0, sizeof(struct usbh_cdc_acm));
        usb_free(cdc_acm_class);
        if (hport->config.intf[intf].devname[0] != '\0')
            USB_LOG_INFO("Unregister CDC ACM Class:%s\r\n", hport->config.intf[intf].devname);
    }
    return ret;
}
static int usbh_cdc_data_connect(struct usbh_hubport *hport, uint8_t intf)
{
	uint8_t __maybe_unused cur_iface       = 0xFF;
	uint8_t __maybe_unused cur_alt_setting = 0xFF;
	uint8_t __maybe_unused cur_iClass      = 0xFF;
	uint8_t __maybe_unused cur_nep         = 0;
	uint8_t __maybe_unused cur_iFunction   = 0;
	uint8_t idx = 0;
	int ret;
    struct usbh_cdc_acm *cdc_acm_class = usb_malloc(sizeof(struct usbh_cdc_acm));
    if (cdc_acm_class == NULL) {
        USB_LOG_ERR("Fail to alloc cdc_acm_class\r\n");
        return -ENOMEM;
    }
    memset(cdc_acm_class, 0x00, sizeof(struct usbh_cdc_acm));
    ret = usbh_cdc_acm_devno_alloc(cdc_acm_class);
    if (ret < 0) {
        USB_LOG_ERR("[-]%s, ret:%d\r\n", __func__, ret);
        return ret;
    }
    cdc_acm_class->hport = hport;
    cdc_acm_class->intf = intf;
    hport->config.intf[intf].priv = cdc_acm_class;
#ifdef CONFIG_USBHOST_CDC_ACM_NOTIFY   ///?????
    ep_desc = &hport->config.intf[intf].altsetting[0].ep[0].ep_desc;
    ep_cfg.ep_addr = ep_desc->bEndpointAddress;
    ep_cfg.ep_type = ep_desc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
    ep_cfg.ep_mps = ep_desc->wMaxPacketSize;
    ep_cfg.ep_interval = ep_desc->bInterval;
    ep_cfg.hport = hport;
    usbh_pipe_alloc(&cdc_acm_class->intin, &ep_cfg);
#endif
	uint8_t t_iInterface = 0;
	uint8_t *p = hport->raw_config_desc;
	while (p[DESC_bLength]) {
		switch (p[DESC_bDescriptorType]) {
			case USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION:
				cur_iFunction = p[INTF_ASSOC_iFUNCTION];
				break;
			case USB_DESCRIPTOR_TYPE_INTERFACE:
				cur_iface       = p[INTF_DESC_bInterfaceNumber];   // current interface number
				cur_alt_setting = p[INTF_DESC_bAlternateSetting];
				cur_iClass      = p[INTF_DESC_bInterfaceClass];    // current interface class
				cur_nep         = p[INTF_DESC_bNumEndpoints];
				break;
			case USB_DESCRIPTOR_TYPE_ENDPOINT:
				if (cur_iface == cdc_acm_class->intf && cur_nep!=0)
				{
					os_memcpy(&hport->config.intf[cur_iface].altsetting[0].ep[idx].ep_desc, &p[DESC_bLength], p[DESC_bLength]);
					t_iInterface = cur_iFunction;
					idx++;
				}
				break;
			default:
				break;
		}
		p += p[DESC_bLength]; /* skip to next descriptor */
	}
	if (t_iInterface != 0)
	{
		char ttt[16] = {0};
		uint8_t ep0_buffer[255];
		struct usb_setup_packet *setup = &hport->setup;
		/* Get string */
		setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_STANDARD | USB_REQUEST_RECIPIENT_DEVICE;
		setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
		setup->wValue = (uint16_t)((USB_DESCRIPTOR_TYPE_STRING << 8) | t_iInterface);
		setup->wIndex = 0x0409;
		setup->wLength = 255;
		ret = usbh_control_transfer(hport->ep0, setup, ep0_buffer);
		if (ret < 0) {
			USB_LOG_ERR("Failed to get string,errorcode:%d\r\n", ret);
#if CONFIG_USB_CDC_MODEM         
                	extern beken2_timer_t acm_count_dev_onetimer;
                	if (rtos_is_oneshot_timer_running(&acm_count_dev_onetimer))
			{
				rtos_stop_oneshot_timer(&acm_count_dev_onetimer);
			}
#endif
                    return ret;
		}
		usbh_cdc_get_string(ep0_buffer, &ttt[0]);
		usbh_cdc_acm_match_funtion(cdc_acm_class, &ttt[0]);
	}
#if 0
    struct usb_endpoint_descriptor * ep_desc;
	{
	    for (uint8_t i = 0; i < hport->config.intf[intf].altsetting[0].intf_desc.bNumEndpoints; i++) {
	        ep_desc = &hport->config.intf[intf].altsetting[0].ep[i].ep_desc;
	        if (ep_desc->bEndpointAddress & 0x80) {
	            usbh_hport_activate_epx(&cdc_acm_class->bulkin, hport, ep_desc);
	        } else {
	            usbh_hport_activate_epx(&cdc_acm_class->bulkout, hport, ep_desc);
	        }
	    }
      USB_LOG_INFO("Register CDC DATA Class:%s\r\n", hport->config.intf[intf].devname);
	}
#endif
	snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, cdc_acm_class->minor);
#if CONFIG_USB_CDC_MODEM
	extern void bk_usb_cdc_connect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class);
	bk_usb_cdc_connect_notify(hport, intf, USB_DEVICE_CLASS_CDC_DATA);
#endif
	return ret;
}
static int usbh_cdc_data_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
#if CONFIG_USB_CDC_MODEM
	extern void bk_usb_cdc_disconnect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class);
	bk_usb_cdc_disconnect_notify(hport, intf, USB_DEVICE_CLASS_CDC_DATA);
#endif
    int ret = 0;
    struct usbh_cdc_acm *cdc_acm_class = (struct usbh_cdc_acm *)hport->config.intf[intf].priv;
    if (cdc_acm_class) {
        usbh_cdc_acm_devno_free(cdc_acm_class);
        if (cdc_acm_class->bulkin) {
            usbh_pipe_free(cdc_acm_class->bulkin);
        }
        if (cdc_acm_class->bulkout) {
            usbh_pipe_free(cdc_acm_class->bulkout);
        }
        memset(cdc_acm_class, 0, sizeof(struct usbh_cdc_acm));
        usb_free(cdc_acm_class);
        if (hport->config.intf[intf].devname[0] != '\0')
            USB_LOG_INFO("Unregister CDC DATA Class:%s\r\n", hport->config.intf[intf].devname);
    }
    return ret;
}
void bk_usbh_cdc_sw_init(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class)
{
	if(!hport)
		return;
	USB_LOG_DBG("[+]%s\r\n", __func__);
	if (interface_sub_class == CDC_SUBCLASS_ACM)
	{
		usbh_cdc_acm_connect(hport, interface_num);
	}
	USB_LOG_DBG("[-]%s\r\n", __func__);
}
void bk_usbh_cdc_sw_deinit(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class)
{
	if(!hport)
		return;
	USB_LOG_DBG("[+]%s\r\n", __func__);
	if(interface_sub_class == CDC_SUBCLASS_ACM)
		usbh_cdc_acm_disconnect(hport, interface_num);
	else if (interface_sub_class == USB_DEVICE_CLASS_CDC) {
		usbh_cdc_acm_disconnect(hport, interface_num);
	} else if (interface_sub_class == USB_DEVICE_CLASS_CDC_DATA) {
		usbh_cdc_data_disconnect(hport, interface_num);
	}
	USB_LOG_DBG("[-]%s\r\n", __func__);
}
int32_t bk_usbh_cdc_sw_activate_epx(struct usbh_hubport *hport, struct usbh_cdc_acm *cdc_acm_class, uint8_t intf)
{
	int32_t ret = 0;
	uint8_t i = 0;
	struct usb_endpoint_descriptor *ep_desc;
	if (!hport || !cdc_acm_class) {
		USB_LOG_WRN("cdc activate_epx Fail, intf:%d\r\n", intf);
		return -1;
	}
	for (i = 0; i < hport->config.intf[intf].altsetting[0].intf_desc.bNumEndpoints; i++) {
		ep_desc = &hport->config.intf[intf].altsetting[0].ep[i].ep_desc;
		if (ep_desc->bmAttributes == USB_ENDPOINT_TYPE_BULK)
		{
			if (ep_desc->bEndpointAddress & 0x80) {
				ret = usbh_hport_activate_epx(&cdc_acm_class->bulkin, hport, ep_desc);
			} else {
				ret = usbh_hport_activate_epx(&cdc_acm_class->bulkout, hport, ep_desc);
			}
		}
	}
	snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, cdc_acm_class->minor);
	USB_LOG_INFO("Register CDC DATA Class:%s\r\n", hport->config.intf[intf].devname);
	return ret;
}
int32_t bk_usbh_cdc_sw_deactivate_epx(struct usbh_hubport *hport, struct usbh_cdc_acm *cdc_acm_class, uint8_t intf)
{
    int ret = 0;
    if (cdc_acm_class)
    {
        //usbh_cdc_acm_devno_free(cdc_acm_class);
        if (cdc_acm_class->bulkin) {
            usbh_pipe_free(cdc_acm_class->bulkin);
        }
        if (cdc_acm_class->bulkout) {
            usbh_pipe_free(cdc_acm_class->bulkout);
        }
        //memset(cdc_acm_class, 0, sizeof(struct usbh_cdc_acm));
        //usb_free(cdc_acm_class);
        //if (hport->config.intf[intf].devname[0] != '\0')
            //USB_LOG_INFO("Unregister CDC DATA Class:%s\r\n", hport->config.intf[intf].devname);
    }
    return ret;
}
const struct usbh_class_driver cdc_acm_class_driver = {
    .driver_name = "cdc_acm",
    .connect     = usbh_cdc_acm_connect,
    .disconnect  = usbh_cdc_acm_disconnect
};
const struct usbh_class_driver cdc_data_class_driver = {
    .driver_name = "cdc_data",
    .connect     = usbh_cdc_data_connect,
    .disconnect  = usbh_cdc_data_disconnect
};
CLASS_INFO_DEFINE const struct usbh_class_info cdc_acm_class_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_CDC,
    .subclass = CDC_ABSTRACT_CONTROL_MODEL,
    .protocol = CDC_COMMON_PROTOCOL_AT_COMMANDS,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &cdc_acm_class_driver
};
CLASS_INFO_DEFINE const struct usbh_class_info cdc_data_class_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_CDC_DATA,
    .subclass = 0x00,
    .protocol = 0x00,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &cdc_data_class_driver
};
void usbh_cdc_acm_class_register()
{
	usbh_register_class_driver(0, (void *)&cdc_acm_class_info);
}
void usbh_cdc_data_class_register()
{
	usbh_register_class_driver(0, (void *)&cdc_data_class_info);
}
int usbh_cdc_acm_bulk_in_transfer(struct usbh_cdc_acm *cdc_acm_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
	int ret;
	struct usbh_urb *urb = &cdc_acm_class->bulkin_urb;
	usbh_complete_callback_t callback = urb->complete;
	void *arg = urb->arg;
	usbh_bulk_urb_fill(urb, cdc_acm_class->bulkin, buffer, buflen, timeout, callback, NULL);
	urb->complete = callback;
	urb->arg = arg;
	ret = usbh_submit_urb(urb);
	if (ret == 0) {
		ret = urb->actual_length;
	}
	return ret;
}
int usbh_cdc_acm_bulk_out_transfer(struct usbh_cdc_acm *cdc_acm_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
	int ret;
	struct usbh_urb *urb = &cdc_acm_class->bulkout_urb;
	usbh_complete_callback_t callback = urb->complete;
	void *arg = urb->arg;
	usbh_bulk_urb_fill(urb, cdc_acm_class->bulkout, buffer, buflen, timeout, callback, NULL);
	urb->complete = callback;
	urb->arg = arg;
	ret = usbh_submit_urb(urb);
	if (ret == 0) {
		ret = urb->actual_length;
	}
	return ret;
}
