#ifndef __AUDIO_PARAM_CONTROL_H__
#define __AUDIO_PARAM_CONTROL_H__

#include <stdint.h>

#define EQ_ID_DL_VOICE     0
#define EQ_ID_UL_VOICE     1
#define EQ_ID_AUDIO        2

typedef struct _app_eq_para_t
{
    int32_t a[2];
    int32_t b[3];
}app_eq_para_t;

typedef struct _app_eq_load_para_t
{
	uint32_t freq;
	uint32_t gain;
	uint32_t q_val;
	uint8_t  type;
	uint8_t  enable;
}app_eq_load_para_t;

typedef struct _app_eq_load_t
{
	uint32_t f_gain;
	uint32_t samplerate;
	app_eq_load_para_t eq_load_para[16];
}app_eq_load_t;

typedef struct _app_aud_eq_config_t
{
	uint8_t app_eq_en;
	uint8_t eq_en;
	uint32_t framecnt;
	uint32_t filters;
	int32_t globle_gain;
	app_eq_para_t eq_para[16];
	app_eq_load_t eq_load;
}app_aud_eq_config_t;


typedef struct _app_aud_sys_config_t
{
	uint8_t app_sys_en;
	uint8_t mic0_digital_gain;
	uint8_t mic0_analog_gain;
	uint8_t mic1_digital_gain;
	uint8_t mic1_analog_gain;

	uint8_t speaker_chan0_digital_gain;
	uint8_t speaker_chan0_analog_gain;
	uint8_t speaker_chan1_digital_gain;
	uint8_t speaker_chan1_analog_gain;

	uint8_t dmic_enable; // digital mic
	uint8_t main_mic_select;
	uint8_t adc_sample_rate;
	uint8_t dac_sample_rate;

	uint8_t mic_mode;  //signed_end/diffen
	uint8_t spk_mode;  //signed_end/diffen
	uint8_t mic_vbias; //0b00=2.4v, 0b11=1.8v

	uint8_t extend[5];
}app_aud_sys_config_t;

typedef struct _app_aud_sys_mic_config_t
{
	uint8_t app_sys_mic_en;
	uint8_t mic0_digital_gain;
	uint8_t mic0_analog_gain;
	uint8_t mic1_digital_gain;
	uint8_t mic1_analog_gain;

	uint8_t dmic_enable; // digital mic
	uint8_t main_mic_select;

	uint8_t mic_mode;  //signed_end/diffen
	uint8_t mic_vbias; //0b00=2.4v, 0b11=1.8v
}app_aud_sys_mic_config_t;

typedef struct _app_aud_sys_spk_config_t
{
	uint8_t app_sys_spk_en;
	uint8_t speaker_chan0_digital_gain;
	uint8_t speaker_chan0_analog_gain;
//	uint8_t speaker_chan1_digital_gain;
//	uint8_t speaker_chan1_analog_gain;
	uint8_t spk_mode;  //signed_end/diffen
}app_aud_sys_spk_config_t;

//typedef enum
//{
////	NS_CLOSE = 0,
////	NS_AI    = 1,
////	NS_TRADITION = 2,
////	NS_MODE_MAX,
//}AUD_NS_TYPE;

typedef struct _app_aud_aec_v3_config_t
{
	uint8_t app_aec_en;
	uint8_t aec_enable;
	uint16_t init_flags;

	uint8_t ec_filter;
	uint8_t ns_filter;
	int8_t  ref_scale;
	uint8_t drc_gain;
	uint8_t voice_vol;

	uint32_t ec_depth;
	uint32_t mic_delay;

	uint8_t ns_level;
	uint8_t ns_para;
	uint8_t ns_type;

	uint8_t vad_enable;
	int16_t vad_start_threshold;
	int16_t vad_stop_threshold;
	int16_t vad_silence_threshold;
	int16_t vad_eng_threshold;

	uint8_t dual_mic_enable;
	uint8_t dual_mic_distance;
	uint8_t rsvd[2];
}app_aud_aec_v3_config_t;

typedef enum
{
	AUD_SERVICE_DOORBELL_VOC = 0,
	AUD_SERVICE_ASR          = 1,
	AUD_SERVICE_AI_VOC       = 2,
	AUD_SERVICE_SINGLE_MIC   = 3,
	AUD_SERVICE_SINGLE_SPK   = 4,
	AUD_SERVICE_MAX,
}app_aud_service_type_t;

typedef struct _app_aud_para_t
{
	app_aud_service_type_t service_type;
	void * service_handle;

	app_aud_sys_config_t sys_config;
	app_aud_sys_mic_config_t sys_mic_config;
	app_aud_sys_spk_config_t sys_spk_config;

	app_aud_eq_config_t eq_dl_config;
	app_aud_eq_config_t eq_ul_config;

	app_aud_aec_v3_config_t aec_v3_config;
}app_aud_para_t;

void bk_aud_debug_get_audpara(app_aud_para_t * aud_para_ptr, app_aud_service_type_t service_type);

void bk_aud_debug_set_service_type(app_aud_service_type_t service_type);

void bk_app_aud_get_service_handle(void * service, app_aud_service_type_t service_type);

void bk_app_aud_set_service_off(app_aud_service_type_t service_type);

void bk_app_update_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type);

void bk_app_load_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type);

void bk_app_update_aud_eq_config(app_aud_eq_config_t *eq_config, app_aud_service_type_t service_type);

void bk_app_load_aud_eq_config(app_eq_load_t *eq_load, app_aud_service_type_t service_type);

void bk_app_update_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type);

void bk_app_load_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type);

#endif /* __AUDIO_PARAM_CONTROL_H__ */

