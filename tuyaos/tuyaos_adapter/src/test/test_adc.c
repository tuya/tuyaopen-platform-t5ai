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
#include "tkl_adc.h"
#include "tkl_pinmux.h"

void cli_adc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#define ADC_CHAN_NUM    5
#define ADC_CONV_TIMES  8

    uint8_t chan_list[ADC_CHAN_NUM] = {25, 24, 28, 12, 13};
    TUYA_ADC_BASE_CFG_T tkl_cfg;
    INT32_T adc_chan0 = tkl_io_pin_to_func(12, TUYA_IO_TYPE_ADC);
    INT32_T adc_chan1 = tkl_io_pin_to_func(13, TUYA_IO_TYPE_ADC);
    INT32_T adc_chan2 = tkl_io_pin_to_func(24, TUYA_IO_TYPE_ADC);
    INT32_T adc_chan3 = tkl_io_pin_to_func(25, TUYA_IO_TYPE_ADC);
    INT32_T adc_chan4 = tkl_io_pin_to_func(28, TUYA_IO_TYPE_ADC);

    tkl_cfg.ch_list.data = BIT(adc_chan0 & 0xFF);
    tkl_cfg.ch_list.data |= BIT(adc_chan1 & 0xFF);
    tkl_cfg.ch_list.data |= BIT(adc_chan2 & 0xFF);
    tkl_cfg.ch_list.data |= BIT(adc_chan3 & 0xFF);
    tkl_cfg.ch_list.data |= BIT(adc_chan4 & 0xFF);

    tkl_cfg.ch_nums = ADC_CHAN_NUM;
    tkl_cfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
    tkl_cfg.width = 12;
    tkl_cfg.mode = TUYA_ADC_CONTINUOUS;
    tkl_cfg.conv_cnt = ADC_CONV_TIMES;
    tkl_adc_init(0, &tkl_cfg);

    INT32_T buffer[ADC_CHAN_NUM][ADC_CONV_TIMES] = {0};
    memset(buffer, 0, sizeof(buffer));

    tkl_adc_read_voltage(0, (INT32_T *)buffer, ADC_CHAN_NUM * ADC_CONV_TIMES);
    uint8_t outbuf[128] = {0};

    snprintf(outbuf, 128, "%s", "adc input voltage:\r\n");
    tkl_log_output(outbuf);

    for (int i = 0; i < ADC_CHAN_NUM; i++) {
        memset(outbuf, 0, 128);
        snprintf(outbuf, 128, "ch: %2d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d\r\n", chan_list[i],
                buffer[i][0], buffer[i][1], buffer[i][2], buffer[i][3],
                buffer[i][4], buffer[i][5], buffer[i][6], buffer[i][7]);
        tkl_log_output(outbuf);
    }

    // bk_printf("adc input voltage:\r\n");
    // for (int i = 0; i < ADC_CHAN_NUM; i++) {
    //     bk_printf("ch: %d, ", chan_list[i]);
    //     for (int j = 0; j < ADC_CONV_TIMES; j++) {
    //         bk_printf(" %d", buffer[i][j]);
    //     }
    //     bk_printf("\r\n");
    // }
    // bk_printf("\r\n");
}


