#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"
#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>

#if CONFIG_VOICE_SERVICE_MP3_DECODER
#include <components/bk_audio/audio_decoders/modules/mp3_decoder.h>
#endif


#define TAG "voc"

#if 0
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#endif

#define VOICE_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, VOICE_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)

typedef enum
{
    LISTENER_IDLE = 0,
    LISTENER_START,
    LISTENER_EXIT
} listener_op_t;

typedef struct
{
    listener_op_t op;
    void *param;
} listener_msg_t;

struct voice
{
    audio_pipeline_handle_t record_pipeline;    /**< record pipeline handle, [mic]-->[aec]-->[encode]-->[raw_read] */
    audio_pipeline_handle_t play_pipeline;      /**< play pipeline handle, [raw_write]-->[decode]-->[speaker] */
    mic_type_t              mic_type;           /**< onboard mic or uac mic */
    audio_element_handle_t  mic_str;            /**< mic stream handle */
    bool                    aec_en;             /**< aec enable handle */
    audio_element_handle_t  aec_alg;            /**< aec algorithm handle */
    audio_port_handle_t     aec_alg_ref_rb;   /**< ringbuffer save refrence data of aec algorithm, [speaker]-->(ringbuffer)-->[aec] */
    audio_enc_type_t        enc_type;           /**< encoder type */
    audio_element_handle_t  mic_enc;            /**< mic encoder handle */
    audio_element_handle_t  raw_read;           /**< raw read stream handle, used to read data that has been encoded */

    audio_element_handle_t  raw_write;          /**< raw write stream handle, used to write data to decoder */
    audio_dec_type_t        dec_type;           /**< decoder type */
    audio_element_handle_t  spk_dec;            /**< speaker decoder handle */
    spk_type_t              spk_type;           /**< onboard speaker or uac speaker */
    audio_element_handle_t  spk_str;            /**< speaker stream handle */
    #if CONFIG_VOICE_SERVICE_EQ
    bool eq_en;
    audio_element_handle_t  eq_str;             /**< eq stream handle */
    #endif

    audio_event_iface_handle_t record_evt;      /**< speaker stream handle */
    audio_event_iface_handle_t play_evt;        /**< speaker stream handle */

    voice_sta_t             status;             /**< voice handle status */

    voice_event_handle      event_handle;       /**< voice event handle callback */
    void *                  args;               /**< the parameter of event_handle func */

    beken_thread_t          listener_task_hdl;
    beken_queue_t           listener_msg_que;
    beken_semaphore_t       listener_sem;
    bool                    listener_is_running;
};


static bk_err_t record_pipeline_deinit(voice_handle_t voice_handle)
{
    BK_LOGD(TAG, "%s\n", __func__);

    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->record_pipeline)
    {
        return BK_OK;
    }

    if (BK_OK != audio_pipeline_terminate(voice_handle->record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline terminate fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->mic_str && BK_OK != audio_pipeline_unregister(voice_handle->record_pipeline, voice_handle->mic_str))
    {
        BK_LOGE(TAG, "%s, %d, unregister mic_stream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->aec_en && voice_handle->aec_alg)
    {
        if (BK_OK != audio_pipeline_unregister(voice_handle->record_pipeline, voice_handle->aec_alg))
        {
            BK_LOGE(TAG, "%s, %d, unregister aec_algorithm fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    if (voice_handle->mic_enc && BK_OK != audio_pipeline_unregister(voice_handle->record_pipeline, voice_handle->mic_enc))
    {
        BK_LOGE(TAG, "%s, %d, unregister encoder fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->raw_read && BK_OK != audio_pipeline_unregister(voice_handle->record_pipeline, voice_handle->raw_read))
    {
        BK_LOGE(TAG, "%s, %d, unregister raw_read staream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->record_evt)
    {
        /* deinit listener */
        if (BK_OK != audio_pipeline_remove_listener(voice_handle->record_pipeline))
        {
            BK_LOGE(TAG, "%s, %d, remove record pipeline fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        if (BK_OK != audio_event_iface_destroy(voice_handle->record_evt))
        {
            BK_LOGE(TAG, "%s, %d, destroy record pipeline fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    if (BK_OK != audio_pipeline_deinit(voice_handle->record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->record_pipeline = NULL;
    }

    if (voice_handle->mic_str && BK_OK != audio_element_deinit(voice_handle->mic_str))
    {
        BK_LOGE(TAG, "%s, %d, mic_stream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->mic_str = NULL;
    }

    if (voice_handle->aec_en && voice_handle->aec_alg)
    {
        if (BK_OK != audio_element_deinit(voice_handle->aec_alg))
        {
            BK_LOGE(TAG, "%s, %d, aec_algorithm deinit fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        else
        {
            voice_handle->aec_alg = NULL;
        }
    }

    if (voice_handle->mic_enc && BK_OK != audio_element_deinit(voice_handle->mic_enc))
    {
        BK_LOGE(TAG, "%s, %d, encoder deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->mic_enc = NULL;
    }

    if (voice_handle->raw_read && BK_OK != audio_element_deinit(voice_handle->raw_read))
    {
        BK_LOGE(TAG, "%s, %d, raw_read staream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->raw_read = NULL;
    }

    return BK_OK;
}

static bk_err_t record_pipeline_init(voice_handle_t voice_handle, voice_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    BK_LOGD(TAG, "%s\n", __func__);

    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    BK_LOGD(TAG, "step1: record pipeline init\n");
    audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    record_pipeline_cfg.rb_size = 320;
    voice_handle->record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
    VOICE_CHECK_NULL(voice_handle->record_pipeline, goto fail);

    BK_LOGD(TAG, "step2: init record elements\n");
    if (voice_handle->mic_type == MIC_TYPE_ONBOARD)
    {
        voice_handle->mic_str = onboard_mic_stream_init(&cfg->mic_cfg.onboard_mic_cfg);
        VOICE_CHECK_NULL(voice_handle->mic_str, goto fail);
    }
#if CONFIG_ADK_UAC_MIC_STREAM
    else if (voice_handle->mic_type == MIC_TYPE_UAC)
    {
        voice_handle->mic_str = uac_mic_stream_init(&cfg->mic_cfg.uac_mic_cfg);
        VOICE_CHECK_NULL(voice_handle->mic_str, goto fail);
    }
#endif
#if CONFIG_ADK_ONBOARD_DUAL_DMIC_MIC_STREAM
    else if (voice_handle->mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
    {
        if(!cfg->aec_en && cfg->mic_cfg.onboard_dual_dmic_mic_cfg.dual_dmic)
        {
            cfg->mic_cfg.onboard_dual_dmic_mic_cfg.dual_dmic_sgl_out = 1;
        }
        voice_handle->mic_str = onboard_dual_dmic_mic_stream_init(&cfg->mic_cfg.onboard_dual_dmic_mic_cfg);
        VOICE_CHECK_NULL(voice_handle->mic_str, goto fail);
    }
#endif
    else
    {
        //nothing todo
        BK_LOGE(TAG, "%s, %d, mic_type: %d is not support \n", __func__, __LINE__, voice_handle->mic_type);
        goto fail;
    }

    if (voice_handle->aec_en)
    {

        #if CONFIG_ADK_AEC_V3_ALGORITHM
        bk_voice_cal_vad_buf_size(cfg, voice_handle);
        voice_handle->aec_alg = aec_v3_algorithm_init(&cfg->aec_cfg.aec_alg_cfg);
        #else
        BK_LOGE(TAG, "%s, %d,AEC V3 but CONFIG_ADK_AEC_V3_ALGORITHM is not set!\n", __func__, __LINE__);
        #endif
        
        VOICE_CHECK_NULL(voice_handle->aec_alg, goto fail);
    }

    switch (voice_handle->enc_type)
    {
        case AUDIO_ENC_TYPE_G711A:
        case AUDIO_ENC_TYPE_G711U:
            voice_handle->mic_enc = g711_encoder_init(&cfg->enc_cfg.g711_enc_cfg);
            break;

#if CONFIG_VOICE_SERVICE_AAC_ENCODER
        case AUDIO_ENC_TYPE_AAC:
            voice_handle->mic_enc = aac_encoder_init(&cfg->enc_cfg.aac_enc_cfg);
            break;
#endif

#if CONFIG_VOICE_SERVICE_G722_ENCODER
        case AUDIO_ENC_TYPE_G722:
            voice_handle->mic_enc = g722_encoder_init(&cfg->enc_cfg.g722_enc_cfg);
            break;
#endif
#if CONFIG_VOICE_SERVICE_OPUS_ENCODER
        case AUDIO_ENC_TYPE_OPUS:
            voice_handle->mic_enc = opus_enc_init(&cfg->enc_cfg.opus_enc_cfg);
            break;
#endif

        case AUDIO_ENC_TYPE_PCM:
            /* not need encoder */
            break;

        default:
            BK_LOGE(TAG, "%s, %d, enc_type: %d is not support\n", __func__, __LINE__, voice_handle->enc_type);
            goto fail;
            break;
    }
    if (voice_handle->enc_type != AUDIO_ENC_TYPE_PCM)
    {
        VOICE_CHECK_NULL(voice_handle->mic_enc, goto fail);
    }

    raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_read_cfg.type = AUDIO_STREAM_READER;
    raw_read_cfg.out_block_size = cfg->read_pool_size;
    raw_read_cfg.out_block_num = 1;
    #if CONFIG_VOICE_SERVICE_OPUS_ENCODER
    if(AUDIO_ENC_TYPE_OPUS == voice_handle->enc_type)
    {
        raw_read_cfg.output_port_type = PORT_TYPE_FB;
    }
    #endif
    voice_handle->raw_read = raw_stream_init(&raw_read_cfg);
    VOICE_CHECK_NULL(voice_handle->raw_read, goto fail);

    BK_LOGD(TAG, "step3: record pipeline register\n");
    if (BK_OK != audio_pipeline_register(voice_handle->record_pipeline, voice_handle->mic_str, "mic"))
    {
        BK_LOGE(TAG, "%s, %d, register mic_stream fail\n", __func__, __LINE__);
        goto fail;
    }

    if (voice_handle->aec_en)
    {
        if (BK_OK != audio_pipeline_register(voice_handle->record_pipeline, voice_handle->aec_alg, "aec_alg"))
        {
            BK_LOGE(TAG, "%s, %d, register aec_algorithm fail\n", __func__, __LINE__);
            goto fail;
        }
    }

    if (voice_handle->mic_enc && BK_OK != audio_pipeline_register(voice_handle->record_pipeline, voice_handle->mic_enc, "encode"))
    {
        BK_LOGE(TAG, "%s, %d, register encoder fail\n", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != audio_pipeline_register(voice_handle->record_pipeline, voice_handle->raw_read, "raw_read"))
    {
        BK_LOGE(TAG, "%s, %d, register raw_read stream fail\n", __func__, __LINE__);
        goto fail;
    }

    BK_LOGD(TAG, "step4: record pipeline link\n");
    /* pipeline record */
    if (voice_handle->aec_en)
    {
        if (voice_handle->mic_enc)
        {
            ret = audio_pipeline_link(voice_handle->record_pipeline, (const char *[])
            {"mic", "aec_alg", "encode", "raw_read"
            }, 4);
        }
        else
        {
            ret = audio_pipeline_link(voice_handle->record_pipeline, (const char *[])
            {"mic", "aec_alg", "raw_read"
            }, 3);
        }
    }
    else
    {
        if (voice_handle->mic_enc)
        {
            ret = audio_pipeline_link(voice_handle->record_pipeline, (const char *[])
            {"mic", "encode", "raw_read"
            }, 3);
        }
        else
        {
            ret = audio_pipeline_link(voice_handle->record_pipeline, (const char *[])
            {"mic", "raw_read"
            }, 2);
        }
    }

    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline link fail\n", __func__, __LINE__);
        goto fail;
    }

    if (voice_handle->event_handle)
    {
        BK_LOGD(TAG, "step5: init record event listener\n");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        voice_handle->record_evt = audio_event_iface_init(&evt_cfg);
        if (voice_handle->record_evt == NULL)
        {
            BK_LOGE(TAG, "%s, %d, record_event init fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != audio_pipeline_set_listener(voice_handle->record_pipeline, voice_handle->record_evt))
        {
            BK_LOGE(TAG, "%s, %d, init record pipeline listener fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;

fail:
    record_pipeline_deinit(voice_handle);

    return BK_FAIL;
}

static bk_err_t record_pipeline_start(audio_pipeline_handle_t record_pipeline)
{
    VOICE_CHECK_NULL(record_pipeline, return BK_FAIL);

    if (BK_OK != audio_pipeline_run(record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline run fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t record_pipeline_stop(audio_pipeline_handle_t record_pipeline)
{
    VOICE_CHECK_NULL(record_pipeline, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != audio_pipeline_stop(record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline wait stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_deinit(voice_handle_t voice_handle)
{
    BK_LOGD(TAG, "%s\n", __func__);

    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->play_pipeline)
    {
        return BK_OK;
    }

    audio_element_set_input_timeout(voice_handle->spk_dec, 0);

    if (BK_OK != audio_pipeline_terminate(voice_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline terminate fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->raw_write && BK_OK != audio_pipeline_unregister(voice_handle->play_pipeline, voice_handle->raw_write))
    {
        BK_LOGE(TAG, "%s, %d, unregister raw_write stream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->spk_dec && BK_OK != audio_pipeline_unregister(voice_handle->play_pipeline, voice_handle->spk_dec))
    {
        BK_LOGE(TAG, "%s, %d, unregister decoder fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->spk_str && BK_OK != audio_pipeline_unregister(voice_handle->play_pipeline, voice_handle->spk_str))
    {
        BK_LOGE(TAG, "%s, %d, unregister spk stream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (voice_handle->play_evt)
    {
        /* deinit listener */
        if (BK_OK != audio_pipeline_remove_listener(voice_handle->play_pipeline))
        {
            BK_LOGE(TAG, "%s, %d, pipeline terminate fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        if (BK_OK != audio_event_iface_destroy(voice_handle->play_evt))
        {
            BK_LOGE(TAG, "%s, %d, pipeline terminate fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    if (BK_OK != audio_pipeline_deinit(voice_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->play_pipeline = NULL;
    }

    if (voice_handle->raw_write && BK_OK != audio_element_deinit(voice_handle->raw_write))
    {
        BK_LOGE(TAG, "%s, %d, raw_write stream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->raw_write = NULL;
    }

    if (voice_handle->spk_dec && BK_OK != audio_element_deinit(voice_handle->spk_dec))
    {
        BK_LOGE(TAG, "%s, %d, decoder deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->spk_dec = NULL;
    }

    if (voice_handle->spk_str && BK_OK != audio_element_deinit(voice_handle->spk_str))
    {
        BK_LOGE(TAG, "%s, %d, spk stream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        voice_handle->spk_str = NULL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_init(voice_handle_t voice_handle, voice_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;

    BK_LOGD(TAG, "%s\n", __func__);
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    BK_LOGD(TAG, "step1: play pipeline init\n");
    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    play_pipeline_cfg.rb_size = 320;
    voice_handle->play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
    VOICE_CHECK_NULL(voice_handle->play_pipeline, return BK_FAIL);

    BK_LOGD(TAG, "step2: init play elements\n");
    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    raw_write_cfg.out_block_size = cfg->write_pool_size;
    raw_write_cfg.out_block_num = 1;
    #if CONFIG_VOICE_SERVICE_OPUS_DECODER
    if(AUDIO_DEC_TYPE_OPUS == voice_handle->dec_type)
    {
        raw_write_cfg.output_port_type = PORT_TYPE_FB;
    }
    #endif
    voice_handle->raw_write = raw_stream_init(&raw_write_cfg);
    VOICE_CHECK_NULL(voice_handle->raw_write, goto fail);

    switch (voice_handle->dec_type)
    {
        case AUDIO_DEC_TYPE_G711A:
        case AUDIO_DEC_TYPE_G711U:
            voice_handle->spk_dec = g711_decoder_init(&cfg->dec_cfg.g711_dec_cfg);
            break;

#if CONFIG_VOICE_SERVICE_AAC_DECODER
        case AUDIO_DEC_TYPE_AAC:
            voice_handle->spk_dec = aac_decoder_init(&cfg->dec_cfg.aac_dec_cfg);
            break;
#endif

#if CONFIG_VOICE_SERVICE_G722_DECODER
        case AUDIO_DEC_TYPE_G722:
            voice_handle->spk_dec = g722_decoder_init(&cfg->dec_cfg.g722_dec_cfg);
            break;
#endif
#if CONFIG_VOICE_SERVICE_OPUS_DECODER
        case AUDIO_DEC_TYPE_OPUS:
            voice_handle->spk_dec = opus_dec_init(&cfg->dec_cfg.opus_dec_cfg);
            break;
#endif


        case AUDIO_DEC_TYPE_PCM:
            /* not need decoder */
            break;

        default:
            BK_LOGE(TAG, "%s, %d, dec_type: %d is not support\n", __func__, __LINE__, voice_handle->dec_type);
            goto fail;
            break;
    }
    if (voice_handle->dec_type != AUDIO_DEC_TYPE_PCM)
    {
        VOICE_CHECK_NULL(voice_handle->spk_dec, goto fail);
    }

    if (voice_handle->spk_type == SPK_TYPE_ONBOARD)
    {
        voice_handle->spk_str = onboard_speaker_stream_init(&cfg->spk_cfg.onboard_spk_cfg);
    }
#if CONFIG_ADK_UAC_SPEAKER_STREAM
    else if (voice_handle->spk_type == SPK_TYPE_UAC)
    {
        voice_handle->spk_str = uac_speaker_stream_init(&cfg->spk_cfg.uac_spk_cfg);
    }
#endif
    else
    {
        //nothing todo
        BK_LOGE(TAG, "%s, %d, spk_type: %d is not support \n", __func__, __LINE__, voice_handle->spk_type);
        goto fail;
    }
    VOICE_CHECK_NULL(voice_handle->spk_str, goto fail);

    #if CONFIG_VOICE_SERVICE_EQ
    if(cfg->eq_en)
    {
        voice_handle->eq_str = eq_algorithm_init(&cfg->eq_cfg.eq_alg_cfg);
        if(!voice_handle->eq_str)
        {
            BK_LOGE(TAG, "%s, %d, register eq fail\n", __func__, __LINE__);
            goto fail;
        }
    }
    #endif

    BK_LOGD(TAG, "step3: play pipeline register\n");
    if (BK_OK != audio_pipeline_register(voice_handle->play_pipeline, voice_handle->raw_write, "raw_write"))
    {
        BK_LOGE(TAG, "%s, %d, register raw_write stream fail", __func__, __LINE__);
        goto fail;
    }

    if (voice_handle->spk_dec && BK_OK != audio_pipeline_register(voice_handle->play_pipeline, voice_handle->spk_dec, "decode"))
    {
        BK_LOGE(TAG, "%s, %d, register decoder fail", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != audio_pipeline_register(voice_handle->play_pipeline, voice_handle->spk_str, "spk"))
    {
        BK_LOGE(TAG, "%s, %d, register spk stream fail", __func__, __LINE__);
        goto fail;
    }
    
    #if CONFIG_VOICE_SERVICE_EQ
    if(cfg->eq_en)
    {
        if (BK_OK != audio_pipeline_register(voice_handle->play_pipeline, voice_handle->eq_str, "eq"))
        {
            BK_LOGE(TAG, "%s, %d, register eq fail\n", __func__, __LINE__);
            goto fail;
        }
    }
    #endif

    BK_LOGD(TAG, "step4: play pipeline link\n");
    #if CONFIG_VOICE_SERVICE_EQ
    if(cfg->eq_en)
    {
        if (voice_handle->spk_dec)
        {
            ret = audio_pipeline_link(voice_handle->play_pipeline, (const char *[])
            {"raw_write", "decode", "eq", "spk"
            }, 4);
        }
        else
        {
            ret = audio_pipeline_link(voice_handle->play_pipeline, (const char *[])
            {"raw_write",  "eq", "spk"
            }, 3);
        }
    }
    else
    #endif
    {
        if (voice_handle->spk_dec)
        {
            ret = audio_pipeline_link(voice_handle->play_pipeline, (const char *[])
            {"raw_write", "decode", "spk"
            }, 3);
        }
        else
        {
            ret = audio_pipeline_link(voice_handle->play_pipeline, (const char *[])
            {"raw_write", "spk"
            }, 2);
        }
    }
    
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline link fail\n", __func__, __LINE__);
        goto fail;
    }

    if (voice_handle->event_handle)
    {
        BK_LOGD(TAG, "step5: init play event listener\n");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        voice_handle->play_evt = audio_event_iface_init(&evt_cfg);
        if (voice_handle->play_evt == NULL)
        {
            BK_LOGE(TAG, "%s, %d, play_event init fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != audio_pipeline_set_listener(voice_handle->play_pipeline, voice_handle->play_evt))
        {
            BK_LOGE(TAG, "%s, %d, init play pipeline listener fail\n", __func__, __LINE__);
            goto fail;
        }
    }

    return BK_OK;

fail:
    play_pipeline_deinit(voice_handle);

    return BK_FAIL;
}

static bk_err_t play_pipeline_start(audio_pipeline_handle_t play_pipeline)
{
    VOICE_CHECK_NULL(play_pipeline, return BK_FAIL);

    if (BK_OK != audio_pipeline_run(play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline run fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_stop(audio_pipeline_handle_t play_pipeline)
{
    VOICE_CHECK_NULL(play_pipeline, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != audio_pipeline_stop(play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline wait stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t listener_send_msg(beken_queue_t queue, listener_op_t op, void *param)
{
    bk_err_t ret;
    listener_msg_t msg;

    if (!queue)
    {
        BK_LOGE(TAG, "%s, %d, queue: %p \n", __func__, __LINE__, queue);
        return BK_FAIL;
    }

    msg.op = op;
    msg.param = param;
    ret = rtos_push_to_queue(&queue, &msg, BEKEN_NO_WAIT);
    if (kNoErr != ret)
    {
        BK_LOGE(TAG, "%s, %d, listener send message: %d fail, ret: %d\n", __func__, __LINE__, op, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

static void listener_task_main(beken_thread_arg_t param_data)
{
    voice_handle_t voice_handle = (voice_handle_t)param_data;
    bk_err_t ret = BK_OK;

    voice_handle->listener_is_running = false;
    long unsigned int wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&voice_handle->listener_sem);

    while (1)
    {
        listener_msg_t listener_msg;
        ret = rtos_pop_from_queue(&voice_handle->listener_msg_que, &listener_msg, wait_time);
        if (kNoErr == ret)
        {
            switch (listener_msg.op)
            {
                case LISTENER_IDLE:
                    voice_handle->listener_is_running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case LISTENER_EXIT:
                    goto exit;
                    break;

                case LISTENER_START:
                    voice_handle->listener_is_running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        audio_event_iface_msg_t event_msg;
        audio_element_status_t el_status = AEL_STATUS_NONE;
        if (voice_handle->listener_is_running)
        {
            ret = audio_event_iface_listen(voice_handle->record_evt, &event_msg, 20 / portTICK_RATE_MS);//portMAX_DELAY
            if (ret == BK_OK)
            {
                if (event_msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
                {
                    el_status = (int)(uintptr_t)event_msg.data;
                    switch (el_status)
                    {
                        case AEL_STATUS_ERROR_OPEN:
                        case AEL_STATUS_ERROR_INPUT:
                        case AEL_STATUS_ERROR_PROCESS:
                        case AEL_STATUS_ERROR_OUTPUT:
                        case AEL_STATUS_ERROR_CLOSE:
                        case AEL_STATUS_ERROR_TIMEOUT:
                        case AEL_STATUS_ERROR_UNKNOWN:
                            if (voice_handle->status == VOICE_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>>record pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop voice pipeline */
                                bk_voice_stop(voice_handle);

                                if ((audio_element_handle_t)event_msg.source == voice_handle->mic_str)
                                {
                                    bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_MIC_NOT_SUPPORT, NULL, voice_handle->args);
                                }
                                else
                                {
                                    bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_ERROR_UNKNOW, NULL, voice_handle->args);
                                }

                                /* stop listener */
                                voice_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        case AEL_STATUS_STATE_STOPPED:
                        case AEL_STATUS_STATE_FINISHED:
                            if (voice_handle->status == VOICE_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>>record pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop voice pipeline */
                                bk_voice_stop(voice_handle);
                                bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_STOP, NULL, voice_handle->args);

                                /* stop listener */
                                voice_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }

            ret = audio_event_iface_listen(voice_handle->play_evt, &event_msg, 20 / portTICK_RATE_MS);//portMAX_DELAY
            if (ret == BK_OK)
            {
                if (event_msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
                {
                    el_status = (int)(uintptr_t)event_msg.data;
                    switch (el_status)
                    {
                        case AEL_STATUS_ERROR_OPEN:
                        case AEL_STATUS_ERROR_INPUT:
                        case AEL_STATUS_ERROR_PROCESS:
                        case AEL_STATUS_ERROR_OUTPUT:
                        case AEL_STATUS_ERROR_CLOSE:
                        case AEL_STATUS_ERROR_TIMEOUT:
                        case AEL_STATUS_ERROR_UNKNOWN:
                            if (voice_handle->status == VOICE_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>play pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop voice pipeline */
                                bk_voice_stop(voice_handle);

                                if ((audio_element_handle_t)event_msg.source == voice_handle->spk_str)
                                {
                                    bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_SPK_NOT_SUPPORT, NULL, voice_handle->args);
                                }
                                else
                                {
                                    bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_ERROR_UNKNOW, NULL, voice_handle->args);
                                }

                                /* stop listener */
                                voice_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        case AEL_STATUS_STATE_STOPPED:
                        case AEL_STATUS_STATE_FINISHED:
                            if (voice_handle->status == VOICE_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>play pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop voice pipeline */
                                bk_voice_stop(voice_handle);
                                bk_voice_event_handle(voice_handle->event_handle, VOC_EVT_STOP, NULL, voice_handle->args);

                                /* stop listener */
                                voice_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }
        }
    }

exit:

    if (voice_handle->listener_msg_que)
    {
        rtos_deinit_queue(&voice_handle->listener_msg_que);
        voice_handle->listener_msg_que = NULL;
    }

    /* delete task */
    voice_handle->listener_task_hdl = NULL;

    rtos_set_semaphore(&voice_handle->listener_sem);

    rtos_delete_thread(NULL);
}

static bk_err_t listener_init(voice_handle_t voice_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    ret = rtos_init_semaphore(&voice_handle->listener_sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate listener semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&voice_handle->listener_msg_que,
                          "voc_listener_que",
                          sizeof(listener_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate voice listener message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = audio_create_thread(&voice_handle->listener_task_hdl,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "voc_listener",
                             (beken_thread_function_t)listener_task_main,
                             1024,
                             (beken_thread_arg_t)voice_handle,
                             1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create voice listener task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&voice_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    BK_LOGD(TAG, "init voice listener task complete\n");

    return BK_OK;

fail:

    if (voice_handle->listener_sem)
    {
        rtos_deinit_semaphore(&voice_handle->listener_sem);
        voice_handle->listener_sem = NULL;
    }

    if (voice_handle->listener_msg_que)
    {
        rtos_deinit_queue(&voice_handle->listener_msg_que);
        voice_handle->listener_msg_que = NULL;
    }

    voice_handle->listener_task_hdl = NULL;

    return BK_FAIL;
}

static bk_err_t listener_deinit(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != listener_send_msg(voice_handle->listener_msg_que, LISTENER_EXIT, NULL))
    {
        return BK_FAIL;
    }

    rtos_get_semaphore(&voice_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&voice_handle->listener_sem);
    voice_handle->listener_sem = NULL;

    BK_LOGD(TAG, "deinit voice listener complete\n");

    return BK_OK;
}

static bk_err_t listener_start(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = listener_send_msg(voice_handle->listener_msg_que, LISTENER_START, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t listener_stop(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = listener_send_msg(voice_handle->listener_msg_que, LISTENER_IDLE, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}


static bk_err_t voice_config_check(voice_cfg_t cfg)
{
    /* check encode config */
    if (cfg.enc_type != AUDIO_ENC_TYPE_G711A
            && cfg.enc_type != AUDIO_ENC_TYPE_G711U
#if CONFIG_VOICE_SERVICE_AAC_ENCODER
            && cfg.enc_type != AUDIO_ENC_TYPE_AAC
#endif
#if CONFIG_VOICE_SERVICE_G722_ENCODER
            && cfg.enc_type != AUDIO_ENC_TYPE_G722
#endif
#if CONFIG_VOICE_SERVICE_OPUS_ENCODER
            && cfg.enc_type != AUDIO_ENC_TYPE_OPUS
#endif
            && cfg.enc_type != AUDIO_ENC_TYPE_PCM)
    {
        BK_LOGE(TAG, "%s, %d, enc_type: %d not support\n", __func__, __LINE__, cfg.enc_type);
        return BK_FAIL;
    }

    /* check decode config */
    if (cfg.dec_type != AUDIO_DEC_TYPE_G711A
            && cfg.dec_type != AUDIO_DEC_TYPE_G711U
#if CONFIG_VOICE_SERVICE_AAC_DECODER
            && cfg.dec_type != AUDIO_DEC_TYPE_AAC
#endif
#if CONFIG_VOICE_SERVICE_G722_DECODER
            && cfg.dec_type != AUDIO_DEC_TYPE_G722
#endif
#if CONFIG_VOICE_SERVICE_OPUS_DECODER
            && cfg.dec_type != AUDIO_DEC_TYPE_OPUS
#endif
            && cfg.dec_type != AUDIO_DEC_TYPE_PCM)
    {
        BK_LOGE(TAG, "%s, %d, dec_type: %d not support\n", __func__, __LINE__, cfg.dec_type);
        return BK_FAIL;
    }

    /* UAC only support AEC_MODE_SOFTWARE mode if aec enable. */
    if ((cfg.mic_type == MIC_TYPE_UAC || cfg.spk_type == SPK_TYPE_UAC) && cfg.aec_en && cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode != AEC_MODE_SOFTWARE)
    {
        BK_LOGE(TAG, "%s, %d, UAC only support AEC_MODE_SOFTWARE mode if aec enable\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* check mic type and mic sample rate config, only support 8000 or 16000, mono channel, 16bit */
    if (cfg.mic_type == MIC_TYPE_ONBOARD)
    {
        if (cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate != 8000 && cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate != 16000)
        {
            BK_LOGE(TAG, "%s, %d, onboard mic sample rate: %d not support\n", __func__, __LINE__, cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate);
            return BK_FAIL;
        }

        if (cfg.mic_cfg.onboard_mic_cfg.adc_cfg.bits != 16)
        {
            BK_LOGE(TAG, "%s, %d, onboard mic adc bits: %d is not support\n", __func__, __LINE__, cfg.mic_cfg.onboard_mic_cfg.adc_cfg.bits);
            return BK_FAIL;
        }

        /*
            When aec enable,
            if aec mode is AEC_MODE_HARDWARE, mic channel is 2 (one channel is mic data, other channel is ref data from speaker)
            if aec mode is AEC_MODE_SOFTWARE, mic channel is 1 (the channel is mic data)
         */
        if (cfg.aec_en)
        {
            if ((cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_HARDWARE && cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num != 2)
                || (cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_SOFTWARE && cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num != 1))
            {
                BK_LOGE(TAG, "%s, %d, aec mode: %d, mic chanels: %d are not match\n", __func__, __LINE__, cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode, cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num);
                return BK_FAIL;
            }
        }
    }
    else if (cfg.mic_type == MIC_TYPE_UAC)
    {
        if (cfg.mic_cfg.uac_mic_cfg.samp_rate != 8000 && cfg.mic_cfg.uac_mic_cfg.samp_rate != 16000)
        {
            BK_LOGE(TAG, "%s, %d, onboard mic sample rate: %d not support\n", __func__, __LINE__, cfg.mic_cfg.uac_mic_cfg.samp_rate);
            return BK_FAIL;
        }

        /* UAC mic only support AEC_MODE_SOFTWARE mode if aec enable. */
        if (cfg.aec_en)
        {
            if (cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode != AEC_MODE_SOFTWARE || cfg.mic_cfg.uac_mic_cfg.chl_num != 1)
            {
                BK_LOGE(TAG, "%s, %d, aec mode: %d, mic chanels: %d are not support\n", __func__, __LINE__, cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode, cfg.mic_cfg.uac_mic_cfg.chl_num);
                return BK_FAIL;
            }
        }
    }
    else if (cfg.mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
    {
        if (cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.sample_rate != 8000 && cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.sample_rate != 16000)
        {
            BK_LOGE(TAG, "%s, %d, onboard dual dmic mic sample rate: %d not support\n", __func__, __LINE__, cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.sample_rate);
            return BK_FAIL;
        }

        if (cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.bits != 16)
        {
            BK_LOGE(TAG, "%s, %d, onboard dual dmic mic adc bits: %d is not support\n", __func__, __LINE__, cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.bits);
            return BK_FAIL;
        }

        /*
            When aec enable,
            if aec mode is AEC_MODE_HARDWARE, mic channel is 2 (one channel is mic data, other channel is ref data from speaker)
            if aec mode is AEC_MODE_SOFTWARE, mic channel is 1 (the channel is mic data)
         */
        if (cfg.aec_en)
        {
            if ((cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_HARDWARE && cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.chl_num != 2)
                || (cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_SOFTWARE && cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.chl_num != 1))
            {
                BK_LOGE(TAG, "%s, %d, aec mode: %d, mic chanels: %d are not match\n", __func__, __LINE__, cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode, cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.chl_num);
                return BK_FAIL;
            }

            if ((cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_HARDWARE && !cfg.mic_cfg.onboard_dual_dmic_mic_cfg.ref_mode)
                || (cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_SOFTWARE && cfg.mic_cfg.onboard_dual_dmic_mic_cfg.ref_mode))
            {
                BK_LOGE(TAG, "%s, %d, aec mode: %d, mic ref mode: %d are not match\n", __func__, __LINE__, cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode, cfg.mic_cfg.onboard_dual_dmic_mic_cfg.ref_mode);
                return BK_FAIL;
            }

            if ((cfg.aec_cfg.aec_alg_cfg.dual_ch && !cfg.mic_cfg.onboard_dual_dmic_mic_cfg.dual_dmic)
                || (!cfg.aec_cfg.aec_alg_cfg.dual_ch && cfg.mic_cfg.onboard_dual_dmic_mic_cfg.dual_dmic))
            {
                BK_LOGE(TAG, "%s, %d, aec dual dmic: %d, mic dual dmic: %d are not match\n", __func__, __LINE__, cfg.aec_cfg.aec_alg_cfg.dual_ch, cfg.mic_cfg.onboard_dual_dmic_mic_cfg.dual_dmic);
                return BK_FAIL;
            }
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, cfg.mic_type: %d not support\n", __func__, __LINE__, cfg.mic_type);
        return BK_FAIL;
    }

    /* check spk type and spk sample rate config, only support 8000 or 16000 */
    if (cfg.spk_type == SPK_TYPE_ONBOARD)
    {
#if 0
        if (cfg.spk_cfg.onboard_spk_cfg.sample_rate != 8000 && cfg.spk_cfg.onboard_spk_cfg.sample_rate != 16000)
        {
            BK_LOGE(TAG, "%s, %d, onboard spk sample rate: %d not support\n", __func__, __LINE__, cfg.spk_cfg.onboard_spk_cfg.sample_rate);
            return BK_FAIL;
        }
#endif
        if (cfg.spk_cfg.onboard_spk_cfg.bits != 16 || cfg.spk_cfg.onboard_spk_cfg.chl_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, onboard spk dac bits: %d, chl_num: %d is not right\n", __func__, __LINE__, cfg.spk_cfg.onboard_spk_cfg.bits, cfg.spk_cfg.onboard_spk_cfg.chl_num);
            return BK_FAIL;
        }

        /* When aec enable, multi_out_rb_num is 1 (output speaker data to ring buffer save ref data of aec) */
        if (cfg.aec_en && cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode == AEC_MODE_SOFTWARE && cfg.spk_cfg.onboard_spk_cfg.multi_out_port_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, aec_en: %d, aec_mode: %d, spk multi_out_port_num: %d are not match\n", __func__, __LINE__, cfg.aec_en, cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode, cfg.spk_cfg.onboard_spk_cfg.multi_out_port_num);
            return BK_FAIL;
        }

    }
    else if (cfg.spk_type == SPK_TYPE_UAC)
    {
#if 0
        if (cfg.spk_cfg.uac_spk_cfg.samp_rate != 8000 && cfg.spk_cfg.uac_spk_cfg.samp_rate != 16000)
        {
            BK_LOGE(TAG, "%s, %d, onboard spk sample rate: %d not support\n", __func__, __LINE__, cfg.spk_cfg.uac_spk_cfg.samp_rate);
            return BK_FAIL;
        }
#endif

        if (cfg.spk_cfg.uac_spk_cfg.chl_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, uac spk chl_num: %d is not right\n", __func__, __LINE__, cfg.spk_cfg.uac_spk_cfg.chl_num);
            return BK_FAIL;
        }

        /* When aec enable, multi_out_rb_num is 1 (output speaker data to ring buffer save ref data of aec) */
        if (cfg.aec_en && cfg.spk_cfg.uac_spk_cfg.multi_out_port_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, aec_en: %d, multi_out_port_num: %d are not match\n", __func__, __LINE__, cfg.aec_en, cfg.spk_cfg.uac_spk_cfg.multi_out_port_num);
            return BK_FAIL;
        }

    }
    else
    {
        BK_LOGE(TAG, "%s, %d, cfg.spk_type: %d not support\n", __func__, __LINE__, cfg.spk_type);
        return BK_FAIL;
    }

    return BK_OK;
}

voice_handle_t bk_voice_init(voice_cfg_t *cfg)
{
    if (BK_OK != voice_config_check(*cfg))
    {
        BK_LOGE(TAG, "%s, %d, check voice config\n", __func__, __LINE__);
        return NULL;
    }

#if (CONFIG_VOICE_SERVICE_USE_PSRAM)
    voice_handle_t voice_handle = (voice_handle_t)psram_malloc(sizeof(struct voice));
#elif (CONFIG_VOICE_SERVICE_USE_AUDIO_HEAP)
    voice_handle_t voice_handle = (voice_handle_t)audio_heap_malloc(sizeof(struct voice));
#else
    voice_handle_t voice_handle = (voice_handle_t)os_malloc(sizeof(struct voice));
#endif


    VOICE_CHECK_NULL(voice_handle, return NULL);

    os_memset(voice_handle, 0, sizeof(struct voice));

    /* copy config */
    voice_handle->mic_type = cfg->mic_type;
    voice_handle->aec_en = cfg->aec_en;
    voice_handle->enc_type = cfg->enc_type;
    voice_handle->dec_type = cfg->dec_type;
    voice_handle->spk_type = cfg->spk_type;
    #if CONFIG_VOICE_SERVICE_EQ
    voice_handle->eq_en = cfg->eq_en;
    #endif
    voice_handle->event_handle = cfg->event_handle;
    voice_handle->args = cfg->args;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);


    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 0, 0);

    if (BK_OK != record_pipeline_init(voice_handle, cfg))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline_open fail\n", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != play_pipeline_init(voice_handle, cfg))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline_open fail\n", __func__, __LINE__);
        goto fail;
    }

    /* malloc ring buffer to save ref data of aec */
    if (voice_handle->aec_en)
    {
        /* frame_size = sample_rate * 20ms / 1000 * bits */
        ringbuf_port_cfg_t rb_config = {cfg->aec_cfg.aec_alg_cfg.aec_cfg.fs * 20 / 1000 * 2 * 2};
        voice_handle->aec_alg_ref_rb = ringbuf_port_init(&rb_config);
        VOICE_CHECK_NULL(voice_handle->aec_alg_ref_rb, goto fail);
        
// Modified by TUYA Start
        if(AEC_MODE_HARDWARE == cfg->aec_cfg.aec_alg_cfg.aec_cfg.mode && cfg->aec_cfg.aec_alg_cfg.dual_ch)
// Modified by TUYA End
        {
            /* link aec_alg_ref_rb to mic stream and aec algorithm */
            if (BK_OK !=  audio_element_set_multi_input_port(voice_handle->aec_alg, voice_handle->aec_alg_ref_rb, 0))
            {
                BK_LOGE(TAG, "%s, %d, link aec_alg_ref_rb to aec_alg fail\n", __func__, __LINE__);
                goto fail;
            }

            if (BK_OK !=  audio_element_set_multi_output_port(voice_handle->mic_str, voice_handle->aec_alg_ref_rb, 0))
            {
                BK_LOGE(TAG, "%s, %d, link apk_stream to aec_alg_ref_rb fail\n", __func__, __LINE__);
                goto fail;
            }
        }
        else
        {
            /* link aec_alg_ref_rb to spk stream and aec algorithm */
            if (BK_OK !=  audio_element_set_multi_input_port(voice_handle->aec_alg, voice_handle->aec_alg_ref_rb, 0))
            {
                BK_LOGE(TAG, "%s, %d, link aec_alg_ref_rb to aec_alg fail\n", __func__, __LINE__);
                goto fail;
            }

            if (BK_OK !=  audio_element_set_multi_output_port(voice_handle->spk_str, voice_handle->aec_alg_ref_rb, 0))
            {
                BK_LOGE(TAG, "%s, %d, link apk_stream to aec_alg_ref_rb fail\n", __func__, __LINE__);
                goto fail;
            }
        }
    }

    /* check whether event_handle was been register.
     * If true, init pipeline listener.
     * If false, not init pipeline listener.
     */
    if (voice_handle->event_handle && BK_OK != listener_init(voice_handle))
    {
        BK_LOGE(TAG, "%s, %d, voice listener init fail\n", __func__, __LINE__);
        goto fail;
    }

    voice_handle->status = VOICE_STA_IDLE;

    return voice_handle;

fail:

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    record_pipeline_deinit(voice_handle);

    play_pipeline_deinit(voice_handle);

    if (voice_handle->aec_alg_ref_rb)
    {
        audio_port_deinit(voice_handle->aec_alg_ref_rb);
        voice_handle->aec_alg_ref_rb = NULL;
    }

    listener_deinit(voice_handle);

#if (CONFIG_VOICE_SERVICE_USE_PSRAM)
    psram_free(voice_handle);
#elif (CONFIG_VOICE_SERVICE_USE_AUDIO_HEAP)
    audio_heap_free(voice_handle);
#else
    os_free(voice_handle);
#endif

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);

    return NULL;
}

bk_err_t bk_voice_deinit(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (voice_handle->status == VOICE_STA_RUNNING)
    {
        bk_voice_stop(voice_handle);
    }

    listener_stop(voice_handle);

    record_pipeline_deinit(voice_handle);
    BK_LOGD(TAG, "%s, record_pipeline deinit complete\n", __func__);
    play_pipeline_deinit(voice_handle);
    BK_LOGD(TAG, "%s, play_pipeline deinit complete\n", __func__);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    if (voice_handle->aec_alg_ref_rb)
    {
        audio_port_deinit(voice_handle->aec_alg_ref_rb);
        voice_handle->aec_alg_ref_rb = NULL;
    }

    listener_deinit(voice_handle);

    voice_handle->status = VOICE_STA_NONE;

#if (CONFIG_VOICE_SERVICE_USE_PSRAM)
        psram_free(voice_handle);
#elif (CONFIG_VOICE_SERVICE_USE_AUDIO_HEAP)
        audio_heap_free(voice_handle);
#else
        os_free(voice_handle);
#endif

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);

    return BK_OK;
}

bk_err_t bk_voice_start(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (voice_handle->status == VOICE_STA_RUNNING)
    {
        return BK_OK;
    }

    if (voice_handle->status != VOICE_STA_IDLE && voice_handle->status != VOICE_STA_STOPED)
    {
        BK_LOGE(TAG, "%s, %d, voice status: %d is error\n", __func__, __LINE__, voice_handle->status);
        return BK_FAIL;
    }

    listener_start(voice_handle);

    if (BK_OK != record_pipeline_start(voice_handle->record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline run fail\n", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != play_pipeline_start(voice_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline run fail\n", __func__, __LINE__);
        goto fail;
    }

    voice_handle->status = VOICE_STA_RUNNING;

    return BK_OK;

fail:

    listener_stop(voice_handle);

    record_pipeline_stop(voice_handle->record_pipeline);

    play_pipeline_stop(voice_handle->play_pipeline);

    return BK_FAIL;
}

bk_err_t bk_voice_stop(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (voice_handle->status == VOICE_STA_IDLE || voice_handle->status == VOICE_STA_STOPED)
    {
        return BK_OK;
    }

    if (voice_handle->status != VOICE_STA_RUNNING)
    {
        BK_LOGE(TAG, "%s, %d, voice status: %d is error\n", __func__, __LINE__, voice_handle->status);
        return BK_FAIL;
    }

    voice_handle->status = VOICE_STA_STOPPING;

    listener_stop(voice_handle);

    if (BK_OK != record_pipeline_stop(voice_handle->record_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, record_pipeline stop fail\n", __func__, __LINE__);
    }

    if (BK_OK != play_pipeline_stop(voice_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline stop fail\n", __func__, __LINE__);
    }

    voice_handle->status = VOICE_STA_STOPED;

    return BK_OK;
}

int bk_voice_read_mic_data(voice_handle_t voice_handle, char *buffer, uint32_t size)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
    if (!buffer || size == 0)
    {
        BK_LOGE(TAG, "%s, %d, buffer: %p, size: %d\n", __func__, __LINE__, buffer, size);
        return BK_FAIL;
    }

    return raw_stream_read(voice_handle->raw_read, buffer, size);
}

int bk_voice_write_spk_data(voice_handle_t voice_handle, char *buffer, uint32_t size)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
    if (!buffer || size == 0 || !voice_handle->raw_write)
    {
        BK_LOGE(TAG, "%s, %d, buffer: %p, size: %d, raw_write: %p\n", __func__, __LINE__, buffer, size, voice_handle->raw_write);
        return BK_FAIL;
    }

    return raw_stream_write(voice_handle->raw_write, buffer, size);
}

bk_err_t bk_voice_get_micstr(voice_handle_t voice_handle, audio_element_handle_t *mic_str)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	VOICE_CHECK_NULL(mic_str, return BK_FAIL);
	if (voice_handle->mic_str) {
		*mic_str = voice_handle->mic_str;
		return BK_OK;
	} else {
		return BK_FAIL;
	}
}
bk_err_t bk_voice_get_micstr_type(voice_handle_t voice_handle, mic_type_t *mic_type)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	if (voice_handle->mic_str) {
		*mic_type = voice_handle->mic_type;
		return BK_OK;
	} else {
		return BK_FAIL;
	}
}

bk_err_t bk_voice_get_spkstr(voice_handle_t voice_handle, audio_element_handle_t *spk_str)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	VOICE_CHECK_NULL(spk_str, return BK_FAIL);
	if (voice_handle->spk_str) {
		*spk_str = voice_handle->spk_str;
		return BK_OK;
	} else {
		return BK_FAIL;
	}
}
bk_err_t bk_voice_get_spkstr_type(voice_handle_t voice_handle, spk_type_t *spk_type)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	if (voice_handle->spk_str) {
		*spk_type = voice_handle->spk_type;
		return BK_OK;
	} else {
		return BK_FAIL;
	}
}

bk_err_t bk_voice_get_aec_alg(voice_handle_t voice_handle, audio_element_handle_t *aec_alg)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	VOICE_CHECK_NULL(aec_alg, return BK_FAIL);
	if (voice_handle->aec_en) {
		if (voice_handle->aec_alg) {
			*aec_alg = voice_handle->aec_alg;
			return BK_OK;
		} else {
			return BK_FAIL;
		}
	} else {
		return BK_FAIL;
	}
}

#if CONFIG_VOICE_SERVICE_EQ
bk_err_t bk_voice_get_eq_alg(voice_handle_t voice_handle, audio_element_handle_t *eq_alg)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	VOICE_CHECK_NULL(eq_alg, return BK_FAIL);
	if (voice_handle->eq_en) {
		if (voice_handle->eq_str) {
		*eq_alg = voice_handle->eq_str;
		return BK_OK;
		} else {
			return BK_FAIL;
		}
	} else {
		return BK_FAIL;
	}
}
#endif

void * bk_voice_get_record_pipeline(voice_handle_t voice_handle)
{
	VOICE_CHECK_NULL(voice_handle, return NULL);
	VOICE_CHECK_NULL(voice_handle->record_pipeline, return NULL);
	if (voice_handle->record_pipeline) {
		return voice_handle->record_pipeline;
	} else {
		return NULL;
	}
}

void * bk_voice_get_play_pipeline(voice_handle_t voice_handle)
{
	VOICE_CHECK_NULL(voice_handle, return NULL);
	VOICE_CHECK_NULL(voice_handle->play_pipeline, return NULL);
	if (voice_handle->play_pipeline) {
		return voice_handle->play_pipeline;
	} else {
		return NULL;
	}
}


int bk_voice_get_mic_str(voice_handle_t voice_handle, voice_cfg_t *cfg)
{
	VOICE_CHECK_NULL(voice_handle, return BK_FAIL);
	if (cfg->aec_en)
		return ((int)voice_handle->aec_alg);
	else
		return ((int)voice_handle->mic_str);
}

bk_err_t bk_voice_get_status(voice_handle_t voice_handle, voice_sta_t *status)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    *status = voice_handle->status;

    return BK_OK;
}

audio_element_handle_t bk_voice_get_spk_element(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return NULL);

    return voice_handle->spk_str;
}

/* used for amp system, not smp system */
#if (CONFIG_SOC_SMP)
bk_err_t bk_voice_event_handle(voice_event_handle event_handle, vioce_evt_t event, void *param, void *args)
{
    VOICE_CHECK_NULL(event_handle, return BK_FAIL);

    return event_handle(event, param, args);
}
#endif

#if 0
int bk_voice_abort_read_mic_data(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->raw_read)
    {
        BK_LOGE(TAG, "%s, %d, raw_read: %p\n", __func__, __LINE__, voice_handle->raw_read);
        return BK_FAIL;
    }

    return audio_element_abort_input_port(voice_handle->raw_read);
}

int bk_voice_abort_write_spk_data(voice_handle_t voice_handle)
{
    VOICE_CHECK_NULL(voice_handle, return BK_FAIL);

    if (!voice_handle->raw_write)
    {
        BK_LOGE(TAG, "%s, %d, raw_write: %p\n", __func__, __LINE__, voice_handle->raw_write);
        return BK_FAIL;
    }

    return audio_element_abort_output_port(voice_handle->raw_write);
}
#endif

static uint8_t bk_voice_get_enc_frame_ms(voice_cfg_t *cfg)
{
    return cfg->enc_common.frame_in_ms;
}

static uint32_t bk_voice_get_enc_in_frame_size(voice_cfg_t *cfg)
{
    return cfg->enc_common.frame_in_size;
}

void bk_voice_cal_vad_buf_size(voice_cfg_t *cfg, voice_handle_t voice_handle)
{
    if(cfg->aec_en && cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_enable)
    {
        if(!cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_buf_size || !cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_buf_size)
        {
            uint32_t enc_input_frame_size = bk_voice_get_enc_in_frame_size(cfg);
            uint8_t enc_frame_in_ms = bk_voice_get_enc_frame_ms(cfg);
            uint32 vad_buf_len;
            vad_buf_len = enc_input_frame_size*((cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_start_threshold + enc_frame_in_ms - 1)/enc_frame_in_ms);
            cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_buf_size = vad_buf_len;
            cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_frame_size = enc_input_frame_size;
        }
        BK_LOGD(TAG, "%s, %d, vad buf size: %d,frame size: %d\n", __func__, __LINE__, cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_buf_size, cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_frame_size);
    }
}

#if CONFIG_VOICE_SERVICE_MP3_DECODER
static uint16_t bk_voice_get_mp3_frame_sample_cnt(uint16_t sample_rate, uint8_t ch_num)
{
    uint16_t sample_cnt = MAX_NSAMP;
    switch (sample_rate) //only support layer3
    {
        case 8000: //MPEG2.5,frame sample cnt:576,72ms
        case 11025://MPEG2.5,frame sample cnt:576,52ms
        case 12000://MPEG2.5,frame sample cnt:576,48ms
        case 16000://MPEG2,frame sample cnt:576,36ms
        case 22050://MPEG2,frame sample cnt:576,26ms
        case 24000://MPEG2,frame sample cnt:576,24ms
        {
            break;
        }
        case 32000://MPEG1,frame sample cnt:1152,36ms
        case 44100://MPEG1,frame sample cnt:1152,26ms
        case 48000://MPEG1,frame sample cnt:1152,24ms
        {
            sample_cnt = MAX_NSAMP*MAX_NGRAN;
            break;
        }
        default:
        {
            BK_LOGE(TAG, "%s,%d unsupported dac sample rate:%d\n",__func__, __LINE__,dac_sample_rate);
            break;
        }
    }

    return (sample_cnt*ch_num);
}
#endif

