#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "headset_user_config.h"

#include "a2dp_sink_demo.h"

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_a2dp_types.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

#include <driver/sbc_types.h>
//#include <driver/aud_types.h>
//#include <driver/aud.h>
#ifdef CONFIG_A2DP_AUDIO
//#include "aud_intf.h"
#endif
#include <driver/sbc.h>
#include "ring_buffer_node.h"
#include "components/bluetooth/bk_dm_avrcp.h"
#include "bluetooth_storage.h"
#if CONFIG_WIFI_COEX_SCHEME
#include "bk_coex_ext.h"
#endif
#include "bt_manager.h"

#include "bk_gpio.h"

#if CONFIG_AAC_DECODER
#include "modules/aacdec.h"
#include "bk_aac_decoder.h"
#endif

#include <driver/aud_dac.h>
#include <driver/aud_dac_types.h>

#include "audio_play.h"

#include "mpeg4_latm_dec.h"

#define TAG "a2dp_sink"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            LOGI("CHECK_NULL fail \n");\
            return;\
        }\
    } while(0)


#define CODEC_AUDIO_SBC                      0x00U
#define CODEC_AUDIO_AAC                      0x02U

#define A2DP_SBC_CHANNEL_MONO                        0x08U
#define A2DP_SBC_CHANNEL_DUAL                        0x04U
#define A2DP_SBC_CHANNEL_STEREO                      0x02U
#define A2DP_SBC_CHANNEL_JOINT_STEREO                0x01U


#define A2DP_CACHE_BUFFER_SIZE  ((CONFIG_A2DP_CACHE_FRAME_NUM + 2) * 1024)
#define A2DP_SBC_FRAME_BUFFER_MAX_SIZE  128   /**< A2DP SBC encoded frame maximum size */
#define A2DP_AAC_FRAME_BUFFER_MAX_SIZE  1024  /**< A2DP AAC encoded frame maximum size */

#define A2DP_SPEAKER_THREAD_PRI             BEKEN_DEFAULT_WORKER_PRIORITY-2
#define A2DP_SPEAKER_WRITE_SBC_FRAME_NUM    7
#define A2DP_SBC_MAX_FRAME_NUMS             0xF*5*1
#define A2DP_SBC_FRAME_HEADER_LEN           13

#if CONFIG_AAC_DECODER
#define A2DP_ACC_SINGLE_CHANNEL_MAX_FRAM_SIZE   768   //1024(samples)/48k/s(sample rate)*288k/s (max bitrate)/8
#define A2DP_AAC_MAX_FRAME_NUMS                 5     //21ms/frame*5
#endif

#define AVRCP_PASSTHROUTH_CMD_TIMEOUT                     2000

static int speaker_task_init();
static void speaker_task(void *arg);


enum
{
    BT_AUDIO_MSG_NULL = 0,
    BT_AUDIO_D2DP_START_MSG = 1,
    BT_AUDIO_D2DP_STOP_MSG = 2,
    BT_AUDIO_D2DP_DATA_IND_MSG = 3,
    BT_AUDIO_D2DP_SEND_DATA_2_SPK_MSG = 4,
    BT_AUDIO_WIFI_STATE_UPDATE_MSG = 5,
};


typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} bt_audio_sink_demo_msg_t;

typedef struct
{
    uint8_t wifi_state;
    uint8_t a2dp_state;
    uint8_t avrcp_state;
    beken2_timer_t avrcp_connect_tmr;
} bt_env_s;


static bk_a2dp_mcc_t bt_audio_a2dp_sink_codec = {0};


static beken_queue_t bt_audio_sink_demo_msg_que = NULL;
static beken_thread_t bt_audio_sink_demo_thread_handle = NULL;

#ifdef CONFIG_A2DP_AUDIO
static sbcdecodercontext_t bt_audio_sink_sbc_decoder;
#if CONFIG_AAC_DECODER
static aacdecodercontext_t bt_audio_sink_aac_decoder;
#endif

static RingBufferNodeContext s_a2dp_frame_nodes;
static uint8_t s_spk_is_started = 0;
static uint16_t frame_length = 0;
static uint8_t *p_cache_buff = NULL;

static beken_thread_t a2dp_speaker_thread_handle = NULL;
static beken_semaphore_t a2dp_speaker_sema = NULL;
static beken_semaphore_t a2dp_speaker_exit_sema = NULL;
//static beken_timer_t a2dp_speaker_tmr = {0};


#endif
static uint8_t s_a2dp_vol = DEFAULT_A2DP_VOLUME;//0~0x7f
static uint16_t s_tg_current_registered_noti;

static bt_env_s s_bt_env;
#if CONFIG_WIFI_COEX_SCHEME
static coex_to_bt_func_p_t s_coex_to_bt_func = {0};
#endif

static beken_semaphore_t s_bt_api_event_cb_sema = NULL;
static beken_semaphore_t s_bt_avrcp_event_cb_sema = NULL;

static audio_play_t *s_audio_play_obj;

static bk_err_t bk_bt_dac_set_gain(uint8_t gain)
{
    if(s_audio_play_obj)
    {
        LOGI("%s set gain 0x%x\n", __func__, gain);
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

void avrcp_connect_timer_hdl(void *param, unsigned int ulparam)
{
    rtos_deinit_oneshot_timer(&s_bt_env.avrcp_connect_tmr);
    if (0 == s_bt_env.avrcp_state)
    {
        bk_bt_avrcp_connect(bt_manager_get_connected_device());
    }
}

static void bk_bt_start_avrcp_connect(void)
{
    if (!rtos_is_oneshot_timer_init(&s_bt_env.avrcp_connect_tmr))
    {
        rtos_init_oneshot_timer(&s_bt_env.avrcp_connect_tmr, 300, (timer_2handler_t)avrcp_connect_timer_hdl, NULL, 0);
        rtos_start_oneshot_timer(&s_bt_env.avrcp_connect_tmr);
    }
}

static bk_err_t one_spk_frame_played_cmpl_handler(unsigned int size)
{
    bt_audio_sink_demo_msg_t demo_msg;
    int rc = -1;

    if (bt_audio_sink_demo_msg_que == NULL)
    {
        return BK_OK;
    }

    demo_msg.type = BT_AUDIO_D2DP_SEND_DATA_2_SPK_MSG;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_sink_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);

    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);
    }
    return BK_OK;
}

void bt_audio_sink_demo_main(void *arg)
{
    uint8_t recon_addr[6] = {0};
    if ((bluetooth_storage_get_newest_linkkey_info(recon_addr,NULL)) < 0)
    {
		LOGI("%s can't find linkkey info\n", __func__);
        bt_manager_set_mode(BT_MNG_MODE_PAIRING);
    }
    else
    {
		LOGI("%s find addr\n", __func__);
        if(bluetooth_storage_find_volume_by_addr(recon_addr, &s_a2dp_vol) < 0)
        {
            s_a2dp_vol = DEFAULT_A2DP_VOLUME; //default vol
        }

        if(s_a2dp_vol == 0)
		{
		    s_a2dp_vol = DEFAULT_A2DP_VOLUME;
		}

        LOGI("initial volume %d %02x:%02x:%02x:%02x:%02x:%02x\r\n",s_a2dp_vol,
                        recon_addr[5],
                        recon_addr[4],
                        recon_addr[3],
                        recon_addr[2],
                        recon_addr[1],
                        recon_addr[0]
                        );

        bt_manager_start_reconnect(recon_addr, 1);
    }

    while (1)
    {
        bk_err_t err;
        bt_audio_sink_demo_msg_t msg;

        err = rtos_pop_from_queue(&bt_audio_sink_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == err)
        {
            switch (msg.type)
            {
            case BT_AUDIO_D2DP_START_MSG:
            {
                LOGI("BT_AUDIO_D2DP_START_MSG \r\n");

#ifdef CONFIG_A2DP_AUDIO
                bk_a2dp_mcc_t *p_codec_info = (bk_a2dp_mcc_t *)msg.data;
                if (CODEC_AUDIO_SBC == p_codec_info->type)
                {
                    uint8_t chnl_mode = p_codec_info->cie.sbc_codec.channel_mode;
                    uint8_t chnls = p_codec_info->cie.sbc_codec.channels;
                    uint8_t subbands = p_codec_info->cie.sbc_codec.subbands;
                    uint8_t blocks = p_codec_info->cie.sbc_codec.block_len;
                    uint8_t bitpool = p_codec_info->cie.sbc_codec.bit_pool;
                    if(chnl_mode == A2DP_SBC_CHANNEL_MONO || chnl_mode == A2DP_SBC_CHANNEL_DUAL)
                    {
                         frame_length = 4 + ((4 * subbands * chnls)>>3) + ((blocks*chnls*bitpool+7)>>3);
                    }else if(chnl_mode == A2DP_SBC_CHANNEL_STEREO)
                    {
                        frame_length = 4 + ((4 * subbands * chnls)>>3) + ((blocks*bitpool+7)>>3);
                    }else //A2DP_SBC_CHANNEL_JOINT_STEREO
                    {
                        frame_length = 4 + ((4 * subbands * chnls)>>3) + ((subbands+blocks*bitpool+7)>>3);
                    }

                    //get_sbc_encoder_len(p_codec_info);

                    LOGI("cm:%d, c:%d, s:%d, b:%d, bi:%d, frame_length:%d \n",chnl_mode, chnls, subbands,blocks, bitpool, frame_length);
                    bk_sbc_decoder_init(&bt_audio_sink_sbc_decoder);
                    if(p_cache_buff)
                    {
                        LOGE("p_cache_buffer remalloc, error !!");
                    }else
                    {
                        p_cache_buff  = (uint8_t *)os_malloc((frame_length + 2) * A2DP_SBC_MAX_FRAME_NUMS); //max number of frames 4bit
                        if (!p_cache_buff)
                        {
                            LOGE("%s, malloc cache buf failed!!!\r\n", __func__);
                        }
                        else
                        {
                            ring_buffer_node_init(&s_a2dp_frame_nodes, p_cache_buff, frame_length + 2, A2DP_SBC_MAX_FRAME_NUMS);
                        }
                    }
                }
#if (CONFIG_AAC_DECODER)
                else if(CODEC_AUDIO_AAC == p_codec_info->type)
                {
                    uint8_t channle = p_codec_info->cie.aac_codec.channels;
                    uint32_t sample_rate = p_codec_info->cie.aac_codec.sample_rate;
                    int ret = bk_aac_decoder_init(&bt_audio_sink_aac_decoder, sample_rate, channle);
                    if(ret < 0)
                    {
                        LOGE("aac deocder init fail \n");
                        return;
                    }
                	frame_length = A2DP_ACC_SINGLE_CHANNEL_MAX_FRAM_SIZE*channle;
                	LOGI("ch:%d, frame_length:%d \n", channle, frame_length);
                    if(p_cache_buff)
                    {
                        LOGE("p_cache_buffer remalloc, error !!");
                    }else
                    {
                        p_cache_buff  = (uint8_t *)os_malloc(frame_length * A2DP_AAC_MAX_FRAME_NUMS); //max number of frames 4bit
                        if (!p_cache_buff)
                        {
                            LOGE("%s, malloc cache buf failed!!!\r\n", __func__);
                        }
                        else
                        {
                            ring_buffer_node_init(&s_a2dp_frame_nodes, p_cache_buff, frame_length, A2DP_AAC_MAX_FRAME_NUMS);
                        }
                    }
                }
#endif
                s_spk_is_started = 1;
                speaker_task_init();
#endif
                os_free(msg.data);
            }
            break;

            case BT_AUDIO_D2DP_DATA_IND_MSG:
            {
#ifdef CONFIG_A2DP_AUDIO

                uint8 *fb = (uint8_t *)msg.data;
                if(s_spk_is_started)
                {
                    if (CODEC_AUDIO_SBC == bt_audio_a2dp_sink_codec.type)
                    {
                        uint8_t payload_header = *fb++;
                        uint8_t frame_num = payload_header&0xF;

                        if(msg.len - 1 != frame_length*frame_num)
                        {
                            //LOGI("recv undef sbc, payload_header %d, payload_len: %d, frame_num:%d %d\n", payload_header, msg.len - 1, frame_num, frame_length);
                        }

                        if((msg.len - 1) % frame_num)
                        {
                            LOGE("%s frame len invalid\n", __func__);
                        }


                        for(uint8_t i = 0; i < frame_num; i++)
                        {
                            if (ring_buffer_node_get_free_nodes(&s_a2dp_frame_nodes))
                            {
                                uint16_t tmp_len = (msg.len - 1) / frame_num;

                                ring_buffer_node_write(&s_a2dp_frame_nodes, fb, tmp_len);
                                fb += tmp_len;
                            }
                            else
                            {
                                LOGI("A2DP frame nodes buffer(sbc) is full %d %d\n", i, frame_num);
                                //os_free(msg.data);
                                break;
                            }
                        }

                    }
#if CONFIG_AAC_DECODER
                    else if(CODEC_AUDIO_AAC == bt_audio_a2dp_sink_codec.type)
                    {
                        // LOGI("-> %d \n", msg.len);
                        uint8_t *inbuf = &fb[9];
                        uint32_t inlen = 0;
                        uint8_t  len   = 255;

                        do
                        {
                            inlen += len = *inbuf++;
                        }
                        while (len == 255);

                        {
                            if (ring_buffer_node_get_free_nodes(&s_a2dp_frame_nodes))
                            {
                                uint8_t *output = 0;
                                uint32_t output_len = 0;

                                if(mpeg4_latm_decode(fb, msg.len, &output, &output_len))
                                {
                                    LOGE("====%s latm decode err, discard it\n", __func__);
                                    os_free(msg.data);
                                    break;
                                }
                                else if(msg.len - (output - fb) != output_len ||
                                                inlen != output_len ||
                                                output != inbuf)
                                {
                                    LOGE("====%s len not match, total %d, type1 offset %d len %d, type2 offset %d len %d\n", __func__, msg.len, output - fb, inbuf - fb, output_len, inlen);
                                }

                                uint8_t *node = ring_buffer_node_get_write_node(&s_a2dp_frame_nodes);

                                os_memcpy(node, &inlen, sizeof(inlen));
                                os_memcpy(node + sizeof(inlen), inbuf, inlen);
                            }
                            else
                            {
                                LOGI("A2DP frame nodes buffer(aac) is full\n");
                                os_free(msg.data);
                                break;
                            }
                        }
                    }
#endif
                    else
                    {
                        LOGE("%s, Unsupported a2dp codec %d \r\n", __func__, bt_audio_a2dp_sink_codec.type);
                    }
                    if(a2dp_speaker_sema)
                    {
                        rtos_set_semaphore(&a2dp_speaker_sema);
                    }
                }
                os_free(msg.data);
#else
                os_free(msg.data);
#endif
            }
            break;

            case BT_AUDIO_D2DP_STOP_MSG:
            {
                LOGI("BT_AUDIO_D2DP_STOP_MSG \r\n");
#ifdef CONFIG_A2DP_AUDIO

                if(a2dp_speaker_thread_handle)
                {
                    s_spk_is_started = 0;
                    if (kNoErr != rtos_init_semaphore(&a2dp_speaker_exit_sema, 1))
                    {
                        LOGE("init sema fail, %d \n", __LINE__);
                    }
                    if(a2dp_speaker_sema)
                    {
                        rtos_set_semaphore(&a2dp_speaker_sema);
                    }
                    LOGI("%s wait thread end\n", __func__);
                    if (a2dp_speaker_exit_sema)
                    {
                        rtos_get_semaphore(&a2dp_speaker_exit_sema, BEKEN_WAIT_FOREVER);
                    }
                    LOGI("%s thread end !!!\n", __func__);
                    a2dp_speaker_thread_handle = NULL;
                    if (a2dp_speaker_exit_sema)
                    {
                        rtos_deinit_semaphore(&a2dp_speaker_exit_sema);
                        a2dp_speaker_exit_sema = NULL;
                    }
                }

#endif

            }
            break;

            case BT_AUDIO_D2DP_SEND_DATA_2_SPK_MSG:
            {

            }
            break;

            case BT_AUDIO_WIFI_STATE_UPDATE_MSG:
            {
                LOGI("BT_AUDIO_WIFI_STATE_UPDATE_MSG \r\n");
                uint8_t reconn_addr[8] = {0};
                bt_manager_get_reconnect_device(reconn_addr);
                if (0 == s_bt_env.wifi_state && BT_STATE_WAIT_FOR_RECONNECT == bt_manager_get_connect_state())
                {
                    bt_manager_start_reconnect(reconn_addr, 1);
                }

                if (s_bt_env.wifi_state && BT_STATE_RECONNECTING == bt_manager_get_connect_state())
                {
                   bk_bt_gap_create_conn_cancel(reconn_addr);
                }
            }
            break;

            default:
                break;
            }
        }
    }

    rtos_deinit_queue(&bt_audio_sink_demo_msg_que);
    bt_audio_sink_demo_msg_que = NULL;
    bt_audio_sink_demo_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

int bt_audio_sink_demo_task_init(void)
{
    bk_err_t ret = BK_OK;

    if ((!bt_audio_sink_demo_thread_handle) && (!bt_audio_sink_demo_msg_que))
    {
        ret = rtos_init_queue(&bt_audio_sink_demo_msg_que,
                              "bt_audio_sink_demo_msg_que",
                              sizeof(bt_audio_sink_demo_msg_t),
                              BT_AUDIO_SINK_DEMO_MSG_COUNT);

        if (ret != kNoErr)
        {
            LOGE("bt_audio sink demo msg queue failed \r\n");
            return BK_FAIL;
        }

        ret = rtos_create_thread(&bt_audio_sink_demo_thread_handle,
                                 A2DP_SINK_DEMO_TASK_PRIORITY,
                                 "bt_audio_sink_demo",
                                 (beken_thread_function_t)bt_audio_sink_demo_main,
                                 4096,
                                 (beken_thread_arg_t)0);

        if (ret != kNoErr)
        {
            LOGE("bt_audio sink demo task fail \r\n");
            rtos_deinit_queue(&bt_audio_sink_demo_msg_que);
            bt_audio_sink_demo_msg_que = NULL;
            bt_audio_sink_demo_thread_handle = NULL;
        }

        return kNoErr;
    }
    else
    {
        return kInProgressErr;
    }
}

void bt_audio_sink_media_data_ind(const uint8_t *data, uint16_t data_len)
{
    bt_audio_sink_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_sink_demo_msg_t));

    if (bt_audio_sink_demo_msg_que == NULL)
    {
        return;
    }

    demo_msg.data = (char *) os_malloc(data_len);

    if (demo_msg.data == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(demo_msg.data, data, data_len);
    demo_msg.type = BT_AUDIO_D2DP_DATA_IND_MSG;
    demo_msg.len = data_len;

    rc = rtos_push_to_queue(&bt_audio_sink_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);

    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);

        if (demo_msg.data)
        {
            os_free(demo_msg.data);
        }
    }
}

void bt_audio_a2dp_sink_suspend_ind(void)
{
    bt_audio_sink_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_sink_demo_msg_t));

    if (bt_audio_sink_demo_msg_que == NULL)
    {
        return;
    }

    demo_msg.type = BT_AUDIO_D2DP_STOP_MSG;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_sink_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);

    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);
    }
}

void bt_audio_a2dp_sink_start_ind(bk_a2dp_mcc_t *codec)
{
    bt_audio_sink_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_sink_demo_msg_t));

    if (bt_audio_sink_demo_msg_que == NULL)
    {
        return;
    }

    demo_msg.data = (char *) os_malloc(sizeof(bk_a2dp_mcc_t));

    if (demo_msg.data == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(demo_msg.data, codec, sizeof(bk_a2dp_mcc_t));
    demo_msg.type = BT_AUDIO_D2DP_START_MSG;
    demo_msg.len = sizeof(bk_a2dp_mcc_t);

    rc = rtos_push_to_queue(&bt_audio_sink_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);

    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);
    }
}

static bk_a2dp_audio_state_t s_audio_state = BK_A2DP_AUDIO_STATE_SUSPEND;

void bk_bt_app_a2dp_sink_cb(bk_a2dp_cb_event_t event, bk_a2dp_cb_param_t *p_param)
{
    LOGI("%s event: %d\r\n", __func__, event);

    bk_a2dp_cb_param_t *a2dp = (bk_a2dp_cb_param_t *)(p_param);

    switch (event)
    {
    case BK_A2DP_PROF_STATE_EVT:
    {
        LOGI("a2dp prof init action %d status %d reason %d\r\n",
                        p_param->a2dp_prof_stat.action, p_param->a2dp_prof_stat.status, p_param->a2dp_prof_stat.reason);

        if (!p_param->a2dp_prof_stat.status)
        {
            if (s_bt_api_event_cb_sema)
            {
                rtos_set_semaphore( &s_bt_api_event_cb_sema );
            }
        }
    }
    break;

    case BK_A2DP_CONNECTION_STATE_EVT:
    {
        uint8_t *bda = a2dp->conn_state.remote_bda;
        LOGI("A2DP connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
                  a2dp->conn_state.state, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);

        if (BK_A2DP_CONNECTION_STATE_DISCONNECTED == a2dp->conn_state.state)
        {
            s_bt_env.a2dp_state = 0;
            if (BK_A2DP_AUDIO_STATE_STARTED == s_audio_state)
            {
                s_audio_state = BK_A2DP_AUDIO_STATE_SUSPEND;
                bt_audio_a2dp_sink_suspend_ind();
            }
        }
        else if (BK_A2DP_CONNECTION_STATE_CONNECTED == a2dp->conn_state.state)
        {
            bt_manager_set_connect_state(BT_STATE_PROFILE_CONNECTED);
            s_bt_env.a2dp_state = 1;
            //bluetooth_storage_update_to_newest(bda);
            //bluetooth_storage_sync_to_flash();
            if (0 == s_bt_env.avrcp_state)
            {
                bk_bt_start_avrcp_connect();
            }
        }
    }
    break;

    case BK_A2DP_AUDIO_STATE_EVT:
    {
        LOGI("A2DP audio state: %d\r\n", a2dp->audio_state.state);

        if (BK_A2DP_AUDIO_STATE_STARTED == a2dp->audio_state.state)
        {
            s_audio_state = a2dp->audio_state.state;
            bt_audio_a2dp_sink_start_ind(&bt_audio_a2dp_sink_codec);
        }
        else if ((BK_A2DP_AUDIO_STATE_SUSPEND == a2dp->audio_state.state) && (BK_A2DP_AUDIO_STATE_STARTED == s_audio_state))
        {
            s_audio_state = a2dp->audio_state.state;
            bt_audio_a2dp_sink_suspend_ind();
        }
    }
    break;

    case BK_A2DP_AUDIO_CFG_EVT:
    {
        bt_audio_a2dp_sink_codec = a2dp->audio_cfg.mcc;
        LOGI("%s, codec_id %d \r\n", __func__, bt_audio_a2dp_sink_codec.type);
    }
    break;

    default:
        LOGW("Invalid A2DP event: %d\r\n", event);
        break;
    }
}

static void gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    switch (event)
    {
        case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        break;

        case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        break;

        case BK_BT_GAP_AUTH_CMPL_EVT:
        break;

        case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
        {
            s_a2dp_vol = DEFAULT_A2DP_VOLUME;
            bluetooth_storage_save_volume(param->link_key_notif.bda, s_a2dp_vol);
        }
        break;

        case BK_BT_GAP_LINK_KEY_REQ_EVT:
        {
        }
        break;

        default:
            break;
    }

}

uint8_t avrcp_remote_bda[6] = {0};

static void bt_av_notify_evt_handler(uint8_t event_id, bk_avrcp_rn_param_t *event_parameter)
{
    switch (event_id)
    {
        /* when track status changed, this event comes */
        case BK_AVRCP_RN_PLAY_STATUS_CHANGE:
        {
            LOGI("Playback status changed: 0x%x\r\n", event_parameter->playback);
            bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_PLAY_STATUS_CHANGE, 0);
        }
        break;

        case BK_AVRCP_RN_TRACK_CHANGE:
        {
            uint64_t track = 0;
            os_memcpy(&track, event_parameter->elm_id, sizeof(event_parameter->elm_id));
            LOGI("track changed: %lld\n", track);
            bk_bt_app_avrcp_ct_get_attr(BK_AVRCP_MEDIA_ATTR_ID_TITLE);
            bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_TRACK_CHANGE, 0);
        }
        break;

        case BK_AVRCP_RN_PLAY_POS_CHANGED:
        {
            LOGI("play pos changed: %d ms\n", event_parameter->play_pos);
            bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_PLAY_POS_CHANGED, 1);
        }
        break;

        case BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE:
        {
            LOGI("avaliable player changed\n");
            bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE, 0);
        }
        break;

        /* others */
        default:
        LOGW("unhandled event: %d\r\n", event_id);
            break;
    }
}

static void bk_bt_app_avrcp_ct_cb(bk_avrcp_ct_cb_event_t event, bk_avrcp_ct_cb_param_t *param)
{
    LOGI("%s event: %d\r\n", __func__, event);

    bk_avrcp_ct_cb_param_t *avrcp = (bk_avrcp_ct_cb_param_t *)(param);

    switch (event)
    {
        case BK_AVRCP_CT_CONNECTION_STATE_EVT:
        {
            uint8_t *bda = avrcp->conn_state.remote_bda;
        LOGI("AVRCP CT connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
                      avrcp->conn_state.connected, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);

        s_bt_env.avrcp_state = avrcp->conn_state.connected;
            if (avrcp->conn_state.connected)
            {
                os_memcpy(avrcp_remote_bda, bda, 6);
                bk_bt_avrcp_ct_send_get_rn_capabilities_cmd(bda);
            }
            else
            {
                os_memset(avrcp_remote_bda, 0x0, sizeof(avrcp_remote_bda));
            }
        }
        break;

        case BK_AVRCP_CT_PASSTHROUGH_RSP_EVT:
        {
            struct avrcp_ct_psth_rsp_param *pm = (typeof(pm))param;

            LOGI("AVRCP psth rsp 0x%x op 0x%x release %d tl %d %02x:%02x:%02x:%02x:%02x:%02x\n",
                       pm->rsp_code,
                       pm->key_code,
                       pm->key_state,
                       pm->tl,
                       pm->remote_bda[5],
                       pm->remote_bda[4],
                       pm->remote_bda[3],
                       pm->remote_bda[2],
                       pm->remote_bda[1],
                       pm->remote_bda[0]);
            if(s_bt_avrcp_event_cb_sema
                && pm->key_state == BK_AVRCP_PT_CMD_STATE_PRESSED //press state set sem
                && (pm->key_code == BK_AVRCP_PT_CMD_PLAY
                ||  pm->key_code == BK_AVRCP_PT_CMD_PAUSE
                ||  pm->key_code == BK_AVRCP_PT_CMD_FORWARD
                ||  pm->key_code == BK_AVRCP_PT_CMD_BACKWARD
                ||  pm->key_code == BK_AVRCP_PT_CMD_VOL_DOWN
                ||  pm->key_code == BK_AVRCP_PT_CMD_VOL_UP) )
            {
                rtos_set_semaphore(&s_bt_avrcp_event_cb_sema);
            }
        }
        break;

        case BK_AVRCP_CT_GET_RN_CAPABILITIES_RSP_EVT:
        {
            LOGI("AVRCP peer supported notification events 0x%x %02x:%02x:%02x:%02x:%02x:%02x\n", avrcp->get_rn_caps_rsp.evt_set.bits,
                            avrcp->get_rn_caps_rsp.remote_bda[5],
                            avrcp->get_rn_caps_rsp.remote_bda[4],
                            avrcp->get_rn_caps_rsp.remote_bda[3],
                            avrcp->get_rn_caps_rsp.remote_bda[2],
                            avrcp->get_rn_caps_rsp.remote_bda[1],
                            avrcp->get_rn_caps_rsp.remote_bda[0]);

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_PLAY_STATUS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_PLAY_STATUS_CHANGE, 0);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_TRACK_CHANGE))
            {
                //bk_bt_app_avrcp_ct_get_attr(BK_AVRCP_MEDIA_ATTR_ID_TITLE);
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_TRACK_CHANGE, 0);
            }
#if 0
            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_PLAY_POS_CHANGED))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_PLAY_POS_CHANGED, 1);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_SYSTEM_STATUS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_SYSTEM_STATUS_CHANGE, 0);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_APP_SETTING_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_APP_SETTING_CHANGE, 0);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE, 0);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_ADDRESSED_PLAYER_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_ADDRESSED_PLAYER_CHANGE, 0);
            }

            if (avrcp->get_rn_caps_rsp.evt_set.bits & (0x01 << BK_AVRCP_RN_UIDS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(avrcp_remote_bda, BK_AVRCP_RN_UIDS_CHANGE, 0);
            }
#endif
        }
        break;

        case BK_AVRCP_CT_CHANGE_NOTIFY_EVT:
        {
            LOGI("AVRCP event notification: %d %02x:%02x:%02x:%02x:%02x:%02x\n", avrcp->change_ntf.event_id,
                            avrcp->change_ntf.remote_bda[5],
                            avrcp->change_ntf.remote_bda[4],
                            avrcp->change_ntf.remote_bda[3],
                            avrcp->change_ntf.remote_bda[2],
                            avrcp->change_ntf.remote_bda[1],
                            avrcp->change_ntf.remote_bda[0]);

            bt_av_notify_evt_handler(avrcp->change_ntf.event_id, &avrcp->change_ntf.event_parameter);
        }
        break;

        case BK_AVRCP_CT_GET_ELEM_ATTR_RSP_EVT:
        {
            struct avrcp_ct_elem_attr_rsp_param *pm = (typeof(pm))param;
            LOGI("%s get elem rsp status %d count %d %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, pm->status, pm->attr_count,
                            pm->remote_bda[5],
                            pm->remote_bda[4],
                            pm->remote_bda[3],
                            pm->remote_bda[2],
                            pm->remote_bda[1],
                            pm->remote_bda[0]);

            for (uint32_t i = 0; i < pm->attr_count; ++i)
            {
                LOGI("%s get elem attr 0x%x charset 0x%x len %d\n", __func__, pm->attr_array[i].attr_id, pm->attr_array[i].attr_text_charset, pm->attr_array[i].attr_length);
            }
        }
        break;

        default:
            LOGW("Invalid AVRCP event: %d\r\n", event);
        break;
    }
}
static void avrcp_tg_cb(bk_avrcp_tg_cb_event_t event, bk_avrcp_tg_cb_param_t *param)
{
    int ret = 0;

    //LOGI("%s event: %d\n", __func__, event);

    switch (event)
    {
    case BK_AVRCP_TG_CONNECTION_STATE_EVT:
    {
        uint8_t status = param->conn_stat.connected;
        uint8_t *bda = param->conn_stat.remote_bda;
        LOGI("%s avrcp tg connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n", __func__,
                  status, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);

        if (!status)
        {
            s_tg_current_registered_noti = 0;
        }
    }
    break;

    case BK_AVRCP_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    {
        LOGI("%s recv abs vol %d %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, param->set_abs_vol.volume,
                        param->set_abs_vol.remote_bda[5],
                        param->set_abs_vol.remote_bda[4],
                        param->set_abs_vol.remote_bda[3],
                        param->set_abs_vol.remote_bda[2],
                        param->set_abs_vol.remote_bda[1],
                        param->set_abs_vol.remote_bda[0]);

        s_a2dp_vol = param->set_abs_vol.volume;

        bk_bt_dac_set_gain(s_a2dp_vol >> 1);
        if (s_a2dp_vol)
        {
            //sys_hal_aud_dacmute_en(0); //TODO
        }
        else
        {
            //sys_hal_aud_dacmute_en(1); //TODO
        }
        bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
    }
    break;

    case BK_AVRCP_TG_REGISTER_NOTIFICATION_EVT:
    {
        LOGI("%s recv reg evt 0x%x param %d %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, param->reg_ntf.event_id, param->reg_ntf.event_parameter,
                        param->reg_ntf.remote_bda[5],
                        param->reg_ntf.remote_bda[4],
                        param->reg_ntf.remote_bda[3],
                        param->reg_ntf.remote_bda[2],
                        param->reg_ntf.remote_bda[1],
                        param->reg_ntf.remote_bda[0]);

        s_tg_current_registered_noti |= (1 << param->reg_ntf.event_id);

        bk_avrcp_rn_param_t cmd;

        os_memset(&cmd, 0, sizeof(cmd));

        switch (param->reg_ntf.event_id)
        {
        case BK_AVRCP_RN_VOLUME_CHANGE:
            cmd.volume = s_a2dp_vol;
            break;

        default:
            LOGW("%s unknow reg event 0x%x\n", __func__, param->reg_ntf.event_id);
            goto error;
        }

        ret = bk_bt_avrcp_tg_send_rn_rsp(avrcp_remote_bda, param->reg_ntf.event_id, BK_AVRCP_RN_RSP_INTERIM, &cmd);

        if (ret)
        {
            LOGE("%s send rn rsp err %d\n", __func__, ret);
        }
    }
    break;

    default:
        LOGW("%s unknow event 0x%x\n", __func__, event);
        break;
    }

error:;
}

void bk_bt_app_avrcp_ct_play(void)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_PLAY, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if(s_bt_avrcp_event_cb_sema)
    {
        rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_PLAY, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_pause(void)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_PAUSE, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if(s_bt_avrcp_event_cb_sema)
    {
        rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_PAUSE, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_next(void)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_FORWARD, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if(s_bt_avrcp_event_cb_sema)
    {
        rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_FORWARD, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_prev(void)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_BACKWARD, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if(s_bt_avrcp_event_cb_sema)
    {
        rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_BACKWARD, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_rewind(uint32_t ms)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_REWIND, BK_AVRCP_PT_CMD_STATE_PRESSED);

    if(ms)
    {
        rtos_delay_milliseconds(ms);
    }

    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_REWIND, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_fast_forward(uint32_t ms)
{
    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_FAST_FORWARD, BK_AVRCP_PT_CMD_STATE_PRESSED);

    if(ms)
    {
        rtos_delay_milliseconds(ms);
    }

    bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_FAST_FORWARD, BK_AVRCP_PT_CMD_STATE_RELEASED);
}

void bk_bt_app_avrcp_ct_vol_up(void)
{
    uint8_t idx = (s_a2dp_vol + 5) >> 3;
    uint8_t old = s_a2dp_vol;
    if (idx < 16)
    {
        idx += 1;
        s_a2dp_vol = (idx <= 0) ? 0 : (idx >= 16) ? 0x7f :((idx-1) * 8 + 9);
#ifdef CONFIG_A2DP_AUDIO
        bk_bt_dac_set_gain(s_a2dp_vol >> 1);
        if (s_a2dp_vol)
        {
            //sys_hal_aud_dacmute_en(0);//TODO
        }
        else
        {
            //sys_hal_aud_dacmute_en(1);//TODO
        }

        if (s_tg_current_registered_noti & (1 << BK_AVRCP_RN_VOLUME_CHANGE))
        {
            bk_avrcp_rn_param_t cmd;
            int ret = 0;

            os_memset(&cmd, 0, sizeof(cmd));

            cmd.volume = s_a2dp_vol;

            ret = bk_bt_avrcp_tg_send_rn_rsp(avrcp_remote_bda, BK_AVRCP_RN_VOLUME_CHANGE, BK_AVRCP_RN_RSP_CHANGED, &cmd);

            if (ret)
            {
                LOGE("%s send rn rsp err %d\n", __func__, ret);
            }

            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
        else if(0)
        {
            LOGE("%s peer not reg vol change, adjust local only !!!\n", __func__);
            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
        else
        {
            bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_VOL_UP, BK_AVRCP_PT_CMD_STATE_PRESSED);
            if(s_bt_avrcp_event_cb_sema)
            {
                rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
            }
            bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_VOL_UP, BK_AVRCP_PT_CMD_STATE_RELEASED);

            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
#endif
        LOGI("vol_up, vol: %d -> %d\r\n", old, s_a2dp_vol);
    }
}

void bk_bt_app_avrcp_ct_vol_down(void)
{
    uint8_t idx = (s_a2dp_vol + 5) >> 3;
    uint8_t old = s_a2dp_vol;

    if (idx > 0)
    {
        idx -= 1;
        s_a2dp_vol = (idx <= 0) ? 0 : (idx >= 16) ? 0x7f :((idx-1) * 8 + 9);
#ifdef CONFIG_A2DP_AUDIO
        bk_bt_dac_set_gain(s_a2dp_vol >> 1);
        if (s_a2dp_vol)
        {
            //sys_hal_aud_dacmute_en(0);//TODO
        }
        else
        {
            //sys_hal_aud_dacmute_en(1);//TODO
        }

        if (s_tg_current_registered_noti & (1 << BK_AVRCP_RN_VOLUME_CHANGE))
        {
            bk_avrcp_rn_param_t cmd;
            int ret = 0;

            os_memset(&cmd, 0, sizeof(cmd));

            cmd.volume = s_a2dp_vol;

            ret = bk_bt_avrcp_tg_send_rn_rsp(avrcp_remote_bda, BK_AVRCP_RN_VOLUME_CHANGE, BK_AVRCP_RN_RSP_CHANGED, &cmd);

            if (ret)
            {
                LOGE("%s send rn rsp err %d\n", __func__, ret);
            }

            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
        else if(0)
        {
            LOGE("%s peer not reg vol change, adjust local only !!!\n", __func__);
            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
        else
        {
            bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_VOL_DOWN, BK_AVRCP_PT_CMD_STATE_PRESSED);
            if(s_bt_avrcp_event_cb_sema)
            {
                rtos_get_semaphore(&s_bt_avrcp_event_cb_sema, AVRCP_PASSTHROUTH_CMD_TIMEOUT);
            }
            bk_bt_avrcp_ct_send_passthrough_cmd(avrcp_remote_bda, BK_AVRCP_PT_CMD_VOL_DOWN, BK_AVRCP_PT_CMD_STATE_RELEASED);

            bluetooth_storage_save_volume(bt_manager_get_connected_device(), s_a2dp_vol);
        }
#endif
        LOGI("vol_down, vol: %d -> %d\r\n", old, s_a2dp_vol);
    }
}

void bt_wifi_state_updated(void)
{
    bt_audio_sink_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_sink_demo_msg_t));

    if (bt_audio_sink_demo_msg_que == NULL)
    {
        return;
    }

    demo_msg.type = BT_AUDIO_WIFI_STATE_UPDATE_MSG;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_sink_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);

    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);
    }
}

#if CONFIG_WIFI_COEX_SCHEME
void wifi_state_callback(uint8_t status_id, uint8_t status_info)
{
    if (COEX_WIFI_STAT_ID_SCANNING == status_id || COEX_WIFI_STAT_ID_CONNECTING == status_id)
    {
        if (status_info)
        {
            s_bt_env.wifi_state |= (1 << status_id);
        }
        else
        {
            s_bt_env.wifi_state &= ~(1 << status_id);
        }
        bt_wifi_state_updated();
    }
}

static void coex_regisiter_wifi_state(void)
{
    os_memset(&s_coex_to_bt_func, 0, sizeof(s_coex_to_bt_func));
    s_coex_to_bt_func.version = 0x01;
    s_coex_to_bt_func.inform_wifi_status = wifi_state_callback;

    coex_bt_if_init(&s_coex_to_bt_func);
}
#endif

int32_t bk_bt_app_avrcp_ct_get_attr(uint32_t attr)
{
    uint32_t media_attr_id_mask = (1 << BK_AVRCP_MEDIA_ATTR_ID_TITLE);

    if(attr)
    {
        media_attr_id_mask = (1 << attr);
    }

    return bk_bt_avrcp_ct_send_get_elem_attribute_cmd(avrcp_remote_bda, media_attr_id_mask);
}

static void bk_bt_a2dp_connect(uint8_t *remote_addr)
{
    if (s_bt_env.wifi_state)
    {
        bt_manager_set_connect_state(BT_STATE_WAIT_FOR_RECONNECT);
        return;
    }
    bk_bt_a2dp_sink_connect(remote_addr);
}

static void bk_bt_a2dp_stop_connect()
{
    if (rtos_is_oneshot_timer_init(&s_bt_env.avrcp_connect_tmr))
    {
        if (rtos_is_oneshot_timer_running(&s_bt_env.avrcp_connect_tmr))
        {
            rtos_stop_oneshot_timer(&s_bt_env.avrcp_connect_tmr);
        }
        rtos_deinit_oneshot_timer(&s_bt_env.avrcp_connect_tmr);
    }
}

static void bk_bt_a2dp_disconnect(uint8_t *remote_addr)
{
    bk_bt_a2dp_sink_disconnect(bt_manager_get_connected_device());
}

int a2dp_sink_demo_init(uint8_t aac_supported)
{
    int ret = 0;
    LOGI("%s\r\n", __func__);

    if (aac_supported)
    {
#if (!CONFIG_AAC_DECODER)
        LOGI("%s AAC is not supported!\n", __func__);
        return -1;
#endif
    }

    bk_avrcp_rn_evt_cap_mask_t tmp_cap = {0};
    bk_avrcp_rn_evt_cap_mask_t final_cap =
    {
        .bits = 0
        | (1 << BK_AVRCP_RN_VOLUME_CHANGE)
        ,
    };

    if (!s_bt_api_event_cb_sema)
    {
        ret = rtos_init_semaphore(&s_bt_api_event_cb_sema, 1);

        if (ret)
        {
            LOGE("%s sem init err %d\n", __func__, ret);
            return -1;
        }
    }
    if(!s_bt_avrcp_event_cb_sema)
    {
        ret = rtos_init_semaphore(&s_bt_avrcp_event_cb_sema, 1);

        if (ret)
        {
            LOGE("%s avrcp evt sem init err %d\n", __func__, ret);
            return -1;
        }
    }

#if CONFIG_WIFI_COEX_SCHEME
    coex_regisiter_wifi_state();
#endif

    btm_callback_s btm_cb =
    {
        .gap_cb = gap_event_cb,
        .start_connect_cb = bk_bt_a2dp_connect,
        .start_disconnect_cb = bk_bt_a2dp_disconnect,
        .stop_connect_cb = bk_bt_a2dp_stop_connect,
    };
    bt_manager_register_callback(&btm_cb);

    bt_audio_sink_demo_task_init();

    bk_bt_avrcp_ct_init();
    bk_bt_avrcp_ct_register_callback(bk_bt_app_avrcp_ct_cb);

    bk_bt_avrcp_tg_init();

    bk_bt_avrcp_tg_get_rn_evt_cap(BK_AVRCP_RN_CAP_API_METHOD_ALLOWED, &tmp_cap);

    final_cap.bits &= tmp_cap.bits;

    LOGI("%s set rn cap 0x%x\n", __func__, final_cap.bits);

    ret = bk_bt_avrcp_tg_set_rn_evt_cap(&final_cap);

    if (ret)
    {
        LOGE("%s set rn cap err %d\n", __func__, ret);
        return -1;
    }

    bk_bt_avrcp_tg_register_callback(avrcp_tg_cb);

    ret = bk_bt_a2dp_register_callback(bk_bt_app_a2dp_sink_cb);
    if (ret)
    {
        LOGE("%s bk_bt_a2dp_register_callback err %d\n", __func__, ret);
        return -1;
    }

    ret = bk_bt_a2dp_sink_init(aac_supported);

    if (ret)
    {
        LOGI("%s a2dp sink init err %d\n", __func__, ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);

    if (ret)
    {
        LOGI("%s get sem for a2dp sink init err\n", __func__);
        return -1;
    }

    ret = bk_bt_a2dp_sink_register_data_callback(&bt_audio_sink_media_data_ind);

    if (ret)
    {
        LOGE("%s bk_bt_a2dp_sink_register_data_callback err %d\n",__func__,  ret);
        return -1;
    }

    return 0;
}


static int speaker_task_init()
{
    bk_err_t ret = BK_OK;
    if (!a2dp_speaker_thread_handle)
    {
        ret = rtos_create_thread(&a2dp_speaker_thread_handle,
                                 A2DP_SPEAKER_THREAD_PRI,
                                 "bt_a2dp_speaker",
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
        LOGE("%s task is exist !!!\n", __func__);
        return kInProgressErr;
    }
}

static void speaker_task(void *arg)
{
    bk_err_t ret = 0;

    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();

    cfg.nChans = CONFIG_BOARD_AUDIO_CHANNLE_NUM;
    cfg.sampRate = (bt_audio_a2dp_sink_codec.type == CODEC_AUDIO_SBC ? bt_audio_a2dp_sink_codec.cie.sbc_codec.sample_rate : bt_audio_a2dp_sink_codec.cie.aac_codec.sample_rate);
    cfg.volume = (s_a2dp_vol >> 1);
    cfg.frame_size = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size = cfg.frame_size * 2;

    LOGI("%s wait hfp task end\n", __func__);
    extern int32_t wait_hfp_speaker_mic_task_end(void);
    wait_hfp_speaker_mic_task_end();

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

    if (a2dp_speaker_sema == NULL)
    {
        if (kNoErr != rtos_init_semaphore(&a2dp_speaker_sema, 1))
        {
            LOGE("init sema fail, %d \n", __LINE__);
            goto end;
        }
    }

    LOGI("%s init a2dp success!! \r\n", __func__);

    while (1)
    {
        if(!s_spk_is_started)
        {
            break;
        }

        rtos_get_semaphore(&a2dp_speaker_sema, BEKEN_WAIT_FOREVER);

        uint32_t frame_nodes = ring_buffer_node_get_fill_nodes(&s_a2dp_frame_nodes);
        while(frame_nodes)
        {
//            LOGI("speaker get node :%d \n", frame_nodes);
            int i = 0;
            uint8_t s_frame = (CODEC_AUDIO_SBC == bt_audio_a2dp_sink_codec.type ? (frame_nodes>A2DP_SPEAKER_WRITE_SBC_FRAME_NUM ? A2DP_SPEAKER_WRITE_SBC_FRAME_NUM:frame_nodes) : frame_nodes);
            s_frame = frame_nodes;
            for (; i < s_frame; i++)
            {
                uint8_t *inbuf = ring_buffer_node_peek_read_node(&s_a2dp_frame_nodes);
                uint16_t tmp_len = (inbuf[1] << 8 | inbuf[0]);

                inbuf += 2;

                bk_err_t ret;

                if (CODEC_AUDIO_SBC == bt_audio_a2dp_sink_codec.type)
                {
                    ret = bk_sbc_decoder_frame_decode(&bt_audio_sink_sbc_decoder, inbuf, tmp_len);
                    ring_buffer_node_take_read_node(&s_a2dp_frame_nodes);
                    if (ret < 0)
                    {
                        LOGE("sbc_decoder_decode error <%d> %d %d\n", ret, tmp_len, s_frame);
                        continue;
                    }
                    int16_t *dst = (int16_t*)bt_audio_sink_sbc_decoder.pcm_sample;
                    int16_t w_len = bt_audio_sink_sbc_decoder.pcm_length * bt_audio_sink_sbc_decoder.channel_number * 2;
                    if(CONFIG_BOARD_AUDIO_CHANNLE_NUM == 1)
                    {
                        if(bt_audio_a2dp_sink_codec.cie.sbc_codec.channels == 2)
                        {
                            for(int i=0; i<bt_audio_sink_sbc_decoder.pcm_length * 2; i++)
                            {
                                dst[i] = dst[i*2];
                            }
                        }

                        w_len = bt_audio_sink_sbc_decoder.pcm_length * 2;
                    }

                    int size = audio_play_write_data(s_audio_play_obj, (char *)dst, w_len);
                    if (size <= 0)
                    {
                        LOGE("raw_stream_write size fail: %d \n", size);
                        break;
                    }
                    else
                    {
                        //LOGI("raw_stream_write size: %d \n", size);
                    }
                }
#if (CONFIG_AAC_DECODER)
                else if(CODEC_AUDIO_AAC == bt_audio_a2dp_sink_codec.type)
                {
                    uint32_t len = 0;
                    memcpy(&len, inbuf, sizeof(len));

                    // LOGI("<- %d \n", len);
                    uint8_t *out_buf = 0;
                    uint32_t out_len = 0;
                    ret = bk_aac_decoder_decode(&bt_audio_sink_aac_decoder, (inbuf + sizeof(len)), len, &out_buf, &out_len);
                    if(ret == 0)
                    {
                        int16_t *dst = (int16_t*)out_buf;
                        int16_t w_len = out_len;
                        if(CONFIG_BOARD_AUDIO_CHANNLE_NUM == 1)
                        {
                            for(uint16_t i=0;i<out_len/2;i++)
                            {
                                dst[i] = dst[i*2];
                            }
                            w_len = out_len/2;
                        }

                        int size = audio_play_write_data(s_audio_play_obj, (char *)dst, w_len);
                        if (size <= 0)
                        {
                            LOGE("raw_stream_write size fail: %d \n", size);
                            break;
                        }
                        else
                        {
                            // LOGI("raw_stream_write size: %d \n", size);
                        }
                    }else
                    {
                        LOGE("aac_decoder_decode error <%d>\n", ret);
                    }
                }
#endif
                else
                {
                    LOGE("unsupported codec!! /n");
                }
            }
            frame_nodes -= s_frame;
        }
    }

end:;
    LOGI("%s a2dp exit start!! \r\n", __func__);

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

    LOGI("%s a2dp end!!\r\n", __func__);
    rtos_deinit_semaphore(&a2dp_speaker_sema);
    a2dp_speaker_sema = NULL;
    if (CODEC_AUDIO_SBC == bt_audio_a2dp_sink_codec.type)
    {
        bk_sbc_decoder_deinit();
    }
#if (CONFIG_AAC_DECODER)
    bk_aac_decoder_deinit(&bt_audio_sink_aac_decoder);
#endif
    ring_buffer_node_clear(&s_a2dp_frame_nodes);
    ring_buffer_node_deinit(&s_a2dp_frame_nodes);
    if (p_cache_buff)
    {
        os_free(p_cache_buff);
        p_cache_buff = NULL;
    }
    if(s_spk_is_started)
    {
        s_spk_is_started = 0;
        LOGE("!! speaker tash exit error !!\n");
    }

    if (a2dp_speaker_exit_sema)
    {
        rtos_set_semaphore(&a2dp_speaker_exit_sema);
    }

    rtos_delete_thread(NULL);
}

int32_t wait_a2dp_speaker_task_end(void)
{
    while(a2dp_speaker_thread_handle)
    {
        rtos_delay_milliseconds(20);
    }

    return 0;
}
