#ifndef _USBH_VIDEO_H_
#define _USBH_VIDEO_H_

#include <components/usb.h>
#include <components/usb_types.h>
#include <modules/bk_musb.h>
#include "usb_driver.h"

/* 
 * H.264 guidFormat: 34363248-0000-0010-8000-00AA00389B71 
 * H.265 guidFormat: 35363248-0000-0010-8000-00AA00389B71 
 */
#define BK_USB_VIDEO_H26X_FORMAT_CHECK_NUM       3
#define BK_USB_VIDEO_H264_FORMAT_GUID_INDEX      0x34
#define BK_USB_VIDEO_H265_FORMAT_GUID_INDEX      0x35

/*
 * The value is defined in field dwFrameInterval of  'Video Streaming MJPEG
Frame Type Descriptor' */
#define FRAME_RATE_MULTIPLIER_100ns  10000000
#define FRAMEINTERVAL30               0x00051615
#define FRAMEINTERVAL25               0x00061A80
#define FRAMEINTERVAL20               0x0007A120
#define FRAMEINTERVAL15               0x000A2C2A
#define FRAMEINTERVAL10               0x000F4240
#define FRAMEINTERVAL05               0x001E8480


typedef struct
{
    bk_uvc_frame *mjpeg_frame;
    uint8_t mjpeg_frame_num;
	uint8_t mjpeg_parse_index;
    bk_uvc_frame *h264_frame;
	uint8_t h264_frame_num;
	uint8_t h264_parse_index;
    bk_uvc_frame *h265_frame;
	uint8_t h265_frame_num;
	uint8_t h265_parse_index;
    bk_uvc_frame *yuv_frame;
	uint8_t yuv_frame_num;
	uint8_t yuv_parse_index;
} bk_usbh_video_frame_resolution;

typedef uint32_t (*bk_usbh_video_data_trans_complete)(void *, void *);

typedef struct
{
    uint8_t vc_interface_number;
    uint8_t vc_interface_alternate;
    uint8_t vc_interface_numendpoints;
	uint8_t vc_interface_endpoint_index;
    uint8_t vs_interface_number;
    uint8_t vs_interface_alternate;
    uint8_t vs_interface_numendpoints;
	uint8_t vs_interface_endpoint_index;
	uint16_t version;

	uint8_t default_format_index;
	uint8_t default_frame_index;
	uint32_t default_video_fps;
	bk_uvc_format_index support_format_index;
	bk_usbh_video_frame_resolution support_frame;

	struct uvc_processing_unit_descriptor *uvc_cs_process_uint_des;
	struct s_bk_usb_endpoint_descriptor *uvc_cs_ep_desc;
	struct s_bk_usb_endpoint_descriptor *uvc_cs_all_ep_desc;
	struct s_bk_usb_endpoint_descriptor *uvc_vs_ep_desc;
	struct s_bk_usb_endpoint_descriptor *uvc_vs_all_ep_desc;
	uint8_t *vs_data_buffer;

	bk_usb_pipe_ptr uvc_cs_pipe_ptr;
	bk_usb_pipe_ptr uvc_vs_pipe_ptr;

	bk_usb_driver_comprehensive_ops *usb_driver;

	s_usb_device_request *control_setup;
	s_usb_transfer_buffer_info *control_transfer_buffer_info;
	struct bk_usb_isoch_irp *uvc_isoch_irp;
	bk_usb_bulk_irp uvc_irp;
} bk_usbh_video_device;

bk_err_t bk_usbh_video_sw_init(bk_usbh_video_device *uvc_device);
bk_err_t bk_usbh_video_sw_deinit(bk_usbh_video_device *uvc_device);

void bk_usb_updata_video_interface(uint8_t interface_sub_class, uint8_t *buffer);
void bk_usb_updata_video_endpoint(uint8_t interface_sub_class, uint8_t *buffer);
void bk_usb_video_open_pipe(bk_usbh_video_device *uvc_device);
void bk_usb_video_close_pipe(bk_usbh_video_device *uvc_device);

void bk_usb_parse_video_control_descriptors(uint8_t *buffer);
void bk_usb_parse_video_streaming_descriptors(uint8_t *buffer);

void bk_uvc_set_resolution_framerate(void *param);
void bk_uvc_get_resolution_framerate(void *param, uint16_t count);
void bk_usbh_video_start_handle(bk_usbh_video_device *uvc_device);
void bk_usbh_video_stop_handle(bk_usbh_video_device *uvc_device);

bk_err_t bk_uvc_trigger_video_stream_rx(void *uvc_device);

#endif
