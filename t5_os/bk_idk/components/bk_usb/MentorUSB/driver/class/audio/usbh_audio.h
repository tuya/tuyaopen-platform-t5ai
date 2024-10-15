#ifndef _USBH_AUDIO_H_
#define _USBH_AUDIO_H_

#include <components/usb.h>
#include <components/usb_types.h>
#include <modules/bk_musb.h>
#include "usb_driver.h"

typedef struct
{
	uint16_t version;
	uint16_t current_device;
	struct usb_interface_descriptor_t *uac_interface_desc_staging_port;
	struct usb_interface_descriptor_t *uac_control_intf_desc;
	/* UAC Microphone*/
	struct audio_cs_if_ac_input_terminal_descriptor *uac_mic_input_terminal_desc;
	struct audio_cs_if_ac_feature_unit_descriptor *uac_mic_featrue_unit_desc;
	struct audio_cs_if_ac_output_terminal_descriptor *uac_mic_output_terminal_desc;
	struct usb_interface_descriptor_t *uac_mic_stream_intf_desc;
	struct audio_cs_if_as_general_descriptor *uac_mic_stream_general_intf_desc;
	struct audio_cs_if_as_format_type_descriptor *uac_mic_stream_format_type_desc;
	struct audio_ep_descriptor *uac_mic_ep_desc;
	struct audio_cs_ep_ep_general_descriptor *uac_mic_data_ep_desc;
	uint32_t uac_mic_set_samfreq;
	/* UAC Speaker*/
	struct audio_cs_if_ac_input_terminal_descriptor *uac_spk_input_terminal_desc;
	struct audio_cs_if_ac_feature_unit_descriptor *uac_spk_featrue_unit_desc;
	struct audio_cs_if_ac_output_terminal_descriptor *uac_spk_output_terminal_desc;
	struct usb_interface_descriptor_t *uac_spk_stream_intf_desc;
	struct audio_cs_if_as_general_descriptor *uac_spk_stream_general_intf_desc;
	struct audio_cs_if_as_format_type_descriptor *uac_spk_stream_format_type_desc;
	struct audio_ep_descriptor *uac_spk_ep_desc;
	struct audio_cs_ep_ep_general_descriptor *uac_spk_data_ep_desc;
	uint32_t uac_spk_set_samfreq;
	uint32_t uac_spk_attribute;

	bk_usb_pipe_ptr uac_mic_cs_pipe_ptr;
	bk_usb_pipe_ptr uac_mic_as_pipe_ptr;
	bk_usb_pipe_ptr uac_spk_cs_pipe_ptr;
	bk_usb_pipe_ptr uac_spk_as_pipe_ptr;

	bk_usb_driver_comprehensive_ops *usb_driver;

	s_usb_device_request *control_setup;
	s_usb_transfer_buffer_info *control_transfer_buffer_info;
	struct bk_usb_isoch_irp *uac_mic_isoch_irp;
	uint8_t *uac_mic_as_buffer;
	uint32_t uac_mic_as_buffer_len;
	struct bk_usb_isoch_irp *uac_spk_isoch_irp;
	uint8_t *uac_spk_as_buffer;
	uint32_t uac_spk_as_buffer_len;
}bk_usbh_audio_device;

bk_err_t bk_usbh_audio_sw_init(bk_usbh_audio_device *uac_device);
bk_err_t bk_usbh_audio_sw_deinit(bk_usbh_audio_device *uac_device);
void bk_usb_parse_audio_control_descriptors(uint8_t *buffer);
void bk_usb_parse_audio_streaming_descriptors(uint8_t *buffer);
void bk_usb_updata_audio_interface(uint8_t interface_sub_class, uint8_t *buffer);
void bk_usb_updata_audio_endpoint(uint8_t interface_sub_class, uint8_t *buffer);
void bk_usb_updata_audio_data_endpoint_descriptor(uint8_t *buffer);
void bk_usb_audio_open_pipe(bk_usbh_audio_device *uac_device);
void bk_usb_audio_close_pipe(bk_usbh_audio_device *uac_device);

uint32_t bk_usbh_audio_get_attribute(bk_usbh_audio_device *uac_device, uint8_t request, uint8_t intf, uint8_t entity_id, uint16_t cs, uint16_t len, uint32_t delay_ms);
uint32_t bk_usbh_audio_set_cur(bk_usbh_audio_device *uac_device, uint8_t intf, uint8_t entity_id, uint16_t cs, uint16_t len, uint32_t delay_ms);

bk_err_t bk_uac_start_mic_handle(bk_usbh_audio_device *uac_device);
bk_err_t bk_uac_stop_mic_handle(bk_usbh_audio_device *uac_device);
bk_err_t bk_uac_start_speaker_handle(bk_usbh_audio_device *uac_device);
bk_err_t bk_uac_stop_speaker_handle(bk_usbh_audio_device *uac_device);

bk_err_t bk_uac_trigger_audio_stream_rx(void *uac_device);
bk_err_t bk_uac_trigger_audio_stream_tx(void *uac_device);

#endif
