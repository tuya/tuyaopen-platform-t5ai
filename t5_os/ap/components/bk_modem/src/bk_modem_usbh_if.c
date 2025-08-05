/**
 ****************************************************************************************
 *
 * @file bk_modem_usbh_if.c
 *
 * @brief USB host interface file.
 *
 ****************************************************************************************
 */
#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_usbh_if.h"
#include "bk_usb_cdc_modem.h"

static BK_MODEM_USB_STATE_T g_modem_usb_state = MODEM_USB_IDLE;

void bk_modem_usbh_conn_ind(uint32_t cnt)
{
	BK_MODEM_LOGI("[+]%s, %d\n", __func__, cnt);
	if (g_modem_usb_state != MODEM_USB_CONN)
	{
		bk_modem_send_msg(MSG_MODEM_CONN_IND, cnt,0,0);
		g_modem_usb_state = MODEM_USB_CONN;
	}
}

void bk_modem_usbh_disconn_ind(void)
{
	if (g_modem_usb_state != MODEM_USB_DISCONN)
	{
		BK_MODEM_LOGI("[+]%s\n", __func__);
		bk_modem_send_msg(MSG_MODEM_DISC_IND, 0,0,0);
		g_modem_usb_state = MODEM_USB_DISCONN;
	}
}

void bk_modem_usbh_close(void)
{
	bk_usb_cdc_close();
	g_modem_usb_state = MODEM_USB_IDLE;
}

void bk_modem_usbh_bulkout_ind(char *p_tx, uint32_t l_tx)
{
	bk_cdc_acm_modem_write(p_tx, l_tx);
}

void bk_modem_usbh_bulkin_ind(uint8_t *p_rx, uint32_t l_rx)
{
	bk_modem_dte_recv_data(l_rx, (uint8_t *)p_rx);
}


void bk_modem_usbh_poweron_ind(void)
{
	bk_usb_cdc_modem();
	bk_usb_cdc_open();
}

const bk_modem_usbh_if_t bk_modem_usbh_if = {
    .bk_modem_usbh_conn_ind = bk_modem_usbh_conn_ind,
    .bk_modem_usbh_disconn_ind = bk_modem_usbh_disconn_ind,
    .bk_modem_usbh_bulkin_ind = bk_modem_usbh_bulkin_ind,
};