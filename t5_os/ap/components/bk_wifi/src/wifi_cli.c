#include "conv_utf8_pub.h"
#include "wdrv_main.h"
#include "wdrv_cntrl.h"
#include "wdrv_ipc.h"
#include "wifi_api.h"
#include "pbuf.h"
#include <components/netif.h>
#include <components/event.h>
#include "net.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include <components/netif.h>
#include "lwip/ping.h"
#include "wifi_demo.h"
#include "ftp/ftpd.h"
/**
 * @brief default AP configuration
 * */
#define WIFI_DEFAULT_AP_CONFIG() {\
    .ssid = "ap_default_ssid",\
    .password = "",\
    .channel = 0,\
    .security = WIFI_SECURITY_WPA2_MIXED,\
    .hidden = 0,\
    .max_con = 0,\
    .reserved = {0},\
    }

#if  1 //def CONFIG_WIFI_VNET_CONTROLLER
void wdrv_demo_connect(char *oob_ssid, char *connect_key)
{
    wifi_sta_config_t sta_config = {0};
    int len;

    len = os_strlen(oob_ssid);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGD("ssid name more than 32 Bytes\r\n");
        return;
    }
    os_strcpy(sta_config.ssid, oob_ssid);
    if (connect_key)
        os_strcpy(sta_config.password, connect_key);

    WDRV_LOGD("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());
}

int wdrv_demo_softap_init(char *ap_ssid, char *ap_key, char *ap_channel)
{
    wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
#if 0
    netif_ip4_config_t ip4_config = {0};
#endif
    int len, key_len = 0;
    len = os_strlen(ap_ssid);
    if (ap_key)
        key_len = os_strlen(ap_key);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGE("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
    if (0 == len) {
        WDRV_LOGE("ssid name must not be null\r\n");
        return BK_FAIL;
    }

    if (8 > key_len)
        WDRV_LOGE("key less than 8 Bytes, the security will be set NONE\r\n");

    if (64 < key_len) {
        WDRV_LOGE("key more than 64 Bytes\r\n");
        return BK_FAIL;
    }
#if 0
    os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
    os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
    os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
    BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));
#endif

    os_strcpy(ap_config.ssid, ap_ssid);
    if (ap_key)
        os_strcpy(ap_config.password, ap_key);

    if (ap_channel) {
        int channel;
        char *end;

        channel = strtol(ap_channel, &end, 0);
        if (*end) {
            WDRV_LOGE("Invalid number '%s'", ap_channel);
            return BK_FAIL;
        }
        ap_config.channel = channel;
    }

    //bk_wifi_ap_init();
    WDRV_LOGD("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_RETURN_ON_ERR(bk_wifi_ap_start());

    return BK_OK;
}

int wdrv_demo_hidden_softap_init(char *ap_ssid, char *ap_key, char *ap_channel)
{
    wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
    netif_ip4_config_t ip4_config = {0};
    int len, key_len = 0;
    len = os_strlen(ap_ssid);
    if (ap_key)
        key_len = os_strlen(ap_key);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGE("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
    if (0 == len) {
        WDRV_LOGE("ssid name must not be null\r\n");
        return BK_FAIL;
    }

    if (8 > key_len)
        WDRV_LOGE("key less than 8 Bytes, the security will be set NONE\r\n");

    if (64 < key_len) {
        WDRV_LOGE("key more than 64 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
    os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
    os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
    BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));

    os_strcpy(ap_config.ssid, ap_ssid);
    if (ap_key)
        os_strcpy(ap_config.password, ap_key);

    if (ap_channel) {
        int channel;
        char *end;

        channel = strtol(ap_channel, &end, 0);
        if (*end) {
            WDRV_LOGE("Invalid number '%s'", ap_channel);
            return BK_FAIL;
        }
        ap_config.channel = channel;
    }

    WDRV_LOGD("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    ap_config.hidden = true;
    BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_RETURN_ON_ERR(bk_wifi_ap_start());

    return BK_OK;
}

static int wlan_scan_done_handler(void *arg, event_module_t event_module,
                                         int event_id, void *event_data)
{
    wifi_scan_result_t scan_result = {0};

    BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
    BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
    bk_wifi_scan_free_result(&scan_result);

    return BK_OK;
}

void demo_scan_adv_app_init(uint8_t *oob_ssid)
{
    wifi_scan_config_t scan_config = {0};

    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
                         wlan_scan_done_handler, NULL);

    if (oob_ssid) {
        os_strncpy(scan_config.ssid, (char *)oob_ssid, WIFI_SSID_STR_LEN);
        BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));
    } else
        BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
}

static const char *wdr_ifname[NETIF_IF_COUNT] = {
    "sta", "ap",
};

static inline const char *wdr_if_idx_name(netif_if_t ifx)
{
    if (ifx > NETIF_IF_COUNT)
        return "unkown";
    return wdr_ifname[ifx];
}

#define WDRV_CLI_DUMP_IP(_prompt, _ifx, _cfg) do {\
    WDRV_LOGD("%s wdrv_netif(%s) wdrv_ip4=%s wdrv_mask=%s wdrv_gate=%s wdrv_dns=%s\n", (_prompt),\
                wdr_if_idx_name(_ifx),\
                (_cfg)->ip, (_cfg)->mask, (_cfg)->gateway, (_cfg)->dns);\
} while(0)

void wdrv_ip_cmd_show_ip(int ifx)
{
    netif_ip4_config_t config;

    if (ifx == NETIF_IF_STA || ifx == NETIF_IF_AP || ifx == NETIF_IF_ETH) {
        BK_LOG_ON_ERR(bk_netif_get_ip4_config(ifx, &config));
        WDRV_CLI_DUMP_IP(" ", ifx, &config);
    } else {
        BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &config));
        WDRV_CLI_DUMP_IP(" ", NETIF_IF_STA, &config);
        BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &config));
        WDRV_CLI_DUMP_IP(" ", NETIF_IF_AP, &config);
#ifdef CONFIG_ETH
        BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_ETH, &config));
        WDRV_CLI_DUMP_IP(" ", NETIF_IF_ETH, &config);
#endif
    }
}

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

/**
 * hexstr2bin - Convert ASCII hex string into binary data
 * @hex: ASCII hex string (e.g., "01ab")
 * @buf: Buffer for the binary data
 * @len: Length of the text to convert in bytes (of buf); hex will be double
 * this size
 * Returns: 0 on success, -1 on failure (invalid hex string)
 */
int cli_hexstr2bin(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

#define WDRV_CMD_CNT (sizeof(s_wdrv_commands) / sizeof(struct cli_command))
static void wdrv_cmd_help(void)
{
    printf("wdrv <arg1> <arg2> <arg3> <arg4>\r\n");
    printf("--------------WLAN COMMAND---------------------------------\r\n");
    printf("wdrv scan_wifi                                 - scan AP\r\n");
    printf("wdrv connect [ssid] [password]                 - connect with AP\r\n");
    printf("wdrv disconnect                                - disconnect with ap\r\n");
    printf("wdrv start_ap [ssid] [password]                - start SoftAP\r\n");
    printf("wdrv stop_ap                                   - stop softAP\r\n");
    printf("wdrv get_wlan_stat                             - get wlan status\r\n");
    printf("wdrv wifi_mmd [enable]                         - config Wi-Fi multimedia mode\r\n");
    printf("wdrv set_netinfo [ip] [mask] [gw] [dns]        - config net info\r\n");
    printf("wdrv set_ar [ar_en]                            - config auto reconnect\r\n");
    printf("--------------BLE COMMAND----------------------------------\r\n");
    printf("wdrv open_ble                                  - open ble\r\n");
    printf("wdrv close ble                                 - close ble\r\n");
    printf("--------------SYSTEM COMMAND-------------------------------\r\n");
    printf("wdrv set_mac [mac]                             - set mac addr(set_mac 112233aabbcc)\r\n");
    printf("wdrv get_mac                                   - get mac addr\r\n");
    printf("wdrv enter_sleep                               - ask controller goto sleep\r\n");
    printf("wdrv exit_sleep                                - ask controller exit sleep\r\n");
    printf("wdrv send_at [AT string]                       - send AT command\r\n");
    printf("wdrv keepalive_cfg [ip] [port]                 - start keepalive demo\r\n");
    printf("wdrv set_time [time]                           - set time\r\n");
    printf("wdrv get_time                                  - get time\r\n");
    printf("wdrv cust [data string]                        - start customer demo\r\n");
    printf("wdrv start_ota                                 - notify controller to START OTA\r\n");
    printf("wdrv send_ota_pkt [offset] [size] [finish]     - send demo OTA packet\r\n");
    printf("wdrv stop_ota                                  - notify controller to STOP OTA\r\n");
}
static int wifi_filter_cb(const uint8_t *data, uint32_t len, const wifi_frame_info_t *frame_info)
{
	if (!data) {
		CLI_LOGE("null data\n");
		return BK_OK;
	}

    BK_LOGD(NULL, "%s,%d,frame:0x%x,len:%d,frame_info:0x%x\n",__func__,__LINE__,data,len,frame_info);

	return BK_OK;
}

static void wdrv_handle_cli_commmand(char *pcWriteBuffer, int xWriteBufferLen, int argC, char **argV)
{
    if(argC <= 1) {
        printf("Invalid argC = %d.\r\n", argC);
        wdrv_cmd_help();
        return;
    }
    else if ((argC == 2) && 
        (!os_strcmp(argV[1], "-h") || !os_strcmp(argV[1], "help"))) {
        wdrv_cmd_help();
        return;
    }

    /// Wi-Fi command
    if (!strcasecmp(argV[1], "scan_wifi")) {
        //bk_ioctl_scan_wifi_cmd();
    } else if (!strcasecmp(argV[1], "connect")) {
        char *ssid = NULL;
        char *password = "";
        if (argC >= 2)
            ssid = argV[2];

        if (argC >= 3)
            password = argV[3];
        char *oob_ssid_tp = ssid;
        if (oob_ssid_tp)
            wdrv_demo_connect((char *)oob_ssid_tp, password);
    } else if (!strcasecmp(argV[1], "sta")) {
        char *ssid = NULL;
        char *password = "";
        if (argC >= 2)
            ssid = argV[2];

        if (argC >= 3)
            password = argV[3];
        char *oob_ssid_tp = ssid;
        if (oob_ssid_tp)
            demo_sta_app_init((char *)oob_ssid_tp, password);
    } else if (!strcasecmp(argV[1], "stop_sta")) {
        bk_wifi_sta_stop();
    } else if (!strcasecmp(argV[1], "get_config")) {
        wifi_sta_config_t config;
        bk_wifi_sta_get_config(&config);
        WDRV_LOGD("ssid:%s pw:%s\r\n", config.ssid, config.password);
    } else if (!strcasecmp(argV[1], "get_mac")) {
        uint8_t base_mac[BK_MAC_ADDR_LEN] = {0};
        uint8_t sta_mac[BK_MAC_ADDR_LEN] = {0};
        uint8_t ap_mac[BK_MAC_ADDR_LEN] = {0};
        BK_LOG_ON_ERR(bk_wdrv_get_mac(base_mac, MAC_TYPE_BASE));
        BK_LOG_ON_ERR(bk_wifi_sta_get_mac(sta_mac));
        BK_LOG_ON_ERR(bk_wifi_ap_get_mac(ap_mac));
        WDRV_LOGD("base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
        WDRV_LOGD("sta mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(sta_mac));
        WDRV_LOGD("ap mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(ap_mac));
    } else if (!strcasecmp(argV[1], "set_mac")) {
        uint8_t base_mac[BK_MAC_ADDR_LEN] = {0};
        if (argC == 3) {
            cli_hexstr2bin(argV[2], base_mac, BK_MAC_ADDR_LEN);
            bk_wifi_set_mac_address((char *)base_mac);
            WDRV_LOGI(BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
        }
    } else if (!strcasecmp(argV[1], "ip")) {
        wdrv_ip_cmd_show_ip(NETIF_IF_COUNT);
    } else if (!strcasecmp(argV[1], "ping")) {
        uint32_t cnt = 4;
        if (argC >= 3) {
            cnt = os_strtoul(argV[3], NULL, 10);
            ping_start(argV[2], cnt, 60);
        }
    } else if (!strcasecmp(argV[1], "scan")) {
        if (argC < 2) 
           demo_scan_adv_app_init(NULL);
        else {
              uint8_t *ap_ssid;
              ap_ssid = (uint8_t *)argV[2];
              demo_scan_adv_app_init(ap_ssid);
        }
    } else if (!strcasecmp(argV[1], "stop_scan")) {
        BK_LOG_ON_ERR(bk_wifi_scan_stop());
    } else if (!strcasecmp(argV[1], "scan_result")) {
        wifi_scan_result_t scan_result = {0};
        BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
        BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
        bk_wifi_scan_free_result(&scan_result);
    } else if (!strcasecmp(argV[1], "stats")) {
        wdrv_print_debug_info();
    }
    else if (strcasecmp(argV[1], "m_mode") == 0) {
        WDRV_LOGD("media_mode: %d\n",os_strtoul(argV[2], NULL, 10));
        bk_wifi_set_wifi_media_mode(os_strtoul(argV[2], NULL, 10));
    }
    else if (strcasecmp(argV[1], "m_quality") == 0) {
        WDRV_LOGD("media_quality: %d\n",os_strtoul(argV[2], NULL, 10));
        bk_wifi_set_video_quality(os_strtoul(argV[2], NULL, 10));
    }
#if CONFIG_WIFI_SOFTAP
    else if (!strcasecmp(argV[1], "start_ap")) {
        char *ap_ssid = NULL;
        char *ap_key = "";
        char *ap_channel = NULL;
        if (argC == 3)
            ap_ssid = argV[2];
        else if (argC == 4) {
            ap_ssid = argV[2];
            if (os_strlen(argV[3]) <= 2)
                ap_channel = argV[3];
            else
                ap_key = argV[3];
        } else if (argC == 5) {
            ap_ssid = argV[2];
            ap_key = argV[3];
            ap_channel = argV[4];
        } else {
            CLI_LOGD("Invalid parameters\n");
            return;
        }

        char *oob_ssid_softap = ap_ssid;
        if (oob_ssid_softap) {
            wdrv_demo_softap_init((char *)oob_ssid_softap, ap_key, ap_channel);
        }
    }else if (!strcasecmp(argV[1], "stop_ap")) {
            bk_wifi_ap_stop();
    }
    else if (!strcasecmp(argV[1], "start_hidden_softap")) {
        char *ap_ssid = NULL;
        char *ap_key = "";
        char *ap_channel = NULL;
        if (argC == 3)
            ap_ssid = argV[2];
        else if (argC == 4) {
            ap_ssid = argV[2];
            if (os_strlen(argV[3]) <= 2)
                ap_channel = argV[3];
            else
                ap_key = argV[3];
        } else if (argC == 5) {
            ap_ssid = argV[2];
            ap_key = argV[3];
            ap_channel = argV[4];
        } else {
            CLI_LOGD("Invalid parameters\n");
            return;
        }

        char *oob_ssid_softap = ap_ssid;
        if (oob_ssid_softap) {
            wdrv_demo_hidden_softap_init((char *)oob_ssid_softap, ap_key, ap_channel);
        }
    }
#endif
    else if (!strcasecmp(argV[1], "filter")) {
            bk_wifi_filter_register_cb(wifi_filter_cb);
    }
#if CONFIG_BRIDGE
    else if (!strcasecmp(argV[1], "bridge_open")) {
        char *ssid = NULL, br_ssid[63] = {0};
        bk_bridge_config_t br_config = {0};
        if (argC >= 2)
            ssid = argV[2];

        if (argC >= 3)
            br_config.key = argV[3];
        br_config.ext_sta_ssid = ssid;
        os_snprintf(br_ssid, 63, "%s_brr", ssid);
        br_config.bridge_ssid = br_ssid;
        if (br_config.bridge_ssid)
            bk_bridge_start(&br_config);
    } else if (!strcasecmp(argV[1], "bridge_close")) {
        bk_bridge_stop();
    }
#endif
    else {
        printf("Invalid wdrv command\n");
        wdrv_cmd_help();
    }
}

void cli_wifi_ftp_server_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGI("Invalid ftp server paramter\r\n");
		goto error;
	}

	if(os_strcmp(argv[1], "server") == 0) {
		#if CONFIG_FTP_SERVER
		#if CONFIG_VFS
		ftpd_server_init();
		#endif
		#endif
	}
	else {
		CLI_LOGI("Invalid ftp server paramter\r\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

static const struct cli_command s_wdrv_commands[] = {
    {"wdrv", "wdrv", wdrv_handle_cli_commmand},
    {"ftp", "ftp server", cli_wifi_ftp_server_cmd},
};

int wdrv_cli_init(void)
{
    return cli_register_commands(s_wdrv_commands, WDRV_CMD_CNT);
}
#endif
    
