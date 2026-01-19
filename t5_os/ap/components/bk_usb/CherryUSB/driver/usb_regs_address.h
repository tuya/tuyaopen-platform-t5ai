#include <common/bk_include.h>
#include "sdkconfig.h"

#define USB_BASE_ADDR           SOC_USB_REG_BASE

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#define REG_AHB2_USB_DEVICE_ID         (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x280)))
#define REG_AHB2_USB_VERSION_ID        (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x284)))
#define REG_AHB2_USB_GLOBAL_CTRL       (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x288)))
#define REG_AHB2_USB_DEVICE_STATUS     (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x28c)))
#define REG_AHB2_USB_OTG_CFG           (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x290)))
#define REG_AHB2_USB_DMA_ENDP          (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x294)))
#define REG_AHB2_USB_VTH               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x298)))
#define REG_AHB2_USB_GEN               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x29C)))
#define REG_AHB2_USB_STAT              (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x2A0)))
#define REG_AHB2_USB_INT               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x2A4)))
#define REG_AHB2_USB_RESET             (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x2A8)))
#define REG_AHB2_USB_DEV_CFG           (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x2AC)))

//REG_USB_USR_700
#define REG_USB_USR_DEVICE_ID          (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x700)))
//REG_USB_USR_704
#define REG_USB_USR_VERSION_ID         (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x704)))
//REG_USB_USR_708
#define REG_USB_USR_SOFT_RESETEN       (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x708)))
//REG_USB_USR_70C
#define REG_USB_USR_GLOBALSTATUS       (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x70C)))
//REG_USB_USR_710
#define REG_USB_USR_CONFIG             (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x710)))

#define REG_USB_PHY_00                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x400)))
#define REG_USB_PHY_01                 (*((volatile unsigned long *)   (USB_BASE_ADDR + 0x404)))

/* 708 Register Bit*/
#define R708_USB_USR_SOFT_RESETN        (0x1 << 0)
#define R708_USB_USR_BPS_CLKGATE        (0x1 << 1)

/* 70C Register Bit*/
#define R70C_USB_USR_CFG_DATAED         (0xFF)
#define R70C_USB_USR_TEST_BIST          (0x1 << 8)
#define R70C_USB_USR_DTO                (0x1 << 9)
#define R70C_USB_USR_RES_INT            (0x1 << 10)

/* 710 Register Bit 
 * bit[0]-bit[13] refer Naneng Phy Spec
 */
#define R710_USB_USR_TML                (0x1 << 0)
#define R710_USB_USR_CFG_RSTN           (0x1 << 1)
#define R710_USB_USR_TX_ENABLE_N        (0x1 << 2)
#define R710_USB_USR_TX_SE0             (0x1 << 3)
#define R710_USB_USR_TX_DAT             (0x1 << 4)
#define R710_USB_USR_REFCLK_MODE        (0x1 << 5)
#define R710_USB_USR_PLL_EN             (0x1 << 6)
#define R710_USB_USR_RESET              (0x1 << 7)
#define R710_USB_USR_SELF_TEST          (0x1 << 8)
#define R710_USB_USR_DATA_BUSL6_8       (0x1 << 9)
#define R710_USB_USR_OTG_SUSPENDM       (0x1 << 10)
#define R710_USB_USR_OTG_SUSPENDM_BYPS  (0x1 << 11)
#define R710_USB_USR_FSLS_SERIALMODE    (0x1 << 12)
#define R710_USB_USR_TXBITSTUFF_ENABLE  (0x1 << 13)
#define R710_USB_USR_ID_DIG_REG         (0x1 << 14)
#define R710_USB_USR_ID_DIG_SEL         (0x1 << 15)
#define R710_USB_USR_OTG_AVALID_REG     (0x1 << 16)
#define R710_USB_USR_OTG_AVALID_SEL     (0x1 << 17)
#define R710_USB_USR_OTG_VBUSVALID_REG  (0x1 << 18)
#define R710_USB_USR_OTG_VBUSVALID_SEL  (0x1 << 19)
#define R710_USB_USR_OTG_SESSEND_REG    (0x1 << 20)
#define R710_USB_USR_OTG_SESSEND_SEL    (0x1 << 21)
#endif

