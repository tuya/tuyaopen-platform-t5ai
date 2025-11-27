// Copyright 2025-2026 Beken
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

#include <components/bk_player_service.h>
#include <components/bk_player_service_types.h>
#include "prompt_tone_test.h"

#if CONFIG_VOICE_SERVICE_TEST
#include <components/bk_voice_service.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#endif

#define TAG "player_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
#define OUTPUT_RB_SIZE  (4 * 1024)
static audio_port_handle_t gl_output_port_handle = NULL;
static audio_port_handle_t gl_output_port1_handle = NULL;
extern voice_handle_t gl_voice_handle;
#endif


static bk_player_handle_t gl_player_handle = NULL;
static bk_player_handle_t gl_player1_handle = NULL;

static int player_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGD("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);

    return BK_OK;
}

static int player1_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGD("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);

    return BK_OK;
}

int player_event_handler(int data, void *params, void *args)
{
    LOGD("%s, %d, data: %d\n", __func__, __LINE__, data);

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        audio_element_info_t *music_info = (audio_element_info_t *)params;
        audio_element_handle_t spk_element = bk_voice_get_spk_element(gl_voice_handle);
        if (spk_element == NULL)
        {
            LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = {0};
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.dig_gain = 0x2d;
        port_info.ana_gain = 0x01;
        port_info.bits = music_info->bits;
        port_info.port_id = 1;
        port_info.priority = 1;
        port_info.port = gl_output_port_handle;
        port_info.notify_cb = player_port_state_notify_handler;
        port_info.user_data = NULL;
        if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
        {
            LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        LOGD("player_event_handler, BK_PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", music_info->sample_rates, music_info->bits, music_info->channels);
        LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
#endif
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("%s, %d, BK_PLAYER_EVENT_FINISH\n", __func__, __LINE__);
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}

int player1_event_handler(int data, void *params, void *args)
{
    LOGD("%s, %d, data: %d\n", __func__, __LINE__, data);

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        audio_element_info_t *music_info = (audio_element_info_t *)params;
        audio_element_handle_t spk_element = bk_voice_get_spk_element(gl_voice_handle);
        if (spk_element == NULL)
        {
            LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = {0};
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.dig_gain = 0x2d;
        port_info.ana_gain = 0x01;
        port_info.bits = music_info->bits;
        port_info.port_id = 2;
        port_info.priority = 2;
        port_info.port = gl_output_port1_handle;
        port_info.notify_cb = player1_port_state_notify_handler;
        port_info.user_data = NULL;
        if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
        {
            LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        LOGI("player_event_handler, BK_PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", music_info->sample_rates, music_info->bits, music_info->channels);
        LOGI("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
#endif
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("%s, %d, BK_PLAYER_EVENT_FINISH\n", __func__, __LINE__);
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}

void cli_player_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s +++\n", __func__);
    bk_err_t ret = BK_OK;
    player_uri_info_t uri_info = {0};

    if (argc < 2)
    {
        LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
        return;
    }

    if (os_strcmp(argv[1], "create") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        bk_player_cfg_t player_cfg = DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG();
        player_cfg.spk_cfg.onboard_spk_cfg.sample_rate = 8000;
        player_cfg.args = NULL;
        if (os_strcmp(argv[2], "0") == 0)
        {
            player_cfg.event_handle = player_event_handler;
            gl_player_handle = bk_player_create(&player_cfg);
            if (!gl_player_handle)
            {
                LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                return;
            }
        }
        else if (os_strcmp(argv[2], "1") == 0)
        {
            player_cfg.event_handle = player1_event_handler;
            gl_player1_handle = bk_player_create(&player_cfg);
            if (!gl_player1_handle)
            {
                LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                return;
            }
        }
        else
        {
            LOGE("%s, %d, argv[2]: %s not right\n", __func__, __LINE__, argv[2]);
            return;
        }
        //bk_player_set_decode_type(gl_player_handle, AUDIO_DEC_TYPE_PCM);
    }
    else if (os_strcmp(argv[1], "set_uri") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        if (os_strcmp(argv[2], "array") == 0)
        {
            if (argc < 4)
            {
                LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
                return;
            }
            int array_id = os_strtoul(argv[3], NULL, 10);
            uri_info.uri_type = PLAYER_URI_TYPE_ARRAY;
            if (array_id == 0)
            {
                uri_info.uri = (char *)asr_wakeup_prompt_tone_array;
                uri_info.total_len = sizeof(asr_wakeup_prompt_tone_array);    
            }
            else if (array_id == 1)
            {
                uri_info.uri = (char *)asr_standby_prompt_tone_array;
                uri_info.total_len = sizeof(asr_standby_prompt_tone_array);      
            }
#if 0
            else if (array_id == 2)
            {
                uri_info.uri = (char *)network_provision_prompt_tone_array;
                uri_info.total_len = sizeof(network_provision_prompt_tone_array);
            }
            else if (array_id == 3)
            {
                uri_info.uri = (char *)network_provision_success_prompt_tone_array;
                uri_info.total_len = sizeof(network_provision_success_prompt_tone_array);         
            }
            else if (array_id == 4)
            {
                uri_info.uri = (char *)network_provision_fail_prompt_tone_array;
                uri_info.total_len = sizeof(network_provision_fail_prompt_tone_array);
            }
#endif
            else
            {
                LOGE("%s, %d, array_id: %d not right\n", __func__, __LINE__, array_id);
                return;
            }

            if (argc == 5)
            {
                if (os_strcmp(argv[4], "1") == 0)
                {
                    ret = bk_player_set_uri(gl_player1_handle, &uri_info);
                }
                else
                {
                    ret = bk_player_set_uri(gl_player_handle, &uri_info);
                }
            }
            else
            {
                ret = bk_player_set_uri(gl_player_handle, &uri_info);
            }
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
            }
            //TODO
            //mic_type = MIC_TYPE_ONBOARD;
        }
        else if (os_strcmp(argv[2], "vfs") == 0)
        {
            if (argc < 4)
            {
                LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
                return;
            }
            uri_info.uri_type = PLAYER_URI_TYPE_VFS;
            uri_info.uri = argv[3];

            if (argc == 5)
            {
                if (os_strcmp(argv[4], "1") == 0)
                {
                    ret = bk_player_set_uri(gl_player1_handle, &uri_info);
                }
                else
                {
                    ret = bk_player_set_uri(gl_player_handle, &uri_info);
                }
            }
            else
            {
                ret = bk_player_set_uri(gl_player_handle, &uri_info);
            }
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
            }
        }
        else
        {
            LOGE("%s, %d, uri type: %s is not support\n", __func__, __LINE__, argv[2]);
            return;
        }
    }
    else if (os_strcmp(argv[1], "start") == 0)
    {
        if (argc == 3)
        {
            if (os_strcmp(argv[2], "1") == 0)
            {
                ret = bk_player_start(gl_player1_handle);
            }
            else
            {
                ret = bk_player_start(gl_player_handle);
            }
        }
        else
        {
            ret = bk_player_start(gl_player_handle);
        }
        if (ret != BK_OK)
        {
            LOGE("%s, %d, bk_player_start fail\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "set_dec_type") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        if (os_strcmp(argv[2], "1") == 0)
        {
            ret = bk_player_set_decode_type(gl_player1_handle, os_strtoul(argv[2], NULL, 10));
        }
        else
        {
            ret = bk_player_set_decode_type(gl_player_handle, os_strtoul(argv[2], NULL, 10));
        }

        if (ret != BK_OK)
        {
            LOGE("%s, %d, bk_player_set_decode_type fail\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        if (os_strcmp(argv[2], "1") == 0)
        {
            ret = bk_player_stop(gl_player1_handle);
        }
        else
        {
            ret = bk_player_stop(gl_player_handle);
        }

        if (ret != BK_OK)
        {
            LOGE("%s, %d, bk_player_stop fail\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "destroy") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        if (os_strcmp(argv[2], "1") == 0)
        {
            ret = bk_player_destroy(gl_player1_handle);
        }
        else
        {
            ret = bk_player_destroy(gl_player_handle);
        }

        if (ret != BK_OK)
        {
            LOGE("%s, %d, bk_player_deinit fail\n", __func__, __LINE__);
        }
    }
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    else if (os_strcmp(argv[1], "prompt_tone") == 0)
    {
        if (argc < 4)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        uint32_t tone_id = 0;

        if (os_strcmp(argv[2], "start") == 0)
        {
            if (argc < 5)
            {
                LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
                return;
            }

            tone_id = os_strtoul(argv[3], NULL, 10);

            /* step 1: create player */
            bk_player_cfg_t player_cfg = DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG();
            if (tone_id == 1)
            {
                player_cfg.event_handle = player1_event_handler;
            }
            else
            {
                player_cfg.event_handle = player_event_handler;
            }
            player_cfg.args = NULL;
            bk_player_handle_t tmp_player_handle = NULL;
            if (tone_id == 1)
            {
                gl_player1_handle = bk_player_create(&player_cfg);
                if (!gl_player1_handle)
                {
                    LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                    return;
                }
                tmp_player_handle = gl_player1_handle;
            }
            else
            {
                gl_player_handle = bk_player_create(&player_cfg);
                if (!gl_player_handle)
                {
                    LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                    return;
                }
                tmp_player_handle = gl_player_handle;
            }

            /* step 2: set uri */
            uri_info.uri_type = PLAYER_URI_TYPE_VFS;
            uri_info.uri = argv[4];
            ret = bk_player_set_uri(tmp_player_handle, &uri_info);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
            }

            /* step 3: set output port */
            ringbuf_port_cfg_t cfg = RINGBUF_PORT_CFG_DEFAULT();
            cfg.ringbuf_size = OUTPUT_RB_SIZE;
            audio_port_handle_t tmp_output_port_handle = NULL;
            if (tone_id == 1)
            {
                gl_output_port1_handle = ringbuf_port_init(&cfg);
                if (gl_output_port1_handle == NULL)
                {
                    LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
                    return;
                }
                tmp_output_port_handle = gl_output_port1_handle;
            }
            else
            {
                gl_output_port_handle = ringbuf_port_init(&cfg);
                if (gl_output_port_handle == NULL)
                {
                    LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
                    return;
                }
                tmp_output_port_handle = gl_output_port_handle;
            }

            ret = bk_player_set_output_port(tmp_player_handle, tmp_output_port_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_set_output_port fail\n", __func__, __LINE__);
                return;
            }

            if (!gl_voice_handle)
            {
                LOGE("%s, %d, gl_voice_handle is NULL\n", __func__, __LINE__);
                return;
            }

            /* step 4: start player */
            ret = bk_player_start(tmp_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_start fail\n", __func__, __LINE__);
                return;
            }
        }
        else if (os_strcmp(argv[2], "stop") == 0)
        {
            bk_player_handle_t tmp_player_handle = NULL;
            audio_port_handle_t tmp_output_port_handle = NULL;
            uint32_t tone_id = os_strtoul(argv[3], NULL, 10);
            if (tone_id == 1)
            {
                tmp_player_handle = gl_player1_handle;
                tmp_output_port_handle = gl_output_port1_handle;
            }
            else
            {
                tmp_player_handle = gl_player_handle;
                tmp_output_port_handle = gl_output_port_handle;
            }

            ret = bk_player_stop(tmp_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_stop fail\n", __func__, __LINE__);
            }

            audio_element_handle_t spk_element = bk_voice_get_spk_element(gl_voice_handle);
            if (spk_element == NULL)
            {
                LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
                return;
            }

            audio_port_info_t port_info = {0};
            port_info.chl_num = 1;
            port_info.sample_rate = 8000;
            port_info.dig_gain = 0x2d;
            port_info.ana_gain = 0x01;
            port_info.bits = 16;
            if (tone_id == 1)
            {
                port_info.port_id = 2;
                port_info.priority = 2;
            }
            else
            {
                port_info.port_id = 1;
                port_info.priority = 1;
            }
            port_info.port = NULL;
            if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
            {
                LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                return;
            }

            bk_player_set_output_port(tmp_player_handle, NULL);

            audio_port_deinit(tmp_output_port_handle);

            ret = bk_player_destroy(tmp_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_destroy fail\n", __func__, __LINE__);
            }
        }
    }
#endif
    else
    {
        LOGE("%s, %d, cmd: %s is not support fail\n", __func__, __LINE__, argv[1]);
    }

    LOGD("%s ---complete\n", __func__);

    return;
}

#define PLAYER_CMD_CNT   (sizeof(s_player_commands) / sizeof(struct cli_command))

static const struct cli_command s_player_commands[] =
{
    {"player", "player ...", cli_player_test_cmd},
};

int cli_player_init(void)
{
    return cli_register_commands(s_player_commands, PLAYER_CMD_CNT);
}
