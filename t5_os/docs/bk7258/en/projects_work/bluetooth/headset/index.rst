Headset
======================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
-------------------------------------
    This project show how headset work with main function: bluetooth a2dp sink/avrcp tg ct/ble

2 Code Path
-------------------------------------
	demo：`./projects/bluetooth/headset <https://gitlab.bekencorp.com/wifi/armino/-/tree/main/projects/bluetooth/headset>`_

	build cmd：``make bk7258 PROJECT=bluetooth/headset``


3 cli
-------------------------------------
    You can use cli to test avrcp after connection completed and so on.(Attention: cli's effective depends on the phone and phone's app.)

    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=play                     | play                  |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=pause                    | pause                 |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=next                     | next music            |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=prev                     | prev music            |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=rewind[,msec]            | rewind                |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=fast_forward[,mesc]      | fast forward          |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=vol_up                   | volume up             |
    +-----------------------------------------+-----------------------+
    | AT+BTAVRCPCTRL=vol_down                 | volume down           |
    +-----------------------------------------+-----------------------+
    | AT+BTENTERPAIRINGMODE                   | enter pairing mode    |
    +-----------------------------------------+-----------------------+


