#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <driver/pwr_clk.h>

#include <common/bk_err.h>
#include <common/bk_include.h>

#include "cli.h"
#include "mb_ipc_cmd.h"
#include "bk_usb_cdc_modem.h"
#include "bk_cherry_usb_cdc_acm_api.h"

#define TAG "cdc_modem"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static beken_queue_t bk_usb_cdc_modem_msg_queue = NULL;
static beken_thread_t bk_usb_cdc_modem_thread = NULL;
static beken_queue_t bk_usb_cdc_modem_msg_rx_queue = NULL;
static beken_thread_t bk_usb_cdc_modem_rx_thread = NULL;
static beken_queue_t bk_usb_cdc_modem_msg_tx_queue = NULL;
static beken_thread_t bk_usb_cdc_modem_tx_thread = NULL;

static uint8_t g_usb_cdc_modem_close = 0;

static uint8_t g_rx_buf[512] = {0};

uint8_t *bk_usb_cdc_modem_get_rxpuf(void)
{
	return  &g_rx_buf[0];
}

static bk_err_t cdc_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	BK_USB_MODEM_CDC_MSG_T msg;

	if (bk_usb_cdc_modem_msg_queue)
	{
		msg.type = type;
		msg.data = param;
		ret = rtos_push_to_queue(&bk_usb_cdc_modem_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			LOGE("bk_usb_cdc_modem_msg_queue Fail, ret:%d\n", ret);
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

static bk_err_t cdc_send_rxmsg(uint8_t type, uint32_t data_len, uint32_t *p)
{
	bk_err_t ret = kNoErr;
	BK_USB_MODEM_CDC_MSG_T msg;

	if (bk_usb_cdc_modem_msg_rx_queue)
	{
		msg.type = type;
		msg.data = data_len;
		msg.param = p;        

		ret = rtos_push_to_queue(&bk_usb_cdc_modem_msg_rx_queue, &msg, 100);//BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

static bk_err_t cdc_send_txmsg(uint8_t type, uint32_t data_len, uint32_t* p)
{
	bk_err_t ret = kNoErr;
	BK_USB_MODEM_CDC_MSG_T msg;

	if (bk_usb_cdc_modem_msg_tx_queue)
	{
		msg.type = type;
		msg.data = data_len;

        	if (data_len)
        	{
        		msg.param = os_malloc(data_len);

        		if (msg.param)
        		{
        			os_memset((uint8_t *)msg.param, 0, data_len);
        			os_memcpy((uint8_t *)msg.param, (uint8_t *)p, data_len);
        		}
        		else
        		{
        			LOGW("%s: msg %d alloc fail \n", __func__, type);
        			return BK_FAIL;
        		}
        	}
        	else
        	{
        		msg.param = (void *)p;                
        	}
             
		ret = rtos_push_to_queue(&bk_usb_cdc_modem_msg_tx_queue, &msg, BEKEN_NO_WAIT);//BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
          		if (data_len)
          		{
        			os_free(msg.param);
          		}
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

void bk_cdc_acm_bulkin_data(uint8_t *pbuf, uint16_t len)
{
	cdc_send_rxmsg(CDC_STATUS_BULKIN_DATA, len, (uint32_t *)pbuf);
}

void bk_cdc_acm_state_notify(CDC_STATUS_t * dev_state)
{
	uint32_t state = dev_state->status;
	switch(state)
	{
		case CDC_STATUS_CONN:
			LOGI("CDC_STATUS_CONN\n");
			cdc_send_msg(CDC_STATUS_CONN, dev_state->dev_cnt);
			break;
		case CDC_STATUS_DISCON:
			LOGI("CDC_STATUS_DISCON\n");
			cdc_send_msg(CDC_STATUS_DISCON, 0);
			break;
		default:
			break;
	}
}

void bk_usb_cdc_open(void)
{
	cdc_send_msg(CDC_STATUS_OPEN, 0);
}

void bk_usb_cdc_close(void)
{
	cdc_send_msg(CDC_STATUS_CLOSE, 0);
}

int32_t bk_cdc_acm_modem_write(char *p_tx, uint32_t l_tx)
{
    cdc_send_txmsg(CDC_STATUS_BULKOUT_DATA, l_tx, (uint32_t*)p_tx);
    return 0;
}

static int32_t bk_cdc_acm_modem_write_handle(char *p_tx, uint32_t l_tx)
{
	int32  ret = 0;
	if (l_tx > CDC_EXTX_MAX_SIZE) 
	{
		LOGE("[+]%s, Transbuf overflow!\r\n", __func__);
		return ret;
	}

	uint8_t segment_cnt = (l_tx + CDC_TX_MAX_SIZE - 1)/CDC_TX_MAX_SIZE;
	uint32_t ops = 0;
	int data_cnt = 0;
	uint32_t data_len = 0;

	while (1)
	{
		data_len = (l_tx-ops > CDC_TX_MAX_SIZE)? CDC_TX_MAX_SIZE: (l_tx-ops);

                ret = bk_cdc_acm_io_write_data(p_tx+ops, data_len);
		if (ret < 0)
		{
                       /// need reconnect
		}

		ops += data_len;
		data_cnt ++;
		if (data_cnt == segment_cnt)
		{
			break;
		}
	}
	
	return 0;
}

static void bk_cdc_usbh_upload_ind(uint32_t data_len, uint32_t *p_buf)
{
	uint8_t rx_buf[512] = {0};   
	os_memcpy(&rx_buf[0], p_buf, data_len);
	bk_cdc_acm_io_read();
	bk_modem_usbh_if.bk_modem_usbh_bulkin_ind(&rx_buf[0], data_len);
}

static void bk_usb_cdc_modem_thread_main(beken_thread_arg_t arg)
{
	int ret = BK_OK;
	BK_USB_MODEM_CDC_MSG_T msg;
	while (1)
	{
		ret = rtos_pop_from_queue(&bk_usb_cdc_modem_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		LOGV("[+]%s, type %d\n", __func__, msg.type);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case CDC_STATUS_OPEN:
					g_usb_cdc_modem_close = 0;
					bk_usb_cdc_open_ind(USB_HOST_MODE);
					break;
				case CDC_STATUS_CLOSE:
					g_usb_cdc_modem_close = 1;
					bk_usb_cdc_close_ind(USB_HOST_MODE);
					break;
				case CDC_STATUS_CONN:
					{
						uint32_t cnt = (uint32_t)msg.data;
						bk_modem_usbh_if.bk_modem_usbh_conn_ind(cnt);
					}
					break;
				case CDC_STATUS_DISCON:
					{
						bk_modem_usbh_if.bk_modem_usbh_disconn_ind();
						if (g_usb_cdc_modem_close == 1)
							g_usb_cdc_modem_close = 0;
					}
					break;
				default:
					break;
			}
		}
	}
}

static void bk_usb_cdc_modem_rx_thread_main(beken_thread_arg_t arg)
{
	int ret = BK_OK;
	BK_USB_MODEM_CDC_MSG_T msg;
	while (1)
	{
		ret = rtos_pop_from_queue(&bk_usb_cdc_modem_msg_rx_queue, &msg, BEKEN_WAIT_FOREVER);
		LOGV("[+]%s, type %d\n", __func__, msg.type);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case CDC_STATUS_BULKIN_DATA:
					bk_cdc_usbh_upload_ind(msg.data, msg.param);
					break;
				default:
					break;
			}
		}
	}
}

static void bk_usb_cdc_modem_tx_thread_main(beken_thread_arg_t arg)
{
	int ret = BK_OK;
	BK_USB_MODEM_CDC_MSG_T msg;
	while (1)
	{
		ret = rtos_pop_from_queue(&bk_usb_cdc_modem_msg_tx_queue, &msg, BEKEN_WAIT_FOREVER);
		LOGV("[+]%s, type %d\n", __func__, msg.type);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case CDC_STATUS_BULKOUT_DATA:
				{
					bk_cdc_acm_modem_write_handle((char *)msg.param, msg.data);                    
					break;
				}
				default:
					break;
			}
		}
		if (msg.data)
			os_free(msg.param);        
	}
}

void bk_usb_cdc_modem(void)
{
	int ret = kNoErr;

	if (bk_usb_cdc_modem_msg_queue == NULL)
	{
		ret = rtos_init_queue(&bk_usb_cdc_modem_msg_queue, "bk_usb_cdc_modem_msg_queue", sizeof(BK_USB_MODEM_CDC_MSG_T), 64);
		if (ret != kNoErr)
		{
			LOGE("init bk_usb_cdc_modem_msg_queue failed\r\n");
			goto error;
		}
	}
	if (bk_usb_cdc_modem_thread == NULL)
	{
		ret = rtos_create_thread(&bk_usb_cdc_modem_thread,
							4,
							"bk_usb_cdc_modem_thread",
							(beken_thread_function_t)bk_usb_cdc_modem_thread_main,
							4*1024,
							NULL);

		if (ret != kNoErr)
		{
			goto error;
		}
	}
	if (bk_usb_cdc_modem_msg_rx_queue == NULL)
	{
		ret = rtos_init_queue(&bk_usb_cdc_modem_msg_rx_queue, "bk_usb_cdc_modem_msg_rx_queue", sizeof(BK_USB_MODEM_CDC_MSG_T), 64);
		if (ret != kNoErr)
		{
			LOGE("init cdc_msg_queue failed\r\n");
			goto error;
		}
	}
	if (bk_usb_cdc_modem_rx_thread == NULL)
	{
		ret = rtos_create_thread(&bk_usb_cdc_modem_rx_thread,
							4,
							"bk_usb_cdc_modem_rx_thread",
							(beken_thread_function_t)bk_usb_cdc_modem_rx_thread_main,
							4*1024,
							NULL);

		if (ret != kNoErr)
		{
			goto error;
		}
	}

	if (bk_usb_cdc_modem_msg_tx_queue == NULL)
	{
		ret = rtos_init_queue(&bk_usb_cdc_modem_msg_tx_queue, "bk_usb_cdc_modem_msg_tx_queue", sizeof(BK_USB_MODEM_CDC_MSG_T), 64);
		if (ret != kNoErr)
		{
			LOGE("init bk_usb_cdc_modem_msg_tx_queue failed\r\n");
			goto error;
		}
	}
	if (bk_usb_cdc_modem_tx_thread == NULL)
	{
		ret = rtos_create_thread(&bk_usb_cdc_modem_tx_thread,
							4,
							"bk_usb_cdc_modem_tx_thread",
							(beken_thread_function_t)bk_usb_cdc_modem_tx_thread_main,
							4*1024,
							NULL);

		if (ret != kNoErr)
		{
			goto error;
		}
	}    
	return;
error:
	if (bk_usb_cdc_modem_msg_queue)
	{
		rtos_deinit_queue(&bk_usb_cdc_modem_msg_queue);
		bk_usb_cdc_modem_msg_queue = NULL;
	}
	if (bk_usb_cdc_modem_thread)
	{
		rtos_delete_thread(bk_usb_cdc_modem_thread);
		bk_usb_cdc_modem_thread = NULL;        
	}
	if (bk_usb_cdc_modem_msg_rx_queue)
	{
		rtos_deinit_queue(&bk_usb_cdc_modem_msg_rx_queue);
		bk_usb_cdc_modem_msg_rx_queue = NULL;
	}
	if (bk_usb_cdc_modem_rx_thread)
	{
		rtos_delete_thread(bk_usb_cdc_modem_rx_thread);
		bk_usb_cdc_modem_rx_thread = NULL;        
	}
	if (bk_usb_cdc_modem_msg_tx_queue)
	{
		rtos_deinit_queue(&bk_usb_cdc_modem_msg_tx_queue);
		bk_usb_cdc_modem_msg_tx_queue = NULL;
	}
	if (bk_usb_cdc_modem_tx_thread)
	{
		rtos_delete_thread(bk_usb_cdc_modem_tx_thread);
		bk_usb_cdc_modem_tx_thread = NULL;        
	}    
}

