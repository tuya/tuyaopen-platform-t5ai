/*************************************************************
 * @file        driver_sys_ctrl.h
 * @brief       Header file of driver_sys_ctrl.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_SYS_CTRL_H__

#define __DRIVER_SYS_CTRL_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "BK_config.h"


#if (SOC_BK7256 != CFG_SOC_NAME)

#define REG_SYS_CTRL_BASE_ADDR              (0x00800000UL)


#define REG_SYS_CTRL_CHIP_ID_ADDR           (REG_SYS_CTRL_BASE_ADDR + 0x00 * 4)
#define REG_SYS_CTRL_CHIP_ID_MASK           0xFFFFFFFFUL
#define REG_SYS_CTRL_CHIP_ID                (*((volatile unsigned long *) REG_SYS_CTRL_CHIP_ID_ADDR))


#define REG_SYS_CTRL_DEVICE_ID_ADDR         (REG_SYS_CTRL_BASE_ADDR + 0x01 * 4)
#define REG_SYS_CTRL_DEVICE_ID_MASK         0xFFFFFFFFUL
#define REG_SYS_CTRL_DEVICE_ID              (*((volatile unsigned long *) REG_SYS_CTRL_DEVICE_ID_ADDR))


#define REG_SYS_CTRL_CLK_SET_ADDR           (REG_SYS_CTRL_BASE_ADDR + 0x02 * 4)
#define REG_SYS_CTRL_CLK_SET_MASK           0x00003FF3UL
#define REG_SYS_CTRL_CLK_SET                (*((volatile unsigned long *) REG_SYS_CTRL_CLK_SET_ADDR))

#define SYS_CTRL_CLK_SET_MCLK_POSI          0
#define SYS_CTRL_CLK_SET_MCLK_MASK          (0x03UL << SYS_CTRL_CLK_SET_MCLK_POSI)
#define SYS_CTRL_CLK_SET_MCLK_DCO           (0x00UL << SYS_CTRL_CLK_SET_MCLK_POSI)
#define SYS_CTRL_CLK_SET_MCLK_XTAL_26M      (0x01UL << SYS_CTRL_CLK_SET_MCLK_POSI)
#define SYS_CTRL_CLK_SET_MCLK_DPLL          (0x02UL << SYS_CTRL_CLK_SET_MCLK_POSI)
#define SYS_CTRL_CLK_SET_MCLK_LPO           (0x03UL << SYS_CTRL_CLK_SET_MCLK_POSI)

#define SYS_CTRL_CLK_SET_MCLK_DIV_POSI      4
#define SYS_CTRL_CLK_SET_MCLK_DIV_MASK      (0x0FUL << SYS_CTRL_CLK_SET_MCLK_DIV_POSI)

#define SYS_CTRL_CLK_SET_FLASH_26M_POSI     8
#define SYS_CTRL_CLK_SET_FLASH_26M_MASK     (0x01UL << SYS_CTRL_CLK_SET_FLASH_26M_POSI)

#define SYS_CTRL_CLK_SET_AHB_CLK_DIV_POSI   9
#define SYS_CTRL_CLK_SET_AHB_CLK_DIV_MASK   (0x01UL << SYS_CTRL_CLK_SET_AHB_CLK_DIV_POSI)

#define SYS_CTRL_CLK_SET_MODEM_PWD_POSI     10
#define SYS_CTRL_CLK_SET_MODEM_PWD_MASK     (0x01UL << SYS_CTRL_CLK_SET_MODEM_PWD_POSI)

#define SYS_CTRL_CLK_SET_MAC_PWD_POSI       11
#define SYS_CTRL_CLK_SET_MAC_PWD_MASK       (0x01UL << SYS_CTRL_CLK_SET_MAC_PWD_POSI)

#define SYS_CTRL_CLK_SET_MPIF_CLK_POSI      12
#define SYS_CTRL_CLK_SET_MPIF_CLK_MASK      (0x01UL << SYS_CTRL_CLK_SET_MPIF_CLK_POSI)

#define SYS_CTRL_CLK_SET_SDIO_CLK_POSI      13
#define SYS_CTRL_CLK_SET_SDIO_CLK_MASK      (0x01UL << SYS_CTRL_CLK_SET_SDIO_CLK_POSI)


#define REG_SYS_CTRL_RESET_ADDR             (REG_SYS_CTRL_BASE_ADDR + 0x04 * 4)
#define REG_SYS_CTRL_RESET_MASK             0x000000FFUL
#define REG_SYS_CTRL_RESET                  (*((volatile unsigned long *) REG_SYS_CTRL_RESET_ADDR))

#define SYS_CTRL_MODEM_SUBCHIP_RESET_POSI   0
#define SYS_CTRL_MODEM_SUBCHIP_RESET_MASK   (0x01UL << SYS_CTRL_MODEM_SUBCHIP_RESET_POSI)

#define SYS_CTRL_DSP_SUBSYS_RESET_POSI      1
#define SYS_CTRL_DSP_SUBSYS_RESET_MASK      (0x01UL << SYS_CTRL_DSP_SUBSYS_RESET_POSI)

#define SYS_CTRL_MAC_SUBSYS_RESET_POSI      2
#define SYS_CTRL_MAC_SUBSYS_RESET_MASK      (0x01UL << SYS_CTRL_MAC_SUBSYS_RESET_POSI)

#define SYS_CTRL_TL410_BOOT_POSI            3
#define SYS_CTRL_TL410_BOOT_MASK            (0x01UL << SYS_CTRL_TL410_BOOT_POSI)
#define SYS_CTRL_TL410_BOOT_CLEAR           (0x00UL << SYS_CTRL_TL410_BOOT_POSI)
#define SYS_CTRL_TL410_BOOT_SET             (0x01UL << SYS_CTRL_TL410_BOOT_POSI)

#define SYS_CTRL_USB_SUBSYS_RESET_POSI      4
#define SYS_CTRL_USB_SUBSYS_RESET_MASK      (0x01UL << SYS_CTRL_USB_SUBSYS_RESET_POSI)

#define SYS_CTRL_TL410_EXT_WAIT_POSI        5
#define SYS_CTRL_TL410_EXT_WAIT_MASK        (0x01UL << SYS_CTRL_TL410_EXT_WAIT_POSI)
#define SYS_CTRL_TL410_EXT_WAIT_CLEAR       (0x00UL << SYS_CTRL_TL410_EXT_WAIT_POSI)
#define SYS_CTRL_TL410_EXT_WAIT_SET         (0x01UL << SYS_CTRL_TL410_EXT_WAIT_POSI)

#define SYS_CTRL_MODEM_CORE_RESET_POSI      6
#define SYS_CTRL_MODEM_CORE_RESET_MASK      (0x01UL << SYS_CTRL_MODEM_CORE_RESET_POSI)

#define SYS_CTRL_MAC_WAKEUP_ARM_POSI        7
#define SYS_CTRL_MAC_WAKEUP_ARM_MASK        (0x01UL << SYS_CTRL_MAC_WAKEUP_ARM_POSI)
#define SYS_CTRL_MAC_WAKEUP_ARM_CLEAR       (0x00UL << SYS_CTRL_MAC_WAKEUP_ARM_POSI)
#define SYS_CTRL_MAC_WAKEUP_ARM_SET         (0x01UL << SYS_CTRL_MAC_WAKEUP_ARM_POSI)


#define REG_SYS_CTRL_DSP_SUBSYS_RESET_REQ_ADDR  (REG_SYS_CTRL_BASE_ADDR + 0x0B * 4)
#define REG_SYS_CTRL_DSP_SUBSYS_RESET_REQ_MASK  0xFFFFFFFFUL
#define REG_SYS_CTRL_DSP_SUBSYS_RESET_REQ       (*((volatile unsigned long *) REG_SYS_CTRL_DSP_SUBSYS_RESET_REQ_ADDR))

#define DSP_SUBSYS_RESET_REQ_WORD_POSI      0
#define DSP_SUBSYS_RESET_REQ_WORD_MASK      (0xFFFFFFFFUL << DSP_SUBSYS_RESET_REQ_WORD_POSI)
#define DSP_SUBSYS_RESET_REQ_WORD           0x7111C410UL


#define REG_SYS_CTRL_ANALOG_REG0_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x16 * 4)
#define REG_SYS_CTRL_ANALOG_REG0_MASK       0xFFFFFFFFUL
#define REG_SYS_CTRL_ANALOG_REG0            (*((volatile unsigned long *) REG_SYS_CTRL_ANALOG_REG0_ADDR))


#define REG_SYS_CTRL_ANALOG_REG1_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x17 * 4)
#define REG_SYS_CTRL_ANALOG_REG1_MASK       0xFFFFFFFFUL
#define REG_SYS_CTRL_ANALOG_REG1            (*((volatile unsigned long *) REG_SYS_CTRL_ANALOG_REG1_ADDR))
#define SPI_TRIG_BIT                             (1 << 19)
#define SPI_DET_EN                               (1 << 4)

#define REG_SYS_CTRL_ANALOG_REG2_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x18 * 4)
#define REG_SYS_CTRL_ANALOG_REG2_MASK       0xFFFFFFFFUL
#define REG_SYS_CTRL_ANALOG_REG2            (*((volatile unsigned long *) REG_SYS_CTRL_ANALOG_REG2_ADDR))


#define REG_SYS_CTRL_ANALOG_REG3_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x19 * 4)
#define REG_SYS_CTRL_ANALOG_REG3_MASK       0xFFFFFFFFUL
#define REG_SYS_CTRL_ANALOG_REG3            (*((volatile unsigned long *) REG_SYS_CTRL_ANALOG_REG3_ADDR))


#define REG_SYS_CTRL_ANALOG_REG4_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x1A * 4)
#define REG_SYS_CTRL_ANALOG_REG4_MASK       0xFFFFFFFFUL
#define REG_SYS_CTRL_ANALOG_REG4            (*((volatile unsigned long *) REG_SYS_CTRL_ANALOG_REG4_ADDR))


#define REG_SYS_CTRL_LPO_CLK_ADDR           (REG_SYS_CTRL_BASE_ADDR + 0x40 * 4)
#define REG_SYS_CTRL_LPO_CLK_MASK           0x03UL
#define REG_SYS_CTRL_LPO_CLK                (*((volatile unsigned long *) REG_SYS_CTRL_LPO_CLK_ADDR))

#define SYS_CTRL_LPO_CLK_MUX_POSI           0
#define SYS_CTRL_LPO_CLK_MUX_MASK           (0x03UL << SYS_CTRL_LPO_CLK_MUX_POSI)
#define SYS_CTRL_LPO_CLK_MUX_ROSC_32KHz     (0x00UL << SYS_CTRL_LPO_CLK_MUX_POSI)
#define SYS_CTRL_LPO_CLK_MUX_XTAL_32KHz     (0x01UL << SYS_CTRL_LPO_CLK_MUX_POSI)
#define SYS_CTRL_LPO_CLK_MUX_DIV_32KHz      (0x02UL << SYS_CTRL_LPO_CLK_MUX_POSI)


#define REG_SYS_CTRL_SLEEP_ADDR             (REG_SYS_CTRL_BASE_ADDR + 0x41 * 4)
#define REG_SYS_CTRL_SLEEP_MASK             0x00F7FFFFUL
#define REG_SYS_CTRL_SLEEP                  (*((volatile unsigned long *) REG_SYS_CTRL_SLEEP_ADDR))

#define SYS_CTRL_SLEEP_SLEEP_MODE_POSI      0
#define SYS_CTRL_SLEEP_SLEEP_MODE_MASK      (0x0000FFFF << SYS_CTRL_SLEEP_SLEEP_MODE_POSI)
#define SYS_CTRL_SLEEP_SLEEP_MODE_NORMAL    (0x00004F89 << SYS_CTRL_SLEEP_SLEEP_MODE_POSI)
#define SYS_CTRL_SLEEP_SLEEP_MODE_LOW       (0x0000B706 << SYS_CTRL_SLEEP_SLEEP_MODE_POSI)
#define SYS_CTRL_SLEEP_SLEEP_MODE_DEEPSLEEP (0x0000ADC1 << SYS_CTRL_SLEEP_SLEEP_MODE_POSI)

#define SYS_CTRL_SLEEP_ROSC_PWD_POSI        16
#define SYS_CTRL_SLEEP_ROSC_PWD_MASK        (0x01UL << SYS_CTRL_SLEEP_ROSC_PWD_POSI)
#define SYS_CTRL_SLEEP_ROSC_PWD_SET         (0x01UL << SYS_CTRL_SLEEP_ROSC_PWD_POSI)

#define SYS_CTRL_SLEEP_FLASH_PWD_POSI       17
#define SYS_CTRL_SLEEP_FLASH_PWD_MASK       (0x01UL << SYS_CTRL_SLEEP_FLASH_PWD_POSI)
#define SYS_CTRL_SLEEP_FLASH_PWD_SET        (0x01UL << SYS_CTRL_SLEEP_FLASH_PWD_POSI)

#define SYS_CTRL_SLEEP_DCO_PWD_POSI         18
#define SYS_CTRL_SLEEP_DCO_PWD_MASK         (0x01UL << SYS_CTRL_SLEEP_DCO_PWD_POSI)
#define SYS_CTRL_SLEEP_DCO_PWD_SET          (0x01UL << SYS_CTRL_SLEEP_DCO_PWD_POSI)

#define SCTRL_BASE                            (0x00800000)
#define SCTRL_SLEEP                           (SCTRL_BASE + 65 * 4)
#define GPIO_SLEEP_SWITCH_BIT                    (1 << 19)
#define SCTRL_GPIO_WAKEUP_INT_STATUS          (SCTRL_BASE + 74 * 4)
#define SCTRL_GPIO_WAKEUP_INT_STATUS1          (SCTRL_BASE + 83 * 4)

#define SYS_CTRL_SLEEP_POR_CORE_DLY_POSI    20
#define SYS_CTRL_SLEEP_POR_CORE_DLY_MASK    (0x0FUL << SYS_CTRL_SLEEP_POR_CORE_DLY_POSI)
#define SYS_CTRL_SLEEP_POR_CORE_DLY_SET     (0x0FUL << SYS_CTRL_SLEEP_POR_CORE_DLY_POSI)


#define REG_WIFI_PWD_ADDR                   (REG_SYS_CTRL_BASE_ADDR + 0x43 * 4)
#define REG_DSP_PWD_ADDR                    (REG_SYS_CTRL_BASE_ADDR + 0x44 * 4)
#define REG_USB_PWD_ADDR                    (REG_SYS_CTRL_BASE_ADDR + 0x45 * 4)

#define REG_WIFI_PWD                        (*((volatile unsigned long *) REG_WIFI_PWD_ADDR))
#define REG_DSP_PWD                         (*((volatile unsigned long *) REG_DSP_PWD_ADDR))
#define REG_USB_PWD                         (*((volatile unsigned long *) REG_USB_PWD_ADDR))

#if (SOC_BK7271 == CFG_SOC_NAME)
#define REG_SYS_CTRL_BLOCK_EN_ADDR          SCTRL_BLOCK_EN_CFG
#else
#define REG_SYS_CTRL_BLOCK_EN_ADDR          (REG_SYS_CTRL_BASE_ADDR + 0x4B * 4)
#endif
#define REG_SYS_CTRL_BLOCK_EN_MASK          0xFFFFBFBFUL
#define REG_SYS_CTRL_BLOCK_EN               (*((volatile unsigned long *) REG_SYS_CTRL_BLOCK_EN_ADDR))

#define SYS_CTRL_BLOCK_EN_SW_POSI           0
#define SYS_CTRL_BLOCK_EN_SW_MASK           (0x000FBFBF << SYS_CTRL_BLOCK_EN_SW_POSI)
#define SYS_CTRL_BLOCK_EN_SW_FLASH          0x00000001
#define SYS_CTRL_BLOCK_EN_SW_DCO_26MHz      0x00000002
#define SYS_CTRL_BLOCK_EN_SW_ROSC_32KHz     0x00000004
#define SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz     0x00000008
#define SYS_CTRL_BLOCK_EN_SW_XTAL_32KHz     0x00000010
#define SYS_CTRL_BLOCK_EN_SW_DPLL           0x00000020
#define SYS_CTRL_BLOCK_EN_SW_DIGITAL_LDO    0x00000080
#define SYS_CTRL_BLOCK_EN_SW_ANALOG_LDO     0x00000100
#define SYS_CTRL_BLOCK_EN_SW_IO_LDO         0x00000200
#define SYS_CTRL_BLOCK_EN_SW_XTAL_TO_RF     0x00000400
#define SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz_LOW_POWER   0x00000800
#define SYS_CTRL_BLOCK_EN_SW_TEMP_SENSOR    0x00001000
#define SYS_CTRL_BLOCK_EN_SW_ADC_ENABLE     0x00002000
#define SYS_CTRL_BLOCK_EN_SW_AUDIO_L_CHNNEL 0x00008000
#define SYS_CTRL_BLOCK_EN_SW_AUDIO_R_CHNNEL 0x00010000
#define SYS_CTRL_BLOCK_EN_SW_MIC_L_CHNNEL   0x00020000
#define SYS_CTRL_BLOCK_EN_SW_MIC_R_CHNNEL   0x00040000
#define SYS_CTRL_BLOCK_EN_SW_LINE_IN        0x00080000

#define SYS_CTRL_BLOCK_EN_VALID_POSI        20
#define SYS_CTRL_BLOCK_EN_VALID_MASK        (0x0FFFUL << SYS_CTRL_BLOCK_EN_VALID_POSI)
#define SYS_CTRL_BLOCK_EN_VALID_SET         (0x0A5CUL << SYS_CTRL_BLOCK_EN_VALID_POSI)


#define REG_SYS_CTRL_BIAS_CALIB_ADDR        (REG_SYS_CTRL_BASE_ADDR + 0x4C * 4)
#define REG_SYS_CTRL_BIAS_CALIB_MASK        0x001F1F11UL
#define REG_SYS_CTRL_BIAS_CALIB             (*((volatile unsigned long *) REG_SYS_CTRL_BIAS_CALIB_ADDR))

#define SYS_CTRL_BIAS_CALIB_TRIG_POSI       0
#define SYS_CTRL_BIAS_CALIB_TRIG_MASK       (0x01UL << SYS_CTRL_BIAS_CALIB_TRIG_POSI)
#define SYS_CTRL_BIAS_CALIB_TRIG_SET        (0x01UL << SYS_CTRL_BIAS_CALIB_TRIG_POSI)

#define SYS_CTRL_BIAS_CALIB_MANUAL_POSI     4
#define SYS_CTRL_BIAS_CALIB_MANUAL_MASK     (0x01UL << SYS_CTRL_BIAS_CALIB_MANUAL_POSI)
#define SYS_CTRL_BIAS_CALIB_MANUAL_SET      (0x01UL << SYS_CTRL_BIAS_CALIB_MANUAL_POSI)

#define SYS_CTRL_BIAS_CALIB_SETTING_POSI    8
#define SYS_CTRL_BIAS_CALIB_SETTING_MASK    (0x1FUL << SYS_CTRL_BIAS_CALIB_SETTING_POSI)

#define SYS_CTRL_BIAS_CALIB_RESULT_POSI     16
#define SYS_CTRL_BIAS_CALIB_RESULT_MASK     (0x1FUL << SYS_CTRL_BIAS_CALIB_RESULT_POSI)


#define REG_SYS_CTRL_ROSC_CONFIG_ADDR       (REG_SYS_CTRL_BASE_ADDR + 0x4D * 4)
#define REG_SYS_CTRL_ROSC_CONFIG_MASK       0x7FFF0077UL
#define REG_SYS_CTRL_ROSC_CONFIG            (*((volatile unsigned long *) REG_SYS_CTRL_ROSC_CONFIG_ADDR))

#if (SOC_BK7271 == CFG_SOC_NAME)
#define SCTRL_BASE                            (0x00800000)

#define MAC_HCLK_EN_BIT                          (1 << 27)
#define PHY_HCLK_EN_BIT                          (1 << 26)
#define MTB_PRIVILEGE_POSI                          (24)
#define MTB_PRIVILEGE_MASK                          (0x3)
#define MTB_PRIVILEGE_ACCESS_NONE                   (0x0)
#define MTB_PRIVILEGE_ACCESS_AHB_PART               (0x1)
#define MTB_PRIVILEGE_ACCESS_AHB                    (0x2)
#define MTB_PRIVILEGE_ACCESS_AHB_DTCM               (0x3)

#define FLASH_SPI_MUX_BIT                        (1 << 22)

#define HCLK_DIV2_POSI                           (2)
#define HCLK_DIV2_MASK                           (0x3)
#define HCLK_DIV2_EN_BIT                         (1 << 2)

#define SCTRL_RESET_MASK                         (0x1FFFFF)
#define MAC_SUBSYS_RESET_BIT                     (1 << 20)
#define MODEM_CORE_RESET_BIT                     (1 << 19)
#define MODEM_SUBCHIP_RESET_BIT                  (1 << 16)

#define MODEM_SUBCHIP_RESET_WORD                 (0x7171e802)

#define SCTRL_MAC_SUBSYS_RESET_REQ            (SCTRL_BASE + 12 * 4)
#define MAC_SUBSYS_RESET_POSI                    (0)
#define MAC_SUBSYS_RESET_MASK                    (0xFFFF)
#define MAC_SUBSYS_RESET_WORD                    (0xE802U)

#define ANA_SPI_STATE_POSI                       (0)
#define ANA_SPI_STAET_MASK                       (0x7F)

#define SCTRL_ANALOG_CTRL8                    (SCTRL_BASE + 0x20*4) //ana_regA actually
#define LINE_IN_EN                               (1 << 21)
#define LINE2_IN_EN                              (1 << 20)
#define LINE_IN_GAIN_MASK                        (0x3)
#define LINE_IN_GAIN_POSI                        (22)
#define AUD_DAC_GAIN_MASK                        (0x1F)
#define AUD_DAC_GAIN_POSI                        (2)
#define AUD_DAC_MUTE_EN                          (1 << 0)

#define SCTRL_ANALOG_CTRL9                    (SCTRL_BASE + 0x21*4) //ana_regB actually
#define DAC_DIFF_EN                              (1 << 31)
#define EN_AUD_DAC_L                             (1 << 30)
#define EN_AUD_DAC_R                             (1 << 29)
#define DAC_PA_OUTPUT_EN                         (1 << 24)
#define DAC_DRIVER_OUTPUT_EN                     (1 << 23)
#define AUD_DAC_DGA_EN                           (1 << 6)

#define SCTRL_ANALOG_CTRL10                   (SCTRL_BASE + 0x22*4) //ana_regC actually
#define DAC_N_END_OUPT_L                         (1 << 8)
#define DAC_N_END_OUPT_R                         (1 << 7)
#define DAC_VSEL_MASK                            (0x3)
#define DAC_VSEL_POSI                            (1)

#define SCTRL_ANALOG_CTRL13                   (SCTRL_BASE + 0x23*4) //ana_regD for audio
#define AUDIO_DCO_EN                             (1 << 10)
#define MIC1_PWR_DOWN                            (1 << 29)
#define MIC2_PWR_DOWN                            (1 << 24)
#define MIC3_PWR_DOWN                            (1 << 19)
#define MIC4_PWR_DOWN                            (1 << 14)

#define SCTRL_ANALOG_AUDIO_PLL_SDM            (SCTRL_BASE + 0x24*4) //ana_regE for audio

#define SCTRL_ANALOG_AUDIO_PLL_CTRL           (SCTRL_BASE + 0x25*4) //ana_regF for audio
#define AUDIO_PLL_AUDIO_EN                       (1 << 31)
#define AUDIO_PLL_SPI_TRIGGER                    (1 << 18)
#define AUDIO_PLL_RESET                          (1 << 3)


#define SCTRL_ANALOG_CTRL0                      REG_SYS_CTRL_ANALOG_REG0_ADDR
#define SCTRL_ANALOG_CTRL1                      REG_SYS_CTRL_ANALOG_REG1_ADDR
#define SCTRL_ANALOG_CTRL2                      REG_SYS_CTRL_ANALOG_REG2_ADDR
#define SCTRL_ANALOG_CTRL3                      REG_SYS_CTRL_ANALOG_REG3_ADDR
#define SCTRL_ANALOG_CTRL4                      REG_SYS_CTRL_ANALOG_REG4_ADDR
#define SCTRL_ANALOG_CTRL5                      REG_SYS_CTRL_ANALOG_REG5_ADDR

#define SCTRL_ROSC_CAL                        SCTRL_ANALOG_CTRL2
#define ROSC_CAL_MANU_CIN_POSI                   (1)
#define ROSC_CAL_MANU_CIN_MASK                   (0x3F)
#define ROSC_CAL_MANU_EN_BIT                     (1 << 0)
#define ROSC_CAL_INTVAL_POSI                     (22)
#define ROSC_CAL_INTVAL_MASK                     (0x3FF)
#define ROSC_CAL_MODE_BIT                        (1 << 10)
#define ROSC_CAL_TRIG_BIT                        (1 << 11)
#define ROSC_CAL_EN_BIT                          (1 << 9) //confirmed by huaming

#define SCTRL_BIAS                            SCTRL_ANALOG_CTRL3
#define ROSC_CAL_MANU_FIN_POSI                   (23)
#define ROSC_CAL_MANU_FIN_MASK                   (0x1FF)
#define LDO_VAL_MANUAL_POSI                      (16)
#define LDO_VAL_MANUAL_MASK                      (0x1F)
#define BIAS_CAL_MANUAL_BIT                      (1 << 22)
#define BIAS_CAL_TRIGGER_BIT                     (1 << 21)

#define REG_GPIO_EXTRAL_INT_CFG              (SCTRL_BASE + 42*4)
#define DPLL_UNLOCK_INT_EN                    (1 << 0)
#define AUDIO_DPLL_UNLOCK_INT_EN              (1 << 1)

#define REG_GPIO_EXTRAL_INT_CFG1              (SCTRL_BASE + 43*4)
#define DPLL_UNLOCK_INT                       (1 << 0)
#define AUDIO_DPLL_UNLOCK_INT                 (1 << 1)
#define USB_PLUG_IN_INT_EN                    (1 << 4)
#define USB_PLUG_OUT_INT_EN                   (1 << 5)
#define USB_PLUG_IN_INT                       (1 << 6)
#define USB_PLUG_OUT_INT                      (1 << 7)
#define GPIO_EXTRAL_INT_MASK                  (DPLL_UNLOCK_INT | AUDIO_DPLL_UNLOCK_INT | USB_PLUG_IN_INT | USB_PLUG_OUT_INT)



#define BASEADDR_SYS_CTRL                                       0x00800000
#define addSYS_CTRL_Reg0x2                                      *((volatile unsigned long *) (0x00800000+0x2*4))
#define addSYS_CTRL_Reg0x10                                     *((volatile unsigned long *) (0x00800000+0x10*4))
#define addICU_Reg0x12                                          *((volatile unsigned long *) (0x00802000+0x12*4))
#define addPMU_Reg0x2                                           *((volatile unsigned long *) (0x00800200+0x2*4))
#define clrf_SYS_CTRL_Reg0x2_flash_26m_mux                      addSYS_CTRL_Reg0x2 &= ~0x100
#define addPMU_Reg0xe                                           *((volatile unsigned long *) (0x00800200+0xe*4))
#define addGPIO_Reg0xf                                          *((volatile unsigned long *) (0x00800300+0xf*4))
#define addPMU_Reg0xd                                           *((volatile unsigned long *) (0x00800200+0xd*4))
#define addPMU_Reg0xf                                           *((volatile unsigned long *) (0x00800200+0xf*4))
#define bitPMU_Reg0xf_rtc_wku_en                                0x2
#define bitPMU_Reg0xf_rtc_wku_int                               0x100

#endif

extern unsigned long get_chip_id(void);
extern unsigned long get_device_id(void);
extern void sys_ctrl_save_deep_gpio_wake_status(void);

#endif  //bk7256

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_SYS_CTRL_H__ */
