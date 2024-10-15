JPEG Decode SW
=================================

:link_to_translation:`zh_CN:[中文]`

1. JPEG Soft Decoding Process
------------------------------------

The main process of JPEG soft decoding is as follows:

1) Pre decoding, parsing the header of a JPEG data to obtain image format, image length and width, quantization table, and Huffman table;

2) Search for quantization tables and Huffman tables to parse a block of data;

3) Convert a block data into the image format that needs to be output;

4) Write the converted image data into the output image;

5) Repeat 2-4 until an exception occurs or decoding is complete;

.. figure:: ../../../_static/jpeg_decode_sw_1.png
    :align: center
    :alt: jpeg_decode sw
    :figclass: align-center

    Figure 1. jpeg soft decode process

2. Memory requirements
------------------------------------

JPEG soft decoding relies on threads within 1K, and the main buffers and purposes required are as follows:

1) A 10240 buffer for storing Huffman tables and intermediate data processing;

2) A buffer of size 0xB0, used to store intermediate running pointers and variables;

3) A 16 * 16 * 2 sized buffer for image rotation after soft decoding;

3. Use of JPEG soft decoding module
------------------------------------

1) Call `bk_jpeg_dec_sw_init` to initialize the soft decoding module;

2) Call `bk_jpeg_dec_sw_start` to decode the incoming image;

3) Call `bk_jpeg_dec_sw_deinit` to release the internal buffer of soft decoding;

Please refer to `API Reference : <../../api-reference/multi_media/bk_jpegdec_sw.html>`_ for function parameters.

4. One time decoding function
------------------------------------

For some scenarios, a one-time decoding interface `bk_jpeg_dec_sw_start_one_time` is provided,
Before using this interface, there is no need to call `bk_jpeg_dec_sw_init`, and the buffer is passed in externally;

If NULL is passed in externally, it will be automatically requested by the interface internally; Automatically release the internal requested buffer after decoding is completed;

.. note::

    This interface can be decoded in parallel with the soft decoding module, but the decoding speed will be reduced when parallel;