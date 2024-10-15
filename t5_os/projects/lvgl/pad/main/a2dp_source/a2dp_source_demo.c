#include <stdio.h>
#include <stdlib.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <modules/pm.h>
#include <components/log.h>
#include "diskio.h"

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_a2dp_types.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#include "modules/mp3dec.h"
#include "modules/sbc_encoder.h"
#include "modules/audio_rsp_types.h"
#include "modules/audio_rsp.h"
#include "common/bk_assert.h"

//#include "audio_osi_wrapper.h"
#include "ff.h"
#include "ring_buffer_particle.h"
#include "modules/pm.h"
#include "driver/pwr_clk.h"
#include "a2dp_source_demo.h"


#if CONFIG_SOC_BK7256XX
    #define PCM_USE_AMP_CALL 0
#elif CONFIG_CPU_CNT > 1 && CONFIG_MAILBOX
    #define PCM_USE_AMP_CALL 1
#else
    #define PCM_USE_AMP_CALL 0
#endif

#if PCM_USE_AMP_CALL
    #include "driver/media_types.h"
    #include "media_evt.h"
#endif

#if CONFIG_SOC_BK7256XX
#define A2DP_CPU_FRQ PM_CPU_FRQ_240M
#else
#define A2DP_CPU_FRQ PM_CPU_FRQ_320M
#endif

#define OLD_GAP_API_IMPL 1

typedef struct
{
    uint8_t addr[6];
} device_addr_t;

typedef struct
{
    uint8_t inited;
    uint16_t conn_handle;
    uint8_t discovery_status;
    //bd_addr_t peer_addr;
    device_addr_t peer_addr;
    uint8_t conn_state;
    uint8_t start_status;
    uint32_t mtu;
    uint8_t read_cb_pause;
    uint8_t local_action_pending;
} a2dp_env_s;

enum
{
    BT_A2DP_SOURCE_MSG_READ_PCM_FROM_BUFF = 1,
};

enum
{
    A2DP_SOURCE_DEBUG_LEVEL_ERROR,
    A2DP_SOURCE_DEBUG_LEVEL_WARNING,
    A2DP_SOURCE_DEBUG_LEVEL_INFO,
    A2DP_SOURCE_DEBUG_LEVEL_DEBUG,
    A2DP_SOURCE_DEBUG_LEVEL_VERBOSE,
};



#define USER_A2DP_MAIN_TASK 0
#define TASK_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#define DISCONNECT_REASON_REMOTE_USER_TERMINATE 0x13
#define MP3_DECODE_BUFF_SIZE (MAX_NSAMP * MAX_NCHAN * MAX_NGRAN * sizeof(uint16_t))
#define DECODE_TRIGGER_TIME (50) //ms
#define DECODE_RB_SIZE ((s_decode_trigger_size > MP3_DECODE_BUFF_SIZE ? s_decode_trigger_size : MP3_DECODE_BUFF_SIZE) + MP3_DECODE_BUFF_SIZE + 1)
#define A2DP_SOURCE_WRITE_AUTO_TIMER_MS 30
#define SBC_SAMPLE_DEPTH 16

#if DECODE_TRIGGER_TIME <= A2DP_SOURCE_WRITE_AUTO_TIMER_MS
    #error "DECODE_TRIGGER_TIME must > A2DP_SOURCE_WRITE_AUTO_TIMER_MS !!!"
#endif

#define CONNECTION_PACKET_TYPE 0xcc18
#define CONNECTION_PAGE_SCAN_REPETITIOIN_MODE 0x01
#define CONNECTION_CLOCK_OFFSET 0x00

#define A2DP_SOURCE_DEBUG_LEVEL A2DP_SOURCE_DEBUG_LEVEL_INFO

#define a2dp_loge(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_ERROR)   BK_LOGE("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define a2dp_logw(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_WARNING) BK_LOGW("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define a2dp_logi(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_INFO)    BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define a2dp_logd(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_DEBUG)   BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define a2dp_logv(format, ...) do{if(A2DP_SOURCE_DEBUG_LEVEL >= A2DP_SOURCE_DEBUG_LEVEL_VERBOSE) BK_LOGI("a2dp_s", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)

extern bk_err_t bk_audio_osi_funcs_init(void);
static bk_err_t a2dp_source_demo_create_mp3_decode_task(void);
static bk_err_t a2dp_source_demo_stop_mp3_decode_task(void);
static int avdtp_source_suspend(void);


static bk_bt_linkkey_storage_t s_bt_linkkey;
static a2dp_env_s a2dp_env;
static bk_a2dp_mcc_t s_a2dp_cap_info;

static beken_semaphore_t s_bt_api_event_cb_sema = NULL;


#if USER_A2DP_MAIN_TASK
    static beken_thread_t bt_a2dp_source_main_thread_handle = NULL;
#endif


static beken_semaphore_t s_source_need_decode_sema = NULL;
static beken_thread_t bt_a2dp_source_decode_thread_handle = NULL;
static ring_buffer_particle_ctx s_rb_ctx;
static FIL mp3file;
static uint8_t s_file_path[64] = {0};
static MP3FrameInfo s_mp3_frame_info = {0};
static uint32_t s_decode_trigger_size; //44.1khz buffer size = 176400 * DECODE_TRIGGER_TIME Bytes

static uint8_t s_decode_task_run;
static uint8_t s_is_bk_aud_rsp_inited;

static SbcEncoderContext s_sbc_software_encoder_ctx;
//uint8_t g_is_a2dp_source_extenal_encode_impl = 0;

#if PCM_USE_AMP_CALL
    static volatile uint8_t s_is_cpu1_task_ready;
#endif

#if PCM_USE_AMP_CALL
    extern bk_err_t media_send_msg_sync(uint32_t event, uint32_t param);
#endif

static void *mp3_private_alloc(size_t size)
{
    return os_malloc(size);
}

static void mp3_private_free(void *buff)
{
    os_free(buff);
}

static void *mp3_private_memset(void *s, unsigned char c, size_t n)
{
    return os_memset(s, c, n);
}

static void *mp3_private_alloc_psram(size_t size)
{
    return psram_malloc(size);
}

static void mp3_private_free_psram(void *buff)
{
    psram_free(buff);
}

static void *mp3_private_memset_psram(void *s, unsigned char c, size_t n)
{
    os_memset_word((uint32_t *)s, c, n);
    return s;
}

static uint32_t bt_api_event_cb(bt_event_enum_t event, void *param)
{
    switch (event)
    {
    case BK_DM_BT_EVENT_DISCONNECT:
    {
        uint8_t *addr = (uint8_t *)param;
        a2dp_logi("Disconnected from %02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        if (os_memcmp(addr, a2dp_env.peer_addr.addr, sizeof(a2dp_env.peer_addr.addr)))
        {
            a2dp_logw("disconnect is not match our connect");
            break;
        }

        os_memset(&a2dp_env, 0, sizeof(a2dp_env));
        a2dp_env.inited = 1;

        //        if (s_bt_api_event_cb_sema)
        //        {
        //            rtos_set_semaphore(&s_bt_api_event_cb_sema);
        //        }

        bt_a2dp_source_demo_stop_all();
    }

    break;

    case BK_DM_BT_EVENT_CONNECTION_COMPLETE:
    {

        uint8_t *addr = (uint8_t *)param;
        a2dp_logi("Connected to %02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        if (os_memcmp(addr, a2dp_env.peer_addr.addr, sizeof(a2dp_env.peer_addr.addr)))
        {
            a2dp_logw("connect is not match our connect");
            break;
        }

        //        if (s_bt_api_event_cb_sema)
        //        {
        //            rtos_set_semaphore(&s_bt_api_event_cb_sema);
        //        }
    }
    break;

    case BK_DM_BT_EVENT_LINKKEY_NOTIFY:
    {
        bk_bt_linkkey_storage_t *linkkey = (typeof(linkkey))param;

        a2dp_logi("recv linkkey %02X:%02X:%02X:%02X:%02X:%02X",
                  linkkey->addr[5],
                  linkkey->addr[4],
                  linkkey->addr[3],
                  linkkey->addr[2],
                  linkkey->addr[1],
                  linkkey->addr[0]);

        os_memcpy(&s_bt_linkkey, linkkey, sizeof(s_bt_linkkey));
    }
    break;

    case BK_DM_BT_EVENT_LINKKEY_REQ:
    {
        uint8_t *addr = (typeof(addr))param;

        if (!os_memcmp(addr, s_bt_linkkey.addr, sizeof(s_bt_linkkey.addr)))
        {
            a2dp_logi("found linkkey %02X:%02X:%02X:%02X:%02X:%02X",
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            bk_bt_gap_linkkey_reply(1, &s_bt_linkkey);
        }
        else
        {
            bk_bt_linkkey_storage_t tmp;

            a2dp_logi("notfound linkkey %02X:%02X:%02X:%02X:%02X:%02X",
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            os_memset(&tmp, 0, sizeof(tmp));
            os_memcpy(tmp.addr, addr, sizeof(tmp.addr));

            bk_bt_gap_linkkey_reply(0, &tmp);
        }

    }
    break;

#if 0//OLD_GAP_API_IMPL

    case BK_DM_BT_GAP_DISC_RES_EVT:
    {
        bk_bt_gap_cb_param_t *cb = (typeof(cb))param;

#if A2DP_SOURCE_DEBUG_LEVEL <= A2DP_SOURCE_DEBUG_LEVEL_INFO

        char name[128] = {0};
        int8_t rssi = 0;
        uint32_t cod = 0;
        uint16_t eir_data_len = 0;

        for (int i = 0; i < cb->disc_res.num_prop; ++i)
        {
            if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_BDNAME)
            {
                os_memcpy(name, cb->disc_res.prop[i].val, cb->disc_res.prop[i].len < sizeof(name) - 1 ? cb->disc_res.prop[i].len : sizeof(name) - 1);
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_COD)
            {
                os_memcpy(&cod, cb->disc_res.prop[i].val, sizeof(cod));
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_RSSI)
            {
                os_memcpy(&rssi, cb->disc_res.prop[i].val, sizeof(rssi));
            }
            else if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_EIR)
            {
                eir_data_len = cb->disc_res.prop[i].len;
            }
        }

        if (name[0])
        {
            a2dp_logi("discovery name %s mac 0x%02X:%02X:%02X:%02X:%02X:%02X rssi %d cod 0x%X eir len %d",
                      name,
                      cb->disc_res.bda[5],
                      cb->disc_res.bda[4],
                      cb->disc_res.bda[3],
                      cb->disc_res.bda[2],
                      cb->disc_res.bda[1],
                      cb->disc_res.bda[0],
                      rssi, eir_data_len);
        }
        else
        {
            a2dp_logi("discovery mac 0x%02X:%02X:%02X:%02X:%02X:%02X rssi %d cod 0x%X eir len %d",
                      cb->disc_res.bda[5],
                      cb->disc_res.bda[4],
                      cb->disc_res.bda[3],
                      cb->disc_res.bda[2],
                      cb->disc_res.bda[1],
                      cb->disc_res.bda[0],
                      rssi, eir_data_len);
        }

#elif A2DP_SOURCE_DEBUG_LEVEL <= A2DP_SOURCE_DEBUG_LEVEL_DEBUG

        a2dp_logd("BK_DM_BT_GAP_DISC_RES_EVT mac 0x%02X:%02X:%02X:%02X:%02X:%02X num prop %d",
                  cb->disc_res.bda[5],
                  cb->disc_res.bda[4],
                  cb->disc_res.bda[3],
                  cb->disc_res.bda[2],
                  cb->disc_res.bda[1],
                  cb->disc_res.bda[0],
                  cb->disc_res.num_prop);

        for (int i = 0; i < cb->disc_res.num_prop; ++i)
        {
            if (cb->disc_res.prop[i].type == BK_BT_GAP_DEV_PROP_BDNAME)
            {
                char name[128] = {0};
                os_memcpy(name, cb->disc_res.prop[i].val, cb->disc_res.prop[i].len);
                a2dp_logd("name %s", name);
            }
            else
            {
                a2dp_logd("type %d len %d", cb->disc_res.prop[i].type, cb->disc_res.prop[i].len);
            }
        }

#endif
    }
    break;

    case BK_DM_BT_GAP_DISC_STATE_CHANGED_EVT:
    {
        bk_bt_gap_cb_param_t *cb = (typeof(cb))param;
        a2dp_logi("discovery change %d", cb->disc_st_chg.state);
        a2dp_env.discovery_status = cb->disc_st_chg.state;
    }

    break;
#endif

    default:
        break;
    }

    return 0;
}


static void bk_bt_a2dp_source_event_cb(bk_a2dp_cb_event_t event, bk_a2dp_cb_param_t *p_param)
{
    a2dp_logd("event: %d", __func__, event);

    switch (event)
    {
    case BK_A2DP_CONNECTION_STATE_EVT:
    {
        uint8_t status = p_param->conn_state.state;
        uint8_t *bda = p_param->conn_state.remote_bda;
        a2dp_logi("A2DP connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                  p_param->conn_state.state, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);

        if (a2dp_env.conn_state != status)
        {
            a2dp_env.conn_state = status;

            if (status == BK_A2DP_CONNECTION_STATE_CONNECTED || status == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
            {
                if (s_bt_api_event_cb_sema != NULL)
                {
                    rtos_set_semaphore( &s_bt_api_event_cb_sema );
                }
            }
        }

        if (status == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            bt_a2dp_source_demo_stop_all();
            a2dp_env.start_status = 0;
            a2dp_env.mtu = 0;
        }
    }
    break;

    case BK_A2DP_AUDIO_STATE_EVT:
    {
        a2dp_logi("A2DP audio state: %d", p_param->audio_state.state);

        if (BK_A2DP_AUDIO_STATE_STARTED == p_param->audio_state.state)
        {
            if (a2dp_env.start_status == 0 )
            {
                a2dp_env.start_status = 1;

                a2dp_source_demo_create_mp3_decode_task();

                if (s_bt_api_event_cb_sema != NULL && a2dp_env.local_action_pending)
                {
                    rtos_set_semaphore( &s_bt_api_event_cb_sema );
                }
            }
        }
        else if ((BK_A2DP_AUDIO_STATE_SUSPEND == p_param->audio_state.state))
        {
            if (a2dp_env.start_status == 1 )
            {
                a2dp_env.start_status = 0;

                a2dp_source_demo_stop_mp3_decode_task();

                if (s_bt_api_event_cb_sema != NULL && a2dp_env.local_action_pending)
                {
                    rtos_set_semaphore( &s_bt_api_event_cb_sema );
                }
            }
        }
    }
    break;

    case BK_A2DP_AUDIO_SOURCE_CFG_EVT:
    {
        os_memcpy(&s_a2dp_cap_info, &p_param->audio_source_cfg.mcc, sizeof(s_a2dp_cap_info));

        //        s_a2dp_cap_info.cie.sbc_codec.channels = p_param->audio_source_cfg.mcc.cie.sbc_codec.channels;
        //        s_a2dp_cap_info.cie.sbc_codec.channel_mode = p_param->audio_source_cfg.mcc.cie.sbc_codec.channel_mode;
        //        s_a2dp_cap_info.cie.sbc_codec.block_len = p_param->audio_source_cfg.mcc.cie.sbc_codec.block_len;
        //        s_a2dp_cap_info.cie.sbc_codec.subbands = p_param->audio_source_cfg.mcc.cie.sbc_codec.subbands;
        //        s_a2dp_cap_info.cie.sbc_codec.sample_rate = p_param->audio_source_cfg.mcc.cie.sbc_codec.sample_rate;
        //        s_a2dp_cap_info.cie.sbc_codec.bit_pool = p_param->audio_source_cfg.mcc.cie.sbc_codec.bit_pool;
        //        s_a2dp_cap_info.cie.sbc_codec.alloc_mode = p_param->audio_source_cfg.mcc.cie.sbc_codec.alloc_mode;

        a2dp_env.mtu = p_param->audio_source_cfg.mtu;

        a2dp_logi("channel %d sample rate %d mtu %d", s_a2dp_cap_info.cie.sbc_codec.channels,
                  s_a2dp_cap_info.cie.sbc_codec.sample_rate,
                  a2dp_env.mtu);

        if (s_bt_api_event_cb_sema != NULL)
        {
            rtos_set_semaphore( &s_bt_api_event_cb_sema );
        }
    }
    break;

    default:
        a2dp_loge("Invalid A2DP event: %d", event);
        break;
    }
}

int bt_a2dp_source_demo_discover(uint32_t sec, uint32_t num_report)
{
#if 0//OLD_GAP_API_IMPL
    uint8_t inq_len = sec * 100 / 128;
    uint8_t num_rpt = (num_report > 255 ? 255 : num_report);

    if (a2dp_env.discovery_status != BK_BT_GAP_DISCOVERY_STOPPED)
    {
        a2dp_loge("currently is discovering");
        return -1;
    }

    if (inq_len > 0x30)
    {
        inq_len = 0x30;
    }

    if (inq_len == 0)
    {
        inq_len = 1;
    }

    bk_bt_gap_set_event_callback(bt_api_event_cb);

    bk_bt_gap_start_discovery(BK_BT_INQ_MODE_GENERAL_INQUIRY, inq_len, num_rpt);
#endif
    return 0;
}

int bt_a2dp_source_demo_discover_cancel(void)
{
#if 0//OLD_GAP_API_IMPL

    if (a2dp_env.discovery_status != BK_BT_GAP_DISCOVERY_STARTED)
    {
        a2dp_loge("currently is not discovering");
        return -1;
    }

    bk_bt_gap_set_event_callback(bt_api_event_cb);

    bk_bt_gap_cancel_discovery();
#endif
    return 0;
}

int bt_a2dp_source_demo_connect(uint8_t *addr)
{
    int err = kNoErr;

    if (!a2dp_env.inited)
    {
        bk_bt_a2dp_source_init();
        bk_bt_a2dp_register_callback(bk_bt_a2dp_source_event_cb);
        a2dp_env.inited = 1;
    }

    if (!s_bt_api_event_cb_sema)
    {
        err = rtos_init_semaphore(&s_bt_api_event_cb_sema, 1);

        if (err)
        {
            a2dp_loge("sem init err %d", err);
            goto error;
        }
    }

    bk_bt_gap_set_event_callback(bt_api_event_cb);

#if 0//OLD_GAP_API_IMPL

    if (a2dp_env.discovery_status != BK_BT_GAP_DISCOVERY_STOPPED)
    {
        a2dp_loge("currently is discovering");
        return 0;
    }

#endif

    if (a2dp_env.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        a2dp_loge("already connected");
        return 0;
    }

    if (a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_DISCONNECTED)
    {
        a2dp_loge("remote device is not idle, please disconnect first");
        err = -1;
        goto error;
    }

    os_memcpy(a2dp_env.peer_addr.addr, addr, sizeof(a2dp_env.peer_addr.addr));

    a2dp_logi("start connect a2dp");

    err = bk_bt_a2dp_source_connect(a2dp_env.peer_addr.addr);

    if (err)
    {
        a2dp_loge("connect a2dp err %d", err);
        goto error;
    }

    //    a2dp_logi("start wait bt link connect cb");
    //
    //    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 12 * 1000);
    //
    //    if (err != kNoErr)
    //    {
    //        a2dp_loge("rtos_get_semaphore s_bt_api_event_cb_sema err %d", err);
    //        goto error;
    //    }

    a2dp_logi("start wait a2dp connect cb");

    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 12 * 1000);

    if (err)
    {
        a2dp_loge("get sem for connect err");
        goto error;
    }

    a2dp_logi("start wait a2dp cap report cb");

    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);

    if (err)
    {
        a2dp_loge("get sem for cap select err");
        goto error;
    }

    a2dp_logi("a2dp connect complete");

    if (s_bt_api_event_cb_sema)
    {
        rtos_deinit_semaphore(&s_bt_api_event_cb_sema);
        s_bt_api_event_cb_sema = NULL;
    }

    return 0;
error:

    if (s_bt_api_event_cb_sema)
    {
        rtos_deinit_semaphore(&s_bt_api_event_cb_sema);
        s_bt_api_event_cb_sema = NULL;
    }

    return err;
}


int bt_a2dp_source_demo_disconnect(uint8_t *addr)
{
    int err = kNoErr;

    if (!a2dp_env.inited)
    {
        bk_bt_a2dp_source_init();
        bk_bt_a2dp_register_callback(bk_bt_a2dp_source_event_cb);
        a2dp_env.inited = 1;
    }

    if (!s_bt_api_event_cb_sema)
    {
        err = rtos_init_semaphore(&s_bt_api_event_cb_sema, 1);

        if (err)
        {
            a2dp_loge("sem init err %d", err);
            goto error;
        }
    }

    bk_bt_gap_set_event_callback(bt_api_event_cb);

    if (a2dp_env.conn_state == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
    {
        a2dp_loge("already disconnect");
        return 0;
    }

    if (a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        a2dp_loge("remote device is not connected");
        err = -1;
        goto error;
    }

    if (os_memcmp(addr, a2dp_env.peer_addr.addr, sizeof(a2dp_env.peer_addr.addr)))
    {
        a2dp_loge("addr not match");
        err = -1;
        goto error;
    }

    a2dp_logi("start disconnect a2dp source");

    err = bk_bt_a2dp_source_disconnect(a2dp_env.peer_addr.addr);

    if (err)
    {
        a2dp_loge("disconnect a2dp err %d", err);
        err = -1;
        goto error;
    }

    a2dp_logi("start wait a2dp source disconnect complete");

    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);

    if (err)
    {
        a2dp_loge("disconnect a2dp timeout %d", err);
        err = -1;
        goto error;
    }

    //    a2dp_logi("start wait bt link disconnect event complete");
    //
    //    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);
    //
    //    if (err != kNoErr)
    //    {
    //        a2dp_loge("disconnect bt link timeout %d s_bt_api_event_cb_sema", err);
    //        err = -1;
    //        goto error;
    //    }

    if (s_bt_api_event_cb_sema)
    {
        rtos_deinit_semaphore(&s_bt_api_event_cb_sema);
        s_bt_api_event_cb_sema = NULL;
    }

    a2dp_logi("disconnect complete");
    return 0;

error:

    if (s_bt_api_event_cb_sema)
    {
        rtos_deinit_semaphore(&s_bt_api_event_cb_sema);
        s_bt_api_event_cb_sema = NULL;
    }

    return err;
}

static int avdtp_source_start(void)
{
    int err = kNoErr;

    if (!s_bt_api_event_cb_sema)
    {
        err = rtos_init_semaphore(&s_bt_api_event_cb_sema, 1);

        if (err)
        {
            a2dp_loge("sem init err %d", err);
            goto error;
        }
    }

    if (a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        a2dp_loge("remote device is not connected");
        err = -1;
        goto error;
    }

    if (a2dp_env.start_status)
    {
        a2dp_loge("is already start");
        return 0;
    }

    a2dp_env.local_action_pending = 1;
    err = bk_a2dp_media_ctrl(BK_A2DP_MEDIA_CTRL_START);

    if (err)
    {
        a2dp_loge("start err %d", err);
        goto error;
    }

    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);

    if (err)
    {
        a2dp_loge("get sem err");
        goto error;
    }

    a2dp_env.local_action_pending = 0;
    return 0;
error:
    a2dp_env.local_action_pending = 0;
    return err;
}


static int32_t a2dp_source_data_cb(uint8_t *buf, int32_t len)
{
    uint32_t read_len = 0;

    if (a2dp_env.read_cb_pause)
    {
        return 0;
    }

    if (ring_buffer_particle_len(&s_rb_ctx) < len)
    {
        a2dp_loge("ring buffer not enough data %d < %d ", ring_buffer_particle_len(&s_rb_ctx), len);
    }
    else
    {
        ring_buffer_particle_read(&s_rb_ctx, buf, len, &read_len);
    }

    if (s_source_need_decode_sema)
    {
        rtos_set_semaphore(&s_source_need_decode_sema);
    }

    return read_len;
}

static int32_t a2dp_source_pcm_encode_cb(uint8_t type, uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len)
{
    int32_t encode_len = 0;

    if (type != 0)
    {
        a2dp_loge("type not match %d", type);
        return -1;
    }

#if PCM_USE_AMP_CALL
    bt_audio_encode_req_t rsp_req;
    int ret = 0;

    os_memset(&rsp_req, 0, sizeof(rsp_req));

    rsp_req.handle = &s_sbc_software_encoder_ctx;
    rsp_req.in_addr = in_addr;
    rsp_req.type = type;
    rsp_req.out_len_ptr = (typeof(rsp_req.out_len_ptr))&encode_len;

    ret = media_send_msg_sync(EVENT_BT_PCM_ENCODE_REQ, (uint32_t)&rsp_req);

    if (ret)
    {
        a2dp_loge("EVENT_BT_PCM_ENCODE_REQ err %d !!", ret);
        return -1;
    }

#else
    encode_len = sbc_encoder_encode(&s_sbc_software_encoder_ctx, (const int16_t *)in_addr);
#endif

    if (!encode_len)
    {
        a2dp_loge("encode err %d", encode_len);
        return -1;
    }

    if (encode_len > *out_len)
    {
        a2dp_loge("encode len %d > out_len %d", encode_len, *out_len);
        return -1;
    }

    os_memcpy(out_addr, s_sbc_software_encoder_ctx.stream, encode_len);

    *out_len = encode_len;

    return 0;
}

static int32_t a2dp_source_pcm_resample_cb(uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len)
{
    int32_t ret = 0;
    uint32_t input_len = 0;
    uint32_t output_len = 0;

    if (!s_is_bk_aud_rsp_inited)
    {
        a2dp_loge("resample not init");
        return -1;
    }

#if PCM_USE_AMP_CALL

    bt_audio_resample_req_t rsp_req;

    os_memset(&rsp_req, 0, sizeof(rsp_req));

    input_len = *in_len;
    output_len = *out_len;

    rsp_req.in_addr = in_addr;
    rsp_req.out_addr = out_addr;
    rsp_req.in_bytes_ptr = &input_len;
    rsp_req.out_bytes_ptr = &output_len;

    //a2dp_logi("in %d %d", input_len, output_len);

    ret = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_REQ, (uint32_t)&rsp_req);

    if (ret)
    {
        a2dp_loge("EVENT_BT_PCM_RESAMPLE_REQ err %d !!", ret);
        ret = -1;
    }

    //a2dp_logi("out %d %d", input_len, output_len);
    *in_len = input_len;
    *out_len = output_len;

#else

    input_len = *in_len / (s_mp3_frame_info.bitsPerSample / 8);
    output_len = *out_len / (s_mp3_frame_info.bitsPerSample / 8);

    ret = bk_aud_rsp_process((int16_t *)in_addr, &input_len, (int16_t *)out_addr, &output_len);

    if (ret)
    {
        a2dp_loge("resample err %d", ret);
    }

    *in_len = input_len * (s_mp3_frame_info.bitsPerSample / 8);
    *out_len = output_len * (s_mp3_frame_info.bitsPerSample / 8);

#endif

    return ret;
}

int bt_a2dp_source_demo_music_play(uint8_t is_mp3, uint8_t *file_path)
{
    bk_err_t error = 0;

    if (file_path && file_path[0])
    {
        os_strncpy((char *)s_file_path, (char *)file_path, sizeof(s_file_path) - 1);
    }

    if (s_file_path[0] == 0)
    {
        a2dp_loge("file_path err");
        error = -1;
        goto error;
    }

    if (a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        a2dp_loge("not connected");
        error = 0;
        goto error;
    }

    error = a2dp_source_demo_create_mp3_decode_task();

    if (error)
    {
        a2dp_loge("create task err %d", error);
        goto error;
    }

    error = bk_a2dp_source_set_pcm_data_format((uint32_t)s_mp3_frame_info.samprate, (uint8_t)s_mp3_frame_info.bitsPerSample, (uint8_t)s_mp3_frame_info.nChans);

    if (error)
    {
        a2dp_loge("set format err %d", error);
        goto error;
    }

    error = bk_a2dp_source_register_data_callback(a2dp_source_data_cb);

    if (error)
    {
        a2dp_loge("reg data cb err %d", error);
        goto error;
    }

    error = bk_a2dp_source_register_pcm_encode_callback(a2dp_source_pcm_encode_cb);

    if (error)
    {
        a2dp_loge("reg pcm encode cb err %d", error);
        goto error;
    }

    error = bk_a2dp_source_register_pcm_resample_callback(a2dp_source_pcm_resample_cb);

    if (error)
    {
        a2dp_loge("reg pcm resample cb err %d", error);
        goto error;
    }

    error = avdtp_source_start();

    if (error)
    {
        a2dp_loge("start err!!!");
        goto error;
    }

    a2dp_env.read_cb_pause = 0;
    return 0;
error:;
    bt_a2dp_source_demo_music_stop();
    return error;
}

int bt_a2dp_source_demo_music_stop(void)
{
    int32_t ret = avdtp_source_suspend();

    //a2dp_source_demo_stop_mp3_decode_task();

    return ret;
}

int bt_a2dp_source_demo_music_pause(void)
{
    a2dp_env.read_cb_pause = 1;
    return 0;
    //return avdtp_source_suspend();
}

int bt_a2dp_source_demo_music_resume(void)
{
    bk_err_t error = 0;

    //    error = avdtp_source_start();
    //
    //    if (error)
    //    {
    //        a2dp_loge("start err!!!");
    //        return error;
    //    }

    a2dp_env.read_cb_pause = 0;
    return error;
}

int bt_a2dp_source_demo_music_prev(void)
{
    //todo: currently not impl music prev and next function, so play same music now.
    bt_a2dp_source_demo_music_stop();

    return bt_a2dp_source_demo_music_play(1, s_file_path);
}

int bt_a2dp_source_demo_music_next(void)
{
    //todo: currently not impl music prev and next function, so play same music now.
    bt_a2dp_source_demo_music_stop();

    return bt_a2dp_source_demo_music_play(1, s_file_path);
}

int bt_a2dp_source_demo_get_play_status(void)
{
    if (a2dp_env.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED &&
            a2dp_env.read_cb_pause == 0 &&
            a2dp_env.start_status &&
            bt_a2dp_source_decode_thread_handle)
    {
        return A2DP_PLAY_STATUS_PLAYING;
    }

    if (a2dp_env.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED &&
            a2dp_env.read_cb_pause &&
            a2dp_env.start_status &&
            bt_a2dp_source_decode_thread_handle)
    {
        return A2DP_PLAY_STATUS_PAUSED;
    }

    return A2DP_PLAY_STATUS_STOPPED;
}

uint32_t bt_a2dp_source_demo_get_play_pos(void)
{
    if (bt_a2dp_source_demo_get_play_status() == A2DP_PLAY_STATUS_STOPPED)
    {
        return 0xFFFFFFFF;
    }
    else
    {
        //todo: not impl
        return 0;
    }
}

static int avdtp_source_suspend(void)
{
    int err = kNoErr;

    if (!s_bt_api_event_cb_sema)
    {
        err = rtos_init_semaphore(&s_bt_api_event_cb_sema, 1);

        if (err)
        {
            a2dp_loge("sem init err %d", err);
            goto error;
        }
    }

    if (a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        a2dp_loge("remote device is not connected");
        err = -1;
        goto error;
    }

    if (a2dp_env.start_status == 0)
    {
        a2dp_loge("is already suspend");
        return 0;
    }

    a2dp_env.local_action_pending = 1;
    err = bk_a2dp_media_ctrl(BK_A2DP_MEDIA_CTRL_SUSPEND);

    if (err)
    {
        a2dp_loge("suspend err %d", err);
        goto error;
    }

    err = rtos_get_semaphore(&s_bt_api_event_cb_sema, 6 * 1000);

    if (err)
    {
        a2dp_loge("get sem err");
        goto error;
    }

    a2dp_env.local_action_pending = 0;
    return 0;
error:
    a2dp_env.local_action_pending = 0;
    return err;
}

static int32_t a2dp_source_demo_sbc_encoder_deinit(void)
{
    int ret = 0;

#if PCM_USE_AMP_CALL

    if (s_is_cpu1_task_ready)
    {
        ret = media_send_msg_sync(EVENT_BT_PCM_ENCODE_DEINIT_REQ, 0);

        if (ret)
        {
            a2dp_loge("EVENT_BT_PCM_ENCODE_DEINIT_REQ err %d !!", ret);
            return -1;
        }
    }

#endif

    os_memset(&s_sbc_software_encoder_ctx, 0, sizeof(s_sbc_software_encoder_ctx));

    return ret;
}

static int32_t a2dp_source_demo_sbc_encoder_init(void)
{
    bt_err_t ret = 0;

    a2dp_logi("");

    os_memset(&s_sbc_software_encoder_ctx, 0, sizeof(s_sbc_software_encoder_ctx));

    sbc_encoder_init(&s_sbc_software_encoder_ctx, s_a2dp_cap_info.cie.sbc_codec.sample_rate, 1);

    uint8_t alloc_mode = 0;

    switch (s_a2dp_cap_info.cie.sbc_codec.alloc_mode)
    {
    case 2://A2DP_SBC_ALLOCATION_METHOD_SNR:
        alloc_mode = 1;
        break;

    default:
    case 1://A2DP_SBC_ALLOCATION_METHOD_LOUDNESS:
        alloc_mode = 0;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD, alloc_mode); //0:loundness, 1:SNR

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD err %d", ret);
        return ret;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_BITPOOL, s_a2dp_cap_info.cie.sbc_codec.bit_pool);

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_BITPOOL err %d", ret);
        return ret;
    }


    uint8_t block_mode = 3;

    switch (s_a2dp_cap_info.cie.sbc_codec.block_len)
    {
    case 4://A2DP_SBC_BLOCK_LENGTH_4:
        block_mode = 0;
        break;

    case 8://A2DP_SBC_BLOCK_LENGTH_8:
        block_mode = 1;
        break;

    case 12://A2DP_SBC_BLOCK_LENGTH_12:
        block_mode = 2;
        break;

    default:
    case 16://A2DP_SBC_BLOCK_LENGTH_16:
        block_mode = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE, block_mode); //0:4, 1:8, 2:12, 3:16

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE err %d", ret);
        return ret;
    }

    uint8_t channle_mode = 3;

    switch (s_a2dp_cap_info.cie.sbc_codec.channel_mode)
    {
    case 8:
        channle_mode = 0;
        break;

    case 4:
        channle_mode = 1;
        break;

    case 2:
        channle_mode = 2;
        break;

    default:
    case 1:
        channle_mode = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE, channle_mode); //0:MONO, 1:DUAL, 2:STEREO, 3:JOINT STEREO

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE err %d", ret);
        return ret;
    }

    uint8_t samp_rate_select = 2;

    switch (s_a2dp_cap_info.cie.sbc_codec.sample_rate)
    {
    case 16000:
        samp_rate_select = 0;
        break;

    case 32000:
        samp_rate_select = 1;
        break;

    default:
    case 44100:
        samp_rate_select = 2;
        break;

    case 48000:
        samp_rate_select = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX, samp_rate_select); //0:16000, 1:32000, 2:44100, 3:48000

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX err %d", ret);
        return ret;
    }

    uint8_t subband = 1;

    switch (s_a2dp_cap_info.cie.sbc_codec.subbands)
    {
    case 4:
        subband = 0;
        break;

    default:
    case 8:
        subband = 1;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE, subband); //0:4, 1:8

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        a2dp_loge("SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE err %d", ret);
        return ret;
    }

#if PCM_USE_AMP_CALL

    ret = media_send_msg_sync(EVENT_BT_PCM_ENCODE_INIT_REQ, 0);

    if (ret)
    {
        a2dp_loge("EVENT_BT_PCM_ENCODE_INIT_REQ err %d !!", ret);
        return -1;
    }

#endif

    a2dp_logi("sbc encode count %d", s_sbc_software_encoder_ctx.pcm_length, A2DP_SOURCE_WRITE_AUTO_TIMER_MS * s_sbc_software_encoder_ctx.sample_rate);

    return 0;
}


static int32_t get_mp3_info(MP3FrameInfo *info)
{
    FRESULT fr = 0;
    char full_path[64] = {0};
    uint8_t tag_header[10] = {0};
    unsigned int num_rd = 0;
    uint8_t *mp3_read_start_ptr = NULL;
    uint8_t *pcm_write_ptr = NULL;
    int bytesleft = 0;
    uint8_t *current_mp3_read_ptr = NULL;
    uint8_t id3v2_maj_ver = 0;
    uint16_t id3v2_min_ver = 0;
    uint32_t file_size = 0;
    FATFS *s_pfs = NULL;
    uint32_t frame_start_offset = 0;
    int ret = 0;
    HMP3Decoder *s_mp3_decoder = NULL;

    (void)id3v2_maj_ver;

    os_memset(info, 0, sizeof(*info));

    s_pfs = os_malloc(sizeof(*s_pfs));

    if (!s_pfs)
    {
        a2dp_loge("s_pfs malloc failed!");
        goto error;
    }

    os_memset(s_pfs, 0, sizeof(*s_pfs));

    fr = f_mount(s_pfs, "1:", 1);

    if (fr != FR_OK)
    {
        a2dp_loge("f_mount failed:%d", fr);
        goto error;
    }

    a2dp_logi("f_mount OK!");

    MP3SetBuffMethod(mp3_private_alloc, mp3_private_free, mp3_private_memset);
    //MP3SetBuffMethodAlwaysFourAlignedAccess(mp3_private_alloc_psram, mp3_private_free_psram, mp3_private_memset_psram);

    s_mp3_decoder = MP3InitDecoder();

    if (!s_mp3_decoder)
    {
        a2dp_loge("s_mp3_decoder MP3InitDecoder failed!");
        goto error;
    }

    a2dp_logi("MP3InitDecoder init successful!");

    /*open file to read mp3 data */
    sprintf((char *)full_path, "%d:/%s", DISK_NUMBER_SDIO_SD, s_file_path);
    fr = f_open(&mp3file, full_path, FA_OPEN_EXISTING | FA_READ);

    if (fr != FR_OK)
    {
        a2dp_loge("open %s failed!", full_path);
        goto error;
    }

    fr = f_read(&mp3file, (void *)tag_header, sizeof(tag_header), &num_rd);

    if (fr != FR_OK || num_rd != sizeof(tag_header))
    {
        a2dp_loge("read %s failed!", full_path);
        goto error;
    }

    if (os_memcmp(tag_header, "ID3", 3))
    {
        FILINFO info;
        uint8_t id3v1[128] = {0};

        //a2dp_loge("ID3v2 not found!, try found ID3v1");

        os_memset(&info, 0, sizeof(info));
        fr = f_stat(full_path, &info);

        if (fr != FR_OK)
        {
            a2dp_loge("f_stat err %d", fr);
            goto error;
        }

        file_size = info.fsize;

        fr = f_lseek(&mp3file, file_size - 128);

        if (fr != FR_OK)
        {
            a2dp_loge("f_lseek to ID3v1 end err %d", fr);
            goto error;
        }

        fr = f_read(&mp3file, id3v1, sizeof(id3v1), &num_rd);

        if (fr != FR_OK || num_rd != sizeof(id3v1))
        {
            a2dp_loge("read ID3v1 err %d num %d", fr, num_rd);
            goto error;
        }

        if (os_memcmp(id3v1, "TAG", 3))
        {
            a2dp_loge("ID3v1 not found!");
            goto error;
        }

        frame_start_offset = 0;

        f_lseek(&mp3file, frame_start_offset);

        a2dp_logi("found ID3v1");

        id3v2_maj_ver = 1;
    }
    else
    {
        uint32_t tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
        frame_start_offset = sizeof(tag_header) + tag_size;

        id3v2_min_ver = ((tag_header[4] << 8) | tag_header[3]);
        id3v2_maj_ver = 2;

        a2dp_logi("ID3v2.%d flag 0x%x len %d", id3v2_min_ver, tag_header[5], tag_size);

        if (id3v2_min_ver == 4 && (tag_header[5] & (1 << 4)))
        {
            frame_start_offset += 10;
        }

        f_lseek(&mp3file, frame_start_offset);
    }

    a2dp_logi("mp3 file open successfully!");

    mp3_read_start_ptr = os_malloc(MAINBUF_SIZE * 2);

    if (!mp3_read_start_ptr)
    {
        a2dp_loge("mp3_read_ptr alloc err");
        goto error;
    }

    pcm_write_ptr = os_malloc(MP3_DECODE_BUFF_SIZE);

    if (!pcm_write_ptr)
    {
        a2dp_loge("pcm_write_ptr alloc err");
        goto error;
    }

    //get frame info
    {
        fr = f_read(&mp3file, mp3_read_start_ptr, MAINBUF_SIZE, &num_rd);

        if (fr != FR_OK)
        {
            a2dp_loge("test read frame %d %s failed!", num_rd, full_path);
            goto error;
        }

        bytesleft = MP3_DECODE_BUFF_SIZE;

        current_mp3_read_ptr = mp3_read_start_ptr;

        ret = MP3Decode(s_mp3_decoder, &current_mp3_read_ptr, &bytesleft, (int16_t *)pcm_write_ptr, 0);

        if (ret != ERR_MP3_NONE)
        {
            a2dp_loge("MP3Decode failed, code is %d bytesleft %d", ret, bytesleft);
            goto error;
        }

        MP3GetLastFrameInfo(s_mp3_decoder, &s_mp3_frame_info);

        a2dp_logi("bytesleft %d readsize %d", bytesleft, f_tell(&mp3file));
        a2dp_logi("Bitrate: %d kb/s, Samprate: %d, samplebits %d", (s_mp3_frame_info.bitrate) / 1000, s_mp3_frame_info.samprate, s_mp3_frame_info.bitsPerSample);
        a2dp_logi("Channel: %d, Version: %d, Layer: %d", s_mp3_frame_info.nChans, s_mp3_frame_info.version, s_mp3_frame_info.layer);
        a2dp_logi("OutputSamps: %d %d", s_mp3_frame_info.outputSamps, s_mp3_frame_info.outputSamps * s_mp3_frame_info.bitsPerSample / 8);

        f_lseek(&mp3file, frame_start_offset);
    }
error:;

    if (s_mp3_decoder)
    {
        MP3FreeDecoder(s_mp3_decoder);
        s_mp3_decoder = NULL;
    }

    if (mp3file.fs)
    {
        f_close(&mp3file);
        os_memset(&mp3file, 0, sizeof(mp3file));
    }

    if (s_pfs)
    {
        fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);

        if (fr)
        {
            a2dp_loge("f_unmount err %d", fr);
        }

        os_free(s_pfs);
        s_pfs = NULL;
    }

    if (pcm_write_ptr)
    {
        os_free(pcm_write_ptr);
        pcm_write_ptr = NULL;
    }

    if (mp3_read_start_ptr)
    {
        os_free(mp3_read_start_ptr);
        mp3_read_start_ptr = NULL;
    }

    return 0;
}

static void bt_a2dp_source_decode_task(void *arg)
{
    FRESULT fr = 0;
    char full_path[64] = {0};
    uint8_t tag_header[10] = {0};
    unsigned int num_rd = 0;
    uint8_t *mp3_read_start_ptr = NULL;
    uint8_t *pcm_write_ptr = NULL;
    int bytesleft = 0;
    uint8_t *current_mp3_read_ptr = NULL;
    uint8_t *mp3_read_end_ptr = NULL;
    uint8_t id3v2_maj_ver = 0;
    uint16_t id3v2_min_ver = 0;
    uint32_t file_size = 0;
    uint32_t pcm_decode_size = 0;

    uint32_t frame_start_offset = 0;
    int ret = 0;
    MP3FrameInfo tmp_mp3_frame_info;
    HMP3Decoder *s_mp3_decoder = NULL;
    uint8_t *task_ctrl = (typeof(task_ctrl))arg;
    FATFS *s_pfs = NULL;

    *task_ctrl = 1;

    s_pfs = os_malloc(sizeof(FATFS));

    if (NULL == s_pfs)
    {
        a2dp_loge("s_pfs malloc failed!");
        goto error;
    }

    os_memset(&tmp_mp3_frame_info, 0, sizeof(tmp_mp3_frame_info));
    os_memset(s_pfs, 0, sizeof(*s_pfs));

    fr = f_mount(s_pfs, "1:", 1);

    if (fr != FR_OK)
    {
        a2dp_loge("f_mount failed:%d", fr);
        goto error;
    }

    //can't free !!!!!
    //    os_free(s_pfs);
    //    s_pfs = NULL;

    a2dp_logi("f_mount OK!");

    MP3SetBuffMethod(mp3_private_alloc, mp3_private_free, mp3_private_memset);
    //MP3SetBuffMethodAlwaysFourAlignedAccess(mp3_private_alloc_psram, mp3_private_free_psram, mp3_private_memset_psram);

    s_mp3_decoder = MP3InitDecoder();

    if (!s_mp3_decoder)
    {
        a2dp_loge("s_mp3_decoder MP3InitDecoder failed!");
        goto error;
    }

    a2dp_logi("MP3InitDecoder init successful!");

    /*open file to read mp3 data */
    sprintf((char *)full_path, "%d:/%s", DISK_NUMBER_SDIO_SD, s_file_path);
    fr = f_open(&mp3file, full_path, FA_OPEN_EXISTING | FA_READ);

    if (fr != FR_OK)
    {
        a2dp_loge("open %s failed!", full_path);
        goto error;
    }

    fr = f_read(&mp3file, (void *)tag_header, sizeof(tag_header), &num_rd);

    if (fr != FR_OK || num_rd != sizeof(tag_header))
    {
        a2dp_loge("read %s failed!", full_path);
        goto error;
    }

    if (os_memcmp(tag_header, "ID3", 3))
    {
        FILINFO info;
        uint8_t id3v1[128] = {0};

        //a2dp_loge("ID3v2 not found!, try found ID3v1");

        os_memset(&info, 0, sizeof(info));
        fr = f_stat(full_path, &info);

        if (fr != FR_OK)
        {
            a2dp_loge("f_stat err %d", fr);
            goto error;
        }

        file_size = info.fsize;

        fr = f_lseek(&mp3file, file_size - 128);

        if (fr != FR_OK)
        {
            a2dp_loge("f_lseek to ID3v1 end err %d", fr);
            goto error;
        }

        fr = f_read(&mp3file, id3v1, sizeof(id3v1), &num_rd);

        if (fr != FR_OK || num_rd != sizeof(id3v1))
        {
            a2dp_loge("read ID3v1 err %d num %d", fr, num_rd);
            goto error;
        }

        if (os_memcmp(id3v1, "TAG", 3))
        {
            a2dp_loge("ID3v1 not found!");
            goto error;
        }

        frame_start_offset = 0;

        f_lseek(&mp3file, frame_start_offset);

        a2dp_logi("found ID3v1");

        id3v2_maj_ver = 1;
    }
    else
    {
        uint32_t tag_size = ((tag_header[6] & 0x7F) << 21) | ((tag_header[7] & 0x7F) << 14) | ((tag_header[8] & 0x7F) << 7) | (tag_header[9] & 0x7F);
        frame_start_offset = sizeof(tag_header) + tag_size;

        id3v2_min_ver = ((tag_header[4] << 8) | tag_header[3]);
        id3v2_maj_ver = 2;

        a2dp_logi("ID3v2.%d flag 0x%x len %d", id3v2_min_ver, tag_header[5], tag_size);

        if (id3v2_min_ver == 4 && (tag_header[5] & (1 << 4)))
        {
            frame_start_offset += 10;
        }

        f_lseek(&mp3file, frame_start_offset);
    }

    a2dp_logi("mp3 file open successfully!");

    mp3_read_start_ptr = os_malloc(MAINBUF_SIZE * 3 / 2);

    if (!mp3_read_start_ptr)
    {
        a2dp_loge("mp3_read_ptr alloc err");
        goto error;
    }

    pcm_write_ptr = os_malloc(MP3_DECODE_BUFF_SIZE);

    if (!pcm_write_ptr)
    {
        a2dp_loge("pcm_write_ptr alloc err");
        goto error;
    }

    bytesleft = 0;
    current_mp3_read_ptr = mp3_read_start_ptr;

    mp3_read_end_ptr = mp3_read_start_ptr + MAINBUF_SIZE * 3 / 2;

    while (*task_ctrl)
    {
        if (current_mp3_read_ptr - mp3_read_start_ptr > MAINBUF_SIZE)
        {
            a2dp_logv("%p move to %p %d %d", current_mp3_read_ptr, mp3_read_start_ptr, current_mp3_read_ptr - mp3_read_start_ptr, bytesleft);

            os_memmove(mp3_read_start_ptr, current_mp3_read_ptr, bytesleft);
            current_mp3_read_ptr = mp3_read_start_ptr;
        }

        if (0 != mp3_read_end_ptr - current_mp3_read_ptr - bytesleft)
        {
            fr = f_read(&mp3file, current_mp3_read_ptr + bytesleft, mp3_read_end_ptr - current_mp3_read_ptr - bytesleft, &num_rd);

            if (fr != FR_OK)
            {
                a2dp_loge("read %d %s failed!", num_rd, full_path);
                goto error;
            }

            if (!num_rd)
            {
                a2dp_logi("file end, return to begin");

                os_memmove(mp3_read_start_ptr, current_mp3_read_ptr, bytesleft);
                current_mp3_read_ptr = mp3_read_start_ptr;

                f_lseek(&mp3file, frame_start_offset);
                continue;
            }

            if (id3v2_maj_ver == 1 && file_size - f_tell(&mp3file) <= 128)
            {
                num_rd -= f_tell(&mp3file) - (file_size - 128);
            }

            bytesleft += num_rd;
        }

        a2dp_logv("bytesleft %d %p", bytesleft, current_mp3_read_ptr);

        do
        {
            uint8_t *last_success_read_ptr = current_mp3_read_ptr;
            int last_success_bytesleft = bytesleft;

            if (!bytesleft)
            {
                break;
            }

            while (ring_buffer_particle_len(&s_rb_ctx) > s_decode_trigger_size)
            {
                if (!*task_ctrl)
                {
                    goto error;
                }

                a2dp_logd("ring buffer not read too much, wait %d", ring_buffer_particle_len(&s_rb_ctx));
                rtos_get_semaphore(&s_source_need_decode_sema, BEKEN_WAIT_FOREVER);
            }

            ret = MP3Decode(s_mp3_decoder, &current_mp3_read_ptr, &bytesleft, (int16_t *)pcm_write_ptr, 0);

            if (ret != ERR_MP3_NONE)
            {
                if (ERR_MP3_INDATA_UNDERFLOW == ret)
                {
                    bytesleft = last_success_bytesleft;
                    current_mp3_read_ptr = last_success_read_ptr;
                    break;
                }

                a2dp_loge("MP3Decode failed %d bytesleft %d %d", ret, bytesleft, last_success_bytesleft);
                goto error;
            }

            //            os_memset(&s_mp3_frame_info, 0, sizeof(s_mp3_frame_info));
            MP3GetLastFrameInfo(s_mp3_decoder, &tmp_mp3_frame_info);

            a2dp_logv("start write ring buff %d", tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8);

            if (ring_buffer_particle_write(&s_rb_ctx, pcm_write_ptr, tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8))
            {
                a2dp_logd("ring_buffer full %d", ring_buffer_particle_len(&s_rb_ctx));
                //ring_buffer_particle_debug(&s_rb_ctx);

                bytesleft = last_success_bytesleft;
                current_mp3_read_ptr = last_success_read_ptr;
                continue;
            }
            else
            {
                a2dp_logv("write %d, %d", tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8, ring_buffer_particle_len(&s_rb_ctx));
            }

            pcm_decode_size += tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8;

            //            a2dp_logv("bytesleft %d %p readsize %d", bytesleft, current_mp3_read_ptr, f_tell(&mp3file));
            //            a2dp_logv("Bitrate: %d kb/s, Samprate: %d", (tmp_mp3_frame_info.bitrate) / 1000, tmp_mp3_frame_info.samprate);
            //            a2dp_logv("Channel: %d, Version: %d, Layer: %d", tmp_mp3_frame_info.nChans, tmp_mp3_frame_info.version, tmp_mp3_frame_info.layer);
            //            a2dp_logv("OutputSamps: %d %d", tmp_mp3_frame_info.outputSamps, tmp_mp3_frame_info.outputSamps * tmp_mp3_frame_info.bitsPerSample / 8);
        }
        while (tmp_mp3_frame_info.outputSamps && *task_ctrl);

        if (0)//!num_rd || (id3v2_maj_ver == 1 && file_size - f_tell(&mp3file) <= 128))
        {
            a2dp_logi("decode end %d %d", bytesleft, pcm_decode_size);
            a2dp_logi("samplerate %d channel %d ver %d", tmp_mp3_frame_info.samprate, tmp_mp3_frame_info.nChans, tmp_mp3_frame_info.version);
            break;
        }

        if (!*task_ctrl)
        {
            goto error;
        }
    }

error:;

    if (s_mp3_decoder)
    {
        MP3FreeDecoder(s_mp3_decoder);
        s_mp3_decoder = NULL;
    }

    if (mp3file.fs)
    {
        f_close(&mp3file);
        os_memset(&mp3file, 0, sizeof(mp3file));
    }

    if (s_pfs)
    {
        fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);

        if (fr)
        {
            a2dp_loge("f_unmount err %d", fr);
        }

        os_free(s_pfs);
        s_pfs = NULL;
    }

    if (pcm_write_ptr)
    {
        os_free(pcm_write_ptr);
        pcm_write_ptr = NULL;
    }

    if (mp3_read_start_ptr)
    {
        os_free(mp3_read_start_ptr);
        mp3_read_start_ptr = NULL;
    }

    a2dp_logi("exit");
    //bt_a2dp_source_decode_thread_handle = NULL;
    rtos_delete_thread(NULL);

    return;
}

static bk_err_t a2dp_source_demo_stop_mp3_decode_task(void)
{
    int ret = 0;

    a2dp_logi("step 1");

    if (bt_a2dp_source_decode_thread_handle)
    {
        s_decode_task_run = 0;
        rtos_set_semaphore(&s_source_need_decode_sema);
        rtos_thread_join(&bt_a2dp_source_decode_thread_handle);
        bt_a2dp_source_decode_thread_handle = NULL;
    }

    ring_buffer_particle_deinit(&s_rb_ctx);

    if (s_source_need_decode_sema)
    {
        rtos_deinit_semaphore(&s_source_need_decode_sema);
        s_source_need_decode_sema = NULL;
    }

    s_decode_trigger_size = 0;

    a2dp_logi("step 2");

    if (s_is_bk_aud_rsp_inited)
    {
#if PCM_USE_AMP_CALL

        if (s_is_cpu1_task_ready)
        {
            ret = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_DEINIT_REQ, 0);

            if (ret)
            {
                a2dp_loge("EVENT_BT_PCM_RESAMPLE_DEINIT_REQ err %d !!", ret);
            }
        }

#else
        (void)ret;
        bk_aud_rsp_deinit();
#endif
        s_is_bk_aud_rsp_inited = 0;
    }

    a2dp_logi("step 3");

    a2dp_source_demo_sbc_encoder_deinit();

    ret = bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);

    if (ret)
    {
        a2dp_loge("set cpu fre to default err %d !!", ret);
    }
	
#if PCM_USE_AMP_CALL

    if (s_is_cpu1_task_ready)
    {
        a2dp_logi("step 4");
        s_is_cpu1_task_ready = 0;

        ret = media_send_msg_sync(EVENT_BT_AUDIO_DEINIT_REQ, 0);

        if (ret)
        {
            a2dp_loge("EVENT_BT_AUDIO_DEINIT_REQ err %d !!", ret);
        }

        ret = bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);

        if (ret)
        {
            a2dp_loge("set cpu fre to default err %d !!", ret);
        }

        ret = bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

        if (ret)
        {
            a2dp_loge("cpu1 power down err %d !!", ret);
        }
    }
#endif

    return 0;
}

static bk_err_t a2dp_source_demo_create_mp3_decode_task(void)
{
    bk_err_t err = 0;

    if (bt_a2dp_source_decode_thread_handle)
    {
        a2dp_loge("already create");
        return 0;
    }

    s_decode_trigger_size = 0;
    bk_audio_osi_funcs_init();
    get_mp3_info(&s_mp3_frame_info);

    if (!s_mp3_frame_info.samprate)
    {
        a2dp_loge("get_mp3_info err !!");
        err = -1;
        goto error;
    }
    else
    {
        a2dp_logi("get_mp3_info success !!");
    }

    s_decode_trigger_size = ((size_t)(s_mp3_frame_info.samprate * s_mp3_frame_info.nChans * (s_mp3_frame_info.bitsPerSample / 8) * DECODE_TRIGGER_TIME / 1000));

    if (!s_source_need_decode_sema)
    {
        a2dp_logd("start rtos_init_semaphore");
        err = rtos_init_semaphore(&s_source_need_decode_sema, 1);
        a2dp_logd("end rtos_init_semaphore");
    }

    if (err)
    {
        a2dp_loge("sem init s_source_need_decode_sema err %d", err);
        goto error;
    }

    if (ring_buffer_particle_is_init(&s_rb_ctx))
    {
        a2dp_loge("rb is already init");
        err = -1;
        goto error;
    }

    err = ring_buffer_particle_init(&s_rb_ctx, DECODE_RB_SIZE);

    if (err)
    {
        a2dp_loge("pcm_decode_ring_buffer alloc err");
        goto error;
    }

#if PCM_USE_AMP_CALL

    err = bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);

    if (err)
    {
        a2dp_loge("cpu1 power up err %d !!", err);
        err = -1;
        goto error;
    }

    //rtos_delay_milliseconds(500);
    err = bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_320M);

    if (err)
    {
        a2dp_loge("set cpu fre err %d !!", err);
        err = -1;
        goto error;
    }

    err = media_send_msg_sync(EVENT_BT_AUDIO_INIT_REQ, 0);

    if (err)
    {
        a2dp_loge("EVENT_BT_AUDIO_INIT_REQ err %d !!", err);
        err = -1;
        goto error;
    }

    s_is_cpu1_task_ready = 1;

#endif

    err = bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, A2DP_CPU_FRQ);

    if (err)
    {
        a2dp_loge("set cpu fre err %d !!", err);
        err = -1;
        goto error;
    }



    err = a2dp_source_demo_sbc_encoder_init();

    if (err)
    {
        a2dp_loge("a2dp_source_demo_sbc_encoder_init alloc err");
        goto error;
    }

    if (s_mp3_frame_info.samprate != s_a2dp_cap_info.cie.sbc_codec.sample_rate || s_mp3_frame_info.bitsPerSample != SBC_SAMPLE_DEPTH)
    {
        if (!s_is_bk_aud_rsp_inited)
        {

#if PCM_USE_AMP_CALL

            const bt_audio_resample_init_req_t rsp_init_req =
            {
                .src_rate = s_mp3_frame_info.samprate,
                .src_ch = s_a2dp_cap_info.cie.sbc_codec.channels,//s_input_pcm_info.nChans,
                .src_bits = s_mp3_frame_info.bitsPerSample,
                .dest_rate = s_a2dp_cap_info.cie.sbc_codec.sample_rate,
                .dest_ch = s_a2dp_cap_info.cie.sbc_codec.channels,
                .dest_bits = SBC_SAMPLE_DEPTH,
                .complexity = 0,
                .down_ch_idx = 0,
            };

            err = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_INIT_REQ, (uint32_t)&rsp_init_req);

            if (err)
            {
                a2dp_loge("EVENT_BT_PCM_RESAMPLE_INIT_REQ err %d !!", err);
                err = -1;
                goto error;
            }

#else

            const aud_rsp_cfg_t cfg =
            {
                .src_rate = s_mp3_frame_info.samprate,
                .src_ch = s_a2dp_cap_info.cie.sbc_codec.channels,//s_input_pcm_info.nChans,
                .src_bits = s_mp3_frame_info.bitsPerSample,
                .dest_rate = s_a2dp_cap_info.cie.sbc_codec.sample_rate,
                .dest_ch = s_a2dp_cap_info.cie.sbc_codec.channels,
                .dest_bits = SBC_SAMPLE_DEPTH,
                .complexity = 0,
                .down_ch_idx = 0,
            };

            a2dp_logd("start bk_aud_rsp_init");
            err = bk_aud_rsp_init(cfg);
            a2dp_logd("end bk_aud_rsp_init");

            if (err)
            {
                a2dp_loge("bk_aud_rsp_init err %d !!", err);
                goto error;
            }

#endif

            s_is_bk_aud_rsp_inited = 1;
            a2dp_logi("bk_aud_rsp_init ok");
        }
    }

    if (!bt_a2dp_source_decode_thread_handle)
    {
        err = rtos_create_thread(&bt_a2dp_source_decode_thread_handle,
                                 TASK_PRIORITY - 2,
                                 "a2dp_source_decode",
                                 (beken_thread_function_t)bt_a2dp_source_decode_task,
                                 1024 * 3,
                                 (beken_thread_arg_t)&s_decode_task_run);

        if (err)
        {
            a2dp_loge("task fail");

            bt_a2dp_source_decode_thread_handle = NULL;
            goto error;
        }
        else
        {
            while (ring_buffer_particle_len(&s_rb_ctx) < DECODE_RB_SIZE / 2)
            {
                rtos_delay_milliseconds(100);
            }
        }
    }

    return 0;

error:;

    a2dp_source_demo_stop_mp3_decode_task();

    return err;
}


bk_err_t bt_a2dp_source_demo_stop_all(void)
{
    a2dp_logd("step 1");
    a2dp_source_demo_stop_mp3_decode_task();
    a2dp_logd("step 2");
    bt_a2dp_source_demo_disconnect(a2dp_env.peer_addr.addr);

    return 0;
}

static void a2dp_source_demo_main_task(void *arg)
{
    bk_err_t error = BK_OK;
    uint8_t *addr = (typeof(addr))arg;

    BK_ASSERT(4 == INT_CEIL(3, 4));

    if (!a2dp_env.inited)
    {
        bk_bt_a2dp_source_init();
        bk_bt_a2dp_register_callback(bk_bt_a2dp_source_event_cb);
        a2dp_env.inited = 1;
    }

    os_memcpy(a2dp_env.peer_addr.addr, addr, sizeof(a2dp_env.peer_addr.addr));


    error = bt_a2dp_source_demo_connect(a2dp_env.peer_addr.addr);

    if (error)
    {
        a2dp_loge("connect err!!!");
        goto error;
    }

    error = bt_a2dp_source_demo_music_play(1, s_file_path);

    if (error)
    {
        goto error;
    }

#if USER_A2DP_MAIN_TASK
    rtos_delete_thread(NULL);
#endif
    return;
error:;

    bt_a2dp_source_demo_stop_all();

#if USER_A2DP_MAIN_TASK
    rtos_delete_thread(NULL);
#endif
    return;
}

bk_err_t bt_a2dp_source_demo_test(uint8_t *addr, uint8_t is_mp3, uint8_t *file_path)
{
#if USER_A2DP_MAIN_TASK
    bk_err_t err = 0;

    if (!bt_a2dp_source_main_thread_handle)
    {
        os_strcpy((char *)s_file_path, (char *)file_path);
        s_is_mp3 = is_mp3;

        err = rtos_create_thread(&bt_a2dp_source_main_thread_handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                                 "a2dp_source_main",
                                 (beken_thread_function_t)a2dp_source_demo_main_task,
                                 1024 * 4,
                                 (beken_thread_arg_t)addr);

        if (err)
        {
            a2dp_loge("task fail");

            bt_a2dp_source_main_thread_handle = NULL;
            goto error;
        }
    }

    rtos_thread_join(bt_a2dp_source_main_thread_handle);
    bt_a2dp_source_main_thread_handle = NULL;

    return 0;
error:;
    return err;
#else
    os_strcpy((char *)s_file_path, (char *)file_path);
    a2dp_source_demo_main_task((void *)addr);
    return 0;
#endif
}

bk_err_t bt_a2dp_source_demo_test_performance(uint32_t cpu_fre, uint32_t bytes, uint32_t loop, uint32_t cpu_id)
{
    bk_err_t ret = 0;
    uint8_t init = 0;
    const uint32_t src_bytes = bytes;//256;//1024 * 4;
    const uint32_t dest_bytes = 4 + (typeof(dest_bytes))(src_bytes * 48000.0 / 44100);
    const uint32_t loop_count = loop;
    uint8_t *input_buff = NULL, *output_buff = NULL;

    if (s_is_bk_aud_rsp_inited)
    {
        a2dp_loge("rsp is run");
        ret = -1;
        goto error;
    }

#if PCM_USE_AMP_CALL

    if (cpu_id >= 1)
    {
        ret = bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);

        if (ret)
        {
            a2dp_loge("cpu1 power up err %d !!", ret);
            ret = -1;
            goto error;
        }

        //rtos_delay_milliseconds(500);

        ret = media_send_msg_sync(EVENT_BT_AUDIO_INIT_REQ, 0);

        if (ret)
        {
            a2dp_loge("EVENT_BT_AUDIO_INIT_REQ err %d !!", ret);
            ret = -1;
            goto error;
        }
    }

#endif

#if PCM_USE_AMP_CALL

    if (cpu_id >= 1)
    {
        const bt_audio_resample_init_req_t rsp_init_req =
        {
            .src_rate = 44100,
            .src_ch = 2,//s_input_pcm_info.nChans,
            .src_bits = 16,
            .dest_rate = 48000,
            .dest_ch = 2,
            .dest_bits = 16,
            .complexity = 0,
            .down_ch_idx = 0,
        };

        ret = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_INIT_REQ, (uint32_t)&rsp_init_req);

        if (ret)
        {
            a2dp_loge("EVENT_BT_PCM_RESAMPLE_INIT_REQ err %d !!", ret);
            ret = -1;
            goto error;
        }

    }
    else
#endif
    {
        const aud_rsp_cfg_t cfg =
        {
            .src_rate = 44100,
            .src_ch = 2,//s_input_pcm_info.nChans,
            .src_bits = 16,
            .dest_rate = 48000,
            .dest_ch = 2,
            .dest_bits = 16,
            .complexity = 0,
            .down_ch_idx = 0,
        };

        ret = bk_aud_rsp_init(cfg);

        if (ret)
        {
            a2dp_loge("bk_aud_rsp_init err %d !!", ret);
            ret = -1;
            goto error;
        }

    }

    init = 1;

    input_buff = os_malloc(src_bytes);
    output_buff = os_malloc(dest_bytes);

    if (!input_buff || !output_buff)
    {
        a2dp_loge("malloc err");
        ret = -1;
        goto error;
    }

    //    os_memset(input_buff, 0, src_bytes);
    //    os_memset(output_buff, 0, dest_bytes);

    if (cpu_fre)
    {
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, cpu_fre);//PM_CPU_FRQ_240M
    }

    beken_time_t before = rtos_get_time();

    for (int i = 0; i < loop_count; ++i)
    {
        uint32_t input_len = 0;
        uint32_t output_len = 0;
#if PCM_USE_AMP_CALL

        if (cpu_id >= 1)
        {
            bt_audio_resample_req_t rsp_req;

            os_memset(&rsp_req, 0, sizeof(rsp_req));

            input_len = src_bytes;
            output_len = dest_bytes;

            rsp_req.in_addr = input_buff;
            rsp_req.out_addr = output_buff;
            rsp_req.in_bytes_ptr = &input_len;
            rsp_req.out_bytes_ptr = &output_len;


            a2dp_logi("resample send evt %p %p %p %d %d", &rsp_req, rsp_req.in_addr, rsp_req.out_addr, input_len, output_len);
            ret = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_REQ, (uint32_t)&rsp_req);

            if (ret)
            {
                a2dp_loge("EVENT_BT_PCM_RESAMPLE_REQ err %d !!", ret);
                ret = -1;
                goto error;
            }
        }
        else
#endif
        {
            input_len = src_bytes / 2;
            output_len = dest_bytes / 2;

            ret = bk_aud_rsp_process((int16_t *)input_buff, &input_len, (int16_t *)output_buff, &output_len);

            if (ret)
            {
                a2dp_loge("bk_aud_rsp_process err %d !!", ret);
                goto error;
            }
        }
    }

    beken_time_t after = rtos_get_time();

    a2dp_logi("input bytes %dKB time cost %d", src_bytes * loop_count / 1024, after - before);

error:;

    if (init)
    {
#if PCM_USE_AMP_CALL

        if (cpu_id >= 1)
        {
            ret = media_send_msg_sync(EVENT_BT_PCM_RESAMPLE_DEINIT_REQ, 0);

            if (ret)
            {
                a2dp_loge("EVENT_BT_PCM_RESAMPLE_DEINIT_REQ err %d !!", ret);
            }
        }
        else
#endif
        {
            bk_aud_rsp_deinit();
        }
    }

    if (input_buff)
    {
        os_free(input_buff);
    }

    if (output_buff)
    {
        os_free(output_buff);
    }

    if (cpu_fre)
    {
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
    }

#if PCM_USE_AMP_CALL

    if (cpu_id >= 1)
    {
        ret = media_send_msg_sync(EVENT_BT_AUDIO_DEINIT_REQ, 0);

        if (ret)
        {
            a2dp_loge("EVENT_BT_AUDIO_DEINIT_REQ err %d !!", ret);
        }

        ret = bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

        if (ret)
        {
            a2dp_loge("cpu1 power down err %d !!", ret);
        }

    }

#endif

    return ret;
}
