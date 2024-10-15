**Support Board: BK7236**

Timer
=======================================

Project Overview
--------------------------
General purpose timer groups TIMG0 and TIMG1 each contain three timers, each with a 32-bit up-counter. The three counters per timer group are driven by a 4-bit prescaler. 
General purpose timers can be used to trigger an interrupt after a set period or act as a hardware clock.

Each TIMG module has the following characteristics:

 - 3 timer (Timer0/1/2)
 - 3 32-bit up-counters
 - 4-bit prescaler used to divide the counter clock frequency by a factor between 1 and 16
 - The real-time value of the counter can be read

Hardware Requirements
----------------------------
NA

Configure and Build
--------------------------------

Configure the Project
+++++++++++++++++++++++++++++++

NA

Build
+++++++++++++++++++++++++++++++

Enter the following command in the Armino root directory to compile:

> make bk7236 PROJECT=examples/peripherals/timer


Build different APP version
*****************************************

If you want to run a secure upgrade, it is recommended to compile twice with different APP_VERSION, for example:

  - When compiling for the first time, APP_VERSION="v1.0", the generated all-app.bin is used for burning.
  - When compiling for the second time, APP_VERSION="v2.0", the generated ota.bin is used for upgrade.

You can determine the currently running version by checking the APP Version in the startup log. The version used can also be judged by printing time:

```
bk_init:I(0):armino app init: Jul 21 2023 10:12:47
bk_init:I(2):APP Version: v1.0
```

Connect UART
++++++++++++++++++++++++++++++++++

UART0 is used for:

 - BKFIL to download images

Flash all-app.bin
++++++++++++++++++++++++++++++++++

Burn Armino/build/secureboot/bk7236/all-app.bin to the board using BKFIL.

Running and Output
----------------------------------

Reset board

Case  - Timer
+++++++++++++++++++++++++++++++++++

log Output
***********************************

::

 os:I(0):mem_type start    end      size    
 os:I(0):-------- -------- -------- --------
 os:I(0):itcm     0x0      0x3c58   15448   
 os:I(0):dtcm     0x20000000 0x20001908 6408    
 os:I(0):ram      0x28000000 0x2807ebf8 519160  
 os:I(0):non_heap 0x28000000 0x2800fcb0 64688   
 os:I(0):iram     0x28000000 0x28000814 2068    
 os:I(0):data     0x28000814 0x280011b0 2460    
 os:I(0):bss      0x280011c0 0x2800fcb0 60144   
 os:I(0):non_cache 0x2800fcb0 0x2800fcb0 0       
 os:I(0):heap     0x2800fcb0 0x2807ebf8 454472  
 os:I(0):psram    0x60700000 0x60780000 524288  
 flash:I(0):id=0xc84016
 flash:I(0):write status reg:38, status_reg_size:1
 I(0):ate enabled is 1
 I(0):driver_init end
 init:I(0):reason - deep sleep gpio
 init:I(0):regs - 3, 3, 0
 init:I(0):armino rev: 
 init:I(0):armino soc id:53434647_72360001
 init:I(0):trng enable
 os:I(0):prvHeapInit-psram start addr:0x60700000, size:524288
 mailbox:I(0):mailbox_config
 mailbox:I(0):bk_mailbox_recv_callback_register
 mailbox:I(0):bk_mailbox_recv_callback_register
 init:I(0):Intialize random stack guard.
 os:I(0):create main, tcb=280118a0, stack=[28010878-28011878:4096], prio=5
 I(0):start user app thread.
 os:I(0):create app, tcb=28012948, stack=[28011920-28012920:4096], prio=2
 os:I(0):os time(0ms).
 os:I(0):base aon rtc time: 0:46
 os:I(0):create IDLE, tcb=20000330, stack=[2000038c-2000098c:1536], prio=0
 os:I(0):create Tmr Svc, tcb=2000098c, stack=[200009e8-20000fe8:1536], prio=5
 timer:E(0):timer0 is reserved for us timer!
 timer:E(2):timer0 is reserved for us timer!
 
 [11:05:29.498]收←◆timer_ex:I(1844):timer0(0) enter timer0_example_isr
 timer_ex:I(1844):current counter value:2844
 timer_ex:I(1844):period counter value:52000000
 timer_ex:I(1844):timer0-5 enable status:0x0
 timer_ex:I(1844):current counter value(0)
 timer_ex:I(1844):timer0-5 enable status:0x1
 timer_ex:I(1844):current counter value(101105)
 
 [11:05:31.506]收←◆timer_ex:I(3680):timer0(0) enter timer0_example_isr
 timer_ex:I(3680):current counter value:1795
 timer_ex:I(3680):period counter value:52000000
 timer_ex:I(3680):timer0-5 enable status:0x0
 timer_ex:I(3680):current counter value(0)
 timer_ex:I(3680):timer0-5 enable status:0x1
 timer_ex:I(3680):current counter value(101244)
 
 [11:05:32.524]收←◆timer_ex:I(4602):timer1(1) enter timer1_example_isr
 timer_ex:I(4602):current counter value:2132
 timer_ex:I(4602):period counter value:26000000
 timer_ex:I(4602):timer0-5 enable status:0x2


