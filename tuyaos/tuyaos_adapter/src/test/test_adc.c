/*
 * test_adc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

#include "bk_saradc.h"
#include <driver/adc.h>
#include "adc_statis.h"
#include <os/os.h>
#include "sys_driver.h"
#include "tuya_cloud_types.h"

void cli_adc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int32_t buf[10];
    TUYA_ADC_BASE_CFG_T cfg;
    uint16_t value   = 0;
    float cali_value = 0;
    adc_chan_t adc_chan = ADC_3;

    static uint8_t is_init = 0;

    memset(buf, 0, sizeof(buf));

    if (!is_init) {
        BK_LOG_ON_ERR(bk_adc_driver_init());
        is_init = 1;
    }

    BK_LOG_ON_ERR(bk_adc_acquire());
    sys_drv_set_ana_pwd_gadc_buf(1);
    BK_LOG_ON_ERR(bk_adc_init(adc_chan));
    adc_config_t config = {0};

    config.chan = adc_chan;
    config.adc_mode = 3;
    config.src_clk = 1;
    config.clk = 0x30e035;
    config.saturate_mode = 4;
    config.steady_ctrl= 7;
    config.adc_filter = 0;
    if(config.adc_mode == ADC_CONTINUOUS_MODE) {
        config.sample_rate = 0;
    }

    BK_LOG_ON_ERR(bk_adc_set_config(&config));
    BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
    BK_LOG_ON_ERR(bk_adc_start());
    BK_LOG_ON_ERR(bk_adc_read(&value, 1000));

    if(adc_chan == 0)
    {
        //cali_value = ((float)value/4096*5)*1.2*1000;
        cali_value = saradc_calculate(value);
        cali_value = cali_value*5/2;
    }
    else if(adc_chan == 7 || adc_chan == 8 || adc_chan == 9)
    {
        bk_printf("adc_chan %d has been used\r\n", adc_chan);
    }
    else
    {
        //cali_value = ((float)value/4096*2)*1.2*1000;
        cali_value = saradc_calculate(value);
    }

    bk_adc_stop();
    sys_drv_set_ana_pwd_gadc_buf(0);
    bk_adc_deinit(adc_chan);
    bk_adc_release();
    bk_printf("adc read: %d, %f\r\n", value, cali_value * 2);

//    BK_LOG_ON_ERR(bk_adc_driver_deinit());
}


