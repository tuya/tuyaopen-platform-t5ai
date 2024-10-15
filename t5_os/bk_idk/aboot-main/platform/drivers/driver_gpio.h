/*************************************************************
 * @file        driver_gpio.h
 * @brief       Header file of driver_gpio.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_GPIO_H__

#define __DRIVER_GPIO_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "BK_System.h"

#define REG_GPIO_BASE_ADDR                       (0x44000400UL)

#define GPIO_CHANNEL_NUMBER_ALL             32
#define GPIO_CHANNEL_NUMBER_MAX             (GPIO_CHANNEL_NUMBER_ALL - 1)
#define GPIO_CHANNEL_NUMBER_MIN             0

#define REG_GPIO_X_CONFIG_ADDR(x)           (REG_GPIO_BASE_ADDR + (x) * 4)
#define REG_GPIO_0_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x00 * 4)
#define REG_GPIO_1_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x01 * 4)
#define REG_GPIO_2_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x02 * 4)
#define REG_GPIO_3_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x03 * 4)
#define REG_GPIO_4_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x04 * 4)
#define REG_GPIO_5_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x05 * 4)
#define REG_GPIO_6_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x06 * 4)
#define REG_GPIO_7_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x07 * 4)
#define REG_GPIO_8_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x08 * 4)
#define REG_GPIO_9_CONFIG_ADDR              (REG_GPIO_BASE_ADDR + 0x09 * 4)
#define REG_GPIO_10_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0A * 4)
#define REG_GPIO_11_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0B * 4)
#define REG_GPIO_12_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0C * 4)
#define REG_GPIO_13_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0D * 4)
#define REG_GPIO_14_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0E * 4)
#define REG_GPIO_15_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x0F * 4)
#define REG_GPIO_16_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x10 * 4)
#define REG_GPIO_17_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x11 * 4)
#define REG_GPIO_18_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x12 * 4)
#define REG_GPIO_19_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x13 * 4)
#define REG_GPIO_20_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x14 * 4)
#define REG_GPIO_21_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x15 * 4)
#define REG_GPIO_22_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x16 * 4)
#define REG_GPIO_23_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x17 * 4)
#define REG_GPIO_24_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x18 * 4)
#define REG_GPIO_25_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x19 * 4)
#define REG_GPIO_26_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1A * 4)
#define REG_GPIO_27_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1B * 4)
#define REG_GPIO_28_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1C * 4)
#define REG_GPIO_29_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1D * 4)
#define REG_GPIO_30_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1E * 4)
#define REG_GPIO_31_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 0x1F * 4)

#define REG_GPIO_40_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 40*4)
#define REG_GPIO_41_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 41*4)
#define REG_GPIO_42_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 42*4)
#define REG_GPIO_43_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 43*4)
#define REG_GPIO_44_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 44*4)
#define REG_GPIO_45_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 45*4)
#define REG_GPIO_46_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 46*4)
#define REG_GPIO_47_CONFIG_ADDR             (REG_GPIO_BASE_ADDR + 47*4)

#define REG_GPIO_X_CONFIG(x)                (*((volatile unsigned long *) REG_GPIO_X_CONFIG_ADDR(x)))
#define REG_GPIO_0_CONFIG                   (*((volatile unsigned long *) REG_GPIO_0_CONFIG_ADDR))
#define REG_GPIO_1_CONFIG                   (*((volatile unsigned long *) REG_GPIO_1_CONFIG_ADDR))
#define REG_GPIO_2_CONFIG                   (*((volatile unsigned long *) REG_GPIO_2_CONFIG_ADDR))
#define REG_GPIO_3_CONFIG                   (*((volatile unsigned long *) REG_GPIO_3_CONFIG_ADDR))
#define REG_GPIO_4_CONFIG                   (*((volatile unsigned long *) REG_GPIO_4_CONFIG_ADDR))
#define REG_GPIO_5_CONFIG                   (*((volatile unsigned long *) REG_GPIO_5_CONFIG_ADDR))
#define REG_GPIO_6_CONFIG                   (*((volatile unsigned long *) REG_GPIO_6_CONFIG_ADDR))
#define REG_GPIO_7_CONFIG                   (*((volatile unsigned long *) REG_GPIO_7_CONFIG_ADDR))
#define REG_GPIO_8_CONFIG                   (*((volatile unsigned long *) REG_GPIO_8_CONFIG_ADDR))
#define REG_GPIO_9_CONFIG                   (*((volatile unsigned long *) REG_GPIO_9_CONFIG_ADDR))
#define REG_GPIO_10_CONFIG                  (*((volatile unsigned long *) REG_GPIO_10_CONFIG_ADDR))
#define REG_GPIO_11_CONFIG                  (*((volatile unsigned long *) REG_GPIO_11_CONFIG_ADDR))
#define REG_GPIO_12_CONFIG                  (*((volatile unsigned long *) REG_GPIO_12_CONFIG_ADDR))
#define REG_GPIO_13_CONFIG                  (*((volatile unsigned long *) REG_GPIO_13_CONFIG_ADDR))
#define REG_GPIO_14_CONFIG                  (*((volatile unsigned long *) REG_GPIO_14_CONFIG_ADDR))
#define REG_GPIO_15_CONFIG                  (*((volatile unsigned long *) REG_GPIO_15_CONFIG_ADDR))
#define REG_GPIO_16_CONFIG                  (*((volatile unsigned long *) REG_GPIO_16_CONFIG_ADDR))
#define REG_GPIO_17_CONFIG                  (*((volatile unsigned long *) REG_GPIO_17_CONFIG_ADDR))
#define REG_GPIO_18_CONFIG                  (*((volatile unsigned long *) REG_GPIO_18_CONFIG_ADDR))
#define REG_GPIO_19_CONFIG                  (*((volatile unsigned long *) REG_GPIO_19_CONFIG_ADDR))
#define REG_GPIO_20_CONFIG                  (*((volatile unsigned long *) REG_GPIO_20_CONFIG_ADDR))
#define REG_GPIO_21_CONFIG                  (*((volatile unsigned long *) REG_GPIO_21_CONFIG_ADDR))
#define REG_GPIO_22_CONFIG                  (*((volatile unsigned long *) REG_GPIO_22_CONFIG_ADDR))
#define REG_GPIO_23_CONFIG                  (*((volatile unsigned long *) REG_GPIO_23_CONFIG_ADDR))
#define REG_GPIO_24_CONFIG                  (*((volatile unsigned long *) REG_GPIO_24_CONFIG_ADDR))
#define REG_GPIO_25_CONFIG                  (*((volatile unsigned long *) REG_GPIO_25_CONFIG_ADDR))
#define REG_GPIO_26_CONFIG                  (*((volatile unsigned long *) REG_GPIO_26_CONFIG_ADDR))
#define REG_GPIO_27_CONFIG                  (*((volatile unsigned long *) REG_GPIO_27_CONFIG_ADDR))
#define REG_GPIO_28_CONFIG                  (*((volatile unsigned long *) REG_GPIO_28_CONFIG_ADDR))
#define REG_GPIO_29_CONFIG                  (*((volatile unsigned long *) REG_GPIO_29_CONFIG_ADDR))
#define REG_GPIO_30_CONFIG                  (*((volatile unsigned long *) REG_GPIO_30_CONFIG_ADDR))
#define REG_GPIO_31_CONFIG                  (*((volatile unsigned long *) REG_GPIO_31_CONFIG_ADDR))
#define REG_GPIO_32_CONFIG                  (*((volatile unsigned long *) REG_GPIO_32_CONFIG_ADDR))
#define REG_GPIO_33_CONFIG                  (*((volatile unsigned long *) REG_GPIO_33_CONFIG_ADDR))
#define REG_GPIO_34_CONFIG                  (*((volatile unsigned long *) REG_GPIO_34_CONFIG_ADDR))
#define REG_GPIO_35_CONFIG                  (*((volatile unsigned long *) REG_GPIO_35_CONFIG_ADDR))
#define REG_GPIO_36_CONFIG                  (*((volatile unsigned long *) REG_GPIO_36_CONFIG_ADDR))
#define REG_GPIO_37_CONFIG                  (*((volatile unsigned long *) REG_GPIO_37_CONFIG_ADDR))
#define REG_GPIO_38_CONFIG                  (*((volatile unsigned long *) REG_GPIO_38_CONFIG_ADDR))
#define REG_GPIO_39_CONFIG                  (*((volatile unsigned long *) REG_GPIO_39_CONFIG_ADDR))
#define REG_GPIO_40_CONFIG                  (*((volatile unsigned long *) REG_GPIO_40_CONFIG_ADDR))
#define REG_GPIO_41_CONFIG                  (*((volatile unsigned long *) REG_GPIO_41_CONFIG_ADDR))
#define REG_GPIO_42_CONFIG                  (*((volatile unsigned long *) REG_GPIO_42_CONFIG_ADDR))
#define REG_GPIO_43_CONFIG                  (*((volatile unsigned long *) REG_GPIO_43_CONFIG_ADDR))
#define REG_GPIO_44_CONFIG                  (*((volatile unsigned long *) REG_GPIO_44_CONFIG_ADDR))
#define REG_GPIO_45_CONFIG                  (*((volatile unsigned long *) REG_GPIO_45_CONFIG_ADDR))
#define REG_GPIO_46_CONFIG                  (*((volatile unsigned long *) REG_GPIO_46_CONFIG_ADDR))
#define REG_GPIO_47_CONFIG                  (*((volatile unsigned long *) REG_GPIO_47_CONFIG_ADDR))

#define GPIO_CFG_INPUT_DATA_POSI            0
#define GPIO_CFG_INPUT_DATA_MASK            (0x01UL << GPIO_CFG_INPUT_DATA_POSI)
#define GPIO_X_CFG_INPUT_DATA_GET(x)        (REG_GPIO_X_CONFIG(x) & GPIO_CFG_INPUT_DATA_MASK)

#define GPIO_CFG_OUTPUT_DATA_POSI           1
#define GPIO_CFG_OUTPUT_DATA_MASK           (0x01UL << GPIO_CFG_OUTPUT_DATA_POSI)
#define GPIO_X_CFG_OUTPUT_DATA_SET(x)       (REG_GPIO_X_CONFIG(x) |=  GPIO_CFG_OUTPUT_DATA_MASK)
#define GPIO_X_CFG_OUTPUT_DATA_CLEAR(x)     (REG_GPIO_X_CONFIG(x) &= (~GPIO_CFG_OUTPUT_DATA_MASK))

#define GPIO_CFG_INPUT_ENABLE_POSI          2
#define GPIO_CFG_INPUT_ENABLE_MASK          (0x01UL << GPIO_CFG_INPUT_ENABLE_POSI)
#define GPIO_CFG_INPUT_ENABLE_SET           (0x01UL << GPIO_CFG_INPUT_ENABLE_POSI)

#define GPIO_CFG_OUTPUT_ENABLE_POSI         3
#define GPIO_CFG_OUTPUT_ENABLE_MASK         (0x01UL << GPIO_CFG_OUTPUT_ENABLE_POSI)

#define GPIO_CFG_PULL_MODE_POSI             4
#define GPIO_CFG_PULL_MODE_MASK             (0x01UL << GPIO_CFG_PULL_MODE_POSI)
#define GPIO_CFG_PULL_MODE_PULL_DOWN        (0x00UL << GPIO_CFG_PULL_MODE_POSI)
#define GPIO_CFG_PULL_MODE_PULL_UP          (0x01UL << GPIO_CFG_PULL_MODE_POSI)

#define GPIO_CFG_PULL_ENABLE_POSI           5
#define GPIO_CFG_PULL_ENABLE_MASK           (0x01UL << GPIO_CFG_PULL_ENABLE_POSI)
#define GPIO_CFG_PULL_ENABLE_SET            (0x01UL << GPIO_CFG_PULL_ENABLE_POSI)

#define GPIO_CFG_FUNCTION_ENABLE_POSI       6
#define GPIO_CFG_FUNCTION_ENABLE_MASK       (0x01UL << GPIO_CFG_FUNCTION_ENABLE_POSI)
#define GPIO_CFG_FUNCTION_ENABLE_SET        (0x01UL << GPIO_CFG_FUNCTION_ENABLE_POSI)

#define GPIO_CFG_INPUT_MONITOR_POSI         7
#define GPIO_CFG_INPUT_MONITOR_MASK         (0x01UL << GPIO_CFG_INPUT_MONITOR_POSI)
#define GPIO_CFG_INPUT_MONITOR_SET          (0x01UL << GPIO_CFG_INPUT_MONITOR_POSI)


#define GPIO_UART0_RX_PIN                   10
#define GPIO_UART0_TX_PIN                   11

#define GPIO_UART0_CTS_PIN                  12
#define GPIO_UART0_RTS_PIN                  13

#define GPIO_UART0_RX_CONFIG                REG_GPIO_10_CONFIG
#define GPIO_UART0_TX_CONFIG                REG_GPIO_11_CONFIG

#define GPIO_UART0_CTS_CONFIG               REG_GPIO_12_CONFIG
#define GPIO_UART0_RTS_CONFIG               REG_GPIO_13_CONFIG

#define GPIO_UART1_TX_PIN                   0
#define GPIO_UART1_RX_PIN                   1

#define GPIO_UART1_TX_CONFIG                REG_GPIO_0_CONFIG
#define GPIO_UART1_RX_CONFIG                REG_GPIO_1_CONFIG

#define GPIO_UART2_TX_PIN                   40
#define GPIO_UART2_RX_PIN                   41
#define GPIO_UART2_TX_CONFIG                REG_GPIO_40_CONFIG
#define GPIO_UART2_RX_CONFIG                REG_GPIO_41_CONFIG


#define REG_ICU_BASE_ADDR                   (0x44010000UL)

// for GPIO 0-7
#define REG_GPIO_FUNC_CFG                   (REG_ICU_BASE_ADDR + 0x30 * 4)
#define GPIO_PCFG_POSI(x)                   (((x)-0)*4)
#define GPIO_PCFG_MASK(x)                   (0x07UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_1_FUNC(x)                 (0x00UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_2_FUNC(x)                 (0x01UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_3_FUNC(x)                 (0x02UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_4_FUNC(x)                 (0x03UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_5_FUNC(x)                 (0x04UL << GPIO_PCFG_POSI(x))
#define GPIO_PCFG_6_FUNC(x)                 (0x05UL << GPIO_PCFG_POSI(x))

// for GPIO 8-15
#define REG_GPIO_FUNC_CFG_2                  (REG_ICU_BASE_ADDR + 0x31 * 4)
#define GPIO_PCFG2_POSI(x)                   (((x)-8)*4)
#define GPIO_PCFG2_MASK(x)                   (0x07UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_1_FUNC(x)                 (0x00UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_2_FUNC(x)                 (0x01UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_3_FUNC(x)                 (0x02UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_4_FUNC(x)                 (0x03UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_5_FUNC(x)                 (0x04UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_6_FUNC(x)                 (0x05UL << GPIO_PCFG2_POSI(x))

// for GPIO 16-23
#define REG_GPIO_FUNC_CFG_3                  (REG_ICU_BASE_ADDR + 0x32 * 4)
#define GPIO_PCFG3_POSI(x)                   (((x)-16)*4)
#define GPIO_PCFG3_MASK(x)                   (0x07UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_1_FUNC(x)                 (0x00UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_2_FUNC(x)                 (0x01UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_3_FUNC(x)                 (0x02UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_4_FUNC(x)                 (0x03UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_5_FUNC(x)                 (0x04UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_6_FUNC(x)                 (0x05UL << GPIO_PCFG3_POSI(x))

// for GPIO 24-31
#define REG_GPIO_FUNC_CFG_4                  (REG_ICU_BASE_ADDR + 0x33 * 4)
#define GPIO_PCFG4_POSI(x)                   (((x)-24)*4)
#define GPIO_PCFG4_MASK(x)                   (0x07UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_1_FUNC(x)                 (0x00UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_2_FUNC(x)                 (0x01UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_3_FUNC(x)                 (0x02UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_4_FUNC(x)                 (0x03UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_5_FUNC(x)                 (0x04UL << GPIO_PCFG4_POSI(x))
#define GPIO_PCFG4_6_FUNC(x)                 (0x05UL << GPIO_PCFG4_POSI(x))

// for GPIO 32-39
#define REG_GPIO_FUNC_CFG_5                  (REG_ICU_BASE_ADDR + 0x34 * 4)
#define GPIO_PCFG5_POSI(x)                   (((x)-32)*4)
#define GPIO_PCFG5_MASK(x)                   (0x07UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_1_FUNC(x)                 (0x00UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_2_FUNC(x)                 (0x01UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_3_FUNC(x)                 (0x02UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_4_FUNC(x)                 (0x03UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_5_FUNC(x)                 (0x04UL << GPIO_PCFG5_POSI(x))
#define GPIO_PCFG5_6_FUNC(x)                 (0x05UL << GPIO_PCFG5_POSI(x))

// for GPIO 40-47
#define REG_GPIO_FUNC_CFG_6                  (REG_ICU_BASE_ADDR + 0x35 * 4)
#define GPIO_PCFG6_POSI(x)                   (((x)-40)*4)
#define GPIO_PCFG6_MASK(x)                   (0x07UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_1_FUNC(x)                 (0x00UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_2_FUNC(x)                 (0x01UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_3_FUNC(x)                 (0x02UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_4_FUNC(x)                 (0x03UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_5_FUNC(x)                 (0x04UL << GPIO_PCFG6_POSI(x))
#define GPIO_PCFG6_6_FUNC(x)                 (0x05UL << GPIO_PCFG6_POSI(x))

#define REG_GPIO_FUNTION_MODE               (*((volatile unsigned long *) REG_GPIO_FUNC_CFG))
#define REG_GPIO_FUNTION_MODE_2             (*((volatile unsigned long *) REG_GPIO_FUNC_CFG_2))
#define REG_GPIO_FUNTION_MODE_3             (*((volatile unsigned long *) REG_GPIO_FUNC_CFG_3))
#define REG_GPIO_FUNTION_MODE_4             (*((volatile unsigned long *) REG_GPIO_FUNC_CFG_4))
#define REG_GPIO_FUNTION_MODE_5             (*((volatile unsigned long *) REG_GPIO_FUNC_CFG_5))
#define REG_GPIO_FUNTION_MODE_6             (*((volatile unsigned long *) REG_GPIO_FUNC_CFG_6))

#define GPIO_2F_PWM0_PIN                    6
#define GPIO_2F_PWM1_PIN                    7
#define GPIO_2F_PWM2_PIN                    8
#define GPIO_2F_PWM3_PIN                    9
#define GPIO_2F_PWM4_PIN                    24
#define GPIO_2F_PWM5_PIN                    25
#define GPIO_2F_PWM0_CONFIG                 REG_GPIO_6_CONFIG
#define GPIO_2F_PWM1_CONFIG                 REG_GPIO_7_CONFIG
#define GPIO_2F_PWM2_CONFIG                 REG_GPIO_8_CONFIG
#define GPIO_2F_PWM3_CONFIG                 REG_GPIO_9_CONFIG
#define GPIO_2F_PWM4_CONFIG                 REG_GPIO_18_CONFIG
#define GPIO_2F_PWM5_CONFIG                 REG_GPIO_19_CONFIG


    /*************************************************************
     * GPIO_Int_Enable
     * Description: set GPIO int enable
     * Parameters:  ucChannel:  GPIO channel
     *              ucMode:     GPIO mode, 0: GPIO low voltage interrupt
     *                                     1: GPIO high voltage interrupt
     *                                     2: GPIO positive edge interrupt
     *                                     3: GPIO negative edge interrupt
     *              p_Int_Handler: int handler
     * return:      none
     * error:       none
     */
    extern void GPIO_Int_Enable(unsigned char ucChannel, unsigned char ucMode,
                                void (*p_Int_Handler)(unsigned char));
    /*************************************************************
     * GPIO_Int_Disable
     * Description: set GPIO int disable
     * Parameters:  ucChannel:  GPIO channel
     * return:      none
     * error:       none
     */
    extern void GPIO_Int_Disable(unsigned char ucChannel);
    /*************************************************************
     * GPIO_int_handler_clear
     * Description: clear GPIO int handler
     * Parameters:  ucChannel:  GPIO channel
     * return:      none
     * error:       none
     */
    extern void GPIO_int_handler_clear(unsigned char ucChannel);


    /*************************************************************
     * GPIO_InterruptHandler
     * Description: GPIO interrupt handler
     * Parameters:  none
     * return:      none
     * error:       none
     */
    extern void GPIO_InterruptHandler(void);


    /*************************************************************
     * GPIO_Set_Mode
     * Description: set GPIO mode
     * Parameters:  ucChannel:   GPIO channel
     *              ucDirection: GPIO direction, bit[0:1]: 0: input, 1: output
     *                                                     2/3: high-impedance state
     *              bPullEnable: GPIO pull enable, FALSE: disable, TRUE: enable
     *              bPullmode:   GPIO pull up/down, FALSE: pull down, TRUE: pull up
     * return:      none
     * error:       none
     */
    extern void GPIO_Set_Mode(unsigned char ucChannel, unsigned char ucDirection, bool bPullEnable, bool bPullmode);
    extern void GPIO_Output(unsigned char ucChannel, bool bOutputData);
    extern void GPIO_Output_Reverse(unsigned char ucChannel);
    extern unsigned char GPIO_Input(unsigned char ucChannel);


    extern void GPIO_UART_function_enable(unsigned char ucChannel);

    extern void GPIO_I2C0_function_enable(void);
    extern void GPIO_I2C_FM_function_enable(void);
    extern void GPIO_I2C1_function_enable(void);

    extern void GPIO_SPI_function_enable(unsigned char ucChannel);
    extern void GPIO_SPI_DMA_function_enable(unsigned char ucChannel);

    extern void GPIO_I2S_function_enable(unsigned char ucChannel);

    extern void GPIO_PWM_function_enable(unsigned char ucChannel);
    extern void GPIO_PWM2_function_enable(unsigned char ucChannel);

    extern void GPIO_ADC_function_enable(unsigned char ucChannel);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_GPIO_H__ */
