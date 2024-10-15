# Project：Wi-Fi Repeater Project

## Introduction
Repeater, is the middle of the continuation, the extension of the extension of the meaning. 
The main role of wireless repeater is to extend the WiFi signal, after the extension of WiFi, 
so that the original coverage of WiFi places can not have WiFi, free of wiring troubles. 
The two wireless routers are bridged together to achieve full coverage of WiFi signals.

* Run the demo as station mode and join the target AP
```sh
sta ssid password
```

```
cmd: ap bk_test
cmd: sta bk_test



## Special Macro Configuration Description：

```
CONFIG_WIFI_CLI_ENABLE=y       // support wifi cli enable
CONFIG_IPERF_TEST=y            // support wifi iperf test
CONFIG_CLI=y                   // support cli enable

```

## Complie Command:

```sh
make bk7236 PROJECT=wifi/repeater
make bk7256 PROJECT=wifi/repeater
make bk7258 PROJECT=wifi/repeater
```

(Note: all-app.bin located in `armino/build/repeater/bk7256` )




