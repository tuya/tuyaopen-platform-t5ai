## Project：rlk_demo

## Application：
	This demos are applications based on RAW LINK. Supported applications is as follows:
	
	RLK PING: rlk_ping target [options]
			target:must be mac address(full 6 byte or last 3 byte)
			option:
					-c        ping count, default is 4 times
					-i        interval, default is 1 second
					-s        packetsize, default is 32byte
					-t        timeout, default is 1 second(Does not support setting a timeout greater than 1 second)
					--stop    stop ping program
					-h        help info

## Special Macro Configuration Description：

## Complie Command:
1、make bk7256 PROJECT=wifi/rlk_demo

## CPU:
1、bk7256: cpu0+cpu1

## WIFI: Law Link wifi
