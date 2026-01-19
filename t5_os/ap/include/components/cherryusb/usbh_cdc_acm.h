/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_CDC_ACM_H
#define USBH_CDC_ACM_H

#include "usb_cdc.h"
#include "usb_hc.h"


enum {
	USBH_CDC_FUNCTION_PPP = 1,
	USBH_CDC_FUNCTION_AT,
	USBH_CDC_FUNCTION_MAX,
};


struct usbh_cdc_acm {
	struct usbh_hubport *hport;

//	struct usb_endpoint_descriptor *bulkin;  /* Bulk IN endpoint */
//	struct usb_endpoint_descriptor *bulkout; /* Bulk OUT endpoint */
	bool dtr;
	bool rts;
	usbh_pipe_t bulkin;  /* Bulk IN endpoint */
	usbh_pipe_t bulkout; /* Bulk OUT endpoint */

	struct usbh_urb bulkout_urb;
	struct usbh_urb bulkin_urb;

	struct cdc_line_coding linecoding;

	uint8_t minor;

	uint8_t intf;
	uint8_t ctrl_intf; /* Control interface number */
	uint8_t data_intf; /* Data interface number */

#ifdef CONFIG_USBHOST_CDC_ACM_NOTIFY
	struct usbh_urb intin_urb;
	struct usb_endpoint_descriptor *intin;	 /* INTR IN endpoint (optional) */
#endif

	/// add function type
	uint8_t function;
};

int usbh_cdc_acm_set_line_coding(struct usbh_cdc_acm *cdc_acm_class, struct cdc_line_coding *line_coding);
int usbh_cdc_acm_get_line_coding(struct usbh_cdc_acm *cdc_acm_class, struct cdc_line_coding *line_coding);
int usbh_cdc_acm_set_line_state(struct usbh_cdc_acm *cdc_acm_class, bool dtr, bool rts);

int usbh_cdc_acm_bulk_in_transfer(struct usbh_cdc_acm *cdc_acm_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout);
int usbh_cdc_acm_bulk_out_transfer(struct usbh_cdc_acm *cdc_acm_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout);

//void usbh_cdc_acm_run(struct usbh_cdc_acm *cdc_acm_class);
//void usbh_cdc_acm_stop(struct usbh_cdc_acm *cdc_acm_class);

void bk_usbh_cdc_sw_init(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);
void bk_usbh_cdc_sw_deinit(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);
int32_t bk_usbh_cdc_sw_activate_epx(struct usbh_hubport *hport, struct usbh_cdc_acm *cdc_acm_class, uint8_t intf);
int32_t bk_usbh_cdc_sw_deactivate_epx(struct usbh_hubport *hport, struct usbh_cdc_acm *cdc_acm_class, uint8_t intf);

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif /* USBH_CDC_ACM_H */
