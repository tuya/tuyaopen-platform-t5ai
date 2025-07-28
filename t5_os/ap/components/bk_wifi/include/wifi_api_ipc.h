/*
 * Copyright 2020-2025 Beken

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "os/os.h"
#include "wdrv_co_list.h"
#include "wdrv_cntrl.h"
#include "wifi_log.h"

#define WIFI_API_IPC_COM_REQ_MAX_ARGC           6

enum BK_WIFI_API_CMD_TYPE
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
    STA_GET_LINK_STATE_WITH_REASON      = 0x31A,

    // AP management command section
    AP_SET_CONFIG                       = 0x320,
    AP_START                            = 0x321,
    AP_STOP                             = 0x322,

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
    WIFI_CAPA_CONFIG                    = 0x362,
    SEND_ARP_SET_RATE_REQ               = 0x363,
    SET_MAC_ADDRESS                     = 0x364,
    STA_GET_MAC                         = 0x365,
    AP_GET_MAC                          = 0x366,

    //Common PHY command section
    PHY_CAL_RFCALI                      = 0x380,

    // RAW Wi-Fi command section
    SEND_RAW                            = 0x390,

    BK_WIFI_API_CMD_BUTT                = BK_CMD_WIFI_API_END
};

enum BK_WIFI_API_EVT_TYPE
{
    // Wi-Fi event
    STA_EVT_XXX_0                       = 0x300,  //BK_EVT_WIFI_API_START
    STA_EVT_XXX_1                       = 0x301,

    MONITOR_REGISTER_CB_IND             = 0x310,
    FILER_REGISTER_CB_IND               = 0x311,

    BK_WIFI_API_EVT_BUTT                = BK_EVT_WIFI_API_END
};

struct wifi_api_req_no_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_1_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_2_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_3_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_4_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_5_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_6_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    uint32_t arg6;
    wdrv_cmd_cfm cmd_cfm;
};
typedef struct wifi_arg_ipc_info
{
    uint32_t argc;
    uint32_t args[WIFI_API_IPC_COM_REQ_MAX_ARGC];
} wifi_api_arg_info_t;

struct wifi_api_com_req
{
    wdrv_cmd_hdr cmd_hdr;
    wifi_api_arg_info_t arg_info;
    wdrv_cmd_cfm cmd_cfm;
};

bk_err_t wifi_send_com_api_cmd(uint32_t cmd_id, uint32_t argc, ...);
bk_err_t wifi_handle_api_evt(uint32_t evt_id, uint8_t *evt_data, uint16_t evt_len);




#ifdef __cplusplus
}
#endif

