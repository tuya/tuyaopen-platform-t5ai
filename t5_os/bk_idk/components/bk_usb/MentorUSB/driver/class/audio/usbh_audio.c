#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usbh_audio.h"
#include <components/usb.h>
#include <modules/bk_musb.h>


static bk_usbh_audio_device *s_bk_uac_device = {0};
static beken_semaphore_t s_uac_transfer_wait_complete_sem = NULL;

bk_err_t bk_usbh_audio_sw_init(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	bool malloc_success_flag = 0;
	s_bk_uac_device = uac_device;

	do{
		uac_device->control_setup = (s_usb_device_request *)os_malloc(sizeof(s_usb_device_request));
		if(!uac_device->control_setup) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)uac_device->control_setup, 0x0, sizeof(uac_device->control_setup));

		uac_device->control_transfer_buffer_info = (s_usb_transfer_buffer_info *)os_malloc(sizeof(s_usb_transfer_buffer_info));
		if(!uac_device->control_transfer_buffer_info) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)uac_device->control_transfer_buffer_info, 0x0, sizeof(uac_device->control_transfer_buffer_info));

	}while(0);

	if(malloc_success_flag) {
		USB_DRIVER_LOGE("%s malloc fail.\r\n", __func__);
		if(uac_device->control_setup) {
			os_free(uac_device->control_setup);
			uac_device->control_setup = NULL;
		}
		if(uac_device->control_transfer_buffer_info) {
			os_free(uac_device->control_transfer_buffer_info);
			uac_device->control_transfer_buffer_info = NULL;
		}

		if(uac_device) {
			os_free(uac_device);
			uac_device = NULL;
		}
		return BK_FAIL;
	}

	uac_device->uac_mic_as_pipe_ptr = NULL;
	uac_device->uac_spk_as_pipe_ptr = NULL;

	if(!s_uac_transfer_wait_complete_sem)
		rtos_init_semaphore(&s_uac_transfer_wait_complete_sem, 1);

	uac_device->usb_driver->usbh_class_sw_init_status |= (0x1 << USB_UAC_MIC_DEVICE);
	uac_device->usb_driver->usbh_class_sw_init_status |= (0x1 << USB_UAC_SPEAKER_DEVICE);

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

	return BK_OK;

}

bk_err_t bk_usbh_audio_sw_deinit(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	if(s_uac_transfer_wait_complete_sem) {
		rtos_deinit_semaphore(&s_uac_transfer_wait_complete_sem);
		s_uac_transfer_wait_complete_sem = NULL;
	}

	if(uac_device->control_setup) {
		os_free(uac_device->control_setup);
		uac_device->control_setup = NULL;
	}
	if(uac_device->control_transfer_buffer_info) {
		os_free(uac_device->control_transfer_buffer_info);
		uac_device->control_transfer_buffer_info = NULL;
	}
	uac_device->usb_driver->usbh_class_sw_init_status &= ~(0x1 << USB_UAC_MIC_DEVICE);
	uac_device->usb_driver->usbh_class_sw_init_status &= ~(0x1 << USB_UAC_SPEAKER_DEVICE);

	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	return BK_OK;

}

void bk_usb_parse_audio_control_descriptors(uint8_t *buffer)
{
	s_bk_usb_subclass_descriptor_header *pdesc = (s_bk_usb_subclass_descriptor_header *)buffer;
	bk_usbh_audio_device *dev = s_bk_uac_device;
	uint32_t index = 0;
	struct audio_cs_if_ac_header_descriptor *header_desc;
	struct audio_cs_if_ac_input_terminal_descriptor *input_desc;
	struct audio_cs_if_ac_output_terminal_descriptor *output_desc;
	struct audio_cs_if_ac_selector_unit_descriptor *sel_desc;
	struct audio_cs_if_ac_feature_unit_descriptor *feature_desc;

	switch(pdesc->bDescriptorSubtype)
	{
		case AUDIO_CONTROL_HEADER:
			header_desc = (struct audio_cs_if_ac_header_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Control Interface Header Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", header_desc->bLength, header_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", header_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubtype            : 0x%x (Header)\r\n", header_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bcdADC                        : 0x%x\r\n", header_desc->bcdADC);
			USB_DRIVER_LOGD("wTotalLength                  : 0x%x\r\n", header_desc->wTotalLength);
			USB_DRIVER_LOGD("bInCollection                 : 0x%x\r\n", header_desc->bInCollection);
			for(index = 0; index < header_desc->bInCollection; index++)
				USB_DRIVER_LOGD("baInterfaceNr[%d]              : 0x%x\r\n", index+1, header_desc->baInterfaceNr[index]);
			dev->version = header_desc->bcdADC;
			break;

		case AUDIO_CONTROL_INPUT_TERMINAL:
			input_desc = (struct audio_cs_if_ac_input_terminal_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Control Input Terminal Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", input_desc->bLength, input_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", input_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubtype            : 0x%x (Input Terminal)\r\n", input_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bTerminalID                   : 0x%x\r\n", input_desc->bTerminalID);
			USB_DRIVER_LOGD("wTerminalType                 : 0x%x\r\n", input_desc->wTerminalType);
			USB_DRIVER_LOGD("bAssocTerminal                : 0x%x\r\n", input_desc->bAssocTerminal);
			USB_DRIVER_LOGD("bNrChannels                   : 0x%x\r\n", input_desc->bNrChannels);
			USB_DRIVER_LOGD("wChannelConfig                : 0x%x\r\n", input_desc->wChannelConfig);
			USB_DRIVER_LOGD("iChannelNames                 : 0x%x\r\n", input_desc->iChannelNames);
			USB_DRIVER_LOGD("iTerminal                     : 0x%x\r\n", input_desc->iTerminal);
			if(input_desc->wTerminalType == AUDIO_TERMINAL_STREAMING)
				dev->uac_spk_input_terminal_desc = input_desc;
			else if(input_desc->wTerminalType == AUDIO_INTERM_MIC)
				dev->uac_mic_input_terminal_desc = input_desc;
			else
				USB_DRIVER_LOGW("Unknown input terminal descriptor\r\n");
			break;

		case AUDIO_CONTROL_OUTPUT_TERMINAL:
			output_desc = (struct audio_cs_if_ac_output_terminal_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Control Output Terminal Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", output_desc->bLength, output_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", output_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubtype            : 0x%x (Output Terminal)\r\n", output_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bTerminalID                   : 0x%x\r\n", output_desc->bTerminalID);
			USB_DRIVER_LOGD("wTerminalType                 : 0x%x\r\n", output_desc->wTerminalType);
			USB_DRIVER_LOGD("bAssocTerminalv               : 0x%x\r\n", output_desc->bAssocTerminal);
			USB_DRIVER_LOGD("bSourceID                     : 0x%x\r\n", output_desc->bSourceID);
			USB_DRIVER_LOGD("iTerminal                     : 0x%x\r\n", output_desc->iTerminal);
			if(output_desc->wTerminalType == AUDIO_TERMINAL_STREAMING)
				dev->uac_mic_output_terminal_desc = output_desc;
			else if(output_desc->wTerminalType == AUDIO_OUTTERM_SPEAKER)
				dev->uac_spk_output_terminal_desc = output_desc;
			else
				USB_DRIVER_LOGW("Unknown output terminal descriptor\r\n");
			break;

		case AUDIO_CONTROL_SELECTOR_UNIT:
			sel_desc = (struct audio_cs_if_ac_selector_unit_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Control Selector Unit Descriptor -----------	\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", sel_desc->bLength, sel_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", sel_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Selector Unit)\r\n", sel_desc->bDescriptorSubType);
			USB_DRIVER_LOGD("bUnitID                       : 0x%x\r\n", sel_desc->bUnitID);
			USB_DRIVER_LOGD("bNrInPins                     : 0x%x (%d Input Pin)\r\n", sel_desc->bNrInPins, sel_desc->bNrInPins);
			USB_DRIVER_LOGD("baSourceID[1]                 : 0x%x (%d)\r\n", sel_desc->baSourceID[0], sel_desc->baSourceID[0]);
			USB_DRIVER_LOGD("iSelector                     : 0x%x\r\n", sel_desc->iSelector);
			break;

		case AUDIO_CONTROL_FEATURE_UNIT:
			feature_desc = (struct audio_cs_if_ac_feature_unit_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Control Feature Unit Descriptor -----------   \r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", feature_desc->bLength, feature_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", feature_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubtype            : 0x%x (Feature Unit)\r\n", feature_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bSourceID                     : 0x%x (%d)\r\n", feature_desc->bSourceID, feature_desc->bSourceID);
			USB_DRIVER_LOGD("bControlSize                  : 0x%x (%d)\r\n", feature_desc->bControlSize, feature_desc->bControlSize);
			uint32_t more_bmacontrols_count =  feature_desc->bLength - sizeof(struct audio_cs_if_ac_feature_unit_descriptor);
			for(index = 0; index <= more_bmacontrols_count; index++)
			{
				if(feature_desc->bControlSize > 1) {
					USB_DRIVER_LOGD("baSourceID[%d]                   : 0x%x ", index, feature_desc->bmaControls[index]);
					os_printf(" 0x%x\r\n",  feature_desc->bmaControls[index+1] );
					index++;
				} else
					USB_DRIVER_LOGD("bmaControls[%d]              : 0x%x\r\n", index, feature_desc->bmaControls[index]);
			}
			USB_DRIVER_LOGD("iFeature                       : 0x%x\r\n", feature_desc->iFeature);
			if(dev->uac_spk_input_terminal_desc->bTerminalID == feature_desc->bSourceID)
				dev->uac_spk_featrue_unit_desc = feature_desc;
			else if(dev->uac_mic_input_terminal_desc->bTerminalID == feature_desc->bSourceID)
				dev->uac_mic_featrue_unit_desc = feature_desc;
			else
				USB_DRIVER_LOGW("Unknown feature descriptor\r\n");
			break;

		default:
			break;
	}
}

void bk_usb_parse_audio_streaming_descriptors(uint8_t *buffer)
{
	s_bk_usb_subclass_descriptor_header *pdesc = (s_bk_usb_subclass_descriptor_header *)buffer;
	bk_usbh_audio_device *dev = s_bk_uac_device;
	uint32_t tSamFreq_hz = 0;
	struct audio_cs_if_as_general_descriptor *gen_desc;
	struct audio_cs_if_as_format_type_descriptor *format_desc;

	switch(pdesc->bDescriptorSubtype)
	{
		case AUDIO_STREAMING_GENERAL:
			gen_desc = (struct audio_cs_if_as_general_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Streaming Interface Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", gen_desc->bLength, gen_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", gen_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (AS_GENERAL)\r\n", gen_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bTerminalLink                 : 0x%x (Terminal ID %d)\r\n", gen_desc->bTerminalLink, gen_desc->bTerminalLink);
			USB_DRIVER_LOGD("bDelay                        : 0x%x (%d frame)\r\n", gen_desc->bDelay, gen_desc->bDelay);
			USB_DRIVER_LOGD("wFormatTag                    : 0x%x\r\n", gen_desc->wFormatTag);
			if(dev->uac_spk_input_terminal_desc->bTerminalID == gen_desc->bTerminalLink) {
				dev->current_device = USB_UAC_SPEAKER_DEVICE;
				dev->uac_spk_stream_intf_desc = dev->uac_interface_desc_staging_port;
				dev->uac_spk_stream_general_intf_desc = gen_desc;
			} else if(dev->uac_mic_output_terminal_desc->bTerminalID == gen_desc->bTerminalLink) {
				dev->current_device = USB_UAC_MIC_DEVICE;
				dev->uac_mic_stream_intf_desc = dev->uac_interface_desc_staging_port;
				dev->uac_mic_stream_general_intf_desc = gen_desc;
			} else
				USB_DRIVER_LOGW("Unknown streaming general descriptor\r\n");
			break;

		case AUDIO_STREAMING_FORMAT_TYPE:
			format_desc = (struct audio_cs_if_as_format_type_descriptor *)buffer;
			USB_DRIVER_LOGD("------------ Audio Streaming Format Type Descriptor -----------\r\n");
			USB_DRIVER_LOGD("bLength                       : 0x%x (%d bytes)\r\n", format_desc->bLength, format_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType               : 0x%x (Audio Interface Descriptor)\r\n", format_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubType            : 0x%x (Format Type)\r\n", format_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bFormatType                   : 0x%x\r\n", format_desc->bFormatType);
			USB_DRIVER_LOGD("bNrChannels                   : 0x%x (%d channel)\r\n", format_desc->bNrChannels, format_desc->bNrChannels);
			USB_DRIVER_LOGD("bSubframeSize                 : 0x%x (%d bytes per subframe)\r\n", format_desc->bSubframeSize, format_desc->bSubframeSize);
			USB_DRIVER_LOGD("bBitResolution                : 0x%x (%d bits per sample)\r\n", format_desc->bBitResolution, format_desc->bBitResolution);
			USB_DRIVER_LOGD("bSamFreqType                  : 0x%x (supports %d sample frequencies)\r\n", format_desc->bSamFreqType, format_desc->bSamFreqType);
			if( format_desc->bSamFreqType > 0) {
				for(int i = 0, j = 1; i < (format_desc->bSamFreqType) * 3; i += 3, j++)
				{
					tSamFreq_hz |= (uint32_t)( format_desc->tSamFreq[i]);
					tSamFreq_hz |= (uint32_t)(( format_desc->tSamFreq[i+1]) << 8);
					tSamFreq_hz |= (uint32_t)(( format_desc->tSamFreq[i+2]) << 16);
					USB_DRIVER_LOGD("tSamFreq[%d]                   : 0x%x (%d)\r\n", j, tSamFreq_hz, tSamFreq_hz);
					tSamFreq_hz = 0x0;
				}
			}

			switch(dev->current_device)
			{
				case USB_UAC_MIC_DEVICE:
					dev->usb_driver->usbh_class_connect_status |= (0x1 << USB_UAC_MIC_DEVICE);
					dev->uac_mic_stream_format_type_desc = format_desc;
					break;
				case USB_UAC_SPEAKER_DEVICE:
					dev->usb_driver->usbh_class_connect_status |= (0x1 << USB_UAC_SPEAKER_DEVICE);
					dev->uac_spk_stream_format_type_desc = format_desc;
					break;
				default:
					break;
			}
			break;
		default:
		  break;
	}
}

void bk_usb_updata_audio_interface(uint8_t interface_sub_class, uint8_t *buffer)
{
	struct usb_interface_descriptor_t *interface_desc = (struct usb_interface_descriptor_t *)buffer;
	bk_usbh_audio_device *dev = s_bk_uac_device;

	if(!interface_desc)
		return;
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);

	switch (interface_sub_class)
	{
		case BK_USB_SUBCLASS_CONTROL:
			dev->uac_control_intf_desc = (struct usb_interface_descriptor_t *)buffer;
			break;
		case BK_USB_SUBCLASS_STREAMING:
			dev->uac_interface_desc_staging_port = (struct usb_interface_descriptor_t *)buffer;
			break;
		default:
			break;
	}
	USB_DRIVER_LOGD("[-]%s\r\n", __func__);
}

void bk_usb_updata_audio_endpoint(uint8_t interface_sub_class, uint8_t *buffer)
{
	struct audio_ep_descriptor *ep_desc = (struct audio_ep_descriptor *)buffer;
	bk_usbh_audio_device *dev = s_bk_uac_device;

	switch (interface_sub_class)
	{
		case BK_USB_SUBCLASS_CONTROL:
			break;
		case BK_USB_SUBCLASS_STREAMING:
			switch(dev->current_device)
			{
				case USB_UAC_MIC_DEVICE:
					if(ep_desc->bEndpointAddress & MUSB_DIR_IN)
						dev->uac_mic_ep_desc = ep_desc;
					else
						USB_DRIVER_LOGE("%s Check that the parsing is correct! (Mic)\r\n", __func__);
					break;
				case USB_UAC_SPEAKER_DEVICE:
					if(ep_desc->bEndpointAddress & MUSB_DIR_IN)
						USB_DRIVER_LOGE("%s Check that the parsing is correct! (Speaker)\r\n", __func__);
					else
							dev->uac_spk_ep_desc = ep_desc;
					break;
				default:
					USB_DRIVER_LOGE("%s Device error\r\n", __func__);
					break;
			}
			break;
		default:
			break;
	}
}

void bk_usb_updata_audio_data_endpoint_descriptor(uint8_t *buffer)
{
	bk_usbh_audio_device *dev = s_bk_uac_device;

	switch(dev->current_device)
	{
		case USB_UAC_MIC_DEVICE:
			dev->uac_mic_data_ep_desc = (struct audio_cs_ep_ep_general_descriptor *)buffer;
			break;
		case USB_UAC_SPEAKER_DEVICE:
			dev->uac_spk_data_ep_desc = (struct audio_cs_ep_ep_general_descriptor *)buffer;
			break;
		default:
			USB_DRIVER_LOGE("%s Device error\r\n", __func__);
			break;
	}

}

void bk_usb_audio_open_pipe(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!uac_device)
		return;

	bk_usb_driver_comprehensive_ops *usb_driver = uac_device->usb_driver;

	void *pipe_ptr = NULL;

	pipe_ptr = (bk_usb_pipe_ptr)bk_usb_init_pipe(usb_driver->usb_bus, usb_driver->usb_device, (uint8_t *)uac_device->uac_spk_ep_desc);
	uac_device->uac_spk_as_pipe_ptr = (bk_usb_pipe_ptr)pipe_ptr;

	pipe_ptr = (bk_usb_pipe_ptr)bk_usb_init_pipe(usb_driver->usb_bus, usb_driver->usb_device, (uint8_t *)uac_device->uac_mic_ep_desc);
	uac_device->uac_mic_as_pipe_ptr = (bk_usb_pipe_ptr)pipe_ptr;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

}

void bk_usb_audio_close_pipe(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!uac_device)
		return;

	bk_usb_deinit_pipe(uac_device->uac_mic_as_pipe_ptr);
	bk_usb_deinit_pipe(uac_device->uac_spk_as_pipe_ptr);
	uac_device->uac_mic_as_pipe_ptr = NULL;
	uac_device->uac_spk_as_pipe_ptr = NULL;
	uac_device->uac_spk_ep_desc = NULL;
	uac_device->uac_mic_ep_desc = NULL;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

}

static uint32_t bk_usbh_audio_commit_control_callback(void *pContext, void *pControlIrp)
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
    if(s_uac_transfer_wait_complete_sem != NULL) {
        rtos_set_semaphore(&s_uac_transfer_wait_complete_sem);
    }
	return BK_OK;
}

uint32_t bk_usbh_audio_endpoint_selectors(bk_usbh_audio_device *uac_device, uint8_t endpoint, uint8_t entity_id, uint8_t cs, uint16_t len, uint32_t delay_ms)
{
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

    uint32_t ret;

    uac_device->control_setup->bmRequestType = MUSB_DIR_OUT | MUSB_TYPE_CLASS | MUSB_RECIP_ENDPOINT;
    uac_device->control_setup->bRequest = AUDIO_REQUEST_SET_CUR;
    uac_device->control_setup->wValue = cs << 8;
    uac_device->control_setup->wIndex = endpoint;
    uac_device->control_setup->wLength = len;

	uac_device->control_transfer_buffer_info->dwOutLength = len;
	uac_device->control_transfer_buffer_info->dwInLength = len;
	uac_device->control_transfer_buffer_info->pCompleteParam = uac_device->control_setup;

    ret = bk_usb_control_transfer(uac_device->control_setup, uac_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }
    if(s_uac_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uac_transfer_wait_complete_sem, delay_ms);
    }

    return ret;
}

uint32_t bk_usbh_audio_get_attribute(bk_usbh_audio_device *uac_device, uint8_t request, uint8_t intf, uint8_t entity_id, uint16_t cs, uint16_t len, uint32_t delay_ms)
{
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

    uint32_t ret;
	void *parameter = {0};

    uac_device->control_setup->bmRequestType = MUSB_DIR_IN | MUSB_TYPE_CLASS | MUSB_RECIP_INTERFACE;
    uac_device->control_setup->bRequest = request;
    uac_device->control_setup->wValue = cs;
    uac_device->control_setup->wIndex = (entity_id << 8) | intf;
    uac_device->control_setup->wLength = len;

	parameter = (void *)bk_usbh_audio_commit_control_callback;
	uac_device->control_transfer_buffer_info->pfIrpComplete = parameter;

	uac_device->control_transfer_buffer_info->dwOutLength = len;
	uac_device->control_transfer_buffer_info->dwInLength = len;
	uac_device->control_transfer_buffer_info->pCompleteParam = uac_device->control_setup;

    ret = bk_usb_control_transfer(uac_device->control_setup, uac_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }

    if(s_uac_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uac_transfer_wait_complete_sem, delay_ms);
    }
    return ret;
}

uint32_t bk_usbh_audio_set_cur(bk_usbh_audio_device *uac_device, uint8_t intf, uint8_t entity_id, uint16_t cs, uint16_t len, uint32_t delay_ms)
{
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

    uint32_t ret;
	void *parameter = {0};

    uac_device->control_setup->bmRequestType = MUSB_DIR_OUT | MUSB_TYPE_CLASS | MUSB_RECIP_INTERFACE;
    uac_device->control_setup->bRequest = AUDIO_REQUEST_SET_CUR;
    uac_device->control_setup->wValue = cs;
    uac_device->control_setup->wIndex = (entity_id << 8) | intf;
    uac_device->control_setup->wLength = len;

	parameter = (void *)bk_usbh_audio_commit_control_callback;
	uac_device->control_transfer_buffer_info->pfIrpComplete = parameter;
	uac_device->control_transfer_buffer_info->dwOutLength = len;
	uac_device->control_transfer_buffer_info->dwInLength = len;
	uac_device->control_transfer_buffer_info->pCompleteParam = uac_device->control_setup;

    ret = bk_usb_control_transfer(uac_device->control_setup, uac_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }
    if(s_uac_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uac_transfer_wait_complete_sem, delay_ms);
    }

    return ret;
}

uint32_t bk_usbh_audio_interface_ops(bk_usbh_audio_device *uac_device, uint8_t intf, uint8_t entity_id, uint8_t cs, uint16_t len, uint32_t delay_ms)
{
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

    uint32_t ret;

    uac_device->control_setup->bmRequestType = MUSB_DIR_OUT | MUSB_TYPE_STANDARD | MUSB_RECIP_INTERFACE;
    uac_device->control_setup->bRequest = MUSB_REQ_SET_INTERFACE;
    uac_device->control_setup->wValue = cs;
    uac_device->control_setup->wIndex = (entity_id << 8) | intf;
    uac_device->control_setup->wLength = len;

	uac_device->control_transfer_buffer_info->pOutBuffer = NULL;
	uac_device->control_transfer_buffer_info->pInBuffer = NULL;
	uac_device->control_transfer_buffer_info->dwOutLength = len;
	uac_device->control_transfer_buffer_info->dwInLength = len;
	uac_device->control_transfer_buffer_info->pCompleteParam = uac_device->control_setup;

    ret = bk_usb_control_transfer(uac_device->control_setup, uac_device->control_transfer_buffer_info);
    if (ret < 0) {
        return ret;
    }
    if(s_uac_transfer_wait_complete_sem) {
        rtos_get_semaphore(&s_uac_transfer_wait_complete_sem, delay_ms);
    }

    return ret;
}

bk_err_t bk_uac_start_mic_handle(bk_usbh_audio_device *uac_device)
{
	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	void *parameter = {0};

	uac_device->control_transfer_buffer_info->pOutBuffer = (uint8_t *)&uac_device->uac_mic_set_samfreq;
	uac_device->control_transfer_buffer_info->pInBuffer = (uint8_t *)&uac_device->uac_mic_set_samfreq;
	parameter = (void *)bk_usbh_audio_commit_control_callback;
	uac_device->control_transfer_buffer_info->pfIrpComplete = parameter;

	if(uac_device->uac_mic_data_ep_desc->bmAttributes & AUDIO_EP_CONTROL_SAMPLING_FEQ)
		bk_usbh_audio_endpoint_selectors(uac_device, uac_device->uac_mic_ep_desc->bEndpointAddress, 0x0, AUDIO_EP_CONTROL_SAMPLING_FEQ, 3, 20);
	
	bk_usbh_audio_interface_ops(uac_device, uac_device->uac_mic_stream_intf_desc->bInterfaceNumber, 0x00, uac_device->uac_mic_stream_intf_desc->bAlternateSetting, 0, 50);

	if(uac_device->uac_mic_data_ep_desc->bmAttributes & AUDIO_EP_CONTROL_SAMPLING_FEQ)
		bk_usbh_audio_endpoint_selectors(uac_device, uac_device->uac_mic_ep_desc->bEndpointAddress, 0x0, AUDIO_EP_CONTROL_SAMPLING_FEQ, 3, 20);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_stop_mic_handle(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	struct bk_usb_isoch_irp *pIsochIrp = uac_device->uac_mic_isoch_irp;

	bk_usbh_audio_interface_ops(uac_device, uac_device->uac_mic_stream_intf_desc->bInterfaceNumber, 0x00, 0x0, 0, 50);
	bk_usb_cancel_transfer(USB_ENDPOINT_ISOCH_TRANSFER, pIsochIrp);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_start_speaker_handle(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	void *parameter = {0};

	uac_device->control_transfer_buffer_info->pOutBuffer = (uint8_t *)&uac_device->uac_spk_set_samfreq;
	uac_device->control_transfer_buffer_info->pInBuffer = (uint8_t *)&uac_device->uac_spk_set_samfreq;
	parameter = (void *)bk_usbh_audio_commit_control_callback;
	uac_device->control_transfer_buffer_info->pfIrpComplete = parameter;

	if(uac_device->uac_spk_data_ep_desc->bmAttributes & AUDIO_EP_CONTROL_SAMPLING_FEQ)
		bk_usbh_audio_endpoint_selectors(uac_device, uac_device->uac_spk_ep_desc->bEndpointAddress, 0x0, AUDIO_EP_CONTROL_SAMPLING_FEQ, 3, 20);
	bk_usbh_audio_interface_ops(uac_device, uac_device->uac_spk_stream_intf_desc->bInterfaceNumber, 0x00, uac_device->uac_spk_stream_intf_desc->bAlternateSetting, 0, 50);

	if(uac_device->uac_spk_data_ep_desc->bmAttributes & AUDIO_EP_CONTROL_SAMPLING_FEQ)
		bk_usbh_audio_endpoint_selectors(uac_device, uac_device->uac_spk_ep_desc->bEndpointAddress, 0x0, AUDIO_EP_CONTROL_SAMPLING_FEQ, 3, 20);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_stop_speaker_handle(bk_usbh_audio_device *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uac_device)
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	struct bk_usb_isoch_irp *pIsochIrp = uac_device->uac_spk_isoch_irp;

	bk_usbh_audio_interface_ops(uac_device, uac_device->uac_spk_stream_intf_desc->bInterfaceNumber, 0x00, 0x0, 0, 50);
	bk_usb_cancel_transfer(USB_ENDPOINT_ISOCH_TRANSFER, pIsochIrp);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_trigger_audio_stream_rx(void *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uac_device) {
		USB_DRIVER_LOGE("[+]%s uac_device is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	bk_usbh_audio_device *device = uac_device;
	if(!(device->uac_mic_as_pipe_ptr)) {
		USB_DRIVER_LOGE("[+]%s pipe_ptr is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	struct bk_usb_isoch_irp *pIsochIrp = device->uac_mic_isoch_irp;
	if(!(pIsochIrp->pBuffer)) {
		USB_DRIVER_LOGE("[+]%s buffer is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	bk_usb_transfer_request(USB_ENDPOINT_ISOCH_TRANSFER, pIsochIrp);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_uac_trigger_audio_stream_tx(void *uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uac_device){
		USB_DRIVER_LOGE("[+]%s uac_device is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	bk_usbh_audio_device *device = uac_device;
	if(!device->uac_spk_as_pipe_ptr) {
		USB_DRIVER_LOGE("[+]%s pipe_ptr is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	struct bk_usb_isoch_irp *pIsochIrp = device->uac_spk_isoch_irp;
	if(!pIsochIrp->pBuffer) {
		USB_DRIVER_LOGE("[+]%s buffer is null\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	bk_usb_transfer_request(USB_ENDPOINT_ISOCH_TRANSFER, pIsochIrp);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

