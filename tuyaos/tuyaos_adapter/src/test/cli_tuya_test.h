/*
 * cli_tuya_test.h
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CLI_TUYA_TEST_H
#define CLI_TUYA_TEST_H

#include "FreeRTOS.h"
#include "task.h"

#include "sdkconfig.h"
#include "tuya_cloud_types.h"
#include "tkl_ipc.h"

#define __PRINT_MACRO(x) #x
#define PRINT_MACRO(x) #x"="__PRINT_MACRO(x)
//#pragma message(PRINT_MACRO(AON_RTC_DEFAULT_CLOCK_FREQ))
#define TUYA_CLI_MAX_ARG_NUM    8
#define TUYA_CLI_ARG_MAX_LEN    16

// lcd select
#define USED_LCD_T50P181CQ          1

#ifdef USED_LCD_ST7701SN
    #define CLI_TEST_LCD_IC_NAME    "st7701sn"
    #define CLI_TEST_LCD_WIDTH      480
    #define CLI_TEST_LCD_HEIGHT     854
#elif USED_LCD_T50P181CQ
    #define CLI_TEST_LCD_IC_NAME    "T50P181CQ"
    #define CLI_TEST_LCD_WIDTH      480
    #define CLI_TEST_LCD_HEIGHT     864
#endif

extern char *__test_ic_name;
#define TEST_SDCARD_MOUNT_POINT "/sdcard"

#if (CONFIG_CPU_INDEX == 1)
void cli_wifi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_xlcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_adc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_gpio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_tuya_media_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_xqspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_xqspi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_littlefs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
// void cli_tkl_timer_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_timer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_usb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_sdcard_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_spi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_spi_2_3_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_eth_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_iperf_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_speaker_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_mic_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_tuya_i2s_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_xmtd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
int cli_mp3_init(void);
int cli_dwt_init(void);
#endif // CONFIG_SYS_CPU1

#endif /* !CLI_TUYA_TEST_H */
