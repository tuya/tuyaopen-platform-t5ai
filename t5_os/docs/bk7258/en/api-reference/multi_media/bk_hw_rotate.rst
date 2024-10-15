Rotate APIs
================

:link_to_translation:`zh_CN:[中文]`
	
Rotate Interface
----------------------------

The main function of the Rotate module is to rotate the yuv422 image stored in memory into an image in rgb565 format, and store it in another memory after completion


The modules work in three modes:

1. Rotate clockwise mode

2. Counterclockwise rotation mode

3. Format conversion without rotation


- For rotation, if the input data is RGB565 data, then only rotation is performed


demo run dependent macro configuration:

+-----------------------+---------------------------------+----------------------------------------------------+-----+
|Name                   |Description                      |   File                                             |value|
+=======================+=================================+====================================================+=====+
|CONFIG_HW_ROTATE_PFC   | Configures the Rotate function  |``\middleware\soc\bk7236\bk7236.defconfig``         |  y  |
+-----------------------+---------------------------------+----------------------------------------------------+-----+

Rotate Categories
----------------------------

API Reference
----------------------------------------
.. include:: ../../_build/inc/rott_driver.inc

API Typedefs
-------------------------------------------
.. include:: ../../_build/inc/rott_types.inc
