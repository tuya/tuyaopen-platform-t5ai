H264解码
=================================

:link_to_translation:`en:[English]`

1、H264解码
--------------------

    H264解码部分在手机端实现，本文使用ffmpeg库进行解码。


2、ffmpeg官网地址
--------------------

	https://ffmpeg.org/

3、解码流程图
--------------------

.. figure:: ../../../_static/ffmpeg_h264_decode_flow.png
    :align: center
    :alt: fill specific color
    :figclass: align-center

    Figure 1. decode_flow

4、流程说明
-------------------------------

1. 注册解码器:``avcodec_register_all``

   用于注册所有已编译的编解码器

2. 查找解码器:``avcodec_find_decoder``

   根据指定的解码器ID查找对应的解码器,h264解码器对应ID为：AV_CODEC_ID_H264

3. 初始化解析器:``av_parser_init``

   初始化解析器上下文。解析器在处理数据流之前，可以用于预处理输入数据，例如找到帧边界、提取关键信息等。

4. 分配解码器上下文:``avcodec_alloc_context3``

   分配一个 AVCodecContext 结构体，并设置默认值。AVCodecContext 结构体是 FFmpeg 中用于存储编解码器的上下文信息的数据结构。

5. 打开解码器:``avcodec_open2``

   打开编解码器并初始化其上下文（AVCodecContext）。在解码器上下文初始化之后，通过这个函数可以打开解码器，以便开始进行解码操作。

6. 接收数据

   每次开始解码之前，先判断数据是否以sps帧开始，直到查找到sps开始的数据，才开始进行解码，否则丢弃数据，不进行解码。

7. 发送数据给解析器:``av_parser_parse2``

   解析输入的媒体数据流，通常用于找到帧的边界

8. 发送数据给解码器:``avcodec_send_packet``

   用于向解码器发送数据包以进行解码。它将输入的数据包发送到解码器上下文进行解码。

9. 接收解码以后的数据帧:``avcodec_receive_frame``

   用于从解码器接收解码后的帧。在使用 ``avcodec_send_packet`` 函数发送数据包进行解码后，可以使用 ``avcodec_receive_frame`` 来获取解码器解码出的图像帧。
