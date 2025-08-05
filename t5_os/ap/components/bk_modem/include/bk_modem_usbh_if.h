
#ifndef _BK_MODEM_USBH_IF_H_
#define _BK_MODEM_USBH_IF_H_

#include <common/bk_include.h>

typedef enum
{
	MODEM_USB_CONN,
	MODEM_USB_DISCONN,
	MODEM_USB_IDLE,
}BK_MODEM_USB_STATE_T;

void bk_modem_usbh_close(void);
void bk_modem_usbh_poweron_ind(void);
void bk_modem_usbh_conn_ind(uint32_t cnt);
void bk_modem_usbh_disconn_ind(void);
void bk_modem_usbh_bulkout_ind(char *p_tx, uint32_t l_tx);

#endif
