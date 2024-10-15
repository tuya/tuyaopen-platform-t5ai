#include <os/os.h>
#include <os/mem.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "usbh_video.h"
#include "usbh_audio.h"
#include "usb_driver.h"
#include <driver/media_types.h>
#include <driver/audio_ring_buff.h>
#include "bk_uvc_uac_api.h"

#include <driver/gpio.h>

#if CONFIG_MENTOR_USB
#include <modules/bk_musb.h>
#include <components/usb_audio.h>

typedef struct
{
	bk_usbh_video_device *uvc_dev;
	bk_usbh_audio_device *uac_dev;
	bk_usb_driver_comprehensive_ops *usb_driver;
	bk_uvc_uac_app_callback *uvc_all_cb;
	bk_uvc_uac_app_callback *uac_all_cb;
	camera_packet_t *uvc_packet;
	camera_packet_control_t *usb_uvc_buffer_ops_funcs;

	struct audio_packet_control_t *usb_uac_buffer_ops_funcs;
	RingBufferContext *audio_mic_rb;
	uint8_t *mic_ring_buffer;
	uint8_t *mic_shuttle_buffer;
	uint16_t mic_length[1];
	uint16_t mic_actual_length[1];
	uint8_t mic_awstatus[1];
	RingBufferContext *audio_spk_rb;
	uint8_t *spk_ring_buffer;
	uint8_t *spk_shuttle_buffer;
	uint16_t spk_length[1];
	uint16_t spk_actual_length[1];
	uint8_t spk_awstatus[1];
} bk_uvc_uac_fusion_device;

static bk_uvc_uac_fusion_device *s_uvc_uac_device = {0};
static bool s_uvc_uac_sw_init_status = 0;

static const uint8_t uvc_uac_periphera_list[] =
{
    MUSB_TARGET_CLASS, MUSB_CLASS_MISC,
    MUSB_TARGET_INTERFACE, 0,
    MUSB_TARGET_CLASS, MUSB_CLASS_VIDEO,
    MUSB_TARGET_SUBCLASS, 1,
    MUSB_TARGET_ACCEPT,
    0 /* placeholder for driver index */
};

static bk_err_t bk_uvc_uac_updata_video_stream_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);
static bk_err_t bk_uvc_uac_updata_audio_stream_rx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);
static bk_err_t bk_uvc_uac_updata_audio_stream_tx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);

static uint8_t bk_uvc_uac_demo_connect(void     *pPrivateData, bk_usb_bus_handle hBus, bk_usb_device *pUsbDevice, const uint8_t *pPeripheralList)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_usb_driver_comprehensive_ops *usb_driver = uvc_uac_device->usb_driver;
	bk_usbh_video_device *uvc_device = uvc_uac_device->uvc_dev;
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;

	usb_driver->usb_bus = hBus;
	usb_driver->usb_device = pUsbDevice;
	uvc_device->usb_driver = usb_driver;
	uac_device->usb_driver = usb_driver;
	bk_usbh_video_sw_init(uvc_device);
	bk_usbh_audio_sw_init(uac_device);

	bk_usb_drv_send_msg(USB_DRV_PRASE_DESCRIPTOR, (void *)usb_driver);
	bk_usb_drv_send_msg(USB_DRV_INIT_CONTROL_TRANS, (void *)usb_driver);
	bk_usb_drv_send_msg(USB_DRV_VIDEO_OPEN_PIPE, (void *)uvc_device);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_OPEN_PIPE, (void *)uac_device);
	bk_usb_drv_send_msg(USB_DRV_CONNECT_HANDLE, (void *)usb_driver);

	/* Configure the device */
	return BK_OK;
}

static void bk_uvc_uac_flush_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return;
	}

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs) {
		if(uvc_uac_device->usb_uvc_buffer_ops_funcs->free && uvc_uac_device->uvc_packet) {
			uvc_uac_device->usb_uvc_buffer_ops_funcs->free(uvc_uac_device->uvc_packet);
			uvc_uac_device->uvc_packet = NULL;
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

}

static void bk_uvc_uac_demo_disconnect (void *pPrivateData, bk_usb_bus_handle hBus, bk_usb_device *pUsbDevice)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	bk_uvc_uac_flush_buffer(uvc_uac_device);

	bk_usb_video_close_pipe(uvc_uac_device->uvc_dev);
	bk_usb_audio_close_pipe(uvc_uac_device->uac_dev);

	bk_usb_drv_send_msg_front(USB_DRV_DISCONNECT_HANDLE, (void *)uvc_uac_device->usb_driver);

	uvc_uac_device->usb_driver->usbh_class_start_status = 0x0;
	uvc_uac_device->usb_driver->usbh_class_connect_status = 0x0;

	bk_usbh_video_sw_deinit(uvc_uac_device->uvc_dev);
	bk_usbh_audio_sw_deinit(uvc_uac_device->uac_dev);

	return;
}

const struct bk_usb_class_driver s_video_audio_intf_driver = {
    .peripheral_list = uvc_uac_periphera_list,
    .peripheral_list_length = (uint16_t)sizeof(uvc_uac_periphera_list),
    .driver_name = "video_audio",
    .driver_index = 0,
    .device_connected = bk_uvc_uac_demo_connect,
    .device_disconnected = bk_uvc_uac_demo_disconnect,
    .device_process = NULL
};

bk_err_t bk_usb_uvc_uac_sw_init(void *set_config)
{
	if(s_uvc_uac_sw_init_status) {
		USB_DRIVER_LOGD("%s INITED\r\n", __func__);
		return BK_FAIL;
	}
	bk_usb_register_class_driver(MUSB_PORT_TYPE_HOST, (void *)&s_video_audio_intf_driver);

	bool malloc_success_flag = 0;

	do{
		s_uvc_uac_device = (bk_uvc_uac_fusion_device *)os_malloc(sizeof(bk_uvc_uac_fusion_device));
		if(!s_uvc_uac_device) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device, 0x0, sizeof(bk_uvc_uac_fusion_device));

		s_uvc_uac_device->uvc_dev = (bk_usbh_video_device *)os_malloc(sizeof(bk_usbh_video_device));
		if(!s_uvc_uac_device->uvc_dev) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uvc_dev, 0x0, sizeof(bk_usbh_video_device));

		s_uvc_uac_device->uvc_dev->uvc_isoch_irp = (struct bk_usb_isoch_irp *)os_malloc(sizeof(struct bk_usb_isoch_irp));
		if(!s_uvc_uac_device->uvc_dev->uvc_isoch_irp) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uvc_dev, 0x0, sizeof(struct bk_usb_isoch_irp));

		s_uvc_uac_device->uac_dev = (bk_usbh_audio_device *)os_malloc(sizeof(bk_usbh_audio_device));
		if(!s_uvc_uac_device->uac_dev) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_dev, 0x0, sizeof(bk_usbh_audio_device));

		s_uvc_uac_device->uac_dev->uac_mic_isoch_irp = (struct bk_usb_isoch_irp *)os_malloc(sizeof(struct bk_usb_isoch_irp));
		if(!s_uvc_uac_device->uac_dev->uac_mic_isoch_irp) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_dev->uac_mic_isoch_irp, 0x0, sizeof(struct bk_usb_isoch_irp));

		s_uvc_uac_device->uac_dev->uac_spk_isoch_irp = (struct bk_usb_isoch_irp *)os_malloc(sizeof(struct bk_usb_isoch_irp));
		if(!s_uvc_uac_device->uac_dev->uac_spk_isoch_irp) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_dev->uac_spk_isoch_irp, 0x0, sizeof(struct bk_usb_isoch_irp));

		s_uvc_uac_device->usb_driver = (bk_usb_driver_comprehensive_ops *)os_malloc(sizeof(bk_usb_driver_comprehensive_ops));
		if(!s_uvc_uac_device->usb_driver) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->usb_driver, 0x0, sizeof(bk_usb_driver_comprehensive_ops));

		s_uvc_uac_device->uvc_all_cb = (bk_uvc_uac_app_callback *)os_malloc(sizeof(bk_uvc_uac_app_callback));
		if(!s_uvc_uac_device->uvc_all_cb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uvc_all_cb, 0x0, sizeof(bk_uvc_uac_app_callback));

		s_uvc_uac_device->uac_all_cb = (bk_uvc_uac_app_callback *)os_malloc(sizeof(bk_uvc_uac_app_callback));
		if(!s_uvc_uac_device->uac_all_cb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_all_cb, 0x0, sizeof(bk_uvc_uac_app_callback));

		s_uvc_uac_device->audio_mic_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if(!s_uvc_uac_device->audio_mic_rb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->audio_mic_rb, 0x0, sizeof(RingBufferContext));
	
		s_uvc_uac_device->audio_spk_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if(!s_uvc_uac_device->audio_spk_rb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->audio_spk_rb, 0x0, sizeof(RingBufferContext));

	} while(0);


	if(malloc_success_flag) {
		if(s_uvc_uac_device->uvc_dev->uvc_isoch_irp) {
			os_free(s_uvc_uac_device->uvc_dev->uvc_isoch_irp);
			s_uvc_uac_device->uvc_dev->uvc_isoch_irp = NULL;
		}
		if(s_uvc_uac_device->uvc_dev) {
			os_free(s_uvc_uac_device->uvc_dev);
			s_uvc_uac_device->uvc_dev = NULL;
		}

		if(s_uvc_uac_device->uac_dev->uac_mic_isoch_irp) {
			os_free(s_uvc_uac_device->uac_dev->uac_mic_isoch_irp);
			s_uvc_uac_device->uac_dev->uac_mic_isoch_irp = NULL;
		}
		if(s_uvc_uac_device->uac_dev->uac_spk_isoch_irp) {
			os_free(s_uvc_uac_device->uac_dev->uac_spk_isoch_irp);
			s_uvc_uac_device->uac_dev->uac_spk_isoch_irp = NULL;
		}
		if(s_uvc_uac_device->uac_dev) {
			os_free(s_uvc_uac_device->uac_dev);
			s_uvc_uac_device->uac_dev = NULL;
		}
		
		if(s_uvc_uac_device->usb_driver) {
			os_free(s_uvc_uac_device->usb_driver);
			s_uvc_uac_device->usb_driver = NULL;
		}

		if(s_uvc_uac_device->uvc_all_cb) {
			os_free(s_uvc_uac_device->uvc_all_cb);
			s_uvc_uac_device->uvc_all_cb = NULL;
		}

		if(s_uvc_uac_device->uac_all_cb) {
			os_free(s_uvc_uac_device->uac_all_cb);
			s_uvc_uac_device->uac_all_cb = NULL;
		}

		if(s_uvc_uac_device->audio_mic_rb) {
					os_free(s_uvc_uac_device->audio_mic_rb);
					s_uvc_uac_device->audio_mic_rb = NULL;
		}

		if(s_uvc_uac_device->audio_spk_rb) {
			os_free(s_uvc_uac_device->audio_spk_rb);
			s_uvc_uac_device->audio_spk_rb = NULL;
		}

		if(s_uvc_uac_device) {
			os_free(s_uvc_uac_device);
			s_uvc_uac_device = NULL;
		}
		return BK_FAIL;
	}
	s_uvc_uac_device->uvc_packet = NULL;
	s_uvc_uac_device->usb_uvc_buffer_ops_funcs = NULL;
	s_uvc_uac_sw_init_status = 1;

	return BK_OK;

}

bk_err_t bk_usb_uvc_uac_sw_deinit()
{
	if(!s_uvc_uac_sw_init_status) {
		USB_DRIVER_LOGD("%s DEINITED\r\n", __func__);
		return BK_FAIL;
	}

	if(s_uvc_uac_sw_init_status) {
		s_uvc_uac_device->uvc_packet = NULL;

		if(s_uvc_uac_device->uvc_dev->uvc_isoch_irp) {
			os_free(s_uvc_uac_device->uvc_dev->uvc_isoch_irp);
			s_uvc_uac_device->uvc_dev->uvc_isoch_irp = NULL;
		}
		if(s_uvc_uac_device->uvc_dev) {
			os_free(s_uvc_uac_device->uvc_dev);
			s_uvc_uac_device->uvc_dev = NULL;
		}

		if(s_uvc_uac_device->uac_dev->uac_mic_isoch_irp) {
			os_free(s_uvc_uac_device->uac_dev->uac_mic_isoch_irp);
			s_uvc_uac_device->uac_dev->uac_mic_isoch_irp = NULL;
		}
		if(s_uvc_uac_device->uac_dev->uac_spk_isoch_irp) {
			os_free(s_uvc_uac_device->uac_dev->uac_spk_isoch_irp);
			s_uvc_uac_device->uac_dev->uac_spk_isoch_irp = NULL;
		}

		if(s_uvc_uac_device->uac_dev) {
			os_free(s_uvc_uac_device->uac_dev);
			s_uvc_uac_device->uac_dev = NULL;
		}
		
		if(s_uvc_uac_device->usb_driver) {
			os_free(s_uvc_uac_device->usb_driver);
			s_uvc_uac_device->usb_driver = NULL;
		}

		if(s_uvc_uac_device->uvc_all_cb) {
			os_free(s_uvc_uac_device->uvc_all_cb);
			s_uvc_uac_device->uvc_all_cb = NULL;
		}

		if(s_uvc_uac_device->uac_all_cb) {
			os_free(s_uvc_uac_device->uac_all_cb);
			s_uvc_uac_device->uac_all_cb = NULL;
		}

		if(s_uvc_uac_device) {
			os_free(s_uvc_uac_device);
			s_uvc_uac_device = NULL;
		}
	}
	bk_usb_unregister_class_driver(MUSB_PORT_TYPE_HOST, (void *)&s_video_audio_intf_driver);

	s_uvc_uac_sw_init_status = 0;

	return BK_OK;

}

bk_err_t bk_uvc_uac_register_callback(bk_uvc_uac_app_callback *uvc_uac_all_cb)
{
	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;
	bk_usb_driver_comprehensive_ops *usb_driver = device->usb_driver;
	bk_uvc_uac_app_callback *cb = uvc_uac_all_cb;

	switch (cb->dev) {
		case USB_UVC_DEVICE:
			os_memcpy((void *)device->uvc_all_cb, cb, sizeof(bk_uvc_uac_app_callback));
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_DISCONNECT, cb->dev, cb->uvc_uac_disconnect_cb);
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_CONNECT, cb->dev, cb->uvc_uac_connect_cb);
			break;
		case USB_UAC_MIC_DEVICE:
		case USB_UAC_SPEAKER_DEVICE:
			os_memcpy((void *)device->uac_all_cb, cb, sizeof(bk_uvc_uac_app_callback));
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_DISCONNECT, cb->dev, cb->uvc_uac_disconnect_cb);
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_CONNECT, cb->dev, cb->uvc_uac_connect_cb);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_usb_uvc_register_disconnect_callback(void *param)
{
	bk_uvc_uac_app_callback *uvc_all_cb = s_uvc_uac_device->uvc_all_cb;
	uvc_all_cb->dev = USB_UVC_DEVICE;
	uvc_all_cb->uvc_uac_disconnect_cb = param;
	bk_uvc_uac_register_callback(uvc_all_cb);

	return BK_OK;
}

bk_err_t bk_usb_uvc_register_connect_callback(void *param)
{
	bk_uvc_uac_app_callback *uvc_all_cb = s_uvc_uac_device->uvc_all_cb;
	uvc_all_cb->dev = USB_UVC_DEVICE;
	uvc_all_cb->uvc_uac_connect_cb = param;
	bk_uvc_uac_register_callback(uvc_all_cb);

	return BK_OK;
}

static uint32_t bk_uvc_video_stream_rxed_callback(void *pCompleteParam, struct bk_usb_isoch_irp *pIrp)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = (bk_uvc_uac_fusion_device *)pCompleteParam;
	bk_usbh_video_device *uvc_device = uvc_uac_device->uvc_dev;
	USB_DRIVER_LOGD("[+]%s \r\n", __func__);

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs == NULL)
		return BK_FAIL;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		if(uvc_uac_device->usb_uvc_buffer_ops_funcs->free) {
			uvc_uac_device->usb_uvc_buffer_ops_funcs->free(uvc_uac_device->uvc_packet);
			uvc_uac_device->uvc_packet = NULL;
		}
		return BK_FAIL;
	}

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs->push) {
		uvc_uac_device->usb_uvc_buffer_ops_funcs->push(uvc_uac_device->uvc_packet);
		uvc_uac_device->uvc_packet = NULL;
	}

	bk_uvc_uac_updata_video_stream_buffer(uvc_uac_device);
	bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
	USB_DRIVER_LOGD("[-]%s \r\n", __func__);

    return BK_OK;
}

static bk_err_t bk_uvc_uac_updata_video_stream_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	bk_usbh_video_device *uvc_device = uvc_uac_device->uvc_dev;
	struct bk_usb_isoch_irp   *pIsochIrp = uvc_device->uvc_isoch_irp;
	uint32_t        wIndex;
	bk_usb_bulk_irp *vsIrp = &uvc_device->uvc_irp;

	USB_DRIVER_LOGD("[+]%s \r\n", __func__);

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs) {
		if(uvc_uac_device->uvc_packet == NULL)
			uvc_uac_device->uvc_packet = uvc_uac_device->usb_uvc_buffer_ops_funcs->malloc();
	} else
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	camera_packet_t *packet = uvc_uac_device->uvc_packet;

	if(!packet) {
		USB_DRIVER_LOGE("%s packet is NULL\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	USB_DRIVER_LOGD("%s bmAttributes %d\r\n", __func__, uvc_device->uvc_vs_ep_desc->bmAttributes);

	if((uvc_device->uvc_vs_ep_desc->bmAttributes & BK_USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_ISOCH_TRANSFER){
		pIsochIrp->pBuffer = packet->data_buffer;
		uvc_device->vs_data_buffer = packet->data_buffer;
		pIsochIrp->wFrameCount = packet->num_packets;
		pIsochIrp->adwActualLength = packet->actual_num_byte;
		pIsochIrp->adwStatus = packet->state;
		pIsochIrp->adwLength = packet->num_byte;
	    for (wIndex = 0; wIndex < pIsochIrp->wFrameCount; wIndex++)
	    {
	       pIsochIrp->adwLength[wIndex] = uvc_device->uvc_vs_ep_desc->wMaxPacketSize;
	       pIsochIrp->adwActualLength[wIndex] = 0;
	       pIsochIrp->adwStatus[wIndex] = 0;
	    }
	    pIsochIrp->hPipe = (bk_usb_pipe_ptr)uvc_device->uvc_vs_pipe_ptr;
	    pIsochIrp->wCurrentFrame    = 0;
	    pIsochIrp->wCallbackInterval = 0; /* Callback only when data transfer is completed */
	    pIsochIrp->pfIrpComplete    = (bk_usb_isoch_irp_complete)bk_uvc_video_stream_rxed_callback;
	    pIsochIrp->pCompleteParam   = (void *)uvc_uac_device;
	    pIsochIrp->bIsrCallback		= 0;
	    pIsochIrp->bAllowDma		= 1;
	} else if((uvc_device->uvc_vs_ep_desc->bmAttributes & BK_USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_BULK_TRANSFER){
		vsIrp->hPipe = (bk_usb_pipe_ptr)uvc_device->uvc_vs_pipe_ptr;
	    vsIrp->dwStatus = 0;
	    vsIrp->dwActualLength = 0;
	    vsIrp->pfIrpComplete = (bk_usb_irp_complete)bk_uvc_video_stream_rxed_callback;
	    vsIrp->pCompleteParam = (void *)uvc_uac_device;
	    vsIrp->bAllowShortTransfer = 1;
	    vsIrp->bIsrCallback = 0;
	    vsIrp->bAllowDma = 1;
	} else
		return BK_FAIL;

	USB_DRIVER_LOGD("[-]%s \r\n", __func__);
	return BK_OK;
}

bk_err_t bk_uvc_start(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s uvc disconnect \r\n",__func__);
		return BK_FAIL;
	}

	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE)) {
		USB_DRIVER_LOGE("%s is start\r\n",__func__);
		return BK_FAIL;
	}
	bk_usbh_video_device *uvc_device = uvc_uac_device->uvc_dev;

	if(!bk_uvc_uac_updata_video_stream_buffer(uvc_uac_device)) {
		uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UVC_DEVICE);
		bk_usb_drv_send_msg(USB_DRV_VIDEO_START, (void *)uvc_device);
		bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
	} else {
		USB_DRIVER_LOGE("%s check register buffer ops?\r\n",__func__);
		return BK_FAIL;
	}
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uvc_stop(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s uvc disconnect\r\n",__func__);
		return BK_FAIL;
	}

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UVC_DEVICE);

	bk_usbh_video_device *uvc_device = uvc_uac_device->uvc_dev;
	bk_usb_drv_send_msg(USB_DRV_VIDEO_STOP, (void *)uvc_device);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return BK_OK;
}

bk_err_t bk_usb_uac_register_disconnect_callback(void *param)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_uvc_uac_app_callback *uac_all_cb = s_uvc_uac_device->uac_all_cb;
	uac_all_cb->dev = USB_UAC_MIC_DEVICE;
	uac_all_cb->uvc_uac_disconnect_cb = param;
	bk_uvc_uac_register_callback(uac_all_cb);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_uac_register_connect_callback(void *param)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_uvc_uac_app_callback *uac_all_cb = s_uvc_uac_device->uac_all_cb;
	uac_all_cb->dev = USB_UAC_MIC_DEVICE;
	uac_all_cb->uvc_uac_connect_cb = param;
	bk_uvc_uac_register_callback(uac_all_cb);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

static uint32_t bk_usbh_audio_stream_rxed_callback(void *pCompleteParam, struct bk_usb_isoch_irp *pIrp)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = (bk_uvc_uac_fusion_device *)pCompleteParam;
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	audio_packet_t packet;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		ring_buffer_clear(uvc_uac_device->audio_mic_rb);
		if(uvc_uac_device->audio_mic_rb->address) {
			os_free(uvc_uac_device->audio_mic_rb->address);
			uvc_uac_device->audio_mic_rb->address = NULL;
			os_free(uvc_uac_device->mic_ring_buffer);
			uvc_uac_device->mic_ring_buffer = NULL;
		}
		if(uvc_uac_device->mic_shuttle_buffer) {
			os_free(uvc_uac_device->mic_shuttle_buffer);
			uvc_uac_device->mic_shuttle_buffer = NULL;
		}
		return BK_FAIL;
	}


	/*write ring buffer*/
	ring_buffer_write(uvc_uac_device->audio_mic_rb, uvc_uac_device->mic_ring_buffer, uac_device->uac_mic_ep_desc->wMaxPacketSize);

	if(ring_buffer_get_free_size(uvc_uac_device->audio_mic_rb) < (uvc_uac_device->audio_mic_rb->capacity / 2))
	{
		ring_buffer_read(uvc_uac_device->audio_mic_rb, uvc_uac_device->mic_shuttle_buffer, (uvc_uac_device->audio_mic_rb->capacity / 2));
		packet.data_buffer = uvc_uac_device->mic_shuttle_buffer;
		packet.data_buffer_size = (uvc_uac_device->audio_mic_rb->capacity / 2);
		uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_push(&packet);
	}

	bk_uvc_uac_updata_audio_stream_rx_buffer(uvc_uac_device);

	bk_usb_drv_send_msg(USB_DRV_AUDIO_RX, (void *)uac_device);

    return BK_OK;
}

static bk_err_t bk_uvc_uac_updata_audio_stream_rx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	struct bk_usb_isoch_irp   *pIsochIrp = uac_device->uac_mic_isoch_irp;
	uint32_t wIndex;

	if(!(s_uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) return BK_FAIL;

	if(!uvc_uac_device->mic_ring_buffer) {
		uvc_uac_device->mic_ring_buffer = (uint8_t *)os_malloc(uac_device->uac_mic_ep_desc->wMaxPacketSize);

		if (!uvc_uac_device->mic_ring_buffer) {
			return BK_ERR_USB_OPERATION_NULL_POINTER;
		} else
			memset(uvc_uac_device->mic_ring_buffer, 0, uac_device->uac_mic_ep_desc->wMaxPacketSize);
	}

	pIsochIrp->pBuffer = uvc_uac_device->mic_ring_buffer;
	pIsochIrp->wFrameCount = 1;//packet->num_packets;
	pIsochIrp->adwActualLength = &uvc_uac_device->mic_length[0];//packet->actual_num_byte;
	pIsochIrp->adwStatus = &uvc_uac_device->mic_awstatus[0];//packet->state;
	pIsochIrp->adwLength = &uvc_uac_device->mic_actual_length[0];//packet->num_byte;

	for (wIndex = 0; wIndex < pIsochIrp->wFrameCount; wIndex++)
	{
		pIsochIrp->adwLength[wIndex] = uac_device->uac_mic_ep_desc->wMaxPacketSize;
		pIsochIrp->adwActualLength[wIndex] = 0;
		pIsochIrp->adwStatus[wIndex] = 0;
	}
	pIsochIrp->hPipe = (bk_usb_pipe_ptr)uac_device->uac_mic_as_pipe_ptr;
	pIsochIrp->wCurrentFrame    = 0;
	pIsochIrp->wCallbackInterval = 0; /* Callback only when data transfer is completed */
	pIsochIrp->pfIrpComplete    = (bk_usb_isoch_irp_complete)bk_usbh_audio_stream_rxed_callback;
	pIsochIrp->pCompleteParam   = (void *)uvc_uac_device;
	pIsochIrp->bIsrCallback		= 0;
	pIsochIrp->bAllowDma		= 1;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_uac_start_mic()
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		USB_DRIVER_LOGE("%s Mic disconnect\r\n",__func__);
		return BK_FAIL;
	}
	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE)) {
		USB_DRIVER_LOGE("%s is start\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UAC_MIC_DEVICE);

	audio_packet_t *packet;
	packet = uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_malloc(USB_UAC_MIC_DEVICE);
	uint32_t pool_buffer_size = packet->data_buffer_size;
	USB_DRIVER_LOGI("%s pool_buffer_size:%d\r\n",__func__,pool_buffer_size);

	uint8_t *mic_ring_buffer = (uint8_t *)os_malloc(pool_buffer_size);
	uvc_uac_device->mic_shuttle_buffer = (uint8_t *)os_malloc((pool_buffer_size / 2));

	ring_buffer_init(uvc_uac_device->audio_mic_rb, mic_ring_buffer, pool_buffer_size, DMA_ID_MAX, 0);
	bk_uvc_uac_updata_audio_stream_rx_buffer(uvc_uac_device);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_MIC_START, (void *)uac_device);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_RX, (void *)uac_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_stop_mic()
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		USB_DRIVER_LOGE("%s Mic disconnect\r\n",__func__);
		return BK_FAIL;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UAC_MIC_DEVICE);

	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	bk_usb_drv_send_msg(USB_DRV_AUDIO_MIC_STOP, (void *)uac_device);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return BK_OK;
}

static uint32_t bk_usbh_audio_stream_txed_callback(void *pCompleteParam, struct bk_usb_isoch_irp *pIrp)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = (bk_uvc_uac_fusion_device *)pCompleteParam;
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		ring_buffer_clear(uvc_uac_device->audio_spk_rb);
		if(uvc_uac_device->audio_spk_rb->address) {
			os_free(uvc_uac_device->audio_spk_rb->address);
			uvc_uac_device->audio_spk_rb->address = NULL;
			os_free(uvc_uac_device->spk_ring_buffer);
			uvc_uac_device->spk_ring_buffer = NULL;
		}
		if(uvc_uac_device->spk_shuttle_buffer) {
			os_free(uvc_uac_device->spk_shuttle_buffer);
			uvc_uac_device->spk_shuttle_buffer = NULL;
		}
		return BK_FAIL;
	}

	bk_uvc_uac_updata_audio_stream_tx_buffer(uvc_uac_device);

	if(!(ring_buffer_read(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_ring_buffer, uac_device->uac_spk_ep_desc->wMaxPacketSize) > 0))
		memset(uvc_uac_device->spk_ring_buffer, 0x0, uac_device->uac_spk_ep_desc->wMaxPacketSize);

	bk_usb_drv_send_msg(USB_DRV_AUDIO_TX, (void *)uac_device);

    return BK_OK;
}

static bk_err_t bk_uvc_uac_updata_audio_stream_tx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	struct bk_usb_isoch_irp *pIsochIrp = uac_device->uac_spk_isoch_irp;
	uint32_t wIndex;  

	audio_packet_t packet;
	if(ring_buffer_get_free_size(uvc_uac_device->audio_spk_rb) > (uvc_uac_device->audio_spk_rb->capacity / 2))
	{
		packet.data_buffer = uvc_uac_device->spk_shuttle_buffer;
		packet.data_buffer_size = (uvc_uac_device->audio_spk_rb->capacity / 2);
		uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_free(&packet);
		ring_buffer_write(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_shuttle_buffer, (uvc_uac_device->audio_spk_rb->capacity / 2));
	}

	if(!uvc_uac_device->spk_ring_buffer) {
		uvc_uac_device->spk_ring_buffer = (uint8_t *)os_malloc(uac_device->uac_spk_ep_desc->wMaxPacketSize);

		if (uvc_uac_device->spk_ring_buffer == NULL) {
			USB_DRIVER_LOGE("No memory to alloc urb\r\n");
			return BK_ERR_USB_OPERATION_NULL_POINTER;
		} else
			memset(uvc_uac_device->spk_ring_buffer, 0, uac_device->uac_spk_ep_desc->wMaxPacketSize);
	}

	pIsochIrp->pBuffer = uvc_uac_device->spk_ring_buffer ;
	pIsochIrp->wFrameCount = 1;//packet->num_packets;
	pIsochIrp->adwActualLength = &uvc_uac_device->spk_length[0];//packet->actual_num_byte;
	pIsochIrp->adwStatus = &uvc_uac_device->spk_awstatus[0];//packet->state;
	pIsochIrp->adwLength = &uvc_uac_device->spk_actual_length[0];//packet->num_byte;

	for (wIndex = 0; wIndex < pIsochIrp->wFrameCount; wIndex++)
	{
		pIsochIrp->adwLength[wIndex] = (uint16_t)uac_device->uac_spk_ep_desc->wMaxPacketSize;
		pIsochIrp->adwActualLength[wIndex] = 0;
		pIsochIrp->adwStatus[wIndex] = 0;
	}

	pIsochIrp->hPipe = (bk_usb_pipe_ptr)uac_device->uac_spk_as_pipe_ptr;
	pIsochIrp->wCurrentFrame    = 0;
	pIsochIrp->wCallbackInterval = 0; /* Callback only when data transfer is completed */
	pIsochIrp->pfIrpComplete    = (bk_usb_isoch_irp_complete)bk_usbh_audio_stream_txed_callback;
	pIsochIrp->pCompleteParam   = (void *)uvc_uac_device;
	pIsochIrp->bIsrCallback		= 0;
	pIsochIrp->bAllowDma		= 1;

	return BK_OK;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

bk_err_t bk_uac_start_speaker(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;

	if(!uvc_uac_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		return BK_FAIL;
	}
	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE)) {
		return BK_FAIL;
	}

	audio_packet_t *packet;
	packet = uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_malloc(USB_UAC_SPEAKER_DEVICE);
	uint32_t pool_buffer_size = packet->data_buffer_size;
	USB_DRIVER_LOGI("%s pool_buffer_size:%d\r\n",__func__,pool_buffer_size);

	uint8_t *spk_ring_buffer = (uint8_t *)os_malloc(pool_buffer_size);
	uvc_uac_device->spk_shuttle_buffer = (uint8_t *)os_malloc((pool_buffer_size / 2));

	ring_buffer_init(uvc_uac_device->audio_spk_rb, spk_ring_buffer, pool_buffer_size, DMA_ID_MAX, 0);
	bk_uvc_uac_updata_audio_stream_tx_buffer(uvc_uac_device);
	if(!(ring_buffer_read(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_ring_buffer, uac_device->uac_spk_ep_desc->wMaxPacketSize) > 0))
		memset(uvc_uac_device->spk_ring_buffer, 0, uac_device->uac_spk_ep_desc->wMaxPacketSize);

	uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UAC_SPEAKER_DEVICE);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_SPK_START, (void *)uac_device);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_TX, (void *)uac_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_stop_speaker(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s Spk disconnect\r\n",__func__);
		return BK_FAIL;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UAC_SPEAKER_DEVICE);

	bk_usbh_audio_device *uac_device = uvc_uac_device->uac_dev;
	bk_usb_drv_send_msg(USB_DRV_AUDIO_SPK_STOP, (void *)uac_device);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return BK_OK;
}

bk_err_t bk_usb_uvc_get_param(bk_uvc_device_brief_info_t *uvc_device_info)
{
	USB_DRIVER_LOGI("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(!uvc_device_info)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
	USB_DRIVER_LOGI("[=]%s\r\n",__func__);

	struct usb_device_descriptor_t dev_descriptor;
	bk_usbh_video_device *uvc_device = s_uvc_uac_device->uvc_dev;
	uint32_t ep_num = uvc_device->vs_interface_numendpoints;
	uint8_t h264_index = uvc_device->support_format_index.h264_format_index;
	uint8_t h265_index = uvc_device->support_format_index.h265_format_index;
	uint8_t mjpeg_index = uvc_device->support_format_index.mjpeg_format_index;
	uint8_t yuv_index = uvc_device->support_format_index.yuv_format_index;

	bk_usb_get_device_descriptor(&dev_descriptor);
	uvc_device_info->vendor_id = dev_descriptor.idVendor;
	uvc_device_info->product_id = dev_descriptor.idProduct;

	if(h264_index > 0) {
		uvc_device_info->all_frame.h264_frame = uvc_device->support_frame.h264_frame;
		uvc_device_info->all_frame.h264_frame_num = uvc_device->support_frame.h264_frame_num;
		uvc_device_info->format_index.h264_format_index = h264_index;
	}

	if(h265_index > 0) {
		uvc_device_info->all_frame.h265_frame = uvc_device->support_frame.h265_frame;
		uvc_device_info->all_frame.h265_frame_num = uvc_device->support_frame.h265_frame_num;
		uvc_device_info->format_index.h265_format_index = h265_index;
	}

	if(mjpeg_index > 0){
		uvc_device_info->all_frame.mjpeg_frame = uvc_device->support_frame.mjpeg_frame;
		uvc_device_info->all_frame.mjpeg_frame_num = uvc_device->support_frame.mjpeg_frame_num;
		uvc_device_info->format_index.mjpeg_format_index = mjpeg_index;
	}

	if(yuv_index > 0){
		uvc_device_info->all_frame.yuv_frame = uvc_device->support_frame.yuv_frame;
		uvc_device_info->all_frame.yuv_frame_num = uvc_device->support_frame.yuv_frame_num;
		uvc_device_info->format_index.yuv_format_index = yuv_index;
	}

	uvc_device_info->endpoints_num = ep_num;
	uvc_device_info->ep_desc = uvc_device->uvc_vs_all_ep_desc;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uvc_set_param(bk_uvc_config_t *uvc_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(!uvc_config)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	struct usb_device_descriptor_t dev_descriptor;
	bk_usbh_video_device *uvc_device = s_uvc_uac_device->uvc_dev;
	bk_uvc_frame *frame = {0};
	uint8_t frame_num = 0;

	uint8_t config_format_index = uvc_config->format_index;
	uint8_t h264_index = uvc_device->support_format_index.h264_format_index;
	uint8_t h265_index = uvc_device->support_format_index.h265_format_index;
	uint8_t mjpeg_index = uvc_device->support_format_index.mjpeg_format_index;

	if(config_format_index == mjpeg_index) {
		frame = uvc_device->support_frame.mjpeg_frame;
		frame_num = uvc_device->support_frame.mjpeg_frame_num;
	}else if(config_format_index == h264_index) {
		frame = uvc_device->support_frame.h264_frame;
		frame_num = uvc_device->support_frame.h264_frame_num;
	}else if(config_format_index == h265_index){
		frame = uvc_device->support_frame.h265_frame;
		frame_num = uvc_device->support_frame.h265_frame_num;
	}else{
		frame = uvc_device->support_frame.yuv_frame;
		frame_num = uvc_device->support_frame.yuv_frame_num;
	}

	for(int index = 0; index < frame_num; index++)
	{
		if(uvc_config->height == frame[index].height && uvc_config->width == frame[index].width) {
			USB_DRIVER_LOGD("%s Check h:%d w:%d\r\n", __func__, frame[index].height, frame[index].width);
			if(frame[index].index != uvc_config->frame_index)
				return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
		}
	}

	bk_usb_get_device_descriptor(&dev_descriptor);
	if(uvc_config->vendor_id != dev_descriptor.idVendor)
		USB_DRIVER_LOGW("%s warning VID\r\n",__func__);

	if(uvc_config->product_id != dev_descriptor.idProduct)
		USB_DRIVER_LOGW("%s warning PID\r\n",__func__);

	
	uvc_device->default_frame_index = uvc_config->frame_index;
	uvc_device->default_video_fps = uvc_config->fps;
	uvc_device->default_format_index = uvc_config->format_index;

	if(uvc_config->ep_desc) {
		if((uvc_config->ep_desc->bEndpointAddress != uvc_device->uvc_vs_ep_desc->bEndpointAddress)
			|| (uvc_config->ep_desc->bmAttributes != uvc_device->uvc_vs_ep_desc->bmAttributes)
			|| (uvc_config->ep_desc->wMaxPacketSize != uvc_device->uvc_vs_ep_desc->wMaxPacketSize)
			|| (uvc_config->ep_desc->bInterval != uvc_device->uvc_vs_ep_desc->bInterval)
			)
		{
			bk_usb_video_close_pipe(uvc_device);
			uvc_device->uvc_vs_ep_desc = uvc_config->ep_desc;
			bk_usb_drv_send_msg(USB_DRV_VIDEO_OPEN_PIPE, (void *)uvc_device);
		} else
			uvc_device->uvc_vs_ep_desc = uvc_config->ep_desc;
	}
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uvc_register_transfer_buffer_ops(void *ops)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	s_uvc_uac_device->usb_uvc_buffer_ops_funcs = (camera_packet_control_t *)ops;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_uac_get_param(bk_uac_device_brief_info_t *uac_device_info)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(!uac_device_info)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	struct usb_device_descriptor_t dev_descriptor;
	bk_usbh_audio_device *uac_device = s_uvc_uac_device->uac_dev;
	bk_usb_driver_comprehensive_ops *usb_driver = s_uvc_uac_device->usb_driver;

	bk_usb_get_device_descriptor(&dev_descriptor);
	uac_device_info->vendor_id = dev_descriptor.idVendor;
	uac_device_info->product_id = dev_descriptor.idProduct;

	if((usb_driver->usbh_class_connect_status) & (0x1 << USB_UAC_MIC_DEVICE)) {
		uac_device_info->mic_format_tag = uac_device->uac_mic_stream_general_intf_desc->wFormatTag;
		uac_device_info->mic_samples_frequence_num = uac_device->uac_mic_stream_format_type_desc->bSamFreqType;
		uac_device_info->mic_samples_frequence = uac_device->uac_mic_stream_format_type_desc->tSamFreq;
		uac_device_info->mic_ep_desc = uac_device->uac_mic_ep_desc;
	} else
		return BK_FAIL;
	
	if((usb_driver->usbh_class_connect_status) & (0x1 << USB_UAC_SPEAKER_DEVICE)) {

		uac_device_info->spk_format_tag = uac_device->uac_spk_stream_general_intf_desc->wFormatTag;
		uac_device_info->spk_samples_frequence_num = uac_device->uac_spk_stream_format_type_desc->bSamFreqType;
		uac_device_info->spk_samples_frequence = uac_device->uac_spk_stream_format_type_desc->tSamFreq;
		uac_device_info->spk_ep_desc = uac_device->uac_spk_ep_desc;
	} else
		return BK_FAIL;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uac_set_param(bk_uac_config_t *uac_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(!uac_config)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	bk_usbh_audio_device *uac_device = s_uvc_uac_device->uac_dev;
	struct usb_device_descriptor_t dev_descriptor;
	bk_usb_driver_comprehensive_ops *usb_driver = s_uvc_uac_device->usb_driver;

	if(!((usb_driver->usbh_class_connect_status) & (0x1 << USB_UAC_MIC_DEVICE))) {
		USB_DRIVER_LOGE("%s UNSUPPORT UAC MICPHONE\r\n",__func__);
		return BK_FAIL;
	}
	if(!((usb_driver->usbh_class_connect_status) & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s UNSUPPORT UAC MICPHONE\r\n",__func__);
		return BK_FAIL;
	}
	bk_usb_get_device_descriptor(&dev_descriptor);
	if(uac_config->vendor_id != dev_descriptor.idVendor)
		USB_DRIVER_LOGW("%s warning VID\r\n",__func__);

	if(uac_config->product_id != dev_descriptor.idProduct)
		USB_DRIVER_LOGW("%s warning PID\r\n",__func__);

	if((uac_config->mic_format_tag) != (uac_device->uac_mic_stream_general_intf_desc->wFormatTag))
		USB_DRIVER_LOGW("%s warning mic_format_tag\r\n",__func__);

	if((uac_config->spk_format_tag) != (uac_device->uac_spk_stream_general_intf_desc->wFormatTag))
		USB_DRIVER_LOGW("%s warning spk_format_tag\r\n",__func__);

	uac_device->uac_mic_set_samfreq = uac_config->mic_samples_frequence;
	uac_device->uac_mic_ep_desc = uac_config->mic_ep_desc;
	uac_device->uac_spk_set_samfreq = uac_config->spk_samples_frequence;
	uac_device->uac_spk_ep_desc = uac_config->spk_ep_desc;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uac_register_transfer_buffer_ops(void *ops)
{
	USB_DRIVER_LOGI("[+]%s\r\n",__func__);
	s_uvc_uac_device->usb_uac_buffer_ops_funcs = (struct audio_packet_control_t *)ops;
#if 0
	struct bk_uac_ring_buffer_t *uac_rb_t = (struct bk_uac_ring_buffer_t *)ops;
	uac_rb_t->mic_ring_buffer_p = (void *)s_uvc_uac_device->audio_mic_rb;
	uac_rb_t->spk_ring_buffer_p = (void *)s_uvc_uac_device->audio_spk_rb;
#endif
	USB_DRIVER_LOGI("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_uvc_check_support_attribute(uint32_t attribute)
{
    uint8_t controls_last;
    uint8_t controls_first;
    bk_usbh_video_device *uvc_device = s_uvc_uac_device->uvc_dev;
    controls_first = uvc_device->uvc_cs_process_uint_des->bmControls[0];
    controls_last = uvc_device->uvc_cs_process_uint_des->bmControls[1];
    switch(attribute)
    {
        case UVC_PU_CONTROL_UNDEFINED:
            return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
            break;
        case UVC_PU_BACKLIGHT_COMPENSATION_CONTROL:
            return !(controls_last & (0x01 << 0));
            break;
        case UVC_PU_BRIGHTNESS_CONTROL:
            return !(controls_first & (0x01 << 0));
            break;
        case UVC_PU_CONTRAST_CONTROL:
            return !(controls_first & (0x01 << 1));
            break;
        case UVC_PU_GAIN_CONTROL:
            return !(controls_last & (0x01 << 1));
            break;
        case UVC_PU_POWER_LINE_FREQUENCY_CONTROL:
            return !(controls_last & (0x01 << 2));
            break;
        case UVC_PU_HUE_CONTROL:
            return !(controls_first & (0x01 << 2));
            break;
        case UVC_PU_SATURATION_CONTROL:
            return !(controls_first & (0x01 << 3));
            break;
        case UVC_PU_SHARPNESS_CONTROL:
            return !(controls_first & (0x01 << 4));
            break;
        case UVC_PU_GAMMA_CONTROL:
            return !(controls_first & (0x01 << 5));
            break;
        case UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
            return !(controls_first & (0x01 << 6));
            break;
        case UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
            return !(controls_last & (0x01 << 4));
            break;
        case UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL:
            return !(controls_first & (0x01 << 7));
            break;
        case UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
            return !(controls_last & (0x01 << 5));
            break;
        case UVC_PU_DIGITAL_MULTIPLIER_CONTROL:
            return !(controls_last & (0x01 << 6));
            break;
        case UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
            return !(controls_last & (0x01 << 7));
            break;
        case UVC_PU_HUE_AUTO_CONTROL:
            return !(controls_last & (0x01 << 3));
            break;
        default:
            break;
    }

	return BK_OK;
}

bk_err_t bk_usb_uvc_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{
	bk_usb_uvc_check_support_attribute(attribute);

	return BK_OK;
}

bk_err_t bk_usb_uac_check_support_attribute(uint32_t attribute)
{
	bk_usbh_audio_device *uac_device = s_uvc_uac_device->uac_dev;

	uint8_t bmacontrols_count = (uac_device->uac_spk_featrue_unit_desc->bLength) - sizeof(struct audio_cs_if_ac_feature_unit_descriptor);
	uint8_t controls_last = 0;
	uint8_t controls_first = 0;
	uint32_t ret = BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
	for(int i = 0; i <= bmacontrols_count; i++)
	{
		if(uac_device->uac_spk_featrue_unit_desc->bControlSize == 2) {
			controls_last = uac_device->uac_spk_featrue_unit_desc->bmaControls[i];
			controls_first = uac_device->uac_spk_featrue_unit_desc->bmaControls[i+1];
		} else
			controls_last = uac_device->uac_spk_featrue_unit_desc->bmaControls[i];
	
		switch(attribute)
		{
			case AUDIO_FU_CONTROL_UNDEF:
				ret = BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
				break;
			case AUDIO_FU_CONTROL_MUTE:
				if(controls_last & AUDIO_CONTROL_MUTE)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_VOLUME:
				if(controls_last & AUDIO_CONTROL_VOLUME)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_BASS:
				if(controls_last & AUDIO_CONTROL_BASS)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_MID:
				if(controls_last & AUDIO_CONTROL_MID)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_TREBLE:
				if(controls_last & AUDIO_CONTROL_TREBLE)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_EQUALIZER:
				if(controls_last & AUDIO_CONTROL_GRAPHIC_EQUALIZER)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_AGC:
				if(controls_last & AUDIO_CONTROL_AUTOMATIC_GAIN)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_DELAY:
				if(controls_last & AUDIO_CONTROL_DEALY)
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_BASS_BOOST:
				if(controls_first & (AUDIO_CONTROL_BASS_BOOST >> 8))
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_LOUDNESS:
				if(controls_first & (AUDIO_CONTROL_LOUDNESS >> 8))
					ret = BK_OK;
				break;
			case AUDIO_FU_CONTROL_INP_GAIN:
				break;
			case AUDIO_FU_CONTROL_INP_GAIN_PAD:
				break;
			case AUDIO_FU_CONTROL_PHASE_INVERT:
				break;
			case AUDIO_FU_CONTROL_UNDERFLOW:
				break;
			case AUDIO_FU_CONTROL_OVERFLOW:
				break;
			case AUDIO_FU_CONTROL_LATENCY:
				break;
			default:
				break;
		}
	   if(ret == BK_OK) {
		  uac_device->uac_spk_attribute = ((attribute << 8) | i);
		  break;
	   } else {
		  if(uac_device->uac_spk_featrue_unit_desc->bControlSize == 2)
			  i++;
	   }
	}
	return ret;
}

bk_err_t bk_usb_uac_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{
    uint32_t pattribute = attribute;

    if(BK_OK != bk_usb_uac_check_support_attribute(attribute))
        return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

    uint8_t data[2] = {0};
    uint32_t set_param = *param;
    uint32_t get_param = 0x0;
    uint32_t ret = BK_OK;
	bk_usbh_audio_device *uac_device = s_uvc_uac_device->uac_dev;
	uint8_t bInterfaceNumber = uac_device->uac_control_intf_desc->bInterfaceNumber;
	uint8_t bUnitID = uac_device->uac_spk_featrue_unit_desc->bUnitID;
	uac_device->control_transfer_buffer_info->pOutBuffer = (uint8_t *)data;
	uac_device->control_transfer_buffer_info->pInBuffer = (uint8_t *)data;
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_usb_driver_task_lock_mutex();
	pattribute = uac_device->uac_spk_attribute;

	USB_DRIVER_LOGD("%s pattribute:0x%x bInterfaceNumber:%x bUnitID:%x\r\n",__func__,pattribute, bInterfaceNumber, bUnitID);

    switch(ops)
    {
        case USB_ATTRIBUTE_SET_CUR:
            data[0] = 0xFF & set_param;
            data[1] = 0xFF & (set_param >> 8);
            ret = bk_usbh_audio_set_cur(uac_device, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        case USB_ATTRIBUTE_GET_CUR:
            ret = bk_usbh_audio_get_attribute(uac_device, AUDIO_REQUEST_GET_CUR, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        case USB_ATTRIBUTE_GET_MIN:
            ret = bk_usbh_audio_get_attribute(uac_device, AUDIO_REQUEST_GET_MIN, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        case USB_ATTRIBUTE_GET_MAX:
            ret = bk_usbh_audio_get_attribute(uac_device, AUDIO_REQUEST_GET_MAX, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        case USB_ATTRIBUTE_GET_RES:
            ret = bk_usbh_audio_get_attribute(uac_device, AUDIO_REQUEST_GET_RES, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        case USB_ATTRIBUTE_GET_LEN:
            ret = bk_usbh_audio_get_attribute(uac_device, AUDIO_REQUEST_GET_MEM, bInterfaceNumber, bUnitID, pattribute, 2, 20);
            break;
        default:
            break;
    }
	bk_usb_driver_task_unlock_mutex();
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
    get_param |= data[1];
    get_param = (get_param << 8) | data[0];
	USB_DRIVER_LOGD("[-]%s get_param 0x%x\r\n",__func__,get_param);
    *param = get_param;
    return ret;

}

#endif



