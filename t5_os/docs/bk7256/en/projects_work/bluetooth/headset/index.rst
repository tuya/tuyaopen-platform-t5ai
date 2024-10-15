Headset
======================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
-------------------------------------
    This project show how headset work with main function: bluetooth a2dp sink/avrcp tg ct/ble

2 Code Path
-------------------------------------
	demo：`./projects/bluetooth/headset <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/bluetooth/headset>`_

	build cmd：``make bk7256 PROJECT=bluetooth/headset``


3 cli
-------------------------------------
    You can use cli to test avrcp after connection completed and so on.(Attention: cli's effective depends on the phone and phone's app.)

    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,play                     | play                  |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,pause                    | pause                 |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,next                     | next music            |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,prev                     | prev music            |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,rewind[,msec]            | rewind                |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,fast_forward[,mesc]      | fast forward          |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,vol_up                   | volume up             |
    +-------------------------------------------+-----------------------+
    | AT+BT=AVRCP_CTRL,vol_down                 | volume down           |
    +-------------------------------------------+-----------------------+
    | AT+BT=ENTER_PAIRING_MODE                  | enter pairing mode    |
    +-------------------------------------------+-----------------------+


