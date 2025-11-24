
#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>

#if CONFIG_VOICE_SERVICE
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#endif

#if (CONFIG_ASR_SERVICE)
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>
#endif

#define TAG "AUDIO_PARAM"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define AUDIO_PARAM_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            LOGE("%s, %d, AUDIO_PARAM_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)
#define AUDIO_PARAM_CHECK_TYPE(type, act) do {\
        if (type > (AUD_SERVICE_MAX-1)) {\
            LOGE("%s, %d, service_type %d is invalid \n", __func__, __LINE__, service_type);\
            {act;};\
        }\
    } while(0)

typedef struct
{
	void * service;
	app_aud_service_type_t service_type;
}aud_service_param_ctrl_t;

aud_service_param_ctrl_t aud_service_param_ctrl[AUD_SERVICE_MAX];

void bk_app_aud_get_service_handle(void * service, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(service, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    switch (service_type) {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE
            aud_service_param_ctrl[service_type].service = (voice_handle_t)service;
            aud_service_param_ctrl[service_type].service_type = service_type;
        #endif
            break;
        case AUD_SERVICE_ASR:
        #if (CONFIG_ASR_SERVICE)
            aud_service_param_ctrl[service_type].service = (asr_handle_t)service;
            aud_service_param_ctrl[service_type].service_type = service_type;
        #endif
            break;
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_SPK:
        case AUD_SERVICE_SINGLE_MIC:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;
    }
 }

void bk_app_aud_set_service_off(app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    LOGD("%s, service_type:%d", __func__, service_type);
    switch (service_type) {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE
            aud_service_param_ctrl[service_type].service = NULL;
            aud_service_param_ctrl[service_type].service_type = AUD_SERVICE_MAX;
        #endif
            break;
        case AUD_SERVICE_ASR:
        #if (CONFIG_ASR_SERVICE)
            aud_service_param_ctrl[service_type].service = NULL;
            aud_service_param_ctrl[service_type].service_type = AUD_SERVICE_MAX;
        #endif
            break;
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_SPK:
        case AUD_SERVICE_SINGLE_MIC:
            LOGW("The service is not supported now! service_type:%d\n", service_type); 
            break;
        default:
            break;
    }
 }

 void bk_app_update_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(sys_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;

                audio_element_handle_t mic_str = NULL;
                mic_type_t mic_type = MIC_TYPE_INVALID;

                bk_voice_get_micstr(voice_handle, &mic_str);
                bk_voice_get_micstr_type(voice_handle, &mic_type);

                if (mic_str && mic_type == MIC_TYPE_ONBOARD)
                {
                    onboard_mic_stream_set_digital_gain(mic_str, sys_config->mic0_digital_gain);
                    onboard_mic_stream_set_analog_gain(mic_str, sys_config->mic0_analog_gain);
                } else if (mic_str && mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
                {
                    ;
                }

                audio_element_handle_t spk_str = NULL;
                spk_type_t spk_type = SPK_TYPE_INVALID;

                bk_voice_get_spkstr(voice_handle, &spk_str);
                bk_voice_get_spkstr_type(voice_handle, &spk_type);

                if (spk_str && spk_type == SPK_TYPE_ONBOARD)
                {
                    onboard_speaker_stream_set_digital_gain(spk_str, sys_config->speaker_chan0_digital_gain);
                    onboard_speaker_stream_set_analog_gain(spk_str, sys_config->speaker_chan0_analog_gain);
                }
            }
            #endif
            break;
        case AUD_SERVICE_ASR:
            #if (CONFIG_ASR_SERVICE)
             if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_ASR)
             {
                 asr_handle_t asr_handle = (asr_handle_t)aud_service_param_ctrl[service_type].service;
                 if (asr_handle->mic_str && asr_handle->mic_type == MIC_TYPE_ONBOARD)
                 {
                     onboard_mic_stream_set_digital_gain(asr_handle->mic_str, sys_config->mic0_digital_gain);
                     onboard_mic_stream_set_analog_gain(asr_handle->mic_str, sys_config->mic0_analog_gain);
                 }
             }
            #endif
            break; 
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;

    }
}

void bk_app_load_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(sys_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
         #if CONFIG_VOICE_SERVICE
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;

                audio_element_handle_t mic_str = NULL;
                mic_type_t mic_type = MIC_TYPE_INVALID;

                bk_voice_get_micstr(voice_handle, &mic_str);
                bk_voice_get_micstr_type(voice_handle, &mic_type);

                if (mic_str && mic_type == MIC_TYPE_ONBOARD)
                {
                    onboard_mic_stream_get_digital_gain(mic_str, &sys_config->mic0_digital_gain);
                    onboard_mic_stream_get_analog_gain(mic_str, &sys_config->mic0_analog_gain);
                } else if (mic_str && mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
                {
                    ;
                }

                audio_element_handle_t spk_str = NULL;
                spk_type_t spk_type = SPK_TYPE_INVALID;

                bk_voice_get_spkstr(voice_handle, &spk_str);
                bk_voice_get_spkstr_type(voice_handle, &spk_type);

                if (spk_str && spk_type == SPK_TYPE_ONBOARD)
                {
                    onboard_speaker_stream_get_digital_gain(spk_str, &sys_config->speaker_chan0_digital_gain);
                    onboard_speaker_stream_get_analog_gain(spk_str, &sys_config->speaker_chan0_analog_gain);
                }
            }
            #endif
            break;
        case AUD_SERVICE_ASR:
            #if (CONFIG_ASR_SERVICE)
                if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_ASR)
                {
                    asr_handle_t asr_handle = (asr_handle_t)aud_service_param_ctrl[service_type].service;
                    if (asr_handle->mic_str && asr_handle->mic_type == MIC_TYPE_ONBOARD)
                    {
                        onboard_mic_stream_get_digital_gain(asr_handle->mic_str, &sys_config->mic0_digital_gain);
                        onboard_mic_stream_get_analog_gain(asr_handle->mic_str, &sys_config->mic0_analog_gain);
                    }
                }
            #endif
            break;
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            break;
        default:
            break;
    }
}

void bk_app_update_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(aec_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    LOGD("[+]%s, ec_depth:%d\n", __func__, aec_config->ec_depth);
    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_ADK_AEC_V3_ALGORITHM
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                audio_element_handle_t aec_alg = NULL;
                bk_voice_get_aec_alg(voice_handle, &aec_alg);
                if (aec_alg)
                {
                    aec_v3_algorithm_set_config(aec_alg, (void *)aec_config);
                }
            }
        #endif
        break;
        case AUD_SERVICE_ASR:
            //break;
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;
    }
}

void bk_app_load_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(aec_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    LOGD("[+]%s, ec_depth:%d\n", __func__, aec_config->ec_depth);

    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_ADK_AEC_V3_ALGORITHM
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                audio_element_handle_t aec_alg = NULL;
                bk_voice_get_aec_alg(voice_handle, &aec_alg);
                if (aec_alg)
                {
                    aec_v3_algorithm_get_config(aec_alg, (void *)aec_config);
                }
            }
        #endif
        break;
        case AUD_SERVICE_ASR:
            //break;
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;
    }
}

void bk_app_update_aud_eq_config(app_aud_eq_config_t *eq_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(eq_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_VOICE_SERVICE_EQ
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                audio_element_handle_t eq_alg = NULL;
                bk_voice_get_eq_alg(voice_handle, &eq_alg);
                if (eq_alg)
                {
                    eq_algorithm_set_config(eq_alg, (void *)eq_config);
                }
            }
        #endif
            break;
        case AUD_SERVICE_ASR:
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;
    }
}

void bk_app_load_aud_eq_config(app_eq_load_t *eq_load, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(eq_load, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    switch(service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_VOICE_SERVICE_EQ
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_DOORBELL_VOC)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                audio_element_handle_t eq_alg = NULL;
                bk_voice_get_eq_alg(voice_handle, &eq_alg);
                if (eq_alg)
                {
                    eq_algorithm_get_config(eq_alg, (void *)eq_load);
                }
            }
        #endif
            break;
        case AUD_SERVICE_ASR:
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_SINGLE_MIC:
        case AUD_SERVICE_SINGLE_SPK:
            LOGW("The service is not supported now! service_type:%d\n", service_type);
            break;
        default:
            break;
    }
}
