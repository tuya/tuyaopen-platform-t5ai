**Support Board: BK7236**
 
I2c
===============================

Project Overview
-------------------------------
This project is used to demonstrate the functions of the peripheral module i2c

Hardware Requirements
-------------------------------
BK7236 development board

Configure and Build
-------------------------------

Configure
***************************

Build
***************************
Execute the following command in the Armino root directory to compile:make bk7236

Connect UART
***************************
Uart0 is used for Logging and CLI

Flash
***************************
Burn all-app.bin to the board using BKFIL.

Running and Output
-------------------------------
i2c0 communication as follows:
 
  - The BK7236 development board is used as the master device, and the eeprom is used as the slave device  
  - The clock interface swclk of BK7236 i2c module is connected with the clock interface sclk_e2 of eeprom i2c module  
  - The data interface swdio of BK7236 i2c module is connected with the data interface sdio_e2 of eeprom i2c module  
  - Block block BK7236 development board and eeprom, total eeprom external voltage of 3.3v  
  - The SWCLK BK7236 and swdio on 2k resistors, respectively

The cmd example of i2c0 as follows:
  
  - i2c_driver init  
  - i2c 0 init  
  - i2c 0 memory_write 10 0x50  
  - i2c 0 memory_read 10

i2c1 communication as follows:

  - The BK7236 development board is used as the master device, and the eeprom is used as the slave device  
  - The clock interface TXD of BK7236 i2c module is connected with the clock interface sclk_e2 of eeprom i2c module  
  - The data interface RXD of BK7236 i2c module is connected with the data interface sdio_e2 of eeprom i2c module  
  - Block block BK7236 development board and eeprom, total eeprom external voltage of 3.3v

The cmd example of i2c1 as follows:

  - i2c_driver init  
  - i2c 1 init  
  - i2c 1 memory_write 10 0x50  
  - i2c 1 memory_read 10

Case 1 of i2c0
***************************

Operate
***************************
Execute cmd init i2c_driver and observe the process

Output
***************************

::

 [SYNC]:i2c driver init

Case 2 of i2c0
**************************

Operate
***************************
Execute cmd init i2c and observe the process

Output
***************************

::

 [SYNC]:I2C(0) init ok, baud_rate:400000
 [SYNC]:I2C(0) init

Case 3 of i2c0
*************************

Operate
**************************
Execute cmd i2c 0 memory_write 10 0x50 and observe the process

Output
**************************

::

 [SYNC]:memory_write buf_len:10

Case 4 of i2c0
**************************

Operate
**************************
Execute cmd i2c 0 memory_read 10 and observe the process

Output
**************************

::

 [SYNC]:i2c_read_buf[0]=0  
 [SYNC]:i2c_read_buf[1]=1  
 [SYNC]:i2c_read_buf[2]=2  
 [SYNC]:i2c_read_buf[3]=3  
 [SYNC]:i2c_read_buf[4]=4  
 [SYNC]:i2c_read_buf[5]=5  
 [SYNC]:i2c_read_buf[6]=6  
 [SYNC]:i2c_read_buf[7]=7  
 [SYNC]:i2c_read_buf[8]=8  
 [SYNC]:i2c_read_buf[9]=9  
 [SYNC]:i2c(0) memory_read buf_len:10

Case 5 of i2c1
***************************

Operate
***************************
Execute cmd init i2c_driver and observe the process

Output
***************************

::

 [SYNC]:i2c driver init

Case 6 of i2c1
**************************

Operate
***************************
Execute cmd init i2c and observe the process

Output
***************************

::

 [SYNC]:I2C(1) init ok, baud_rate:400000
 [SYNC]:I2C(1) init

Case 7 of i2c1
*************************

Operate
**************************
Execute cmd i2c 1 memory_write 10 0x50 and observe the process

Output
**************************

::

 [SYNC]:memory_write buf_len:10

Case 8 of i2c1
**************************
Operate
**************************
Execute cmd i2c 1 memory_read 10 and observe the process

Output
**************************

::

 [SYNC]:i2c_read_buf[0]=0  
 [SYNC]:i2c_read_buf[1]=1  
 [SYNC]:i2c_read_buf[2]=2  
 [SYNC]:i2c_read_buf[3]=3  
 [SYNC]:i2c_read_buf[4]=4  
 [SYNC]:i2c_read_buf[5]=5  
 [SYNC]:i2c_read_buf[6]=6  
 [SYNC]:i2c_read_buf[7]=7  
 [SYNC]:i2c_read_buf[8]=8  
 [SYNC]:i2c_read_buf[9]=9  
 [SYNC]:i2c(1) memory_read buf_len:10
