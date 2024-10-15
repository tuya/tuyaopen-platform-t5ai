Display APIs
================

:link_to_translation:`zh_CN:[中文]`

LCD Display Interface
----------------------------

The BK LCD Display Driver supports following interfaces:

- i8080 LCD  interface
    1) The screen Driving IC used in the project is ST7796S
    2) The data transmission of the 8080 interface is 8-bit
    3) The interface supports RGB565 data

- RGB interface
    1) The supported screens in the project include Driving IC ST7282, HX8282 and GC9503V
    2) the data supported by the interface including RGB565, YUYV, UYVY, YYUV, UVYY, VUYY

LCD API Categories
----------------------------

LCD 8080 Config
----------------------------------------

.. figure:: ../../../_static/lcd_8080_flow.png
    :align: center
    :alt: 880 lcd Overview
    :figclass: align-center

    Figure 1. 8080 lcd config flow

LCD RGB Config
----------------------------------------

.. figure:: ../../../_static/lcd_rgb_flow.png
    :align: center
    :alt: rgb lcd Overview
    :figclass: align-center

    Figure 2. rgb lcd config flow

LCD parcical display 
----------------------------------------

.. figure:: ../../../_static/lcd_partical_display.png
    :align: center
    :alt: rgb lcd Overview
    :figclass: align-center

    Figure 3. rgb lcd config flow

局部显示参数配置

.. figure:: ../../../_static/partical_params_config.png
    :align: center
    :alt: rgb lcd Overview
    :figclass: align-center

    Figure 4. rgb lcd config flow

API Reference
----------------------------------------
.. include:: ../../_build/inc/lcd.inc

API Typedefs
-------------------------------------------
.. include:: ../../_build/inc/lcd_types.inc
