JPEGDEC APIs
================

:link_to_translation:`zh_CN:[中文]`

.. important::

   The JPEGDEC API v1.0 is the lastest stable JPEGDEC APIs. All new applications should use JPEGDEC API v1.0. This component is software decode not hardware decode

JPEGDEC API Categories
----------------------------

Most of JPEGDEC APIs can be categoried as:

 - JPEGDEC APIs

   The common APIs are prefixed with bk_jpeg_dec_sw, e.g. bk_jpeg_dec_sw_init() etc.

JPEGDEC APIs:
 - :cpp:func:`bk_jpeg_dec_sw_init` - init jpeg decode software module
 - :cpp:func:`bk_jpeg_dec_sw_deinit` - deinit jpeg decode software module
 - :cpp:func:`bk_jpeg_dec_sw_start` - excute jpeg decode
 - :cpp:func:`bk_jpeg_dec_sw_register_finish_callback` - register jpeg decode finish callback
 - :cpp:func:`bk_jpeg_dec_sw_start_one_time` - excute jpeg decode without relying on initalization

API Reference
----------------------------------------

.. include:: ../../_build/inc/jpeg_decode_sw.inc

API Typedefs
----------------------------------------