中控(Central)
======================================

:link_to_translation:`en:[English]`

1 功能概述
-------------------------------------

	用于手机、拨号盘等主设备场景，开启a2dp source、avrcp ct/tg、ble等feature。

2 代码路径
-------------------------------------

	Demo路径：`./projects/bluetooth/central <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/bluetooth/central>`_

	编译命令：``make bk7258 PROJECT=bluetooth/central``

3 a2dp source cli命令简介
-------------------------------------

    +--------------------------------------------------+---------------------------+
    | a2dp_player connect <xx:xx:xx:xx:xx:xx>          | 连接音响                  |
    +--------------------------------------------------+---------------------------+
    | a2dp_player disconnect <xx:xx:xx:xx:xx:xx>       | 断开连接                  |
    +--------------------------------------------------+---------------------------+
    | a2dp_player play <xxx.mp3>                       | 播放mp3                   |
    +--------------------------------------------------+---------------------------+
    | a2dp_player stop                                 | 停止播放                  |
    +--------------------------------------------------+---------------------------+
    | a2dp_player pause                                | 暂停                      |
    +--------------------------------------------------+---------------------------+
    | a2dp_player resume                               | 恢复                      |
    +--------------------------------------------------+---------------------------+
    | a2dp_player abs_vol <xxx>                        | 设置音响绝对音量 0 ~ 127  |
    |                                                  | (是否有效视对端而定)      |
    +--------------------------------------------------+---------------------------+

4 a2dp source 测试过程
-------------------------------------

    | 1.准备一张sd卡，格式化成exfat，将project/bluetooth/central/1_qcs.mp3放入根目录。(必须是16bits的mp3)
    | 2.插入sd卡，开机。
    | 3.令音响进入配对模式
    | 4.输入 ``a2dp_player connect xx:xx:xx:xx:xx:xx``，其中xx为音响地址。等待连接成功
    | 5.如果连接成功，会提示"a2dp connect complete"，如果连接失败，会提示"Unsuccessful Connection Complete"之类的log。
    | 6.输入 ``a2dp_player play xxx.mp3``。
    | 7.如果sd卡正常，会提示"f_mount OK"，如果音乐文件存在，会提示"mp3 file open successfully"
    | 8.此时可听到播出声音
    | 9.正在播放的情况下，可以stop pause，停止播放的情况下可以play。(尽量连接后立刻play，不要stop，参见章节5第一节)
    | 10.如果音响支持avrcp，可以通过音响控制播放暂停。(存在兼容性问题，参见章节5)


5 a2dp source 兼容性说明
-------------------------------------

    | 1.仅播放歌曲场景，某些音响(例如JBL)如果长时间处于stop(本地a2dp_player stop 或对端avdtp suspend)状态，会主动断开连接(JBL直接下电蓝牙)。log会提示bt_api_event_cb:Disconnected from xx:xx:xx:xx:xx:xx
    | 2.某些音响(例如xiaomi)不会向本地注册avrcp playback notify，会导致两端操作播放暂停时状态不一致的现象。
    | 3.某些音响(例如xiaomi)不会向本地报告avrcp volume change，此时音响调节音量，central无法得知。
    | 4.某些音响(例如xiaomi)不支持设置绝对音量。

6 a2dp source 其他注意事项
-------------------------------------

    | 1.如果sdcard有问题，会出现 f_mount failed 或 read data crc error 的提示
