#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/uart.h>

#include <components/system.h>
#include <components/audio_param_ctrl.h>

#include "bk_uart.h"

extern void bk_set_printf_sync(uint8_t enable);
extern int bk_get_printf_sync(void);

#define APP_AUD_PARAS_TX_TMP_LEN   (0x20)

#define APP_SYS_PARA_TX_TOTALLEN  (0xF)
#define APP_SYS_PARA_TX_HEADERLEN (0x4)
#define APP_SYS_PARA_RX_DATALEN   (0x9)

#define APP_AEC_PARA_TX_TOTALLEN  (0x1E)
#define APP_AEC_PARA_TX_HEADERLEN (0x4)
#define APP_AEC_PARA_RX_DATALEN   (0x18)

#define APP_LOAD_EQ_PARAMS  (0xF0)
#define APP_LOAD_SYS_PARAMS (0xF1)
#define APP_LOAD_AEC_PARAMS (0xF2)

static app_aud_eq_config_t *eq_dbg_eq_para = NULL;
static app_aud_sys_config_t *sys_dbg_sys_para = NULL;
static app_aud_aec_v3_config_t *aec_dbg_aec_v3_para = NULL;

static app_aud_para_t * p_aud_para[AUD_SERVICE_MAX] = {NULL};
static app_aud_service_type_t g_service_type = AUD_SERVICE_MAX;

void bk_aud_debug_set_service_type(app_aud_service_type_t service_type)
{
	g_service_type = service_type;
}

void bk_aud_debug_get_audpara(app_aud_para_t * aud_para_ptr, app_aud_service_type_t service_type)
{
	p_aud_para[service_type] = aud_para_ptr;
}

static void app_load_eq_params(void)
{
//	int32_t log_level = bk_get_printf_sync();
//	bk_set_printf_sync(1);

//	app_aud_eq_config_t eq_para = {0};
	app_eq_load_t eq_load = {0};
	if (p_aud_para[g_service_type]->eq_dl_config.app_eq_en) {
		os_memcpy(&eq_load, &p_aud_para[g_service_type]->eq_dl_config.eq_load, sizeof(app_eq_load_t));
	} else
	{
		bk_app_load_aud_eq_config(&eq_load, g_service_type);
	}

	{
		uint32_t tx_len = 12;
		uint8_t __maybe_unused tmp[APP_AUD_PARAS_TX_TMP_LEN] = {0};
		tmp[0] = 0x01;
		tmp[1] = 0xe0;
		tmp[2] = 0xfc;
		tmp[3] = tx_len - 4;
		tmp[4] = 0xb2;
		tmp[5] = 0xfe;
		tmp[6]  = (eq_load.f_gain)&0xFF;
		tmp[7]  = (eq_load.f_gain>>8)&0xFF;
		tmp[8]  = (eq_load.f_gain>>16)&0xFF;
		tmp[9]  = (eq_load.f_gain>>24)&0xFF;
		tmp[10] = (eq_load.samplerate)&0xFF;
		tmp[11] = (eq_load.samplerate>>8)&0xFF;

		for(uint32_t i = 0; i <(tx_len); i++) {
			BK_LOG_RAW("%02x", tmp[i]);
		}
		BK_LOG_RAW("\n");
	}

	{
		uint32_t tx_len = 0x15;
		uint8_t __maybe_unused tmp[APP_AUD_PARAS_TX_TMP_LEN] = {0};
		tmp[0] = 0x01;
		tmp[1] = 0xe0;
		tmp[2] = 0xfc;
		tmp[3] = tx_len - 4;
		tmp[4] = 0xb2;
		tmp[5] = 0xfa;

		for (uint8_t index = 0; index < 15; index++)
		{
			tmp[6] = index;
			tmp[7] = eq_load.eq_load_para[index].enable;

			tmp[8]  = (eq_load.eq_load_para[index].freq>>24)&0xFF;
			tmp[9]  = (eq_load.eq_load_para[index].freq>>16)&0xFF;
			tmp[10] = (eq_load.eq_load_para[index].freq>>8)&0xFF;
			tmp[11] = (eq_load.eq_load_para[index].freq>>0)&0xFF;

			tmp[12] = (eq_load.eq_load_para[index].gain>>24)&0xFF;
			tmp[13] = (eq_load.eq_load_para[index].gain>>16)&0xFF;
			tmp[14] = (eq_load.eq_load_para[index].gain>>8)&0xFF;
			tmp[15] = (eq_load.eq_load_para[index].gain>>0)&0xFF;

			tmp[16] = (eq_load.eq_load_para[index].q_val>>24)&0xFF;
			tmp[17] = (eq_load.eq_load_para[index].q_val>>16)&0xFF;
			tmp[18] = (eq_load.eq_load_para[index].q_val>>8)&0xFF;
			tmp[19] = (eq_load.eq_load_para[index].q_val>>0)&0xFF;

			tmp[20] = eq_load.eq_load_para[index].type;

			rtos_delay_milliseconds(10);

			for(uint32_t i = 0; i <(tx_len); i++) {
				BK_LOG_RAW("%02x", tmp[i]);
			}
			BK_LOG_RAW("\n");

		}
	}
//	bk_set_printf_sync(log_level);
}

static void app_load_sys_params(void)
{
//	int32_t log_level = bk_get_printf_sync();
//	bk_set_printf_sync(1);

	app_aud_sys_config_t sys_config = {0};
//	os_printf("[+]%s, app_sys_en:%d\n", __func__, p_aud_para[g_service_type]->sys_config.app_sys_en);
	if (p_aud_para[g_service_type]->sys_config.app_sys_en) {
		os_memcpy(&sys_config, &p_aud_para[g_service_type]->sys_config, sizeof(app_aud_sys_config_t));
	} else
	{
		bk_app_load_aud_sys_config(&sys_config, g_service_type);
	}
	uint32_t tx_len = APP_SYS_PARA_TX_TOTALLEN;
	uint8_t tmp[APP_AUD_PARAS_TX_TMP_LEN] = {0};
	tmp[0] = 0x01; tmp[1] = 0xe0; tmp[2] = 0xfc;
	tmp[3] = APP_SYS_PARA_TX_TOTALLEN - APP_SYS_PARA_TX_HEADERLEN;
	tmp[4] = 0xb3; tmp[5] = 0xf9;
	tmp[6]  = sys_config.mic0_digital_gain;
	tmp[7]  = sys_config.mic0_analog_gain;
	tmp[8]  = sys_config.mic1_analog_gain;
	tmp[9]  = sys_config.speaker_chan0_digital_gain;
	tmp[10] = sys_config.speaker_chan0_analog_gain;
	tmp[11] = sys_config.main_mic_select;
	tmp[12] = sys_config.mic_mode;
	tmp[13] = sys_config.spk_mode;
	tmp[14] = sys_config.mic_vbias;

	for(uint32_t i = 0; i <(tx_len); i++) {
		BK_LOG_RAW("%02x", tmp[i]);
	}
	BK_LOG_RAW("\n");

//	bk_set_printf_sync(log_level);
}

static void app_load_aec_v3_params(void)
{
//	int32_t log_level = bk_get_printf_sync();
//	bk_set_printf_sync(1);

	app_aud_aec_v3_config_t aec_v3_config = {0};
	if (p_aud_para[g_service_type]->aec_v3_config.app_aec_en) {
		os_memcpy(&aec_v3_config, &p_aud_para[g_service_type]->aec_v3_config, sizeof(app_aud_aec_v3_config_t));
	} else
	{
		bk_app_load_aud_aec_v3_config(&aec_v3_config, g_service_type);
	}

	uint32_t tx_len = APP_AEC_PARA_TX_TOTALLEN;
	uint8_t tmp[APP_AUD_PARAS_TX_TMP_LEN] = {0};
	tmp[0] = 0x01; tmp[1] = 0xe0; tmp[2] = 0xfc;
	tmp[3] = APP_AEC_PARA_TX_TOTALLEN - APP_AEC_PARA_TX_HEADERLEN;
	tmp[4] = 0xb4; tmp[5] = 0xff;
	tmp[6]  = aec_v3_config.aec_enable;
	tmp[7]  = aec_v3_config.ec_filter;
	tmp[8]  = (aec_v3_config.init_flags>>8)&0xFF;
	tmp[9]  = (aec_v3_config.init_flags)&0xFF;
	tmp[10] = aec_v3_config.ns_filter;
	tmp[11] = aec_v3_config.ref_scale;
	tmp[12] = aec_v3_config.drc_gain;
	tmp[13] = aec_v3_config.voice_vol;
	tmp[14] = aec_v3_config.ec_depth;
	tmp[15] = aec_v3_config.mic_delay;
	tmp[16] = aec_v3_config.ns_level;
	tmp[17] = aec_v3_config.ns_para;
	tmp[18] = aec_v3_config.ns_type;
	tmp[19] = aec_v3_config.vad_enable;
	tmp[20] = (aec_v3_config.vad_start_threshold>>8)&0xFF;
	tmp[21] = (aec_v3_config.vad_start_threshold)&0xFF;
	tmp[22] = (aec_v3_config.vad_stop_threshold>>8)&0xFF;
	tmp[23] = (aec_v3_config.vad_stop_threshold)&0xFF;
	tmp[24] = (aec_v3_config.vad_silence_threshold>>8)&0xFF;
	tmp[25] = (aec_v3_config.vad_silence_threshold)&0xFF;
	tmp[26] = (aec_v3_config.vad_eng_threshold>>8)&0xFF;
	tmp[27] = (aec_v3_config.vad_eng_threshold)&0xFF;
	tmp[28] = aec_v3_config.dual_mic_enable;
	tmp[29] = (aec_v3_config.dual_mic_distance);

	for(uint32_t i = 0; i <(tx_len); i++) {
		BK_LOG_RAW("%02x", tmp[i]);
	}
	BK_LOG_RAW("\n");
//	bk_set_printf_sync(log_level);
}

void app_dbg_audparam(uint8_t * params, int len)
{
//	g_service_type = AUD_SERVICE_DOORBELL_VOC;//AUD_SERVICE_ASR;
//	BK_LOGD(NULL, "%s, g_service_type:%d\n", __func__, g_service_type);
	if (p_aud_para[g_service_type] == NULL || params == NULL)
	{
		BK_LOGE(NULL, "%s input params is NULL\r\n", __func__);
		return;
	}
	if (len < 1)
	{
		BK_LOGE(NULL, "%s input params len is %d, less than 1\r\n", __func__, len);
		return;
	}
//	bk_aud_debug_set_service_type(params[1]); //TBD
	switch(params[0])
	{
		case APP_LOAD_EQ_PARAMS:
			app_load_eq_params();
			break;
		case APP_LOAD_SYS_PARAMS:
			app_load_sys_params();
			break;
		case APP_LOAD_AEC_PARAMS:
			app_load_aec_v3_params();
			break;
		case 0xF9:
		{
			if(sys_dbg_sys_para == NULL)
				sys_dbg_sys_para = os_malloc(sizeof(app_aud_sys_config_t));

			if(sys_dbg_sys_para)
			{
				sys_dbg_sys_para->mic0_digital_gain 		 = params[1];
				sys_dbg_sys_para->mic0_analog_gain			 = params[2];
				sys_dbg_sys_para->mic1_analog_gain			 = params[3];
				sys_dbg_sys_para->speaker_chan0_digital_gain = params[4];
				sys_dbg_sys_para->speaker_chan0_analog_gain  = params[5];
				sys_dbg_sys_para->main_mic_select			 = params[6];
				sys_dbg_sys_para->mic_mode					 = params[7];
				sys_dbg_sys_para->spk_mode					 = params[8];
				sys_dbg_sys_para->mic_vbias 				 = params[9];

				p_aud_para[g_service_type]->sys_config.mic0_digital_gain		   = sys_dbg_sys_para->mic0_digital_gain;
				p_aud_para[g_service_type]->sys_config.mic0_analog_gain			   = sys_dbg_sys_para->mic0_analog_gain;
				p_aud_para[g_service_type]->sys_config.mic1_analog_gain			   = sys_dbg_sys_para->mic1_analog_gain;
				p_aud_para[g_service_type]->sys_config.speaker_chan0_digital_gain  = sys_dbg_sys_para->speaker_chan0_digital_gain;
				p_aud_para[g_service_type]->sys_config.speaker_chan1_analog_gain   = sys_dbg_sys_para->speaker_chan0_analog_gain;
				p_aud_para[g_service_type]->sys_config.main_mic_select			   = sys_dbg_sys_para->main_mic_select;
				p_aud_para[g_service_type]->sys_config.mic_mode					   = sys_dbg_sys_para->mic_mode;
				p_aud_para[g_service_type]->sys_config.spk_mode					   = sys_dbg_sys_para->spk_mode;
				p_aud_para[g_service_type]->sys_config.mic_vbias				   = sys_dbg_sys_para->mic_vbias;

				BK_LOG_RAW("rcv sys_params: ");
				for(uint32_t i = 1; i <(APP_SYS_PARA_RX_DATALEN+1); i++)
				{
					BK_LOG_RAW("%02x ", params[i]);
				}
				BK_LOG_RAW("\n");

				bk_app_update_aud_sys_config(sys_dbg_sys_para, g_service_type);
				
				if(sys_dbg_sys_para)
				{
					os_free(sys_dbg_sys_para);
					sys_dbg_sys_para = NULL;
				}
			}
			else
			{
				BK_LOGW(NULL, "sys_dbg_sys_para is NULL\r\n");
			}
		}
			break;
		case 0xFF:
		{
			if(aec_dbg_aec_v3_para == NULL)
				aec_dbg_aec_v3_para = os_malloc(sizeof(app_aud_aec_v3_config_t));

			if(aec_dbg_aec_v3_para)
			{
				aec_dbg_aec_v3_para->aec_enable = params[1];
				aec_dbg_aec_v3_para->ec_filter  = params[2];
				aec_dbg_aec_v3_para->init_flags = (params[3]<<8)|(params[4]);
				aec_dbg_aec_v3_para->ns_filter  = params[5];
				aec_dbg_aec_v3_para->ref_scale  = params[6];
				aec_dbg_aec_v3_para->drc_gain   = params[7];
				aec_dbg_aec_v3_para->voice_vol  = params[8];
				aec_dbg_aec_v3_para->ec_depth   = params[9];
				aec_dbg_aec_v3_para->mic_delay  = params[10];
				aec_dbg_aec_v3_para->ns_level   = params[11];
				aec_dbg_aec_v3_para->ns_para    = params[12];
				aec_dbg_aec_v3_para->ns_type    = params[13];
				aec_dbg_aec_v3_para->vad_enable = params[14];
				aec_dbg_aec_v3_para->vad_start_threshold   = (params[15]<<8)|(params[16]);
				aec_dbg_aec_v3_para->vad_stop_threshold    = (params[17]<<8)|(params[18]);
				aec_dbg_aec_v3_para->vad_silence_threshold = (params[19]<<8)|(params[20]);
				aec_dbg_aec_v3_para->vad_eng_threshold     = (params[21]<<8)|(params[22]);
				aec_dbg_aec_v3_para->dual_mic_enable   = params[23];
				aec_dbg_aec_v3_para->dual_mic_distance = params[24];

				p_aud_para[g_service_type]->aec_v3_config.aec_enable            = aec_dbg_aec_v3_para->aec_enable;
				p_aud_para[g_service_type]->aec_v3_config.ec_filter             = aec_dbg_aec_v3_para->ec_filter;
				p_aud_para[g_service_type]->aec_v3_config.init_flags            = aec_dbg_aec_v3_para->init_flags;
				p_aud_para[g_service_type]->aec_v3_config.ns_filter             = aec_dbg_aec_v3_para->ns_filter;
				p_aud_para[g_service_type]->aec_v3_config.ref_scale             = aec_dbg_aec_v3_para->ref_scale;
				p_aud_para[g_service_type]->aec_v3_config.drc_gain              = aec_dbg_aec_v3_para->drc_gain;
				p_aud_para[g_service_type]->aec_v3_config.voice_vol             = aec_dbg_aec_v3_para->voice_vol;
				p_aud_para[g_service_type]->aec_v3_config.ec_depth              = aec_dbg_aec_v3_para->ec_depth;
				p_aud_para[g_service_type]->aec_v3_config.mic_delay             = aec_dbg_aec_v3_para->mic_delay;
				p_aud_para[g_service_type]->aec_v3_config.ns_level              = aec_dbg_aec_v3_para->ns_level;
				p_aud_para[g_service_type]->aec_v3_config.ns_para               = aec_dbg_aec_v3_para->ns_para;
				p_aud_para[g_service_type]->aec_v3_config.ns_type               = aec_dbg_aec_v3_para->ns_type;
				p_aud_para[g_service_type]->aec_v3_config.vad_enable            = aec_dbg_aec_v3_para->vad_enable;
				p_aud_para[g_service_type]->aec_v3_config.vad_start_threshold   = aec_dbg_aec_v3_para->vad_start_threshold;
				p_aud_para[g_service_type]->aec_v3_config.vad_stop_threshold    = aec_dbg_aec_v3_para->vad_stop_threshold;
				p_aud_para[g_service_type]->aec_v3_config.vad_silence_threshold = aec_dbg_aec_v3_para->vad_silence_threshold;
				p_aud_para[g_service_type]->aec_v3_config.vad_eng_threshold     = aec_dbg_aec_v3_para->vad_eng_threshold;
				p_aud_para[g_service_type]->aec_v3_config.dual_mic_enable       = aec_dbg_aec_v3_para->dual_mic_enable;
				p_aud_para[g_service_type]->aec_v3_config.dual_mic_distance     = aec_dbg_aec_v3_para->dual_mic_distance;

				BK_LOG_RAW("rcv aec_params: ");
				for(uint32_t i = 1; i < (APP_AEC_PARA_RX_DATALEN+1); i++)
				{
					BK_LOG_RAW("%02x ", params[i]);
				}
				BK_LOG_RAW("\n");

				bk_app_update_aud_aec_v3_config(aec_dbg_aec_v3_para, g_service_type);

				if(aec_dbg_aec_v3_para)
				{
					os_free(aec_dbg_aec_v3_para);
					aec_dbg_aec_v3_para = NULL;
				}
			}
			else
			{
				BK_LOGD(NULL, "aec_dbg_aec_v3_para is NULL\r\n");
			}
		}
			break;
		case 0xFA:
		{
			uint8_t index   = params[1];
			uint16_t enable = params[2];
			if(eq_dbg_eq_para)
			{
				eq_dbg_eq_para->eq_para[index].a[0] = (params[3] | (params[4] << 8) | (params[5] << 16) | (params[6] << 24));
				eq_dbg_eq_para->eq_para[index].a[1] = (params[7] | (params[8] << 8) | (params[9] << 16) | (params[10] << 24));
				eq_dbg_eq_para->eq_para[index].b[0] = (params[11] | (params[12] << 8) | (params[13] << 16) | (params[14] << 24));
				eq_dbg_eq_para->eq_para[index].b[1] = (params[15] | (params[16] << 8) | (params[17] << 16) | (params[18] << 24));
				eq_dbg_eq_para->eq_para[index].b[2] = (params[19] | (params[20] << 8) | (params[21] << 16) | (params[22] << 24)); 

				eq_dbg_eq_para->eq_load.eq_load_para[index].freq   = (params[23]<<24)|(params[24]<<16)|(params[25]<<8)|(params[26]);
				eq_dbg_eq_para->eq_load.eq_load_para[index].gain   = (params[27]<<24)|(params[28]<<16)|(params[29]<<8)|(params[30]);
				eq_dbg_eq_para->eq_load.eq_load_para[index].q_val  = (params[31]<<24)|(params[32]<<16)|(params[33]<<8)|(params[34]);
				eq_dbg_eq_para->eq_load.eq_load_para[index].type   = params[35];
				eq_dbg_eq_para->eq_load.eq_load_para[index].enable = params[2];

				if(enable)
				{
					eq_dbg_eq_para->filters++;
				}
				eq_dbg_eq_para->eq_en = eq_dbg_eq_para->filters > 0 ? 1 : 0;

				p_aud_para[g_service_type]->eq_dl_config.eq_para[index].a[0]   = eq_dbg_eq_para->eq_para[index].a[0];
				p_aud_para[g_service_type]->eq_dl_config.eq_para[index].a[1]   = eq_dbg_eq_para->eq_para[index].a[1];
				p_aud_para[g_service_type]->eq_dl_config.eq_para[index].b[0]   = eq_dbg_eq_para->eq_para[index].b[0];
				p_aud_para[g_service_type]->eq_dl_config.eq_para[index].b[1]   = eq_dbg_eq_para->eq_para[index].b[1];
				p_aud_para[g_service_type]->eq_dl_config.eq_para[index].b[2]   = eq_dbg_eq_para->eq_para[index].b[2]; 

				p_aud_para[g_service_type]->eq_dl_config.eq_load.eq_load_para[index].freq    = eq_dbg_eq_para->eq_load.eq_load_para[index].freq;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.eq_load_para[index].gain    = eq_dbg_eq_para->eq_load.eq_load_para[index].gain;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.eq_load_para[index].q_val   = eq_dbg_eq_para->eq_load.eq_load_para[index].q_val;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.eq_load_para[index].type    = eq_dbg_eq_para->eq_load.eq_load_para[index].type;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.eq_load_para[index].enable  = eq_dbg_eq_para->eq_load.eq_load_para[index].enable;

				p_aud_para[g_service_type]->eq_dl_config.filters = eq_dbg_eq_para->filters;
				p_aud_para[g_service_type]->eq_dl_config.eq_en   = eq_dbg_eq_para->eq_en;
			}
			else
			{
				BK_LOGW(NULL, "eq_dbg_eq_para is NULL\r\n");
			}
		}
			break;
		case 0xFD:
			if(params[1] == 0x01)
			{
				if(eq_dbg_eq_para == NULL) {
					eq_dbg_eq_para = os_malloc(sizeof(app_aud_eq_config_t));
				}
				if(eq_dbg_eq_para) {
					eq_dbg_eq_para->filters = 0;
				}
			}
			else if(params[1] == 0x00)
			{
				bk_app_update_aud_eq_config(eq_dbg_eq_para, g_service_type);
				if(eq_dbg_eq_para) {
					os_free(eq_dbg_eq_para);
					eq_dbg_eq_para = NULL;
				}
			}
			break;
		case 0xFE:
		{
			uint16_t __maybe_unused enable = (params[2] << 8 | params[1]);
			uint16_t __maybe_unused total_gain = (params[4] << 8 | params[3]);
			uint8_t __maybe_unused eqType = params[5]; /// unused params now
			if(eq_dbg_eq_para)
			{
				eq_dbg_eq_para->globle_gain         = (uint32_t)(1.12f * total_gain);
				eq_dbg_eq_para->eq_load.f_gain      = (params[9]<<24)|(params[8]<<16)|(params[7]<<8)|(params[6]);
				eq_dbg_eq_para->eq_load.samplerate  = (params[11]<<8)|(params[10]);

				p_aud_para[g_service_type]->eq_dl_config.globle_gain        = eq_dbg_eq_para->globle_gain;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.f_gain     = eq_dbg_eq_para->eq_load.f_gain;
				p_aud_para[g_service_type]->eq_dl_config.eq_load.samplerate = eq_dbg_eq_para->eq_load.samplerate;
			}
		}
			break;
		default:
			break;
	}
}


