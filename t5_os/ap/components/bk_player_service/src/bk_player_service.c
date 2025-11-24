#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>

#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/array_stream.h>
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include <components/bk_audio/audio_decoders/g711_decoder.h>
#include <components/bk_audio/audio_decoders/mp3_decoder.h>
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <components/bk_audio/audio_decoders/wav_decoder.h>

#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_player_service.h>
#include <components/bk_player_service_types.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>

#define TAG "player"

#define PLAYER_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGE(TAG, "%s, %d, PLAYER_CHECK_NULL fail \n", __func__, __LINE__);\
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

struct player
{
    audio_pipeline_handle_t play_pipeline;      /**< play pipeline handle, [input]-->[decode]-->[speaker] */
    audio_strm_type_t       in_strm_type;       /**< input stream type */
    audio_element_handle_t  in_stream;          /**< input stream handle, used to write data to decoder */
    audio_dec_type_t        dec_type;           /**< decoder type */
    audio_element_handle_t  spk_dec;            /**< speaker decoder handle */
    spk_type_t              spk_type;           /**< onboard speaker or uac speaker */
    audio_element_handle_t  spk_str;            /**< speaker stream handle */

    audio_event_iface_handle_t play_evt;        /**< play event handle */

    bk_player_state_t       state;              /**< player state */

    player_event_handle_cb  event_handle;       /**< player event handle callback */
    void *                  args;               /**< the parameter of event_handle func */

    beken_thread_t          listener_task_hdl;
    beken_queue_t           listener_msg_que;
    beken_semaphore_t       listener_sem;
    bool                    listener_is_running;
};

static bk_err_t play_pipeline_start(audio_pipeline_handle_t play_pipeline)
{
    PLAYER_CHECK_NULL(play_pipeline, return BK_FAIL);

    if (BK_OK != audio_pipeline_run(play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline run fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_stop(audio_pipeline_handle_t play_pipeline)
{
    PLAYER_CHECK_NULL(play_pipeline, return BK_FAIL);

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

static bk_err_t play_pipeline_deinit(bk_player_handle_t player_handle)
{
    BK_LOGD(TAG, "%s\n", __func__);

    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    if (!player_handle->play_pipeline)
    {
        return BK_OK;
    }

    //audio_element_set_input_timeout(player_handle->spk_dec, 0);

    if (player_handle->state == PLAYER_STATE_PLAYING)
    {
        play_pipeline_stop(player_handle->play_pipeline);
    }

    if (BK_OK != audio_pipeline_terminate(player_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline terminate fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (player_handle->in_stream && BK_OK != audio_pipeline_unregister(player_handle->play_pipeline, player_handle->in_stream))
    {
        BK_LOGE(TAG, "%s, %d, unregister raw_write stream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (player_handle->spk_dec && BK_OK != audio_pipeline_unregister(player_handle->play_pipeline, player_handle->spk_dec))
    {
        BK_LOGE(TAG, "%s, %d, unregister decoder fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (player_handle->spk_str && BK_OK != audio_pipeline_unregister(player_handle->play_pipeline, player_handle->spk_str))
    {
        BK_LOGE(TAG, "%s, %d, unregister spk stream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (player_handle->play_evt)
    {
        /* deinit listener */
        if (BK_OK != audio_pipeline_remove_listener(player_handle->play_pipeline))
        {
            BK_LOGE(TAG, "%s, %d, pipeline terminate fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        if (BK_OK != audio_event_iface_destroy(player_handle->play_evt))
        {
            BK_LOGE(TAG, "%s, %d, pipeline terminate fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    if (BK_OK != audio_pipeline_deinit(player_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        player_handle->play_pipeline = NULL;
    }

    if (player_handle->in_stream && BK_OK != audio_element_deinit(player_handle->in_stream))
    {
        BK_LOGE(TAG, "%s, %d, raw_write stream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        player_handle->in_stream = NULL;
    }

    if (player_handle->spk_dec && BK_OK != audio_element_deinit(player_handle->spk_dec))
    {
        BK_LOGE(TAG, "%s, %d, decoder deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        player_handle->spk_dec = NULL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_init(bk_player_handle_t player_handle)
{
    bk_err_t ret = BK_OK;

    BK_LOGD(TAG, "%s\n", __func__);
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    BK_LOGD(TAG, "step1: play pipeline init\n");
    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    play_pipeline_cfg.rb_size = 4 * 1024;
    player_handle->play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
    PLAYER_CHECK_NULL(player_handle->play_pipeline, return BK_FAIL);
    BK_LOGD(TAG, "step2: init play elements\n");

    switch (player_handle->in_strm_type)
    {
        case AUDIO_STRM_TYPE_ARRAY:
        {
            array_stream_cfg_t array_reader_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
            array_reader_cfg.type = AUDIO_STREAM_READER;
            player_handle->in_stream = array_stream_init(&array_reader_cfg);
        }
            break;

        case AUDIO_STRM_TYPE_VFS:
        {
            vfs_stream_cfg_t vfs_reader_cfg = DEFAULT_VFS_STREAM_CONFIG();
            vfs_reader_cfg.type = AUDIO_STREAM_READER;
            player_handle->in_stream = vfs_stream_init(&vfs_reader_cfg);
        }
            break;

         default:
            BK_LOGE(TAG, "%s, %d, in_stream_type: %d is not support\n", __func__, __LINE__, player_handle->in_strm_type);
            goto fail;
            break;
    }
    PLAYER_CHECK_NULL(player_handle->in_stream, goto fail);

    switch (player_handle->dec_type)
    {
        case AUDIO_DEC_TYPE_G711A:
        case AUDIO_DEC_TYPE_G711U:
        {
            g711_decoder_cfg_t g711_dec_cfg = DEFAULT_G711_DECODER_CONFIG();
            g711_dec_cfg.dec_mode = player_handle->dec_type;
            player_handle->spk_dec = g711_decoder_init(&g711_dec_cfg);
        }
            break;

        case AUDIO_DEC_TYPE_MP3:
        {
            mp3_decoder_cfg_t mp3_dec_cfg = DEFAULT_MP3_DECODER_CONFIG();
            player_handle->spk_dec = mp3_decoder_init(&mp3_dec_cfg);
        }
            break;

        case AUDIO_DEC_TYPE_PCM:
            /* not need decoder */
            break;

        case AUDIO_DEC_TYPE_WAV:
        {
            wav_decoder_cfg_t wav_dec_cfg = DEFAULT_WAV_DECODER_CONFIG();
            player_handle->spk_dec = wav_decoder_init(&wav_dec_cfg);
        }
            break;

        default:
            BK_LOGE(TAG, "%s, %d, dec_type: %d is not support\n", __func__, __LINE__, player_handle->dec_type);
            goto fail;
            break;
    }
    if (player_handle->dec_type != AUDIO_DEC_TYPE_PCM)
    {
        PLAYER_CHECK_NULL(player_handle->spk_dec, goto fail);
    }

    BK_LOGD(TAG, "step3: play pipeline register\n");
    if (BK_OK != audio_pipeline_register(player_handle->play_pipeline, player_handle->in_stream, "input"))
    {
        BK_LOGE(TAG, "%s, %d, register input stream fail", __func__, __LINE__);
        goto fail;
    }

    if (player_handle->spk_dec && BK_OK != audio_pipeline_register(player_handle->play_pipeline, player_handle->spk_dec, "decode"))
    {
        BK_LOGE(TAG, "%s, %d, register decoder fail", __func__, __LINE__);
        goto fail;
    }

    if (player_handle->spk_str && BK_OK != audio_pipeline_register(player_handle->play_pipeline, player_handle->spk_str, "spk"))
    {
        BK_LOGE(TAG, "%s, %d, register spk stream fail", __func__, __LINE__);
        goto fail;
    }

    BK_LOGD(TAG, "step4: play pipeline link\n");
    if (player_handle->spk_str)
    {
        if (player_handle->spk_dec)
        {
            ret = audio_pipeline_link(player_handle->play_pipeline, (const char *[])
            {"input", "decode", "spk"
            }, 3);
        }
        else
        {
            ret = audio_pipeline_link(player_handle->play_pipeline, (const char *[])
            {"input", "spk"
            }, 2);
        }
    }
    else
    {
        if (player_handle->spk_dec)
        {
            ret = audio_pipeline_link(player_handle->play_pipeline, (const char *[])
            {"input", "decode"
            }, 2);
        }
        else
        {
            ret = audio_pipeline_link(player_handle->play_pipeline, (const char *[])
            {"input"
            }, 1);
        }
    }
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline link fail\n", __func__, __LINE__);
        goto fail;
    }

    if (player_handle->event_handle)
    {
        BK_LOGD(TAG, "step5: init play event listener\n");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        player_handle->play_evt = audio_event_iface_init(&evt_cfg);
        if (player_handle->play_evt == NULL)
        {
            BK_LOGE(TAG, "%s, %d, play_event init fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != audio_pipeline_set_listener(player_handle->play_pipeline, player_handle->play_evt))
        {
            BK_LOGE(TAG, "%s, %d, init play pipeline listener fail\n", __func__, __LINE__);
            goto fail;
        }
    }

    return BK_OK;

fail:
    play_pipeline_deinit(player_handle);

    return BK_FAIL;
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
    bk_player_handle_t player_handle = (bk_player_handle_t)param_data;
    bk_err_t ret = BK_OK;

    player_handle->listener_is_running = false;
    long unsigned int wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&player_handle->listener_sem);

    while (1)
    {
        listener_msg_t listener_msg;
        ret = rtos_pop_from_queue(&player_handle->listener_msg_que, &listener_msg, wait_time);
        if (kNoErr == ret)
        {
            switch (listener_msg.op)
            {
                case LISTENER_IDLE:
                    player_handle->listener_is_running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case LISTENER_EXIT:
                    goto exit;
                    break;

                case LISTENER_START:
                    player_handle->listener_is_running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        audio_event_iface_msg_t event_msg;
        audio_element_status_t el_status = AEL_STATUS_NONE;
        if (player_handle->listener_is_running)
        {
            ret = audio_event_iface_listen(player_handle->play_evt, &event_msg, 10 / portTICK_RATE_MS);//portMAX_DELAY
            if (ret == BK_OK)
            {
                //BK_LOGW(TAG, "%s, %d, ++>>play pipeline event received, state: %d, ele: %p, player state: %d\n", __func__, __LINE__, (int)event_msg.data, event_msg.source, player_handle->state);
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
                            if (player_handle->state == PLAYER_STATE_PLAYING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>>record pipeline event received, state: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop voice pipeline */
                                bk_player_stop(player_handle);
                                audio_pipeline_reset_port(player_handle->play_pipeline);
                                audio_pipeline_reset_elements(player_handle->play_pipeline);
                                audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);
#if 0
                                if ((audio_element_handle_t)event_msg.source == player_handle->mic_str)
                                {
                                    bk_voice_event_handle(player_handle->event_handle, VOC_EVT_MIC_NOT_SUPPORT, NULL, player_handle->args);
                                }
                                else
                                {
                                    bk_voice_event_handle(player_handle->event_handle, VOC_EVT_ERROR_UNKNOW, NULL, player_handle->args);
                                }
#endif
                                /* stop listener */
                                player_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        case AEL_STATUS_STATE_STOPPED:
                        case AEL_STATUS_STATE_FINISHED:
                            BK_LOGW(TAG, "%s, %d, ++>>play pipeline event received, state: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                            if (player_handle->spk_str)
                            {
                                /* Stop the player when receiving a finish status report from the speaker stream */
                                if (el_status == AEL_STATUS_STATE_FINISHED && event_msg.source == player_handle->spk_str && player_handle->state == PLAYER_STATE_PLAYING)
                                {
                                    //BK_LOGW(TAG, "%s, %d, ++>>play pipeline event received, state: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                    /* stop play pipeline */
                                    bk_player_stop(player_handle);
                                    audio_pipeline_reset_port(player_handle->play_pipeline);
                                    audio_pipeline_reset_elements(player_handle->play_pipeline);
                                    audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);
                                    if (player_handle->event_handle)
                                    {
                                        player_handle->event_handle(PLAYER_EVENT_STOP, NULL, player_handle->args);
                                    }
                                    /* stop listener */
                                    player_handle->listener_is_running = false;
                                    wait_time = BEKEN_WAIT_FOREVER;
                                    continue;
                                }
                            }
                            else
                            {
                                //BK_LOGW(TAG, "%s, %d, ++>>play pipeline event received, state: %d, ele: %p, player state: %d\n", __func__, __LINE__, (int)event_msg.data, event_msg.source, player_handle->state);
                                /* Stop the player when receiving a finish status report from the speaker stream */
                                if (player_handle->spk_dec && el_status == AEL_STATUS_STATE_FINISHED && event_msg.source == player_handle->spk_dec && player_handle->state == PLAYER_STATE_PLAYING)
                                {
                                    /* stop play pipeline */
                                    bk_player_stop(player_handle);
                                    audio_pipeline_reset_port(player_handle->play_pipeline);
                                    audio_pipeline_reset_elements(player_handle->play_pipeline);
                                    audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);
                                    if (player_handle->event_handle)
                                    {
                                        player_handle->event_handle(PLAYER_EVENT_STOP, NULL, player_handle->args);
                                    }
                                    /* stop listener */
                                    player_handle->listener_is_running = false;
                                    wait_time = BEKEN_WAIT_FOREVER;
                                    continue;
                                }
                            }
                            break;


                        default:
                            break;
                    }
                }
                else if (event_msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO && event_msg.source == player_handle->spk_dec)
                {
                    audio_element_info_t music_info = {0};
                    audio_element_getinfo(player_handle->spk_dec, &music_info);
                    BK_LOGD(TAG, "[ * ] Receive music info from spk decoder, sample_rates=%d, bits=%d, ch=%d \n", music_info.sample_rates, music_info.bits, music_info.channels);

                    if (player_handle->spk_str)
                    {
                        if (player_handle->spk_type == SPK_TYPE_ONBOARD)
                        {
                            //audio_element_setinfo(player_handle->spk_str, &music_info);
                            onboard_speaker_stream_set_param(player_handle->spk_str, music_info.sample_rates, music_info.bits, music_info.channels);
                        }
                        else if (player_handle->spk_type == SPK_TYPE_UAC)
                        {
                            /* Not support */
                            //TODO
                            BK_LOGD(TAG, "Dynamic switching of UAC speaker format information is not supported \n");
                        }
                    }
                    else
                    {
                        if (player_handle->event_handle)
                        {
                            player_handle->event_handle(PLAYER_EVENT_MUSIC_INFO,  &music_info, player_handle->args);
                        }
                    }
                }
                else
                {
                    BK_LOGW(TAG, "%s, %d, ++>>play pipeline event received, state: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                    //TODO
                }
            }
        }
    }

exit:

    if (player_handle->listener_msg_que)
    {
        rtos_deinit_queue(&player_handle->listener_msg_que);
        player_handle->listener_msg_que = NULL;
    }

    /* delete task */
    player_handle->listener_task_hdl = NULL;

    rtos_set_semaphore(&player_handle->listener_sem);

    rtos_delete_thread(NULL);
}

static bk_err_t listener_init(bk_player_handle_t player_handle)
{
    bk_err_t ret = BK_OK;

    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    ret = rtos_init_semaphore(&player_handle->listener_sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate listener semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&player_handle->listener_msg_que,
                          "player_listener_que",
                          sizeof(listener_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate player listener message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = audio_create_thread(&player_handle->listener_task_hdl,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "player_listener",
                             (beken_thread_function_t)listener_task_main,
                             2048,
                             (beken_thread_arg_t)player_handle,
                             1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create player listener task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&player_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    BK_LOGD(TAG, "init player listener task complete\n");

    return BK_OK;

fail:

    if (player_handle->listener_sem)
    {
        rtos_deinit_semaphore(&player_handle->listener_sem);
        player_handle->listener_sem = NULL;
    }

    if (player_handle->listener_msg_que)
    {
        rtos_deinit_queue(&player_handle->listener_msg_que);
        player_handle->listener_msg_que = NULL;
    }

    player_handle->listener_task_hdl = NULL;

    return BK_FAIL;
}

static bk_err_t listener_deinit(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    if (!player_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != listener_send_msg(player_handle->listener_msg_que, LISTENER_EXIT, NULL))
    {
        return BK_FAIL;
    }

    rtos_get_semaphore(&player_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&player_handle->listener_sem);
    player_handle->listener_sem = NULL;

    BK_LOGD(TAG, "deinit player listener complete\n");

    return BK_OK;
}

static bk_err_t listener_start(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    if (!player_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = listener_send_msg(player_handle->listener_msg_que, LISTENER_START, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t listener_stop(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    if (!player_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = listener_send_msg(player_handle->listener_msg_que, LISTENER_IDLE, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}


static bk_err_t player_config_check(bk_player_cfg_t *cfg)
{
    /* check spk type and spk sample rate config, only support 8000 or 16000 */
    if (cfg->spk_type == SPK_TYPE_ONBOARD)
    {
        if (cfg->spk_cfg.onboard_spk_cfg.bits != 16 || cfg->spk_cfg.onboard_spk_cfg.chl_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, onboard spk dac bits: %d, chl_num: %d is not right\n", __func__, __LINE__, cfg->spk_cfg.onboard_spk_cfg.bits, cfg->spk_cfg.onboard_spk_cfg.chl_num);
            return BK_FAIL;
        }
    }
    else if (cfg->spk_type == SPK_TYPE_UAC)
    {
        if (cfg->spk_cfg.uac_spk_cfg.chl_num != 1)
        {
            BK_LOGE(TAG, "%s, %d, uac spk chl_num: %d is not right\n", __func__, __LINE__, cfg->spk_cfg.uac_spk_cfg.chl_num);
            return BK_FAIL;
        }
    }
    else if (cfg->spk_type == SPK_TYPE_INVALID)
    {
        BK_LOGD(TAG, "%s, %d, cfg->spk_type: %d, not playback\n", __func__, __LINE__, cfg->spk_type);
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, cfg->spk_type: %d not support\n", __func__, __LINE__, cfg->spk_type);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_player_handle_t bk_player_create(bk_player_cfg_t *cfg)
{

    if (BK_OK != player_config_check(cfg))
    {
        BK_LOGE(TAG, "%s, %d, check player config\n", __func__, __LINE__);
        return NULL;
    }

    bk_player_handle_t player_handle = (bk_player_handle_t)psram_malloc(sizeof(struct player));
    PLAYER_CHECK_NULL(player_handle, return NULL);
    os_memset(player_handle, 0, sizeof(struct player));

    /* copy config */
    player_handle->spk_type = cfg->spk_type;
    if (player_handle->spk_type == SPK_TYPE_ONBOARD)
    {
        player_handle->spk_str = onboard_speaker_stream_init(&cfg->spk_cfg.onboard_spk_cfg);
    }
#if CONFIG_ADK_UAC_SPEAKER_STREAM
    else if (player_handle->spk_type == SPK_TYPE_UAC)
    {
        player_handle->spk_str = uac_speaker_stream_init(&cfg->spk_cfg.uac_spk_cfg);
    }
#endif
    else if (player_handle->spk_type == SPK_TYPE_INVALID)
    {
        //nothing todo
    }
    else
    {
        BK_LOGD(TAG, "%s, %d, spk_type: %d is not support \n", __func__, __LINE__, player_handle->spk_type);
        goto fail;
    }

    if (player_handle->spk_type == SPK_TYPE_ONBOARD || player_handle->spk_type == SPK_TYPE_UAC)
    {
        PLAYER_CHECK_NULL(player_handle->spk_str, goto fail);
    }

    player_handle->event_handle = cfg->event_handle;
    player_handle->args = cfg->args;

     /* check whether event_handle was been register.
     * If true, init pipeline listener.
     * If false, not init pipeline listener.
     */
    if (player_handle->event_handle && BK_OK != listener_init(player_handle))
    {
        BK_LOGE(TAG, "%s, %d, player listener init fail\n", __func__, __LINE__);
        goto fail;
    }

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 0, 0);

    player_handle->state = PLAYER_STATE_IDLE;

    return player_handle;

fail:

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    listener_deinit(player_handle);

    if (player_handle->spk_str)
    {
        audio_element_deinit(player_handle->spk_str);
        player_handle->spk_str = NULL;
    }

    os_free(player_handle);

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);

    return NULL;
}

bk_err_t bk_player_set_decode_type(bk_player_handle_t player_handle, audio_dec_type_t dec_type)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    player_handle->dec_type = dec_type;

    return BK_OK;
}

static audio_dec_type_t get_decode_type_by_uri(char *uri)
{
    PLAYER_CHECK_NULL(uri, return AUDIO_DEC_TYPE_INVALID);

    audio_dec_type_t dec_type = AUDIO_DEC_TYPE_INVALID;

    char *ext = strrchr(uri, '.');
    if (!ext)
    {
        BK_LOGD(TAG, "%s, %d, the format of uri: %s is not right\n", __func__, __LINE__, uri);
        return AUDIO_DEC_TYPE_INVALID;
    }

    if (strcasecmp(ext, ".mp3") == 0)
    {
        dec_type = AUDIO_DEC_TYPE_MP3;
    }
    else if (strcasecmp(ext, ".aac") == 0)
    {
        dec_type = AUDIO_DEC_TYPE_AAC;
    }
    else if (strcasecmp(ext, ".wav") == 0)
    {
        dec_type = AUDIO_DEC_TYPE_WAV;
    }
    else if (strcasecmp(ext, ".amr") == 0)
    {
        dec_type = AUDIO_DEC_TYPE_AMR;
    }
    else if (strcasecmp(ext, ".pcm") == 0)
    {
        dec_type = AUDIO_DEC_TYPE_PCM;
    }
    else
    {
        dec_type = AUDIO_DEC_TYPE_INVALID;
    }

    BK_LOGD(TAG, "%s, %d, dec_type: %d \n", __func__, __LINE__, dec_type);

    return dec_type;
}

bk_err_t bk_player_set_uri(bk_player_handle_t player_handle, player_uri_info_t *uri_info)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);
    PLAYER_CHECK_NULL(uri_info, return BK_FAIL);

    bk_err_t ret = BK_OK;

    audio_strm_type_t in_strm_type = AUDIO_STRM_TYPE_INVALID;
    audio_dec_type_t dec_type = AUDIO_DEC_TYPE_INVALID;

    switch (uri_info->uri_type)
    {
        case PLAYER_URI_TYPE_ARRAY:
            in_strm_type = AUDIO_STRM_TYPE_ARRAY;
            break;

        case PLAYER_URI_TYPE_VFS:
            in_strm_type = AUDIO_STRM_TYPE_VFS;
            dec_type = get_decode_type_by_uri(uri_info->uri);
            break;

        case PLAYER_URI_TYPE_URL:
#if 0
            in_stream_type = AUDIO_STREAM_TYPE_URL;
            dec_type = get_decode_type_by_uri(uri_info->uri);
            if (dec_type == AUDIO_DEC_TYPE_INVALID)
            {
                /* get decode type and total size(byte) from http response */
                BK_LOGE(TAG, "%s, %d, uri_type: %d is not support\n", __func__, __LINE__, uri_info->uri_type);
                return BK_FAIL;
            }
#endif
            break;

        default:
            BK_LOGE(TAG, "%s, %d, uri_type: %d is not support\n", __func__, __LINE__, uri_info->uri_type);
            return BK_FAIL;
    }

    if (player_handle->play_pipeline)
    {
        /* Check if the input stream type and decoding types have changed */
        if (in_strm_type == AUDIO_STRM_TYPE_ARRAY)
        {
            if (player_handle->in_strm_type != in_strm_type)
            {
                /* stop play and deinit play pipepline */
                bk_player_stop(player_handle);
                play_pipeline_deinit(player_handle);
            }
            else
            {
                audio_pipeline_reset_port(player_handle->play_pipeline);
                audio_pipeline_reset_elements(player_handle->play_pipeline);
                audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);
            }
        }
        else
        {
            if (player_handle->in_strm_type != in_strm_type || player_handle->dec_type != dec_type)
            {
                /* stop play and deinit play pipepline */
                bk_player_stop(player_handle);
                play_pipeline_deinit(player_handle);
            }
            else
            {
                audio_pipeline_reset_port(player_handle->play_pipeline);
                audio_pipeline_reset_elements(player_handle->play_pipeline);
                audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);
            }
        }
    }

    if (!player_handle->play_pipeline)
    {
        player_handle->in_strm_type = in_strm_type;

        if (in_strm_type == AUDIO_STRM_TYPE_ARRAY)
        {
            if (player_handle->dec_type == AUDIO_DEC_TYPE_INVALID)
            {
                BK_LOGE(TAG, "%s, %d, in_strm_type is array, but dec_type is invalid, please set dec_type by bk_player_set_decode_type \n", __func__, __LINE__);
                return BK_FAIL;
            }
        }
        else
        {
            player_handle->dec_type = dec_type;
        }

        ret = play_pipeline_init(player_handle);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, play_pipeline init fail\n", __func__, __LINE__);
            return BK_FAIL;
        }


    }

    if (in_strm_type == AUDIO_STRM_TYPE_ARRAY)
    {
        array_stream_set_data(player_handle->in_stream, (uint8_t *)uri_info->uri, uri_info->total_len);
    }
    else
    {
        audio_element_set_uri(player_handle->in_stream, uri_info->uri);
    }

    return BK_OK;
}

bk_err_t bk_player_destroy(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (player_handle->state == PLAYER_STATE_PLAYING)
    {
        bk_player_stop(player_handle);
    }

    listener_stop(player_handle);

    play_pipeline_deinit(player_handle);
    BK_LOGD(TAG, "%s, play_pipeline deinit complete\n", __func__);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    listener_deinit(player_handle);

    player_handle->state = PLAYER_STATE_NONE;

    if (player_handle->spk_str)
    {
        audio_element_deinit(player_handle->spk_str);
        player_handle->spk_str = NULL;
    }

    os_free(player_handle);

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);

    return BK_OK;
}

bk_err_t bk_player_start(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    if (player_handle->state == PLAYER_STATE_PLAYING)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    if (player_handle->state == PLAYER_STATE_NONE)
    {
        BK_LOGE(TAG, "%s, %d, player state: %d is error\n", __func__, __LINE__, player_handle->state);
        return BK_FAIL;
    }

    listener_start(player_handle);

    if (BK_OK != play_pipeline_start(player_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline run fail\n", __func__, __LINE__);
        goto fail;
    }

    player_handle->state = PLAYER_STATE_PLAYING;

    return BK_OK;

fail:

    listener_stop(player_handle);

    play_pipeline_stop(player_handle->play_pipeline);

    return BK_FAIL;
}

bk_err_t bk_player_stop(bk_player_handle_t player_handle)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s, STATE: %d\n", __func__, player_handle->state);
    if (player_handle->state == PLAYER_STATE_NONE || player_handle->state == PLAYER_STATE_IDLE || player_handle->state == PLAYER_STATE_STOPED)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    listener_stop(player_handle);

    if (BK_OK != play_pipeline_stop(player_handle->play_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, play_pipeline stop fail\n", __func__, __LINE__);
    }

    audio_pipeline_reset_port(player_handle->play_pipeline);
    audio_pipeline_reset_elements(player_handle->play_pipeline);
    audio_pipeline_change_state(player_handle->play_pipeline, AEL_STATE_INIT);

    player_handle->state = PLAYER_STATE_STOPED;

    return BK_OK;
}

bk_err_t bk_player_get_state(bk_player_handle_t player_handle, bk_player_state_t *state)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);

    *state = player_handle->state;

    return BK_OK;
}

bk_err_t bk_player_set_output_port(bk_player_handle_t player_handle, audio_port_handle_t port)
{
    PLAYER_CHECK_NULL(player_handle, return BK_FAIL);
    //PLAYER_CHECK_NULL(port, return BK_FAIL);

    if (player_handle->spk_str)
    {
        BK_LOGW(TAG, "%s, %d, speaker stream is exist, not need set output port\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (!player_handle->play_pipeline)
    {
        BK_LOGW(TAG, "%s, %d, please set uri first\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (player_handle->dec_type == AUDIO_DEC_TYPE_INVALID)
    {
        audio_element_set_output_port(player_handle->in_stream, port);
    }
    else
    {
        audio_element_set_output_port(player_handle->spk_dec, port);
    }

    return BK_OK;
}
