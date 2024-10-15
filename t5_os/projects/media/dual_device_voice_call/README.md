## Project：dual_device_voice_call

## Life Cycle：2024-12-13 ~~ 2024-12-06

## Application：
	This demo is used as the client side of the dual board pairing. The client 
connect AP provided by server, sends the voice data collected by the mic to the server peer, 
and at the same time receives the voice data transmitted by the server peer and 
sends it to the speaker for playback.

## Special Macro Configuration Description：

CONFIG_ASDF=y                            //support audio software development framework
CONFIG_ASDF_WORK_CPU0=y
CONFIG_ASDF_ONBOARD_MIC_STREAM=y         //support audio onboard mic
CONFIG_ASDF_ONBOARD_SPEAKER_STREAM=y     //support audio onboard speaker
CONFIG_ASDF_G711_ENCODER=y               //support audio g711 encoder
CONFIG_ASDF_G711_DECODER=y               //support audio g711 decoder
CONFIG_ASDF_RAW_STREAM=y                 //support audio raw stream
CONFIG_ASDF_AEC_ALGORITHM=y              //support audio aec alogrithm

## Complie Command:
1、make bk7256 PROJECT=media/dual_device_voice_call

## CPU:
1、bk7256: cpu0

## Media: audio

## WIFI: AP
