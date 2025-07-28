/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os/os.h>
#include <os/mem.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "sys_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usbh_ch34x.h"
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/uart_types.h>
#include <components/usb.h>
#include <components/usbh_simulate_uart_api.h>
#include <components/cherryusb/usbh_core.h>
#include <../components/bk_common/include/bk_fifo.h>

#define USBH_TO_UART_TAG "usbh_t_uart"
#define USBH_TO_UART_LOGI(...) BK_LOGI(USBH_TO_UART_TAG, ##__VA_ARGS__)
#define USBH_TO_UART_LOGW(...) BK_LOGW(USBH_TO_UART_TAG, ##__VA_ARGS__)
#define USBH_TO_UART_LOGE(...) BK_LOGE(USBH_TO_UART_TAG, ##__VA_ARGS__)
#define USBH_TO_UART_LOGD(...) BK_LOGD(USBH_TO_UART_TAG, ##__VA_ARGS__)
#define USBH_TO_UART_LOGV(...) BK_LOGV(USBH_TO_UART_TAG, ##__VA_ARGS__)

#define USBTUART_FORMAT "/dev/ttyUSB%d"
#define KFIFO_RX_SIZE 1024
#define KFIFO_RX_IS_FULL_WAIT_MS 10
#define KFIFO_TX_SIZE 1024

#define FULLSPEED_BULK_BUFFER_SIZE 64
#define HIGHSPEED_BULK_BUFFER_SIZE 512

typedef enum {
	USB_SIMULATE_UART_TX = 0, 
	USB_SIMULATE_UART_TXED,
	USB_SIMULATE_UART_RX, 
	USB_SIMULATE_UART_RXED,
    USB_SIMULATE_UART_RX_KFIFO_IS_FULL,
    USB_SIMULATE_UART_DISCONNECT,
	USB_SIMULATE_UART_INVILAD
} usb_simulate_uart_task_ops;

typedef struct {
    kfifo_t *rx;
    uint32_t rx_size;
    kfifo_t *tx;
    uint32_t tx_size;
} usbh_simulate_uart_fifo_t;

typedef struct {
    usb_simulate_uart_id_t id;
    bool init_flag;
    bool connect_flag;
    struct usbh_ch34x *device;
    usbh_simulate_uart_fifo_t fifo;
    uart_config_t config;
    uint8_t rx_buffer[FULLSPEED_BULK_BUFFER_SIZE];
    beken_semaphore_t rx_semphr;
    uint8_t tx_buffer[FULLSPEED_BULK_BUFFER_SIZE];
    usb_simulate_uart_callback all_cb;
} usbh_simulate_uart_id_config_t;

typedef struct {
	uint32_t op;
	void *p;
} usbh_simulate_uart_msg_t;

typedef struct {
    uint32_t usbh_simulate_uart_driver_init;
    beken_thread_t usbh_simulate_uart_thread_hdl;
    beken_queue_t usbh_simulate_uart_msg_que;
} usbh_simulate_uart_config_t;

static usbh_simulate_uart_id_config_t s_usbh_simulate_serial_id[USB_SIMULATE_UART_ID_MAX];
static usbh_simulate_uart_config_t s_usbh_simulate_driver_config;

static bk_err_t usbh_simulate_uart_send_msg(int op, void *param)
{
	bk_err_t ret;
	usbh_simulate_uart_config_t *config = &s_usbh_simulate_driver_config;
	usbh_simulate_uart_msg_t msg;

	msg.op = op;
	msg.p = param;

	if (config->usbh_simulate_uart_msg_que) {
		ret = rtos_push_to_queue(&config->usbh_simulate_uart_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static bk_err_t usbh_simulate_uart_front_send_msg(int op, void *param)
{
	bk_err_t ret;
	usbh_simulate_uart_config_t *config = &s_usbh_simulate_driver_config;
	usbh_simulate_uart_msg_t msg;

	msg.op = op;
	msg.p = param;

	if (config->usbh_simulate_uart_msg_que) {
		ret = rtos_push_to_queue_front(&config->usbh_simulate_uart_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static void bk_usbh_to_uart_kfifo_init(usbh_simulate_uart_id_config_t *serial_id)
{
    if (serial_id->fifo.rx_size > 0) {
        if (serial_id->fifo.rx) {
            kfifo_free(serial_id->fifo.rx);
        }
        serial_id->fifo.rx = kfifo_alloc(serial_id->fifo.rx_size);
    }
    
    if (serial_id->fifo.tx_size > 0) {
        if (serial_id->fifo.tx) {
            kfifo_free(serial_id->fifo.tx);
        }
        serial_id->fifo.tx = kfifo_alloc(serial_id->fifo.tx_size);
    }
}
static void bk_usbh_to_uart_kfifo_deinit(usbh_simulate_uart_id_config_t *serial_id)
{
    if (serial_id->fifo.rx) {
        kfifo_free(serial_id->fifo.rx);
        serial_id->fifo.rx = NULL;
    }
    
    if (serial_id->fifo.tx) {
        kfifo_free(serial_id->fifo.tx);
        serial_id->fifo.tx = NULL;
    }
}

static uint32_t bk_usbh_to_uart_tx_fifo_get(usbh_simulate_uart_id_config_t *serial_id, uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (serial_id->fifo.tx) {
        size = kfifo_get(serial_id->fifo.tx, buf, len);
    }

    return size;
}

static uint32_t bk_usbh_to_uart_tx_fifo_put(usbh_simulate_uart_id_config_t *serial_id, uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (serial_id->fifo.tx) {
        size = kfifo_put(serial_id->fifo.tx, buf, len);
    }

    return size;
}

static uint32_t bk_usbh_to_uart_tx_size_get(usbh_simulate_uart_id_config_t *serial_id)
{
    uint32_t size = 0;

    if (serial_id->fifo.tx) {
        size = kfifo_data_size(serial_id->fifo.tx);
    }

    return size;
}

static void bk_usbh_to_uart_tx_fifo_clr(usbh_simulate_uart_id_config_t *serial_id)
{
    if (serial_id->fifo.tx) {
        serial_id->fifo.tx->in = 0;
        serial_id->fifo.tx->out = 0;
    }
}

static uint32_t bk_usbh_to_uart_rx_fifo_get(usbh_simulate_uart_id_config_t *serial_id, uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_enter_critical();

    if (serial_id->fifo.rx) {
        size = kfifo_get(serial_id->fifo.rx, buf, len);
    }

	rtos_exit_critical(flag);
    return size;
}

static uint32_t bk_usbh_to_uart_rx_fifo_put(usbh_simulate_uart_id_config_t *serial_id, uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_enter_critical();

    if (serial_id->fifo.rx) {
        size = kfifo_put(serial_id->fifo.rx, buf, len);
    }

	rtos_exit_critical(flag);
    return size;
}

static uint32_t bk_usbh_to_uart_rx_size_get(usbh_simulate_uart_id_config_t *serial_id)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_enter_critical();

    if (serial_id->fifo.rx) {
        size = kfifo_data_size(serial_id->fifo.rx);
    }

	rtos_exit_critical(flag);

    return size;
}

static void bk_usbh_to_uart_rx_fifo_clr(usbh_simulate_uart_id_config_t *serial_id)
{
    if (serial_id->fifo.rx) {
        // kfifo_clear(s_can_env->can_f.tx);
        serial_id->fifo.rx->in = 0;
        serial_id->fifo.rx->out = 0;
    }
}

static void bk_usbh_to_uart_simulate_rxed_callback(void *pCompleteParam, int nbytes)
{
    USBH_TO_UART_LOGV("%s pCompleteParam:%x nbytes:%d\r\n", __func__, pCompleteParam, nbytes);
    usbh_simulate_uart_id_config_t *serial_id = pCompleteParam;
    if(!serial_id->connect_flag) {
        return;
    }
    usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RXED, (void *)serial_id);
}

static bk_err_t bk_usbh_to_uart_rx_data_handler(usbh_simulate_uart_id_config_t *serial_id)
{
    if(!serial_id->connect_flag) {
        return BK_ERR_PARAM;
    }
    uint32_t rx_mps = serial_id->device->bulkin_ep_d->wMaxPacketSize;
    uint8_t *data = serial_id->rx_buffer;
    struct usbh_ch34x *device = serial_id->device;
    struct usbh_urb *urb = &device->bulkin_urb;
    usbh_pipe_t pipe = device->bulkin;

    urb->pipe = pipe;
    urb->complete = (usbh_complete_callback_t)bk_usbh_to_uart_simulate_rxed_callback;
    urb->arg = serial_id;
    urb->timeout = 0;
    urb->num_of_iso_packets = 1;
    urb->transfer_buffer = data;
    urb->transfer_buffer_length = rx_mps;
    urb->actual_length = 0;
    
    return usbh_submit_urb(urb);
}

static bk_err_t bk_usbh_to_uart_rxed_data_handler(usbh_simulate_uart_id_config_t *serial_id)
{
    if(!serial_id->connect_flag) {
        return BK_ERR_PARAM;
    }
    struct usbh_ch34x *device = serial_id->device;
    struct usbh_urb *urb = &device->bulkin_urb;
    uint8_t *data = serial_id->rx_buffer;
    usb_simulate_uart_id_t id = serial_id->id;
    usb_to_uart_cb_t rx_cb = serial_id->all_cb.rx_cb;
    void *rx_arg = serial_id->all_cb.rx_arg;

    bk_err_t ret = bk_usbh_to_uart_rx_fifo_put(serial_id, data, urb->actual_length);
    rtos_set_semaphore(&(serial_id->rx_semphr));

    if(rx_cb) {
        rx_cb(id, rx_arg);
    }

    uint32_t unused_size = 0;
    unused_size = kfifo_unused(serial_id->fifo.rx);
    uint32_t rx_mps = serial_id->device->bulkin_ep_d->wMaxPacketSize;
    if(unused_size > rx_mps) {
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RX, (void *)serial_id);
    } else {
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RX_KFIFO_IS_FULL, (void *)serial_id);
    }
    return ret;
}

static void bk_usbh_to_uart_rx_kfifo_full_handler(usbh_simulate_uart_id_config_t *serial_id)
{
    if(!serial_id->connect_flag) {
        return;
    }
    uint32_t unused_size = 0;
    unused_size = kfifo_unused(serial_id->fifo.rx);
    uint32_t rx_mps = serial_id->device->bulkin_ep_d->wMaxPacketSize;
    if(unused_size > rx_mps) {
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RX, (void *)serial_id);
    } else {
        rtos_delay_milliseconds(KFIFO_RX_IS_FULL_WAIT_MS);
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RX_KFIFO_IS_FULL, (void *)serial_id);
    }
}

bk_err_t bk_usbh_to_uart_simulate_read(usb_simulate_uart_id_t id, uint8_t *data, uint32_t cnt, uint32_t timeout_ms)
{
    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];
    if(!serial_id->connect_flag) {
        USBH_TO_UART_LOGE("%s Please check ID:%d Disconnect\r\n", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }
    uint32_t rx_size = 0, read_size = 0;
    uint32_t recv_size = 0;

    if (data == NULL || 0 == cnt || NULL == serial_id->fifo.rx) {
        return BK_ERR_PARAM;
    }

    while (cnt) {
        rx_size = bk_usbh_to_uart_rx_size_get(serial_id);
        read_size = min(rx_size, cnt);
        if (read_size) {
            bk_usbh_to_uart_rx_fifo_get(serial_id, data, read_size);

            cnt -= read_size;
            data += read_size;
            recv_size += read_size;
        }

        if ((cnt > 0) && (timeout_ms > 0)) {
            if (rtos_get_semaphore(&(serial_id->rx_semphr), timeout_ms) != BK_OK) {
                return BK_ERR_TIMEOUT;
            }
        } else {
                return recv_size;
        }
    }
    return BK_ERR_IS_FALL;
}

static void bk_usbh_to_uart_simulate_txed_callback(void *pCompleteParam, int nbytes)
{
    USBH_TO_UART_LOGV("%s pCompleteParam:%x nbytes:%d\r\n", __func__, pCompleteParam, nbytes);
    usbh_simulate_uart_id_config_t *serial_id = pCompleteParam;
    struct usbh_ch34x *device = serial_id->device;
    struct usbh_urb *urb = &device->bulkout_urb;

    if(serial_id->connect_flag) {
    } else {
        return;
    }
    usb_simulate_uart_id_t id = serial_id->id;
    usb_to_uart_cb_t tx_cb = serial_id->all_cb.tx_cb;
    void *tx_arg = serial_id->all_cb.tx_arg;

    if(urb->timeout > 0) {
        if(tx_cb) {
            tx_cb(id, tx_arg);
        }
    } else {
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_TXED, (void *)serial_id);
    }
}

static bk_err_t bk_usbh_to_uart_txed_data_handler(usbh_simulate_uart_id_config_t *serial_id)
{
    if(!serial_id->connect_flag) {
        return BK_ERR_PARAM;
    }
    usb_to_uart_cb_t tx_cb = serial_id->all_cb.tx_cb;
    void *tx_arg = serial_id->all_cb.tx_arg;

    uint32_t cnt = bk_usbh_to_uart_tx_size_get(serial_id);
    if(cnt == 0) 
    {
        if(tx_cb) {
            tx_cb(serial_id->id, tx_arg);
        }
    } else {
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_TX, (void *)serial_id);
    }
    return BK_OK;
}

static bk_err_t bk_usbh_to_uart_tx_data_handler(usbh_simulate_uart_id_config_t *serial_id)
{
    if(!serial_id->connect_flag) {
        return BK_ERR_PARAM;
    }
    uint32_t tx_mps = serial_id->device->bulkout_ep_d->wMaxPacketSize;
    uint8_t *data = serial_id->rx_buffer;
    struct usbh_ch34x *device = serial_id->device;
    struct usbh_urb *urb = &device->bulkout_urb;
    usbh_pipe_t pipe = device->bulkout;

    uint32_t cnt = bk_usbh_to_uart_tx_size_get(serial_id);

    cnt = min(cnt, tx_mps);

    bk_usbh_to_uart_tx_fifo_get(serial_id, data, cnt);

    urb->pipe = pipe;
    urb->complete = (usbh_complete_callback_t)bk_usbh_to_uart_simulate_txed_callback;
    urb->arg = serial_id;
    urb->timeout = 0;
    urb->num_of_iso_packets = 1;
    urb->transfer_buffer = data;
    urb->transfer_buffer_length = cnt;
    urb->actual_length = 0;
        
    return usbh_submit_urb(urb);
}

bk_err_t bk_usbh_to_uart_simulate_write(usb_simulate_uart_id_t id, uint8_t *data, uint32_t cnt, uint32_t timeout_ms)
{
    if(!(id < USB_SIMULATE_UART_ID_MAX)) {
        USBH_TO_UART_LOGE("%s Please check ID:%d\r\n", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }

    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];

    if(!serial_id->connect_flag) {
        USBH_TO_UART_LOGE("%s Please check ID:%d Disconnect\r\n", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }

    if(timeout_ms > 0) {
        struct usbh_ch34x *device = serial_id->device;
        struct usbh_urb *urb = &device->bulkout_urb;
        usbh_pipe_t pipe = device->bulkout;
        urb->pipe = pipe;
        urb->complete = (usbh_complete_callback_t)bk_usbh_to_uart_simulate_txed_callback;
        urb->arg = serial_id;
        urb->timeout = timeout_ms;
        urb->num_of_iso_packets = 1;
        urb->transfer_buffer = data;
        urb->transfer_buffer_length = cnt;
        urb->actual_length = 0;
        
        return usbh_submit_urb(urb);
    } else {
        uint32_t unused_size = 0;
        unused_size = kfifo_unused(serial_id->fifo.tx);
        if(unused_size < cnt) {
            USBH_TO_UART_LOGE("%s Please check ID:%d TXFIFO is FULL\r\n", __func__, id);
           return BK_ERR_NO_MEM;
        }
        bk_usbh_to_uart_tx_fifo_put(serial_id, data, cnt);
        usbh_simulate_uart_send_msg(USB_SIMULATE_UART_TX, (void *)serial_id);
    }
    return BK_OK;
}

void usbh_to_uart_connect_disconnect_api(uint32_t connect_flag, void *class)
{
    usbh_simulate_uart_id_config_t *serial_id = NULL;
    usb_simulate_uart_callback *all_cb = NULL;
    usb_to_uart_cb_t c_cb = NULL;
    void *c_arg = NULL;
    usb_to_uart_cb_t d_cb = NULL;
    void *d_arg = NULL;
    char devname[CONFIG_USBHOST_DEV_NAMELEN];

    for(usb_simulate_uart_id_t id = USB_SIMULATE_UART_ID_0; id < USB_SIMULATE_UART_ID_MAX; id++)
    {
        serial_id = &s_usbh_simulate_serial_id[id];
        serial_id->id = id;
        all_cb = &serial_id->all_cb;
        c_cb = all_cb->connect_cb;
        c_arg = all_cb->connect_arg;
        d_cb = all_cb->disconnect_cb;
        d_arg = all_cb->disconnect_arg;
        snprintf(devname, CONFIG_USBHOST_DEV_NAMELEN, USBTUART_FORMAT, id);
        struct usbh_ch34x *device =  usbh_find_class_instance(devname);
        if(device == class) {
            serial_id->device = device;
            if(connect_flag) {
                serial_id->connect_flag = 1;
                if(c_cb) c_cb(id, c_arg);
                usbh_simulate_uart_send_msg(USB_SIMULATE_UART_RX, (void *)serial_id);
            }
        } else {
            if(!connect_flag) {
                if(d_cb) d_cb(id, d_arg);
                
                if(serial_id->connect_flag) {
                    usbh_simulate_uart_front_send_msg(USB_SIMULATE_UART_DISCONNECT, (void *)serial_id);
                    serial_id->connect_flag = 0;
                }
            }
        }
    }
}

bk_err_t bk_usbh_to_uart_simulate_register_callback(usb_simulate_uart_id_t id, usb_simulate_uart_callback *cb_p)
{
    bk_err_t ret = BK_OK;
    usb_simulate_uart_callback *all_cb;

    if((id < USB_SIMULATE_UART_ID_MAX) && (cb_p != NULL)) {
        all_cb = &s_usbh_simulate_serial_id[id].all_cb;

        all_cb->rx_cb = cb_p->rx_cb;
        all_cb->rx_arg = cb_p->rx_arg;
        all_cb->tx_cb = cb_p->tx_cb;
        all_cb->tx_arg = cb_p->tx_arg;
        all_cb->connect_cb = cb_p->connect_cb;
        all_cb->connect_arg = cb_p->connect_arg;
        all_cb->disconnect_cb = cb_p->disconnect_cb;
        all_cb->disconnect_arg = cb_p->disconnect_arg;
        ret = BK_OK;
    } else {
        USBH_TO_UART_LOGE("%s Please check ID:%d or cb_p:%x\r\n", __func__, id, cb_p);
        ret = BK_ERR_UART_INVALID_ID;
    }

    return ret;
}

static void usbh_simulate_uart_handler(void *p)
{
    usbh_simulate_uart_config_t *config = p;
	bk_err_t ret;
    usbh_simulate_uart_id_config_t *serial_id = NULL;

	while (1) {
		usbh_simulate_uart_msg_t msg;
		ret = rtos_pop_from_queue(&config->usbh_simulate_uart_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_SIMULATE_UART_TX:
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_tx_data_handler(serial_id);
					break;
				case USB_SIMULATE_UART_TXED:
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_txed_data_handler(serial_id);
					break;
                case USB_SIMULATE_UART_RX:
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_rx_data_handler(serial_id);
					break;
				case USB_SIMULATE_UART_RXED:
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_rxed_data_handler(serial_id);
					break;
                case USB_SIMULATE_UART_RX_KFIFO_IS_FULL:
                    USBH_TO_UART_LOGW("%s USB_SIMULATE_UART_RX_KFIFO_IS_FULL\r\n", __func__);
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_rx_kfifo_full_handler(serial_id);
                    break;
                case USB_SIMULATE_UART_DISCONNECT:
                    rtos_reset_queue(&config->usbh_simulate_uart_msg_que);
                    serial_id = (usbh_simulate_uart_id_config_t *)msg.p;
                    bk_usbh_to_uart_tx_fifo_clr(serial_id);
                    bk_usbh_to_uart_rx_fifo_clr(serial_id);
                    break;
				default:
					break;
			}
		}
	}
}

bk_err_t bk_usbh_to_uart_simulate_driver_init(usb_simulate_uart_id_t id)
{
    if(!(id < USB_SIMULATE_UART_ID_MAX)) {
        USBH_TO_UART_LOGE("%s Please check ID:%d", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }
    usbh_simulate_uart_config_t *config = &s_usbh_simulate_driver_config;
    if(config->usbh_simulate_uart_driver_init) {
        config->usbh_simulate_uart_driver_init |= (0x1 << id);
        USBH_TO_UART_LOGE("usbh_simulate_uart_driver_init:%x \r\n", config->usbh_simulate_uart_driver_init);
        return BK_OK;
    }
    usbh_class_serial_ch340_register_driver();

    usbh_class_serial_ch340_register_disc_cb((usbh_simulate_uart_con_disc_cb_t)usbh_to_uart_connect_disconnect_api);

    uint32_t ret = BK_OK;
    ret = bk_usb_open(USB_HOST_MODE);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s Please USB OPEN FAIL ret:%d\r\n", __func__, ret);
        return ret;
    }

	if ((!config->usbh_simulate_uart_thread_hdl) && (!config->usbh_simulate_uart_msg_que)) {
        ret = rtos_init_queue(&config->usbh_simulate_uart_msg_que, "usb_t_uart_q", sizeof(usbh_simulate_uart_msg_t), 10);
        if (ret != kNoErr) {
            USBH_TO_UART_LOGE("ceate can demo message queue fail \r\n");
            return ret;
        }

        ret = rtos_create_thread(&config->usbh_simulate_uart_thread_hdl, 4, "usb_t_uart", usbh_simulate_uart_handler, 2048, (void *)config);
		if (ret != kNoErr) {
			USBH_TO_UART_LOGE("create usb to uart task fail \r\n");
			rtos_deinit_queue(&config->usbh_simulate_uart_msg_que);
			config->usbh_simulate_uart_thread_hdl = NULL;
			config->usbh_simulate_uart_msg_que = NULL;
            return ret;
		}
	}

    bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
    config->usbh_simulate_uart_driver_init |= (0x1 << id);
    return BK_OK;
}

bk_err_t bk_usbh_to_uart_simulate_driver_deinit(usb_simulate_uart_id_t id)
{
    if(!(id < USB_SIMULATE_UART_ID_MAX)) {
        USBH_TO_UART_LOGE("%s Please check ID:%d\r\n", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }

    usbh_simulate_uart_config_t *config = &s_usbh_simulate_driver_config;
    config->usbh_simulate_uart_driver_init &= ~(0x1 << id);

    if(config->usbh_simulate_uart_driver_init) {
        return BK_OK;
    }
    bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 0);

    uint32_t ret = BK_OK;

    if(config->usbh_simulate_uart_thread_hdl)
    {
        rtos_delete_thread(&config->usbh_simulate_uart_thread_hdl);
        config->usbh_simulate_uart_thread_hdl = NULL;
    }
    if(config->usbh_simulate_uart_msg_que) {
        rtos_deinit_queue(&config->usbh_simulate_uart_msg_que);
        config->usbh_simulate_uart_msg_que = NULL;
    }

    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];
    uint32_t wait_disconnect_count = 5;
    uint32_t software_trigger_disconnect = 1;
    while(serial_id->connect_flag) {
        wait_disconnect_count--;
        if(wait_disconnect_count == 0){
            USBH_TO_UART_LOGE("%s wait disconnect timeout\r\n", __func__);
            extern void usbh_musb_trigger_disconnect_by_sw();
            usbh_musb_trigger_disconnect_by_sw();
            wait_disconnect_count = 5;
            software_trigger_disconnect--;
        }

        if((wait_disconnect_count == 0)&&(software_trigger_disconnect == 0))
        {
            USBH_TO_UART_LOGE("%s software disconnect timeout\r\n", __func__);
            return BK_FAIL;
        }
        rtos_delay_milliseconds(100);
    }

    ret = bk_usb_close();
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s Please USB CLOSE FAIL ret:%d\r\n", __func__, ret);
        return ret;
    }

    return BK_OK;
}

bk_err_t bk_usbh_to_uart_init(usb_simulate_uart_id_t id, const uart_config_t *config)
{
    bk_err_t ret = BK_OK;
    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];
    serial_id->fifo.rx_size = KFIFO_RX_SIZE;
    serial_id->fifo.tx_size = KFIFO_TX_SIZE;

  
    ret = rtos_init_semaphore(&(serial_id->rx_semphr), 1);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s init rx sem fail ret:%d\r\n", __func__, ret);
         return ret;
    }

    bk_usbh_to_uart_kfifo_init(serial_id);

    serial_id->config.data_bits = config->data_bits;
    serial_id->config.parity = config->parity;          /**< UART parity */
    serial_id->config.stop_bits = config->stop_bits;    /**< UART stop bits */
    serial_id->config.flow_ctrl = config->flow_ctrl; /**< UART flow control  */
    serial_id->config.src_clk = config->src_clk;        /**< UART source clock */
    serial_id->config.rx_dma_en = config->rx_dma_en;
    serial_id->config.tx_dma_en = config->tx_dma_en;

    return BK_OK;
}

bk_err_t bk_usbh_to_uart_deinit(usb_simulate_uart_id_t id)
{
    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];
    if(&(serial_id->rx_semphr))
        rtos_deinit_semaphore(&(serial_id->rx_semphr));

    bk_usbh_to_uart_kfifo_deinit(serial_id);
    return BK_OK;
}

bk_err_t bk_usbh_to_uart_simulate_init(usb_simulate_uart_id_t id, const uart_config_t *config)
{
    bk_err_t ret = BK_OK;
    ret = bk_usbh_to_uart_simulate_driver_init(id);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s driver_init fail ret:%d\r\n", __func__, ret);
        return ret;
    }

    ret = bk_usbh_to_uart_init(id, config);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s init fail ret:%d\r\n", __func__, ret);
        return ret;
    }

    return BK_OK;
}

bk_err_t bk_usbh_to_uart_simulate_deinit(usb_simulate_uart_id_t id)
{
    bk_err_t ret = BK_OK;

    ret = bk_usbh_to_uart_simulate_driver_deinit(id);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s driver_init fail ret:%d\r\n", __func__, ret);
        return ret;
    }

    ret = bk_usbh_to_uart_deinit(id);
    if(ret != BK_OK) {
        USBH_TO_UART_LOGE("%s init fail ret:%d\r\n", __func__, ret);
        return ret;
    }

    return BK_OK;
}

bk_err_t bk_usbh_to_uart_simulate_set_baud_rate(usb_simulate_uart_id_t id, uint32_t baud_rate)
{
    bk_err_t ret = BK_OK;
    usbh_simulate_uart_id_config_t *serial_id = &s_usbh_simulate_serial_id[id];
    struct usbh_ch34x *device =  serial_id->device;

    if(!serial_id->connect_flag) return BK_FAIL;

    device->line_coding.dwDTERate = baud_rate;
    device->line_coding.bDataBits = serial_id->config.data_bits + 5;
    device->line_coding.bCharFormat = 0;
    device->line_coding.bParityType = serial_id->config.parity;

    ret = usbh_ch34x_set_line_coding(device, &device->line_coding);
    if(ret < 0) {
        USBH_TO_UART_LOGE("%sset line coding fail ret:%d\r\n", __func__, ret);
        return ret;
    }
    return BK_OK;
}

bk_err_t bk_usbh_to_uart_simulate_check_device(usb_simulate_uart_id_t id)
{
    if(!(id < USB_SIMULATE_UART_ID_MAX)) {
        USBH_TO_UART_LOGE("%s Please check ID:%d\r\n", __func__, id);
        return BK_ERR_UART_INVALID_ID;
    }
    usbh_simulate_uart_id_config_t *serial_id = NULL;
    serial_id = &s_usbh_simulate_serial_id[id];

    if(!serial_id->connect_flag) {
        USBH_TO_UART_LOGE("%s Please check connect_flag:%d\r\n", __func__, serial_id->connect_flag);
        return BK_ERR_UART_ID_NOT_INIT;
    }

    struct usbh_ch34x *device = serial_id->device;

    char devname[CONFIG_USBHOST_DEV_NAMELEN];
    snprintf(devname, CONFIG_USBHOST_DEV_NAMELEN, USBTUART_FORMAT, id);
    struct usbh_ch34x *class =  usbh_find_class_instance(devname); 
    if(device == class) {
        return BK_OK;
    } else {

        return BK_ERR_UART_ID_NOT_INIT;
    }

    return BK_FAIL;
}
