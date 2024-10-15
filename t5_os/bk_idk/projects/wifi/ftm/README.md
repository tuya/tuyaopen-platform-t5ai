Wi-Fi FTM: Fine Timing Measurement.
RTT: Round Trip Time
The distance of the device to a AP can be calculated by RTT. And FTM is one of the ways that can measure RTT.
distance = RTT * c / 2 (Where c is the speed of light)


With the FTM example, users can scan for APs that support FTM Responder role and perform FTM procedure.
1. Start a ap that support FTM Responder role to ensure valid FTM results.
2. Set a valid SSID and a Password of a exist ap in "projects\wifi\ftm\main\Kconfig.projbuild" (You'd better connect to a AP that support FTM Responder role)
3. Generate FTM bin in the path "build/ftm/bk7256/all_app.bin" by the command "make bk7256 PROJECT=wifi/ftm"
4. Download all_app.bin, then it will connect to the ap, and start a FTM procedure automatically after 10s.


main log:
wifi:I(3360):FTM starting
wifi:I(3360):ftm config: ftm_per_burst 10, nb_ftm_rsp 10
wifid:I(3360):scanu_start_req:src_id=13,vif=0,dur=0,chan_cnt=14,ssid_len=0,bssid=0xffff-ffff-ffff
wifid:I(4056):scanu_confirm:status=0,req_type=0,upload_cnt=85,recv_cnt=85,time=696763us,result=37
wifi:I(4644):ftm found 1 responser
wifid:I(4644):The distance to c8:47:8c:40:f5:a5 is 4.20 meters, rtt is 28 nSec
wifi:I(4654):ftm free result