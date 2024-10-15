ADC Example
=====================

Overview
--------------------

This is an example of SARADC. SARADC convert analog signal to digital.

It use defalut configuration to demonstrate the function of ADC.

If you make all-app.bin based on this project,it will execute an ADC sampling task according to the configuration set in adc_main.c


Hardware Requirements
-------------------------------

This project is able to run on any commonly available Beken development board.
You should add an analog signal,such as voltage,to GPIO25.
Then read the average ADC value from log. 



Build
***************************

Build the project and flash it to the board, then run monitor tool to view serial output.

Flash
***************************

See the Getting Started Guide:ref:`project_get-started:` for full steps to configure and use Armino to build projects.


Output
**********************************

::

  os:I(0):
  os:I(0):mem_type start    end      size
  os:I(0):-------- -------- -------- --------
  os:I(0):itcm     0x0      0x3c68   15464
  os:I(0):dtcm     0x20000000 0x20001908 6408
  os:I(0):ram      0x28000000 0x2807ebf8 519160
  os:I(0):non_heap 0x28000000 0x2800fca8 64680
  os:I(0):iram     0x28000000 0x28000814 2068
  os:I(0):data     0x28000814 0x280011b0 2460
  os:I(0):bss      0x280011c0 0x2800fca8 60136
  os:I(0):non_cache 0x2800fca8 0x2800fca8 0
  os:I(0):heap     0x2800fca8 0x2807ebf8 454480
  os:I(0):psram    0x60700000 0x60780000 524288
  flash:I(0):id=0xc86516
  flash:I(0):write status reg:38, status_reg_size:1
  ate enabled is 0
  driver_init end
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
  os:I(0):create main, tcb=28011898, stack=[28010870-28011870:4096], prio=5
  start user app thread.
  os:I(0):create app, tcb=28012940, stack=[28011918-28012918:4096], prio=2
  os:I(0):os time(0ms).
  os:I(0):base aon rtc time: 0:46
  os:I(0):create IDLE, tcb=20000330, stack=[2000038c-2000098c:1536], prio=0
  os:I(0):create Tmr Svc, tcb=2000098c, stack=[200009e8-20000fe8:1536], prio=5
  ==================> adc main
  os:I(2):create adc_task, tcb=28012e60, stack=[28012a38-28012e38:1024], prio=2
  os:I(4):create adc_task, tcb=28013308, stack=[28012ee0-280132e0:1024], prio=2
  adc_exam:I(36):adc task(587989505) sample(0) finished
  adc_exam:I(36):adc task(587989505) sample(0) finished
  adc_exam:I(46):adc task(1) sample value=106

  adc task(587989505) sample(0) finished
  adc_exam:I(3058):adc task(587989505) sample(0) finished
  adc_exam:I(3058):adc task(587989505) sample(0) finished
  adc_exam:I(3060):adc task(1) sample value=104

  adc task(587989505) sample(0) finished
  adc_exam:I(4064):adc task(587989505) sample(0) finished
  adc_exam:I(4064):adc task(587989505) sample(0) finished
  adc_exam:I(4064):adc task(1) sample value=104

