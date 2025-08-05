// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <components/log.h>
#include "cli.h"

#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>


#define TAG "voc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static voice_handle_t gl_voice_handle = NULL;
static voice_read_handle_t gl_voice_read_handle = NULL;
static voice_write_handle_t gl_voice_write_handle = NULL;


int voice_send_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = bk_voice_write_frame_data(gl_voice_write_handle, (char *)data, len);
    if (ret != len)
    {
        LOGE("%s, %d, bk_voice_write_frame_data: %d != %d\n", __func__, __LINE__, ret, len);
    }
    else
    {
        //LOGD("%s, %d, len: %d\n", __func__, __LINE__, len);
    }

    return len;
}

void cli_voice_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s +++\n", __func__);

    if (argc != 9)
    {
        LOGE("%s, %d, agc: %d not right\n", __func__, __LINE__, argc);
        return;
    }

    mic_type_t mic_type = MIC_TYPE_ONBOARD;
    uint32_t mic_samp_rate = 0;
    uint8_t aec_en = 0;
    audio_enc_type_t enc_type = 0;
    audio_dec_type_t dec_type = 0;
    spk_type_t spk_type = SPK_TYPE_ONBOARD;
    uint32_t spk_samp_rate = 0;

    if (os_strcmp(argv[1], "start") == 0)
    {
        if (os_strcmp(argv[2], "onboard") == 0)
        {
            mic_type = MIC_TYPE_ONBOARD;
        }
        else if (os_strcmp(argv[2], "uac") == 0)
        {
            mic_type = MIC_TYPE_UAC;
        }
        else if (os_strcmp(argv[2], "onboard_dual_dmic_mic") == 0)
        {
            mic_type = MIC_TYPE_ONBOARD_DUAL_DMIC_MIC;
        }
        else
        {
            LOGE("%s, %d, mic_type: %s not right\n", __func__, __LINE__, argv[2]);
            return;
        }

        mic_samp_rate = os_strtoul(argv[3], NULL, 10);
        if (mic_samp_rate != 8000 && mic_samp_rate != 16000)
        {
            LOGE("%s, %d, mic_samp_rate: %s not right\n", __func__, __LINE__, mic_samp_rate);
            return;
        }

        aec_en = os_strtoul(argv[4], NULL, 10);
        if (aec_en != 0 && aec_en != 1 && aec_en != 3)
        {
            LOGE("%s, %d, aec_en: %s not right\n", __func__, __LINE__, aec_en);
            return;
        }

        if (os_strcmp(argv[5], "pcm") == 0)
        {
            enc_type = AUDIO_ENC_TYPE_PCM;
        }
        else if (os_strcmp(argv[5], "g711a") == 0)
        {
            enc_type = AUDIO_ENC_TYPE_G711A;
        }
        else if (os_strcmp(argv[5], "g711u") == 0)
        {
            enc_type = AUDIO_ENC_TYPE_G711U;
        }
#if CONFIG_VOICE_SERVICE_AAC_ENCODER
        else if (os_strcmp(argv[5], "aac") == 0)
        {
            enc_type = AUDIO_ENC_TYPE_AAC;
        }
#endif
        else
        {
            LOGE("%s, %d, enc_type: %s not support\n", __func__, __LINE__, argv[5]);
            return;
        }

        if (os_strcmp(argv[6], "pcm") == 0)
        {
            dec_type = AUDIO_DEC_TYPE_PCM;
        }
        else if (os_strcmp(argv[6], "g711a") == 0)
        {
            dec_type = AUDIO_DEC_TYPE_G711A;
        }
        else if (os_strcmp(argv[6], "g711u") == 0)
        {
            dec_type = AUDIO_DEC_TYPE_G711U;
        }
#if CONFIG_VOICE_SERVICE_AAC_DECODER
        else if (os_strcmp(argv[6], "aac") == 0)
        {
            dec_type = AUDIO_DEC_TYPE_AAC;
        }
#endif
        else
        {
            LOGE("%s, %d, dec_type: %s not support\n", __func__, __LINE__, argv[6]);
            return;
        }

        if (os_strcmp(argv[7], "onboard") == 0)
        {
            spk_type = SPK_TYPE_ONBOARD;
        }
        else if (os_strcmp(argv[7], "uac") == 0)
        {
            spk_type = SPK_TYPE_UAC;
        }
        else
        {
            LOGE("%s, %d, spk_type: %s not support\n", __func__, __LINE__, argv[7]);
            return;
        }

        spk_samp_rate = os_strtoul(argv[8], NULL, 10);
        if (spk_samp_rate != 8000 && spk_samp_rate != 16000)
        {
            LOGE("%s, %d, spk_samp_rate: %s not support\n", __func__, __LINE__, spk_samp_rate);
            return;
        }

        /* voice config */
        voice_cfg_t voice_cfg = {0};
        os_memset(&voice_cfg, 0, sizeof(voice_cfg_t));

        if (mic_type == MIC_TYPE_ONBOARD)
        {
            voice_cfg.mic_type = MIC_TYPE_ONBOARD;
            onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
            onboard_mic_cfg.adc_cfg.sample_rate = mic_samp_rate;
            /* one farme size, 20ms */
            if (mic_samp_rate == 8000)
            {
                onboard_mic_cfg.frame_size = 320;
                onboard_mic_cfg.out_block_size = 320;
            }
            else
            {
                onboard_mic_cfg.frame_size = 640;
                onboard_mic_cfg.out_block_size = 640;
            }
            voice_cfg.mic_cfg.onboard_mic_cfg = onboard_mic_cfg;
        }
        else if(mic_type == MIC_TYPE_UAC)
        {
            voice_cfg.mic_type = MIC_TYPE_UAC;
            uac_mic_stream_cfg_t uac_mic_cfg = UAC_MIC_STREAM_CFG_DEFAULT();
            uac_mic_cfg.samp_rate = mic_samp_rate;
            /* one farme size, 20ms */
            if (mic_samp_rate == 8000)
            {
                uac_mic_cfg.frame_size = 320;
                uac_mic_cfg.out_block_size = 320;
            }
            else
            {
                uac_mic_cfg.frame_size = 640;
                uac_mic_cfg.out_block_size = 640;
            }
            uac_mic_cfg.out_block_num = 1;
            voice_cfg.mic_cfg.uac_mic_cfg = uac_mic_cfg;
        }
        else if(mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
        {
            voice_cfg.mic_type = MIC_TYPE_ONBOARD_DUAL_DMIC_MIC;
            onboard_dual_dmic_mic_stream_cfg_t onboard_dual_dmic_mic_cfg = DEFAULT_ONBOARD_DUAL_DMIC_STREAM_CONFIG();
            onboard_dual_dmic_mic_cfg.adc_cfg.sample_rate = mic_samp_rate;
            /* one farme size, 20ms */
            if (mic_samp_rate == 16000)
            {
                onboard_dual_dmic_mic_cfg.frame_size = 640;
                onboard_dual_dmic_mic_cfg.out_block_size = 640;
            }
            else
            {
                LOGE("%s, %d,mic samp rate:%d,dual_dmic only support 16000 sample rate!\n", __func__, __LINE__,mic_samp_rate, mic_samp_rate);
                goto fail;
            }
            
            if(!aec_en)
            {
                onboard_dual_dmic_mic_cfg.dual_dmic_sgl_out = 1;
                LOGD("%s, %d, dual_dmic_mic:no aec,dual_dmic_sgl_out set to 1!\n", __func__, __LINE__);
            }
            voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg = onboard_dual_dmic_mic_cfg;
        }
        else
        {
            LOGE("%s, %d, mic_type:%d is invalid!\n", __func__, __LINE__,mic_type);
            goto fail;
        }

        if (aec_en)
        {
            voice_cfg.aec_en = true;
            voice_cfg.aec_ver = aec_en;
            if(1 == aec_en)//aec v1
            {
                aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
                aec_alg_cfg.out_block_num = 1;
                aec_alg_cfg.aec_cfg.fs = mic_samp_rate;
                voice_cfg.aec_cfg.aec_alg_cfg = aec_alg_cfg;
            }
            else if (3 == aec_en)//aec v3
            {
                aec_v3_algorithm_cfg_t aec_v3_alg_cfg = DEFAULT_AEC_V3_ALGORITHM_CONFIG();
                aec_v3_alg_cfg.out_block_num = 1;
                aec_v3_alg_cfg.aec_cfg.fs = mic_samp_rate;
                if(mic_type == MIC_TYPE_ONBOARD_DUAL_DMIC_MIC)
                {
                    aec_v3_alg_cfg.dual_ch = 1;
                }
                if(aec_v3_alg_cfg.vad_cfg.vad_enable)
                {
                    voice_cfg.enc_common.frame_in_ms = 20;
                    voice_cfg.enc_common.frame_in_size = mic_samp_rate*20/1000*2;
                }
                voice_cfg.aec_cfg.aec_v3_alg_cfg = aec_v3_alg_cfg;
            }
            else
            {
                LOGE("%s, %d, aec ver:%d is invalid!\n", __func__, __LINE__,aec_en);
                goto fail;
            }
        }
        else
        {
            voice_cfg.aec_en = aec_en;
            voice_cfg.aec_cfg.reserve = 0;
        }

        voice_cfg.enc_type = enc_type;
        if (enc_type == AUDIO_ENC_TYPE_G711A || enc_type == AUDIO_ENC_TYPE_G711U)
        {
            g711_encoder_cfg_t g711_enc_cfg = DEFAULT_G711_ENCODER_CONFIG();
            if (enc_type == AUDIO_ENC_TYPE_G711A)
            {
                g711_enc_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            }
            else
            {
                g711_enc_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }

            if (mic_samp_rate == 8000)
            {
                g711_enc_cfg.buf_sz = 320;
                g711_enc_cfg.out_block_size = 160;
                voice_cfg.read_pool_size = 160;
            }
            else
            {
                g711_enc_cfg.buf_sz = 640;
                g711_enc_cfg.out_block_size = 320;
                voice_cfg.read_pool_size = 320;
            }
            voice_cfg.enc_cfg.g711_enc_cfg = g711_enc_cfg;
        }
        else if (enc_type == AUDIO_ENC_TYPE_PCM)
        {
            voice_cfg.enc_cfg.pcm_enc_cfg = 0;
            if (mic_samp_rate == 8000)
            {
                voice_cfg.read_pool_size = 320;
            }
            else
            {
                voice_cfg.read_pool_size = 640;
            }
        }
#if CONFIG_VOICE_SERVICE_AAC_ENCODER
        else if (enc_type == AUDIO_ENC_TYPE_AAC)
        {
            aac_encoder_cfg_t aac_enc_cfg = DEFAULT_AAC_ENCODER_CONFIG();
            aac_enc_cfg.samp_rate = mic_samp_rate;
            aac_enc_cfg.in_pool_len = aac_enc_cfg.buffer_len + aac_enc_cfg.samp_rate * aac_enc_cfg.bits / 8 * aac_enc_cfg.chl_num / 1000 * 20;
            voice_cfg.enc_cfg.aac_enc_cfg = aac_enc_cfg;
        }
#endif
        else
        {
            //noting todo
        }

        voice_cfg.dec_type = dec_type;
        if (dec_type == AUDIO_DEC_TYPE_G711A || dec_type == AUDIO_DEC_TYPE_G711U)
        {
            g711_decoder_cfg_t g711_dec_cfg = DEFAULT_G711_DECODER_CONFIG();
            if (dec_type == AUDIO_DEC_TYPE_G711A)
            {
                g711_dec_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            }
            else
            {
                g711_dec_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }

            if (spk_samp_rate == 8000)
            {
                g711_dec_cfg.buf_sz = 160;
                g711_dec_cfg.out_block_size = 320;
                voice_cfg.write_pool_size = 160;
            }
            else
            {
                g711_dec_cfg.buf_sz = 320;
                g711_dec_cfg.out_block_size = 640;
                voice_cfg.write_pool_size = 320;
            }
            voice_cfg.dec_cfg.g711_dec_cfg = g711_dec_cfg;
        }
        else if (dec_type == AUDIO_DEC_TYPE_PCM)
        {
            voice_cfg.dec_cfg.pcm_dec_cfg = 0;
            if (spk_samp_rate == 8000)
            {
                voice_cfg.write_pool_size = 320;
            }
            else
            {
                voice_cfg.write_pool_size = 640;
            }
        }
#if CONFIG_VOICE_SERVICE_AAC_DECODER
        else if (dec_type == AUDIO_DEC_TYPE_AAC)
        {
            aac_decoder_cfg_t aac_dec_cfg = DEFAULT_AAC_DECODER_CONFIG();
            voice_cfg.dec_cfg.aac_dec_cfg = aac_dec_cfg;
        }
#endif
        else
        {
            //noting todo
        }

        voice_cfg.spk_type = spk_type;
        if (spk_type == SPK_TYPE_ONBOARD)
        {
            onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
            onboard_spk_cfg.sample_rate = spk_samp_rate;
            if(MIC_TYPE_ONBOARD_DUAL_DMIC_MIC == mic_type)
            {
                onboard_spk_cfg.clk_src = AUD_CLK_APLL;
            }
            /* one farme size, 20ms */
            if (spk_samp_rate == 8000)
            {
                onboard_spk_cfg.frame_size = 320;
            }
            else
            {
                onboard_spk_cfg.frame_size = 640;
            }
            voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;
            if (aec_en)
            {
                onboard_spk_cfg.multi_out_port_num = 1;
            }
            else
            {
                onboard_spk_cfg.multi_out_port_num = 0;
            }
            voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;
        }
        else
        {
            uac_speaker_stream_cfg_t uac_spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
            uac_spk_cfg.samp_rate = spk_samp_rate;
            /* one farme size, 20ms */
            if (spk_samp_rate == 8000)
            {
                uac_spk_cfg.frame_size = 320;
            }
            else
            {
                uac_spk_cfg.frame_size = 640;
            }
            if (aec_en)
            {
                uac_spk_cfg.multi_out_port_num = 1;
            }
            else
            {
                uac_spk_cfg.multi_out_port_num = 0;
            }
            voice_cfg.spk_cfg.uac_spk_cfg = uac_spk_cfg;
        }

        voice_cfg.event_handle = NULL;
        voice_cfg.args = NULL;

        /* start voice */
        gl_voice_handle = bk_voice_init(&voice_cfg);
        if (!gl_voice_handle)
        {
            LOGE("%s, %d, voice init fail\n", __func__, __LINE__);
            goto fail;
        }

        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = gl_voice_handle;

#if CONFIG_VOICE_SERVICE_AAC_ENCODER
        if (enc_type == AUDIO_ENC_TYPE_AAC)
        {
            voice_read_cfg.max_read_size = voice_cfg.enc_cfg.aac_enc_cfg.out_block_size;
            //LOGI("%s, %d, aac out_block_size: %d\n", __func__, __LINE__, voice_cfg.enc_cfg.aac_enc_cfg.out_block_size);
        }
        else
        {
            voice_read_cfg.max_read_size = mic_samp_rate * 2 * 20 / 1000; //one frame size(20ms)
        }
#else
        voice_read_cfg.max_read_size = mic_samp_rate * 2 * 20 / 1000; //one frame size(20ms)
#endif
        voice_read_cfg.voice_read_callback = voice_send_callback;
        voice_read_cfg.args = NULL;
        voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
        gl_voice_read_handle = bk_voice_read_init(&voice_read_cfg);
        if (!gl_voice_read_handle)
        {
            LOGE("%s, %d, voice read init fail\n", __func__, __LINE__);
            goto fail;
        }

        voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
        voice_write_cfg.voice_handle = gl_voice_handle;
        voice_write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
        gl_voice_write_handle = bk_voice_write_init(&voice_write_cfg);
        if (!gl_voice_write_handle)
        {
            LOGE("voice write init fail\n");
            goto fail;
        }

        if (BK_OK != bk_voice_start(gl_voice_handle))
        {
            LOGE("%s, %d, voice start fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != bk_voice_read_start(gl_voice_read_handle))
        {
            LOGE("%s, %d, voice read start fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != bk_voice_write_start(gl_voice_write_handle))
        {
            LOGE("voice write start fail\n");
            goto fail;
        }

    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGD("voice stop\n");
        goto fail;
    }
    else
    {
        LOGE("%s, %d, cmd not support\n", __func__, __LINE__);
    }

    LOGD("%s ---complete\n", __func__);

    return;

fail:
    if (gl_voice_read_handle)
    {
        bk_voice_read_stop(gl_voice_read_handle);
    }

    if (gl_voice_write_handle)
    {
        bk_voice_write_stop(gl_voice_write_handle);
    }

    if (gl_voice_handle)
    {
        bk_voice_stop(gl_voice_handle);
    }

    if (gl_voice_read_handle)
    {
        bk_voice_read_deinit(gl_voice_read_handle);
    }

    if (gl_voice_write_handle)
    {
        bk_voice_write_deinit(gl_voice_write_handle);
    }

    if (gl_voice_handle)
    {
        bk_voice_deinit(gl_voice_handle);
    }
    gl_voice_read_handle = NULL;
    gl_voice_write_handle = NULL;
    gl_voice_handle  = NULL;
}

#define VOICE_CMD_CNT   (sizeof(s_voice_commands) / sizeof(struct cli_command))

static const struct cli_command s_voice_commands[] =
{
    /* voice {cmd mic_type mic_samp_rate aec_en enc_type dec_type spk_type}
     *
     * [cmd]            start/stop
     * [mic_type]       onboard/uac/onboard_dual_dmic_mic
     * [mic_samp_rate]  8000/16000
     * [aec_en]         0/1/3
     * [enc_type]       pcm/g711a/g711u/aac
     * [dec_type]       pcm/g711a/g711u/aac
     * [spk_type]       onboard/uac
     * [spk_samp_rate]  8000/16000
     */

    {"voice", "voice {start|stop onboard|uac|onboard_dual_dmic_mic 8000|16000 0|1|3 pcm|g711a|g711u|aac pcm|g711a|g711u|aac onboard|uac 8000|16000}", cli_voice_test_cmd},
};

int cli_voice_init(void)
{
    return cli_register_commands(s_voice_commands, VOICE_CMD_CNT);
}
