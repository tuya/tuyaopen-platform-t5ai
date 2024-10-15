LVGL
===============================

:link_to_translation:`zh_CN:[中文]`

A varity of lvgl demo projects are provided on BK7258 to demonstrate the functions of different typed and scenarios. Details are shown in the table below.

    +--------------------+---------------+------------------+------------------------------------+
    |Project name        |LCD resolution |Data format       |Project description                 |
    +====================+===============+==================+====================================+
    |86box               |480*480        |RGB565            |86box demo                          |
    +--------------------+---------------+------------------+------------------------------------+
    |86box_smart_panel   |480*480        |RGB565            |86box demo with speech recognition  |
    +--------------------+---------------+------------------+------------------------------------+
    |benchmark           |480*480        |RGB565            |LVGL performancescores demo         |
    +--------------------+---------------+------------------+------------------------------------+
    |camera              |480*854        |RGB565            |LVGL and camera image switching demo|
    +--------------------+---------------+------------------+------------------------------------+
    |keypad_encoder      |800*480        |RGB565            |LVGL official demo                  |
    +--------------------+---------------+------------------+------------------------------------+
    |meter               |400*400        |RGB565            |QSPI LCD animation demo             |
    +--------------------+---------------+------------------+------------------------------------+
    |meter_rgb_16M       |720*1280       |RGB888            |RGB888 LCD animation demo           |
    +--------------------+---------------+------------------+------------------------------------+
    |music               |720*1280       |RGB565            |LVGL official demo                  |
    +--------------------+---------------+------------------+------------------------------------+
    |stress              |800*480        |RGB565            |LVGL official demo                  |
    +--------------------+---------------+------------------+------------------------------------+
    |widgets             |1024*600       |RGB565            |LVGL official demo                  |
    +--------------------+---------------+------------------+------------------------------------+

The compilation command of the above demo project is ``make bk7258 PROJECT=lvgl/xxx``. After the compilation and download is completed, it will be executed by default after power on.

Except for the LCD used in the camera project and meter project, which does not include touch function, the rest of the projects have turned on touch function. In addition, in addition to the normal LVGL UI display, the 86box project, 86box_smart_panel project and camera project also show another other different function. After powering on, the display need to be tested according to the following.

 - 86box 

   The 86box project shows how to turn off the LCD display and LVGL drawing. You can enter the command ``86box`` to test the shutdown.

 - 86box_smart_panel

   The 86box_smart_panel project demonstrates the voice recognition function. The voice commands must be issued in Chinese. They are ``阿尔米诺`` ``会客模式`` ``用餐模式`` ``离开模式``. First, you need to issue the ``阿尔米诺`` wake-up word to jump to the demo page befor you can continue to issue other wake-up words for testing.

 - camera

   The camera project shows the switching display of camera image and lvgl image. The specific test method is as follow.

  - Install the IoT software on your mobile phone. After instation, click on the software to go to the main page.
  - Use the IoT software to click on the upper right corner to add the ``BK7258_DL_04`` device and click on start adding. Then click ``Select Wi-Fi`` to connect to a network that can connect to the Internet and click next. 
  - Select your own Bluetooth device, which can be viewed in the power-on log or enter the ``mac`` command to view the MAC address of the device. Click and wait for the device's network connection progress to reach 100%.
  - The phone automatically jump to the camera preview page. You can send the ``lvcam_open`` command to display the camera image on the LCD screen and send the ``lvcam_close`` command to display the LVGL image on the LCD. Continuously switch the display in sequence.
  
.. note::
  The camera resolution used in the camera project is 864x480. In addition, this project can also be tested without using mobile Iot software. You can directly enter the ``media uvc open 864X480`` command to open the camera, and then enter the corresponding switching command.