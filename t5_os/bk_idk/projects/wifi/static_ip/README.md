| Supported Targets | BK7256 | BK7235 | BK7236 | BK7258 |
| ----------------- | -------| -------| -------| -------|

# Armino Wi-Fi static_ip exmaple

(See the README.md file in the upper level 'examples' directory for more informations about examples.)

## How to use example

### Configure the project

```
Kconfig.projbuild
```

Set following parameters under Example Configuration Options:

* Set `WiFi SSID` and `WiFi Password` under Example Configuration Options

* Set `static ip4 address` for your device

* Set `static ip4 netmask` for your device

* Set `static ip4 gateway` for your device

* Set `static ip4 dns server` for your device

### Build and Flash

Build the example project with following command and flash all-app.bin to your board，run UART tool to see output

```
make bk7256 PROJECT=examples/wifi/static_ip
```

(Note: all-app.bin located in sdk/build/static_ip/bk7256)

## Example Output
Note that the output may vary deponding on the practical environment.

When flashing successfully and power on, board will connect to configured target AP automaticly:
```
init:I(0):reason - power on
init:I(0):regs - 0, 55555555, aaaaaaaa
init:I(0):armino rev:
init:I(0):armino soc id:53434647_72560001
wifid I:(10): IP Rev: 802.11ax
wifi I:(308): wifi inited(1) ret(0)
STATIC:I(310):ssid:TP-LINK_923B password:12345678
mac:I(312):sync_mac_record, base mac:c8:47:8c:6b:66:c3
mac:I(314):sync_mac_record, saved base mac:c8:47:8c:6b:66:c3
lwip:I(636):configuring interface sta (with Static IP)
lwip:I(638):using static ip...
STATIC:I(642):STA connected to TP-LINK_923B
STATIC:I(646):STA got ip: 192.168.1.199
```
