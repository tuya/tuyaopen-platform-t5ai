#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"

#define WIFI_API_IPC_COM_REQ_MAX_ARGC           6


enum CIF_WIFI_API_CMD_TYPE
{
    // SCAN management command section
    SCAN_START                          = 0x300,  //BK_CMD_WIFI_API_START
    SCAN_STOP                           = 0x301,
    SCAN_RESULT                         = 0x302,
    SCAN_CONTRY_CODE                    = 0x303,
    SCAN_RESULT_FREE                    = 0x304,

    // STA management command section
    STA_SET_CONFIG                      = 0x310,
    STA_START                           = 0x311,
    STA_STOP                            = 0x312,
    STA_GET_CONFIG                      = 0x313,
    STA_GET_LINK_STATUS                 = 0x314,
    STA_GET_LISTEN_INTERVAL             = 0x315,
    STA_SET_LISTEN_INTERVAL             = 0x316,
    STA_SET_BCN_LOSS_INT                = 0x317,
    STA_SET_BCN_RECV_WIN                = 0x318,
    STA_SET_BCN_LOSS_TIME               = 0x319,
    STA_SET_BCN_MISS_TIME               = 0x31A,
    STA_GET_LINK_STATE_WITH_REASON      = 0x31B,

    // AP management command section
    AP_SET_CONFIG                       = 0x320,
    AP_START                            = 0x321,
    AP_STOP                             = 0x322,
    AP_NETIF_IP4_CONFIG                 = 0x323,

    // PM management Wi-Fi command section
    STA_PM_ENABLE                       = 0x330,
    STA_PM_DISABLE                      = 0x331,

    // MONITOR Wi-Fi command section
    MONITOR_START                       = 0x340,
    MONITOR_STOP                        = 0x341,
    MONITOR_SET_CONFIG                  = 0x342,
    MONITOR_GET_CONFIG                  = 0x343,
    MONITOR_REGISTER_CB                 = 0x344,
    MONITOR_SET_CHANNEL                 = 0x345,
    MONITOR_RESUME                      = 0x346,
    MONITOR_SUSPEND                     = 0x347,
    FILTER_REGISTER_CB                  = 0x348,
    FILTER_FREE_PBUF                    = 0x349,
    MONITOR_FREE_PBUF                   = 0x350,

    // Common Wi-Fi command section
    WIFI_GET_CHANNEL                    = 0x360,
    WIFI_SET_COUNTRY                    = 0x361,
    WIFI_GET_COUNTRY                    = 0x362,
    WIFI_CAPA_CONFIG                    = 0x363,
    SEND_ARP_SET_RATE_REQ               = 0x364,
    SET_MAC_ADDRESS                     = 0x365,
    STA_GET_MAC                         = 0x366,
    AP_GET_MAC                          = 0x367,
    GET_STATUS                          = 0x368,
    WIFI_SET_MEDIA_MODE                 = 0x369,
    WIFI_SET_VIDEO_QUALITY              = 0x36A,
    WIFI_SET_CSA_COEXIST_MODE_FLAG      = 0x36B,
    WIFI_GET_SUPPORT_MODE               = 0x36C,
    WIFI_GET_BCN_CC                     = 0x36D,
    WIFI_SET_BLOCK_BCMC_EN              = 0x36E,
    WIFI_GET_BLOCK_BCMC_EN              = 0x36F,

    //Common PHY command section
    PHY_CAL_RFCALI                      = 0x380,

    // RAW Wi-Fi command section
    SEND_RAW                            = 0x390,


    //sync sta_info_tab
    CHECK_CLIENT_MAC_CONNECTED          = 0x3A0,
    SET_BRIDGE_SYNC_STATE               = 0x3A1,

    // FTM command section
    FTM_START                           = 0x3B0,
    FTM_FREE_RESULT                     = 0x3B1,

    // CSI command section
    CSI_ALG_CONFIG                      = 0x3C0,
    CSI_START                           = 0x3C1,
    CSI_STOP                            = 0x3C2,
    CSI_STATIC_PARAM_RESET              = 0x3C3,
    CSI_INFO_GET                        = 0x3C4,
    CSI_DEMO_LIGHT                      = 0x3C5,

    BK_WIFI_API_CMD_BUTT                = BK_CMD_WIFI_API_END
};

enum CIF_WIFI_API_EVT_TYPE
{
    // Wi-Fi event
    STA_EVT_XXX_0                       = 0x300,  //BK_EVT_WIFI_API_START
    STA_EVT_XXX_1                       = 0x301,

    MONITOR_REGISTER_CB_IND             = 0x310,
    FILER_REGISTER_CB_IND               = 0x311,

    BK_WIFI_API_EVT_BUTT                = BK_EVT_WIFI_API_END
};

typedef struct wifi_arg_ipc_info
{
    uint32_t argc;
    uint32_t args[WIFI_API_IPC_COM_REQ_MAX_ARGC];
} wifi_api_arg_info_t;

bk_err_t cif_handle_wifi_api_cmd(struct bk_msg_hdr *msg);
bk_err_t cif_send_wifi_api_evt(uint32_t cmd_id, uint32_t argc, ...);

#ifdef __cplusplus
}
#endif
