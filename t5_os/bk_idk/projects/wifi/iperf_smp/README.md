# Project：wifi iperf project

## Introduction
This example implements the protocol used by the common performance measurement tool [iPerf]
Performance can be measured between two BKKEN 7236 running this example, or between a single BKKEN 7236 and a computer running the iPerf tool

* Run the demo as station mode and join the target AP
```sh
sta ssid password
```

* Run iperf as server on AP side

```sh
iperf -s -i1
```

* Run iperf as client on target side

```sh
iperf -c 192.168.0.1 -i -t60
```

The console output, which is printed by station TCP RX throughput test, looks like:

```
cmd: ap bk_test
cmd: sta bk_test

netif(sta) ip4=192.168.0.100 mask=255.255.255.0 gate=192.168.0.1 dns=192.168.0.1

>iperf> iperf -s -i -t1000
iperf: new client connected from (192.168.0.100, 61285)
```

```
[0-1] sec bandwidth: 15865 Kbits/sec.
[1-2] sec bandwidth: 17908 Kbits/sec.
[2-3] sec bandwidth: 17434 Kbits/sec.
[3-4] sec bandwidth: 18688 Kbits/sec.
[4-5] sec bandwidth: 18765 Kbits/sec.
[5-6] sec bandwidth: 21490 Kbits/sec.
[6-7] sec bandwidth: 21765 Kbits/sec.
[7-8] sec bandwidth: 21095 Kbits/sec.
[8-9] sec bandwidth: 20800 Kbits/sec.
```

Steps to test station/soft-AP TCP/UDP RX/TX throughput are similar as test steps in station TCP TX.

## Special Macro Configuration Description：

```
CONFIG_WIFI_CLI_ENABLE=y       // support wifi cli enable
CONFIG_IPERF_TEST=y            // support wifi iperf test
CONFIG_CLI=y                   // support cli enable

CONFIG_CPU_CNT=2
CONFIG_SOC_SMP=y
CONFIG_FREERTOS_SMP=y
CONFIG_FREERTOS_SMP_TEMP=y
CONFIG_NR_CPUS=2
CONFIG_SOC_BK7236_SMP_TEMP=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=0
CONFIG_SOC_BK7236_SMP=y
```

## Complie Command:

```sh
make bk7236 PROJECT=examples/wifi/iperf_smp
```

(Note: all-app.bin located in `armino/build/iperf_smp/bk7236` )

## CPU:
BK7236 DualCore

