#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <components/usb.h>
#include <modules/bk_musb.h>
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usbh_video.h"

static bk_usbh_video_device *s_bk_uvc_device = {0};
static beken_semaphore_t s_uvc_transfer_wait_complete_sem = NULL;

bk_err_t bk_usbh_video_sw_init(bk_usbh_video_device *uvc_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	if(!uvc_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	s_bk_uvc_device = uvc_device;

	bool malloc_success_flag = 0;

	do{
		uvc_device->control_setup = (s_usb_device_request *)os_malloc(sizeof(s_usb_device_request));
		if(!uvc_device->control_setup) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)uvc_device->control_setup, 0x0, sizeof(uvc_device->control_setup));

		uvc_device->control_transfer_buffer_info = (s_usb_transfer_buffer_info *)os_malloc(sizeof(s_usb_transfer_buffer_info));
		if(!uvc_device->control_transfer_buffer_info) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)uvc_device->control_transfer_buffer_info, 0x0, sizeof(uvc_device->control_transfer_buffer_info));

	}while(0);

	if(malloc_success_flag) {
		USB_DRIVER_LOGE("%s malloc fail.\r\n", __func__);
		if(uvc_device->control_setup) {
			os_free(uvc_device->control_setup);
			uvc_device->control_setup = NULL;
		}
		if(uvc_device->control_transfer_buffer_info) {
			os_free(uvc_device->control_transfer_buffer_info);
			uvc_device->control_transfer_buffer_info = NULL;
		}

		return BK_FAIL;
	}

	uvc_device->vc_interface_number = 0;
	uvc_device->vc_interface_alternate = 0;
	uvc_device->vc_interface_numendpoints = 0;
	uvc_device->vc_interface_endpoint_index = 0;
	uvc_device->vs_interface_number = 0;
	uvc_device->vs_interface_alternate = 0;
	uvc_device->vs_interface_numendpoints = 0;
	uvc_device->vs_interface_endpoint_index = 0;

	uvc_device->support_frame.mjpeg_frame_num = 0;
	uvc_device->support_frame.mjpeg_frame = NULL;
	uvc_device->support_frame.h264_frame_num = 0;
	uvc_device->support_frame.h264_frame = NULL;
	uvc_device->support_frame.h265_frame_num = 0;
	uvc_device->support_frame.h265_frame = NULL;
	uvc_device->support_frame.yuv_frame_num = 0;
	uvc_device->support_frame.yuv_frame = NULL;
	uvc_device->support_format_index.h264_format_index = 0;
	uvc_device->support_format_index.h265_format_index = 0;
	uvc_device->support_format_index.mjpeg_format_index = 0;
	uvc_device->support_format_index.yuv_format_index = 0;
	uvc_device->uvc_vs_ep_desc = NULL;
	uvc_device->uvc_vs_all_ep_desc = NULL;
	uvc_device->uvc_cs_ep_desc = NULL;
	uvc_device->uvc_cs_all_ep_desc = NULL;
	uvc_device->uvc_vs_pipe_ptr = NULL;
	uvc_device->uvc_cs_pipe_ptr = NULL;

	if(!s_uvc_transfer_wait_complete_sem)
		rtos_init_semaphore(&s_uvc_transfer_wait_complete_sem, 1);

	uvc_device->usb_driver->usbh_class_sw_init_status |= (0x1 << USB_UVC_DEVICE);
	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

	return BK_OK;

}

bk_err_t bk_usbh_video_sw_deinit(bk_usbh_video_device *uvc_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	if(!uvc_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	int index = 0;

	if(s_uvc_transfer_wait_complete_sem) {
		rtos_deinit_semaphore(&s_uvc_transfer_wait_complete_sem);
		s_uvc_transfer_wait_complete_sem = NULL;
	}

	{
	if(uvc_device->support_frame.mjpeg_frame_num > 0) {
		for(index = 0; index < uvc_device->support_frame.mjpeg_frame_num; index++)
		{
			if(uvc_device->support_frame.mjpeg_frame[index].fps) {
				os_free(uvc_device->support_frame.mjpeg_frame[index].fps);
				uvc_device->support_frame.mjpeg_frame[index].fps = NULL;
			}
		}
	}
	if(uvc_device->support_frame.mjpeg_frame) {
		os_free(uvc_device->support_frame.mjpeg_frame);
		uvc_device->support_frame.mjpeg_frame = NULL;
	}
	}

	{
	if(uvc_device->support_frame.h264_frame_num > 0) {
		for(index = 0; index < uvc_device->support_frame.h264_frame_num; index++)
		{
			if(uvc_device->support_frame.h264_frame[index].fps) {
				os_free(uvc_device->support_frame.h264_frame[index].fps);
				uvc_device->support_frame.h264_frame[index].fps = NULL;
			}
		}
	}
	if(uvc_device->support_frame.h264_frame != NULL) {
		os_free(uvc_device->support_frame.h264_frame);
		uvc_device->support_frame.h264_frame = NULL;
	}
	}

	{
	if(uvc_device->support_frame.h265_frame_num > 0) {
		for(index = 0; index < uvc_device->support_frame.h265_frame_num; index++)
		{
			if(uvc_device->support_frame.h265_frame[index].fps) {
				os_free(uvc_device->support_frame.h265_frame[index].fps);
				uvc_device->support_frame.h265_frame[index].fps = NULL;
			}
		}
	}
	if(uvc_device->support_frame.h265_frame != NULL) {
		os_free(uvc_device->support_frame.h265_frame);
		uvc_device->support_frame.h265_frame = NULL;
	}
	}

	{
	if(uvc_device->support_frame.yuv_frame_num > 0) {
		for(index = 0; index < uvc_device->support_frame.yuv_frame_num; index++)
		{
			if(uvc_device->support_frame.yuv_frame[index].fps) {
				os_free(uvc_device->support_frame.yuv_frame[index].fps);
				uvc_device->support_frame.yuv_frame[index].fps = NULL;
			}
		}
	}
	if(uvc_device->support_frame.yuv_frame != NULL) {
		os_free(uvc_device->support_frame.yuv_frame);
		uvc_device->support_frame.yuv_frame = NULL;
	}
	}

	if(uvc_device->control_setup) {
		os_free(uvc_device->control_setup);
		uvc_device->control_setup = NULL;
	}
	if(uvc_device->control_transfer_buffer_info) {
		os_free(uvc_device->control_transfer_buffer_info);
		uvc_device->control_transfer_buffer_info = NULL;
	}

	if(uvc_device->uvc_cs_all_ep_desc) {
		os_free(uvc_device->uvc_cs_all_ep_desc);
		uvc_device->uvc_cs_all_ep_desc = NULL;
	}
	if(uvc_device->uvc_vs_all_ep_desc) {
		os_free(uvc_device->uvc_vs_all_ep_desc);
		uvc_device->uvc_vs_all_ep_desc = NULL;
	}

	uvc_device->vc_interface_number = 0;
	uvc_device->vc_interface_alternate = 0;
	uvc_device->vc_interface_numendpoints = 0;
	uvc_device->vc_interface_endpoint_index = 0;
	uvc_device->vs_interface_number = 0;
	uvc_device->vs_interface_alternate = 0;
	uvc_device->vs_interface_numendpoints = 0;
	uvc_device->vs_interface_endpoint_index = 0;

    uvc_device->usb_driver->usbh_class_sw_init_status &= ~(1 << USB_UVC_DEVICE);

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

	return BK_OK;

}

void bk_usb_updata_video_interface(uint8_t interface_sub_class, uint8_t *buffer)
{
	struct usb_interface_descriptor_t *interface_desc = (struct usb_interface_descriptor_t *)buffer;
	uint8_t numeps = 0;
	if(!interface_desc)
		return;
	bk_usbh_video_device *uvc_device = s_bk_uvc_device;

	switch (interface_sub_class)
	{
		case BK_USB_SUBCLASS_CONTROL:
			uvc_device->vc_interface_number = interface_desc->bInterfaceNumber;
			uvc_device->vc_interface_alternate = interface_desc->bAlternateSetting;
			uvc_device->vc_interface_numendpoints = interface_desc->bNumEndpoints;
			numeps = interface_desc->bNumEndpoints;
			if(numeps > 0) {
				uvc_device->uvc_cs_all_ep_desc = (struct s_bk_usb_endpoint_descriptor *)os_malloc((sizeof(struct s_bk_usb_endpoint_descriptor) * numeps));
				if(!uvc_device->uvc_cs_all_ep_desc) {
					USB_DRIVER_LOGE("%s malloc cs_all_ep_desc fail.\r\n", __func__);
				} else
					os_memset((void *)uvc_device->uvc_cs_all_ep_desc, 0x0, (sizeof(struct s_bk_usb_endpoint_descriptor) * numeps));
			}
			break;
		case BK_USB_SUBCLASS_STREAMING:
			uvc_device->vs_interface_number = interface_desc->bInterfaceNumber;
			uvc_device->vs_interface_alternate = interface_desc->bAlternateSetting;
			uvc_device->vs_interface_numendpoints = interface_desc->bNumEndpoints;
			numeps = interface_desc->bNumEndpoints;
			if(numeps > 0) {
				if(!uvc_device->uvc_vs_all_ep_desc) {
					uvc_device->uvc_vs_all_ep_desc = (struct s_bk_usb_endpoint_descriptor *)os_malloc((sizeof(struct s_bk_usb_endpoint_descriptor) * numeps));
					if(!uvc_device->uvc_vs_all_ep_desc) {
						USB_DRIVER_LOGE("%s malloc vs_all_ep_desc fail.\r\n", __func__);
					} else
						os_memset((void *)uvc_device->uvc_vs_all_ep_desc, 0x0, (sizeof(struct s_bk_usb_endpoint_descriptor) * numeps));
				}
			}
			break;
		default:
			break;
	}
}

void bk_usb_updata_video_endpoint(uint8_t interface_sub_class, uint8_t *buffer)
{
	struct s_bk_usb_endpoint_descriptor *ep_desc = (struct s_bk_usb_endpoint_descriptor *)buffer;
	bk_usbh_video_device *uvc_device = s_bk_uvc_device;
	uint8_t index = 0;

	switch (interface_sub_class)
	{
		case BK_USB_SUBCLASS_CONTROL:
			if(ep_desc && (ep_desc->wMaxPacketSize <= 1024)) {
				index = uvc_device->vc_interface_endpoint_index;
				uvc_device->uvc_cs_ep_desc = ep_desc;
				if(uvc_device->vc_interface_endpoint_index < uvc_device->vc_interface_numendpoints) {
					os_memcpy((void *)&(uvc_device->uvc_cs_all_ep_desc[index]), (void *)(ep_desc), sizeof(struct s_bk_usb_endpoint_descriptor));
					uvc_device->vc_interface_endpoint_index++;
				}
			}
			break;
		case BK_USB_SUBCLASS_STREAMING:
			if(uvc_device->vs_interface_endpoint_index < uvc_device->vs_interface_numendpoints) {
				index = uvc_device->vs_interface_endpoint_index;
				os_memcpy((void *)&(uvc_device->uvc_vs_all_ep_desc[index]), (void *)(ep_desc), ep_desc->bLength);
			}
			if((uvc_device->uvc_vs_ep_desc == NULL) && (ep_desc->wMaxPacketSize <= 1024)) {
				uvc_device->uvc_vs_ep_desc = ep_desc;
			} else if((uvc_device->uvc_vs_ep_desc->wMaxPacketSize < ep_desc->wMaxPacketSize)
					  && (ep_desc->wMaxPacketSize <= 1024)) {
				uvc_device->uvc_vs_ep_desc = ep_desc;
			} else
				break;
			break;
		default:
			break;
	}
}

void bk_usb_parse_video_control_descriptors(uint8_t *buffer)
{
	s_bk_usb_subclass_descriptor_header *pdesc = (s_bk_usb_subclass_descriptor_header *)buffer;
	uint32_t index = 0;
	bk_usbh_video_device *uvc_device = s_bk_uvc_device;

	struct uvc_header_descriptor *header_desc;
	struct uvc_camera_terminal_descriptor *input_desc;
	struct uvc_output_terminal_descriptor *output_desc;
	struct uvc_selector_unit_descriptor *sel_desc;
	struct uvc_processing_unit_descriptor *proc_desc;
	struct uvc_extension_unit_descriptor *exten_desc;

	switch(pdesc->bDescriptorSubtype)
	{
		case UVC_VC_HEADER:
			uvc_device->usb_driver->usbh_class_connect_status |= (0x1 << USB_UVC_DEVICE);
			header_desc = (struct uvc_header_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Interface Header Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", header_desc->bLength, header_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n", header_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Video Control Header)\r\n", header_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bcdUVC                        : 0x%x (UVC Version)\r\n", header_desc->bcdUVC);
			USB_DRIVER_LOGD("wTotalLength                  : 0x%x (%d bytes)\r\n", header_desc->wTotalLength, header_desc->wTotalLength);
			USB_DRIVER_LOGD("dwClockFrequency              : 0x%x\r\n",header_desc->dwClockFrequency);
			USB_DRIVER_LOGD("bInCollection                 : 0x%x (%d VideoStreaming interface)\r\n", header_desc->bInCollection, header_desc->bInCollection);
			for(index = 0; index < header_desc->bInCollection; index++)
				USB_DRIVER_LOGD("baInterfaceNr[%d]               : 0x%x (Interface %d)\r\n", index+1, header_desc->baInterfaceNr[index], header_desc->baInterfaceNr[index]);
			uvc_device->version = header_desc->bcdUVC;

			break;

		case UVC_VC_INPUT_TERMINAL:
			input_desc = (struct uvc_camera_terminal_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Input Terminal Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", input_desc->bLength, input_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n",input_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Input Terminal)\r\n",input_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bTerminalID                   : 0x%x\r\n",input_desc->bTerminalID);
			USB_DRIVER_LOGD("wTerminalType                 : 0x%x\r\n",input_desc->wTerminalType);
			USB_DRIVER_LOGD("bAssocTerminal                : 0x%x\r\n",input_desc->bAssocTerminal);
			USB_DRIVER_LOGD("iTerminal                     : 0x%x\r\n",input_desc->iTerminal);
			USB_DRIVER_LOGD("wObjectiveFocalLengthMin      : 0x%x\r\n",input_desc->wObjectiveFocalLengthMin);
			USB_DRIVER_LOGD("wObjectiveFocalLengthMax      : 0x%x\r\n",input_desc->wObjectiveFocalLengthMax);
			USB_DRIVER_LOGD("wOcularFocalLength            : 0x%x\r\n",input_desc->wOcularFocalLength);
			USB_DRIVER_LOGD("bControlSize                  : 0x%x\r\n",input_desc->bControlSize);
			for(index = 0; index < input_desc->bControlSize; index++)
				USB_DRIVER_LOGD("bmControls[%d]                 : 0x%x\r\n", index+1, input_desc->bmControls[index]);
			break;

		case UVC_VC_OUTPUT_TERMINAL:
			output_desc = (struct uvc_output_terminal_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Output Terminal Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", output_desc->bLength, output_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n", output_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Output Terminal)\r\n", output_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bTerminalID                   : 0x%x\r\n", output_desc->bTerminalID);
			USB_DRIVER_LOGD("wTerminalType                 : 0x%x\r\n", output_desc->wTerminalType);
			USB_DRIVER_LOGD("bAssocTerminalv               : 0x%x\r\n", output_desc->bAssocTerminal);
			USB_DRIVER_LOGD("bSourceID                     : 0x%x\r\n", output_desc->bSourceID);
			USB_DRIVER_LOGD("iTerminal                     : 0x%x\r\n", output_desc->iTerminal);
			break;

		case UVC_VC_SELECTOR_UNIT:
			sel_desc = (struct uvc_selector_unit_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Selector Unit Descriptor -----------	\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", sel_desc->bLength, sel_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n", sel_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Selector Unit)\r\n", sel_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bUnitID                       : 0x%x\r\n", sel_desc->bUnitID);
			USB_DRIVER_LOGD("bNrInPins                     : 0x%x\r\n", sel_desc->bNrInPins);
			USB_DRIVER_LOGD("baSourceID[1]                 : 0x%x\r\n", sel_desc->baSourceID[0]);
			USB_DRIVER_LOGD("iSelector                     : 0x%x\r\n", sel_desc->iSelector);
			break;

		case UVC_VC_PROCESSING_UNIT:
			proc_desc = (struct uvc_processing_unit_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Processing Unit Descriptor -----------   \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", proc_desc->bLength, proc_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n", proc_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Processing Unit)\r\n", proc_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bUnitID                       : 0x%x\r\n",  proc_desc->bUnitID);
			USB_DRIVER_LOGD("bSourceID                     : 0x%x\r\n", proc_desc->bSourceID);
			USB_DRIVER_LOGD("wMaxMultiplier                : 0x%x\r\n", proc_desc->wMaxMultiplier);
			USB_DRIVER_LOGD("bControlSize                  : 0x%x\r\n", proc_desc->bControlSize);
			for(index = 0; index < proc_desc->bControlSize; index++)
				USB_DRIVER_LOGD("bmControls[%d]                 : 0x%x\r\n", index+1, proc_desc->bmControls[index]);
			USB_DRIVER_LOGD("iProcessing                   : 0x%x\r\n", proc_desc->iProcessing);
			uvc_device->uvc_cs_process_uint_des = proc_desc;
			break;

		case UVC_VC_EXTENSION_UNIT:
			exten_desc = (struct uvc_extension_unit_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Video Control Processing Unit Descriptor -----------   \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", exten_desc->bLength, exten_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Control Interface)\r\n", exten_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Extension Unit)\r\n", exten_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bUnitID                       : 0x%x\r\n",  exten_desc->bUnitID);
			USB_DRIVER_LOGD("guidExtensionCode             : 0x");
			for(index = 0; index < 16; index++)
				USB_DRIVER_LOGD("%x", exten_desc->guidExtensionCode[index]);
			USB_DRIVER_LOGD("\r\n");
			USB_DRIVER_LOGD("bNumControls                  : 0x%x (%d Controls)\r\n", exten_desc->bNumControls, exten_desc->bNumControls);
			USB_DRIVER_LOGD("bNrInPins                     : 0x%x\r\n", exten_desc->bNrInPins);
			USB_DRIVER_LOGD("bControlSize                  : 0x%x\r\n", exten_desc->bControlSize);
			USB_DRIVER_LOGD("baSourceID[1]                 : 0x%x\r\n", exten_desc->baSourceID[0]);
			for(index = 0; index <  exten_desc->bControlSize; index++)
				USB_DRIVER_LOGD("bmControls[%d]                 : 0x%x\r\n", index+1,  exten_desc->bmControls[index]);
			USB_DRIVER_LOGD("iExtension                    : 0x%x\r\n", exten_desc->iExtension);
			break;
		default:
			break;
	}
}

void bk_usb_parse_video_streaming_descriptors(uint8_t *buffer)
{
	s_bk_usb_subclass_descriptor_header *pdesc = (s_bk_usb_subclass_descriptor_header *)buffer;
	uint32_t index = 0;
	uint32_t frame_index = 0;
	bk_usbh_video_device *uvc_device = s_bk_uvc_device;

	struct uvc_input_header_descriptor *header_desc;
	struct uvc_format_mjpeg *format_mjpeg;
	struct uvc_frame_mjpeg *frame_mjpeg;
	struct uvc_color_matching_descriptor *color_desc;
	struct uvc_format_uncompressed *format_uncom;
	struct uvc_frame_uncompressed *frame_uncom;
	struct uvc_format_h26x *format_h26x;
	struct uvc_frame_h26x *frame_h26x;

	switch(pdesc->bDescriptorSubtype)
	{
		case UVC_VS_INPUT_HEADER:
			header_desc = (struct uvc_input_header_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ VC-Specific VS Video Input Header Descriptor -----------%d\r\n",frame_index);
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", header_desc->bLength, header_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", header_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Input Header)\r\n", header_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bNumFormats                   : 0x%x\r\n", header_desc->bNumFormats);
			USB_DRIVER_LOGD("wTotalLength                  : 0x%x\r\n", header_desc->wTotalLength);
			if(header_desc->bEndpointAddress & MUSB_DIR_IN) {
				USB_DRIVER_LOGD("bEndpointAddress              : 0x%x (Direction=IN  EndpointID=%d)\r\n", header_desc->bEndpointAddress, (header_desc->bEndpointAddress & 0x0F));
			} else {
				USB_DRIVER_LOGD("bEndpointAddress              : 0x%x (Direction=OUT  EndpointID=%d)\r\n", header_desc->bEndpointAddress, (header_desc->bEndpointAddress & 0x0F));
			}
			USB_DRIVER_LOGD("bmInfo                        : 0x%x\r\n", header_desc->bmInfo);
			USB_DRIVER_LOGD("bTerminalLink                 : 0x%x (Output Terminal ID %d)\r\n", header_desc->bTerminalLink, header_desc->bTerminalLink);
			USB_DRIVER_LOGD("bStillCaptureMethod           : 0x%x\r\n", header_desc->bStillCaptureMethod);
			USB_DRIVER_LOGD("bTriggerSupport               : 0x%x\r\n", header_desc->bTriggerSupport);
			USB_DRIVER_LOGD("bTriggerUsage                 : 0x%x\r\n", header_desc->bTriggerUsage);
			USB_DRIVER_LOGD("bControlSize                  : 0x%x (%d bytes each)\r\n", header_desc->bControlSize, header_desc->bControlSize);
			for(index = 0; index <  header_desc->bControlSize; index++)
				USB_DRIVER_LOGD("bmaControls[%d]             : 0x%x\r\n", index+1,  header_desc->bmaControls[index]);
			break;

		case UVC_VS_FORMAT_MJPEG:
			format_mjpeg = (struct uvc_format_mjpeg *)buffer;
			USB_DRIVER_LOGD("------------ Video Streaming MJPEG Format Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", format_mjpeg->bLength, format_mjpeg->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", format_mjpeg->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Format MJPEG)\r\n", format_mjpeg->bDescriptorSubType);
			USB_DRIVER_LOGD("bFormatIndex                  : 0x%x\r\n", format_mjpeg->bFormatIndex);
			USB_DRIVER_LOGD("bNumFrameDescriptors          : 0x%x\r\n", format_mjpeg->bNumFrameDescriptors);
			USB_DRIVER_LOGD("bmFlags                       : 0x%x\r\n", format_mjpeg->bmFlags);
			USB_DRIVER_LOGD("bDefaultFrameIndex            : 0x%x\r\n", format_mjpeg->bDefaultFrameIndex);
			USB_DRIVER_LOGD("bAspectRatioX                 : 0x%x\r\n", format_mjpeg->bAspectRatioX);
			USB_DRIVER_LOGD("bAspectRatioY                 : 0x%x\r\n", format_mjpeg->bAspectRatioY);
			USB_DRIVER_LOGD("bmInterfaceFlags              : 0x%x\r\n", format_mjpeg->bmInterfaceFlags);
			USB_DRIVER_LOGD("bCopyProtect                  : 0x%x\r\n", format_mjpeg->bCopyProtect);
			uvc_device->default_format_index = format_mjpeg->bFormatIndex;
			uvc_device->support_format_index.mjpeg_format_index = format_mjpeg->bFormatIndex;
			uvc_device->support_frame.mjpeg_frame_num = format_mjpeg->bNumFrameDescriptors;
			uvc_device->support_frame.mjpeg_parse_index = 0;
			uvc_device->support_frame.mjpeg_frame = (bk_uvc_frame *)os_malloc(sizeof(bk_uvc_frame) * format_mjpeg->bNumFrameDescriptors);
			if(!uvc_device->support_frame.mjpeg_frame) {
				break;
			} else
				os_memset((void *)uvc_device->support_frame.mjpeg_frame, 0x0, sizeof(uvc_device->support_frame.mjpeg_frame));

			break;

		case UVC_VS_FRAME_MJPEG:
			frame_mjpeg = (struct uvc_frame_mjpeg *)buffer;
			USB_DRIVER_LOGD("------------ Video Streaming MJPEG Frame Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", frame_mjpeg->bLength, frame_mjpeg->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", frame_mjpeg->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (MJPEG Frame Type)\r\n", frame_mjpeg->bDescriptorSubType);
			USB_DRIVER_LOGD("bFrameIndex                   : 0x%x\r\n", frame_mjpeg->bFrameIndex);
			USB_DRIVER_LOGD("bmCapabilities                : 0x%x\r\n", frame_mjpeg->bmCapabilities);
			USB_DRIVER_LOGD("wWidth                        : 0x%x (%d)\r\n", frame_mjpeg->wWidth, frame_mjpeg->wWidth);
			USB_DRIVER_LOGD("wHeight                       : 0x%x (%d)\r\n", frame_mjpeg->wHeight, frame_mjpeg->wHeight);
			USB_DRIVER_LOGD("dwMinBitRate                  : 0x%x\r\n", frame_mjpeg->dwMinBitRate);
			USB_DRIVER_LOGD("dwMaxBitRate                  : 0x%x\r\n", frame_mjpeg->dwMaxBitRate);
			USB_DRIVER_LOGD("dwMaxVideoFrameBufferSize     : 0x%x\r\n", frame_mjpeg->dwMaxVideoFrameBufferSize);
			USB_DRIVER_LOGD("dwDefaultFrameInterval        : 0x%x (%d fps)\r\n", frame_mjpeg->dwDefaultFrameInterval, 10000000/frame_mjpeg->dwDefaultFrameInterval);
			USB_DRIVER_LOGD("bFrameIntervalType            : 0x%x\r\n", frame_mjpeg->bFrameIntervalType);

			frame_index = uvc_device->support_frame.mjpeg_parse_index;
			uvc_device->support_frame.mjpeg_frame[frame_index].width = frame_mjpeg->wWidth;
			uvc_device->support_frame.mjpeg_frame[frame_index].height = frame_mjpeg->wHeight;
			uvc_device->support_frame.mjpeg_frame[frame_index].fps = (uint32_t *)os_malloc(sizeof(uint32_t) * frame_mjpeg->bFrameIntervalType);
			uvc_device->support_frame.mjpeg_frame[frame_index].fps_num = frame_mjpeg->bFrameIntervalType;
			for(index = 0; index < frame_mjpeg->bFrameIntervalType; index++)
			{
				uvc_device->support_frame.mjpeg_frame[frame_index].fps[index] = 10000000/frame_mjpeg->dwFrameInterval[index];
				USB_DRIVER_LOGD("dwFrameInterval[%d]           : 0x%x (%d fps)\r\n", index+1, frame_mjpeg->dwFrameInterval[index], 10000000/frame_mjpeg->dwFrameInterval[index]);
			}
			uvc_device->support_frame.mjpeg_frame[frame_index].index = frame_mjpeg->bFrameIndex;
			uvc_device->support_frame.mjpeg_parse_index++;
			break;

		case UVC_VS_COLORFORMAT:
			color_desc = (struct uvc_color_matching_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ VS Color Matching Descriptor Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", color_desc->bLength, color_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", color_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Color Matching)\r\n", color_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bColorPrimaries               : 0x%x\r\n", color_desc->bColorPrimaries);
			USB_DRIVER_LOGD("bTransferCharacteristics      : 0x%x\r\n", color_desc->bTransferCharacteristics);
			USB_DRIVER_LOGD("bMatrixCoefficients           : 0x%x\r\n", color_desc->bMatrixCoefficients);
			break;

		case UVC_VS_FORMAT_UNCOMPRESSED:
			format_uncom = (struct uvc_format_uncompressed *)buffer;
			USB_DRIVER_LOGD("------------ VS Uncompressed Format Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", format_uncom->bLength, format_uncom->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", format_uncom->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Uncompressed Format Type)\r\n", format_uncom->bDescriptorSubType);
			USB_DRIVER_LOGD("bFormatIndex                  : 0x%x\r\n", format_uncom->bFormatIndex);
			USB_DRIVER_LOGD("bNumFrameDescriptors          : 0x%x\r\n", format_uncom->bNumFrameDescriptors);
			USB_DRIVER_LOGD("guidFormat                    : 0x");
			for(index = 0; index < 16; index++)
				USB_DRIVER_LOGD("%x",  format_uncom->guidFormat[index]);
			USB_DRIVER_LOGD("\r\n");
			USB_DRIVER_LOGD("bBitsPerPixel                 : 0x%x (%d bits per pixel)\r\n", format_uncom->bBitsPerPixel, format_uncom->bBitsPerPixel);
			USB_DRIVER_LOGD("bDefaultFrameIndex            : 0x%x\r\n", format_uncom->bDefaultFrameIndex);
			USB_DRIVER_LOGD("bAspectRatioX                 : 0x%x\r\n", format_uncom->bAspectRatioX);
			USB_DRIVER_LOGD("bAspectRatioY                 : 0x%x\r\n", format_uncom->bAspectRatioY);
			USB_DRIVER_LOGD("bmInterfaceFlags              : 0x%x\r\n", format_uncom->bmInterfaceFlags);
			USB_DRIVER_LOGD("bCopyProtect                  : 0x%x\r\n", format_uncom->bCopyProtect);

			uvc_device->support_format_index.yuv_format_index = format_uncom->bFormatIndex;
			uvc_device->support_frame.yuv_frame_num = format_uncom->bNumFrameDescriptors;
			uvc_device->support_frame.yuv_parse_index = 0;
			uvc_device->support_frame.yuv_frame = (bk_uvc_frame *)os_malloc(sizeof(bk_uvc_frame) * format_uncom->bNumFrameDescriptors);
			if(!uvc_device->support_frame.yuv_frame) {
				break;
			} else
				os_memset((void *)uvc_device->support_frame.yuv_frame, 0x0, sizeof(uvc_device->support_frame.yuv_frame));
			break;
		
		case UVC_VS_FRAME_UNCOMPRESSED:
			frame_uncom = (struct uvc_frame_uncompressed *)buffer;
			USB_DRIVER_LOGD("------------ VS Uncompressed Frame Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", frame_uncom->bLength, frame_uncom->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Video Streaming Interface)\r\n", frame_uncom->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Uncompressed Frame Type)\r\n", frame_uncom->bDescriptorSubType);
			USB_DRIVER_LOGD("bFrameIndex                   : 0x%x\r\n", frame_uncom->bFrameIndex);
			USB_DRIVER_LOGD("bmCapabilities                : 0x%x\r\n", frame_uncom->bmCapabilities);
			USB_DRIVER_LOGD("wWidth                        : 0x%x (%d)\r\n", frame_uncom->wWidth, frame_uncom->wWidth);
			USB_DRIVER_LOGD("wHeight                       : 0x%x (%d)\r\n", frame_uncom->wHeight, frame_uncom->wHeight);
			USB_DRIVER_LOGD("dwMinBitRate                  : 0x%x\r\n", frame_uncom->dwMinBitRate);
			USB_DRIVER_LOGD("dwMaxBitRate                  : 0x%x\r\n", frame_uncom->dwMaxBitRate);
			USB_DRIVER_LOGD("dwMaxVideoFrameBufferSize     : 0x%x\r\n", frame_uncom->dwMaxVideoFrameBufferSize);
			USB_DRIVER_LOGD("dwDefaultFrameInterval        : 0x%x (%d fps)\r\n", frame_uncom->dwDefaultFrameInterval, 10000000/frame_uncom->dwDefaultFrameInterval);
			USB_DRIVER_LOGD("bFrameIntervalType            : 0x%x\r\n", frame_uncom->bFrameIntervalType);

			frame_index = uvc_device->support_frame.yuv_parse_index;
			uvc_device->support_frame.yuv_frame[frame_index].width = frame_uncom->wWidth;
			uvc_device->support_frame.yuv_frame[frame_index].height = frame_uncom->wHeight;
			uvc_device->support_frame.yuv_frame[frame_index].fps = (uint32_t *)os_malloc(sizeof(uint32_t) * frame_uncom->bFrameIntervalType);
			uvc_device->support_frame.yuv_frame[frame_index].fps_num = frame_uncom->bFrameIntervalType;
			for(index = 0; index < frame_uncom->bFrameIntervalType; index++)
			{
				uvc_device->support_frame.yuv_frame[frame_index].fps[index] = 10000000/frame_uncom->dwFrameInterval[index];
				USB_DRIVER_LOGD("dwFrameInterval[%d]           : 0x%x (%d fps)\r\n", index+1, frame_uncom->dwFrameInterval[index], 10000000/frame_uncom->dwDefaultFrameInterval);
			}
			uvc_device->support_frame.yuv_frame[frame_index].index = frame_uncom->bFrameIndex;
			uvc_device->support_frame.yuv_parse_index++;

			break;

		case UVC_VS_FORMAT_FRAME_BASED:
			format_h26x = (struct uvc_format_h26x *)buffer;
			USB_DRIVER_LOGD("------------ VS BASE Format Type Descriptor -----------\r\n");
			if( format_h26x->guidFormat[BK_USB_VIDEO_H26X_FORMAT_CHECK_NUM] == BK_USB_VIDEO_H264_FORMAT_GUID_INDEX) {
				USB_DRIVER_LOGD("------------ VIDEO H264 FORMAT DESCRIPTOR -----------\r\n");
				uvc_device->support_format_index.h264_format_index = format_h26x->bFormatIndex;
				uvc_device->support_frame.h264_frame_num = format_h26x->bNumFrameDescriptors;
				uvc_device->support_frame.h264_parse_index = 0;
				uvc_device->support_frame.h264_frame = (bk_uvc_frame *)os_malloc(sizeof(bk_uvc_frame) * format_h26x->bNumFrameDescriptors);
				if(!uvc_device->support_frame.h264_frame) {
					break;
				} else
					os_memset((void *)uvc_device->support_frame.h264_frame, 0x0, sizeof(uvc_device->support_frame.h264_frame));
			}
			if( format_h26x->guidFormat[BK_USB_VIDEO_H26X_FORMAT_CHECK_NUM] == BK_USB_VIDEO_H265_FORMAT_GUID_INDEX) {
				USB_DRIVER_LOGD("------------ VIDEO H265 FORMAT DESCRIPTOR -----------\r\n");
				uvc_device->support_format_index.h265_format_index = format_h26x->bFormatIndex;
				uvc_device->support_frame.h265_frame_num = format_h26x->bNumFrameDescriptors;
				uvc_device->support_frame.h265_parse_index = 0;
				uvc_device->support_frame.h265_frame = (bk_uvc_frame *)os_malloc(sizeof(bk_uvc_frame) * format_h26x->bNumFrameDescriptors);
				if(!uvc_device->support_frame.h265_frame) {
					break;
				} else
					os_memset((void *)uvc_device->support_frame.h265_frame, 0x0, sizeof(uvc_device->support_frame.h265_frame));
			}
			USB_DRIVER_LOGD("bLength                       : 0x%x\r\n", format_h26x->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x\r\n", format_h26x->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x\r\n", format_h26x->bDescriptorSubType);
			USB_DRIVER_LOGD("bFormatIndex                  : 0x%x\r\n", format_h26x->bFormatIndex);
			USB_DRIVER_LOGD("bNumFrameDescriptors          : 0x%x\r\n", format_h26x->bNumFrameDescriptors);
			USB_DRIVER_LOGD("guidFormat                    : 0x");
			for(index = 0; index < 16; index++)
				USB_DRIVER_LOGD("%x", format_h26x->guidFormat[index]);
			USB_DRIVER_LOGD("\r\n");
			USB_DRIVER_LOGD("bBitsPerPixel                 : 0x%x\r\n", format_h26x->bBitsPerPixel);
			USB_DRIVER_LOGD("bDefaultFrameIndex            : 0x%x\r\n", format_h26x->bDefaultFrameIndex);
			USB_DRIVER_LOGD("bAspectRatioX                 : 0x%x\r\n", format_h26x->bAspectRatioX);
			USB_DRIVER_LOGD("bAspectRatioY                 : 0x%x\r\n", format_h26x->bAspectRatioY);
			USB_DRIVER_LOGD("bmInterfaceFlags              : 0x%x\r\n", format_h26x->bmInterfaceFlags);
			USB_DRIVER_LOGD("bCopyProtect                  : 0x%x\r\n", format_h26x->bCopyProtect);
			USB_DRIVER_LOGD("bVariableSize                 : 0x%x\r\n", format_h26x->bVariableSize);
			break;

		case UVC_VS_FRAME_FRAME_BASED:
			frame_h26x = (struct uvc_frame_h26x *)buffer;
			USB_DRIVER_LOGD("------------ VS BASE Format Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", frame_h26x->bLength, frame_h26x->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x\r\n", frame_h26x->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x\r\n", frame_h26x->bDescriptorSubType);
			USB_DRIVER_LOGD("bFrameIndex                   : 0x%x\r\n", frame_h26x->bFrameIndex);
			USB_DRIVER_LOGD("bmCapabilities                : 0x%x\r\n", frame_h26x->bmCapabilities);
			USB_DRIVER_LOGD("wWidth                        : 0x%x (%d)\r\n", frame_h26x->wWidth,frame_h26x->wWidth);
			USB_DRIVER_LOGD("wHeight                       : 0x%x (%d)\r\n", frame_h26x->wHeight, frame_h26x->wHeight);
			USB_DRIVER_LOGD("dwMinBitRate                  : 0x%x\r\n", frame_h26x->dwMinBitRate);
			USB_DRIVER_LOGD("dwMaxBitRate                  : 0x%x\r\n", frame_h26x->dwMaxBitRate);
			USB_DRIVER_LOGD("dwDefaultFrameInterval        : 0x%x\r\n", frame_h26x->dwDefaultFrameInterval);
			USB_DRIVER_LOGD("bFrameIntervalType            : 0x%x\r\n", frame_h26x->bFrameIntervalType);
			USB_DRIVER_LOGD("dwBytesPerLine                : 0x%x\r\n", frame_h26x->dwBytesPerLine);

			if(uvc_device->support_format_index.h264_format_index > 0) {
				frame_index = uvc_device->support_frame.h264_parse_index;
				uvc_device->support_frame.h264_frame[frame_index].width = frame_h26x->wWidth;
				uvc_device->support_frame.h264_frame[frame_index].height = frame_h26x->wHeight;
				uvc_device->support_frame.h264_frame[frame_index].fps = (uint32_t *)os_malloc(sizeof(uint32_t) * frame_h26x->bFrameIntervalType);
				uvc_device->support_frame.h264_frame[frame_index].fps_num = frame_h26x->bFrameIntervalType;
				for(index = 0; index < frame_h26x->bFrameIntervalType; index++)
				{
					uvc_device->support_frame.h264_frame[frame_index].fps[index] = 10000000/frame_h26x->dwFrameInterval[index];
					USB_DRIVER_LOGD("dwFrameInterval[%d]           : 0x%x (%d fps)\r\n", index+1, frame_h26x->dwFrameInterval[index], 10000000/frame_h26x->dwFrameInterval[index]);
				}
				uvc_device->support_frame.h264_frame[frame_index].index = frame_h26x->bFrameIndex;
				uvc_device->support_frame.h264_parse_index++;
			} else {
				frame_index = uvc_device->support_frame.h265_parse_index;
				uvc_device->support_frame.h265_frame[frame_index].width = frame_h26x->wWidth;
				uvc_device->support_frame.h265_frame[frame_index].height = frame_h26x->wHeight;
				uvc_device->support_frame.h265_frame[frame_index].fps = (uint32_t *)os_malloc(sizeof(uint32_t) * frame_h26x->bFrameIntervalType);
				uvc_device->support_frame.h265_frame[frame_index].fps_num = frame_h26x->bFrameIntervalType;
				for(index = 0; index < frame_h26x->bFrameIntervalType; index++)
				{
					uvc_device->support_frame.h265_frame[frame_index].fps[index] = 10000000/frame_h26x->dwFrameInterval[index];
					USB_DRIVER_LOGD("dwFrameInterval[%d]		   : 0x%x (%d fps)\r\n", index+1, frame_h26x->dwFrameInterval[index], 10000000/frame_h26x->dwFrameInterval[index]);
				}
				uvc_device->support_frame.h265_frame[frame_index].index = frame_h26x->bFrameIndex;
				uvc_device->support_frame.h265_parse_index++;
			}
			break;
		default:
			break;
	}
}

static uint32_t bk_usbh_video_commit_control_callback(void *pContext, void *pControlIrp)
{
	if(!pContext)
		return BK_FAIL;
	s_usb_device_request *pSetup = (s_usb_device_request *)pContext;
	USB_DRIVER_LOGD("Control_Transfer Done. pSetup->bmRequestType:0x%x\r\n",pSetup->bmRequestType);
	USB_DRIVER_LOGD("Control_Transfer Done. pSetup->bRequest:0x%x\r\n",pSetup->bRequest);
	USB_DRIVER_LOGD("Control_Transfer Done. pSetup->wValue:0x%x\r\n",pSetup->wValue);
	USB_DRIVER_LOGD("Control_Transfer Done. pSetup->wIndex:0x%x\r\n",pSetup->wIndex);
	USB_DRIVER_LOGD("Control_Transfer Done. pSetup->wLength:0x%x\r\n",pSetup->wLength);
	USB_DRIVER_LOGD("%s Done.\r\n", __func__);
    if(s_uvc_transfer_wait_complete_sem != NULL) {
        rtos_set_semaphore(&s_uvc_transfer_wait_complete_sem);
    }
	return BK_OK;
}

static void bk_usbh_video_check_protocol_version(bk_usbh_video_device *uvc_device, uint32_t *protocol_buffer_len)
{
	if(uvc_device->version == 0x0100)
		*protocol_buffer_len = UVC_VERSION_10;
	else if(uvc_device->version == 0x0101)
		*protocol_buffer_len = UVC_VERSION_11;
	else
		*protocol_buffer_len = UVC_VERSION_15;
	return;
}

static uint32_t bk_usbh_video_fps_to_frameinterval(uint32_t fps)
{
    uint32_t ret = 0;
	USB_DRIVER_LOGD("%s set_fps:%d\r\n", __func__, fps);

    switch(fps)
    {
    case FPS_30:
        ret = (int)FRAMEINTERVAL30;
        break;
    case FPS_25:
        ret = (int)FRAMEINTERVAL25;
        break;
    case FPS_20:
        ret = (int)FRAMEINTERVAL20;
        break;
    case FPS_15:
        ret = (int)FRAMEINTERVAL15;
        break;
    case FPS_10:
        ret = (int)FRAMEINTERVAL10;
        break;
    case FPS_5:
        ret = (int)FRAMEINTERVAL05;

        break;
    default:
        ret = (uint32_t) - 1;
        break;
    }

    return ret;
}

static uint32_t bk_usbh_video_frameinterval_to_fps(uint32_t frameinterval)
{
    uint32_t ret = 0;
	USB_DRIVER_LOGD("%s frameinterval:%d\r\n", __func__, frameinterval);

    switch(frameinterval)
    {
    case FRAMEINTERVAL30:
        ret = (int)FPS_30;
        break;
    case FRAMEINTERVAL25:
        ret = (int)FPS_25;
        break;
    case FRAMEINTERVAL20:
        ret = (int)FPS_20;
        break;
    case FRAMEINTERVAL15:
        ret = (int)FPS_15;
        break;
    case FRAMEINTERVAL10:
        ret = (int)FPS_10;
        break;
    case FRAMEINTERVAL05:
        ret = (int)FPS_5;
        break;
    default:
        ret = (uint32_t) - 1;
        break;

    }

    return ret;
}

static void bk_usbh_video_updata_cur_probe_commmit(bk_usbh_video_device *uvc_device, struct uvc_streaming_control_15 *video_info)
{
	uint32_t fps = bk_usbh_video_frameinterval_to_fps(video_info->dwFrameInterval);

	video_info->bFormatIndex = uvc_device->default_format_index;
	video_info->bFrameIndex = uvc_device->default_frame_index;
	USB_DRIVER_LOGD("[+]%s bFormatIndex:%d bFrameIndex:%d\r\n", __func__, video_info->bFormatIndex, video_info->bFrameIndex);
	if(fps < uvc_device->default_video_fps)
		video_info->dwFrameInterval = bk_usbh_video_fps_to_frameinterval(uvc_device->default_video_fps);
}

uint32_t bk_usbh_video_get_cur(bk_usbh_video_device *uvc_device, uint8_t intf, uint8_t entity_id, uint8_t cs, uint16_t len, uint32_t delay_ms)
{
    uint32_t ret;

    uvc_device->control_setup->bmRequestType = MUSB_DIR_IN | MUSB_TYPE_CLASS | MUSB_RECIP_INTERFACE;
    uvc_device->control_setup->bRequest = UVC_GET_CUR;
    uvc_device->control_setup->wValue = cs << 8;
    uvc_device->control_setup->wIndex = (entity_id << 8) | intf;
    uvc_device->control_setup->wLength = len;


	uvc_device->control_transfer_buffer_info->dwOutLength = len;
	uvc_device->control_transfer_buffer_info->dwInLength = len;
	uvc_device->control_transfer_buffer_info->pCompleteParam = uvc_device->control_setup;

    ret = bk_usb_control_transfer(uvc_device->control_setup, uvc_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }

    if(s_uvc_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uvc_transfer_wait_complete_sem, delay_ms);
    }
    return ret;
}

uint32_t bk_usbh_video_set_cur(bk_usbh_video_device *uvc_device, uint8_t intf, uint8_t entity_id, uint8_t cs, uint16_t len, uint32_t delay_ms)
{
    uint32_t ret;

    uvc_device->control_setup->bmRequestType = MUSB_DIR_OUT | MUSB_TYPE_CLASS | MUSB_RECIP_INTERFACE;
    uvc_device->control_setup->bRequest = UVC_SET_CUR;
    uvc_device->control_setup->wValue = cs << 8;
    uvc_device->control_setup->wIndex = (entity_id << 8) | intf;
    uvc_device->control_setup->wLength = len;

	uvc_device->control_transfer_buffer_info->dwOutLength = len;
	uvc_device->control_transfer_buffer_info->dwInLength = len;
	uvc_device->control_transfer_buffer_info->pCompleteParam = uvc_device->control_setup;

    ret = bk_usb_control_transfer(uvc_device->control_setup, uvc_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }
    if(s_uvc_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uvc_transfer_wait_complete_sem, delay_ms);
    }

    return ret;
}

uint32_t bk_usbh_video_interface_ops(bk_usbh_video_device *uvc_device, uint8_t intf, uint8_t entity_id, uint8_t cs, uint16_t len, uint32_t delay_ms)
{
    uint32_t ret;

    uvc_device->control_setup->bmRequestType = MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_INTERFACE;
    uvc_device->control_setup->bRequest = MUSB_REQ_SET_INTERFACE;
    uvc_device->control_setup->wValue = cs;
    uvc_device->control_setup->wIndex = (entity_id << 8) | intf;
    uvc_device->control_setup->wLength = len;

	uvc_device->control_transfer_buffer_info->pOutBuffer = NULL;
	uvc_device->control_transfer_buffer_info->pInBuffer = NULL;
	uvc_device->control_transfer_buffer_info->dwOutLength = len;
	uvc_device->control_transfer_buffer_info->dwInLength = len;
	uvc_device->control_transfer_buffer_info->pCompleteParam = uvc_device->control_setup;

    ret = bk_usb_control_transfer(uvc_device->control_setup, uvc_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }
    if(s_uvc_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uvc_transfer_wait_complete_sem, delay_ms);
    }

    return ret;
}

static void bk_usbh_video_get_frame(bk_uvc_frame *frame, uint8_t frame_num, uvc_resolution_framerate *param, uint16_t count)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	bk_usbh_video_device *uvc_device = s_bk_uvc_device;
	uint8_t default_index = uvc_device->default_format_index;
	uint8_t h264_index = uvc_device->support_format_index.h264_format_index;
	uint8_t h265_index = uvc_device->support_format_index.h265_format_index;
	uint8_t mjpeg_index = uvc_device->support_format_index.mjpeg_format_index;

	USB_DRIVER_LOGD("%s default_index:0x%x\r\n", __func__, default_index);

	if(default_index == mjpeg_index) {
		frame = uvc_device->support_frame.mjpeg_frame;
		frame_num = uvc_device->support_frame.mjpeg_frame_num;
	}else if(default_index == h264_index) {
		frame = uvc_device->support_frame.h264_frame;
		frame_num = uvc_device->support_frame.h264_frame_num;
	}else if(default_index == h265_index){
		frame = uvc_device->support_frame.h265_frame;
		frame_num = uvc_device->support_frame.h265_frame_num;
	}else{
		frame = uvc_device->support_frame.yuv_frame;
		frame_num = uvc_device->support_frame.yuv_frame_num;
	}

	if(!(count > 0))
		return;
	if(frame_num > count)
		frame_num = count;

    for (int index= 0; index < frame_num; index++)
    {
		param[index].height = frame[index].height;
		param[index].width = frame[index].width;
		param[index].fps = FRAME_RATE_MULTIPLIER_100ns/frame[index].fps[0];
		USB_DRIVER_LOGD("bk_uvc_set_resolution_framerate set_h:%d frame_h:%d set_w:%d frame_w:%d\r\n",param[index].height,frame[index].height, param[index].width,frame[index].width);
    }
	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

}

void bk_uvc_set_resolution_framerate(void *param)
{
	bk_usbh_video_device *uvc_device = s_bk_uvc_device;
	uvc_resolution_framerate *set_frame = (uvc_resolution_framerate *)param;
	bk_uvc_frame *frame = {0};
	uint8_t frame_num = 0;

	uint8_t default_index = uvc_device->default_format_index;
	uint8_t h264_index = uvc_device->support_format_index.h264_format_index;
	uint8_t h265_index = uvc_device->support_format_index.h265_format_index;
	uint8_t mjpeg_index = uvc_device->support_format_index.mjpeg_format_index;

	if(default_index == mjpeg_index) {
		frame = uvc_device->support_frame.mjpeg_frame;
		frame_num = uvc_device->support_frame.mjpeg_frame_num;
	}else if(default_index == h264_index) {
		frame = uvc_device->support_frame.h264_frame;
		frame_num = uvc_device->support_frame.h264_frame_num;
	}else if(default_index == h265_index){
		frame = uvc_device->support_frame.h265_frame;
		frame_num = uvc_device->support_frame.h265_frame_num;
	}else{
		frame = uvc_device->support_frame.yuv_frame;
		frame_num = uvc_device->support_frame.yuv_frame_num;
	}

	USB_DRIVER_LOGD("%s set_h:%d set_w:%d set_fps:%d\r\n", __func__, set_frame->height, set_frame->width, set_frame->fps);

	for(int index = 0; index < frame_num; index++)
	{
		USB_DRIVER_LOGD("%s Check h:%d w:%d\r\n", __func__, frame[index].height, frame[index].width);

		if(set_frame->height == frame[index].height && set_frame->width == frame[index].width) {
			uvc_device->default_frame_index = frame[index].index;
			uvc_device->default_video_fps = set_frame->fps;
			USB_DRIVER_LOGD("%s Ready set_h:%d set_w:%d set_fps:%d\r\n", __func__, set_frame->height,set_frame->width, set_frame->fps);
			return;
		}
	}

}

void bk_uvc_get_resolution_framerate(void *param, uint16_t count)
{
	bk_uvc_frame *frame = {0};
	uint8_t frame_num = 0;

	bk_usbh_video_get_frame(frame, frame_num, (uvc_resolution_framerate *)param, count);
}

void bk_usbh_video_start_handle(bk_usbh_video_device *uvc_device)
{
	if(!uvc_device)
		return;

	void *parameter = {0};
	uint32_t version_buffer_len = 0;
	struct uvc_streaming_control_15 video_info;

	bk_usbh_video_check_protocol_version(uvc_device, &version_buffer_len);

	uvc_device->control_transfer_buffer_info->pOutBuffer = (uint8_t *)&video_info;
	uvc_device->control_transfer_buffer_info->pInBuffer = (uint8_t *)&video_info;
	parameter = (void *)bk_usbh_video_commit_control_callback;
	uvc_device->control_transfer_buffer_info->pfIrpComplete = parameter;

	bk_usbh_video_get_cur(uvc_device, 0x01, 0x00, UVC_VS_PROBE_CONTROL, version_buffer_len, 20);
	bk_usbh_video_updata_cur_probe_commmit(uvc_device, &video_info);
	bk_usbh_video_set_cur(uvc_device, 0x01, 0x00, UVC_VS_PROBE_CONTROL, version_buffer_len, 20);
	bk_usbh_video_get_cur(uvc_device, 0x01, 0x00, UVC_VS_PROBE_CONTROL, version_buffer_len, 20);
	bk_usbh_video_set_cur(uvc_device, 0x01, 0x00, UVC_VS_COMMIT_CONTROL, version_buffer_len, 20);
	bk_usbh_video_get_cur(uvc_device, 0x01, 0x00, UVC_VS_PROBE_CONTROL, version_buffer_len, 20);

	if((uvc_device->uvc_vs_ep_desc->bmAttributes & BK_USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_ISOCH_TRANSFER){
		bk_usbh_video_interface_ops(uvc_device, uvc_device->vs_interface_number, 0x00, uvc_device->vs_interface_alternate, 0, 50);
	}

}

void bk_usbh_video_stop_handle(bk_usbh_video_device *uvc_device)
{
	if(!uvc_device)
		return;
	uvc_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UVC_DEVICE);

	bk_usbh_video_interface_ops(uvc_device, uvc_device->vs_interface_number, 0x00, 0x00, 0, 50);
}

void bk_usb_video_open_pipe(bk_usbh_video_device *uvc_device)
{
	if(!uvc_device)
		return;

	bk_usb_driver_comprehensive_ops *usb_driver = uvc_device->usb_driver;

	void *pipe_ptr = NULL;
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	pipe_ptr = bk_usb_init_pipe(usb_driver->usb_bus, usb_driver->usb_device, (uint8_t *)uvc_device->uvc_vs_ep_desc);
	uvc_device->uvc_vs_pipe_ptr = (bk_usb_pipe_ptr)pipe_ptr;
	pipe_ptr = (bk_usb_pipe_ptr)bk_usb_init_pipe(usb_driver->usb_bus, usb_driver->usb_device, (uint8_t *)uvc_device->uvc_cs_ep_desc);
	uvc_device->uvc_cs_pipe_ptr = (bk_usb_pipe_ptr)pipe_ptr;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

void bk_usb_video_close_pipe(bk_usbh_video_device *uvc_device)
{
	if(!uvc_device)
		return;

	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_usb_deinit_pipe(uvc_device->uvc_vs_pipe_ptr);
	bk_usb_deinit_pipe(uvc_device->uvc_cs_pipe_ptr);
	uvc_device->uvc_vs_ep_desc = NULL;
	uvc_device->uvc_cs_ep_desc = NULL;
	uvc_device->uvc_vs_pipe_ptr = NULL;
	uvc_device->uvc_cs_pipe_ptr = NULL;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

}

bk_err_t bk_uvc_trigger_video_stream_rx(void *uvc_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uvc_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	bk_usbh_video_device *device = uvc_device;
	if(!device->uvc_vs_pipe_ptr) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
    bk_usb_bulk_irp *vsIrp = &device->uvc_irp;
	struct bk_usb_isoch_irp *pIsochIrp = device->uvc_isoch_irp;

	if((device->uvc_vs_ep_desc->bmAttributes & BK_USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_ISOCH_TRANSFER){
		bk_usb_transfer_request(USB_ENDPOINT_ISOCH_TRANSFER, pIsochIrp);
	} else if((device->uvc_vs_ep_desc->bmAttributes & BK_USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_BULK_TRANSFER){
		bk_usb_transfer_request(USB_ENDPOINT_BULK_TRANSFER, vsIrp);
	} else
		return BK_FAIL;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

