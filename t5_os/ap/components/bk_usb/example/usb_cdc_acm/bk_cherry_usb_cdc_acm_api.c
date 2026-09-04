#include <os/os.h>
#include <os/mem.h>

#include "usb_driver.h"
#include <driver/media_types.h>
#include "bk_cherry_usb_cdc_acm_api.h"
#include <components/cherryusb/usbh_cdc_acm.h>
#include <components/cherryusb/usbh_core.h>
#include <driver/gpio.h>
#include <modules/pm.h>
#include "mb_ipc_cmd.h"
#include <driver/timer.h>
#if CONFIG_USB_CDC_MODEM
#include "bk_usb_cdc_modem.h"
#endif

static uint32_t acm_cnt = 0;

struct usbh_cdc_acm * acm_device = NULL;
struct usbh_cdc_acm * g_cdc_data_device[USB_CDC_DATA_DEV_NUM_MAX];
struct usbh_cdc_acm * g_cdc_acm_device[USB_CDC_ACM_DEV_NUM_MAX];


static uint8_t g_first_rx_urb_set = 0;
beken2_timer_t acm_count_dev_onetimer;
/*********************************************************************************************************/

#if (CONFIG_USB_DEVICE && CONFIG_USB_HOST)
extern bk_err_t bk_usb_otg_manual_convers_mod(E_USB_MODE close_mod, E_USB_MODE open_mod);
#endif

/*********************************************************************************************************/

static int32_t bk_usbh_cdc_acm_register_in_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkin_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}
static int32_t bk_usbh_cdc_acm_register_out_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkout_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}
void bk_cdc_acm_bulkin_callback(void *arg, int nbytes)
{
	if (nbytes > 0)
	{
		uint8_t *p_buf = bk_usb_cdc_modem_get_rxpuf();
		if (bk_cdc_acm_bulkin_data(p_buf, nbytes) != BK_OK)
		{
			USB_CDC_LOGE("modem rx queue full, hold bulkin\n");
			return;
		}

		acm_device->bulkin_urb.transfer_buffer_length = 0;
	}
	else if (nbytes < 0) {
		USB_CDC_LOGE("Error bulkin status, ret:%d\n", nbytes);
	}
}

void bk_cdc_acm_bulkout_callback(void *arg, int nbytes)
{
	USB_CDC_LOGV("[+]%s, nbytes:%d\r\n", __func__, nbytes);
	if (nbytes < 0)
	{
		USB_CDC_LOGE("modem tx ERROR %d\n", nbytes);
		return;
	}

	if (g_first_rx_urb_set == 0)
	{
		bk_cdc_acm_io_read();
		g_first_rx_urb_set = 1;
	} 
}

void bk_acm_trigger_rx(uint8_t *p_buf)
{
	acm_device->bulkin_urb.complete = bk_cdc_acm_bulkin_callback;
	acm_device->bulkin_urb.pipe     = acm_device->bulkin;
	acm_device->bulkin_urb.transfer_buffer = p_buf;
	acm_device->bulkin_urb.transfer_buffer_length = CDC_RX_MAX_SIZE;
	acm_device->bulkin_urb.timeout = 0;
	acm_device->bulkin_urb.actual_length = 0;
}

void bk_acm_trigger_tx(void)
{
	acm_device->bulkout_urb.complete = bk_cdc_acm_bulkout_callback;
	acm_device->bulkout_urb.pipe     = acm_device->bulkout;
	acm_device->bulkout_urb.transfer_buffer = NULL;
	acm_device->bulkout_urb.transfer_buffer_length = CDC_TX_MAX_SIZE;
	acm_device->bulkout_urb.timeout = 100;
	acm_device->bulkout_urb.actual_length = 0;
}

int32_t bk_cdc_acm_io_read(void)
{
	USB_CDC_LOGV("[+]%s\n", __func__);
	int32_t ret = 0;

	if (acm_device == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}

    	uint8_t * p_buf = bk_usb_cdc_modem_get_rxpuf();

    	bk_acm_trigger_rx(p_buf);
    	ret = usbh_cdc_acm_bulk_in_transfer(acm_device, p_buf, CDC_RX_MAX_SIZE, 0);
    	if(ret < 0)
    	{
    	    	USB_CDC_LOGV("bk_cdc_acm_io_read is TIMEOUT! ret:%d\r\n", ret);
    	}

	return ret;
}

int32_t bk_cdc_acm_io_write_data(char *p_tx, uint32_t l_tx)
{
	uint8_t *buf = NULL;
	uint32_t tx_len = 0;
	int32_t ret = 0;
	uint32_t timeout = 100;

	buf = (uint8_t *)p_tx;
	tx_len = l_tx;

	if (acm_device == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}

	bk_acm_trigger_tx();
	ret = usbh_cdc_acm_bulk_out_transfer(acm_device, buf, tx_len, timeout);
	if(ret != tx_len)
		USB_CDC_LOGE("bk_cdc_acm_io_write_data fail, ret %d\r\n", ret);

	rtos_delay_milliseconds(2);
    
	return ret;
}

static int32_t bk_usb_acm_find_ppp_dev(void)
{
	int32_t i = 0;
	for (i = 0; i < acm_cnt; i++)
	{
		if (g_cdc_data_device[i]->function == USBH_CDC_FUNCTION_PPP)
			return i;
	}
	for (i = 0; i < acm_cnt; i++)
	{
		if (g_cdc_data_device[i]->function == USBH_CDC_FUNCTION_AT) {
			USB_CDC_LOGD("Find AT dev, not ppp dev!\r\n");
			return i;
		}
	}
	return -1;
}

static void bk_usb_acm_count_dev_callback(void *data1, void *data2)
{
	int32_t idx = bk_usb_acm_find_ppp_dev();
	if (idx < 0) 
	{
		USB_CDC_LOGE("Can't find dev!!!!\r\n");
	}
	else
	{
		g_first_rx_urb_set = 0;                      		
		acm_device = g_cdc_data_device[idx];
		bk_usbh_cdc_sw_activate_epx(acm_device->hport, acm_device, acm_device->intf);
		CDC_STATUS_t dev_state;
		dev_state.dev_cnt = (acm_cnt<<16)|(idx);
		dev_state.status = CDC_STATUS_CONN;
		bk_cdc_acm_state_notify(&dev_state);
	}

}
void bk_usb_cdc_open_ind(E_USB_MODE open_usb_mode)
{
	if (open_usb_mode == USB_HOST_MODE)
	{
#if (CONFIG_USB_DEVICE && CONFIG_USB_HOST)
		bk_usb_otg_manual_convers_mod(USB_DEVICE_MODE, USB_HOST_MODE);
#elif (CONFIG_USB_HOST)
		bk_usb_open(USB_HOST_MODE);
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
#endif

		rtos_init_oneshot_timer(&acm_count_dev_onetimer,CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT+100,bk_usb_acm_count_dev_callback,(void *)0,(void *)0);
	}
    	else
    	{
#if (CONFIG_USB_DEVICE && CONFIG_USB_HOST)
		bk_usb_otg_manual_convers_mod(USB_HOST_MODE, USB_DEVICE_MODE);
#elif (CONFIG_USB_DEVICE)
		bk_usb_open(USB_DEVICE_MODE);
#endif   
    	}
}

void bk_usb_cdc_close_ind(E_USB_MODE close_usb_mode)
{
	if (close_usb_mode == USB_HOST_MODE)
	{
#if (CONFIG_USB_DEVICE && CONFIG_USB_HOST)
		bk_usb_otg_manual_convers_mod(USB_HOST_MODE, USB_DEVICE_MODE);
#elif (CONFIG_USB_HOST)
		bk_usb_close();
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 0);
#endif
		if (rtos_is_oneshot_timer_running(&acm_count_dev_onetimer))
		{
			rtos_stop_oneshot_timer(&acm_count_dev_onetimer);
		}
		rtos_deinit_oneshot_timer(&acm_count_dev_onetimer);
	}
	else
	{
#if (CONFIG_USB_DEVICE && CONFIG_USB_HOST)
		bk_usb_otg_manual_convers_mod(USB_DEVICE_MODE, USB_HOST_MODE);
#elif (CONFIG_USB_DEVICE)
		bk_usb_close();
#endif           
	}
}

static void bk_usb_acm_count_dev_checktimer(uint32 data)
{
	if (rtos_is_oneshot_timer_running(&acm_count_dev_onetimer))
	{
		rtos_stop_oneshot_timer(&acm_count_dev_onetimer);
	}
	rtos_start_oneshot_timer(&acm_count_dev_onetimer);
}


void bk_usb_get_cdc_instance(struct usbh_hubport *hport, uint8_t intf, uint32_t class)
{
	struct usbh_cdc_acm *usb_device = NULL;

	usb_device = (struct usbh_cdc_acm *)usbh_find_class_instance(hport->config.intf[intf].devname);
	if (usb_device == NULL) {
		USB_CDC_LOGE("don't find /dev/ttyACM%d\r\n", usb_device->minor);
	}
	if (class == USB_DEVICE_CLASS_CDC)
	{
		if (g_cdc_acm_device[acm_cnt])
		{
			g_cdc_acm_device[acm_cnt] = NULL;
		}
		if (g_cdc_acm_device[acm_cnt] == NULL)
		{
			g_cdc_acm_device[acm_cnt] = usb_device;
			g_cdc_acm_device[acm_cnt]->hport = usb_device->hport;
		//	if (usb_device->minor == 0)
			{
			//	bk_usbh_cdc_acm_register_in_transfer_callback(g_cdc_acm_device[acm_cnt], bk_cdc_acm_bulkin_callback, NULL);
			//	bk_usbh_cdc_acm_register_out_transfer_callback(g_cdc_acm_device[acm_cnt], bk_cdc_acm_bulkout_callback, NULL);
			}
	//		else if (usb_device->minor == 1) {
	//			bk_usbh_cdc_acm_register_in_transfer_callback(g_cdc_acm_device[acm_cnt], bk_cdc_acm_modem_bulkin_callback, NULL);
	//			bk_usbh_cdc_acm_register_out_transfer_callback(g_cdc_acm_device[acm_cnt], bk_cdc_acm_modem_bulkout_callback, NULL);
	//		}
		}
	}	
	else if (class == USB_DEVICE_CLASS_CDC_DATA)
	{
		if (g_cdc_data_device[acm_cnt])
		{
			g_cdc_data_device[acm_cnt] = NULL;
		}
		if (g_cdc_data_device[acm_cnt] == NULL)
		{
			g_cdc_data_device[acm_cnt] = usb_device;
			g_cdc_data_device[acm_cnt]->hport = usb_device->hport;
		//	if (usb_device->minor == 0)
			{
				bk_usbh_cdc_acm_register_in_transfer_callback(g_cdc_data_device[acm_cnt], bk_cdc_acm_bulkin_callback, NULL);
				bk_usbh_cdc_acm_register_out_transfer_callback(g_cdc_data_device[acm_cnt], bk_cdc_acm_bulkout_callback, NULL);
			}
	//		else if (usb_device->minor == 1) {
	//			bk_usbh_cdc_acm_register_in_transfer_callback(g_cdc_data_device[acm_cnt], bk_cdc_acm_modem_bulkin_callback, NULL);
	//			bk_usbh_cdc_acm_register_out_transfer_callback(g_cdc_data_device[acm_cnt], bk_cdc_acm_modem_bulkout_callback, NULL);
	//		}
		}
		USB_CDC_LOGD("bk_usb_get_cdc_instance %d, device:0x%x, intf:%d\r\n", acm_cnt, usb_device,  g_cdc_data_device[acm_cnt]->intf);
		acm_cnt++;
	} 
	else
	{
		USB_CDC_LOGD("bk_usb_get_cdc_instance %d, discard class %d, intf:%d\r\n", acm_cnt, class, intf);
	}
}
#if CONFIG_USB_CDC_MODEM
void bk_usb_cdc_connect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class)
{
	bk_usb_get_cdc_instance(hport, intf, class);

	bk_usb_acm_count_dev_checktimer(ACM_CONNECT_IND);   
}

void bk_usb_cdc_disconnect_notify(struct usbh_hubport *hport, uint8_t intf, uint32_t class)
{
	acm_cnt = 0;
	acm_device = NULL;
	CDC_STATUS_t dev_state;
	dev_state.dev_cnt = 0;
	dev_state.status = CDC_STATUS_DISCON;
	bk_cdc_acm_state_notify(&dev_state);
}
#endif

