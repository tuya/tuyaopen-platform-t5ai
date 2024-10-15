## Project：dvp

## Life Cycle：2023-06-16 ~~ 2023-12-06

## Application：peripheral dvp

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
1、make bk7256 PROJECT=peripheral/dvp

## CPU: riscv

## RAM:
mem_type start      end        size    
-------- ---------- ---------- --------
itcm     0x10000000 0x100007c4 1988    
dtcm     0x20000400 0x20001de8 6632    
ram      0x3000c800 0x3001b6c0 61120   
data     0x3000c800 0x3000e0a0 6304    
bss      0x3000e0a0 0x3001b6c0 54816   
heap     0x38000000 0x38040000 262144  
psram    0x60B00000 0x61000000 5242880 

## Media: video
1、psram range used for media: 0x60000000 - 0x60AFFFFF
psram_based_addr: 0x60000000
according encode frame resolution to allocate psram block, please reference components/media/frame_buffer.c

## Psram: config
注意：此工程默认是在16M的psram板子上使用
因为其cp0上psram的配置如下：
CONFIG_PSRAM_HEAP_BASE=0x60B00000
CONFIG_PSRAM_HEAP_SIZE=0x200000
CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60B00000

cp1上psram配置如下：
CONFIG_PSRAM_HEAP_BASE=0x60D00000
CONFIG_PSRAM_HEAP_SIZE=0x300000
CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60B00000

因为8M psram最大地址为0x60800000,明显上面是超过该地址的
16M psram的最大地址为0x61000000
从上可知：
psram在cp0上的heap起始地址为0x60B00000，heap大小为2Mbyte
在cp1上heap的起始地址为0x60B00000+2Mbyte=0x60D00000，大小为3Mbyte

如果要修改为8M psram上使用
cp0的配置：
CONFIG_PSRAM_HEAP_BASE=0x60700000
CONFIG_PSRAM_HEAP_SIZE=0x80000
CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60700000

cp1的配置：
CONFIG_PSRAM_HEAP_BASE=0x60780000
CONFIG_PSRAM_HEAP_SIZE=0x80000
CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER=0x60700000
