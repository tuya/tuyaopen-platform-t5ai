#include <common/bk_include.h>
#include <common/bk_err.h>
#if ((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && (CONFIG_OTP_V1)
#include <driver/otp.h>
#endif
#include <os/os.h>
#include <math.h>
#include <stdlib.h>

#include "temp_detect_pub.h"
#include "temp_detect.h"
#include "volt_detect.h"

#include "bk_saradc.h"
#include "sdmadc/sdmadc_driver.h"

#define TAG "sensor"
#define ADC_SWITCH_DELT                   2

//TODO: move daemon code in temp_detect.c to this file

#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
typedef struct {
	beken_mutex_t lock;
	float         temperature;
	float         voltage;
} MCU_SENSOR_INFO;
MCU_SENSOR_INFO g_sensor_info;
#endif

#if ((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && (CONFIG_OTP_V1)
static bk_err_t bk_sensor_load_adc_cali_value(void)
{
    uint16_t values[2];
    bk_err_t result;

    result = bk_otp_apb_read(OTP_GADC_CALIBRATION, (uint8_t *)&values, sizeof(values));
    if ((result != BK_OK) || (values[0] == 0) || (values[1] == 0)) {
        BK_LOGW(TAG, "uncali saradc value:[%x %x]\r\n", values[0], values[1]);
        goto LOAD_SDMADC;
    }

    BK_LOGD(TAG, "saradc low value:[%x]\r\n", values[0]);
    BK_LOGD(TAG, "saradc high value:[%x]\r\n", values[1]);
    saradc_set_calibrate_val(&values[0], SARADC_CALIBRATE_LOW);
    saradc_set_calibrate_val(&values[1], SARADC_CALIBRATE_HIGH);

LOAD_SDMADC:
    result = bk_otp_apb_read(OTP_SDMADC_CALIBRATION, (uint8_t *)&values, sizeof(values));
    if ((result != BK_OK) || (values[0] == 0) || (values[1] == 0)) {
        BK_LOGW(TAG, "uncali sdmadc value:[%x %x]\r\n", values[0], values[1]);
        goto FAILURE;
    }

    BK_LOGD(TAG, "sdmadc low value:[%x]\r\n", values[0]);
    BK_LOGD(TAG, "sdmadc high value:[%x]\r\n", values[1]);
    bk_sdmadc_set_calibrate_val(values[0], SARADC_CALIBRATE_LOW);
    bk_sdmadc_set_calibrate_val(values[1], SARADC_CALIBRATE_HIGH);

    return BK_OK;

FAILURE:
    return BK_FAIL;
}
#else
static bk_err_t bk_sensor_load_adc_cali_value(void)
{
    return BK_FAIL;
}
#endif

bk_err_t bk_sensor_init(void)
{
    bk_err_t ret = BK_OK;

    bk_sensor_load_adc_cali_value();
#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
    if (NULL == g_sensor_info.lock)
    {
        ret = rtos_init_mutex(&g_sensor_info.lock);
        g_sensor_info.voltage    = NAN;
        g_sensor_info.temperature = NAN;
    }

#if CONFIG_PHY_MB
    bk_err_t ret_mb = BK_OK;
    extern bk_err_t mb_phy_ipc_init(void);
    ret_mb = mb_phy_ipc_init();
    if(ret_mb != BK_OK)
    {
        BK_LOGE("phy_driver", "mb_phy_ipc_init failed %d.\r\n", ret);
    }
    else
    {
#if (CONFIG_CPU_CNT > 1)
        extern bk_err_t bk_phy_server_init(void);
        ret_mb = bk_phy_server_init();
        if(ret_mb != BK_OK)
        {
            BK_LOGE("phy_driver", "phy svr create failed %d.\r\n", ret);
        }
#endif//CONFIG_CPU_CNT
    }
#endif//CONFIG_PHY_MB
#endif //CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT
    return ret;
}

bk_err_t bk_sensor_deinit(void)
{
	bk_err_t ret = BK_OK;

#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
	if (NULL != g_sensor_info.lock)
	{
		ret = rtos_deinit_mutex(&g_sensor_info.lock);
		g_sensor_info.lock       = NULL;
		g_sensor_info.voltage    = NAN;
		g_sensor_info.temperature = NAN;
	}
#endif
	return ret;
}

#if (CONFIG_TEMP_DETECT)
bk_err_t bk_sensor_set_current_temperature(float temperature)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	g_sensor_info.temperature = temperature;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

bk_err_t bk_sensor_get_current_temperature(float *temperature)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	if (isnan(g_sensor_info.temperature))
	{
		return BK_ERR_TRY_AGAIN;
	}

	if (NULL == temperature)
	{
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	*temperature = g_sensor_info.temperature;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}
#endif

#if (CONFIG_VOLT_DETECT)
bk_err_t bk_sensor_set_current_voltage(float voltage)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	g_sensor_info.voltage = voltage;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

bk_err_t bk_sensor_get_current_voltage(float *voltage)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	if (isnan(g_sensor_info.voltage))
	{
		return BK_ERR_TRY_AGAIN;
	}

	if (NULL == voltage)
	{
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	*voltage = g_sensor_info.voltage;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

#endif
