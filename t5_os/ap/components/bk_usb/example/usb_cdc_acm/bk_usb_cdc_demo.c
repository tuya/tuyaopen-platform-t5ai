
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <driver/pwr_clk.h>
#include <common/bk_err.h>
#include <common/bk_include.h>

#include "mb_ipc_cmd.h"
#include "bk_usb_cdc_demo.h"

#define TAG "cdc_demo"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

uint32_t ipc_cdc_rx_len = 0;
uint8_t ipc_rxdata_dbg[128] = {0};
IPC_CDC_DATA_t dbg_cdc_ipc;

static bk_cdc_hub_status g_cdc_hub_status[10];

static beken_queue_t cdc_msg_queue = NULL;
static beken_thread_t cdc_demo_task = NULL;

static uint8_t tx_buf[6] = {0xEF,0xAA,0x30,0x00,0x00,0x30};

static uint8_t g_cdc_start_done = 0x0;
static uint8_t g_cdc_init_done = 0x0;

static uint32_t s_specified_port = 0xFF;
static uint32_t s_close = 0x0;

void (*usb_cdc_state_cb)(bk_cdc_hub_status *cdc_status);
void bk_usb_cdc_connect_init_cb(void (*cb)(bk_cdc_hub_status *cdc_status))
{
	usb_cdc_state_cb = cb;
}

#if (USB_CDC_CP0_IPC)
extern void ipc_cdc_send_cmd(u8 cmd, u8 *cmd_buf, u16 cmd_len, u8 * rsp_buf, u16 rsp_buf_len);

void bk_usb_cdc_start(void)
{
	ipc_cdc_send_cmd(IPC_CPU0_START_USB_CDC, NULL, 0, NULL, 0);
}

void bk_usb_cdc_open(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_OPEN_USB_CDC, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}

void bk_usb_cdc_close(void)
{
	ipc_cdc_send_cmd(IPC_CPU0_CLOSE_USB_CDC, NULL, 0, NULL, 0);
}

void bk_usb_cdc_stop(void)
{
	ipc_cdc_send_cmd(IPC_CPU0_STOP_USB_CDC, NULL, 0, NULL, 0);
}

void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_SET_USB_CDC_CMD, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}

void bk_usb_cdc_init(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_INIT_USB_CDC_PARAM, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}


#else
extern void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *ipc_cdc);
extern void bk_usb_cdc_init(IPC_CDC_DATA_t *p_cdc_data);

#endif


static bk_err_t cdc_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	cdc_msg_t msg;

	if (cdc_msg_queue)
	{
		msg.type = type;
		msg.data = param;

		ret = rtos_push_to_queue(&cdc_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

void ex_cdc_send_msg(uint8_t type, uint32_t param)
{
	cdc_send_msg(type, param);
}

static void bk_cdc_acm_close(void)
{
	os_memset(&dbg_cdc_ipc, 0x00, sizeof(IPC_CDC_DATA_t));
	os_memset(g_cdc_hub_status, 0x00, sizeof(bk_cdc_hub_status));
	s_specified_port = 0;
	s_close = 0;
	g_cdc_start_done = 0;
	g_cdc_init_done = 0;
	cdc_send_msg(CDC_STATUS_EXIT, 0);

	//free buffer and variable
}

static void bk_cdc_acm_bulkin_cb(void)
{
	LOGV("getdata 0x%x 0x%x %d\n", dbg_cdc_ipc.rx_data, dbg_cdc_ipc.rx_len, ipc_cdc_rx_len);
	if (&dbg_cdc_ipc) {
		uint8_t *temp = (uint8_t *)dbg_cdc_ipc.rx_data;
		for (uint32_t i = 0; i < ipc_cdc_rx_len; i++)
			LOGV("%x\n", temp[i]);
		os_memset(temp, 0x00, ipc_cdc_rx_len);
		ipc_cdc_rx_len = 0;

		dbg_cdc_ipc.state = CDC_STATUS_OUT;
		cdc_send_msg(CDC_STATUS_OUT, 0);
	}
}

static void bk_cdc_acm_bulkout_cb(void *p)
{
	LOGD("[+]%s\r\n", __func__);
	IPC_CDC_DATA_t * p_cdc = (IPC_CDC_DATA_t *)p;
	bk_cdc_acm_bulkout(p_cdc);
}

static void bk_cdc_acm_init(void)
{
	memset(&dbg_cdc_ipc,0x00,sizeof(dbg_cdc_ipc));
	dbg_cdc_ipc.port_idx = s_specified_port;
	dbg_cdc_ipc.rx_data = (uint32_t)(&ipc_rxdata_dbg[0]);
	dbg_cdc_ipc.rx_len  = (uint32_t)(&ipc_cdc_rx_len);
	dbg_cdc_ipc.cmd_len = sizeof(IPC_CDC_DATA_t);
	dbg_cdc_ipc.bk_cdc_acm_bulkin_cb  = bk_cdc_acm_bulkin_cb;
	dbg_cdc_ipc.bk_cdc_acm_bulkout_cb = bk_cdc_acm_bulkout_cb;
	bk_usb_cdc_init(&dbg_cdc_ipc);
}


static void bk_cdc_acm_state_cb(bk_cdc_hub_status *cdc_status)
{
	uint32_t port_idx = cdc_status->port_idx;
	uint32_t port_status = cdc_status->port_status;

	LOGD("[+]%s idx = %d  status = %d\r\n", __func__, port_idx, port_status);

	g_cdc_hub_status[port_idx].port_idx = port_idx;
	g_cdc_hub_status[port_idx].port_status = port_status;

	if (g_cdc_hub_status[port_idx].port_idx == s_specified_port)
	{
		dbg_cdc_ipc.state = port_status;
		switch(port_status) {
			case CDC_STATUS_OPEN:
			//	cdc_send_msg(CDC_STATUS_INIT, 0);
				break;
			case CDC_STATUS_OUT:
			//	cdc_send_msg(CDC_STATUS_OUT, 0);
				break;
			case CDC_STATUS_CLOSE:
				bk_cdc_acm_close();
			//	cdc_send_msg(CDC_STATUS_CLOSE, 0);
				break;
			default:
				break;
		}
	}
}

void demo_bulk_out(uint32_t port)
{
	LOGV("[+]%s %d %d %d\n", __func__, __LINE__, g_cdc_hub_status[port].port_idx, g_cdc_hub_status[port].port_status);
	uint32_t status = g_cdc_hub_status[port].port_status;
	if (status == CDC_STATUS_OPEN)
	{
		s_specified_port = port;
		dbg_cdc_ipc.state = CDC_STATUS_OUT;
		cdc_send_msg(CDC_STATUS_OUT, s_specified_port);
	} else {
		LOGD("Unvalid parameter!!! port_status %d\n", g_cdc_hub_status[port].port_status);
	}
}

static void bk_cdc_demo_task(beken_thread_arg_t arg)
{
	int ret = BK_OK;
	cdc_msg_t msg;

	while (1)
	{
		ret = rtos_pop_from_queue(&cdc_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		LOGV("%s, type %d init %d\n", __func__, msg.type, g_cdc_init_done);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case CDC_STATUS_IDLE: 
					{
						g_cdc_start_done = 1;
						if (!g_cdc_init_done)
						{
							s_specified_port = msg.data;
							bk_cdc_acm_init();
							g_cdc_init_done = 1;
							s_close = 0;
						}
					}
					break;
				case CDC_STATUS_OPEN:
					break;
				case CDC_STATUS_INIT:
					if (!g_cdc_init_done)
					{
						bk_cdc_acm_init();
						g_cdc_init_done = 1;
					}
					break;

				case CDC_STATUS_OUT:
					{
						if (dbg_cdc_ipc.state == CDC_STATUS_OUT && !s_close) {
							if (!g_cdc_init_done)
							{
								bk_cdc_acm_init();
								g_cdc_init_done = 1;
							}

							rtos_delay_milliseconds(2000);

							dbg_cdc_ipc.port_idx = s_specified_port;
							dbg_cdc_ipc.tx_data = (uint32_t)tx_buf;
							dbg_cdc_ipc.tx_len = sizeof(tx_buf)*sizeof(uint8_t);
							dbg_cdc_ipc.cmd_len = sizeof(IPC_CDC_DATA_t);

							bk_cdc_acm_bulkout(&dbg_cdc_ipc);
						}
					}
					break;
				case CDC_STATUS_STOP:
					{
						dbg_cdc_ipc.state = CDC_STATUS_STOP;
						bk_usb_cdc_stop();
					}
					break;
				case CDC_STATUS_REOPEN:
					break;
				case CDC_STATUS_CLOSE:
					{
						s_close = 1;
						bk_usb_cdc_close();
					}
					break;
				case CDC_STATUS_ABNORMAL:
				case CDC_STATUS_EXIT:
					goto exit;
				default:
					break;
			}

		}
	}
exit:
	/* step 1: free que and delete task */
	rtos_deinit_queue(&cdc_msg_queue);
	cdc_msg_queue = NULL;
	cdc_demo_task = NULL;
	rtos_delete_thread(NULL);
}

void bk_usb_cdc_demo(void)
{
	LOGV("[+]%s\n", __func__);
	int ret = kNoErr;
	bk_usb_cdc_connect_init_cb(bk_cdc_acm_state_cb);

	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);

	if (cdc_msg_queue == NULL)
	{
		ret = rtos_init_queue(&cdc_msg_queue, "cdc_msg_queue", sizeof(cdc_msg_t), 10);
		if (ret != kNoErr)
		{
			LOGE("init cdc_msg_queue failed\r\n");
			goto error;
		}
	}

	if (cdc_demo_task == NULL)
	{
		ret = rtos_create_thread(&cdc_demo_task,
							BEKEN_APPLICATION_PRIORITY,
							"cdc_demo_task",
							(beken_thread_function_t)bk_cdc_demo_task,
							2*1024,
							NULL);

		if (ret != kNoErr)
		{
			goto error;
		}
	}
	return;
error:
	if (cdc_msg_queue)
	{
		rtos_deinit_queue(&cdc_msg_queue);
		cdc_msg_queue = NULL;
	}

	if (cdc_demo_task)
	{
		cdc_demo_task = NULL;
		rtos_delete_thread(NULL);
	}
}


