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
#include <driver/gpio_types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
typedef struct{
    TUYA_PIN_NAME_E pin;
    TUYA_PIN_FUNC_E func;
    gpio_dev_t      dev;
}TUYA_PIN_FUNC_MAP_T;
/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

static TUYA_PIN_FUNC_MAP_T pin_func_map[] = {
    {TUYA_IO_PIN_17, TUYA_SPI0_MISO, GPIO_DEV_SPI0_MISO},
    {TUYA_IO_PIN_16, TUYA_SPI0_MOSI, GPIO_DEV_SPI0_MOSI},
    {TUYA_IO_PIN_14, TUYA_SPI0_CLK,  GPIO_DEV_SPI0_SCK},
    {TUYA_IO_PIN_15, TUYA_SPI0_CS, GPIO_DEV_SPI0_CSN},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
extern VOID_T __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E scl_pin);
extern VOID_T __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E sda_pin);
extern gpio_id_t tkl_gpio_get_bk_gpio_id(TUYA_GPIO_NUM_E pin_id);

TUYA_PIN_FUNC_MAP_T *tkl_pinmux_get_func_map(TUYA_PIN_FUNC_E pin_func)
{
    for (int i = 0; i < sizeof(pin_func_map) / sizeof(TUYA_PIN_FUNC_MAP_T); i++) {
        if (pin_func_map[i].func == pin_func) {
            return &pin_func_map[i];
        }
    }

    return NULL;
}


/**
 * @brief tuya io pinmux func
 *
 * @param[in] pin: pin number
 * @param[in] pin_func: pin function
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    switch (pin_func) {
        case TUYA_IIC0_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_0, pin);
            break;
        case TUYA_IIC0_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_0, pin);
            break;
        case TUYA_IIC1_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_1, pin);
            break;
        case TUYA_IIC1_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_1, pin);
            break;
        case TUYA_IIC2_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_2, pin);
            break;
        case TUYA_IIC2_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_2, pin);
            break;
#if 0
        case TUYA_IIC3_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_3, pin);
            break;
        case TUYA_IIC3_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_3, pin);
            break;
        case TUYA_IIC4_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_4, pin);
            break;
        case TUYA_IIC4_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_4, pin);
            break;
        case TUYA_IIC5_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_5, pin);
            break;
        case TUYA_IIC5_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_5, pin);
            break;
#endif

        case TUYA_SPI0_MISO:
        case TUYA_SPI0_MOSI:
        case TUYA_SPI0_CLK: 
        case TUYA_SPI0_CS:{
            TUYA_PIN_FUNC_MAP_T *map = tkl_pinmux_get_func_map(pin_func);
            if(map == NULL) {
                bk_printf("pin_func %d not found\r\n", pin_func);
                return OPRT_INVALID_PARM;
            }

            map->pin = pin;
        }
            break;

        default:
            break;

    }
    return OPRT_OK;
}
INT32_T tkl_io_pin_to_func(UINT32_T pin, TUYA_PIN_TYPE_E pin_type)
{
	INT32_T port_channel = OPRT_NOT_SUPPORTED;

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
            } else if (TUYA_IO_PIN_23 == pin) {
                port_channel = ADC_3;
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

gpio_id_t ty_get_dev_io(gpio_dev_t dev)
{
    TUYA_PIN_FUNC_MAP_T *map;

    for (int i = 0; i < sizeof(pin_func_map) / sizeof(TUYA_PIN_FUNC_MAP_T); i++) {
        if (pin_func_map[i].dev == dev) {
            return tkl_gpio_get_bk_gpio_id(pin_func_map[i].pin);
        }
    }

    return GPIO_NUM;
}
