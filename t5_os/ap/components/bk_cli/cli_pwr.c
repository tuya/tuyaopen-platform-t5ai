#include "cli.h"
#include "bk_manual_ps.h"
#include "bk_mac_ps.h"
#include "bk_mcu_ps.h"
#include "bk_ps.h"
#include "bk_wifi.h"
#include "modules/pm.h"
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/mailbox_channel.h>
#include <driver/gpio.h>
#include <driver/touch.h>
#include <driver/touch_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include <driver/timer.h>
#include <driver/trng.h>
#include <driver/pwr_clk.h>
#include <driver/rosc_32k.h>
#include <driver/rosc_ppm.h>
#if CONFIG_PM_DEMO_ENABLE
#include "pm_ap_demo.h"
#include <driver/pm_ap_core.h>
#endif

#if CONFIG_SYSTEM_CTRL
#define PM_MANUAL_LOW_VOL_VOTE_ENABLE          (0)
#define PM_DEEP_SLEEP_REGISTER_CALLBACK_ENABLE (0x1)

static UINT32 s_cli_sleep_mode = 0;
static UINT32 s_pm_vote1       = 0;
static UINT32 s_pm_vote2       = 0;
static UINT32 s_pm_vote3       = 0;

extern void stop_cpu1_core(void);

#if CONFIG_TOUCH
void cli_pm_touch_callback(void *param)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	BK_LOGD(NULL, "cli_pm_touch_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif
void cli_pm_gpio_callback(gpio_id_t gpio_id)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	BK_LOGD(NULL, "cli_pm_gpio_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}

#define PM_MANUAL_LOW_VOL_VOTE_ENABLE    (0)
#define PM_DEEPSLEEP_RTC_THRESHOLD       (500)
#define PM_SHUTDOWN_RTC_THRESHOLD        (4)        //=500ms
#define PM_OLD_TOUCH_WAKE_SOURCE         (4)
extern void stop_cpu1_core(void);

static void cli_pm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode = 0;
	UINT32 pm_vote1 = 0,pm_vote2 = 0,pm_vote3=0;
	UINT32 pm_wake_source = 0;
	UINT32 pm_param1 = 0,pm_param2 = 0,pm_param3 = 0;
	//rtc_wakeup_param_t      rtc_wakeup_param         = {0};
	system_wakeup_param_t   system_wakeup_param      = {0};
	#if CONFIG_TOUCH
	touch_wakeup_param_t    touch_wakeup_param       = {0};
	#endif
	usbplug_wakeup_param_t  usbplug_wakeup_param     = {0};

	if (argc != 9) 
	{
		BK_LOGD(NULL, "set low power parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode  = os_strtoul(argv[1], NULL, 10);
	pm_wake_source = os_strtoul(argv[2], NULL, 10);
	pm_vote1       = os_strtoul(argv[3], NULL, 10);
	pm_vote2       = os_strtoul(argv[4], NULL, 10);
	pm_vote3       = os_strtoul(argv[5], NULL, 10);
	pm_param1      = os_strtoul(argv[6], NULL, 10);
	pm_param2      = os_strtoul(argv[7], NULL, 10);
	pm_param3      = os_strtoul(argv[8], NULL, 10);

	BK_LOGD(NULL, "cli_pm_cmd %d %d %d %d %d %d %d!!! \r\n",
				pm_sleep_mode,
				pm_wake_source,
				pm_vote1,
				pm_vote2,
				pm_vote3,
				pm_param1,
				pm_param2);
	if((pm_sleep_mode > PM_MODE_DEFAULT)||(pm_wake_source > PM_WAKEUP_SOURCE_INT_NONE))
	{
		BK_LOGD(NULL, "set low power  parameter value  invalid\r\n");
		return;
	}

	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if((pm_vote1 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote2 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote3 > PM_POWER_MODULE_NAME_NONE))
		{
			BK_LOGD(NULL, "set pm vote deepsleep parameter value invalid\r\n");
			return;
		}
	}

	if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if((pm_vote1 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote2 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote3 > PM_SLEEP_MODULE_NAME_MAX))
		{
			BK_LOGD(NULL, "set pm vote low vol parameter value invalid\r\n");
			return;
		}
	}

	s_cli_sleep_mode = pm_sleep_mode;
	s_pm_vote1 = pm_vote1;
	s_pm_vote2 = pm_vote2;
	s_pm_vote3 = pm_vote3;

	/*set wakeup source*/
	if(pm_wake_source == PM_WAKEUP_SOURCE_INT_RTC)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_RTC_ANA_WAKEUP_SUPPORT
			if(pm_param1 < PM_SHUTDOWN_RTC_THRESHOLD)
			{
				BK_LOGD(NULL, "param %d invalid ! must > %d which means 500ms.\r\n",pm_param1,PM_SHUTDOWN_RTC_THRESHOLD);
				return;
			}
			bk_rtc_ana_register_wakeup_source(pm_param1);
			/*workaround fix for unexpecting wakeup from super deep*/
			sys_drv_gpio_ana_wakeup_enable(1, GPIO_4, GPIO_INT_TYPE_MAX);
			#endif
		}
		else
		{
			pm_rtc_wakeup_config_t rtc_wakeup = {0};
			rtc_wakeup.rtc_period = pm_param1;//10s
			bk_pm_ap_rtc_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_RTC,&rtc_wakeup);
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_GPIO)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_GPIO_ANA_WAKEUP_SUPPORT
			bk_gpio_ana_register_wakeup_source(pm_param1,pm_param2);
			#endif
		}
		else
		{
			pm_gpio_wakeup_config_t gpio_wakeup= {pm_param1,pm_param2};
			bk_pm_ap_gpio_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_GPIO,&gpio_wakeup);
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE)
	{   
		if(pm_param1 == WIFI_WAKEUP)
		{
			system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
		}
		else
		{
			system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
		}

		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, &system_wakeup_param);
	}
	else if((pm_wake_source == PM_WAKEUP_SOURCE_INT_TOUCHED)
		||(pm_wake_source == PM_OLD_TOUCH_WAKE_SOURCE))//bk7256 touch wakeup source value is 4,in order to adapt new project for old cmd
	{
		#if CONFIG_TOUCH
		touch_wakeup_param.touch_channel = pm_param1;
		bk_touch_register_touch_isr((1<< touch_wakeup_param.touch_channel), cli_pm_touch_callback, NULL);
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_TOUCHED, &touch_wakeup_param);
		#endif
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_USBPLUG)
	{
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_USBPLUG, &usbplug_wakeup_param);
	}
	else
	{
		;
	}

	/*vote*/
	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if(pm_vote3 == PM_POWER_MODULE_NAME_CPU1)
		{
			#if 1 && (CONFIG_CPU_CNT > 1)
				stop_cpu1_core();
			#endif
		}

	}
	else if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if PM_MANUAL_LOW_VOL_VOTE_ENABLE
		if(pm_vote1 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,0x0);
		}

		if(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,0x0);
		}

		if(pm_vote3 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,0x0);
		}
		#endif

		if((pm_vote1 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote3 == PM_SLEEP_MODULE_NAME_APP))
		{
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,pm_param3);
		}
	}
	else
	{
		;//do something
	}

	bk_pm_ap_sleep_mode_set(pm_sleep_mode);
}
static void cli_pm_debug(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_debug  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set low power debug parameter invalid %d\r\n",argc);
		return;
	}

	pm_debug = os_strtoul(argv[1], NULL, 10);

	//pm_debug_ctrl(pm_debug);

	if(pm_debug == PM_DEBUG_CTRL_STATE)
	{
		//pm_debug_pwr_clk_state();
		//pm_debug_lv_state();
		BK_LOGD(NULL, "Deepsleep wakeup source[%d]\r\n",bk_pm_deep_sleep_wakeup_source_get());
		BK_LOGD(NULL, "Low vol wakeup source[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
	}
	/*for temp debug*/
	if(pm_debug == 16)
	{

	}

	if(pm_debug == 32)
	{

	}
}
static void cli_pm_vote_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode   = 0;
	UINT32 pm_vote         = 0;
	UINT32 pm_vote_value   = 0;
	UINT32 pm_sleep_time   = 0;
	if (argc != 5)
	{
		BK_LOGD(NULL, "set low power vote parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode        = os_strtoul(argv[1], NULL, 10);
	pm_vote              = os_strtoul(argv[2], NULL, 10);
	pm_vote_value        = os_strtoul(argv[3], NULL, 10);
	pm_sleep_time        = os_strtoul(argv[4], NULL, 10);
	if((pm_sleep_mode > PM_MODE_DEFAULT)|| (pm_vote > PM_SLEEP_MODULE_NAME_MAX)||(pm_vote_value > 1))
	{
		BK_LOGD(NULL, "set low power vote parameter value  invalid\r\n");
		return;
	}
	/*vote*/
	if(pm_sleep_mode == LOW_POWER_DEEP_SLEEP)
	{
		if((pm_vote == POWER_MODULE_NAME_BTSP)||(pm_vote == POWER_MODULE_NAME_WIFIP_MAC))
		{
			bk_pm_module_vote_power_ctrl(pm_vote,pm_vote_value);
		}
	}
	else if(pm_sleep_mode == LOW_POWER_MODE_LOW_VOLTAGE)
	{
		bk_pm_module_vote_sleep_ctrl(pm_vote,pm_vote_value,pm_sleep_time);
	}
	else
	{
		;//do something
	}
	pm_printf_current_temperature();
}
#if CONFIG_DEBUG_VERSION
static void cli_pm_vol(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_vol  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm voltage parameter invalid %d\r\n",argc);
		return;
	}

	pm_vol = os_strtoul(argv[1], NULL, 10);
	if ((pm_vol < 0) || (pm_vol > 7))
	{
		BK_LOGD(NULL, "set pm voltage value invalid %d\r\n",pm_vol);
		return;
	}

	bk_pm_lp_vol_set(pm_vol);

}
static void cli_pm_clk(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_clk_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm clk parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_clk_state = os_strtoul(argv[2], NULL, 10);
	if ((pm_clk_state < 0) || (pm_clk_state > 1) || (pm_module_id < 0) || (pm_module_id > 31))
	{
		BK_LOGD(NULL, "set pm clk value invalid %d %d\r\n",pm_clk_state,pm_module_id);
		return;
	}
	bk_pm_clock_ctrl(pm_module_id,pm_clk_state);

}
static void cli_pm_power(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_power_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm power parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_power_state = os_strtoul(argv[2], NULL, 10);
	if (pm_power_state > 1)
	{
		BK_LOGD(NULL, "set pm power value invalid %d %d \r\n",pm_power_state,pm_module_id);
		return;
	}

	bk_pm_module_vote_power_ctrl(pm_module_id,pm_power_state);

}
static void cli_pm_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_freq  = 0;
	UINT32 pm_module_id  = 0;
	pm_cpu_freq_e module_freq = 0;
	pm_cpu_freq_e current_max_freq = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm freq parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_freq = os_strtoul(argv[2], NULL, 10);
	if ((pm_freq > PM_CPU_FRQ_DEFAULT) || (pm_module_id > PM_DEV_ID_MAX))
	{
		BK_LOGD(NULL, "set pm freq value invalid %d %d \r\n",pm_freq,pm_module_id);
		return;
	}

	bk_pm_module_vote_cpu_freq(pm_module_id,pm_freq);

	module_freq =  bk_pm_module_current_cpu_freq_get(pm_module_id);

	current_max_freq = bk_pm_current_max_cpu_freq_get();

	BK_LOGD(NULL, "pm cpu freq test id: %d; freq: %d; current max cpu freq: %d;\r\n",pm_module_id,module_freq,current_max_freq);

}
static void cli_pm_lpo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
	UINT32 pm_lpo  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm lpo parameter invalid %d\r\n",argc);
		return;
	}

	pm_lpo = os_strtoul(argv[1], NULL, 10);
	if ((pm_lpo < 0) || (pm_lpo > 3))
	{
		BK_LOGD(NULL, "set  pm lpo value invalid %d\r\n",pm_lpo);
		return;
	}

	bk_pm_lpo_src_set(pm_lpo);
#endif
}
static void cli_pm_ctrl(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm ctrl parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL, "set pm ctrl value invalid %d\r\n",pm_ctrl);
		return;
	}

	bk_pm_mcu_pm_ctrl(pm_ctrl);

}
static void cli_pm_pwr_state(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_pwr_module        = 0;
	UINT32 pm_pwr_module_state  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm pwr state parameter invalid %d\r\n",argc);
		return;
	}

	pm_pwr_module = os_strtoul(argv[1], NULL, 10);
	if ((pm_pwr_module < 0) || (pm_pwr_module >= PM_POWER_MODULE_NAME_NONE))
	{
		BK_LOGD(NULL, "pm module[%d] not support ,get power state fail\r\n",pm_pwr_module);
		return;
	}

	pm_pwr_module_state = bk_pm_module_power_state_get(pm_pwr_module);
	BK_LOGD(NULL, "Get module[%d] power state[%d] \r\n",pm_pwr_module,pm_pwr_module_state);

}
static void cli_pm_auto_vote(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm auto_vote parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL, "set pm auto vote value invalid %d\r\n",pm_ctrl);
		return;
	}
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
}

#define CLI_DVFS_FREQUNCY_DIV_MAX      (15)
#define CLI_DVFS_FREQUNCY_DIV_BUS_MAX  (1)
static void cli_dvfs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 cksel_core = 0;
	UINT32 ckdiv_core = 0;
	UINT32 ckdiv_bus  = 0;
	UINT32 ckdiv_cpu0 = 0;
	UINT32 ckdiv_cpu1 = 0;

	if (argc != 6) 
	{
		BK_LOGD(NULL, "set dvfs parameter invalid %d\r\n",argc);
		return;
	}

	GLOBAL_INT_DECLARATION();
	cksel_core   = os_strtoul(argv[1], NULL, 10);
	ckdiv_core   = os_strtoul(argv[2], NULL, 10);
	ckdiv_bus    = os_strtoul(argv[3], NULL, 10);
	ckdiv_cpu0   = os_strtoul(argv[4], NULL, 10);
	ckdiv_cpu1   = os_strtoul(argv[5], NULL, 10);

	BK_LOGD(NULL, "cli_dvfs_cmd %d %d %d %d %d !!! \r\n",
				cksel_core,
				ckdiv_core,
				ckdiv_bus,
				ckdiv_cpu0,
				ckdiv_cpu1);
	GLOBAL_INT_DISABLE();
	if(cksel_core > 3)
	{
		BK_LOGD(NULL, "set dvfs cksel core > 3 invalid %d\r\n",cksel_core);
		GLOBAL_INT_RESTORE();
		return;
	}

	if((ckdiv_core > CLI_DVFS_FREQUNCY_DIV_MAX) || (ckdiv_bus > CLI_DVFS_FREQUNCY_DIV_BUS_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX))
	{
		BK_LOGD(NULL, "set dvfs ckdiv_core ckdiv_bus ckdiv_cpu0  ckdiv_cpu0  > 15 invalid\r\n");
		GLOBAL_INT_RESTORE();
		return;
	}
	pm_core_bus_clock_ctrl(cksel_core, ckdiv_core,ckdiv_bus, ckdiv_cpu0,ckdiv_cpu1);
	GLOBAL_INT_RESTORE();
	BK_LOGD(NULL, "switch cpu frequency ok 0x%x 0x%x 0x%x\r\n",sys_drv_all_modules_clk_div_get(CLK_DIV_REG0),sys_drv_cpu_clk_div_get(0),sys_drv_cpu_clk_div_get(1));
}

typedef struct{
	uint32_t cksel_core;  // 0:XTAL       1 : clk_DCO      2 : 320M      3 : 480M
	uint32_t ckdiv_core;  // Frequency division : F/(1+N), N is the data of the reg value 0--15
	uint32_t ckdiv_bus;   // Frequency division : F/(1+N), N is the data of the reg value:0--1
	uint32_t ckdiv_cpu0;  // Frequency division : F/(1+N), N is the data of the reg value:0--15
	uint32_t ckdiv_cpu1;  // 0: cpu0,cpu1 and bus  sel same clock frequence 1: cpu0_clk and  bus_clk is half cpu1_clk
}core_bus_clock_ctrl_t;

#define CORE_BUS_CLOCK_AUTO_TEST 0
#if CORE_BUS_CLOCK_AUTO_TEST
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x2,0x2,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x0,0x0,0x0,0x1 },  /*2 : 320M  ckdiv_cpu1 = 1*/ \
	{0x2,0x1,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x2,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M ckdiv_cpu1 = 0*/ \
	{0x3,0x3,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M ckdiv_cpu1 = 1*/ \
	{0x3,0x2,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x3,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x1 },  /*3 : 480M */ \
}
#else
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M  ckdiv_cpu1 = 0 */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M  ckdiv_cpu1 = 1 */ \
}
#endif
#define DVFS_AUTO_TEST_COUNT (2)
extern void bk_delay_us(uint32 num);
static void cli_dvfs_auto_test_timer_isr(timer_id_t chan)
{
	// uint8_t rand_num;
	// uint8_t i;
	// for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	// {
	// 	rand_num = (uint32_t)bk_rand()%PM_CPU_FRQ_DEFAULT;
	// 	//BK_LOGD(NULL, "dvfs random %d \r\n",rand_num);
	// 	bk_delay_us(5);
	// 	sys_drv_switch_cpu_bus_freq(rand_num);
	// }
}

core_bus_clock_ctrl_t core_bus_clock[] = CORE_BUS_CLOCK_MAP;
static void cli_dvfs_auto_test_all_timer_isr(timer_id_t chan)
{
#if 0
	uint8_t rand_num;
	uint8_t i;
	
	for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	{
		rand_num = (uint32_t)bk_rand()%(sizeof(core_bus_clock)/sizeof(core_bus_clock_ctrl_t));
		//BK_LOGD(NULL, "dvfs random %d \r\n",rand_num);
		//BK_LOGD(NULL, "[cksel:%d] [ckdiv_core:%d] [ckdiv_bus:%d] [ckdiv_cpu0:%d] [ckdiv_cpu1:%d]\r\n",
			//core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
		bk_delay_us(5);
		pm_core_bus_clock_ctrl(core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
	}
#endif
}

static void cli_dvfs_auto_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t period_us;
	
	if (argc != 3)
	{
		BK_LOGD(NULL, "set dvfs_auto_test parameter invalid %d\r\n",argc);
		return;
	}
	period_us = os_strtoul(argv[1], NULL, 10);
	BK_LOGD(NULL, "dvfs auto test period set %d us!\r\n",period_us);

	//bk_trng_driver_init();
	//bk_trng_start();
	
	if (os_strcmp(argv[2], "default") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
	else if (os_strcmp(argv[2], "all") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_all_timer_isr);
	}
	else
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
}

static void cli_pm_wakeup_source(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 sleep_mode = 0;

	if (argc != 2)
	{
		BK_LOGD(NULL, "set get wakeup source parameter invalid %d\r\n",argc);
		return;
	}

	sleep_mode   = os_strtoul(argv[1], NULL, 10);
	if(sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if 0
		BK_LOGD(NULL, "low voltage wakeup source [%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
		#endif
	}
	else if(sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		#if 0
		BK_LOGD(NULL, "deepsleep wakeup source [%d]\r\n",bk_pm_deep_sleep_wakeup_source_get());
		#endif
	}
	else
	{
		BK_LOGD(NULL, "it not support the sleep mode[%d] for wakeup source \r\n",sleep_mode);
	}

}

#if (CONFIG_CPU_CNT > 2)
static void cli_pm_boot_cp2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 boot_cp2_state = 0;
	UINT32 module_name    = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "cp2 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_cp2_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_cp2_ctrl(module_name,boot_cp2_state);
}
#endif//CONFIG_CPU_CNT > 2
static void cli_pm_boot_cp1(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1 && (CONFIG_CPU_CNT > 1)
	UINT32 boot_cp1_state = 0;
	UINT32 module_name    = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "cp1 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_cp1_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_cp1_ctrl(module_name,boot_cp1_state);
#endif
}
#endif//CONFIG_DEBUG_VERSION

#endif//CONFIG_SYSTEM_CTRL

static void cli_pm_demo_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "init") == 0)
	{
		CLI_LOGD("pm demo init\r\n");
		pm_demo_thread_main();
	} 
	else if (os_strcmp(argv[1], "deep_sleep") == 0) 
	{
		CLI_LOGD("pm demo deep sleep\r\n");
		#if CONFIG_PM_DEMO_ENABLE
		pm_ap_core_msg_t msg = {0};
		msg.event = PM_DEMO_ENTER_DEEP_SLEEP;
		bk_pm_demo_send_msg(&msg);
		#endif
	} 
	else if (os_strcmp(argv[1], "low_vol") == 0)
	{
		CLI_LOGD("pm demo low vol\r\n");
		#if CONFIG_PM_DEMO_ENABLE
		pm_ap_core_msg_t msg = {0};
		msg.event = PM_DEMO_ENTER_LOW_VOLTAGE;
		bk_pm_demo_send_msg(&msg);
		#endif
	} 
	else 
	{
		CLI_LOGD("pm demo unknown cmd\r\n");
		return;
	}
}
#define PWR_CMD_CNT (sizeof(s_pwr_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwr_commands[] = {
#if CONFIG_SYSTEM_CTRL
#if CONFIG_DEBUG_VERSION
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"dvfs", "dvfs [cksel_core] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]", cli_dvfs_cmd},
	{"dvfs_auto_test", "dvfs_auto_test [period]", cli_dvfs_auto_test},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
	{"pm_lpo", "pm_lpo [lpo_type]", cli_pm_lpo},
	{"pm_vol", "pm_vol [vol_value]", cli_pm_vol},
	{"pm_clk", "pm_clk [module_name][clk_state]", cli_pm_clk},
	{"pm_power", "pm_power [module_name][ power state]", cli_pm_power},
	{"pm_freq", "pm_freq [module_name][ frequency]", cli_pm_freq},
	{"pm_ctrl", "pm_ctrl [ctrl_value]", cli_pm_ctrl},
	{"pm_pwr_state", "pm_pwr_state [pwr_state]", cli_pm_pwr_state},
	{"pm_auto_vote", "pm_auto_vote [auto_vote_value]", cli_pm_auto_vote},
	{"pm_wakeup_source", "pm_wakeup_source [pm_sleep_mode]", cli_pm_wakeup_source},
#if (CONFIG_CPU_CNT > 2)
	{"pm_boot_cp2", "pm_boot_cp2 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp2},
#endif
	{"pm_boot_cp1", "pm_boot_cp1 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp1},
	{"pm_demo", "pm_demo {init||send_cmd|config_data}", cli_pm_demo_cmd},
#else
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
#endif //CONFIG_DEBUG_VERSION
#endif //CONFIG_SYSTEM_CTRL
};

int cli_pwr_init(void)
{
	return cli_register_commands(s_pwr_commands, PWR_CMD_CNT);
}
