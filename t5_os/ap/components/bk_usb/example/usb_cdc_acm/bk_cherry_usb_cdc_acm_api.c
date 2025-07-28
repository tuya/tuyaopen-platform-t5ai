#include <os/os.h>
#include <os/mem.h>

#include "usb_driver.h"

#include "bk_cherry_usb_cdc_acm_api.h"
#include "usbh_cdc_acm.h"
#include "usbh_core.h"
#include <driver/gpio.h>
#include <modules/pm.h>

#include "mb_ipc_cmd.h"

#include <components/usbh_hub_multiple_classes_api.h>
//#include "media_types.h"

#define BK_CDC_CONNECT         (1)
#define BK_CDC_CONFIG_ERROR    (2)
#define BK_CDC_NO_MEMORY       (3)
#define BK_CDC_NO_RESOURCE     (4)
#define BK_CDC_PPI_ERROR       (5)
#define BK_CDC_DISCONNECT      (6)
#define BK_CDC_NO_RESPON       (7)
#define BK_CDC_DMA_ERROR       (8)
#define BK_CDC_NOT_PERMIT      (9)
#define BK_CDC_POWER_ERROR     (10)

#define MAX_BULK_TRS_SIZE (1024)
#define CDC_TX_MAX_SIZE     510


static uint32_t g_usb_cdc_flag[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1] = {0x0};
static bk_usb_hub_port_info * g_cdc_hub_port_info[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1] = {NULL};

static bk_cdc_hub_status g_cdc_hub_status[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1];

static uint32_t g_cdc_curr_using = 0x0;

static uint32_t io_read_flag = 0;

static beken_queue_t acm_msg_queue   = NULL;
static beken_thread_t acm_class_task = NULL;

static uint8_t cdc_tx_state  = 0;
static uint8_t cdc_tx_finish = 0;

IPC_CDC_DATA_t *g_ipc_cdc_data = NULL;

static uint8_t buffer[512] = {0};

struct usbh_cdc_acm * acm_device[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1] = {NULL};

static uint8_t g_cdc_acm_buf[6] = {0xEF,0xAA, 0x30, 0x00, 0x00, 0x30};

////static uint8_t g_bInterfaceNumber = 0;
////static uint8_t g_interface_sub_class = 0;
////static struct usbh_hubport * g_u_hport = NULL;

static uint8_t g_cdc_process_en = 0;
static uint8_t g_cdc_connect = 0;
#if (USB_CDC_CP1_IPC)
extern void ipc_cdc_send_cmd(u8 cmd, u8 *cmd_buf, u16 cmd_len, u8 * rsp_buf, u16 rsp_buf_len);
void bk_usb_cdc_upload_data(IPC_CDC_DATA_t *ipc_data)
{
	ipc_cdc_send_cmd(IPC_CPU1_UPLOAD_USB_CDC_DATA, (uint8_t *)ipc_data, ipc_data->cmd_len, NULL, 0);
}

void bk_usb_cdc_update_state_notify(bk_cdc_hub_status * cdc_state)
{
	ipc_cdc_send_cmd(IPC_CPU1_UPDATE_USB_CDC_STATE, (uint8_t *)cdc_state, sizeof(bk_cdc_hub_status), NULL, 0);
}

#else
void bk_usb_cdc_upload_data(IPC_CDC_DATA_t *ipc_data)
{
	ipc_data->bk_cdc_acm_bulkin_cb();
}

extern void (*usb_cdc_state_cb)(bk_cdc_hub_status * );
void bk_usb_cdc_update_state_notify(bk_cdc_hub_status * cdc_state)
{
	if(usb_cdc_state_cb != NULL)
		usb_cdc_state_cb(cdc_state);
}
#endif


bk_err_t bk_cdc_acm_demo(void);
void bk_cdc_acm_bulkin_callback(void *arg, int nbytes);
void bk_cdc_acm_bulkout_callback(void *arg, int nbytes);

/*********************************************************************************************************/
static bk_err_t acm_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	acm_msg_t msg;

	if (acm_msg_queue)
	{
		msg.type = type;
		msg.data = param;

		ret = rtos_push_to_queue(&acm_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

void bk_acm_trigger_rx(void)
{
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.complete = bk_cdc_acm_bulkin_callback;
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.pipe     = acm_device[g_ipc_cdc_data->port_idx]->bulkin;
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.transfer_buffer = &buffer[0];
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.transfer_buffer_length = sizeof(buffer)*sizeof(uint8_t);
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.timeout = 0;
	acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.actual_length = 0;
}

void bk_acm_trigger_tx(void)
{
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.complete = bk_cdc_acm_bulkout_callback;
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.pipe     = acm_device[g_ipc_cdc_data->port_idx]->bulkout;
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.transfer_buffer = &g_cdc_acm_buf[0];
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.transfer_buffer_length = sizeof(g_cdc_acm_buf)*sizeof(uint8_t);
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.timeout = 100;
	acm_device[g_ipc_cdc_data->port_idx]->bulkout_urb.actual_length = 0;
}

void bk_cdc_acm_bulkin_callback(void *arg, int nbytes)
{
	USB_CDC_LOGV("[+]%s, %d, %d\r\n", __func__, nbytes, g_ipc_cdc_data->port_idx);
	for (uint32_t i = 0; i < nbytes; i++) {
		USB_CDC_LOGV("%02x ", buffer[i]);
	}
	USB_CDC_LOGV("\n");

	if (nbytes >= 0 && acm_device[g_ipc_cdc_data->port_idx]) {
		*((uint32_t *)g_ipc_cdc_data->rx_len) = nbytes;
		os_memcpy((uint8_t *)(g_ipc_cdc_data->rx_data), &buffer[0], nbytes);
		bk_acm_trigger_rx();
		acm_device[g_ipc_cdc_data->port_idx]->bulkin_urb.actual_length = 0;
		memset(buffer, 0x00, sizeof(buffer));
		acm_send_msg(ACM_UPLOAD_IND, 0);
	}
	else 
		USB_CDC_LOGD("Error bulkin status\n");

}

void bk_cdc_acm_bulkout_callback(void *arg, int nbytes)
{
	USB_CDC_LOGV("[+]%s, nbytes: %d %d\r\n", __func__, nbytes, cdc_tx_finish);

	bk_acm_trigger_tx();
	cdc_tx_state = 1;
	if(cdc_tx_finish)
	{
		acm_send_msg(ACM_BULKIN_IND, 0);
		cdc_tx_finish = 0;
	}
}

int32_t bk_usbh_cdc_acm_register_in_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkin_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}

int32_t bk_usbh_cdc_acm_register_out_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkout_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}


void bk_usb_update_cdc_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class)
{
//	if (interface_sub_class != CDC_SUBCLASS_ACM || !hport)
//		return;
//	USB_CDC_LOGV("[+]%s\r\n", __func__);
//	struct usbh_hubport * u_hport = (struct usbh_hubport *)hport;
//	g_u_hport = (struct usbh_hubport *)hport;
//	g_bInterfaceNumber = bInterfaceNumber;
//	g_interface_sub_class = interface_sub_class;
//
//	bk_usbh_cdc_sw_init(u_hport, bInterfaceNumber, interface_sub_class);
//
//	acm_device[g_ipc_cdc_data->port_idx] = (struct usbh_cdc_acm *)usbh_find_class_instance(u_hport->config.intf[bInterfaceNumber].devname);
//	if (acm_device[g_ipc_cdc_data->port_idx] == NULL)
//		USB_LOG_ERR("don't find /dev/ttyACM0\r\n");
//
//	USB_CDC_LOGV("[-]%s, %s\r\n", __func__, u_hport->config.intf[bInterfaceNumber].devname);
//
//	bk_cdc_acm_demo();
}


int32_t bk_cdc_acm_io_read(void)
{
	USB_CDC_LOGV("[+]%s %d\r\n", __func__, g_ipc_cdc_data->port_idx);
	int32_t ret = 0;
	uint32_t timeout = 0;
	if (acm_device[g_ipc_cdc_data->port_idx] == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}
	bk_acm_trigger_rx();
	usbh_cdc_acm_bulk_in_transfer(acm_device[g_ipc_cdc_data->port_idx], &buffer[0], sizeof(buffer)*sizeof(uint8_t), timeout);

	return ret;
}

/*********************************************************************************************************/

int32_t bk_cdc_acm_io_write(IPC_CDC_DATA_t *p_cdc_data)
{
	USB_CDC_LOGV("[+]%s %d\r\n", __func__, p_cdc_data->port_idx);
	int32_t ret = 0;
	uint32_t timeout = 100;

	if (acm_device[p_cdc_data->port_idx] == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}
	bk_acm_trigger_tx();

	if (p_cdc_data->tx_len > CDC_TX_MAX_SIZE)
	{
		uint32_t sum_len = p_cdc_data->tx_len;
		uint32_t ops = 0;
		uint32_t one_len = 0;
		uint8_t *buf = (uint8_t *)(p_cdc_data->tx_data);
		while(ops < sum_len)
		{
			one_len = CDC_TX_MAX_SIZE;
			if((sum_len - ops) < CDC_TX_MAX_SIZE)
			{
				one_len = sum_len - ops;
				cdc_tx_finish = 1;
			}
			ret = usbh_cdc_acm_bulk_out_transfer(acm_device[p_cdc_data->port_idx], buf+ops, one_len, timeout);

			if(ret == 0)
			{
				ops+= one_len;
			}
			else
			{
				rtos_delay_milliseconds(3);
			}
			USB_CDC_LOGV("out ret:%d\r\n", ret);
		//	rtos_get_semaphore(&acm_tx_msg_sem, 1000);
		//	rtos_delay_milliseconds(1);
			while(!cdc_tx_state)
			{
				rtos_delay_milliseconds(2);
			}
			cdc_tx_state = 0;
		}

	}
	else {
		cdc_tx_finish = 1;
		if (acm_device[p_cdc_data->port_idx])
			ret = usbh_cdc_acm_bulk_out_transfer(acm_device[p_cdc_data->port_idx], (uint8_t *)p_cdc_data->tx_data, p_cdc_data->tx_len, timeout);
	}
	return ret;
}

static void bk_cdc_activate_epx(void)
{
	USB_CDC_LOGV("[+]%s\r\n", __func__);
	bk_usb_hub_port_info *port_info = g_cdc_hub_port_info[g_cdc_curr_using];
	struct usbh_cdc_acm *t_acm_device = acm_device[port_info->port_index];
	if (g_cdc_hub_status[port_info->port_index].port_status == CDC_STATUS_OPEN)
	{
		struct usb_endpoint_descriptor *ep_desc;
		for (uint8_t i = 0; i < port_info->hport->config.intf[t_acm_device->ctrl_intf + 1].altsetting[0].intf_desc.bNumEndpoints; i++) {
			ep_desc = &port_info->hport->config.intf[t_acm_device->ctrl_intf + 1].altsetting[0].ep[i].ep_desc;

			if (ep_desc->bEndpointAddress & 0x80) {
				usbh_hport_activate_epx(&t_acm_device->bulkin, port_info->hport, ep_desc);
			} else {
				usbh_hport_activate_epx(&t_acm_device->bulkout, port_info->hport, ep_desc);
			}
		}

		g_cdc_hub_status[port_info->port_index].port_idx = port_info->port_index;
		g_cdc_hub_status[port_info->port_index].port_status = CDC_STATUS_OUT;
	}
}

static int32_t bk_usbh_cdc_acm_connect(bk_usb_hub_port_info *port_info)
{
	uint32_t port_idx = port_info->hport->port;
	acm_device[port_info->port_index] = (struct usbh_cdc_acm *)port_info->usb_device;
	g_cdc_hub_port_info[port_info->port_index] = port_info;
	struct usbh_cdc_acm * t_acm_device = (struct usbh_cdc_acm *)acm_device[port_info->port_index];

	USB_CDC_LOGV("[+]%s port_index = %d %d acm_device = 0x%x, g_usb_cdc_flag = 0x%x\r\n", __func__, port_info->port_index, port_idx,
		t_acm_device, g_usb_cdc_flag[port_info->port_index]);

	if (t_acm_device)
	{
		bk_usbh_cdc_acm_register_in_transfer_callback(t_acm_device, bk_cdc_acm_bulkin_callback, NULL);
		bk_usbh_cdc_acm_register_out_transfer_callback(t_acm_device, bk_cdc_acm_bulkout_callback, NULL);

		g_cdc_hub_status[port_idx].port_idx = port_info->port_index;
		g_cdc_hub_status[port_idx].port_status = CDC_STATUS_OPEN;

		g_cdc_connect = 1;
		USB_CDC_LOGV("[+]%s port_index = %d %d %d\r\n", __func__, port_idx, g_cdc_hub_status[port_idx].port_idx,
			g_cdc_hub_status[port_idx].port_status);
		bk_usb_cdc_update_state_notify(&g_cdc_hub_status[port_idx]);
	}
	return 0;

}

int32_t bk_usbh_cdc_acm_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
	uint32_t port_idx = port_info->hport->port;
	acm_device[port_info->port_index] = (struct usbh_cdc_acm *)port_info->usb_device;
	g_cdc_hub_port_info[port_info->port_index] = port_info;
	g_usb_cdc_flag[port_info->port_index] = *((uint32_t *)arg);
	struct usbh_cdc_acm * t_acm_device = (struct usbh_cdc_acm *)acm_device[port_info->port_index];

	USB_CDC_LOGV("[+]%s port_index = %d %d acm_device = 0x%x, g_usb_cdc_flag = 0x%x\r\n", __func__, port_info->port_index, port_idx,
		t_acm_device, g_usb_cdc_flag[port_info->port_index]);

	if (t_acm_device && (g_usb_cdc_flag[port_idx] & (0x1<<USB_CDC_DEVICE)))
	{
		bk_usbh_cdc_acm_register_in_transfer_callback(t_acm_device, bk_cdc_acm_bulkin_callback, NULL);
		bk_usbh_cdc_acm_register_out_transfer_callback(t_acm_device, bk_cdc_acm_bulkout_callback, NULL);

		g_cdc_hub_status[port_idx].port_idx = port_info->port_index;
		g_cdc_hub_status[port_idx].port_status = CDC_STATUS_OPEN;

		g_cdc_connect = 1;
		USB_CDC_LOGV("[+]%s port_index = %d %d %d\r\n", __func__, port_idx, g_cdc_hub_status[port_idx].port_idx,
			g_cdc_hub_status[port_idx].port_status);
		bk_usb_cdc_update_state_notify(&g_cdc_hub_status[port_idx]);
	}
	return 0;
}

int32_t bk_usbh_cdc_acm_disconnect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
	uint32_t port = 1;
	for (; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
	{
		if (acm_device[port])
			acm_device[port] = NULL;
		if (g_cdc_hub_port_info[port])
			g_cdc_hub_port_info[port] = NULL;
	}

	memset(g_cdc_hub_status, 0x00, sizeof(bk_cdc_hub_status) * (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1));
	memset(g_usb_cdc_flag, 0x00, sizeof(uint32_t)*(CONFIG_USBHOST_HUB_MAX_EHPORTS + 1));

	io_read_flag = 0;
	g_ipc_cdc_data = NULL;
	g_cdc_connect = 0;

	/* free que and delete task */
	rtos_deinit_queue(&acm_msg_queue);
	acm_msg_queue = NULL;
	acm_class_task = NULL;
	rtos_delete_thread(NULL);

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);
	return 0;
}

/*************************************************************************************************/
void bk_usb_cdc_open(IPC_CDC_DATA_t *ipc_cdc)
{
//	if (!ipc_cdc)
//		USB_CDC_LOGE("Invalid Parameter!\n");
//
//	g_ipc_cdc_data = ipc_cdc;
//
//	bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
//	bk_usb_open(USB_HOST_MODE);
////	bk_usb_device_set_using_status(1, USB_CDC_DEVICE);
//
//	acm_send_msg(ACM_BULKIN_IND, 0);
}

void bk_usb_cdc_free_enumerate_resources(void)
{
//	for (uint32_t port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
//	{
//		if(acm_device[port - 1]) {
//			bk_usbh_cdc_sw_deinit(acm_device[port - 1]->hport, acm_device[port - 1]->ctrl_intf, CDC_SUBCLASS_ACM);
//			acm_device[port - 1] = NULL;
//		}
//	}
//	acm_send_msg(ACM_EXIT_IND, 0);
}

/*************************************************************************************************/
static void bk_usb_cdc_acm_start(void)
{
	uint32_t port = 1;
	int ret = BK_OK;

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
	for (port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
	{
		// register connect callback
		ret = bk_usbh_hub_port_register_connect_callback(port, USB_CDC_DEVICE, bk_usbh_cdc_acm_connect_callback, g_usb_cdc_flag);
		if (ret != BK_OK)
		{
			USB_CDC_LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
			ret = BK_CDC_NOT_PERMIT;
			goto error;
		}

		// register disconect callback
		ret = bk_usbh_hub_port_register_disconnect_callback(port, USB_CDC_DEVICE, bk_usbh_cdc_acm_disconnect_callback, g_usb_cdc_flag);
		if (ret != BK_OK)
		{
			USB_CDC_LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
			ret = BK_CDC_NOT_PERMIT;
			goto error;
		}

		// power_on
		ret = bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_CDC_DEVICE);
		if (ret != BK_OK)
		{
			USB_CDC_LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
			ret = BK_CDC_POWER_ERROR;
			goto error;
		}
	}

	if (!g_cdc_connect)
	{
		for (port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
		{
			ret = bk_usbh_hub_port_check_device(port, USB_CDC_DEVICE, &g_cdc_hub_port_info[port]);
//			if (ret != BK_OK) {
//				USB_CDC_LOGE("USB CDC get enum information fail\n!!!");
//			}
			if (g_cdc_hub_port_info[port]) {
				bk_usbh_cdc_acm_connect(g_cdc_hub_port_info[port]);
			}
		}
		g_cdc_connect = 1;
	}

	g_cdc_process_en = 1;

	return;
error:
	for (port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
	{
		bk_usbh_hub_port_register_connect_callback(port, USB_CDC_DEVICE, NULL, NULL);
		bk_usbh_hub_port_register_disconnect_callback(port, USB_CDC_DEVICE, NULL, NULL);
		bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_CDC_DEVICE);
	}
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);
}

static int bk_usb_cdc_acm_close(void)
{
	int ret = BK_FAIL;
	uint32_t port = 1;
	struct usbh_cdc_acm * t_acm_device = NULL;

	ret = usbh_cdc_acm_set_line_state((struct usbh_cdc_acm * )acm_device[g_cdc_curr_using], 0, 1);
	if (ret < 0) {
		USB_CDC_LOGE("Fail to set line state\r\n");
		return ret;
	}

	g_cdc_process_en = 0;

	for (port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
	{
		t_acm_device = (struct usbh_cdc_acm *)acm_device[port];
		if (t_acm_device && (g_usb_cdc_flag[port] & (0x1<<USB_CDC_DEVICE)))
		{
			bk_usbh_cdc_acm_register_in_transfer_callback(t_acm_device, bk_cdc_acm_bulkin_callback, NULL);
			bk_usbh_cdc_acm_register_out_transfer_callback(t_acm_device, bk_cdc_acm_bulkout_callback, NULL);

			g_cdc_hub_status[port].port_idx = port;
			g_cdc_hub_status[port].port_status = CDC_STATUS_CLOSE;

			USB_CDC_LOGD("[+]%s port = %d %d %d\r\n", __func__, port, g_cdc_hub_status[port].port_idx,
				g_cdc_hub_status[port].port_status);
			bk_usb_cdc_update_state_notify(&g_cdc_hub_status[port]);
		}
	}

	for (port = 1; port < (CONFIG_USBHOST_HUB_MAX_EHPORTS + 1); port++)
	{
		bk_usbh_hub_port_register_connect_callback(port, USB_CDC_DEVICE, NULL, NULL);
		bk_usbh_hub_port_register_disconnect_callback(port, USB_CDC_DEVICE, NULL, NULL);

		bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_CDC_DEVICE);
	}

	return BK_OK;

}


void bk_usb_cdc_start(void)
{
	acm_send_msg(ACM_START_IND, 0);
}

void bk_usb_cdc_close(void)
{
	acm_send_msg(ACM_STOP_IND, 0);
}

void bk_usb_cdc_exit(void)
{
	acm_send_msg(ACM_EXIT_IND, 0);
}

void bk_usb_cdc_stop(void)
{

}

void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *p_cdc_data)
{
	if (p_cdc_data->port_idx != g_cdc_curr_using) {
		g_ipc_cdc_data = p_cdc_data;
		g_cdc_curr_using = p_cdc_data->port_idx;
		io_read_flag = 0;
	}
	acm_send_msg(ACM_BULKOUT_IND, (uint32_t)p_cdc_data);
}

void bk_usb_cdc_init(IPC_CDC_DATA_t *p_cdc_data)
{
	if (!p_cdc_data)
		USB_CDC_LOGE("Invalid Parameter!\n");

	g_ipc_cdc_data = p_cdc_data;
	g_cdc_curr_using = p_cdc_data->port_idx;

	bk_usb_cdc_start();
}

/**************************************************************************************/

void bk_cdc_acm_main(void)
{
	int32_t ret = BK_OK;
	rtos_delay_milliseconds(400);

	while (1)
	{
		acm_msg_t msg;
		ret = rtos_pop_from_queue(&acm_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case ACM_START_IND:
					bk_usb_cdc_acm_start();
					break;
				case ACM_BULKIN_IND:
					{
						if (!io_read_flag)
						{
							bk_cdc_acm_io_read();
							io_read_flag = 1;
						}
					}
					break;

				case ACM_BULKOUT_IND:
					{
						bk_cdc_activate_epx();
						if (g_cdc_process_en)
							bk_cdc_acm_io_write((IPC_CDC_DATA_t *)msg.data);
					}
					break;

				case ACM_UPLOAD_IND:
					{
						if (g_cdc_process_en)
							bk_usb_cdc_upload_data(g_ipc_cdc_data);
					}
					break;

				case ACM_UPDATE_STATE_IND:
					{
						USB_CDC_LOGV("ACM_UPDATE_STATE_IND\n");
						uint32_t state = (uint32_t) msg.data;
						BK_ASSERT(g_ipc_cdc_data);
						g_ipc_cdc_data->state = state;
						if (!state) {
							usbh_cdc_acm_set_line_state(acm_device[g_ipc_cdc_data->port_idx], false, true);
							USB_CDC_LOGV("bk_usb_close %d", bk_usb_close());
							goto exit;
						}
					}
					break;
				case ACM_STOP_IND:
					bk_usb_cdc_acm_close();
					break;
				case ACM_EXIT_IND:
						goto exit;
					break;
				default:
					break;
			}
		}
	}

exit:
	io_read_flag = 0;
	g_ipc_cdc_data = NULL;

	/* step 1: free que and delete task */
	rtos_deinit_queue(&acm_msg_queue);
	acm_msg_queue = NULL;
	acm_class_task = NULL;
	rtos_delete_thread(NULL);
}


bk_err_t bk_cdc_acm_demo(void)
{
	int32_t ret = kNoErr;

	if (acm_class_task)
		return ret;

	if (acm_msg_queue == NULL) {
		ret = rtos_init_queue(&acm_msg_queue, "acm_class_queue", sizeof(acm_msg_t), 20);
		if (ret != kNoErr)
		{
			goto error;
		}
	}

	if (acm_class_task == NULL) {
		ret = rtos_create_thread(&acm_class_task,
								4,
								"acm_class_task",
								(beken_thread_function_t)bk_cdc_acm_main,
								1024,
								acm_msg_queue);

		if (ret == kNoErr)
		{
			return ret;
		}
	}
error:
	if (acm_msg_queue)
	{
		rtos_deinit_queue(&acm_msg_queue);
		acm_msg_queue = NULL;
	}

	if (acm_class_task)
	{
		rtos_delete_thread(&acm_class_task);
		acm_class_task = NULL;
	}
	return ret;
}

