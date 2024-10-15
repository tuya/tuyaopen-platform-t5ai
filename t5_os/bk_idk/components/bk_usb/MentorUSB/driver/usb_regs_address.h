#include <common/bk_include.h>
#include "sdkconfig.h"

#define USB_BASE_ADDR           SOC_USB_REG_BASE

#if (CONFIG_SOC_BK7271)
#define REG_AHB2_USB_OTG_CFG              (*((volatile unsigned char *) (USB_BASE_ADDR + 0x80)))
#define REG_AHB2_USB_DMA_ENDP             (*((volatile unsigned char *) (USB_BASE_ADDR + 0x84)))
#define REG_AHB2_USB_VTH                  (*((volatile unsigned char *) (USB_BASE_ADDR + 0x88)))
#define REG_AHB2_USB_GEN                  (*((volatile unsigned char *) (USB_BASE_ADDR + 0x8C)))
#define REG_AHB2_USB_STAT                 (*((volatile unsigned char *) (USB_BASE_ADDR + 0x90)))
#define REG_AHB2_USB_INT                  (*((volatile unsigned char *) (USB_BASE_ADDR + 0x94)))
#define REG_AHB2_USB_RESET                (*((volatile unsigned char *) (USB_BASE_ADDR + 0x98)))
#define REG_AHB2_USB_DEV_CFG              (*((volatile unsigned char *) (USB_BASE_ADDR + 0x9C)))
#elif (CONFIG_SOC_BK7256XX)
#define REG_AHB2_USB_OTG_CFG           (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x280)))
#define REG_AHB2_USB_DMA_ENDP          (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x284)))
#define REG_AHB2_USB_VTH               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x288)))
#define REG_AHB2_USB_GEN               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x28C)))
#define REG_AHB2_USB_STAT              (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x290)))
#define REG_AHB2_USB_INT               (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x294)))
#define REG_AHB2_USB_RESET             (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x298)))
#define REG_AHB2_USB_DEV_CFG           (*((volatile unsigned char *)   (USB_BASE_ADDR + 0x29C)))
#elif (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
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
#endif

