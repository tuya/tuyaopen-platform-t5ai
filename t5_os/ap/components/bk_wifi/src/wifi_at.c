// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <common/bk_typedef.h>

#include <../../../include/os/os.h>

#include "atsvr_unite.h"
#include "bk_wifi.h"
#include "wifi_at.h"

#include "bk_private/bk_wifi.h"
#include "bk_private/bk_wifi_types.h"
#include "bk_net.h"
#include "event.h"


/*global variable defination*/

#define TAG "AT_WLAN"

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

/*
 * Compact form for string representation of MAC address
 * To be used, e.g., for constructing dbus paths for P2P Devices
 */
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#ifndef os_memcpy
#define os_memcpy(d, s, n) memcpy((d), (s), (n))
#endif
#ifndef os_memmove
#define os_memmove(d, s, n) memmove((d), (s), (n))
#endif
#ifndef os_memset
#define os_memset(s, c, n) memset(s, c, n)
#endif
#ifndef os_memcmp
#define os_memcmp(s1, s2, n) memcmp((s1), (s2), (n))
#endif
#ifndef os_malloc
#define os_malloc(s) malloc((s))
#endif
#ifndef os_realloc
#define os_realloc(p, s) realloc((p), (s))
#endif
#ifndef os_free
#define os_free(p) free((p))
#endif

extern const char *wifi_sec_type_string(wifi_security_t security);

extern _at_svr_ctrl_env_t _at_svr_env;


at_wlan_status_t at_wlan_stat = {0};
static  at_wlan_cfg_t at_wlan_cfg = {0};

static void at_wlan_config_init(at_wlan_cfg_t* cptr)
{
	cptr->sta_use_sema = false;
	cptr->sta_use_sema = false;
	cptr->sta_use_sema = false;
	rtos_init_semaphore(&cptr->at_wlan_scan_sema,1);
	rtos_init_semaphore(&cptr->sta_protection   ,1);
	rtos_init_semaphore(&cptr->sap_protection,1);

}

static bk_err_t at_wlan_status_callback(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	//wifi_linkstate_reason_t info = *(wifi_linkstate_reason_t*)ctxt;
	char* at_tag = "wifi";//must same as reigister tag name
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;
	wifi_event_scan_done_t *scan_done;
    
	BK_LOGV(TAG,"[AT:WLAN],callback_event(%x)\r\n",event_id);
	if(!penv->atsvr_mode)
		return BK_OK;
	else{
		switch(event_id){
		case EVENT_WIFI_SCAN_DONE:
			scan_done = (wifi_event_scan_done_t*)event_data;
			at_wlan_stat.scan_time = scan_done->scan_use_time;

			if (at_wlan_cfg.at_wlan_scan_sema)
			{
				rtos_set_semaphore(&at_wlan_cfg.at_wlan_scan_sema);
			}
			break;
		case EVENT_WIFI_STA_CONNECTED:
			sta_connected = (wifi_event_sta_connected_t*)event_data;
			atsvr_event_sender(at_tag,AT_WLAN_STA_CONNECTED,sizeof(wifi_event_sta_connected_t),sta_connected,true);
			break;
		case EVENT_WIFI_STA_DISCONNECTED:		
			sta_disconnected = (wifi_event_sta_disconnected_t*)event_data;
			at_wlan_stat.disconenct_reason = sta_disconnected->disconnect_reason;
			at_wlan_stat.local_generated = sta_disconnected->local_generated;
			if ((at_wlan_cfg.at_wlan_scan_sema!=NULL)&&(penv->atsvr_mode == true)&&(at_wlan_stat.disconenct_reason==0))
				break;	
			atsvr_event_sender(at_tag,AT_WLAN_STA_DISCONNECT,0,NULL,false);
			if(at_wlan_stat.station_status==AT_WLAN_STATION_UP)
			{
				at_wlan_stat.station_status = AT_WLAN_STATION_DOWN;
			}

			break;
		case EVENT_WIFI_AP_CONNECTED:
			ap_connected = (wifi_event_ap_connected_t*)event_data;
			at_wlan_stat.softap_status= AT_WLAN_SOFTAP_UP;
			atsvr_event_sender(at_tag,AT_WLAN_SAP_CONNECTED,sizeof(wifi_event_ap_connected_t),ap_connected,true);

			break;
		case EVENT_WIFI_AP_DISCONNECTED:
			ap_disconnected = (wifi_event_ap_disconnected_t*)event_data;
			atsvr_event_sender(at_tag,AT_WLAN_SAP_DISCONNECT,sizeof(wifi_event_ap_disconnected_t),ap_disconnected,true);

			break;

		default:

			break;
		}
	}
	return BK_OK;
}

static bk_err_t at_wlan_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	//netif_event_got_ip4_t *got_ip = (netif_event_got_ip4_t*)event_data;
	//char resultbuf[100] = {0};
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
	if(!penv->atsvr_mode)
		return BK_OK;
	else{
		switch (event_id) {
		case EVENT_NETIF_GOT_IP4:		
			atsvr_event_sender("wifi",AT_WLAN_STA_GOT_IP,0,NULL,false);

			break;
		case EVENT_NETIF_DHCP_TIMEOUT:
			BK_LOGE(TAG,"DHCP timeout\r\n");
			rtos_set_semaphore(&at_wlan_cfg.sta_protection);
			memset(at_wlan_stat.static_ip,0,16);
			atsvr_event_sender("wifi",AT_WLAN_LOSS_DHCP,0,NULL,false);

			break;
		default:

			BK_LOGD(TAG,"Module:%d,Unsupport Rx Event:%d\n", event_module, event_id);
			break;
		}
	}

	return BK_OK;
}

static void wlan_at_init(void)
{
	//bk_wlan_status_register_cb(at_wlan_status_callback);
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, at_wlan_status_callback, NULL));
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, at_wlan_netif_event_cb, NULL));
	return;
}


/*function define*/
int at_wlan_event_handler(atsvr_msg_t *msg)
{
	int ret = ATSVR_OK;
	int event = msg->addition_infor;	
	BK_LOGV(TAG,"event:%d\r\n",event);
	char resultbuf[200];
	wifi_event_sta_connected_t *scmsg_param;
	wifi_event_ap_disconnected_t *admsg_param;
	wifi_event_ap_connected_t *acmsg_param;
	netif_ip4_config_t         ip_param;
	os_memset(resultbuf,0,200);
	switch(event){
	case AT_WLAN_STA_SCAN_DONE:
		ATSVR_SIZEOF_OUTPUT_STRRING(AT_WLAN_EVT_SCAN_DONE);
		break;
	case AT_WLAN_STA_DISCONNECT:
		
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_DISCONNECTED"EVT:DISCONNECT REASON:%s(REASON CODE:%d)\r\nlocal generated:%s\r\n\r\n",
			at_wlan_error_code_string((wifi_err_reason_t)at_wlan_stat.disconenct_reason),at_wlan_stat.disconenct_reason,at_wlan_stat.local_generated==1?"yes":"no");
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		break;
	case AT_WLAN_STA_CONNECTED:
		scmsg_param = (wifi_event_sta_connected_t*)(msg->msg_param);
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_CONNECTED"CONNECTED SSID:%s\r\n\r\n",
			scmsg_param->ssid);
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		break;
	case AT_WLAN_STA_GOT_IP:	
		bk_netif_get_ip4_config(NETIF_IF_STA,&ip_param);
		memcpy(&at_wlan_stat.static_ip,&ip_param.ip,16);
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_GOT_IP"sta got ip:%s\r\n\r\n",at_wlan_stat.static_ip);
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		if(at_wlan_cfg.sta_protection != NULL){
			rtos_set_semaphore(&at_wlan_cfg.sta_protection);
			at_wlan_stat.dhcp = 1;
			at_wlan_stat.station_status = AT_WLAN_STATION_UP;
		}else
			snprintf(resultbuf,sizeof(resultbuf),"\r\nsta sema has lost,this connection has failed\r\nEVT:INVALID IP ADDR\r\n\r\n");
		break;
	case AT_WLAN_LOSS_DHCP:
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_STA_LOSS_DHCP"sta has lost DHCP\r\n\r\n");
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		break;
	case AT_WLAN_SAP_DISCONNECT:
		admsg_param = (wifi_event_ap_disconnected_t*)(msg->msg_param);
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_SAP_DISCONNECTED"sta :%02x:%02x:%02x:%02x:%02x:%02x has left AP\r\n\r\n",
			admsg_param->mac[0],admsg_param->mac[1],admsg_param->mac[2],admsg_param->mac[3],admsg_param->mac[4],admsg_param->mac[5]);
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		if(at_wlan_stat.hpad_sta_cnt!=0)
			at_wlan_stat.hpad_sta_cnt--;
		break;
	case AT_WLAN_SAP_CONNECTED:
		acmsg_param = (wifi_event_ap_connected_t*)(msg->msg_param);
		snprintf(resultbuf,sizeof(resultbuf),"\r\n"AT_WLAN_EVT_SAP_CONNECTED"sta :%02x:%02x:%02x:%02x:%02x:%02x has connected AP\r\n\r\n",
		acmsg_param->mac[0],acmsg_param->mac[1],acmsg_param->mac[2],acmsg_param->mac[3],acmsg_param->mac[4],acmsg_param->mac[5]);
		ATSVR_SIZEOF_OUTPUT_STRRING(resultbuf);
		at_wlan_stat.hpad_sta_cnt++;
		break;
	default:
		ret = ATSVR_ERROR;
		break;
	}
	return ret;
}

static int at_wlan_stop_station(void)
{
	BK_LOG_ON_ERR(bk_wifi_sta_stop());
	return 0;
}

static int at_wlan_station_stop(int sync,int argc, char **argv)
{
    if(argc != 0){
		atsvr_cmd_rsp_error();
        return -1;
    }

	if(at_wlan_stop_station() == 0){
		atsvr_cmd_rsp_ok();
		return 0;
	}else{
		atsvr_cmd_rsp_error();
		return -1;
	}
}

static int at_wlan_scan_done_handler(void *arg, event_module_t event_module,
        int event_id, void *event_data)
{
	/* post event */
	FUNC_1PARAM_PTR fn;
	wifi_linkstate_reason_t info = {0};
	fn = (FUNC_1PARAM_PTR)bk_wlan_get_status_cb();
	if(fn) {
		info.state = WIFI_LINKSTATE_STA_SCAN_DONE;
		info.reason_code = 0;
		(*fn)(&info);
	}

	if (at_wlan_cfg.at_wlan_scan_sema) {
		rtos_set_semaphore(&at_wlan_cfg.at_wlan_scan_sema);
	}
	return BK_OK;
}


static int at_wlan_scan_start(wifi_scan_config_t *scan_config,AT_WLAN_SCAN_ATTR_T scan_attr,int notice_val)
{	
	int err;
	wifi_scan_result_t scan_result = {0};
	if(at_wlan_cfg.at_wlan_scan_sema == NULL ) {
		err = rtos_init_semaphore(&at_wlan_cfg.at_wlan_scan_sema,1);
		if(err != kNoErr){
			BK_LOGE(TAG,"atsvr scan semaphore init failed!\r\n");
			goto error;
		}
	}

	if(scan_config)
		BK_LOGV(TAG,"scan ssid %s,type %d ,dur %d ,cnt %d\r\n",scan_config->ssid,scan_config->scan_type,scan_config->duration,scan_config->chan_cnt);

	BK_LOG_ON_ERR(bk_wifi_scan_start(scan_config));

	if(at_wlan_cfg.at_wlan_scan_sema != NULL) {
		err = rtos_get_semaphore(&at_wlan_cfg.at_wlan_scan_sema, AT_WLAN_SCAN_TIMEOUT_MS);
		if(err != kNoErr) {
			BK_LOGE(TAG,"atsvr scan wait semaphore timeout!\r\n");
			goto error;
		} else {
			/*check special scan requirement*/
			if(AT_WLAN_SCAN_DUR == scan_attr)
			{
				if(at_wlan_stat.scan_time > 1600000)//specical requirement
				{
					BK_LOGE(TAG,"scan time exceeded!\r\n");
				}
			}
			bk_wifi_scan_get_result(&scan_result); 
			BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
			bk_wifi_scan_free_result(&scan_result);
		}
	}
	else {
		BK_LOGE(TAG,"atsvr_scan_sema is NULL!!\r\n");
		goto error;
	}
	atsvr_event_sender("wifi",AT_WLAN_STA_SCAN_DONE,0,NULL,false);
	rtos_deinit_semaphore(&at_wlan_cfg.at_wlan_scan_sema);
	return 0;
error:
	if(at_wlan_cfg.at_wlan_scan_sema != NULL)
		rtos_deinit_semaphore(&at_wlan_cfg.at_wlan_scan_sema);
	at_wlan_cfg.at_wlan_scan_sema = NULL;
	at_wlan_stat.scan_time = 0;
	return -1;
}

static int at_wlan_scan_cmd(int sync,int argc, char **argv)
{
	char *ssid = NULL;
	int i = 0;
	int set_ssid = 0;
	int scan_type = 0;
	int scan_duration = 0;
	int scan_cnt=0;
	int chan_nb[WIFI_2BAND_MAX_CHAN_NUM] = {0};
	wifi_scan_config_t scan_config = {0};

	///check AT cmd length
	if((argc > 2) && (argc < 5)) {
		atsvr_cmd_rsp_error();
		return -1;
	} else if (argc >= 5) {
		scan_cnt = atoi(argv[4]);
		if(argc != (scan_cnt + 5)) {
			atsvr_cmd_rsp_error();
			return -1;
		}
	}

	if(argc == 1) {
		ssid = argv[0];
	} else if (argc > 1) {
		set_ssid = atoi(argv[0]);
		if(1 == set_ssid) {
			ssid = argv[1];
		}

		if(argc > 4) {
			scan_type = atoi(argv[2]);
			scan_duration = atoi(argv[3]);
			scan_cnt = atoi(argv[4]);
			for(i = 0;i<scan_cnt ;i++)
				chan_nb[i] = atoi(argv[5+i]);
		}
	}

	scan_config.scan_type = scan_type;
	scan_config.chan_cnt = scan_cnt;
	scan_config.duration = scan_duration;
	for(i = 0;i<scan_cnt ;i++)
		scan_config.chan_nb[i] = chan_nb[i];

	if(NULL != ssid) {
		memcpy(&scan_config.ssid,ssid,WIFI_SSID_STR_LEN);
	}

	if((scan_duration>0) && (scan_duration <= 120)) //unit is ms
	{
		if(at_wlan_scan_start(&scan_config,AT_WLAN_SCAN_DUR,scan_duration) != 0){
			atsvr_cmd_rsp_error();
			return -1;
		}
		else
		{
			atsvr_cmd_rsp_ok();
    		return 0;
		}
	}
	else
	{
		if(at_wlan_scan_start(&scan_config,AT_WLAN_SCAN_NO_USE,-1) != 0){
			atsvr_cmd_rsp_error();
			return -1;
		}
		else
		{
			atsvr_cmd_rsp_ok();
    		return 0;
		}
	}
}

static int at_wlan_scan_cmd_query(int sync,int argc, char **argv)
{
	BK_LOGD(TAG,"scan_query\r\n");
	atsvr_cmd_rsp_ok();
	return 0;
}

#include "conv_utf8_pub.h"
int at_wlan_station_start(int sync, int argc, char **argv)
{
	int err = kNoErr;
	char *ssid = NULL;
	char *password = "";
	
	if ((argc < 1) || (argc > 4)) {
		BK_LOGD(NULL, "invalid argc number\n");
		err = kParamErr;
		goto error;
	}
	
	if (argc >= 1)
		ssid = argv[0];
	
	if (argc >= 2)
		password = argv[1];
	
#if 0 //TODO support BSSID/Channel configuration
	if (argc >= 3)
		bssid = argv[2];
	
	if (argc >= 4)
		channel = argv[3];
#endif
	
	char *oob_ssid_tp = ssid;
#if CONFIG_USE_CONV_UTF8
	oob_ssid_tp = (char *)conv_utf8((uint8_t *)ssid);
#endif
	err = rtos_init_semaphore(&at_wlan_cfg.sta_protection, 1);
	if(err != kNoErr){
		goto error;
	}
	if (oob_ssid_tp) {
		extern int demo_sta_app_init(char *oob_ssid, char *connect_key);
		demo_sta_app_init((char *)oob_ssid_tp, password);
#if CONFIG_USE_CONV_UTF8
		os_free(oob_ssid_tp);
#endif
	} else {
		BK_LOGD(NULL, "not buf for utf8\r\n");
	}

	if (at_wlan_cfg.sta_protection != NULL)
	{
		err = rtos_get_semaphore(&at_wlan_cfg.sta_protection,AT_WLAN_STA_TIMEOUT_MS);
		if (err != kNoErr)
		{
			goto error;
		}
		else
		{
			atsvr_cmd_rsp_ok();
			rtos_deinit_semaphore(&at_wlan_cfg.sta_protection);
			return 0;			
		}
	}

error:
	atsvr_cmd_rsp_error();
	if (at_wlan_cfg.sta_protection != NULL) {
		rtos_deinit_semaphore(&at_wlan_cfg.sta_protection);
	}
	return err;
}


static int at_wlan_get_listen_interval(int sync,int argc, char **argv)
{
	u8 listen_interval = 0;

	if(argc != 0){
		atsvr_cmd_rsp_error();
		return -1;
	}

	if(bk_wifi_get_listen_interval(&listen_interval) == BK_OK){
		atsvr_cmd_rsp_ok();
		return 0;
	}else{
		atsvr_cmd_rsp_error();
		return -1;
	}
}
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
bool wifi_netif_sta_is_got_ip(void)
{
	wifi_linkstate_reason_t info = mhdr_get_station_status();
	return (info.state == WIFI_LINKSTATE_STA_GOT_IP);
}
/**
 * wifi_sec_type_string - Get the security name as a text string
 * @state: security
 * Returns: The security name as a printable text string
 */
const char *wifi_sec_type_string(wifi_security_t security)
{
	switch (security) {
	case WIFI_SECURITY_NONE:
		return "NONE";
	case WIFI_SECURITY_WEP:
		return "WEP";
	case WIFI_SECURITY_WPA_TKIP:
		return "WPA-TKIP";
	case WIFI_SECURITY_WPA_AES:
		return "WPA-AES";
	case WIFI_SECURITY_WPA_MIXED:
		return "WPA-MIX";
	case WIFI_SECURITY_WPA2_TKIP:
		return "WPA2-TKIP";
	case WIFI_SECURITY_WPA2_AES:
		return "WPA2-AES";
	case WIFI_SECURITY_WPA2_MIXED:
		return "WPA2-MIX";
	case WIFI_SECURITY_WPA3_SAE:
		return "WPA3-SAE";
	case WIFI_SECURITY_WPA3_WPA2_MIXED:
		return "WPA3-WPA2-MIX";
	case WIFI_SECURITY_EAP:
		return "EAP";
	case WIFI_SECURITY_OWE:
		return "OWE";
	case WIFI_SECURITY_AUTO:
		return "AUTO";
#ifdef CONFIG_WAPI_SUPPORT
	case WIFI_SECURITY_TYPE_WAPI_PSK:
		return "WAPI_PSK";
	case WIFI_SECURITY_TYPE_WAPI_CERT:
		return "WAPI_CERT";
#endif
	default:
		return "UNKNOWN";
	}
}


//bool ping_status;
extern void ping_stop(void);
extern void ping_start(char* target_name, uint32_t times, size_t size);
static int at_wlan_wifi_ping_start_cmd(int sync,int argc, char **argv)
{
	int err = kNoErr;
	uint32_t cnt = 4;
	uint32_t size = 0;
	if (argc == 0) {
		BK_LOGD(NULL, "Please input: ping <host address>\n");
		goto error;
	}
	
	if (argc > 1)
		cnt = os_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		size = os_strtoul(argv[2], NULL, 10);
	BK_LOGD(NULL, "ping IP address:%s\n", argv[0]);
	ping_start(argv[0], cnt, size);
	if(err == kNoErr) {
        atsvr_cmd_rsp_ok();
		return err;
	}
	else
		goto error;

error:
    atsvr_cmd_rsp_error();
	return err;
}
 
static int at_wlan_wifi_ping_stop_cmd(int sync,int argc, char **argv)
{
    if(argc!=0)
        goto error;

    ping_stop();

    atsvr_cmd_rsp_ok();
  
    return 0;
  
error:
    atsvr_cmd_rsp_error();
    return -1;
}

#if 0

static void at_wlan_get_station_mac_address(unsigned char *mac)
{
	bk_wifi_sta_get_mac((uint8_t*)mac);
}


static int at_wlan_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


static int at_wlan_hex2byte(const char *hex)
{
	int a, b;
	a = at_wlan_hex2num(*hex++);
	if (a < 0)
		return -1;
	b = at_wlan_hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}


int at_wlan_hexstr2bin(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = at_wlan_hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}


static int at_wlan_get_station_mac(int sync,int argc, char **argv)
{
	uint8_t base_mac[BK_MAC_ADDR_LEN] = {0};
	uint8_t sta_mac[BK_MAC_ADDR_LEN] = {0};
	uint8_t ap_mac[BK_MAC_ADDR_LEN] = {0};
	int     ret4 = 0;
	
	if (argc == 0) {
		bk_get_mac(base_mac, MAC_TYPE_BASE);
		bk_wifi_sta_get_mac(sta_mac);
		bk_wifi_ap_get_mac(ap_mac);
		BK_LOGD(NULL, "base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
		BK_LOGD(NULL, "sta mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(sta_mac));
		BK_LOGD(NULL, "ap mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(ap_mac));
		atsvr_cmd_rsp_ok();

	} else if (argc == 1) {
		at_wlan_hexstr2bin(argv[0], base_mac, BK_MAC_ADDR_LEN);
		ret4 = bk_set_base_mac(base_mac);
		BK_LOGD(NULL, "set base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
		if (ret4 != BK_OK)
			atsvr_cmd_rsp_error();
		else
			atsvr_cmd_rsp_ok();

	} else {
		BK_LOGD(NULL, "invalid cmd\r\n");
		atsvr_cmd_rsp_error();
	}
	return 0;

}

static void at_wlan_set_sta_dhcp(int en)
{
	at_wlan_stat.dhcp = ( en ) ? 1 : 0;
}





static int at_wlan_set_station_dhcp(int sync,int argc, char **argv)
{
    char *mode;
    unsigned int param;

    if(argc != 1){
		atsvr_cmd_rsp_error();
        return -1;
    }

    mode = argv[0];
    if(strcmp(mode,"ON DHCP") == 0){
        param = 1;
    }else if(strcmp(mode,"OFF DHCP") == 0){
       param = 0;
    }else if(strcmp(mode,"1") == 0){
        param = 1;
    }else if(strcmp(mode,"0") == 0){
       param = 0;
    }else{
        atsvr_cmd_rsp_error();
        return -1;
    }

    at_wlan_set_sta_dhcp(param);
    atsvr_cmd_rsp_ok();
	return 0;
}

static int at_wlan_get_station_dhcp(void)
{
	return ( at_wlan_stat.dhcp != 0 ) ? 1 : 0;
}

#endif




static int at_wlan_judge_the_string_is_ipv4_string(char *is_ip_string)
{
	int len = strlen(is_ip_string);

	if((len > 16 ) || (len < 7 )){
		return -1;
	}
	if(is_ip_string[(len - 1)] == '.'){
		return -1;
	}
	int num;
	int point_num = 0;
	char *p,*p_prev = is_ip_string;
	char ip_num[4];
	int i,j;

	while(1){
		p = strchr(p_prev, '.');
		if((p == NULL) && (point_num != 3)) {
			return -1;
		}
		point_num++;
		i = p - p_prev;
		if((i <= 3) && (i > 0)){
			for(j = 0; j < i; j++) {
				ip_num[j] = p_prev[j];
				if((ip_num[j] < '0') || (ip_num[j] > '9')){
					return -1;
				}
			}
			ip_num[j] = '\0';
			num = atoi( ip_num );
			if(num < 0 || num > 255){
				return -1;
			}else {
				p_prev = p + 1;
				if(point_num == 3) {
					p = strchr(p_prev, '.');
					if(p != NULL){
						return -1;
					}
					i = strlen(p_prev);
					if(i > 3 || i <= 0){
						return -1;
					}
					for(j = 0; j < i; j++) {
						ip_num[j] = p_prev[j];
						if((ip_num[j] < '0') || (ip_num[j] > '9')){
							return -1;
						}
					}
					ip_num[j] = '\0';
					num = atoi( ip_num );
					if(num < 0 || num > 255){
						return -1;
					}
					return 0;
				}
			}
		}else{
			return -1;
		}
	}

	return 0;
}

static int at_wlan_set_station_static_ip(char *ip,char *mask,char *gate,char *dns,char *dns2)
{
	strncpy((char *)at_wlan_stat.static_ip,ip,16);
    strncpy((char *)at_wlan_stat.static_maks,mask,16);
    strncpy((char *)at_wlan_stat.static_gate,gate,16);
	if( dns != NULL){
		strncpy((char *)at_wlan_stat.static_dns1,dns,16);
	}
	if( dns != NULL){
		strncpy((char *)at_wlan_stat.static_dns2,dns2,16);
	}
	return 0;
}


static void at_wlan_station_static_ip(int sync,int argc, char **argv)
{
    if((argc != 3)&&(argc != 4)){
		atsvr_cmd_rsp_error();
        return;
    }
	char *ip = argv[0];
	char *mask = argv[1];
	char *gate = argv[2];
	char *dns = NULL;
	char *dns2 = NULL;

	if(argc == 4){
		dns = argv[3];
	}
	if(at_wlan_judge_the_string_is_ipv4_string(ip) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]ip:%s error\r\n",ip);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(mask) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]mask:%s error\r\n",mask);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(gate) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]gate:%s error\r\n",gate);
        return;
	}
	if(dns != NULL){
		if(at_wlan_judge_the_string_is_ipv4_string(dns) != 0){
			atsvr_cmd_rsp_error();
			BK_LOGE(TAG,"[at_wlan]dns:%s error\r\n",dns);
	        return;
		}
	}
	if(dns2 != NULL){
		if(at_wlan_judge_the_string_is_ipv4_string(dns2) != 0){
			atsvr_cmd_rsp_error();
			BK_LOGE(TAG,"[at_wlan]dns2:%s error\r\n",dns2);
			return;
		}
	}

	if(at_wlan_set_station_static_ip(ip, mask,gate,dns,dns2) == 0){
		atsvr_cmd_rsp_ok();
	}else{
		atsvr_cmd_rsp_error();
	}

    return;
}

static int at_wlan_get_station_cur_status(void)
{
	return (at_wlan_stat.station_status == AT_WLAN_STATION_UP) ? 1 : 0;
}

static int at_wlan_get_station_status(int sync, int argc, char **argv)
{
	int err = kNoErr;
	char resultbuf[200];
	char* tag = NULL;
	
	bool is_sta_ipup = wifi_netif_sta_is_got_ip();
	bool is_ap_ipup = uap_ip_is_start();
	os_memset(resultbuf,0,200);
	if (argc == 0) 
	{
		wifi_link_status_t link_status = {0};
		wifi_ap_config_t ap_info = {0};
		netif_ip4_config_t ap_ip4_info = {0};
		char ssid[33] = {0};
#if CONFIG_WIFI4
		snprintf(resultbuf,sizeof(resultbuf), "sta: %d, ap: %d, b/g/n\r\n",is_sta_ipup,is_ap_ipup);
		atsvr_output_msg(resultbuf);	
#else
		snprintf(resultbuf,sizeof(resultbuf), "sta: %d, ap: %d, b/g/n\r\n",is_sta_ipup,is_ap_ipup);
		atsvr_output_msg(resultbuf);	
#endif
		//sta_ip_is_start()
		if (is_sta_ipup) {
			os_memset(&link_status, 0x0, sizeof(link_status));
			BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
			os_memcpy(ssid, link_status.ssid, 32);
			os_memset(resultbuf,0,200);
			snprintf(resultbuf,sizeof(resultbuf), "EVT:sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
													link_status.rssi, link_status.aid, ssid, link_status.bssid,
													link_status.channel, wifi_sec_type_string(link_status.security));
			atsvr_output_msg(resultbuf);	
		}
		if (is_ap_ipup) 
		{
			os_memset(&ap_info, 0x0, sizeof(ap_info));
			BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
			os_memcpy(ssid, ap_info.ssid, 32);
			//BK_LOGD(TAG, "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
			//		ssid, ap_info.channel, wifi_sec_type_string(ap_info.security));
			os_memset(resultbuf,0,200);
			snprintf(resultbuf,sizeof(resultbuf), "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
					ssid, ap_info.channel, wifi_sec_type_string(ap_info.security));
			atsvr_output_msg(resultbuf);
			//BK_RETURN_ON_ERR(bk_netif_get_ip4_config_api_temp(NETIF_IF_AP, &ap_ip4_info));

			BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info));
			os_memset(resultbuf,0,200);
			//BK_LOGD(TAG, "[KW:]ap_ip=%s,ap_gate=%s,ap_mask=%s,ap_dns=%s\r\n",
			//		ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
			snprintf(resultbuf,sizeof(resultbuf), "EVT:ap_ip=%s,ap_gate=%s,ap_mask=%s,ap_dns=%s\r\n",
					ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
			atsvr_output_msg(resultbuf);	
		}
		atsvr_cmd_rsp_ok();
	}
	else if (argc == 1) {
		if (os_strcmp(argv[0], "STA") == 0) {
			if (sta_ip_is_start()){
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","STA_WIFI_CONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","STA_WIFI_DISCONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();

			}
			return err;
		}

		else if (os_strcmp(argv[0], "AP") == 0) {
			if (is_ap_ipup){				
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_START");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();

			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_CLOSE");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
			}
			return err;
		}
	}
	else if (argc == 2) {
		if (os_strcmp(argv[0], "STA") == 0) {
			tag = "sta";
			if (sta_ip_is_start()) {
				wifi_link_status_t link_status = {0};
				os_memset(&link_status, 0x0, sizeof(link_status));
				err = bk_wifi_sta_get_link_status(&link_status);
				if(err != kNoErr) {
					err = kGeneralErr;
					goto error;
				}
				if (os_strcmp(argv[1], "RSSI") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%d\r\n","CMDRSP",tag,"rssi",link_status.rssi);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "CHANNEL") == 0) {
					snprintf(resultbuf,sizeof(resultbuf),"%s:%s_%s:%d\r\n","CMDRSP",tag,"chnl",link_status.channel);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "BSSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:" MACSTR "\r\n","CMDRSP",tag,"bssid",MAC2STR(link_status.bssid));
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "SSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s::%s\r\n","CMDRSP",tag,"ssid",link_status.ssid);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "IP") == 0) {
					netif_ip4_config_t sta_ip4_info = {0};
					//err = bk_netif_get_ip4_config(NETIF_IF_STA, &sta_ip4_info);
					err = bk_netif_get_ip4_config(NETIF_IF_STA, &sta_ip4_info);
					if(err != kNoErr) {
						BK_LOGD(NULL, "get ip fail!\n");
						err = kGeneralErr;
						goto error;
					}
					snprintf(resultbuf,sizeof(resultbuf), "%s:STA_IP=%s,GATE=%s,MASK=%s,DNS=%s\r\n","CMDRSP",sta_ip4_info.ip, sta_ip4_info.gateway, sta_ip4_info.mask, sta_ip4_info.dns);
					atsvr_output_msg(resultbuf);
				}

				else {
					BK_LOGD(NULL, "bad parameters\r\n");
					err = kParamErr;
					goto error;
				}
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP","STA_WIFI_DISCONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
				err = kGeneralErr;
			}
			return err;
		}
		else if (os_strcmp(argv[0], "AP") == 0) {
			if (is_ap_ipup) {
				tag = "ap";
				wifi_ap_config_t ap_info = {0};
				os_memset(&ap_info, 0x0, sizeof(ap_info));
				err = bk_wifi_ap_get_config(&ap_info);
				if(err != kNoErr) {
					BK_LOGD(NULL, "get ap link status fail!\n");
					err = kGeneralErr;
					goto error;
				}

				if (os_strcmp(argv[1], "SSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%s\r\n","CMDRSP",tag,"ssid",ap_info.ssid);
					atsvr_output_msg(resultbuf);

				}
				else if (os_strcmp(argv[1], "CHANNEL") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%d\r\n","CMDRSP",tag,"chnl",ap_info.channel);
					atsvr_output_msg(resultbuf);

				}
				else if (os_strcmp(argv[1], "SECURITY") == 0) {				
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%s\r\n","CMDRSP",tag,"security",wifi_sec_type_string(ap_info.security));
					atsvr_output_msg(resultbuf);

				}

				else if (os_strcmp(argv[1], "IP") == 0) {
					netif_ip4_config_t ap_ip4_info = {0};
					//err = bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info);
					err = bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info);
					if(err != kNoErr) {
						BK_LOGD(NULL, "get ip fail!\n");
						err = kGeneralErr;
						goto error;
					}
					snprintf(resultbuf,sizeof(resultbuf), "%s:AP_IP=%s,GATE=%s,MASK=%s,DNS=%s\r\n","CMDRSP",ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
					atsvr_output_msg(resultbuf);

				}
				else {
					BK_LOGD(NULL, "bad parameters\r\n");
					err = kParamErr;
					goto error;
				}
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_CLOSED");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
				err = kGeneralErr;
			}
			return err;
		}
		else {
			BK_LOGD(NULL, "bad parameters\r\n");
			err = kParamErr;
			goto error;
		}
	}
	else {
		BK_LOGD(NULL, "bad parameters\r\n");
		err = kParamErr;
		goto error;
	}
	return 0;
error:
	atsvr_cmd_rsp_error();
	return err;
}

#if 0
static void at_wlan_get_station_mac_address(unsigned char *mac)
{
	bk_wifi_sta_get_mac((uint8_t*)mac);
}
#endif

static int at_wlan_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int at_wlan_hex2byte(const char *hex)
{
	int a, b;
	a = at_wlan_hex2num(*hex++);
	if (a < 0)
		return -1;
	b = at_wlan_hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}


int at_wlan_hexstr2bin(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = at_wlan_hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

static int at_wlan_get_station_mac(int sync,int argc, char **argv)
{
	uint8_t base_mac[BK_MAC_ADDR_LEN] = {0};
	uint8_t sta_mac[BK_MAC_ADDR_LEN] = {0};
	uint8_t ap_mac[BK_MAC_ADDR_LEN] = {0};
	int ret = 0;
	char resultbuf[200];
	os_memset(resultbuf,0,200);
	
	if (argc == 0) {
		extern bk_err_t bk_wdrv_get_mac(uint8_t *mac, mac_type_t type);
		bk_wdrv_get_mac(base_mac, MAC_TYPE_BASE);
		bk_wifi_sta_get_mac(sta_mac);
		bk_wifi_ap_get_mac(ap_mac);
		#if 0
		BK_LOGD(NULL, "base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
		BK_LOGD(NULL, "sta mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(sta_mac));
		BK_LOGD(NULL, "ap mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(ap_mac));
		#else
		snprintf(resultbuf,sizeof(resultbuf), "base mac: "BK_MAC_FORMAT"\nsta mac: "BK_MAC_FORMAT"\nap mac: "BK_MAC_FORMAT"\n",
			BK_MAC_STR(base_mac),BK_MAC_STR(sta_mac),BK_MAC_STR(ap_mac));
		atsvr_output_msg(resultbuf);
		#endif
		atsvr_cmd_rsp_ok();

	}else if (argc == 1) {
		at_wlan_hexstr2bin(argv[0], base_mac, BK_MAC_ADDR_LEN);
		ret = bk_set_base_mac(base_mac);
		//BK_LOGD(NULL, "set base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
		snprintf(resultbuf,sizeof(resultbuf), "set base mac: "BK_MAC_FORMAT"\n",BK_MAC_STR(base_mac));
		atsvr_output_msg(resultbuf);
		if (ret != BK_OK)
			atsvr_cmd_rsp_error();
		else
			atsvr_cmd_rsp_ok();
	}else {
		BK_LOGD(NULL, "invalid cmd\r\n");
		atsvr_cmd_rsp_error();
	}
	return 0;

}

#if 0
static void at_wlan_set_sta_dhcp(int en)
{
	at_wlan_stat.dhcp = ( en ) ? 1 : 0;
}




static int at_wlan_set_station_dhcp(int sync,int argc, char **argv)
{
    char *mode;
    unsigned int param;

    if(argc != 1){
		atsvr_cmd_rsp_error();
        return -1;
    }

    mode = argv[0];
    if(strcmp(mode,"ON DHCP") == 0){
        param = 1;
    }else if(strcmp(mode,"OFF DHCP") == 0){
       param = 0;
    }else if(strcmp(mode,"1") == 0){
        param = 1;
    }else if(strcmp(mode,"0") == 0){
       param = 0;
    }else{
        atsvr_cmd_rsp_error();
        return -1;
    }

    at_wlan_set_sta_dhcp(param);
    atsvr_cmd_rsp_ok();
	return 0;
}

static int at_wlan_get_station_dhcp(void)
{
	return ( at_wlan_stat.dhcp != 0 ) ? 1 : 0;
}







static int at_wlan_judge_the_string_is_ipv4_string(char *is_ip_string)
{
	int len = strlen(is_ip_string);

	if((len > 16 ) || (len < 7 )){
		return -1;
	}
	if(is_ip_string[(len - 1)] == '.'){
		return -1;
	}
	int num;
	int point_num = 0;
	char *p,*p_prev = is_ip_string;
	char ip_num[4];
	int i,j;

	while(1){
		p = strchr(p_prev, '.');
		if((p == NULL) && (point_num != 3)) {
			return -1;
		}
		point_num++;
		i = p - p_prev;
		if((i <= 3) && (i > 0)){
			for(j = 0; j < i; j++) {
				ip_num[j] = p_prev[j];
				if((ip_num[j] < '0') || (ip_num[j] > '9')){
					return -1;
				}
			}
			ip_num[j] = '\0';
			num = atoi( ip_num );
			if(num < 0 || num > 255){
				return -1;
			}else {
				p_prev = p + 1;
				if(point_num == 3) {
					p = strchr(p_prev, '.');
					if(p != NULL){
						return -1;
					}
					i = strlen(p_prev);
					if(i > 3 || i <= 0){
						return -1;
					}
					for(j = 0; j < i; j++) {
						ip_num[j] = p_prev[j];
						if((ip_num[j] < '0') || (ip_num[j] > '9')){
							return -1;
						}
					}
					ip_num[j] = '\0';
					num = atoi( ip_num );
					if(num < 0 || num > 255){
						return -1;
					}
					return 0;
				}
			}
		}else{
			return -1;
		}
	}

	return 0;
}

static int at_wlan_set_station_static_ip(char *ip,char *mask,char *gate,char *dns,char *dns2)
{
	strncpy((char *)at_wlan_stat.static_ip,ip,16);
    strncpy((char *)at_wlan_stat.static_maks,mask,16);
    strncpy((char *)at_wlan_stat.static_gate,gate,16);
	if( dns != NULL){
		strncpy((char *)at_wlan_stat.static_dns1,dns,16);
	}
	if( dns != NULL){
		strncpy((char *)at_wlan_stat.static_dns2,dns2,16);
	}
	return 0;
}


static void at_wlan_station_static_ip(int sync,int argc, char **argv)
{
    if((argc != 3)&&(argc != 4)){
		atsvr_cmd_rsp_error();
        return;
    }
	char *ip = argv[0];
	char *mask = argv[1];
	char *gate = argv[2];
	char *dns = NULL;
	char *dns2 = NULL;

	if(argc == 4){
		dns = argv[3];
	}
	if(at_wlan_judge_the_string_is_ipv4_string(ip) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]ip:%s error\r\n",ip);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(mask) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]mask:%s error\r\n",mask);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(gate) != 0){
		atsvr_cmd_rsp_error();
		BK_LOGE(TAG,"[at_wlan]gate:%s error\r\n",gate);
        return;
	}
	if(dns != NULL){
		if(at_wlan_judge_the_string_is_ipv4_string(dns) != 0){
			atsvr_cmd_rsp_error();
			BK_LOGE(TAG,"[at_wlan]dns:%s error\r\n",dns);
	        return;
		}
	}
	if(dns2 != NULL){
		if(at_wlan_judge_the_string_is_ipv4_string(dns2) != 0){
			atsvr_cmd_rsp_error();
			BK_LOGE(TAG,"[at_wlan]dns2:%s error\r\n",dns2);
			return;
		}
	}

	if(at_wlan_set_station_static_ip(ip, mask,gate,dns,dns2) == 0){
		atsvr_cmd_rsp_ok();
	}else{
		atsvr_cmd_rsp_error();
	}

    return;
}

static int at_wlan_get_station_cur_status(void)
{
	return (at_wlan_stat.station_status == AT_WLAN_STATION_UP) ? 1 : 0;
}

#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
static int at_wlan_get_station_status(int sync, int argc, char **argv)
{
	int err = kNoErr;
	char resultbuf[200];
	char* tag = NULL;
	if (argc == 0) 
	{

	wifi_link_status_t link_status = {0};
	wifi_ap_config_t ap_info = {0};
	netif_ip4_config_t ap_ip4_info = {0};
	char ssid[33] = {0};
#if CONFIG_WIFI4
	snprintf(resultbuf,sizeof(resultbuf), "sta: %d, ap: %d, b/g/n\r\n",wifi_netif_sta_is_got_ip(),uap_ip_is_start());
	atsvr_output_msg(resultbuf);	
#else
	snprintf(resultbuf,sizeof(resultbuf), "sta: %d, ap: %d, b/g/n\r\n",wifi_netif_sta_is_got_ip(),uap_ip_is_start());
	atsvr_output_msg(resultbuf);	
#endif
	if (sta_ip_is_start()) {
		os_memset(&link_status, 0x0, sizeof(link_status));
		BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
		os_memcpy(ssid, link_status.ssid, 32);
		os_memset(resultbuf,0,200);
		snprintf(resultbuf,sizeof(resultbuf), "EVT:sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
												link_status.rssi, link_status.aid, ssid, link_status.bssid,
												link_status.channel, wifi_sec_type_string(link_status.security));
		atsvr_output_msg(resultbuf);	
	}
	if (uap_ip_is_start()) 
	{
		os_memset(&ap_info, 0x0, sizeof(ap_info));
		BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
		os_memcpy(ssid, ap_info.ssid, 32);
		BK_LOGD(TAG, "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
				ssid, ap_info.channel, wifi_sec_type_string(ap_info.security));

		BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info));
		os_memset(resultbuf,0,200);
		//BK_LOGD(TAG, "[KW:]ap_ip=%s,ap_gate=%s,ap_mask=%s,ap_dns=%s\r\n",
		//		ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
		snprintf(resultbuf,sizeof(resultbuf), "EVT:ap_ip=%s,ap_gate=%s,ap_mask=%s,ap_dns=%s\r\n",
				ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
		atsvr_output_msg(resultbuf);	
	}
		atsvr_cmd_rsp_ok();
	}
	else if (argc == 1) {
		if (os_strcmp(argv[0], "STA") == 0) {
			if (sta_ip_is_start()){
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","STA_WIFI_CONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","STA_WIFI_DISCONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();

			}
			return err;
		}

		else if (os_strcmp(argv[0], "AP") == 0) {
			if (uap_ip_is_start()){				
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_START");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();

			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_CLOSE");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
			}
			return err;
		}
	}
	else if (argc == 2) {
		if (os_strcmp(argv[0], "STA") == 0) {
			tag = "sta";
			if (sta_ip_is_start()) {
				wifi_link_status_t link_status = {0};
				os_memset(&link_status, 0x0, sizeof(link_status));
				err = bk_wifi_sta_get_link_status(&link_status);
				if(err != kNoErr) {
					err = kGeneralErr;
					goto error;
				}
				if (os_strcmp(argv[1], "RSSI") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%d\r\n","CMDRSP",tag,"rssi",link_status.rssi);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "CHANNEL") == 0) {
					snprintf(resultbuf,sizeof(resultbuf),"%s:%s_%s:%d\r\n","CMDRSP",tag,"chnl",link_status.channel);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "BSSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:" MACSTR "\r\n","CMDRSP",tag,"bssid",MAC2STR(link_status.bssid));
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "SSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s::%s\r\n","CMDRSP",tag,"ssid",link_status.ssid);
					atsvr_output_msg(resultbuf);
				}

				else if (os_strcmp(argv[1], "IP") == 0) {
					netif_ip4_config_t sta_ip4_info = {0};
					err = bk_netif_get_ip4_config(NETIF_IF_STA, &sta_ip4_info);
					if(err != kNoErr) {
						BK_LOGD(NULL, "get ip fail!\n");
						err = kGeneralErr;
						goto error;
					}
					snprintf(resultbuf,sizeof(resultbuf), "%s:STA_IP=%s,GATE=%s,MASK=%s,DNS=%s\r\n","CMDRSP",sta_ip4_info.ip, sta_ip4_info.gateway, sta_ip4_info.mask, sta_ip4_info.dns);
					atsvr_output_msg(resultbuf);
				}

				else {
					BK_LOGD(NULL, "bad parameters\r\n");
					err = kParamErr;
					goto error;
				}
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP","STA_WIFI_DISCONNECT");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
				err = kGeneralErr;
			}
			return err;
		}
		else if (os_strcmp(argv[0], "AP") == 0) {
			if (uap_ip_is_start()) {
				tag = "ap";
				wifi_ap_config_t ap_info = {0};
				os_memset(&ap_info, 0x0, sizeof(ap_info));
				err = bk_wifi_ap_get_config(&ap_info);
				if(err != kNoErr) {
					BK_LOGD(NULL, "get ap link status fail!\n");
					err = kGeneralErr;
					goto error;
				}

				if (os_strcmp(argv[1], "SSID") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%s\r\n","CMDRSP",tag,"ssid",ap_info.ssid);
					atsvr_output_msg(resultbuf);

				}
				else if (os_strcmp(argv[1], "CHANNEL") == 0) {
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%d\r\n","CMDRSP",tag,"chnl",ap_info.channel);
					atsvr_output_msg(resultbuf);

				}
				else if (os_strcmp(argv[1], "SECURITY") == 0) {				
					snprintf(resultbuf,sizeof(resultbuf), "%s:%s_%s:%s\r\n","CMDRSP",tag,"security",wifi_sec_type_string(ap_info.security));
					atsvr_output_msg(resultbuf);

				}

				else if (os_strcmp(argv[1], "IP") == 0) {
					netif_ip4_config_t ap_ip4_info = {0};
					err = bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info);
					if(err != kNoErr) {
						BK_LOGD(NULL, "get ip fail!\n");
						err = kGeneralErr;
						goto error;
					}
					snprintf(resultbuf,sizeof(resultbuf), "%s:AP_IP=%s,GATE=%s,MASK=%s,DNS=%s\r\n","CMDRSP",ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
					atsvr_output_msg(resultbuf);

				}
				else {
					BK_LOGD(NULL, "bad parameters\r\n");
					err = kParamErr;
					goto error;
				}
				atsvr_cmd_rsp_ok();
			}
			else {
				snprintf(resultbuf,sizeof(resultbuf), "%s:%s\r\n","CMDRSP:","AP_WIFI_CLOSED");
				atsvr_output_msg(resultbuf);
				atsvr_cmd_rsp_ok();
				err = kGeneralErr;
			}
			return err;
		}
		else {
			BK_LOGD(NULL, "bad parameters\r\n");
			err = kParamErr;
			goto error;
		}
	}
	else {
		BK_LOGD(NULL, "bad parameters\r\n");
		err = kParamErr;
		goto error;
	}
	return 0;
error:
	atsvr_cmd_rsp_error();
	return err;
}



static int at_wlan_get_listen_interval(int sync,int argc, char **argv)
{
	u8 listen_interval = 0;

	if(argc != 0){
		atsvr_cmd_rsp_error();
		return -1;
	}

	if(bk_wifi_get_listen_interval(&listen_interval) == BK_OK){
		BK_LOGD(TAG,"listen interval is %d\r\n",listen_interval);
		atsvr_cmd_rsp_ok();
		return 0;
	}else{
		atsvr_cmd_rsp_error();
		return -1;
	}
}

static int at_wlan_get_support_mode(int sync,int argc, char **argv)
{
	u8 support_mode = 0;

	if(argc != 0){
		atsvr_cmd_rsp_error();
		return -1;
	}

	if(bk_wifi_get_support_wifi_mode(&support_mode) == BK_OK){
		BK_LOGD(TAG,"support mode is %d\r\n",support_mode);
		atsvr_cmd_rsp_ok();
		return 0;
	}else{
		atsvr_cmd_rsp_error();
		return -1;
	}

}
#endif

#include "conv_utf8_pub.h"
static int at_wlan_softap_start(int sync, int argc, char **argv)
{
	int err = kNoErr;
	char *ap_ssid = NULL;
	char *ap_key = "";
	char *ap_channel = NULL;

	if (argc == 1)
		ap_ssid = argv[0];
	else if (argc == 2) {
		ap_ssid = argv[0];
		if (os_strlen(argv[1]) <= 2)
			ap_channel = argv[1];
		else
			ap_key = argv[1];
	}
	else if (argc == 3) {
		ap_ssid = argv[0];
		ap_key = argv[1];
		ap_channel = argv[2];
	}
	else {
		BK_LOGD(NULL, "input param error\n");
		err = kParamErr;
		goto error;
	}
	char *oob_ssid_softap = ap_ssid;
#if CONFIG_USE_CONV_UTF8
	oob_ssid_softap = (char *)conv_utf8((uint8_t *)ap_ssid);
#endif
	if (oob_ssid_softap) {
		extern int demo_softap_app_init(char *ap_ssid, char *ap_key, char *ap_channel);
		err = demo_softap_app_init((char *)oob_ssid_softap, ap_key, ap_channel);
#if CONFIG_USE_CONV_UTF8
		os_free(oob_ssid_softap);
#endif
	}
	if (err == kNoErr) {
		atsvr_cmd_rsp_ok();
		return err;
	}
	else {
		goto error;
	}

error:
	atsvr_cmd_rsp_error();

	return err;
}

static int at_wlan_hidden_softap_start(int sync, int argc, char **argv)
{
	int err = kNoErr;
	char *ap_ssid = NULL;
	char *ap_key = "";
	char *ap_channel = NULL;

	if (argc == 1)
		ap_ssid = argv[0];
	else if (argc == 2) {
		ap_ssid = argv[0];
		if (os_strlen(argv[1]) <= 2)
			ap_channel = argv[1];
		else
			ap_key = argv[1];
	}
	else if (argc == 3) {
		ap_ssid = argv[0];
		ap_key = argv[1];
		ap_channel = argv[2];
	}
	else {
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};
	int len;

	if (ap_ssid) {
		len = os_strlen(ap_ssid);
		if (32 < len) {
			os_printf("ssid name more than 32 Bytes\r\n");
			err = kParamErr;
			goto error;
		}

		os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
		os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
		os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
		os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
		if(kNoErr != bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config)) {
			err = kParamErr;
			goto error;
		}

		os_strcpy(ap_config.ssid, ap_ssid);
		os_strcpy(ap_config.password, ap_key);

		if (ap_channel) {
			int channel;
			char *end;

			channel = strtol(ap_channel, &end, 0);
			if (*end) {
				os_printf("Invalid number '%s'", ap_channel);
				err = kParamErr;
				goto error;
			}
			ap_config.channel = channel;
		}

		os_printf("ssid:%s key:%s\r\n", ap_config.ssid, ap_config.password);
		ap_config.hidden = true;
		if(kNoErr != bk_wifi_ap_set_config(&ap_config)) {
			err = kParamErr;
			goto error;
		}

		if(kNoErr != bk_wifi_ap_start()) {
			err = kParamErr;
			goto error;
		}
	}


	if (err == kNoErr) {
		atsvr_cmd_rsp_ok();

		return err;
	}
	else {
		goto error;
	}

error:
	atsvr_cmd_rsp_error();

	return err;
}

static int at_wlan_stop_softap(void)
{
	BK_LOG_ON_ERR(bk_wifi_ap_stop());
	return 0;
}


static int at_wlan_softap_stop(int sync,int argc, char **argv)
{
    if(argc != 0){
		atsvr_cmd_rsp_error();
		return -1;
    }

	if(at_wlan_stop_softap() == 0){
		//set_atsvr_work_state(ATSVR_WK_DONE);
		atsvr_cmd_rsp_ok();
		return 0;
	}else{
		//set_atsvr_work_state(ATSVR_WK_DONE);
		atsvr_cmd_rsp_error();
		return -1;
	}
}

#if 0
static int at_wlan_set_softap_static_ip(char *ip,char *mask,char *gate,char *dns_ip)
{
	strncpy((char *)at_wlan_stat.softap_ip,ip,16);
    strncpy((char *)at_wlan_stat.softap_mask,mask,16);
    strncpy((char *)at_wlan_stat.softap_gate,gate,16);
	if(dns_ip  != NULL){
		strncpy((char *)at_wlan_stat.softap_dns,dns_ip,16);
	}
	return 0;
}


static void at_wlan_softap_static_ip(int sync,int argc, char **argv)
{
    if((argc != 3)&&(argc != 4)){
		atsvr_cmd_rsp_error();
        return;
    }
	char *ip = argv[0];
	char *mask = argv[1];
	char *gate = argv[2];
	char *dns = NULL;

	if(argc == 4){
		dns = argv[3];
	}
	if(at_wlan_judge_the_string_is_ipv4_string(ip) != 0){
		atsvr_cmd_rsp_error();
		ATSVRLOG("[atsvr]ip:%s error\r\n",ip);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(mask) != 0){
		atsvr_cmd_rsp_error();
		ATSVRLOG("[atsvr]mask:%s error\r\n",mask);
        return;
	}
	if(at_wlan_judge_the_string_is_ipv4_string(gate) != 0){
		atsvr_cmd_rsp_error();
		ATSVRLOG("[atsvr]gate:%s error\r\n",gate);
        return;
	}
	if(dns != NULL){
		if(at_wlan_judge_the_string_is_ipv4_string(dns) != 0){
			atsvr_cmd_rsp_error();
			ATSVRLOG("[atsvr]dns:%s error\r\n",dns);
	        return;
		}
	}
	if(at_wlan_set_softap_static_ip(ip, mask,gate,dns) == 0){
		atsvr_cmd_rsp_ok();
	}else{
		atsvr_cmd_rsp_error();
	}

    return;
}


static int at_wlan_get_softap_cur_status(void)
{
	return (at_wlan_stat.softap_status == AT_WLAN_SOFTAP_UP) ? 1 : 0;
}


static int at_wlan_get_softap_status(int sync,int argc, char **argv)
{
    if(0!=strcmp("?",argv[0])){
		atsvr_cmd_rsp_error();
        return -1;
    }

	int statue = at_wlan_get_softap_cur_status();
	if(statue == 1){
		ATSVR_SIZEOF_OUTPUT_STRRING(ATSVR_CMDRSP_HEAD"SOFTAP_UP\r\n");
	}else{
		ATSVR_SIZEOF_OUTPUT_STRRING(ATSVR_CMDRSP_HEAD"SOFTAP_DOWN\r\n");
	}
	//set_atsvr_work_state(ATSVR_WK_DONE);
    atsvr_cmd_rsp_ok();
	return 0;
}


bool ping_status;
void ping_stop(void);
void ping_start(char* target_name, uint32_t times, size_t size);
static int at_wlan_wifi_ping_start_cmd(int sync,int argc, char **argv)
{
	int err = kNoErr;
	uint32_t cnt = 4;
	uint32_t size = 0;
	if (argc == 0) {
		BK_LOGD(NULL, "Please input: ping <host address>\n");
		goto error;
	}
	
	if (argc > 1)
		cnt = os_strtoul(argv[1], NULL, 10);
	if (argc > 2)
		size = os_strtoul(argv[2], NULL, 10);
	BK_LOGD(NULL, "ping IP address:%s\n", argv[0]);
	ping_start(argv[0], cnt, size);
	if(err == kNoErr) {
        atsvr_cmd_rsp_ok();
		return err;
	}
	else
		goto error;

error:
    atsvr_cmd_rsp_error();
	return err;
}

static int at_wlan_wifi_ping_stop_cmd(int sync,int argc, char **argv)
{
    if(argc!=0)
        goto error;

    ping_stop();

    atsvr_cmd_rsp_ok();
  
    return 0;
  
error:
    atsvr_cmd_rsp_error();
    return -1;
}
#endif

static int at_wlan_wifi_close_coex_csa_cmd(int sync,int argc, char **argv)
{
	uint8_t close_csa;
	int err = kNoErr;

	if (argc != 1) {
		BK_LOGD(NULL, "invalid close_coex_csa command\n");
		goto error;
	}

	close_csa = (uint8_t)os_strtoul(argv[0], NULL, 10);
	if ((close_csa == 1) || (close_csa == 0))
	{
		extern bk_err_t bk_wifi_set_csa_coexist_mode_flag(bool is_close);
		err = bk_wifi_set_csa_coexist_mode_flag(!!close_csa);
		if(err == kNoErr) 
		{
			atsvr_cmd_rsp_ok();
			return err;
		}
	}
	else
	{
		BK_LOGD(NULL, "bad parameters %d\r\n",close_csa);
	}

error:
	atsvr_cmd_rsp_error();
	return err;
}

#if 0
#if CONFIG_SOC_BK723L
#include "dns.h"
#include "inet.h"
static int at_wlan_atw_cmd(int sync,int argc, char **argv)
{
	int err = kNoErr;
	char resultbuf[200];
	wifi_link_status_t link_status = {0};
	wifi_ap_config_t ap_info = {0};
	netif_ip4_config_t sta_ip4_info = {0};
	netif_ip4_config_t ap_ip4_info = {0};
	int n = 0;
	bool bwlan0,bwlan1 = false;
	uint8_t sta_mac[BK_MAC_ADDR_LEN] = {0};
	uint8_t ap_mac[BK_MAC_ADDR_LEN] = {0};
	struct wlan_ip_config sta_addr_info = {0};

#if CONFIG_WIFI4

	bwlan0 = wifi_netif_sta_is_got_ip();

	if(0!= uap_ip_is_start())
		bwlan1 = true;

	n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "WIFI wlan0 Status : %s\r\n==============================\r\n",(bwlan0==true) ? "Running":"Stopped");


	atsvr_output_msg(resultbuf);
	os_memset(resultbuf,0,200);
	n = 0;
	if(bwlan0)
	{
		BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nWIFI wlan0 Setting: \r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "===================================\r\n\r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nMODE=>Station\r\nSSID=>%s\r\nCHANNEL=>%d\r\nSECURITY=>%s\r\n",
														link_status.ssid,link_status.channel, wifi_sec_type_string(link_status.security));
		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;
		
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "PASSWORD=>%s\r\n",link_status.password);											
		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;		
	
		bk_wifi_sta_get_mac(sta_mac);	
		err = bk_netif_get_ip4_config(NETIF_IF_STA, &sta_ip4_info);	
		if(err != kNoErr) {
			BK_LOGD(NULL, "get ip fail!\n");
			err = kGeneralErr;
			goto error;
		}

		const ip_addr_t *tmp;

		tmp = dns_getserver(1);
		if(!tmp)
			sta_addr_info.ipv4.dns2 = ip_addr_get_ip4_u32(tmp);

		net_get_if_addr(&sta_addr_info, net_get_sta_handle());
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nInterface(wlan0): \r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "===================================\r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nMAC=>%02x:%02x:%02x:%02x:%02x:%02x\r\nIP=>%s\r\nGW=>%s\r\nmask=>%s\r\n",
														BK_MAC_STR(sta_mac),sta_ip4_info.ip, sta_ip4_info.gateway,sta_ip4_info.mask);
		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;	

		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "DNS1=>%s\r\nDNS2=>%s\r\n\r\n",
														sta_ip4_info.dns,(tmp==NULL)?"0.0.0.0":inet_ntoa(sta_addr_info.ipv4.dns2));

		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;

	}

	n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nWIFI wlan1 Status : %s\r\n",(bwlan1==true) ? "Running":"Stopped");
	n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "==============================\r\n");
	atsvr_output_msg(resultbuf);
	os_memset(resultbuf,0,200);
	n = 0;
	if(bwlan1)
	{

		BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nWIFI wlan1 Setting: \r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "===================================\r\n\r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nMODE=>SoftAP\r\nSSID=>%s\r\nCHANNEL=>%d\r\nSECURITY=>%s\r\n",
														link_status.ssid,link_status.channel, wifi_sec_type_string(link_status.security));
		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;	
	
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "PASSWORD=>%s\r\n",ap_info.password);											
		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;		

		bk_wifi_ap_get_mac(ap_mac);	
		err = bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info);
		if(err != kNoErr) {
			BK_LOGD(NULL, "get ip fail!\n");
			err = kGeneralErr;
			goto error;
		}

		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nInterface(wlan1): \r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "===================================\r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nMAC=>%02x:%02x:%02x:%02x:%02x:%02x\r\nIP=>%s\r\nGW=>%s\r\nmask=>%s\r\n\n\n",
														BK_MAC_STR(ap_mac),ap_ip4_info.ip, ap_ip4_info.gateway,ap_ip4_info.mask);

		atsvr_output_msg(resultbuf);
		os_memset(resultbuf,0,200);
		n = 0;	

		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nAssociated Client List: \r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "======================================================\r\n");
		n = snprintf(resultbuf+n,sizeof(resultbuf) - n, "\r\nClient Num:%d\r\n\n\n",at_wlan_stat.hpad_sta_cnt);
		
		atsvr_output_msg(resultbuf);
	}
	atsvr_cmd_rsp_ok();	
	return 0;
#endif	
error:
	atsvr_cmd_rsp_error();
	return err;
}
#endif
#endif

const struct _atsvr_command wifi_cmds_table[] = {

/*STA*/
	ATSVR_CMD_HADLER("AT+WLMAC","AT+WLMAC",
					NULL,at_wlan_get_station_mac,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+WIFISCAN","AT+WIFISCAN=SET_SSID,SSID,TYPE,DURATION,CNT,NUMBER",at_wlan_scan_cmd_query,at_wlan_scan_cmd,
					false,AT_WLAN_SCAN_TIMEOUT_MS,true,NULL,false),
	ATSVR_CMD_HADLER("AT+STASTOP","AT+STASTOP",
					NULL,at_wlan_station_stop,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+STASTART","AT+STASTART=SSID,PWD",
					NULL,at_wlan_station_start,false,AT_WLAN_STA_TIMEOUT_MS,true,NULL,false),
	ATSVR_CMD_HADLER("AT+GETINTERVAL","AT+GETINTERVAL",NULL,
					at_wlan_get_listen_interval,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+WIFISTATUS","AT+WIFISTATUS",NULL,
						at_wlan_get_station_status,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+STAPING","AT+STAPING=ip;",
						NULL,at_wlan_wifi_ping_start_cmd,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+STAPINGSTOP","STOP:AT+STAPINGSTOP",
					NULL,at_wlan_wifi_ping_stop_cmd,false,0,0,NULL,false),

#if 0

	ATSVR_CMD_HADLER("AT+SETSTADHCP","AT+SETSTADHCP=0/1",
					NULL,at_wlan_set_station_dhcp,false,0,0,NULL,false),


	//ATSVR_CMD_HADLER("AT+STASTATIC","AT+STASTATIC=ip,mask,gate[,dns]",NULL,at_wlan_station_static_ip,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+WIFISTATUS","AT+WIFISTATUS",NULL,
					at_wlan_get_station_status,false,0,0,NULL,false),
#endif
	ATSVR_CMD_HADLER("AT+GETINTERVAL","AT+GETINTERVAL",NULL,
					at_wlan_get_listen_interval,false,0,0,NULL,false),
#if 0
	ATSVR_CMD_HADLER("AT+GETMODE","AT+GETMODE",NULL,
					at_wlan_get_support_mode,false,0,0,NULL,false),
#if CONFIG_SOC_BK723L
	ATSVR_CMD_HADLER("ATW?","Get Station and SoftAP info",NULL,
					at_wlan_atw_cmd,false,0,0,NULL,false),
#endif
#endif
	/*SAP*/
	ATSVR_CMD_HADLER("AT+SAPSTART","AT+SAPSTART=SSID,PWD",
					NULL,at_wlan_softap_start,false,AT_WLAN_SAP_TIMEOUT_MS,true,NULL,true),
	ATSVR_CMD_HADLER("AT+SAPSTOP","AT+SAPSTOP",
					NULL,at_wlan_softap_stop,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+HIDDENSAP","AT+HIDDENSAP=SSID,PWD",
					NULL,at_wlan_hidden_softap_start,false,AT_WLAN_SAP_TIMEOUT_MS,true,NULL,true),

#if 0
	//ATSVR_CMD_HADLER("AT+SAPSTATIC","AT+SAPSTATIC=ip,mask,gate[,dns]",NULL,at_wlan_softap_static_ip,false,0,0,NULL,false),
	//ATSVR_CMD_HADLER("AT+SAPSTATUS","AT+SAPSTATUS",at_wlan_get_softap_status,
	//				NULL,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+STAPING","START:AT+STAPING=ip;STOP:AT+PING=STOP",
					NULL,at_wlan_wifi_ping_start_cmd,false,0,0,NULL,false),
	

#endif
	ATSVR_CMD_HADLER("AT+CLOSECSA","AT+CLOSECSA=0/1",
					NULL,at_wlan_wifi_close_coex_csa_cmd,false,0,0,NULL,false),

};


void wifi_at_cmd_init(void)
{
	int ret;
	ret = atsvr_register_commands(wifi_cmds_table, sizeof(wifi_cmds_table) / sizeof(wifi_cmds_table[0]),"wifi",at_wlan_event_handler);
	if(0 == ret)
		BK_LOGD(NULL, "WIFI AT CMDS INIT OK\r\n");
	wlan_at_init();
}


