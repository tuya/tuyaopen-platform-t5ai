#include <common/bk_include.h>
#include <components/log.h>
#include <components/usb_types.h>
#include <modules/bk_musb.h>
#include "mu_cdi.h"
#include "mu_dev.h"
#include "bk_gpio.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#ifndef _USB_DRIVER_H_
#define _USB_DRIVER_H_


#define USB_DRIVER_TAG "usb_driver"
#define USB_DRIVER_LOGI(...) BK_LOGI(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGW(...) BK_LOGW(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGE(...) BK_LOGE(USB_DRIVER_TAG, ##__VA_ARGS__)
#define USB_DRIVER_LOGD(...) BK_LOGD(USB_DRIVER_TAG, ##__VA_ARGS__)

#define USB_DRIVER_QITEM_COUNT 60
#define CONFIG_USB_DRIVER_TASK_SIZE 2048

#define USB_DP_CAPABILITY_VALUE           (0xF)
#define USB_DN_CAPABILITY_VALUE           (0x0)

#define BK_USB_SWAP16(_data) (((_data) << 8) | ((_data) >> 8))
#define BK_USB_SWAP32(_data) ( ((_data) << 24) | ((_data) << 16) | ((_data) >> 24) | ((_data) >> 16) )


#define BK_USB_SUBCLASS_UNDEFINED                    0x00
#define BK_USB_SUBCLASS_CONTROL                      0x01
#define BK_USB_SUBCLASS_STREAMING                    0x02

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
} s_bk_usb_subclass_descriptor_header;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
} s_bk_usb_descriptor_header;

/*
 * The value is defined in field wWidth and wHeight in 'Video Streaming MJPEG
Frame Type Descriptor'
 */
typedef struct
{
    uint16_t  fps;
    uint16_t  width;
    uint16_t  height;
} uvc_resolution_framerate;

typedef struct
{
	void *private_data;
	uint16_t bcd_cdi_rev;
	uint8_t type;
	uint8_t speed;
} bk_usb_port;

typedef struct _bk_usb_device
{
	bk_usb_port *port;
	void *private_data;
	void *drive_private_data;
	struct usb_device_descriptor_t device_descriptor;
	struct usb_configuration_descriptor_t *current_configuration;
	struct usb_configuration_descriptor_t **apconfig_descriptors;
	uint8_t *descriptor_buffer;
	uint16_t descriptor_buffer_length;
	uint8_t connection_speed;
	uint8_t bus_address;
	struct _bk_usb_device *parent_usb_device;
	uint8_t hu_port;
} bk_usb_device;

/** A port's bus */
typedef void *bk_usb_bus_handle;
typedef uint8_t (*bk_usb_device_connected)(void *, bk_usb_bus_handle, bk_usb_device *, const uint8_t *);
typedef void (*bk_usb_device_disconnected)(void *, bk_usb_bus_handle, bk_usb_device *);
typedef void (*bk_usb_device_process)(void *);

struct bk_usb_class_driver {
	const uint8_t  *peripheral_list;
	uint16_t  peripheral_list_length;
	const char *driver_name;
	uint16_t  driver_index;
	bk_usb_device_connected device_connected;
	bk_usb_device_disconnected device_disconnected;
	bk_usb_device_process device_process;
};

/**
 * Pipe handle
 */
typedef void *bk_usb_pipe_ptr;

typedef uint32_t (*bk_usb_control_irp_complete)(void *, void *);
typedef struct _bk_usb_control_irp
{
	const bk_usb_device *pDevice;

	uint8_t *pOutBuffer;
	uint32_t dwOutLength;

	uint8_t *pInBuffer;
	uint32_t dwInLength;

	uint32_t dwStatus;
	uint32_t dwActualOutLength;
	uint32_t dwActualInLength;
	bk_usb_control_irp_complete pfIrpComplete;
	void *pCompleteParam;
	uint8_t bAllowDma;
	uint8_t resv;
} bk_usb_control_irp;

typedef uint32_t (*bk_usb_isoch_irp_complete)(void *, void *);
struct bk_usb_isoch_irp
{
	bk_usb_pipe_ptr hPipe;
	uint8_t *pBuffer;
	uint16_t *adwLength;
	uint8_t *adwStatus;
	uint16_t *adwActualLength;
	bk_usb_isoch_irp_complete pfIrpComplete;
	void *pCompleteParam;
	uint16_t wFrameCount;
	uint16_t wCurrentFrame;
	uint16_t wCallbackInterval;
	uint8_t bIsrCallback;
	uint8_t bAllowDma;
};

typedef uint32_t (*bk_usb_irp_complete)(void *, void *);
typedef struct _bk_usb_irp
{
	bk_usb_pipe_ptr hPipe;
	uint8_t *pBuffer;
	uint32_t dwLength;
	uint32_t dwStatus;
	uint32_t dwActualLength;
	bk_usb_irp_complete pfIrpComplete;
	void *pCompleteParam;
	uint8_t bAllowShortTransfer;
	uint8_t bIsrCallback;
	uint8_t bAllowDma;
} bk_usb_irp, bk_usb_bulk_irp, bk_usb_interrupt_irp;

typedef struct {
	bk_usb_drv_op_t op;
	void *param;
} bk_usb_drv_msg_t;

typedef struct {
	s_usb_device_request *setup;
	s_usb_transfer_buffer_info *buf_info;
	uint32_t delay_ms;
} bk_usb_control_trans_assembly;

typedef struct {
	uint8_t usbh_class_sw_init_status;
	uint8_t usbh_class_using_status;
	uint8_t usbh_class_connect_status;
	uint8_t usbh_class_start_status;

	FUNCPTR usbh_connect_cb[USB_DEVICE_MAX];
	FUNCPTR usbh_disconnect_cb[USB_DEVICE_MAX];

	bk_usb_control_irp *control_transfer_irp;

	bk_usb_bus_handle usb_bus;
	bk_usb_device *usb_device;

} bk_usb_driver_comprehensive_ops;

bk_err_t bk_usb_driver_sw_init(bk_usb_driver_comprehensive_ops *usb_driver);

bk_err_t bk_usb_register_connection_status_notification_callback(bk_usb_driver_comprehensive_ops *usb_driver, E_USB_CONNECTION_STATUS status, E_USB_DEVICE_T dev, void *callback);

bk_err_t bk_usb_control_transfer_init(void);
bk_err_t bk_usb_control_transfer_deinit(void);
bk_err_t bk_usb_drv_send_msg(int op, void *param);
bk_err_t bk_usb_drv_send_msg_front(int op, void *param);

void bk_usb_driver_task_lock_mutex();
void bk_usb_driver_task_unlock_mutex();
bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops);
bk_err_t bk_usb_adaptor_init(void);

#endif
