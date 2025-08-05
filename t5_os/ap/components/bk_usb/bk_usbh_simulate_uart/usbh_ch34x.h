/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_CH34X_H
#define USBH_CH34X_H

#include <components/cherryusb/usb_cdc.h>
#include <components/cherryusb/usb_hc.h>

#define USB_ERR_NOMEM    1
#define USB_ERR_INVAL    2
#define USB_ERR_NODEV    3
#define USB_ERR_NOTCONN  4
#define USB_ERR_NOTSUPP  5
#define USB_ERR_BUSY     6
#define USB_ERR_RANGE    7
#define USB_ERR_STALL    8
#define USB_ERR_BABBLE   9
#define USB_ERR_NAK      10
#define USB_ERR_DT       11
#define USB_ERR_IO       12
#define USB_ERR_SHUTDOWN 13
#define USB_ERR_TIMEOUT  14

/* Requests */
#define CH34X_READ_VERSION 0x5F
#define CH34X_WRITE_REG    0x9A
#define CH34X_READ_REG     0x95
#define CH34X_SERIAL_INIT  0xA1
#define CH34X_MODEM_CTRL   0xA4

// modem control bits
#define CH34X_BIT_RTS (1 << 6)
#define CH34X_BIT_DTR (1 << 5)

#define CH341_CTO_O   0x10
#define CH341_CTO_D   0x20
#define CH341_CTO_R   0x40
#define CH341_CTI_C   0x01
#define CH341_CTI_DS  0x02
#define CH341_CTRL_RI 0x04
#define CH341_CTI_DC  0x08
#define CH341_CTI_ST  0x0f

#define CH341_L_ER 0x80
#define CH341_L_ET 0x40
#define CH341_L_PS 0x38
#define CH341_L_PM 0x28
#define CH341_L_PE 0x18
#define CH341_L_PO 0x08
#define CH341_L_SB 0x04
#define CH341_L_D8 0x03
#define CH341_L_D7 0x02
#define CH341_L_D6 0x01
#define CH341_L_D5 0x00

struct usbh_ch34x {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *bulkin_ep_d;
    struct usb_endpoint_descriptor *bulkout_ep_d;
    usbh_pipe_t  bulkin;  /* Bulk IN endpoint */
    usbh_pipe_t  bulkout; /* Bulk OUT endpoint */
    struct usbh_urb bulkout_urb;
    struct usbh_urb bulkin_urb;

    struct cdc_line_coding line_coding;

    uint8_t intf;
    uint8_t minor;

    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int usbh_ch34x_set_line_coding(struct usbh_ch34x *ch34x_class, struct cdc_line_coding *line_coding);
int usbh_ch34x_get_line_coding(struct usbh_ch34x *ch34x_class, struct cdc_line_coding *line_coding);
int usbh_ch34x_set_line_state(struct usbh_ch34x *ch34x_class, bool dtr, bool rts);

int usbh_ch34x_bulk_in_transfer(struct usbh_ch34x *ch34x_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout);
int usbh_ch34x_bulk_out_transfer(struct usbh_ch34x *ch34x_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout);

void usbh_ch34x_run(struct usbh_ch34x *ch34x_class);
void usbh_ch34x_stop(struct usbh_ch34x *ch34x_class);


typedef void (*usbh_simulate_uart_con_disc_cb_t)(bool ops, void *arg);
void usbh_class_serial_ch340_register_driver();
void usbh_class_serial_ch340_register_disc_cb(usbh_simulate_uart_con_disc_cb_t callback);
#ifdef __cplusplus
}
#endif

#endif /* USBH_CH34X_H */
