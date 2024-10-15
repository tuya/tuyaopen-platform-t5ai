## Project：av_client

## Life Cycle：2023-06-16 ~~ 2023-12-06

## Application：
	This demo is used as the client side of the dual board pairing. The client
connect AP provided by server, sends the voice data collected by the mic to the server peer,
and at the same time receives the voice data transmitted by the server peer and
sends it to the speaker for playback. In addition, the client collect the image
transmission data and send to the server.

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
1、make bk7258 PROJECT=media/av_client

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
1、psram range used for media: 0x60000000 - 0x606FFFFF
psram_based_addr: 0x60000000
according encode frame resolution to allocate psram block, please reference components/media/frame_buffer.c

2、psram work in 16M config, if need work 8M config, should adjust marco:
CONFIG_PSRAM_MEM_SLAB_USER_SIZE
CONFIG_PSRAM_MEM_SLAB_AUDIO_SIZE
CONFIG_PSRAM_MEM_SLAB_ENCODE_SIZE
CONFIG_PSRAM_MEM_SLAB_DISPLAY_SIZE

CONFIG_PSRAM_AS_SYS_MEMORY
CONFIG_PSRAM_HEAP_CPU0_BASE_ADDER
CONFIG_PSRAM_HEAP_BASE
CONFIG_PSRAM_HEAP_SIZE

## Switch TCP / UDP mode
CONFIG_AV_DEMO_MODE_TCP=y         //support TCP mode
CONFIG_AV_DEMO_MODE_TCP=n         //support UDP mode
