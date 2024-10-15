**Support Board: BK7236**

UART
============================

Project Overview
--------------------------

This project is used for demonstrating [peripheral URAT](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/peripheral/bk_uart.html).
**The security function of BK7236 is still under development and improvement, please note the following restrictions**

UART (Universal Asynchronous Receiver and Transmitter), It is used to realize full duplex or half duplex data exchange between different devices.

Hardware Requirements
----------------------------

.. figure:: ../../../../../_static/uart.png
    :align: center
    :alt: UART Connection
    :figclass: align-center
	
    UART Connection

The figure above shows the UART serial port TTL level hardware connection, where:

  - TXD：For sending data, it should be connected to RXD pin of the receiving device

  - RXD：For receiving data, it should be connected to the TXD pin of the transmitting device

  - GND：Provide the same reference level for both parties

UART Config
---------------

In order to use UART for data transmission, the following parameters need to be configured：

  - Baud rate：The number of bits transmitted per second, generally 9600, 19200, 115200, etc. call function bk_uart_set_baud_rate()

  - Data bits：There can be 5, 6, 7 or 8 bits of data. Generally, we transmit data by byte (8 bits). The sender changes the status (high level or low level) of the data line bit by bit to send them. When transmitting data, the lowest bit is transmitted first, and the highest bit is transmitted last.call function bk_uart_set_data_bits()

  - Check bit：It is used to verify the correctness of data transmission. It can only detect errors, not correct them. call function bk_uart_set_parity()

  - Stop bit：It is used to indicate that the current data transmission is completed, and the logic level is “1”. call function bk_uart_set_stop_bits()

UART Application Scenario
+++++++++++++++++++++++++++++++

At present, UART supports three different application modes：
  - Use the default UART interrupt processing flow： The default ‘bk_uart_write_bytes()/bk_uart_read_bytes()’ is used to process UART receiving and sending. The receiving and sending channel is described in the UART receiving channel section. Generally, this method can meet the needs of most applications.

  - Use the default UART interrupt processing flow and register the user callback： The only difference from the first method is that the user’s registered callback will be called after the UART interrupt is generated.

  - UART interrupt processing is completely implemented by the application itself： When the default UART interrupt processing flow cannot meet the application requirements, such as the UART of the application is implemented by a third-party code, you can use ‘bk_interrupt_Register(xx, isr, arg)’ replaces the default UART interrupt handler. At this time, the receiving and sending process is completely implemented by the application.

Case Build
+++++++++++++++++++++++++++++++

Enter the following command in the Armino root directory to compile:

> make bk7236 PROJECT=examples/peripherals/uart

For preparations such as toolchain installation, please refer to [Quick Start](https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html).

Flash all-app.bin
++++++++++++++++++++++++++++++++++

Burn Armino/build/secureboot/bk7236/all-app.bin to the board using BKFIL.


Running and Output
----------------------------------


Case - uart log
+++++++++++++++++++++++++++++++++++


uart log
*************************************

::

 os:I(0):mem_type start    end      size    
 os:I(0):-------- -------- -------- --------
 os:I(0):itcm     0x0      0x3c58   15448   
 os:I(0):dtcm     0x20000000 0x20001908 6408    
 os:I(0):ram      0x28000000 0x2807ebf8 519160  
 os:I(0):non_heap 0x28000000 0x2800fca8 64680   
 os:I(0):iram     0x28000000 0x28000814 2068    
 os:I(0):data     0x28000814 0x280011b0 2460    
 os:I(0):bss      0x280011c0 0x2800fca8 60136   
 os:I(0):non_cache 0x2800fca8 0x2800fca8 0       
 os:I(0):heap     0x2800fca8 0x2807ebf8 454480  
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
 os:I(0):create main, tcb=28011898, stack=[28010870-28011870:4096], prio=5
 I(0):start user app thread.
 os:I(0):create app, tcb=28012940, stack=[28011918-28012918:4096], prio=2
 os:I(0):os time(0ms).
 os:I(0):base aon rtc time: 0:46
 os:I(0):create IDLE, t`b=20000330, stack=[2000038c-2000098c:1536], prio=0
 os:I(0):create Tmr Svc, tcb=2000098c, stack=[200009e8-20000fe8:1536], prio=5
 uart_exa:I(0):uart send data:
 uart_exa:I(2):data[0]=0x0
 uart_exa:I(4):data[1]=0x1
 uart_exa:I(6):data[2]=0x2
 uart_exa:I(8):data[3]=0x3
 uart_exa:I(10):data[4]=0x4
 uart_exa:I(12):data[5]=0x5
 uart_exa:I(14):data[6]=0x6
 uart_exa:I(16):data[7]=0x7
 uart_exa:I(18):data[8]=0x8
 uart_exa:I(20):data[9]=0x9
 uart_exa:I(22):data[10]=0xa
 uart_exa:I(24):data[11]=0xb
 uart_exa:I(26):data[12]=0xc
 uart_exa:I(28):data[13]=0xd
 uart_exa:I(30):data[14]=0xe
 uart_exa:I(32):data[15]=0xf
 uart_exa:I(34):data[16]=0x10
 uart_exa:I(36):data[17]=0x11
 uart_exa:I(38):data[18]=0x12
 uart_exa:I(40):data[19]=0x13
 uart_exa:I(42):data[20]=0x14
 uart_exa:I(44):data[21]=0x15
 uart_exa:I(46):data[22]=0x16
 uart_exa:I(48):data[23]=0x17
 uart_exa:I(50):data[24]=0x18
 uart_exa:I(52):data[25]=0x19
 uart_exa:I(54):data[26]=0x1a
 uart_exa:I(56):data[27]=0x1b
 uart_exa:I(58):data[28]=0x1c
 uart_exa:I(60):data[29]=0x1d
 uart_exa:I(62):data[30]=0x1e
 uart_exa:I(64):data[31]=0x1f
 uart_exa:I(66):data[32]=0x20
 uart_exa:I(68):data[33]=0x21
 uart_exa:I(70):data[34]=0x22
 uart_exa:I(72):data[35]=0x23
 uart_exa:I(74):data[36]=0x24
 uart_exa:I(76):data[37]=0x25
 uart_exa:I(78):data[38]=0x26
 uart_exa:I(80):data[39]=0x27
 uart_exa:I(82):data[40]=0x28
 uart_exa:I(84):data[41]=0x29
 uart_exa:I(86):data[42]=0x2a
 uart_exa:I(88):data[43]=0x2b
 uart_exa:I(90):data[44]=0x2c
 uart_exa:I(92):data[45]=0x2d
 uart_exa:I(94):data[46]=0x2e
 uart_exa:I(96):data[47]=0x2f
 uart_exa:I(98):data[48]=0x30
 uart_exa:I(100):data[49]=0x31
 uart_exa:I(102):data[50]=0x32
 uart_exa:I(104):data[51]=0x33
 uart_exa:I(106):data[52]=0x34
 uart_exa:I(108):data[53]=0x35
 uart_exa:I(110):data[54]=0x36
 uart_exa:I(112):data[55]=0x37
 uart_exa:I(114):data[56]=0x38
 uart_exa:I(116):data[57]=0x39
 uart_exa:I(118):data[58]=0x3a
 uart_exa:I(120):data[59]=0x3b
 uart_exa:I(122):data[60]=0x3c
 uart_exa:I(124):data[61]=0x3d
 uart_exa:I(126):data[62]=0x3e
 uart_exa:I(128):data[63]=0x3f
 uart_exa:I(130):data[64]=0x40
 uart_exa:I(132):data[65]=0x41
 uart_exa:I(134):data[66]=0x42
 uart_exa:I(136):data[67]=0x43
 uart_exa:I(138):data[68]=0x44
 uart_exa:I(140):data[69]=0x45
 uart_exa:I(142):data[70]=0x46
 uart_exa:I(144):data[71]=0x47
 uart_exa:I(146):data[72]=0x48
 uart_exa:I(148):data[73]=0x49
 uart_exa:I(150):data[74]=0x4a
 uart_exa:I(152):data[75]=0x4b
 uart_exa:I(154):data[76]=0x4c
 uart_exa:I(156):data[77]=0x4d
 uart_exa:I(158):data[78]=0x4e
 uart_exa:I(160):data[79]=0x4f
 uart_exa:I(162):data[80]=0x50
 uart_exa:I(164):data[81]=0x51
 uart_exa:I(166):data[82]=0x52
 uart_exa:I(168):data[83]=0x53
 uart_exa:I(170):data[84]=0x54
 uart_exa:I(172):data[85]=0x55
 uart_exa:I(176):data[86]=0x56
 uart_exa:I(178):data[87]=0x57
 uart_exa:I(180):data[88]=0x58
 uart_exa:I(182):data[89]=0x59
 uart_exa:I(184):data[90]=0x5a
 uart_exa:I(186):data[91]=0x5b
 uart_exa:I(188):data[92]=0x5c
 uart_exa:I(190):data[93]=0x5d
 uart_exa:I(192):data[94]=0x5e
 uart_exa:I(194):data[95]=0x5f
 uart_exa:I(196):data[96]=0x60
 uart_exa:I(198):data[97]=0x61
 uart_exa:I(200):data[98]=0x62
 uart_exa:I(202):data[99]=0x63
 uart_exa:I(204):data[100]=0x64
 uart_exa:I(206):data[101]=0x65
 uart_exa:I(208):data[102]=0x66
 uart_exa:I(210):data[103]=0x67
 uart_exa:I(212):data[104]=0x68
 uart_exa:I(214):data[105]=0x69
 uart_exa:I(216):data[106]=0x6a
 uart_exa:I(218):data[107]=0x6b
 uart_exa:I(220):data[108]=0x6c
 uart_exa:I(222):data[109]=0x6d
 uart_exa:I(224):data[110]=0x6e
 uart_exa:I(226):data[111]=0x6f
 uart_exa:I(228):data[112]=0x70
 uart_exa:I(230):data[113]=0x71
 uart_exa:I(232):data[114]=0x72
 uart_exa:I(234):data[115]=0x73
 uart_exa:I(236):data[116]=0x74
 uart_exa:I(238):data[117]=0x75
 uart_exa:I(240):data[118]=0x76
 uart_exa:I(242):data[119]=0x77
 uart_exa:I(244):data[120]=0x78
 uart_exa:I(246):data[121]=0x79
 uart_exa:I(248):data[122]=0x7a
 uart_exa:I(250):data[123]=0x7b
 uart_exa:I(252):data[124]=0x7c
 uart_exa:I(254):data[125]=0x7d
 uart_exa:I(256):data[126]=0x7e
 uart_exa:I(258):data[127]=0x7f
 \0	
  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~uart_exa:I(270):uart recv data:
 uart_exa:I(272):data[0]=0xf0
 I(274):ATE enabled = 1
