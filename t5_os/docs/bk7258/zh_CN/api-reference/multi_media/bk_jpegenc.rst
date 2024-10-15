JPEGENC APIs
================

:link_to_translation:`en:[English]`

.. important::

   The JPEGENC API v1.0 is the lastest stable JPEGENC APIs. All new applications should use JPEGENC API v1.0.

JPEGENC Interface
----------------------------
The JPEGENC module can work two different mode
 - 0: JPEG ENCODER mode, output jpeg data
 - 1: YUV mode, output camera sample data, yuv data

JPEGENC API Categories
----------------------------

Most of JPEGENC APIs can be categoried as:

 - JPEGENC APIs

   The common APIs are prefixed with bk_jpeg_enc, e.g. bk_jpeg_enc_driver_init() etc.

JPEGENC APIs:
 - :cpp:func:`bk_jpeg_enc_driver_init` - init jpeg encode module driver
 - :cpp:func:`bk_jpeg_enc_driver_deinit` - deinit jpeg encode module driver
 - :cpp:func:`bk_jpeg_enc_init` - init jpeg encode module
 - :cpp:func:`bk_jpeg_enc_deinit` - deinit jpeg encode module
 - :cpp:func:`bk_jpeg_enc_get_frame_size` - get a frame size output from jpeg  encode module

API Reference
----------------------------------------

.. include:: ../../_build/inc/jpeg_enc.inc

API Typedefs
----------------------------------------
.. include:: ../../_build/inc/jpeg_enc_types.inc
