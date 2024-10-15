# ARMINO bk_avdk开发框架

* [English Version](./README.md)

欢迎使用 Armino!
Armino 是**博通集成**推出的物联网开发框架，旨在支持**博通集成** 2022 年之后推出的各种物联网芯片，
同时兼容各种主流物联网操作系统，如 RT-Thread，AliOS，HarmoneyOS 等，Armino 默认支持 FreeRTOS。

bk_avdk为audio 和 video SDK。
bk_idk为Armino生态的基础SDK。

# ARMINO 版本与芯片

下表总结了博通集成芯片在 Armino 各版本中的支持状态，其中 ![alt text][支持] 代表已支持，![alt text][开发中] 代表目前处于开发状态。
当芯片处于开发阶段时，芯片的构建会被关闭，一些重要的内容（如文档和技术规格书等）可能会缺失。请确保使用与芯片相匹配的 Armino 版本


|Branch\Chip   |      [BK7258]      |      Comment            |
|:------------ |:-----------------: |:----------------------: |
|release/v2.0.1|![alt text][支持]  |sdk 2.0 MP Branch   |


[支持]: https://img.shields.io/badge/-supported-green "支持"
[开发中]: https://img.shields.io/badge/-developing-orange "开发中"

==Tag说明，release分支Tag为正式版本，请使用正式发布版本，进行开发。==

release/xxxx分支，格式为vx.x.x.x，例如：v2.0.1.1


# ARMINO 版本与其他物联网操作系统

下表总结了主流物联网操作系统在 Armino 各版本中的支持状态，其中 ![alt text][支持] 代表已支持，![alt text][开发中] 代表目前处于开发状态。

|OS           |        main           |
|:----------- |:---------------------: |
|FreeRTOS     | ![alt text][支持]       |

[支持]: https://img.shields.io/badge/-supported-green "支持"
[开发中]: https://img.shields.io/badge/-developing-orange "开发中"

# 快速入门

下载代码:

git clone --recurse-submodules git@gitlab.bekencorp.com:armino/bk_avdk.git -b v2.0.1.1

编译示例:

make bk7256 PROJECT=lvgl/86box
