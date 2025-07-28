#include <common/bk_include.h>
#include "sdkconfig.h"
#include "stdarg.h"
#include <os/mem.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_sys_ctrl.h"
#include "bk_cli.h"
#include "bk_uart.h"
#include <os/str.h>
#include <components/log.h>
#include <driver/media_types.h>
#include "drv_model_pub.h"
#include "cli.h"
#include <components/usb.h>
#include <components/usb_types.h>
#include <driver/audio_ring_buff.h>

#if CONFIG_USB
#if CONFIG_USBH_MSC
#include "ff.h"
#include "diskio.h"
#endif

static void cli_usb_help(void)
{
	CLI_LOGV("cli_usb_help!\r\n");
}

#if CONFIG_USBH_MSC

static FATFS *pfs = NULL;
uint8_t mount_flag = 0;
extern void usbh_msc_register();

static void udisk_test_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "register") == 0)
	{
		BK_LOGD(NULL, "usbh_msc_register \r\n");
		usbh_msc_register();
	}

	if (os_strcmp(argv[1], "read") == 0)
	{
		FIL file;
		const TCHAR *file_name = argv[2];
		UINT read_len = os_strtoul(argv[3], NULL, 10);
		UINT rd_pos = os_strtoul(argv[4], NULL, 10);

		uint8_t print_buf[64];
		UINT print_size = 0;

		BK_LOG_ON_ERR(f_open(&file, file_name, FA_OPEN_APPEND | FA_READ));
		BK_LOG_ON_ERR(f_lseek(&file, rd_pos));
		BK_LOG_ON_ERR(f_read(&file, print_buf, read_len, &print_size));
		BK_LOG_ON_ERR(f_close(&file));
		print_hex_dump(NULL, print_buf, read_len);
	}
	if (os_strcmp(argv[1], "write") == 0)
	{
		FIL file;
		const TCHAR *file_name = argv[2];
		UINT write_len = os_strtoul(argv[3], NULL, 10);
		UINT st_value = os_strtoul(argv[4], NULL, 10);
		UINT wr_pos = os_strtoul(argv[5], NULL, 10);

		uint8_t write_buf[64];
		// init a array to write
		for (int i = 0; i < 64; i++)
		{
			write_buf[i] = st_value & 0xFF;
		}
		UINT wr_size = 0;

		BK_LOG_ON_ERR(f_open(&file, file_name, FA_OPEN_APPEND | FA_WRITE));
		BK_LOG_ON_ERR(f_lseek(&file, wr_pos));
		BK_LOG_ON_ERR(f_write(&file, write_buf, write_len, &wr_size));
		BK_LOG_ON_ERR(f_close(&file));
	}
	if (os_strcmp(argv[1], "mkdir") == 0)
	{
		const TCHAR *file_name = argv[2];
		BK_LOG_ON_ERR(f_mkdir(file_name));
	}
}


void usb_mount_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (pfs)
		os_free(pfs);

	pfs = os_malloc(sizeof(FATFS));
	if (NULL == pfs) {
		CLI_LOGV("f_mount malloc failed!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = f_mount(pfs, sys_path, 1);
	if (fr != FR_OK) {
		mount_flag = 0;
		CLI_LOGD("usb mount failed:%d\r\n", fr);
	} else {
		mount_flag = 1;
		CLI_LOGD("usb mount OK!\r\n");
	}
}



void usb_unmount_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (mount_flag != 1) {
		CLI_LOGV("usb hasn't initialization!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = f_mount(NULL, sys_path, 1);
	if (fr != FR_OK) {
		CLI_LOGV("unmount %s fail.\r\n", sys_path);
		return;
	}

	if (pfs) {
		os_free(pfs);
		pfs = NULL;
	}

	mount_flag = 0;
	CLI_LOGV("usb unmount OK!\r\n");
}

static FRESULT usb_scan_files(char *path)
{
	FRESULT fr;
	DIR dir;
	FILINFO fno;
	char path_temp[255];

	fr = f_opendir(&dir, path);
	if (fr == FR_OK) {
		while (1) {
			fr = f_readdir(&dir, &fno);
			if (fr != FR_OK)
				break;
			if (fno.fname[0] == 0)
				break;
			if (fno.fattrib & AM_DIR) {
				/* It is a directory */
				os_memset(path_temp, 0, sizeof(path_temp));
				sprintf(path_temp, "%s/%s", path, fno.fname);
				fr = usb_scan_files(path_temp);
				if (fr != FR_OK) break;
			} else {
				/* It is a file. */
				CLI_LOGD("%s/%s\r\n", path, fno.fname);
			}
		}
		f_closedir(&dir);
	} else
		CLI_LOGV("f_opendir failed\r\n");

	return fr;
}

void usb_ls_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (mount_flag != 1) {
		CLI_LOGV("usb hasn't initialization!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = usb_scan_files(sys_path);
	if (fr != FR_OK)
		CLI_LOGV("scan_files failed!\r\n");
}
#endif

#if CONFIG_USB_UVC_DEBUG
void uvc_view_support_attribute(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	E_UVC_ATTRIBUTE_T attribute;
	for(attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION; attribute < UVC_ATTRIBUTE_NUM; attribute++)
	{
        switch(attribute)
        {
            case UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT BACKLIGHT_COMPENSATION: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT BACKLIGHT_COMPENSATION: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_BRIGHTNESS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT BRIGHTNESS: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT BRIGHTNESS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_CONTRAST:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT CONTRAST: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT CONTRAST: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_GAIN:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT GAIN: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT GAIN: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_POWER_LINE_FREQUENCY:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT POWER_LINE_FREQUENCY: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT POWER_LINE_FREQUENCY: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_HUE:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT HUE: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT HUE: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_SATURATION:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT SATURATION: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT SATURATION: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_SHARPNESS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT SHARPNESS: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT SHARPNESS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_GAMMA:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT GAMMA: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT GAMMA: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_TEMPERATURE: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_TEMPERATURE: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_TEMPERATURE_AUTO: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_TEMPERATURE_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_COMPONENT: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_COMPONENT: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_COMPONENT_AUTO: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT WHITE_BALANCE_COMPONENT_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_DIGITAL_MULTIPLIER:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT DIGITAL_MULTIPLIER: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT DIGITAL_MULTIPLIER: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT DIGITAL_MULTIPLIER_LIMIT: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT DIGITAL_MULTIPLIER_LIMIT: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_HUE_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT HUE_AUTO: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT HUE_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_ANALOG_LOCK_STATUS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT ANALOG_LOCK_STATUS: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT ANALOG_LOCK_STATUS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGV("UVC SUPPORT ANALOG_VIDEO_STANDARD: 0  NO\r\n");
                else
                    CLI_LOGV("UVC SUPPORT ANALOG_VIDEO_STANDARD: 1  YES\r\n");
                break;
            default:
                break;
        }
	}
}

void uvc_get_param(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_usb_help();
		return;
	}
	uint32_t attribute = 0x00;
	uint32_t param = 0;

	if (os_strcmp(argv[1], "backlight") == 0) {
		attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION;
	} else if (os_strcmp(argv[1], "brightness") == 0) {
		attribute = UVC_ATTRIBUTE_BRIGHTNESS;
	} else if (os_strcmp(argv[1], "contrast") == 0) {
		attribute = UVC_ATTRIBUTE_CONTRAST;
	} else if (os_strcmp(argv[1], "gain") == 0) {
		attribute = UVC_ATTRIBUTE_GAIN;
	} else if (os_strcmp(argv[1], "powerlinefre") == 0) {
		attribute = UVC_ATTRIBUTE_POWER_LINE_FREQUENCY;
	} else if (os_strcmp(argv[1], "hue") == 0) {
		attribute = UVC_ATTRIBUTE_HUE;
	} else if (os_strcmp(argv[1], "hueauto") == 0) {
		attribute = UVC_ATTRIBUTE_HUE_AUTO;
	} else if (os_strcmp(argv[1], "saturation") == 0) {
		attribute = UVC_ATTRIBUTE_SATURATION;
	} else if (os_strcmp(argv[1], "sharpness") == 0) {
		attribute = UVC_ATTRIBUTE_SHARPNESS;
	} else if (os_strcmp(argv[1], "gamma") == 0) {
		attribute = UVC_ATTRIBUTE_GAMMA;
	} else if (os_strcmp(argv[1], "wbt") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE;
	} else if (os_strcmp(argv[1], "wbtauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO;
	} else if (os_strcmp(argv[1], "wbc") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT;
	} else if (os_strcmp(argv[1], "wbcauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO;
	} else if (os_strcmp(argv[1], "digmu") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER;
	} else if (os_strcmp(argv[1], "digmulimit") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT;
	} else if (os_strcmp(argv[1], "analogvideo") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD;
	} else if (os_strcmp(argv[1], "analoglock") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_LOCK_STATUS;
	} else {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[2], "cur") == 0) {
		CLI_LOGV("uvc cur 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_CUR, attribute, &param));
	} else if (os_strcmp(argv[2], "min") == 0) {
		CLI_LOGV("uvc min 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_MIN, attribute, &param));
	} else if (os_strcmp(argv[2], "max") == 0) {
		CLI_LOGV("uvc max 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_MAX, attribute, &param));
	} else if (os_strcmp(argv[2], "info") == 0) {
		CLI_LOGV("uvc info 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_INFO, attribute, &param));
	} else if (os_strcmp(argv[2], "len") == 0) {
		CLI_LOGV("uvc len 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_LEN, attribute, &param));
	} else if (os_strcmp(argv[2], "res") == 0) {
		CLI_LOGV("uvc res 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_RES, attribute, &param));
	} else if (os_strcmp(argv[2], "def") == 0) {
		CLI_LOGV("uvc def 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_DEF, attribute, &param));
	} else {
		cli_usb_help();
		return;
	}
	CLI_LOGV("uvc param: 0x%x!\r\n", param);

}

void uvc_set_param(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_usb_help();
		CLI_LOGV("cli_usb_help!\r\n");
		return;
	}
	uint32_t attribute = 0x00;
	uint32_t param = 0;

	if (os_strcmp(argv[1], "backlight") == 0) {
		attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION;
	} else if (os_strcmp(argv[1], "brightness") == 0) {
		attribute = UVC_ATTRIBUTE_BRIGHTNESS;
	} else if (os_strcmp(argv[1], "contrast") == 0) {
		attribute = UVC_ATTRIBUTE_CONTRAST;
	} else if (os_strcmp(argv[1], "gain") == 0) {
		attribute = UVC_ATTRIBUTE_GAIN;
	} else if (os_strcmp(argv[1], "powerlinefre") == 0) {
		attribute = UVC_ATTRIBUTE_POWER_LINE_FREQUENCY;
	} else if (os_strcmp(argv[1], "hue") == 0) {
		attribute = UVC_ATTRIBUTE_HUE;
	} else if (os_strcmp(argv[1], "hueauto") == 0) {
		attribute = UVC_ATTRIBUTE_HUE_AUTO;
	} else if (os_strcmp(argv[1], "saturation") == 0) {
		attribute = UVC_ATTRIBUTE_SATURATION;
	} else if (os_strcmp(argv[1], "sharpness") == 0) {
		attribute = UVC_ATTRIBUTE_SHARPNESS;
	} else if (os_strcmp(argv[1], "gamma") == 0) {
		attribute = UVC_ATTRIBUTE_GAMMA;
	} else if (os_strcmp(argv[1], "wbt") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE;
	} else if (os_strcmp(argv[1], "wbtauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO;
	} else if (os_strcmp(argv[1], "wbc") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT;
	} else if (os_strcmp(argv[1], "wbcauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO;
	} else if (os_strcmp(argv[1], "digmu") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER;
	} else if (os_strcmp(argv[1], "digmulimit") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT;
	} else if (os_strcmp(argv[1], "analogvideo") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD;
	} else if (os_strcmp(argv[1], "analoglock") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_LOCK_STATUS;
	} else {
		cli_usb_help();
		return;
	}

	param = os_strtoul(argv[2], NULL, 10);
	bk_usb_uvc_attribute_op(USB_ATTRIBUTE_SET_CUR, attribute, &param);

}

static void uvc_disconnect_uvc_configed(void)
{
	CLI_LOGV("fuvc_notify_uvc_disconnect\r\n");
	return;
}

#endif

#if CONFIG_USB_PLUG_IN_OUT
void cli_usb_plug_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		usb_plug_inout_init();
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		usb_plug_inout_deinit();
	} else {
		cli_usb_help();
		return;
	}

}

void cli_usb_plug_inout(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[1], "open") == 0) {
		usb_plug_inout_open();
		CLI_LOGV("cli_usb_plug_inout inout open!\r\n");
	} else if (os_strcmp(argv[1], "close") == 0) {
		usb_plug_inout_close();
		CLI_LOGV("cli_usb_plug_inout inout close!\r\n");
	} else {
		cli_usb_help();
		return;
	}

}
#endif

#if CONFIG_UVC_OTA_DEMO
#include <components/uvc_ota_common.h>
static uint32_t cli_usb_ctrl_sync_callback(void *pContext, void *pControlIrp)
{
	if(!pContext)
		return BK_FAIL;
	s_usb_device_request *pSetup = (s_usb_device_request *)pContext;
	CLI_LOGV("Control_Transfer Done. pSetup->bmRequestType:0x%x\r\n",pSetup->bmRequestType);
	CLI_LOGV("Control_Transfer Done. pSetup->bRequest:0x%x\r\n",pSetup->bRequest);
	CLI_LOGV("Control_Transfer Done. pSetup->wValue:0x%x\r\n",pSetup->wValue);
	CLI_LOGV("Control_Transfer Done. pSetup->wIndex:0x%x\r\n",pSetup->wIndex);
	CLI_LOGV("Control_Transfer Done. pSetup->wLength:0x%x\r\n",pSetup->wLength);
	CLI_LOGV("cli_usb_ctrl_sync_callback Control_Transfer Done.\r\n");
	return BK_OK;
}

void cli_usb_ota_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGV("cli_usb_ota_ops Please Check the number of paramter.\r\n");
		cli_usb_help();
		return;
	}
	s_usb_device_request *pSetup = {0};
	s_usb_transfer_buffer_info *buffer_info  = {0};
	uint8_t *g_ota_outbuffer = {0};
	uint8_t *g_ota_inbuffer = {0};
	void *parameter = {0};
	char verssion[7] = {'V', '1', '.', '0', '.', '2', 0};
	char firewaredata[6] = {0};
	uint32_t file_len = 0;
	bool usb_transfer_sta = 0;
	bool malloc_status_flag = 0;
	bk_err_t ret;

	if (os_strcmp(argv[1], "cmd_self_test") == 0) {
		if (argc < 8) {
			CLI_LOGV("cli_usb_ota_ops Please Check the number of paramter.\r\n");
			return;
		}

		do{
			pSetup = (s_usb_device_request *)os_malloc(sizeof(s_usb_device_request));
			if(!pSetup) {
				CLI_LOGV("%s pSetup malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			buffer_info = (s_usb_transfer_buffer_info *)os_malloc(sizeof(s_usb_transfer_buffer_info));
			if(!buffer_info) {
				CLI_LOGV("%s buffer_info malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			g_ota_outbuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * (pSetup->wLength));
			if(!g_ota_outbuffer) {
				CLI_LOGV("%s g_ota_outbuffer malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			g_ota_inbuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * (pSetup->wLength));
			if(!g_ota_inbuffer) {
				CLI_LOGV("%s g_ota_inbuffer malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}
		}while(0);

		if(malloc_status_flag) {
			if(g_ota_outbuffer != NULL) {
				os_free(g_ota_outbuffer);
				g_ota_outbuffer = NULL;
			}
			if(g_ota_inbuffer != NULL) {
				os_free(g_ota_inbuffer);
				g_ota_inbuffer = NULL;
			}
			if(pSetup != NULL) {
				os_free(pSetup);
				pSetup = NULL;
			}
			if(buffer_info != NULL) {
				os_free(buffer_info);
				buffer_info = NULL;
			}

			return;
		}

		pSetup->bmRequestType = os_strtoul(argv[2], NULL, 16);
		pSetup->bRequest = os_strtoul(argv[3], NULL, 16);
		pSetup->wValue = os_strtoul(argv[4], NULL, 16);
		pSetup->wIndex = os_strtoul(argv[5], NULL, 16);
		pSetup->wLength = os_strtoul(argv[6], NULL, 16);

		for(int i = 0; i < (pSetup->wLength); i++)
		{
			g_ota_outbuffer[i] = i;
			CLI_LOGV("%s Outbuffer[%d] : %x\r\n", __FUNCTION__, i, g_ota_outbuffer[i]);
		}
		os_memset((void *)g_ota_inbuffer, 0x0, (sizeof(uint8_t) * (pSetup->wLength)));

		buffer_info->pOutBuffer = g_ota_outbuffer;
		buffer_info->dwOutLength = pSetup->wLength;
		buffer_info->pInBuffer = g_ota_inbuffer;
		buffer_info->dwInLength = pSetup->wLength;
		parameter = (void *)cli_usb_ctrl_sync_callback;
		buffer_info->pfIrpComplete = parameter;
		buffer_info->pCompleteParam = pSetup;

		ret = bk_usb_control_transfer(pSetup, buffer_info);
		if(ret) {
			CLI_LOGV("bk_usb_control_transfer fail. ret:%d\r\n", ret);
			os_free(g_ota_outbuffer);
			os_free(g_ota_inbuffer);
			os_free(pSetup);
			os_free(buffer_info);
			return;
		}

		for(int j = 0; j < (buffer_info->dwInLength); j++)
			CLI_LOGV("%s Inbuffer[%d] : %x\r\n", __FUNCTION__, j, g_ota_inbuffer[j]);

		os_free(g_ota_outbuffer);
		os_free(g_ota_inbuffer);
		os_free(pSetup);
		os_free(buffer_info);
		CLI_LOGV("cli_usb_ota_ops cmd_self_test.\r\n");
	} else if (os_strcmp(argv[1], "uvc_ota_init") == 0){
		ret = bk_uvc_ota_demo_init();
		if(ret) {
			CLI_LOGV("bk_uvc_ota_demo_init fail. ret:%d\r\n", ret);
			return;
		}
	} else if (os_strcmp(argv[1], "uvc_ota_deinit") == 0){
		ret = bk_uvc_ota_demo_deinit();
		if(ret) {
			CLI_LOGV("bk_uvc_ota_demo_deinit fail. ret:%d\r\n", ret);
			return;
		}
	} else if (os_strcmp(argv[1], "module_version_check") == 0) {
		firewaredata[0] = 'B';
		/* Fill in reasonable numbers based on the actual situation
		 * and some USB modules cannot be upgraded
		 */
		if (argc > 2) {
			file_len = os_strtoul(argv[2], NULL, 10);
		} else
			file_len = 5546649;/* Self test data length */
		ret = bk_uvc_ota_demo_upgrade_check(verssion, firewaredata, file_len);
		if(ret) {
			CLI_LOGV("bk_uvc_ota_demo_upgrade_check fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGV("uvc_ota_demo_upgrade_check cli_test\r\n");

	} else if (os_strcmp(argv[1], "uvc_ota_done_result") == 0){
		uint32_t result = 0;
		ret = bk_uvc_ota_demo_transmit_result(&result);
		if(ret) {
			CLI_LOGV("bk_uvc_ota_demo_transmit_result fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGV("uvc_ota_done_result result:%d\r\n", result);
	} else if (os_strcmp(argv[1], "uvc_ota_persent") == 0){
		int persent = 0;
		ret = bk_uvc_ota_demo_get_upgrade_persent(&persent);
		if(ret) {
			CLI_LOGV("bk_uvc_ota_demo_get_upgrade_persent fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGV("uvc_ota_persent persent:%d\r\n", persent);
	} else if (os_strcmp(argv[1], "uvc_ota_http_percentage") == 0){
		float percentage = 0;
		ret = bk_uvc_ota_http_to_device_status(&percentage, &usb_transfer_sta);
		if(ret) {
			CLI_LOGV("bk_uvc_ota_http_to_device_status fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGV("uvc_ota_http_percentage percentage::(%.2f)%%  usb_transfer_sta:%d\r\n", percentage, usb_transfer_sta);
	} else if (os_strcmp(argv[1], "uvc_ota_http_download") == 0){
		if (argc != 4) {
			CLI_LOGV("Usage:http_ota [url:]\r\n");
			return;
		}
		uint32_t uri_length = os_strlen(argv[2]) + 1;
		uint8_t wait_ms = os_strtoul(argv[3], NULL, 10);

		ret = bk_uvc_ota_http_download(argv[2], uri_length, wait_ms);
		if(ret) {
			CLI_LOGV("bk_uvc_ota_http_download fail. ret:%d\r\n", ret);
			return;
		}
	} else{
		CLI_LOGV("cli_usb_ota_ops Please Check the number of paramter.\r\n");
	}

}
#endif

#if CONFIG_USB_CDC_ACM_DEMO

void cli_usb_cdc_acm_demo_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGV("%s Please Check the number of paramter.\r\n", __func__);
		cli_usb_help();
		return;
	}
	/* Firstly */
	if (os_strcmp(argv[1], "demo") == 0) {
		// extern void bk_usb_cdc_demo(void);
		// bk_usb_cdc_demo();
	}
	/* Secondly */
	else if (os_strcmp(argv[1], "open") == 0) {
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
		bk_usb_open(USB_HOST_MODE);
	}
}

#endif

#if CONFIG_USB_DEVICE && CONFIG_USBD_HID
extern void bk_usbd_hid_mouse_init(void);
extern void bk_usbd_hid_mouse_deinit(void);
extern void bk_usbd_hid_mouse_cps_check_test(uint8_t c_button);

void cli_usbd_hid_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "mouse_init") == 0) {
		bk_usbd_hid_mouse_init();
	} else if (os_strcmp(argv[1], "mouse_deinit") == 0) {
		bk_usbd_hid_mouse_deinit();
	} else if (os_strcmp(argv[1], "mouse_left_test") == 0) {
		bk_usbd_hid_mouse_cps_check_test(0x1 << 0);
	} else if (os_strcmp(argv[1], "mouse_right_test") == 0) {
		bk_usbd_hid_mouse_cps_check_test(0x1 << 1);
	} else if (os_strcmp(argv[1], "mouse_middle_test") == 0) {
		bk_usbd_hid_mouse_cps_check_test(0x1 << 2);
	} else {
		cli_usb_help();
		return;
	}

}
#endif

#if CONFIG_USBH_SERIAL_CH340_TEST
#include <driver/trng.h>
#include <components/usbh_simulate_uart_api.h>

void cli_usbh_to_uart_rx_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
}

void cli_usbh_to_uart_tx_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
}

void cli_usbh_to_uart_connect_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
}

void cli_usbh_to_uart_disconnect_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
}

void cli_usbh_to_uart_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	uint32_t id = 0xFF;
	uint32_t cnt = 0;
	uint32_t timeout = 0;
	bk_err_t ret = 0;

	id = os_strtoul(argv[2], NULL, 10);

	if (os_strcmp(argv[1], "driver_init") == 0) {
		extern bk_err_t bk_usbh_to_uart_simulate_driver_init(usb_simulate_uart_id_t id);
		bk_usbh_to_uart_simulate_driver_init(id);
	} else if (os_strcmp(argv[1], "driver_deinit") == 0) {
		extern bk_err_t bk_usbh_to_uart_simulate_driver_deinit(usb_simulate_uart_id_t id);
		bk_usbh_to_uart_simulate_driver_deinit(id);
	} else if (os_strcmp(argv[1], "reg_cb") == 0) {
		usb_simulate_uart_callback cb;
		cb.rx_cb = cli_usbh_to_uart_rx_cb;
		cb.rx_arg = NULL;
		cb.tx_cb = cli_usbh_to_uart_tx_cb;
		cb.tx_arg = NULL;
		cb.connect_cb = cli_usbh_to_uart_connect_cb;
		cb.connect_arg = NULL;
		cb.disconnect_cb = cli_usbh_to_uart_disconnect_cb;
		cb.disconnect_arg = NULL;

		bk_usbh_to_uart_simulate_register_callback(id, &cb);
	} else if (os_strcmp(argv[1], "unreg_cb") == 0) {
		usb_simulate_uart_callback cb;
		cb.rx_cb = NULL;
		cb.rx_arg = NULL;
		cb.tx_cb = NULL;
		cb.tx_arg = NULL;
		cb.connect_cb = NULL;
		cb.connect_arg = NULL;
		cb.disconnect_cb = NULL;
		cb.disconnect_arg = NULL;

		bk_usbh_to_uart_simulate_register_callback(id, &cb);
	} else if (os_strcmp(argv[1], "read") == 0) {
		cnt = os_strtoul(argv[3], NULL, 10);
		timeout = os_strtoul(argv[4], NULL, 10);
		uint8_t *data = os_malloc(cnt);
		if(!data) {
			CLI_LOGD("write malloc fail\r\n");
			return;
		}
		memset(data, 0, cnt);
		ret = bk_usbh_to_uart_simulate_read(id, data, cnt, timeout);
		CLI_LOGD("ret: %d\r\n", ret);
		if(ret > 0) {
			for(uint32_t i = 0; i < ret; i++)
			{
				BK_LOGD(NULL, "data[%d]:%x\r\n",i, data[i]);
			}
		}
		CLI_LOGD("\r\n");

		os_free(data);
	} else if (os_strcmp(argv[1], "write") == 0) {
		cnt = os_strtoul(argv[3], NULL, 10);
		timeout = os_strtoul(argv[4], NULL, 10);
		uint32_t random_1 = 0;
		uint8_t *data = os_malloc(cnt);
		if(!data) {
			CLI_LOGD("write malloc fail\r\n");
			return;
		}

		for(uint32_t index = 0; index < cnt; (index += 4)) {
			random_1 = bk_rand();
			data[index] = (random_1 >> 0) & 0xFF;
			data[index + 1] = (random_1 >> 8) & 0xFF;
			data[index + 2] = (random_1 >> 16) & 0xFF;
			data[index + 3] = (random_1 >> 24) & 0xFF;
		}

		ret = bk_usbh_to_uart_simulate_write(id, data, cnt, timeout);
		CLI_LOGD("ret: %d\r\n", ret);

		os_free(data);
	} else if(os_strcmp(argv[1], "init") == 0) {

		uart_config_t config;

		config.data_bits = UART_DATA_8_BITS;    /**< UART data bits */
		config.parity = UART_PARITY_NONE;          /**< UART parity */
		config.stop_bits = UART_STOP_BITS_1;    /**< UART stop bits */
		config.flow_ctrl = UART_FLOWCTRL_CTS_RTS; /**< UART flow control  */
		config.src_clk = UART_SCLK_DCO;        /**< UART source clock */
		config.rx_dma_en = 0;
		config.tx_dma_en = 0;
		ret = bk_usbh_to_uart_simulate_init(id, &config);
		CLI_LOGD("ret: %d\r\n", ret);
	} else if(os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_usbh_to_uart_simulate_deinit(id);
		CLI_LOGD("ret: %d\r\n", ret);
	} else if(os_strcmp(argv[1], "set_b") == 0) {
		cnt = os_strtoul(argv[3], NULL, 10);
		bk_usbh_to_uart_simulate_set_baud_rate(id, cnt);
	} else if(os_strcmp(argv[1], "check") == 0) {
		ret = bk_usbh_to_uart_simulate_check_device(id);
		CLI_LOGD("ret: %d\r\n", ret);
	} else {
		cli_usb_help();
		return;
	}

}

#define SIMUALTE_UART_DEMO_SEND 1
#define SIMUALTE_UART_DEMO_RECEIVE 2
typedef struct {
	int op;
	void *param;
} simulate_uart_demo_msg_t;

typedef struct {
	usb_simulate_uart_id_t id;
	uint8_t *data;
	uint32_t cnt;
	uint32_t timeout;
} simulate_uart_demo_data_ops;

static 	beken_thread_t s_simulate_uart_demo_thread;
static beken_queue_t s_simulate_uart_demo_msg_que;
static simulate_uart_demo_data_ops s_tx_data;
static simulate_uart_demo_data_ops s_rx_data;
static bool connect_flag = 0;
static bk_err_t simulate_uart_demo_send_msg(int op, void *param)
{
	bk_err_t ret;
	simulate_uart_demo_msg_t msg;

	msg.op = op;
	msg.param = param;

	if (s_simulate_uart_demo_msg_que) {
		ret = rtos_push_to_queue(&s_simulate_uart_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

void cli_simulate_uart_demo_rx_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGV("%s id %d\r\n", __func__, id);
}

void cli_simulate_uart_demo_tx_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGV("%s id %d\r\n", __func__, id);
}

void cli_simulate_uart_demo_connect_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
	connect_flag = 1;
	simulate_uart_demo_send_msg(SIMUALTE_UART_DEMO_SEND, NULL);
}

void cli_simulate_uart_demo_disconnect_cb(usb_simulate_uart_id_t id, void *argv)
{
	CLI_LOGD("%s id %d\r\n", __func__, id);
	connect_flag = 0;
}
static void simulate_uart_demo_send()
{
	uint32_t random_1 = 0;
	usb_simulate_uart_id_t id = s_tx_data.id;
	uint32_t cnt = s_tx_data.cnt;
	uint32_t timeout = s_tx_data.timeout;
	uint8_t *data = s_tx_data.data;
	int ret = 0;

	if(!data) {
		CLI_LOGD("write buffer fail\r\n");
		return;
	}

	for(uint32_t index = 0; index < cnt; (index += 4)) {
		random_1 = bk_rand();
		data[index] = (random_1 >> 0) & 0xFF;
		data[index + 1] = (random_1 >> 8) & 0xFF;
		data[index + 2] = (random_1 >> 16) & 0xFF;
		data[index + 3] = (random_1 >> 24) & 0xFF;
	}

	ret = bk_usbh_to_uart_simulate_write(id, data, cnt, timeout);
	CLI_LOGV("%s ret: %d\r\n", __func__, ret);
}

static void simulate_uart_demo_check_data(uint32_t cnt)
{
	uint8_t *txdata = s_tx_data.data;
	uint8_t *rxdata = s_rx_data.data;

	for(uint32_t i = 0; i < cnt; i++)
	{
		if(txdata[i] != rxdata[i]) {
			CLI_LOGD("txdata[%d]:%x rxdata[%d]:%x\r\n", i, txdata[i], i, rxdata[i]);
		}
	}
}

static void simulate_uart_demo_receive()
{
	usb_simulate_uart_id_t id = s_rx_data.id;
	uint32_t cnt = s_rx_data.cnt;
	uint32_t timeout = s_rx_data.timeout;
	uint8_t *data = s_rx_data.data;
	int ret = 0;

	if(!data) {
		CLI_LOGD("read buffer fail\r\n");
		return;
	}

	memset(data, 0, cnt);
	ret = bk_usbh_to_uart_simulate_read(id, data, cnt, timeout);
	CLI_LOGV("%s ret: %d\r\n", __func__, ret);
	if(ret > 0) {
		simulate_uart_demo_check_data(ret);
	}
}

static void simulate_uart_demo_msg_handler(void *param)
{
	bk_err_t ret;

	while (1) {
		simulate_uart_demo_msg_t msg;
		ret = rtos_pop_from_queue(&s_simulate_uart_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case SIMUALTE_UART_DEMO_SEND:
					if(!connect_flag) break;
					CLI_LOGD("%s SIMUALTE_UART_DEMO_SEND\r\n", __func__);
					simulate_uart_demo_send();
					simulate_uart_demo_send_msg(SIMUALTE_UART_DEMO_RECEIVE, NULL);
					break;
				case SIMUALTE_UART_DEMO_RECEIVE:
					if(!connect_flag) break;	
					CLI_LOGD("%s SIMUALTE_UART_DEMO_RECEIVE\r\n", __func__);
					simulate_uart_demo_receive();
					simulate_uart_demo_send_msg(SIMUALTE_UART_DEMO_SEND, NULL);
					break;
				default:
					break;
			}
		}
	}
}
void cli_simulate_uart_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	bk_err_t ret = 0;

	if (os_strcmp(argv[1], "init") == 0) {
		CLI_LOGD("cli_simulate_uart_demo init \r\n");
		/* setup 0  prepare parameters */
		s_tx_data.id = os_strtoul(argv[2], NULL, 10);
		s_tx_data.cnt = os_strtoul(argv[3], NULL, 10);
		s_tx_data.timeout = os_strtoul(argv[4], NULL, 10);
		s_tx_data.data = os_malloc(s_tx_data.cnt);
		if(!s_tx_data.data) {
			CLI_LOGD("write malloc fail\r\n");
			return;
		}

		s_rx_data.id = os_strtoul(argv[2], NULL, 10);
		s_rx_data.cnt = os_strtoul(argv[3], NULL, 10);
		s_rx_data.timeout = os_strtoul(argv[4], NULL, 10);
		s_rx_data.data = os_malloc(s_rx_data.cnt);
		if(!s_rx_data.data) {
			CLI_LOGD("read malloc fail\r\n");
			return;
		}
		/* setup 1 register callback */
		usb_simulate_uart_callback cb;
		cb.rx_cb = cli_simulate_uart_demo_rx_cb;
		cb.rx_arg = NULL;
		cb.tx_cb = cli_simulate_uart_demo_tx_cb;
		cb.tx_arg = NULL;
		cb.connect_cb = cli_simulate_uart_demo_connect_cb;
		cb.connect_arg = NULL;
		cb.disconnect_cb = cli_simulate_uart_demo_disconnect_cb;
		cb.disconnect_arg = NULL;
		bk_usbh_to_uart_simulate_register_callback(s_tx_data.id, &cb);

		/* setup 2 init */
		uart_config_t config;
		config.data_bits = UART_DATA_8_BITS;    /**< UART data bits */
		config.parity = UART_PARITY_NONE;          /**< UART parity */
		config.stop_bits = UART_STOP_BITS_1;    /**< UART stop bits */
		config.flow_ctrl = UART_FLOWCTRL_CTS_RTS; /**< UART flow control  */
		config.src_clk = UART_SCLK_DCO;        /**< UART source clock */
		config.rx_dma_en = 0;
		config.tx_dma_en = 0;
		ret = bk_usbh_to_uart_simulate_init(s_tx_data.id, &config);

		/* setup 3  application processing thread */
		ret = rtos_create_thread(&s_simulate_uart_demo_thread, 4, "usb_t_ser_d", simulate_uart_demo_msg_handler, 1024, NULL);
		if (ret != kNoErr) {
			CLI_LOGD("rtos_create_thread failed!!!\r\n");
			return;
		}

		ret = rtos_init_queue(&s_simulate_uart_demo_msg_que, "usb_t_ser_q", sizeof(simulate_uart_demo_msg_t), 10);
		if (ret != kNoErr) {
			CLI_LOGD("create demo message queue fail \r\n");
			return;
		}

		CLI_LOGD("cli_simulate_uart_demo inited \r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		if(s_simulate_uart_demo_thread)
		{
			rtos_delete_thread(&s_simulate_uart_demo_thread);
		}
		if(s_simulate_uart_demo_msg_que) {
			rtos_deinit_queue(&s_simulate_uart_demo_msg_que);
			s_simulate_uart_demo_msg_que = NULL;
		}
		ret = bk_usbh_to_uart_simulate_deinit(s_tx_data.id);
		CLI_LOGD("ret: %d\r\n", ret);
		if(s_tx_data.data) {
			os_free(s_tx_data.data);
		}
		if(s_rx_data.data) {
			os_free(s_rx_data.data);
		}

		s_tx_data.id = os_strtoul(argv[2], NULL, 10);
		s_tx_data.cnt = os_strtoul(argv[3], NULL, 10);
		s_tx_data.timeout = os_strtoul(argv[4], NULL, 10);
		s_tx_data.data = NULL;
		s_rx_data.id = os_strtoul(argv[2], NULL, 10);
		s_rx_data.cnt = os_strtoul(argv[3], NULL, 10);
		s_rx_data.timeout = os_strtoul(argv[4], NULL, 10);
		s_rx_data.data = NULL;
	} else{

	}
}
#endif

#if CONFIG_USBD_MSC
#include "driver/sd_card.h"

void cli_usbd_msc_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "msc_init") == 0) {
		extern int msc_storage_init(void);
		msc_storage_init();
		CLI_LOGD("%s ,line:%d,msc_storage_init\r\n",__FILE__,__LINE__);
	} else if (os_strcmp(argv[1], "msc_deinit") == 0) {
		extern int msc_storage_deinit(void);
		msc_storage_deinit();
		CLI_LOGD("%s ,line:%d,msc_storage_deinit\r\n",__FILE__,__LINE__);
	} else if (os_strcmp(argv[1], "msc_c_vote") == 0) {
		CLI_LOGD("%s ,line:%d, null sdcard\r\n",__FILE__,__LINE__);
	} else {
		cli_usb_help();
		return;
	}

}
#endif

void cli_usb_base_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	uint32_t gpio_id = 0xFF;
	bool poweron = 0;
	uint32_t ret = 0;

	if (os_strcmp(argv[1], "driver_init") == 0) {
		ret = bk_usb_driver_init();
		CLI_LOGD("%s driver_init ret:%d\r\n",__func__, ret);
	} else if (os_strcmp(argv[1], "driver_deinit") == 0) {
		ret = bk_usb_driver_deinit();
		CLI_LOGD("%s driver_deinit ret:%d\r\n",__func__, ret);
	} else if (os_strcmp(argv[1], "power") == 0) {
		gpio_id = os_strtoul(argv[2], NULL, 16);
		poweron = os_strtoul(argv[3], NULL, 16);
		ret = bk_usb_power_ops(gpio_id, poweron);
		CLI_LOGD("%s power_ops gpio_id:%d power_ops:%d ret:%d\r\n",__func__,gpio_id, poweron, ret);
	} else if (os_strcmp(argv[1], "open_host") == 0) {
		CLI_LOGD("cli_usb_open host! %d\r\n", bk_usb_open(0));
	} else if (os_strcmp(argv[1], "open_dev") == 0) {
		CLI_LOGV("cli_usb_open device! %d\r\n", bk_usb_open(1));
	} else if (os_strcmp(argv[1], "close") == 0) {
		CLI_LOGD("cli_usb_open host! %d\r\n", bk_usb_close());
	} else {
		cli_usb_help();
		return;
	}

}

const struct cli_command usb_host_clis[] = {

#if CONFIG_USBH_MSC
	{"usb_mount", "usb mount", usb_mount_command},
	{"usb_unmount", "usb unmount", usb_unmount_command},
	{"usb_ls", "usb list system", usb_ls_command},
	{"usb_op", "usb_read file length", udisk_test_command},
#endif //CONFIG_USBH_MSC

#if CONFIG_USB_PLUG_IN_OUT
	{"usb_plug", "usb plug init|out", cli_usb_plug_init},
	{"usb_plug_inout", "usb open|close", cli_usb_plug_inout},
#endif //CONFIG_USB_PLUG_IN_OUT

#if CONFIG_UVC_OTA_DEMO
	{"usb_ota", "usb_ota cli_usb_ota_ops|uvc_ota_init|uvc_ota_deinit|uvc_ota_http_download", cli_usb_ota_ops},
#endif //CONFIG_UVC_OTA_DEMO

#if CONFIG_USB_CDC_ACM_DEMO
	{"usb_cdc", "usb_cdc_acm_demo ", cli_usb_cdc_acm_demo_ops},
#endif

#if CONFIG_USBH_SERIAL_CH340_TEST
	{"usbh_to_uart", "usbh_to_uart driver_init|driver_deinit|", cli_usbh_to_uart_ops},
	{"simulate_uart_auto", "simulate_uart_auto init|deinit|", cli_simulate_uart_demo},
#endif

#if CONFIG_USBD_MSC
	{"usbd", "usbd msc_init|msc_deinit", cli_usbd_msc_ops},
#endif //CONFIG_USBD_MSC

#if CONFIG_USBD_HID
	{"usbd_hid", "usbd_hid mouse_init|mouse_deinit|mouse_test", cli_usbd_hid_ops},
#endif //CONFIG_USBD_HID

	{"usb", "usb driver_init|driver_deinit|power[gpio_id ops]|open_host|open_dev|close", cli_usb_base_ops},
};

int cli_usb_init(void)
{
	int ret;
	ret = cli_register_commands(usb_host_clis, sizeof(usb_host_clis) / sizeof(struct cli_command));
	if (ret)
		CLI_LOGV("register usb host commands fail.\r\n");

	return ret;
}
#endif
