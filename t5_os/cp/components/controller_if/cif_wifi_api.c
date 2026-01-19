#include "net.h"
#include "bk_wifi_types.h"
#include "wifi.h"
#include "bk_wifi.h"
#include <stdlib.h>
#include <string.h>
#include "cif_wifi_api.h"
#include "cif_main.h"
#include "cif_ipc.h"

bk_err_t wifi_monitor_cp_cb(const uint8_t *frame, uint32_t len, const wifi_frame_info_t *frame_info)
{
    struct monitor_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint32_t total_len = len + sizeof(wifi_frame_info_t) + sizeof(uint32_t) + sizeof(struct monitor_struct); //extra not used in struct wifi_frame_info_t
    struct pbuf* p_copy = NULL;
    uint8_t* temp_payload = NULL;
    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    struct monitor_struct* hdr = (struct monitor_struct*)(p_copy + 1);
    bk_err_t ret =BK_OK;

    if(p_copy)
    {
        hdr->para[0] = len;

        memcpy(hdr->payload,frame_info,sizeof(wifi_frame_info_t));
        hdr->para[1] = (uint32_t)hdr->payload;
        
        temp_payload = (uint8_t*)hdr->payload + sizeof(wifi_frame_info_t);

        memcpy(temp_payload,frame,len);
        hdr->para[2] = (uint32_t)temp_payload;
        
        //bk_mem_dump("cp",(uint32_t)p_copy,200);
        //BK_LOGD(NULL,"%s,%d,frame:0x%x,len:%d,frame_info:0x%x\n",hdr->para[2],hdr->para[0],hdr->para[1]);
    }
    else
    {
        return BK_FAIL;
    }

    struct ctrl_cmd_hdr *cpdu = (struct ctrl_cmd_hdr*)(p_copy + 1);
    cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
    cpdu->co_hdr.type = RX_MSDU_DATA;
    cpdu->co_hdr.need_free = 0;
    cpdu->co_hdr.vif_idx = 0; //?
    cpdu->co_hdr.special_type = RX_MONITOR_TYPE;
    cpdu->msg_hdr.id = MONITOR_REGISTER_CB_IND;
    cpdu->msg_hdr.param_len = len + sizeof(wifi_frame_info_t) + sizeof(uint32_t);
    ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
    
    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

bk_err_t wifi_filter_cp_cb(const uint8_t *frame, uint32_t len, const wifi_frame_info_t *frame_info)
{
    struct filter_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    uint32_t total_len = len + sizeof(wifi_frame_info_t) + sizeof(uint32_t) + sizeof(struct filter_struct); //extra not used in struct wifi_frame_info_t
    struct pbuf* p_copy = NULL;
    uint8_t* temp_payload = NULL;
    p_copy = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM_RX);
    struct filter_struct* hdr = (struct filter_struct*)(p_copy + 1);
    bk_err_t ret =BK_OK;

    if(p_copy)
    {
        hdr->para[0] = len;

        memcpy(hdr->payload,frame_info,sizeof(wifi_frame_info_t));
        hdr->para[1] = (uint32_t)hdr->payload;
        
        temp_payload = (uint8_t*)hdr->payload + sizeof(wifi_frame_info_t);

        memcpy(temp_payload,frame,len);
        hdr->para[2] = (uint32_t)temp_payload;

        //BK_LOGD(NULL,"%s,%d,frame:0x%x,len:%d,frame_info:0x%x\n",__func__,__LINE__,hdr->para[2],hdr->para[0],hdr->para[1]);
    }
    else
    {
        return BK_FAIL;
    }

    struct ctrl_cmd_hdr *cpdu = (struct ctrl_cmd_hdr*)(p_copy + 1);
    cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
    cpdu->co_hdr.type = RX_MSDU_DATA;
    cpdu->co_hdr.need_free = 0;
    cpdu->co_hdr.vif_idx = 0; //?
    cpdu->co_hdr.special_type = RX_FILTER_TYPE;
    cpdu->msg_hdr.id = FILER_REGISTER_CB_IND;
    cpdu->msg_hdr.param_len = len + sizeof(wifi_frame_info_t) + sizeof(uint32_t);
    ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
    
    if(ret != BK_OK)
    {
        #if CONFIG_CONTROLLER_RX_DIRECT_PSH
        pbuf_free(p_copy);
        #else
        //If rxbuf push fail, free it immediately
        cif_free_rx_buf((uint32_t)p_copy);
        #endif
    }
    return ret;
}

bk_err_t cif_handle_wifi_api_cmd(struct bk_msg_hdr *msg)
{
    bk_err_t ret = BK_OK;
    wifi_api_arg_info_t *arg_info = (wifi_api_arg_info_t *)(msg + 1);

    CIF_LOGD("cif_handle_wifi_api_cmd cmd:%x agrc:%d\n", msg->cmd_id, arg_info->argc);

    if (arg_info->argc)
    {
        BK_ASSERT(arg_info->argc <= WIFI_API_IPC_COM_REQ_MAX_ARGC);
        // CIF_LOGD("arg[0]:%x arg[1]:%x arg[2]:%x arg[3]:%x arg[4]:%x arg[5]:%x\n", 
        //     arg_info->args[0], arg_info->args[1], arg_info->args[2],
        //     arg_info->args[3], arg_info->args[4], arg_info->args[5]);
    }

    switch(msg->cmd_id)
    {
        case STA_PM_ENABLE:
        {
            ret = bk_wifi_sta_pm_enable();
            break;
        }
        case STA_PM_DISABLE:
        {
            ret = bk_wifi_sta_pm_disable();
            break;
        }
        case STA_SET_CONFIG:
        {
            wifi_sta_config_t *config = (wifi_sta_config_t *)arg_info->args[0];
            //CIF_LOGD("sizeof(wifi_sta_config_t)=%d\r\n", sizeof(wifi_sta_config_t));
            ret = bk_wifi_sta_set_config(config);
            break;
        }
        case STA_GET_CONFIG:
        {
            wifi_sta_config_t *config = (wifi_sta_config_t *)arg_info->args[0];
            //CIF_LOGD("sizeof(wifi_sta_config_t)=%d\r\n", sizeof(wifi_sta_config_t));
            ret = bk_wifi_sta_get_config(config);
            break;
        }
        case STA_START:
        {
            ret = bk_wifi_sta_start();
            break;
        }
        case STA_GET_LINK_STATUS:
        {
            wifi_link_status_t *link_status = (wifi_link_status_t *)arg_info->args[0];
            if ((wifi_netif_sta_is_connected() || wifi_netif_sta_is_got_ip()))
            {
                    bk_wifi_sta_get_link_status(link_status);
                    link_status->state = WIFI_LINKSTATE_STA_CONNECTED;
            }
            else
                link_status->state = WIFI_LINKSTATE_STA_DISCONNECTED;

            break;
        }

        case WIFI_GET_CHANNEL:
        {
            uint8_t *channel = (uint8_t *)(arg_info->args[0]);
            *channel = bk_wifi_get_channel();
            break;
        }

        case WIFI_SET_COUNTRY:
        {
            wifi_country_t *country = (wifi_country_t *)(arg_info->args[0]);
            ret = bk_wifi_set_country(country);
            break;
        }

        case WIFI_GET_COUNTRY:
        {
            wifi_country_t *country = (wifi_country_t *)(arg_info->args[0]);
            ret = bk_wifi_get_country(country);
            break;
        }

        case STA_GET_LISTEN_INTERVAL:
        {
            uint8_t *listen_interval = (uint8_t *)(arg_info->args[0]);
            ret = bk_wifi_get_listen_interval(listen_interval);
            break;
        }

        case STA_SET_LISTEN_INTERVAL:
        {
            uint8_t listen_interval = (uint8_t)(arg_info->args[0]);
            ret = bk_wifi_send_listen_interval_req(listen_interval);
            break;
        }

        case STA_SET_BCN_LOSS_INT:
        {
            uint8_t interval = (uint8_t)(arg_info->args[0]);
            uint8_t repeat_num = (uint8_t)(arg_info->args[1]);
            ret = bk_wifi_send_bcn_loss_int_req(interval, repeat_num);
            break;
        }

        case STA_SET_BCN_RECV_WIN:
        {
            uint8_t default_win = (uint8_t)(arg_info->args[0]);
            uint8_t max_win = (uint8_t)(arg_info->args[1]);
            uint8_t step = (uint8_t)(arg_info->args[2]);
            ret = bk_wifi_set_bcn_recv_win(default_win, max_win, step);
            break;
        }

        case STA_SET_BCN_LOSS_TIME:
        {
            uint8_t wait_cnt = (uint8_t)(arg_info->args[0]);
            uint8_t wake_cnt = (uint8_t)(arg_info->args[1]);
            ret = bk_wifi_set_bcn_loss_time(wait_cnt, wake_cnt);
            break;
        }

        case STA_SET_BCN_MISS_TIME:
        {
            uint8_t bcn_miss_time = (uint8_t)(arg_info->args[0]);
            ret = bk_wifi_set_bcn_miss_time(bcn_miss_time);
            break;
        }

        case STA_GET_LINK_STATE_WITH_REASON:
        {
            wifi_linkstate_reason_t *info = (wifi_linkstate_reason_t *)(arg_info->args[0]);
            ret = bk_wifi_sta_get_linkstate_with_reason(info);
            break;
        }

        case WIFI_GET_SUPPORT_MODE:
        {
            uint8_t *support_mode = (uint8_t *)(arg_info->args[0]);
            ret = bk_wifi_get_support_wifi_mode(support_mode);
            break;
        }

        #if CONFIG_WIFI_SCAN_COUNTRY_CODE
        case SCAN_CONTRY_CODE:
        {
            uint8_t *country_code = (uint8_t *)(arg_info->args[0]);
            int *country_code_len = (int *)(arg_info->args[1]);
            ret = bk_scan_country_code(country_code, country_code_len);
            break;
        }

        case WIFI_GET_BCN_CC:
        {
            bool enable = !!(arg_info->args[0]);
            ret = bk_wifi_bcn_cc_rxed_register_cb(NULL, NULL, enable);
            break;
        }
        #endif

        case WIFI_SET_BLOCK_BCMC_EN:
        {
            uint8_t config = (uint8_t)(arg_info->args[0]);
            ret = bk_wifi_set_block_bcmc_en(config);
            break;
        }

        case WIFI_GET_BLOCK_BCMC_EN:
        {
            bool *enable = (bool *)(arg_info->args[0]);
            *enable = bk_wifi_get_block_bcmc_en();
            break;
        }

        #if CONFIG_WIFI_FTM
        case FTM_START:
        {
            wifi_ftm_config_t *config = (wifi_ftm_config_t *)(arg_info->args[0]);
            wifi_ftm_results_t *ftm_results = (wifi_ftm_results_t *)(arg_info->args[1]);
            ret = bk_wifi_ftm_start(config, ftm_results);
            break;
        }

        case FTM_FREE_RESULT:
        {
            wifi_ftm_results_t *ftm_results = (wifi_ftm_results_t *)(arg_info->args[0]);
            ret = bk_wifi_ftm_free_result(ftm_results);
            break;
        }
        #endif

        #if CONFIG_WIFI_CSI_EN
        case CSI_ALG_CONFIG:
        {
            double thres1 = (double)(arg_info->args[0]);
            ret = bk_wifi_csi_alg_config(thres1);
            break;
        }

        case CSI_START:
        {
            uint8_t csi_work_type = (uint8_t)(arg_info->args[0]);
            uint8_t csi_work_mode = (uint8_t)(arg_info->args[1]);
            uint8_t csi_work_identity = (uint8_t)(arg_info->args[2]);
            uint8_t csi_data_format = (uint8_t)(arg_info->args[3]);
            uint32_t csi_data_interval = (uint32_t)(arg_info->args[4]);
            uint32_t delay = (uint32_t)(arg_info->args[5]);
            ret = bk_wifi_csi_start_req(csi_work_type, csi_work_mode, csi_work_identity, csi_data_format,
                                        csi_data_interval, delay);
            break;
        }

        case CSI_STOP:
        {
            ret = bk_wifi_csi_stop_req();
            break;
        }

        case CSI_STATIC_PARAM_RESET:
        {
            uint8_t update_cali_mode = (uint8_t)(arg_info->args[0]);
            uint32_t cali_cnt = (uint32_t)(arg_info->args[1]);
            ret = bk_wifi_csi_static_param_reset_req(update_cali_mode, cali_cnt);
            break;
        }
        #endif

        case CSI_INFO_GET:
        {
            bool enable = !!(arg_info->args[0]);
            ret = bk_wifi_csi_info_cb_register(enable);
            break;
        }

        #if CONFIG_WIFI_CSI_DEMO
        case CSI_DEMO_LIGHT:
        {
            uint8_t color = (uint8_t)(arg_info->args[0]);
            bool flicker = (bool)(arg_info->args[1]);
            ret = bk_wifi_csi_demo_turn_on_light(color, flicker);
            break;
        }
        #endif

        case MONITOR_START:
        {
            ret = bk_wifi_monitor_start();
            break;
        }
        case MONITOR_STOP:
        {
            ret = bk_wifi_monitor_stop();
            break;
        }
        case MONITOR_SET_CHANNEL:
        {
            wifi_channel_t *chan = (wifi_channel_t *)arg_info->args[0];

            ret = bk_wifi_monitor_set_channel(chan);
            break;
        }
        case SEND_RAW:
        {
            uint8_t *buffer = (uint8_t *)arg_info->args[0];
            int len = arg_info->args[1];

            ret =  bk_wifi_send_raw(buffer, len);
            break;
        }
        case PHY_CAL_RFCALI:
        {
            ret = bk_wifi_manual_cal_rfcali_status();
            break;
        }
        case WIFI_CAPA_CONFIG:
        {
            wifi_capability_t capa_id = (wifi_capability_t)arg_info->args[0];
            uint32_t capa_val = arg_info->args[1];

            ret = bk_wifi_capa_config(capa_id,capa_val);
            break;
        }
        case SCAN_START:
        {
            wifi_scan_config_t *scan_config = (wifi_scan_config_t *)arg_info->args[0];
            ret = bk_wifi_scan_start(scan_config);
            break;
        }
        case SCAN_STOP:
        {
            ret = bk_wifi_scan_stop();
            break;
        }
        case SCAN_RESULT:
        {
            wifi_scan_result_t *scan_result = (wifi_scan_result_t *)arg_info->args[0];
            ret = bk_wifi_scan_get_result(scan_result);
            break;
        }
        case MONITOR_REGISTER_CB:
        {
            ret = bk_wifi_monitor_register_cb(wifi_monitor_cp_cb);
            break;
        }
        case FILTER_REGISTER_CB:
        {
            ret = bk_wifi_filter_register_cb(wifi_filter_cp_cb);
            break;
        }
        case SEND_ARP_SET_RATE_REQ:
        {
            ret = bk_wifi_send_arp_set_rate_req((uint16_t)arg_info->args[0]);
            break;
        }
        case SCAN_RESULT_FREE:
        {
            wifi_scan_result_t *scan_result = (wifi_scan_result_t *)arg_info->args[0];
            bk_wifi_scan_free_result(scan_result);
            break;
        }
        case SET_MAC_ADDRESS:
        {
            ret = bk_wifi_set_mac_address((char *)arg_info->args[0]);
            break;
        }
        case STA_GET_MAC:
        {
            ret = bk_wifi_sta_get_mac((uint8_t *)arg_info->args[0]);
            break;
        }
        case AP_GET_MAC:
        {
            ret = bk_wifi_ap_get_mac((uint8_t *)arg_info->args[0]);
            break;
        }
        case GET_STATUS:
        {
            extern bk_err_t bk_wifi_get_wifi_status(void *out);
            ret = bk_wifi_get_wifi_status((void *)arg_info->args[0]);
            break;
        }
        case AP_STOP:
        {
            ret = bk_wifi_ap_stop();
            break;
        }
        case STA_STOP:
        {
            ret = bk_wifi_sta_stop();
            break;
        }
        case WIFI_SET_MEDIA_MODE:
        {
            ret = bk_wifi_set_wifi_media_mode((bool)arg_info->args[0]);
            break;
        }
        case WIFI_SET_VIDEO_QUALITY:
        {
            ret = bk_wifi_set_video_quality((uint8_t)arg_info->args[0]);
            break;
        }
        case WIFI_SET_CSA_COEXIST_MODE_FLAG:
        {
            ret = bk_wifi_set_csa_coexist_mode_flag((bool)arg_info->args[0]);
            break;
        }
        case AP_SET_CONFIG:
        {
            ret = bk_wifi_ap_set_config((wifi_ap_config_t *)arg_info->args[0]);
            break;
        }
        case AP_START:
        {
            ret = bk_wifi_ap_start();
            break;
        }
#if CONFIG_BRIDGE
        case CHECK_CLIENT_MAC_CONNECTED:
        {
            ret = bk_wifi_check_client_mac_connected((uint8_t *)arg_info->args[0]);
            break;
        }
        case SET_BRIDGE_SYNC_STATE:
        {
            ret = bk_wifi_sync_bridge_state(*(bk_bridge_state_t *)arg_info->args[0]);
            break;
        }
#endif
        case AP_NETIF_IP4_CONFIG:
        {
            ret = bk_netif_set_ip4_config(NETIF_IF_AP, (netif_ip4_config_t *)arg_info->args[0]);
            break;
        }
        default:
        {
            ret = BK_ERR_NOT_FOUND;
            break;
        }
    }

    if (cif_bk_cmd_confirm(msg, (uint8_t *)&ret, sizeof(ret)) != BK_OK)
    {
        CIF_LOGE("wifi api confirm FAILED\n",__func__,__LINE__);
        return BK_FAIL;
    }

    //CIF_LOGD("cif_handle_wifi_api_cmd ret:%d\n", ret);
    return BK_OK;
}

bk_err_t cif_send_wifi_api_evt(uint32_t cmd_id, uint32_t argc, ...)
{
     bk_err_t ret = BK_OK;
    wifi_api_arg_info_t arg_info = { 0 };

    if (argc)
    {
        BK_ASSERT (argc <= WIFI_API_IPC_COM_REQ_MAX_ARGC);
        arg_info.argc = argc;

        va_list args;
        va_start(args, argc);
        for (int i = 0; i < argc; i++)
        {
            arg_info.args[i] = va_arg(args, uint32_t);
            //WIFI_LOGD("arg[%d]:%x\n", i, com_req.arg_info.args[i]);
        }
        va_end(args);
    }
    ret = cif_bk_send_event(cmd_id, (uint8_t *)&arg_info, sizeof(wifi_api_arg_info_t));

    if (ret < 0)
    {
        CIF_LOGE("cif_send_wifi_api_evt FAILED, cmd_id:%x argc:%d ret:%d\n", cmd_id, arg_info.argc, ret);
        return BK_ERR_TIMEOUT;
    }

    return ret;
}
