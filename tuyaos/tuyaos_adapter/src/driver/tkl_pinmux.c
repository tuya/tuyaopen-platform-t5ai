/****************************************************************************
 * @file tkl_pinmux.c
 * @brief this module is used to tkl_pinmux
 * @version 0.0.1
 * @date 2023-06-07
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tkl_pinmux.h"
#include "driver/hal/hal_adc_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int32_t tkl_io_pin_to_func(uint32_t pin, TUYA_PIN_TYPE_E pin_type)
{
	int32_t port_channel = OPRT_NOT_SUPPORTED;
    
    switch (pin_type) {
        case TUYA_IO_TYPE_PWM:                  // all pwm channels belong to one port
            if (TUYA_IO_PIN_18 == pin) {
                port_channel = 0;
            } else if (TUYA_IO_PIN_24 == pin) {
                port_channel = 1;
            } else if (TUYA_IO_PIN_32 == pin) {
                port_channel = 2;
            } else if (TUYA_IO_PIN_34 == pin) {
                port_channel = 3;
            } else if (TUYA_IO_PIN_36 == pin) {
                port_channel = 4;
            }
            break;
        case TUYA_IO_TYPE_ADC:
            if (TUYA_IO_PIN_25 == pin) {
                port_channel = ADC_1;
            } else if (TUYA_IO_PIN_24 == pin) {
                port_channel = ADC_2;
            } else if (TUYA_IO_PIN_28 == pin) {
                port_channel = ADC_4;
            } else if (TUYA_IO_PIN_13 == pin) {
                port_channel = ADC_15;
            } else if (TUYA_IO_PIN_12 == pin) {
                port_channel = ADC_14;
            } else if (TUYA_IO_PIN_1 == pin) {
                port_channel = ADC_13;
            } else if (TUYA_IO_PIN_0 == pin) {
                port_channel = ADC_12;
            }
            break;
        case TUYA_IO_TYPE_DAC:
            break;
        case TUYA_IO_TYPE_UART:
            break;
        case TUYA_IO_TYPE_SPI:
            break;
        case TUYA_IO_TYPE_I2C:
            break;
        case TUYA_IO_TYPE_I2S:
            break;
        case TUYA_IO_TYPE_GPIO:
            break;
        default:
            break;
    }

    return port_channel;
}


