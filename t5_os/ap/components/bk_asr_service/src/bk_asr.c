#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"

#include <components/avdk_types.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <driver/pwr_clk.h>
#include <driver/audio_ring_buff.h>

#include <modules/pm.h>

#define TAG "asr"

#if 0
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#endif

#define ASR_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, ASR_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)

typedef enum
{
    ASR_LISTENER_IDLE = 0,
    ASR_LISTENER_START,
    ASR_LISTENER_EXIT
} asr_listener_op_t;

typedef struct
{
    asr_listener_op_t op;
    void *param;
} asr_listener_msg_t;


static bk_err_t asr_pipeline_deinit(asr_handle_t asr_handle)
{
    BK_LOGD(TAG, "%s\n", __func__);
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    if (!asr_handle->asr_pipeline)
    {
        return BK_OK;
    }

    if (BK_OK != audio_pipeline_terminate(asr_handle->asr_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline terminate fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (asr_handle->asr_raw_read && BK_OK != audio_pipeline_unregister(asr_handle->asr_pipeline, asr_handle->asr_raw_read))
    {
        BK_LOGE(TAG, "%s, %d, unregister asr_raw_read staream fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (asr_handle->asr_evt)
    {
        /* deinit listener */
        if (BK_OK != audio_pipeline_remove_listener(asr_handle->asr_pipeline))
        {
            BK_LOGE(TAG, "%s, %d, remove asr pipeline fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        if (BK_OK != audio_event_iface_destroy(asr_handle->asr_evt))
        {
            BK_LOGE(TAG, "%s, %d, destroy asr pipeline fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    if (BK_OK != audio_pipeline_deinit(asr_handle->asr_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        asr_handle->asr_pipeline = NULL;
    }

    if (asr_handle->asr_raw_read && BK_OK != audio_element_deinit(asr_handle->asr_raw_read))
    {
        BK_LOGE(TAG, "%s, %d, asr raw_read staream deinit fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        asr_handle->asr_raw_read = NULL;
    }
    return BK_OK;
}

static bk_err_t asr_pipeline_init(asr_handle_t asr_handle, asr_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    BK_LOGD(TAG, "%s\n", __func__);

    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    BK_LOGD(TAG, "step1: asr pipeline init\n");
    audio_pipeline_cfg_t asr_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    asr_pipeline_cfg.rb_size = 320;
    asr_handle->asr_pipeline = audio_pipeline_init(&asr_pipeline_cfg);
    ASR_CHECK_NULL(asr_handle->asr_pipeline, goto fail);

    BK_LOGD(TAG, "step2: init asr elements\n");
	asr_handle->asr_rsp = rsp_algorithm_init(&cfg->rsp_cfg.rsp_alg_cfg);
	ASR_CHECK_NULL(asr_handle->asr_rsp, goto fail);

    raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_read_cfg.type = AUDIO_STREAM_READER;
    raw_read_cfg.out_block_size = cfg->read_pool_size * 2 *2;
    raw_read_cfg.out_block_num = 1;
    asr_handle->asr_raw_read = raw_stream_init(&raw_read_cfg);
    ASR_CHECK_NULL(asr_handle->asr_raw_read, goto fail);

    BK_LOGD(TAG, "step3: asr pipeline register\n");
    if (BK_OK != audio_pipeline_register(asr_handle->asr_pipeline, asr_handle->asr_rsp, "asr_rsp"))
    {
        BK_LOGE(TAG, "%s, %d, register asr_rsp fail\n", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != audio_pipeline_register(asr_handle->asr_pipeline, asr_handle->asr_raw_read, "asr_raw_read"))
    {
        BK_LOGE(TAG, "%s, %d, register asr_raw_read stream fail\n", __func__, __LINE__);
        goto fail;
    }

    BK_LOGD(TAG, "step4: asr pipeline link\n");
    /* pipeline record */
    ret = audio_pipeline_link(asr_handle->asr_pipeline, (const char *[])
    {"asr_rsp", "asr_raw_read"
    }, 2);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline link fail\n", __func__, __LINE__);
        goto fail;
    }

    if (asr_handle->event_handle)
    {
        BK_LOGD(TAG, "step5: init asr event listener\n");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        asr_handle->asr_evt = audio_event_iface_init(&evt_cfg);
        if (asr_handle->asr_evt == NULL)
        {
            BK_LOGE(TAG, "%s, %d, asr_event init fail\n", __func__, __LINE__);
            goto fail;
        }

        if (BK_OK != audio_pipeline_set_listener(asr_handle->asr_pipeline, asr_handle->asr_evt))
        {
            BK_LOGE(TAG, "%s, %d, init asr pipeline listener fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }
    return BK_OK;
fail:
    asr_pipeline_deinit(asr_handle);
    return BK_FAIL;
}

static bk_err_t asr_pipeline_start(audio_pipeline_handle_t asr_pipeline)
{
    ASR_CHECK_NULL(asr_pipeline, return BK_FAIL);

    if (BK_OK != audio_pipeline_run(asr_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline run fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t asr_pipeline_stop(audio_pipeline_handle_t asr_pipeline)
{
    ASR_CHECK_NULL(asr_pipeline, return BK_FAIL);
    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != audio_pipeline_stop(asr_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(asr_pipeline))
    {
        BK_LOGE(TAG, "%s, %d, asr_pipeline wait stop fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t asr_listener_send_msg(beken_queue_t queue, asr_listener_op_t op, void *param)
{
    bk_err_t ret;
    asr_listener_msg_t msg;

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

static void asr_listener_task_main(beken_thread_arg_t param_data)
{
    asr_handle_t asr_handle = (asr_handle_t)param_data;
    bk_err_t ret = BK_OK;

    asr_handle->listener_is_running = false;
    long unsigned int wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&asr_handle->listener_sem);

    while (1)
    {
        asr_listener_msg_t listener_msg;
        ret = rtos_pop_from_queue(&asr_handle->listener_msg_que, &listener_msg, wait_time);
        if (kNoErr == ret)
        {
            switch (listener_msg.op)
            {
                case ASR_LISTENER_IDLE:
                    asr_handle->listener_is_running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;
                case ASR_LISTENER_EXIT:
                    goto exit;
                    break;

                case ASR_LISTENER_START:
                    asr_handle->listener_is_running = true;
                    wait_time = 0;
                    break;
                default:
                    break;
            }
        }

        audio_event_iface_msg_t event_msg;
        audio_element_status_t el_status = AEL_STATUS_NONE;
        if (asr_handle->listener_is_running)
        {
            ret = audio_event_iface_listen(asr_handle->asr_evt, &event_msg, 20 / portTICK_RATE_MS);//portMAX_DELAY
            if (ret == BK_OK)
            {
                if (event_msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && event_msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
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
                            if (asr_handle->status == ASR_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>>record pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop asr pipeline */
                                bk_asr_stop(asr_handle);
                            #if 0
                                if ((audio_element_handle_t)event_msg.source == asr_handle->mic_str)
                                {
                                    bk_asr_event_handle(asr_handle->event_handle, ASR_EVT_MIC_NOT_SUPPORT, NULL, asr_handle->args);
                                }
                                else
                                {
                                    bk_asr_event_handle(asr_handle->event_handle, ASR_EVT_ERROR_UNKNOW, NULL, asr_handle->args);
                                }
                            #endif
                                /* stop listener */
                                asr_handle->listener_is_running = false;
                                wait_time = BEKEN_WAIT_FOREVER;
                                continue;
                            }
                            break;

                        case AEL_STATUS_STATE_STOPPED:
                        case AEL_STATUS_STATE_FINISHED:
                            if (asr_handle->status == ASR_STA_RUNNING)
                            {
                                BK_LOGW(TAG, "%s, %d, ++>>asr pipeline event received, status: %d, ele: %p\n", __func__, __LINE__, (int)event_msg.data, event_msg.source);
                                /* stop asr pipeline */
                                bk_asr_stop(asr_handle);
                                bk_asr_event_handle(asr_handle->event_handle, ASR_EVT_STOP, NULL, asr_handle->args);

                                /* stop listener */
                                asr_handle->listener_is_running = false;
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

    if (asr_handle->listener_msg_que)
    {
        rtos_deinit_queue(&asr_handle->listener_msg_que);
        asr_handle->listener_msg_que = NULL;
    }

    /* delete task */
    asr_handle->listener_task_hdl = NULL;

    rtos_set_semaphore(&asr_handle->listener_sem);

    rtos_delete_thread(NULL);
}

static bk_err_t asr_listener_init(asr_handle_t asr_handle)
{
    bk_err_t ret = BK_OK;

    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    ret = rtos_init_semaphore(&asr_handle->listener_sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate listener semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&asr_handle->listener_msg_que,
                          "asr_listener_que",
                          sizeof(asr_listener_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate asr listener message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = audio_create_thread(&asr_handle->listener_task_hdl,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "asr_listener",
                             (beken_thread_function_t)asr_listener_task_main,
                             1024,
                             (beken_thread_arg_t)asr_handle,
                             1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create asr listener task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&asr_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    BK_LOGD(TAG, "init asr listener task complete\n");

    return BK_OK;

fail:

    if (asr_handle->listener_sem)
    {
        rtos_deinit_semaphore(&asr_handle->listener_sem);
        asr_handle->listener_sem = NULL;
    }

    if (asr_handle->listener_msg_que)
    {
        rtos_deinit_queue(&asr_handle->listener_msg_que);
        asr_handle->listener_msg_que = NULL;
    }

    asr_handle->listener_task_hdl = NULL;

    return BK_FAIL;
}

static bk_err_t asr_listener_deinit(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    if (!asr_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    if (BK_OK != asr_listener_send_msg(asr_handle->listener_msg_que, ASR_LISTENER_EXIT, NULL))
    {
        return BK_FAIL;
    }

    rtos_get_semaphore(&asr_handle->listener_sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&asr_handle->listener_sem);
    asr_handle->listener_sem = NULL;

    BK_LOGD(TAG, "deinit asr listener complete\n");

    return BK_OK;
}

static bk_err_t asr_listener_start(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    if (!asr_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = asr_listener_send_msg(asr_handle->listener_msg_que, ASR_LISTENER_START, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t asr_listener_stop(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    if (!asr_handle->event_handle)
    {
        return BK_OK;
    }

    BK_LOGD(TAG, "%s\n", __func__);

    bk_err_t ret = asr_listener_send_msg(asr_handle->listener_msg_que, ASR_LISTENER_IDLE, NULL);
    if (ret != BK_OK)
    {
        return BK_FAIL;
    }

    return BK_OK;
}


static bk_err_t asr_config_check(asr_cfg_t cfg)
{
    return BK_OK;
}

asr_handle_t bk_asr_create(asr_cfg_t *cfg)
{
	if (BK_OK != asr_config_check(*cfg))
	{
		BK_LOGE(TAG, "%s, %d, check asr config\n", __func__, __LINE__);
		return NULL;
	}

#if (CONFIG_ASR_SERVICE_USE_PSRAM)
	asr_handle_t asr_handle = (asr_handle_t)psram_malloc(sizeof(struct asr));
#elif (CONFIG_ASR_SERVICE_USE_AUDIO_HEAP)
	asr_handle_t asr_handle = (asr_handle_t)audio_heap_malloc(sizeof(struct asr));
#else
	asr_handle_t asr_handle = (asr_handle_t)os_malloc(sizeof(struct asr));
#endif

	ASR_CHECK_NULL(asr_handle, return NULL);
	os_memset(asr_handle, 0x00, sizeof(struct asr));
	asr_handle->status = ASR_STA_IDLE;
	return asr_handle;
}


bk_err_t bk_asr_init(asr_cfg_t *cfg, asr_handle_t asr_handle)
{
    if (BK_OK != asr_config_check(*cfg))
    {
        BK_LOGE(TAG, "%s, %d, check asr config\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    /* copy config */
    asr_handle->asr_en     = cfg->asr_en;
    asr_handle->asr_rsp_en = cfg->asr_rsp_en;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 0, 0);

	if (cfg->asr_rsp_en)
	{
	    if (BK_OK != asr_pipeline_init(asr_handle, cfg))
	    {
	        BK_LOGE(TAG, "%s, %d, asr_pipeline_open fail\n", __func__, __LINE__);
	        goto fail;
	    }
	}

	if (asr_handle->asr_en)
	{
		/* frame_size = sample_rate * 20ms / 1000 * bits */
		ringbuf_port_cfg_t rb_config = {2048*2};//{320*2};//{cfg->aec_cfg.aec_alg_cfg.aec_cfg.fs * 20 / 1000 * 2 * 2};
		asr_handle->asr_in_rb = ringbuf_port_init(&rb_config);
		ASR_CHECK_NULL(asr_handle->asr_in_rb, goto fail);

		if (!asr_handle->asr_rsp_en)
		{
			raw_stream_cfg_t raw_asr_cfg = RAW_STREAM_CFG_DEFAULT();
			raw_asr_cfg.type            = AUDIO_STREAM_READER;
			raw_asr_cfg.out_block_size  = cfg->read_pool_size; //960;
			raw_asr_cfg.out_block_num   = 2;
			asr_handle->asr_raw_read = raw_stream_init(&raw_asr_cfg);
			ASR_CHECK_NULL(asr_handle->asr_raw_read, goto fail);
			if (BK_OK !=  audio_element_set_input_port(asr_handle->asr_raw_read, asr_handle->asr_in_rb))
			{
				BK_LOGE(TAG, "%s, %d, link asr_raw_read to asr_in_rb fail\n", __func__, __LINE__);
				goto fail;
			}
		//	audio_element_set_input_timeout(asr_handle->asr_raw_read, 10);
			audio_element_run(asr_handle->asr_raw_read);
		} else
		{
			if (BK_OK !=  audio_element_set_input_port(asr_handle->asr_rsp, asr_handle->asr_in_rb))
			{
				BK_LOGE(TAG, "%s, %d, link asr_raw_read to asr_in_rb fail\n", __func__, __LINE__);
				goto fail;
			}
		}

	//	if (asr_handle->aec_en) 
	//	{
	//		if (BK_OK != audio_element_set_multi_output_port(asr_handle->aec_alg, asr_handle->asr_in_rb, 0))
	//		{
	//			BK_LOGE(TAG, "%s, %d, link aec_alg_out to asr_in_rb fail\n", __func__, __LINE__);
	//			goto fail;
	//		}
	//	} else
		{
			if (BK_OK !=  audio_element_set_multi_output_port(asr_handle->mic_str, asr_handle->asr_in_rb, 0))
			{
				BK_LOGE(TAG, "%s, %d, link mic_str to asr_in_rb fail\n", __func__, __LINE__);
				goto fail;
			}
		}
	}

    /* check whether event_handle was been register.
     * If true, init pipeline listener.
     * If false, not init pipeline listener.
     */
    if (asr_handle->event_handle && BK_OK != asr_listener_init(asr_handle))
    {
        BK_LOGE(TAG, "%s, %d, asr listener init fail\n", __func__, __LINE__);
        goto fail;
    }

    asr_handle->status = ASR_STA_IDLE;

    return BK_OK;

fail:

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    asr_listener_deinit(asr_handle);

#if (CONFIG_ASR_SERVICE_USE_PSRAM)
	psram_free(asr_handle);
#elif (CONFIG_ASR_SERVICE_USE_AUDIO_HEAP)
	audio_heap_free(asr_handle);
#else
	os_free(asr_handle);
#endif

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);
    return BK_FAIL;
}

bk_err_t bk_asr_deinit(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (asr_handle->status == ASR_STA_RUNNING)
    {
        bk_asr_stop(asr_handle);
    }

    asr_listener_stop(asr_handle);

    asr_pipeline_deinit(asr_handle);
    BK_LOGD(TAG, "%s, asr_pipeline deinit complete\n", __func__);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    asr_listener_deinit(asr_handle);

    asr_handle->status = ASR_STA_NONE;

#if (CONFIG_ASR_SERVICE_USE_PSRAM)
    psram_free(asr_handle);
#elif (CONFIG_ASR_SERVICE_USE_AUDIO_HEAP)
    audio_heap_free(asr_handle);
#else
    os_free(asr_handle);
#endif

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);
    return BK_OK;
}

bk_err_t bk_asr_start(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    if (asr_handle->status == ASR_STA_RUNNING)
    {
        return BK_OK;
    }

    if (asr_handle->status != ASR_STA_IDLE && asr_handle->status != ASR_STA_STOPED)
    {
        BK_LOGE(TAG, "%s, %d, asr status: %d is error\n", __func__, __LINE__, asr_handle->status);
        return BK_FAIL;
    }

    asr_listener_start(asr_handle);

	if (asr_handle->asr_rsp_en)
	{
	    if (BK_OK != asr_pipeline_start(asr_handle->asr_pipeline))
	    {
	        BK_LOGE(TAG, "%s, %d, asr_pipeline run fail\n", __func__, __LINE__);
	        goto fail;
	    }
	}

    asr_handle->status = ASR_STA_RUNNING;
    return BK_OK;

fail:

    asr_listener_stop(asr_handle);

    if (asr_handle->asr_pipeline)
    {
        asr_pipeline_stop(asr_handle->asr_pipeline);
    }
    return BK_FAIL;
}

bk_err_t bk_asr_stop(asr_handle_t asr_handle)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);
    BK_LOGD(TAG, "%s\n", __func__);

    if (asr_handle->status == ASR_STA_IDLE || asr_handle->status == ASR_STA_STOPED)
    {
        return BK_OK;
    }

    if (asr_handle->status != ASR_STA_RUNNING)
    {
        BK_LOGE(TAG, "%s, %d, asr status: %d is error\n", __func__, __LINE__, asr_handle->status);
        return BK_FAIL;
    }

    asr_handle->status = ASR_STA_STOPPING;

    asr_listener_stop(asr_handle);

    if (asr_handle->asr_pipeline) {
        if (BK_OK != asr_pipeline_stop(asr_handle->asr_pipeline))
        {
            BK_LOGE(TAG, "%s, %d, asr_pipeline stop fail\n", __func__, __LINE__);
        }
    }

    asr_handle->status = ASR_STA_STOPED;
    return BK_OK;
}


int bk_aud_asr_read_mic_data(asr_handle_t asr_handle, char *buffer, uint32_t size)
{
	ASR_CHECK_NULL(asr_handle, return BK_FAIL);
	if (!buffer || size == 0)
	{
		BK_LOGE(TAG, "%s, %d, buffer: %p, size: %d\n", __func__, __LINE__, buffer, size);
		return BK_FAIL;
	}
	return raw_stream_read(asr_handle->asr_raw_read, buffer, size);
}

int bk_aud_asr_get_size(asr_handle_t asr_handle)
{
	ASR_CHECK_NULL(asr_handle, return BK_FAIL);
	if (!asr_handle)
	{
		BK_LOGE(TAG, "%s, %d\n", __func__, __LINE__);
		return BK_FAIL;
	}
	audio_port_handle_t port = audio_element_get_input_port(asr_handle->asr_raw_read);
	return audio_port_get_size(port);
}

int bk_aud_asr_get_filled_size(asr_handle_t asr_handle)
{
	ASR_CHECK_NULL(asr_handle, return BK_FAIL);
	if (!asr_handle)
	{
		BK_LOGE(TAG, "%s, %d\n", __func__, __LINE__);
		return BK_FAIL;
	}
	audio_port_handle_t port = audio_element_get_input_port(asr_handle->asr_raw_read);
	return audio_port_get_filled_size(port);
}

bk_err_t bk_asr_get_status(asr_handle_t asr_handle, asr_sta_t *status)
{
    ASR_CHECK_NULL(asr_handle, return BK_FAIL);
    *status = asr_handle->status;
    return BK_OK;
}

/* used for amp system, not smp system */
#if (CONFIG_SOC_SMP)
bk_err_t bk_asr_event_handle(asr_event_handle event_handle, asr_evt_t event, void *param, void *args)
{
    ASR_CHECK_NULL(event_handle, return BK_FAIL);
    return event_handle(event, param, args);
}
#endif

