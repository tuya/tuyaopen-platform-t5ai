#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usb_regs_address.h"
#if CONFIG_USB_HOST
#include <components/cherryusb/usbh_core.h>
#endif
#if CONFIG_USB_DEVICE
#include <components/cherryusb/usbd_core.h>
#endif
#if CONFIG_USB_HUB
#include <components/cherryusb/usbh_hub.h>
#endif
#if CONFIG_USBH_UAC
#include <components/cherryusb/usbh_audio.h>
#endif
#if CONFIG_USBH_UVC
#include <components/cherryusb/usbh_video.h>
#endif
#if CONFIG_USB_CDC_ACM_DEMO
#include "usbh_cdc_acm.h"
#include "bk_cherry_usb_cdc_acm_api.h"
#endif
#if CONFIG_USBH_SERIAL_CH340
#include "usbh_ch34x.h"
#endif

static beken_mutex_t s_usb_drv_task_mutex = NULL;
static bool s_usb_driver_init_flag = 0;
static bool s_usb_power_on_flag = 0;
static bool s_usb_open_close_flag = 0;

static bk_err_t usb_driver_sw_deinit();

#define USB_DRIVER_RETURN_NOT_INIT() do {\
	if(!s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)


#define USB_DRIVER_RETURN_NOT_DEINIT() do {\
	if(s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)

#define USB_RETURN_NOT_POWERED_ON() do {\
		if(!s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_POWERED_DOWN() do {\
		if(s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_OPENED() do {\
		if(!s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_OPEN;\
		}\
	} while(0)

#define USB_RETURN_NOT_CLOSED() do {\
		if(s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

static void bk_usb_init_all_device_driver_sw(void)
{
#if CONFIG_USB_HUB
	usbh_hub_class_register();
#endif

#if CONFIG_USBH_UVC
	usbh_uvc_class_register();
#endif
#if CONFIG_USBH_UAC
	usbh_uac_class_register();
#endif

#if CONFIG_USBH_MSC
	extern void usbh_msc_register();
	usbh_msc_register();
#endif

#if CONFIG_USB_CDC
//	extern void usbh_cdc_acm_class_register();
//	usbh_cdc_acm_class_register();
	extern void usbh_cdc_data_class_register();
	usbh_cdc_data_class_register();
#endif
#if CONFIG_USBH_SERIAL_CH340
	usbh_class_serial_ch340_register_driver();
#endif
}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	if (ops)
	{
		USB_RETURN_NOT_POWERED_DOWN();
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_HIGH);
		s_usb_power_on_flag = ops;
	}
	else
	{
		USB_RETURN_NOT_POWERED_ON();
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_LOW);
		s_usb_power_on_flag = ops;
	}

	return BK_OK;
}

bk_err_t bk_usb_driver_init(void)
{
	USB_DRIVER_RETURN_NOT_DEINIT();
	USB_DRIVER_LOGV("[+]%s\r\n",__func__);

	bk_usb_init_all_device_driver_sw();

	if(!s_usb_drv_task_mutex){
		rtos_init_mutex(&s_usb_drv_task_mutex);
	}

	s_usb_driver_init_flag = 1;

	USB_DRIVER_LOGV("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();

	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_int_isr_unregister(INT_SRC_USB);
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_USB_1, CLK_PWR_CTRL_PWR_DOWN);

	if(s_usb_drv_task_mutex) {
		rtos_deinit_mutex(&s_usb_drv_task_mutex);
	}

	s_usb_driver_init_flag = 0;
	return BK_OK;
}

void bk_usb_driver_task_lock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_lock_mutex(&s_usb_drv_task_mutex);
}

void bk_usb_driver_task_unlock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_unlock_mutex(&s_usb_drv_task_mutex);
}

extern void delay(INT32 num);
static void bk_usb_host_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
	delay(100);

	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_TML | R710_USB_USR_CFG_RSTN | R710_USB_USR_REFCLK_MODE | 
                 R710_USB_USR_PLL_EN | R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

	REG_USB_USR_CONFIG |= R710_USB_USR_RESET;
	REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_usb_device_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
    //REG_USB_USR_CONFIG |= (0x0<< 0);
	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_REFCLK_MODE | R710_USB_USR_PLL_EN | R710_USB_USR_RESET|
	             R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM | R710_USB_USR_ID_DIG_REG |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

    REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops)
{
	if(ops){
		sys_drv_usb_clock_ctrl(true, NULL);
		delay(100);

		if(!sys_hal_psram_ldo_status()) {
			sys_drv_psram_ldo_enable(1);
		}
		sys_drv_usb_analog_phy_en(1, NULL);

		if(usb_mode == USB_HOST_MODE) {
			bk_usb_host_custom_register_set();
		} else {
			bk_usb_device_custom_register_set();
		}
	} else {
		sys_drv_usb_analog_phy_en(0, NULL);
		sys_drv_usb_clock_ctrl(false, NULL);
	}
}

void bk_usb_phy_register_refresh()
{
#if CONFIG_USB_HOST
	sys_drv_core_intr_group1_disable(2, USB_INTERRUPT_CTRL_BIT);
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_core_intr_group1_enable(2, USB_INTERRUPT_CTRL_BIT);
	if(s_usb_power_on_flag) {
		bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	}
#endif
}

bk_err_t bk_usb_open(uint32_t usb_mode)
{
	USB_DRIVER_LOGV("[+]%s\r\n", __func__);

	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_CLOSED();
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0, 0);
	USB_DRIVER_LOGI("USB_DRV_USB_OPEN!\r\n");
	if(usb_mode == USB_HOST_MODE) {
#if CONFIG_USB_HOST	
		bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
		usbh_initialize();
#endif
	} else if(usb_mode == USB_DEVICE_MODE){
#if CONFIG_USB_DEVICE
		bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);
		usbd_initialize();
#endif
	} else {
		USB_DRIVER_LOGI("PLEASE check USB mode\r\n");
	}

	s_usb_open_close_flag = 1;

	USB_DRIVER_LOGV("[-]%s\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_usb_close(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_OPENED();

	bk_err_t ret = BK_OK;
	USB_DRIVER_LOGI("USB_DRV_USB_CLOSE!\r\n");
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
#if CONFIG_USB_HOST
	ret = usbh_deinitialize();
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
#endif
#if CONFIG_USB_DEVICE
	ret = usbd_deinitialize();
	bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, false);
#endif
	s_usb_open_close_flag = 0;
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
	return ret;
}


