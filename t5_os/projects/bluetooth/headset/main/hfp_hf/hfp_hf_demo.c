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
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "raw_stream.h"
#include "onboard_mic_stream.h"
#include "onboard_speaker_stream.h"

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


static uint8_t bt_audio_hfp_hf_codec = CODEC_VOICE_CVSD;
static uint8_t hfp_peer_addr [ 6 ] = {0};

static sbcdecodercontext_t bt_audio_hf_sbc_decoder;
static SbcEncoderContext bt_audio_hf_sbc_encoder;
static beken_queue_t bt_audio_hf_demo_msg_que = NULL;
static beken_thread_t bt_audio_hf_demo_thread_handle = NULL;

static uint8_t hf_mic_sco_data [ 1024 ] = {0};
static uint16_t hf_mic_data_count = 0;

volatile uint8_t hf_auido_start = 0;
static uint8_t hf_speaker_buffer [ 1024 ] = {0};
static uint16_t hf_speaker_data_count = 0;

static beken_thread_t hf_speaker_thread_handle = NULL;
static beken_thread_t hf_mic_thread_handle = NULL;
static beken_semaphore_t hf_speaker_sema = NULL;

#if HF_LOCAL_ROLLBACK_TEST
static uint16_t mic_read_size = 0;
#endif

static void speaker_task(void *arg);
static int speaker_task_init();
static void mic_task(void *arg);
static int mic_task_init();

int bt_audio_hf_demo_task_init(void);


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
    LOGI("%s event: %d\r\n", __func__, event);

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
                os_memcpy(hfp_peer_addr, param->audio_state.remote_bda, 6);
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
                LOGI("HFP client connect to peer address: %02x:%02x:%02x:%02x:%02x:%02x \n", param->conn_state.remote_bda [ 0 ], param->conn_state.remote_bda [ 1 ],
                     param->conn_state.remote_bda [ 2 ], param->conn_state.remote_bda [ 3 ],
                     param->conn_state.remote_bda [ 4 ], param->conn_state.remote_bda [ 5 ]);
            }
            else if (param->conn_state.state == BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTED)
            {
                LOGI("HFP disconnected \n");
                LOGI("HFP disconnect peer address: %02x:%02x:%02x:%02x:%02x:%02x \n", param->conn_state.remote_bda [ 0 ], param->conn_state.remote_bda [ 1 ],
                     param->conn_state.remote_bda [ 2 ], param->conn_state.remote_bda [ 3 ],
                     param->conn_state.remote_bda [ 4 ], param->conn_state.remote_bda [ 5 ]);
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
            }
            else if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_MIC)
            {
                LOGI("+VGM: HPF Microphone gain: %d \n", param->volume_control.volume);
            }
        }
        break;
        case BK_HF_CLIENT_AT_RESPONSE_EVT:
        {
            if (param->at_response.code == BK_HF_AT_RESPONSE_CODE_CME)
            {
                LOGI("+CME ERROR: HFP AG error code: %d \n", param->at_response.cme);
            }
            else
            {
                LOGI("AT extended response ind code :%d \n", param->at_response.code);
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
                    hf_auido_start = 0;
                    if (hf_speaker_sema)
                    {
                        rtos_set_semaphore(&hf_speaker_sema);
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
                        os_memcpy(hf_speaker_buffer + hf_speaker_data_count, fb, r_len);
                        hf_speaker_data_count += r_len;
                        if (hf_speaker_data_count >= packet_len * packet_num)
                        {
                            if (hf_speaker_sema)
                            {
                                rtos_set_semaphore(&hf_speaker_sema);
                            }
                            hf_speaker_data_count -= packet_len * packet_num;
                        }
#endif
                    }else
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
        return kInProgressErr;
    }

    return kNoErr;
}


static void mic_task(void *arg)
{
    audio_pipeline_handle_t record_pipeline;
    audio_element_handle_t raw_read, onboard_mic;

    audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
    CHECK_NULL(record_pipeline);

    onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    onboard_mic_cfg.adc_cfg.mic_gain = 0x3d;
    onboard_mic_cfg.adc_cfg.samp_rate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    onboard_mic = onboard_mic_stream_init(&onboard_mic_cfg);
    CHECK_NULL(onboard_mic);

    raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_read_cfg.type = AUDIO_STREAM_READER;
    raw_read = raw_stream_init(&raw_read_cfg);
    CHECK_NULL(raw_read);

    if (BK_OK != audio_pipeline_register(record_pipeline, onboard_mic, "onboard_mic"))
    {
        LOGE("register element fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_register(record_pipeline, raw_read, "raw_read"))
    {
        LOGE("register element fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]){"onboard_mic", "raw_read"}, 2))
    {
        LOGE("pipeline link fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_run(record_pipeline))
    {
        LOGE("record_pipeline run fail, %d \n", __LINE__);
        return;
    }

    LOGI("%s init success!! \r\n", __func__);
    hf_mic_data_count = 0;
    uint16_t packet_len = (bt_audio_hfp_hf_codec == CODEC_VOICE_CVSD ? SCO_CVSD_SAMPLES_PER_FRAME : SCO_MSBC_SAMPLES_PER_FRAME);
    int read_size = packet_len * 2 * 3;
    while (hf_auido_start)
    {
        if (hf_mic_data_count+read_size < sizeof(hf_mic_sco_data))
        {
            int size = raw_stream_read(raw_read, (char *)(hf_mic_sco_data + hf_mic_data_count), read_size);
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
                    }else
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

    if (BK_OK != audio_pipeline_stop(record_pipeline))
    {
        LOGE("record_pipeline stop fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline))
    {
        LOGE("record_pipeline wait stop fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_terminate(record_pipeline))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_unregister(record_pipeline, onboard_mic))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_unregister(record_pipeline, raw_read))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_deinit(record_pipeline))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_element_deinit(onboard_mic))
    {
        LOGE("element deinit fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_element_deinit(raw_read))
    {
        LOGE("element deinit fail, %d \n", __LINE__);
        return;
    }

    LOGI("%s end!! %d\r\n", __func__, hf_auido_start);

    hf_mic_thread_handle = NULL;
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
        return kInProgressErr;
    }
}

static void speaker_task(void *arg)
{
    audio_pipeline_handle_t  play_pipeline;
    audio_element_handle_t raw_write, onboard_speaker;

    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
    CHECK_NULL(play_pipeline);

    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    raw_write = raw_stream_init(&raw_write_cfg);
    CHECK_NULL(raw_write);

    onboard_speaker_stream_cfg_t onboard_speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
    onboard_speaker_cfg.samp_rate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    onboard_speaker = onboard_speaker_stream_init(&onboard_speaker_cfg);
    CHECK_NULL(onboard_speaker);

    if (BK_OK != audio_pipeline_register(play_pipeline, raw_write, "raw_write"))
    {
        LOGE("register element fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_register(play_pipeline, onboard_speaker, "onboard_speaker"))
    {
        LOGE("register element fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_link(play_pipeline, (const char *[]){"raw_write", "onboard_speaker"}, 2))
    {
        LOGE("pipeline link fail, %d \n", __LINE__);
        return;
    }

    if (hf_speaker_sema == NULL)
    {
        if (kNoErr != rtos_init_semaphore(&hf_speaker_sema, 1))
        {
            LOGE("init sema fail, %d \n", __LINE__);
        }
    }

    if (BK_OK != audio_pipeline_run(play_pipeline))
    {
        LOGE("play_pipeline run fail, %d \n", __LINE__);
        return;
    }

    LOGI("%s init success!! \r\n", __func__);
    uint16_t packet_len = (bt_audio_hfp_hf_codec == CODEC_VOICE_CVSD ? SCO_CVSD_SAMPLES_PER_FRAME * 16 : SCO_MSBC_SAMPLES_PER_FRAME * 8);
    while (hf_auido_start)
    {
        rtos_get_semaphore(&hf_speaker_sema, BEKEN_WAIT_FOREVER);
#if HF_LOCAL_ROLLBACK_TEST
        (void)(packet_len);
        int size = raw_stream_write(raw_write, (char *)hf_speaker_buffer, mic_read_size);
#else
        int size = raw_stream_write(raw_write, (char *)hf_speaker_buffer, packet_len);
#endif
        if (size <= 0)
        {
            LOGE("raw_stream_write size: %d \n", size);
            break;
        }
        else
        {
            //LOGI("raw_stream_write size: %d \n", size);
        }
    }
    LOGI("%s exit start!! \r\n", __func__);
    if (BK_OK != audio_pipeline_stop(play_pipeline))
    {
        LOGE("play_pipeline stop fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline))
    {
        LOGE("play_pipeline wait stop fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_terminate(play_pipeline))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_unregister(play_pipeline, onboard_speaker))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_pipeline_unregister(play_pipeline, raw_write))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_pipeline_deinit(play_pipeline))
    {
        LOGE("pipeline terminate fail, %d \n", __LINE__);
        return;
    }
    if (BK_OK != audio_element_deinit(onboard_speaker))
    {
        LOGE("element deinit fail, %d \n", __LINE__);
        return;
    }

    if (BK_OK != audio_element_deinit(raw_write))
    {
        LOGE("element deinit fail, %d \n", __LINE__);
        return;
    }

    LOGI("%s end!!\r\n", __func__);

    hf_speaker_thread_handle = NULL;
    rtos_deinit_semaphore(&hf_speaker_sema);
    hf_speaker_sema = NULL;
    rtos_delete_thread(NULL);

}
