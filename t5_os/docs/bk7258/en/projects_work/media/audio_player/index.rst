Audio Player
========================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
-------------------------------------
	The project shows the function of network music player, which mainly realizes the function of network music download and play, pause, continue to play, set the volume, the previous song and the next song.

2 Code Path
-------------------------------------
	Demo path: ``./projects/media/doorbell/main/src/doorbell_udp_service.c``

	Compile command: ``make bk7258 PROJECT=media/audio_player``

3 Cli Overview
-------------------------------------
	The project first needs to send command ``sta {ssid} {password}`` to connect to the available network, and the commands for other functions are as follows:

	+-----------------------------+---------------+
	| audio_player start          | start play    |
	+-----------------------------+---------------+
	| audio_player pause          | pause play    |
	+-----------------------------+---------------+
	| audio_player play           | continue play |
	+-----------------------------+---------------+
	| audio_player {volume_value} | set volume    |
	+-----------------------------+---------------+
	| audio_player last           | previous song |
	+-----------------------------+---------------+
	| audio_player next           | next song     |
	+-----------------------------+---------------+

	The volume value ranges from 0x00 to 0x3F, and the current volume value is 0x2D by default.