## Project：central

## Life Cycle：2023-06-16 ~~ 2023-12-06

## Special Macro Configuration Description：
CONFIG_MEDIA=y                // support media project
CONFIG_WIFI_TRANSFER=y        // support wifi transfer encode frame
CONFIG_IMAGE_STORAGE=y        // support capture frame and save to sdcard
CONFIG_INTEGRATION_DOORBELL=y // support ArminoMedia apk
CONFIG_LCD=y                  // support LCD Disply
CONFIG_LCD_GC9503V=y          // support next lcd type
CONFIG_LCD_H050IWV=y
CONFIG_LCD_HX8282=y
CONFIG_LCD_MD0430R=y
CONFIG_LCD_MD0700R=y
CONFIG_LCD_NT35512=y
CONFIG_LCD_NT35510=y
CONFIG_LCD_NT35510_MCU=y
CONFIG_LCD_ST7282=y
CONFIG_LCD_ST7796S=y
CONFIG_LCD_ST7710S=y
CONFIG_LCD_ST7701S=y
CONFIG_LCD_ST7701S_LY=y

## Complie Command:	
1、make bk7256 PROJECT=bluetooth/lcd_mcu_8080

## CPU: riscv

## RAM:
mem_type start      end        size    
-------- ---------- ---------- --------
itcm     0x10000000 0x100007c4 1988    
dtcm     0x20000400 0x20001de8 6632    
ram      0x3000c800 0x3001b0e0 59616   
data     0x3000c800 0x3000dcf0 5360    
bss      0x3000dcf0 0x3001b0e0 54256   
heap     0x38000000 0x38040000 262144  
psram    0x60700000 0x60800000 1048576 

## Bluetooth: BLE and BT
