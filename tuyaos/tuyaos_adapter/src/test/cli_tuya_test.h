/*
 * cli_tuya_test.h
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CLI_TUYA_TEST_H
#define CLI_TUYA_TEST_H

#include "sdkconfig.h"
#include "tuya_cloud_types.h"


void cli_xlcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_adc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_gpio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_wifi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#endif /* !CLI_TUYA_TEST_H */
