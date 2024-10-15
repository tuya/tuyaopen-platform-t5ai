Camera 概述 
=======================

:link_to_translation:`en:[English]`

.. toctree::
   :maxdepth: 1

1 工作原理
-------------------------------------
	景物通过镜头（LENS）生成的光学图像投射到图像传感器（SENSOR）表面，然后转为电信号，经过A/D（模数转换）转换后变成数字图像信号，再送到数字信号处理芯片（DSP）中加工处理，后输出 YUV 或者 RGB 格式的数据。

.. figure:: ../../../../common/_static/camera_working_principle_flow.png
   :align: center
   :alt: camera_working_principle flow
   :figclass: align-center

   Figure 1.工作原理流程

2 主要部件
-------------------------------------
   一般来说，camera 主要是由 lens 和 sensor IC 两部分组成，其中有的 sensor IC 集成 了 DSP，有的没有集成，但也需要外部 DSP 处理。

1、lens（镜头） 

   camera 的镜头结构是有几片透镜组成，分有塑胶透镜（Plastic）和玻璃透 镜(Glass) ，通常镜头结构有：1P,2P,1G1P,1G3P,2G2P,4G 等。

.. figure:: ../../../../common/_static/lens_type.png
   :align: center
   :alt: lens type
   :figclass: align-center

   Figure 2.镜头结构

2、sensor（图像传感器）

 - Sensor 是一种半导体芯片，有两种类型：CCD（Charge Coupled Device）即电荷耦合器件的缩写 和 CMOS（Complementary Metal-Oxide Semiconductor）互补金属氧化物半导体。
 - Sensor 将从 lens 上传导过来的光线转换为电信号， 再通过内部的 AD 转换为数字信号。 由于 sensor 的每个 pixel 只能感光 R 光或者 B 光或者 G 光， 因此每个像素此时存贮的是单色的， 我们称之为 RAW DATA 数据。
   要想将每个像素的 RAW DATA 数据还原成三基色，就需要 ISP 来处理。 ISP（图像信号处理） 主要完成数字图像的处理工作，把 sensor 采集到的原始数据转换为显示支持的格式。

.. note::
 - 1、CCD传感器，电荷信号先传送，后放大，再A/D，成像质量灵敏度高、分辨率好、噪声小；处理速度慢；造价高，工艺复杂。
 - 2、CMOS传感器，电荷信号先放大，后A/D，再传送；成像质量灵敏度低、噪声明显；处理速度快；造价低，工艺简单。

3、CAMIF（camera 控制器

 芯片上的 camera 接口电路，对设备进行控制，接收 sensor 采集的数据交给 CPU，并送入 LCD 进行显示。

3 摄像头接口
-------------------------------------
 摄像头常见的两种接口:UVC接口和DVP接口。

- USB接口，只有数据线，没有时钟线。

 .. figure:: ../../../../common/_static/uvc_interface.png
    :align: center
    :alt: uvc interface
    :figclass: align-center

    Figure 3.UVC接口


- DVP（Digital Video Port/Parally Port）接口，主要由电源总线，输入总线，输出总线组成。
 
 .. figure:: ../../../../common/_static/dvp_interface.png
    :align: center
    :alt: dvp interface
    :figclass: align-center

    Figure 4.DVP接口

 - 输入总线

   PWDN：camera的使能管脚，可以配置为两种模式，一种为standby，一种为normal work。当配置为standby时，包括复位在内的一切操作对camera是无效的，所以该管脚要设为normal work复位才有效。

   MCLK：提供给camera的工作时钟。

   IIC_SDA/IIC_SCL：用来读写sensor的寄存器，配置寄存器。

 - 输出总线

   PCLK：像素同步信号管脚。
   
   VSYNC：帧同步信号。

   HSYN：行同步信号。

   DATA[0-7]：输出数据管脚。

 - 电源总线

   AVDD：camera的模拟电压，主要给camera的感光区和ADC部分供电。
   
   IOVDD：camera的GPIO口模拟电压，主要给IIC或者DVP部分供电。

   DVDD：camera的数字工作电压，若供电不稳定可能会导致花屏。










