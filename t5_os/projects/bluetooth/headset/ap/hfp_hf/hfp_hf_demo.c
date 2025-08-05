#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "hfp_hf_demo.h"

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include <driver/sbc_types.h>
//#include <driver/aud_types.h>
//#include <driver/aud.h>
#include <driver/sbc.h>
#include "components/bluetooth/bk_ble_types.h"
#include "modules/sbc_encoder.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "components/bluetooth/bk_dm_hfp.h"


#include <driver/aud_dac.h>
#include <driver/aud_dac_types.h>
#include "ring_buffer_particle.h"
#include "audio_play.h"
#include "audio_record.h"
#include "bk_gpio.h"

#define TAG "hfp_client"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define BT_AUDIO_HF_DEMO_MSG_COUNT          (30)
#define SCO_MSBC_SAMPLES_PER_FRAME      120
#define SCO_CVSD_SAMPLES_PER_FRAME      60

#define LOCAL_NAME "soundbar"

#define HF_LOCAL_SPEAKER_WAIT_TIME 2000

#define CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            LOGI("CHECK_NULL fail \n");\
            return;\
        }\
    } while(0)


#define HF_MIC_THREAD_PRI       BEKEN_DEFAULT_WORKER_PRIORITY-1
#define HF_SPEAKER_THREAD_PRI   BEKEN_DEFAULT_WORKER_PRIORITY-1
#define HF_LOCAL_ROLLBACK_TEST        0
#define HF_REMOTE_ROLLBACK_TEST       0
#define HF_AT_TEST 0
#define HF_AT_ENABLE_CMEE 1

enum
{
    BT_AUDIO_MSG_NULL = 0,
    BT_AUDIO_VOICE_START_MSG = 1,
    BT_AUDIO_VOICE_STOP_MSG = 2,
    BT_AUDIO_VOICE_IND_MSG = 3,
};


typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} bt_audio_hf_demo_msg_t;


enum
{
    HFP_STATUS_IDLE,
    HFP_STATUS_WAIT_QUERY_CALL,
    HFP_STATUS_WAIT_VGS,
    HFP_STATUS_WAIT_VGM,

    HFP_STATUS_WAIT_CUSTOM,
    HFP_STATUS_WAIT_QUERY_CURRENT_OP,
    HFP_STATUS_WAIT_RETRIEVE_SUB_INFO,
    HFP_STATUS_WAIT_SEND_VTS,
    HFP_STATUS_WAIT_REQ_LAST_TAG_NUM,
    HFP_STATUS_WAIT_NREC,
    HFP_STATUS_WAIT_VR_ENABLE,
    HFP_STATUS_WAIT_VR_DISBLE,
    HFP_STATUS_WAIT_DIAL,
    HFP_STATUS_WAIT_DIAL_HUP,
    HFP_STATUS_WAIT_DIAL_MEM,
    HFP_STATUS_WAIT_DIAL_MEM_HUP,
    HFP_STATUS_WAIT_REDAIL,
    HFP_STATUS_WAIT_REDAIL_HUP,
    HFP_STATUS_WAIT_DONE,
};

static uint8_t bt_audio_hfp_hf_codec = CODEC_VOICE_CVSD;
static uint8_t hfp_peer_addr [ 6 ] = {0};
static uint8_t hfp_profile_peer_addr [ 6 ] = {0};

static uint8_t s_hfp_status_mach = HFP_STATUS_IDLE;

static sbcdecodercontext_t bt_audio_hf_sbc_decoder;
static SbcEncoderContext bt_audio_hf_sbc_encoder;
static beken_queue_t bt_audio_hf_demo_msg_que = NULL;
static beken_thread_t bt_audio_hf_demo_thread_handle = NULL;

static uint8_t hf_mic_sco_data [ 1024 ] = {0};
static uint16_t hf_mic_data_count = 0;


volatile uint8_t hf_auido_start = 0;

static ring_buffer_particle_ctx s_hfp_sco_spk_data_rb;

static beken_thread_t hf_speaker_thread_handle = NULL;
static beken_thread_t hf_mic_thread_handle = NULL;
static beken_semaphore_t hf_speaker_sema = NULL;
static audio_play_t *s_audio_play_obj;
static audio_record_t *s_audio_record_obj;
static beken_semaphore_t hf_mic_speaker_exit_sema = NULL;

#if HF_LOCAL_ROLLBACK_TEST
static uint16_t mic_read_size = 0;
#endif

static void speaker_task(void *arg);
static int speaker_task_init();
static void mic_task(void *arg);
static int mic_task_init();

int bt_audio_hf_demo_task_init(void);

static bk_err_t bk_bt_dac_set_gain(uint8_t hfp_vol)
{
    uint8_t gain = (0x7f / 15.0) * hfp_vol;

    if(s_audio_play_obj)
    {
        audio_play_set_volume(s_audio_play_obj, gain);

        if (gain == 0)
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_MUTE);
        }
        else
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_UNMUTE);
        }
    }
    else
    {
        LOGE("%s audio play not enable\n", __func__);
    }

    return BK_OK;
}

void bt_audio_hfp_client_voice_data_ind(const uint8_t *data, uint16_t data_len)
{
    bt_audio_hf_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_hf_demo_msg_t));
    if (bt_audio_hf_demo_msg_que == NULL)
        return;

    demo_msg.data = (char *) os_malloc(data_len);
    if (demo_msg.data == NULL)
    {
        LOGI("%s, malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(demo_msg.data, data, data_len);
    demo_msg.type = BT_AUDIO_VOICE_IND_MSG;
    demo_msg.len = data_len;

    rc = rtos_push_to_queue(&bt_audio_hf_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGI("%s, send queue failed\r\n", __func__);
        if (demo_msg.data)
        {
            os_free(demo_msg.data);
        }
    }
}

static void bt_audio_hf_sco_connected(void)
{
    bt_audio_hf_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_hf_demo_msg_t));
    if (bt_audio_hf_demo_msg_que == NULL)
        return;

    demo_msg.type = BT_AUDIO_VOICE_START_MSG;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_hf_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGI("%s, send queue failed\r\n", __func__);
    }
}

static void bt_audio_hf_sco_disconnected(void)
{
    bt_audio_hf_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_hf_demo_msg_t));
    if (bt_audio_hf_demo_msg_que == NULL)
        return;

    demo_msg.type = BT_AUDIO_VOICE_STOP_MSG;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_hf_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGI("%s, send queue failed\r\n", __func__);
    }
}

void bk_bt_app_hfp_client_cb(bk_hf_client_cb_event_t event, bk_hf_client_cb_param_t *param)
{
    LOGI("%s event: %d, addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n", __func__, event, param->remote_bda[0], param->remote_bda[1],
                                                                                  param->remote_bda[2], param->remote_bda[3],
                                                                                  param->remote_bda[4], param->remote_bda[5]);

    switch (event)
    {
        case BK_HF_CLIENT_AUDIO_STATE_EVT:
        {
            LOGI("HFP client audio state: %d\r\n", param->audio_state.state);

            if (BK_HF_CLIENT_AUDIO_STATE_DISCONNECTED == param->audio_state.state)
            {
                bt_audio_hf_sco_disconnected();
            }
            else if (BK_HF_CLIENT_AUDIO_STATE_CONNECTED == param->audio_state.state)
            {
                bt_audio_hfp_hf_codec = param->audio_state.codec_type;
                os_memcpy(hfp_peer_addr, param->remote_bda, 6);
                LOGI("sco connected to %02x:%02x:%02x:%02x:%02x:%02x, codec type %d\n", hfp_peer_addr [ 5 ], hfp_peer_addr [ 4 ], hfp_peer_addr [ 3 ],
                     hfp_peer_addr [ 2 ], hfp_peer_addr [ 1 ], hfp_peer_addr [ 0 ], bt_audio_hfp_hf_codec);

                bt_audio_hf_sco_connected();
            }

        }
        break;
        case BK_HF_CLIENT_CONNECTION_STATE_EVT:
        {
            if (param->conn_state.state == BK_HF_CLIENT_CONNECTION_STATE_SLC_CONNECTED)
            {
                LOGI("HFP service level connected, ag_feature:0x%x, ag_chld_feature:0x%x \n", param->conn_state.peer_feat, param->conn_state.chld_feat);
                LOGI("HFP client connect to peer address: %02x:%02x:%02x:%02x:%02x:%02x \n", param->remote_bda [ 0 ], param->remote_bda [ 1 ],
                     param->remote_bda [ 2 ], param->remote_bda [ 3 ],
                     param->remote_bda [ 4 ], param->remote_bda [ 5 ]);
                os_memcpy(hfp_profile_peer_addr, param->remote_bda, sizeof(param->remote_bda));
                s_hfp_status_mach = HFP_STATUS_WAIT_QUERY_CALL;
                bk_bt_hf_client_query_current_calls(param->remote_bda);
            }
            else if (param->conn_state.state == BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTED)
            {
                LOGI("HFP disconnected \n");
                LOGI("HFP disconnect peer address: %02x:%02x:%02x:%02x:%02x:%02x \n", param->remote_bda [ 0 ], param->remote_bda [ 1 ],
                     param->remote_bda [ 2 ], param->remote_bda [ 3 ],
                     param->remote_bda [ 4 ], param->remote_bda [ 5 ]);
                os_memset(hfp_profile_peer_addr, 0, sizeof(hfp_profile_peer_addr));
            }
        }
        break;
        case BK_HF_CLIENT_BVRA_EVT:
        {
            LOGI("+BRVA: HPF voice recognition activation status: %d \n", param->bvra.value);
        }
        break;
        case BK_HF_CLIENT_CIND_CALL_EVT:
        {
            LOGI("+CIND: HFP call staus:%d \n", param->call.status);
        }
        break;
        case BK_HF_CLIENT_CIND_CALL_SETUP_EVT:
        {
            LOGI("+CIND: HFP call_setup status:%d \n", param->call_setup.status);
        }
        break;
        case BK_HF_CLIENT_CIND_CALL_HELD_EVT:
        {
            LOGI("+CIND: HFP call_hold status:%d \n", param->call_held.status);
        }
        break;
        case BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT:
        {
            LOGI("+CIND: HFP service availability ind: %d\n", param->service_availability.status);
        }
        break;
        case BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT:
        {
            LOGI("+CIND: HFP signal strength ind: %d\n", param->signal_strength.value);
        }
        break;
        case BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT:
        {
            LOGI("+CIND: HFP roming status:%d \n", param->roaming.status);
        }
        break;
        case BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT:
        {
            LOGI("+CIND: HFP battery ind:%d \n", param->battery_level.value);
        }
        break;
        case BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT:
        {
            LOGI("+COPS: HFP network operator name:%s \n", param->cops.name);
        }
        break;
        case BK_HF_CLIENT_BTRH_EVT:
        {
            LOGI("+BTRH: HFP Hold status: %d \n", param->btrh.status);
        }
        break;
        case BK_HF_CLIENT_CLIP_EVT:
        {
            LOGI("+CLIP: HFP calling line number: %s, name:%s \n", param->clip.number, param->clip.name);
        }
        break;
        break;
        case BK_HF_CLIENT_CCWA_EVT:
        {
            LOGI("+CCWA: HFP calling waiting number:%s, name: %s\n", param->ccwa.number, param->ccwa.name);
        }
        break;
        case BK_HF_CLIENT_CLCC_EVT:
        {
            LOGI("+CLCC: HFP calls result dir:%d, idx:%d, mpty:%d, number:%s, status:%d \n", param->clcc.dir, param->clcc.idx, param->clcc.mpty, param->clcc.number, param->clcc.status);
        }
        break;
        case BK_HF_CLIENT_VOLUME_CONTROL_EVT:
        {
            if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_SPK)
            {
                LOGI("+VGS: HPF Speaker gain: %d \n", param->volume_control.volume);
                //bk_bt_dac_set_gain(param->volume_control.volume);
            }
            else if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_MIC)
            {
                LOGI("+VGM: HPF Microphone gain: %d \n", param->volume_control.volume);
            }
        }
        break;
        case BK_HF_CLIENT_AT_RESPONSE_EVT:
        {
            if (0)//param->at_response.code == BK_HF_AT_RESPONSE_CODE_CME)
            {
                LOGI("+CME ERROR: HFP AG error code: %d \n", param->at_response.cme);
            }
            else
            {
                if(param->at_response.code == BK_HF_AT_RESPONSE_CODE_OK)
                {
                    LOGI("BK_HF_CLIENT_AT_RESPONSE_EVT ok, asso_cmd %d, status %d\n", param->at_response.asso_cmd, s_hfp_status_mach);
                }
                else if(param->at_response.code == BK_HF_AT_RESPONSE_CODE_CME)
                {
                    LOGI("BK_HF_CLIENT_AT_RESPONSE_EVT cme err, cme code 0x%x, asso_cmd %d, status %d\n", param->at_response.cme, param->at_response.asso_cmd, s_hfp_status_mach);
                }
                else
                {
                    LOGI("BK_HF_CLIENT_AT_RESPONSE_EVT normal err 0x%x, asso_cmd %d, status %d\n", param->at_response.code, param->at_response.asso_cmd, s_hfp_status_mach);
                }
#if !HF_AT_TEST
                switch(s_hfp_status_mach)
                {
                case HFP_STATUS_WAIT_QUERY_CALL:
                    s_hfp_status_mach = HFP_STATUS_WAIT_VGS;
                    bk_bt_hf_client_volume_update(hfp_profile_peer_addr, BK_HF_VOLUME_CONTROL_TARGET_SPK, 7);
                    break;

                case HFP_STATUS_WAIT_VGS:
                    s_hfp_status_mach = HFP_STATUS_WAIT_VGM;
                    bk_bt_hf_client_volume_update(hfp_profile_peer_addr, BK_HF_VOLUME_CONTROL_TARGET_MIC, 7);
                    break;

                case HFP_STATUS_WAIT_VGM:
                    s_hfp_status_mach = HFP_STATUS_WAIT_DONE;
                    LOGI("%s end op\n", __func__);
                    break;

                }

#else
                switch(param->at_response.asso_cmd)
                {
                case BK_HF_AT_CMD_CLCC:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_QUERY_CALL)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_VGS;
                        bk_bt_hf_client_volume_update(hfp_profile_peer_addr, BK_HF_VOLUME_CONTROL_TARGET_SPK, 7);
                    }
                    break;

                case BK_HF_AT_CMD_VGS:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_VGS)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_VGM;
                        bk_bt_hf_client_volume_update(hfp_profile_peer_addr, BK_HF_VOLUME_CONTROL_TARGET_MIC, 7);
                    }
                    break;

                case BK_HF_AT_CMD_VGM:

                    if(s_hfp_status_mach == HFP_STATUS_WAIT_VGM)
                    {
#if HF_AT_ENABLE_CMEE
                        s_hfp_status_mach = HFP_STATUS_WAIT_CUSTOM;
                        bk_bt_hf_client_send_custom_cmd(hfp_profile_peer_addr, "AT+CMEE=1");
                    }
                    break;

                case BK_HF_AT_CMD_CUSTOM:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_CUSTOM)
                    {
#endif
                        s_hfp_status_mach = HFP_STATUS_WAIT_QUERY_CURRENT_OP;
                        bk_bt_hf_client_query_current_operator_name(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_COPS:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_QUERY_CURRENT_OP)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_RETRIEVE_SUB_INFO;
                        bk_bt_hf_client_retrieve_subscriber_info(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_CNUM:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_RETRIEVE_SUB_INFO)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_SEND_VTS;
                        bk_bt_hf_client_send_dtmf(hfp_profile_peer_addr, "1");
                    }
                    break;

                case BK_HF_AT_CMD_VTS:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_SEND_VTS)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_REQ_LAST_TAG_NUM;
                        bk_bt_hf_client_request_last_voice_tag_number(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_BINP:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_REQ_LAST_TAG_NUM)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_NREC;
                        bk_bt_hf_client_send_nrec(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_NREC:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_NREC)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_VR_ENABLE;
                        bk_bt_hf_client_start_voice_recognition(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_BVRA:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_VR_ENABLE)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_VR_DISBLE;
                        bk_bt_hf_client_stop_voice_recognition(hfp_profile_peer_addr);
                    }
                    else if(s_hfp_status_mach == HFP_STATUS_WAIT_VR_DISBLE)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_DIAL;
                        bk_bt_hf_client_dial(hfp_profile_peer_addr, "112");
                    }
                    break;

                case BK_HF_AT_CMD_OUTCALL:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_HUP;
                        bk_bt_hf_client_reject_call(hfp_profile_peer_addr);
                    }
                    else if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_MEM)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_MEM_HUP;
                        bk_bt_hf_client_reject_call(hfp_profile_peer_addr);
                    }
                    else if(s_hfp_status_mach == HFP_STATUS_WAIT_REDAIL)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_REDAIL_HUP;
                        bk_bt_hf_client_reject_call(hfp_profile_peer_addr);
                    }
                    break;

                case BK_HF_AT_CMD_CHUP:
                    if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_HUP)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_MEM;
                        bk_bt_hf_client_dial_memory(hfp_profile_peer_addr, 1);
                    }
                    else if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_MEM_HUP)
                    {
                        s_hfp_status_mach = HFP_STATUS_WAIT_REDAIL;
                        bk_bt_hf_client_redial(hfp_profile_peer_addr);
                    }
                    else if(s_hfp_status_mach == HFP_STATUS_WAIT_REDAIL_HUP)
                    {
                        LOGI("%s end op\n", __func__);
                        s_hfp_status_mach = HFP_STATUS_WAIT_DONE;
                    }
                    break;

                default:
                    break;
                }
#endif
            }
        }
        break;
        case BK_HF_CLIENT_CNUM_EVT:
        {
            LOGI("+CNUM: HFP subscriber number info, type:%d, number:%s \n", param->cnum.type, param->cnum.number);
        }
        break;
        case BK_HF_CLIENT_BSIR_EVT:
        {
            LOGI("+BSIR: HFP In-band Ring tone staus: %d\n", param->bsir.state);
        }
        break;
        case BK_HF_CLIENT_BINP_EVT:
        {
            LOGI("+BINP: HFP last voice tag record: %s \n", param->binp.number);
        }
        break;
        case BK_HF_CLIENT_RING_IND_EVT:
        {
            LOGI("RING HPF incoming call ind evt\n");
        }
        break;
        default:
            LOGW("Invalid HFP client event: %d\r\n", event);
            break;
    }
}

void hfp_demo_vr(uint8_t enable)
{
    if(enable)
    {
        bk_bt_hf_client_start_voice_recognition(hfp_profile_peer_addr);
    }
    else
    {
        bk_bt_hf_client_stop_voice_recognition(hfp_profile_peer_addr);
    }
}

void hfp_demo_dial(uint8_t enable, uint8_t *num)
{
    if(enable)
    {
        bk_bt_hf_client_dial(hfp_profile_peer_addr, (const char *)num);
    }
    else
    {
        bk_bt_hf_client_reject_call(hfp_profile_peer_addr);
    }
}

void hfp_demo_answer(uint8_t accept)
{
    if(accept)
    {
        bk_bt_hf_client_answer_call(hfp_profile_peer_addr);
    }
    else
    {
        bk_bt_hf_client_reject_call(hfp_profile_peer_addr);
    }
}

void hfp_demo_cust_cmd(uint8_t *cmd)
{
    LOGI("%s len %d\n", __func__, strlen((char *)cmd));
    bk_bt_hf_client_send_custom_cmd(hfp_profile_peer_addr, (const char *)cmd);
}

void bt_audio_hf_demo_main(void *arg)
{
    while (1)
    {
        bk_err_t err;
        bt_audio_hf_demo_msg_t msg;

        err = rtos_pop_from_queue(&bt_audio_hf_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == err)
        {
            switch (msg.type)
            {
                case BT_AUDIO_VOICE_START_MSG:
                {
                    LOGI("BT_AUDIO_VOICE_START_MSG \r\n");


                    if (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec)
                    {
                        bk_sbc_decoder_init(&bt_audio_hf_sbc_decoder);
                        sbc_encoder_init(&bt_audio_hf_sbc_encoder, 16000, 1);
                        sbc_encoder_ctrl(&bt_audio_hf_sbc_encoder, SBC_ENCODER_CTRL_CMD_SET_MSBC_ENCODE_MODE, (uint32_t)NULL);
                    }

                    if(ring_buffer_particle_init(&s_hfp_sco_spk_data_rb, 1024) < 0)
                    {
                        LOGE("%s rb init err !!!\n", __func__);
                        break;
                    }

                    hf_auido_start = 1;
#if !HF_REMOTE_ROLLBACK_TEST || HF_LOCAL_ROLLBACK_TEST
                    mic_task_init();
                    speaker_task_init();
#endif
                    LOGI("hfp audio init ok\r\n");
                }
                break;

                case BT_AUDIO_VOICE_STOP_MSG:
                {
                    LOGI("BT_AUDIO_VOICE_STOP_MSG \r\n");

                    ring_buffer_particle_deinit(&s_hfp_sco_spk_data_rb);

                    if(hf_speaker_thread_handle || hf_mic_thread_handle)
                    {
                        hf_auido_start = 0;
                        if (kNoErr != rtos_init_semaphore(&hf_mic_speaker_exit_sema, 1))
                        {
                            LOGE("init sema fail, %d \n", __LINE__);
                        }
                    }

                    if(hf_mic_thread_handle)
                    {
                        LOGI("%s wait mic thread end\n", __func__);
                        if (hf_mic_speaker_exit_sema)
                        {
                            rtos_get_semaphore(&hf_mic_speaker_exit_sema, BEKEN_WAIT_FOREVER);
                        }
                        LOGI("%s thread end !!!\n", __func__);
                        hf_mic_thread_handle = NULL;
                    }

                    if(hf_speaker_thread_handle)
                    {
                        if (hf_speaker_sema)
                        {
                            rtos_set_semaphore(&hf_speaker_sema);
                        }

                        LOGI("%s wait spk thread end\n", __func__);
                        if (hf_mic_speaker_exit_sema)
                        {
                            rtos_get_semaphore(&hf_mic_speaker_exit_sema, BEKEN_WAIT_FOREVER);
                        }
                        LOGI("%s thread end !!!\n", __func__);
                        hf_speaker_thread_handle = NULL;
                    }

                    if (hf_mic_speaker_exit_sema)
                    {
                        rtos_deinit_semaphore(&hf_mic_speaker_exit_sema);
                        hf_mic_speaker_exit_sema = NULL;
                    }
                }
                break;

                case BT_AUDIO_VOICE_IND_MSG:
                {
                    bk_err_t ret = BK_OK;
                    uint8 *fb = (uint8_t *)msg.data;
                    uint16_t r_len = 0;
                    uint16_t packet_len = SCO_CVSD_SAMPLES_PER_FRAME * 2;
                    uint8_t packet_num = 4;
                    // LOGI("-->len %d, %x %x %x\r\n",msg.len,fb[0], fb[1], fb[2]);

                    if (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec)
                    {
                        fb += 2; //Skip Synchronization Header
                        ret = bk_sbc_decoder_frame_decode(&bt_audio_hf_sbc_decoder, fb, msg.len - 2);
//                        LOGI("sbc decod %d \n", ret);
                        if (ret < 0)
                        {
                            LOGE("msbc decode fail, ret:%d\n", ret);
                        }
                        else
                        {
                            ret = BK_OK;
                            fb = (uint8_t*)bt_audio_hf_sbc_decoder.pcm_sample;
                            packet_len = r_len = SCO_MSBC_SAMPLES_PER_FRAME*2;
                            packet_num = 4;
                        }
                    }
                    else
                    {
                        packet_len = r_len = SCO_CVSD_SAMPLES_PER_FRAME * 2;
                        packet_num = 8;

                        if(r_len != msg.len)
                        {
                            LOGE("%s len not match %d %d\n", __func__, r_len, msg.len);
                        }
                    }

                    if(ret == BK_OK)
                    {
#if HF_REMOTE_ROLLBACK_TEST
                        (void)(packet_num);
                        os_memcpy(hf_speaker_buffer + hf_speaker_data_count, fb, r_len);
                        hf_speaker_data_count += r_len;
                        while (hf_speaker_data_count >= packet_len)
                        {
                            if (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec)
                            {
                                int32_t produced = sbc_encoder_encode(&bt_audio_hf_sbc_encoder, (int16_t *)(hf_speaker_buffer));
//                                    LOGI("[send_mic_data_to_air_msbc]  %d \r\n",produced);
                                bk_bt_hf_client_voice_out_write(hfp_peer_addr, (uint8_t *)&bt_audio_hf_sbc_encoder.stream [ -2 ], produced + 2);
                            }else
                            {
                                bk_bt_hf_client_voice_out_write(hfp_peer_addr, hf_speaker_buffer, packet_len);
                            }
                            hf_speaker_data_count -= packet_len;
                            os_memmove(hf_speaker_buffer, hf_speaker_buffer + packet_len, hf_speaker_data_count);
                        }
#else

                        if(ring_buffer_particle_write(&s_hfp_sco_spk_data_rb, fb, r_len) < 0)
                        {
                            LOGE("%s rb write err %d %d %d\n", __func__, r_len, ring_buffer_particle_len(&s_hfp_sco_spk_data_rb), msg.len);
                        }

						if (hf_speaker_sema)
						{
							rtos_set_semaphore(&hf_speaker_sema);
						}
#endif
                    }
					else
                    {
//                        LOGE("write spk data fail \r\n");
                    }

                    os_free(msg.data);
                }
                break;

                default:
                    break;
            }
        }
    }

    rtos_deinit_queue(&bt_audio_hf_demo_msg_que);
    bt_audio_hf_demo_msg_que = NULL;
    bt_audio_hf_demo_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

int bt_audio_hf_demo_task_init(void)
{
    bk_err_t ret = BK_OK;
    if ((!bt_audio_hf_demo_thread_handle) && (!bt_audio_hf_demo_msg_que))
    {
        ret = rtos_init_queue(&bt_audio_hf_demo_msg_que,
                              "bt_audio_hf_demo_msg_que",
                              sizeof(bt_audio_hf_demo_msg_t),
                              BT_AUDIO_HF_DEMO_MSG_COUNT);
        if (ret != kNoErr)
        {
            LOGI("bt_audio hf demo msg queue failed \r\n");
            return BK_FAIL;
        }

        ret = rtos_create_thread(&bt_audio_hf_demo_thread_handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "bt_audio_hf_demo",
                                 (beken_thread_function_t)bt_audio_hf_demo_main,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGI("bt_audio hf demo task fail \r\n");
            rtos_deinit_queue(&bt_audio_hf_demo_msg_que);
            bt_audio_hf_demo_msg_que = NULL;
            bt_audio_hf_demo_thread_handle = NULL;
        }

        return kNoErr;
    }
    else
    {
        return kInProgressErr;
    }
}

int hfp_hf_demo_init(uint8_t msbc_supported)
{
    int ret = kNoErr;

    LOGI("%s\r\n", __func__);

    bt_audio_hf_demo_task_init();

    ret = bk_bt_hf_client_register_callback(bk_bt_app_hfp_client_cb);
    if (ret)
    {
        LOGI("%s bk_bt_hf_client_register_callback err %d\n", __func__, ret);
        return -1;
    }

    ret = bk_bt_hf_client_init(msbc_supported);
    if (ret)
    {
        LOGI("%s bk_bt_hf_client_init err %d\n", __func__, ret);
        return -1;
    }

    ret = bk_bt_hf_client_register_data_callback(bt_audio_hfp_client_voice_data_ind);
    if (ret)
    {
        LOGI("%s bk_bt_hf_client_register_data_callback err %d\n", __func__, ret);
        return -1;
    }

    return ret;
}


static int mic_task_init()
{
    bk_err_t ret = BK_OK;
    if (!hf_mic_thread_handle)
    {
        ret = rtos_create_thread(&hf_mic_thread_handle,
                                 HF_MIC_THREAD_PRI,
                                 "bt_hf_mic",
                                 (beken_thread_function_t)mic_task,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGE("mic task fail \r\n");
        }

        return kNoErr;
    }
    else
    {
        LOGE("%s mic task already exist \r\n", __func__);
        return kInProgressErr;
    }

    return kNoErr;
}


static void mic_task(void *arg)
{
    int32_t ret = 0;

    audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();

    cfg.nChans = 1;
    cfg.sampRate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    cfg.adc_gain = 0x2d;

    LOGI("%s wait a2dp task end\n", __func__);
    extern int32_t wait_a2dp_speaker_task_end(void);
    wait_a2dp_speaker_task_end();
    s_audio_record_obj = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);

    if(!s_audio_record_obj)
    {
        LOGE("%s create audio record err\n", __func__);

        goto end;
    }

    if((ret = audio_record_open(s_audio_record_obj)) != 0)
    {
        LOGE("%s open audio record err\n", __func__, ret);

        goto end;
    }


    LOGI("%s init success!! \r\n", __func__);
    hf_mic_data_count = 0;
    uint16_t packet_len = (bt_audio_hfp_hf_codec == CODEC_VOICE_CVSD ? SCO_CVSD_SAMPLES_PER_FRAME : SCO_MSBC_SAMPLES_PER_FRAME);
    int read_size = packet_len * 2 * 3;
    while (hf_auido_start)
    {
        if (hf_mic_data_count+read_size < sizeof(hf_mic_sco_data))
        {
            int size = audio_record_read_data(s_audio_record_obj, (char *)(hf_mic_sco_data + hf_mic_data_count), read_size);
            if (size > 0)
            {
#if HF_LOCAL_ROLLBACK_TEST
                mic_read_size = size;
                os_memcpy(hf_speaker_buffer, hf_mic_sco_data, size);
                if (hf_speaker_sema)
                {
                    rtos_set_semaphore(&hf_speaker_sema);
                }
#else
                read_size = packet_len * 2 * 2;
//                LOGI("raw_stream_read size: %d \n", size);
                hf_mic_data_count += size;
                uint16_t send_len = packet_len * 2;
                uint8_t i = 0;
                while (hf_mic_data_count >= send_len)
                {
                    if (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec)
                    {
                        int32_t produced = sbc_encoder_encode(&bt_audio_hf_sbc_encoder, (int16_t *)(hf_mic_sco_data + send_len * i));
//                        LOGI("[send_mic_data_to_air_msbc]  %d \r\n",produced);
                        bk_bt_hf_client_voice_out_write(hfp_peer_addr, (uint8_t *)&bt_audio_hf_sbc_encoder.stream [ -2 ], produced + 2);
                    }
                    else
                    {
                        bk_bt_hf_client_voice_out_write(hfp_peer_addr, hf_mic_sco_data + send_len * i, send_len);
                    }
                    i++;
                    hf_mic_data_count -= send_len;
                }
                if (hf_mic_data_count)
                {
                    os_memmove(hf_mic_sco_data, hf_mic_sco_data + send_len * i, hf_mic_data_count);
                }
#endif
            }
        }
        else
        {
            LOGE("MIC BUFFER FULL \r\n");
            hf_mic_data_count = 0;
        }
    }

end:;

    LOGI("%s exit start!! \r\n", __func__);
    ret = audio_record_close(s_audio_record_obj);

    if(ret)
    {
        LOGE("%s close audio record err %d\n", __func__, ret);
    }

    ret = audio_record_destroy(s_audio_record_obj);

    if(ret)
    {
        LOGE("%s destroy audio record err %d\n", __func__, ret);
    }

    s_audio_record_obj = NULL;

    LOGI("%s end!! %d\r\n", __func__, hf_auido_start);

    if (hf_mic_speaker_exit_sema)
    {
        rtos_set_semaphore(&hf_mic_speaker_exit_sema);
    }

    rtos_delete_thread(NULL);
}

static int speaker_task_init()
{
    bk_err_t ret = BK_OK;
    if (!hf_speaker_thread_handle)
    {
        ret = rtos_create_thread(&hf_speaker_thread_handle,
                                 HF_SPEAKER_THREAD_PRI,
                                 "bt_hf_speaker",
                                 (beken_thread_function_t)speaker_task,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGE("speaker task fail \r\n");
        }

        return kNoErr;
    }
    else
    {
        LOGE("%s speaker task already exist \r\n", __func__);
        return kInProgressErr;
    }
}

static void speaker_task(void *arg)
{
    bk_err_t ret = 0;

    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();

    cfg.nChans = 1;
    cfg.sampRate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    cfg.volume = 0x2d;
    cfg.frame_size = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size = cfg.frame_size * 2;
    LOGI("%s wait a2dp task end\n", __func__);
    extern int32_t wait_a2dp_speaker_task_end(void);
    wait_a2dp_speaker_task_end();

    s_audio_play_obj = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);

    if(!s_audio_play_obj)
    {
        LOGE("%s create audio play err\n", __func__);

        goto end;
    }

    if((ret = audio_play_open(s_audio_play_obj)) != 0)
    {
        LOGE("%s open audio play err\n", __func__, ret);

        goto end;
    }

    if (hf_speaker_sema == NULL)
    {
        if (kNoErr != rtos_init_semaphore(&hf_speaker_sema, 1))
        {
            LOGE("init sema fail, %d \n", __LINE__);
        }
    }

    LOGI("%s init hfp success!! \r\n", __func__);

    uint8_t tmp_recv[240] = {0};
    uint32_t tmp_recv_index = 0;
    uint32_t already_len = 0;

    uint16_t packet_len = (bt_audio_hfp_hf_codec == CODEC_VOICE_CVSD ? SCO_CVSD_SAMPLES_PER_FRAME * 16 : SCO_MSBC_SAMPLES_PER_FRAME * 8);
    while (hf_auido_start)
    {
        rtos_get_semaphore(&hf_speaker_sema, BEKEN_WAIT_FOREVER);
#if HF_LOCAL_ROLLBACK_TEST
        (void)(packet_len);
        int size = audio_play_write_data(s_audio_play_obj, (char *)hf_speaker_buffer, mic_read_size);
#else

        int size = 0;

        while(ring_buffer_particle_len(&s_hfp_sco_spk_data_rb) >= 100)
        {
            uint32_t write_len = 0;
            already_len = 0;
            ring_buffer_particle_read(&s_hfp_sco_spk_data_rb, tmp_recv + tmp_recv_index, sizeof(tmp_recv) - tmp_recv_index, &already_len);
            tmp_recv_index += already_len;
            write_len = tmp_recv_index - (tmp_recv_index % 2);

            size = audio_play_write_data(s_audio_play_obj, (char *)tmp_recv, write_len);

            if (size <= 0)
            {
                LOGE("audio_play_write_data size err: %d %d!!!\n", size, write_len);
                break;
            }
            else
            {
                //LOGI("audio_play_write_data size: %d \n", size);
            }

            os_memmove(tmp_recv, tmp_recv + write_len, tmp_recv_index - write_len);
            tmp_recv_index -= write_len;
        }

#endif
    }
end:;
    LOGI("%s hfp exit start!! \r\n", __func__);

    ret = audio_play_close(s_audio_play_obj);

    if(ret)
    {
        LOGE("%s close audio play err %d\n", __func__, ret);
    }

    ret = audio_play_destroy(s_audio_play_obj);

    if(ret)
    {
        LOGE("%s destroy audio play err %d\n", __func__, ret);
    }

    s_audio_play_obj = NULL;

    if (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec)
    {
        bk_sbc_decoder_deinit();
    }

    LOGI("%s hfp end!!\r\n", __func__);

    rtos_deinit_semaphore(&hf_speaker_sema);
    hf_speaker_sema = NULL;

    if (hf_mic_speaker_exit_sema)
    {
        rtos_set_semaphore(&hf_mic_speaker_exit_sema);
    }

    rtos_delete_thread(NULL);
}
int32_t wait_hfp_speaker_mic_task_end(void)
{
    while(hf_speaker_thread_handle || hf_mic_thread_handle)
    {
        rtos_delay_milliseconds(20);
    }
    return 0;
}
