#include <common/bk_include.h>
#include <os/os.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_read_service.h>


#define TAG "voc_rd"

/*
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
*/

#define VOICE_READ_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, VOICE_READ_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)

#if 0
typedef enum
{
    VOICE_READ_STA_NONE = 0,
    VOICE_READ_STA_IDLE,
    VOICE_READ_STA_RUNNING,
    VOICE_READ_STA_STOPED,
} voice_read_sta_t;
#endif

typedef enum
{
    VOICE_READ_IDLE = 0,
    VOICE_READ_START,
    VOICE_READ_EXIT
} voice_read_op_t;

typedef struct
{
    voice_read_op_t op;
    void *param;
} voice_read_msg_t;


struct voice_read
{
    voice_handle_t voice_handle;                                                    /**< voice handle */
    uint32_t max_read_size;                                                         /**< the max size of data read from voice handle, used in voice_read_callback */
    int (*voice_read_callback)(unsigned char *data, unsigned int len, void *args);  /**< call this callback when avlid data has been read */
    void *args;                                                                     /**< the pravate parameter of callback */
    int task_stack;                                                                 /**< Task stack size */
    int task_core;                                                                  /**< Task running in core (0 or 1) */
    int task_prio;                                                                  /**< Task priority (based on freeRTOS priority) */
    audio_mem_type_t mem_type;                                                      /**< memory type used, sram, psram, audio_heap */
    beken_thread_t voice_read_task_hdl;
    beken_queue_t voice_read_msg_que;
    beken_semaphore_t sem;
    uint8_t *read_buff;
    bool running;
    //voice_read_sta_t status;
};

//#define WIFI_TX_GPIO_DEBUG

#ifdef WIFI_TX_GPIO_DEBUG
#define WIFI_TX_MIC_DATA_START()           do { GPIO_DOWN(9); GPIO_UP(9);} while (0)
#define WIFI_TX_MIC_DATA_END()             do { GPIO_DOWN(9); } while (0)
#else
#define WIFI_TX_MIC_DATA_START()
#define WIFI_TX_MIC_DATA_END()
#endif


#if CONFIG_ADK_UTILS
#define WIFI_TX_DATA_COUNT
#endif // CONFIG_ADK_UTILS

#ifdef WIFI_TX_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t wifi_tx_count_util = {0};
#define WIFI_TX_DATA_COUNT_INTERVAL     (1000 * 4)
#define WIFI_TX_DATA_COUNT_TAG          "WIFI_TX"

#define WIFI_TX_DATA_COUNT_OPEN()               count_util_create(&wifi_tx_count_util, WIFI_TX_DATA_COUNT_INTERVAL, WIFI_TX_DATA_COUNT_TAG)
#define WIFI_TX_DATA_COUNT_CLOSE()              count_util_destroy(&wifi_tx_count_util)
#define WIFI_TX_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&wifi_tx_count_util, size)

#else

#define WIFI_TX_DATA_COUNT_OPEN()
#define WIFI_TX_DATA_COUNT_CLOSE()
#define WIFI_TX_DATA_COUNT_ADD_SIZE(size)

#endif  //WIFI_TX_DATA_COUNT

static bk_err_t voice_read_send_msg(beken_queue_t queue, voice_read_op_t op, void *param)
{
    bk_err_t ret;
    voice_read_msg_t msg;

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
        BK_LOGE(TAG, "%s, %d, send message: %d fail, ret: %d\n", __func__, __LINE__, op, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

static void voice_read_task_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;
    int read_size = 0;

    voice_read_handle_t voice_read_handle = (voice_read_handle_t)param_data;

    //voice_read_handle->status = VOICE_READ_STA_IDLE;
    voice_read_handle->running = false;
    long unsigned int wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&voice_read_handle->sem);

    while (1)
    {
        voice_read_msg_t msg;
        ret = rtos_pop_from_queue(&voice_read_handle->voice_read_msg_que, &msg, wait_time);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case VOICE_READ_IDLE:
                    //voice_read_handle->status = VOICE_READ_STA_IDLE;
                    voice_read_handle->running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case VOICE_READ_EXIT:
                    goto voice_read_exit;
                    break;

                case VOICE_READ_START:
                    //voice_read_handle->status = VOICE_READ_STA_RUNNING;
                    voice_read_handle->running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        /* read mic data and send */
        if (voice_read_handle->running)
        {
            read_size = bk_voice_read_mic_data(voice_read_handle->voice_handle, (char *)voice_read_handle->read_buff, voice_read_handle->max_read_size);
            if (read_size > 0 && voice_read_handle->voice_read_callback)
            {
                WIFI_TX_MIC_DATA_START();
                read_size = voice_read_handle->voice_read_callback(voice_read_handle->read_buff, read_size, voice_read_handle->args);
                WIFI_TX_MIC_DATA_END();
                WIFI_TX_DATA_COUNT_ADD_SIZE(read_size);
            }
            else
            {
                BK_LOGV(TAG, "%s, %d, read voice data fail, read_size: %d \n", __func__, __LINE__, read_size);
            }
        }
    }

voice_read_exit:

    voice_read_handle->running = false;

    /* delete msg queue */
    ret = rtos_deinit_queue(&voice_read_handle->voice_read_msg_que);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, delete message queue fail\n", __func__, __LINE__);
    }
    voice_read_handle->voice_read_msg_que = NULL;

    /* delete task */
    voice_read_handle->voice_read_task_hdl = NULL;

    rtos_set_semaphore(&voice_read_handle->sem);

    rtos_delete_thread(NULL);
}

voice_read_handle_t bk_voice_read_init(voice_read_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    voice_read_handle_t voice_read_handle = NULL;

    VOICE_READ_CHECK_NULL(cfg, return NULL);

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        voice_read_handle = psram_malloc(sizeof(struct voice_read));
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        voice_read_handle = audio_heap_malloc(sizeof(struct voice_read));
    }
#endif
    else
    {
        voice_read_handle = os_malloc(sizeof(struct voice_read));
    }
    VOICE_READ_CHECK_NULL(voice_read_handle, return NULL);

    os_memset(voice_read_handle, 0, sizeof(struct voice_read));

    /* copy config */
    voice_read_handle->voice_handle = cfg->voice_handle;
    voice_read_handle->max_read_size = cfg->max_read_size;
    voice_read_handle->voice_read_callback = cfg->voice_read_callback;
    voice_read_handle->args = cfg->args;
    voice_read_handle->task_stack = cfg->task_stack;
    voice_read_handle->task_core = cfg->task_core;
    voice_read_handle->task_prio = cfg->task_prio;
    voice_read_handle->mem_type = cfg->mem_type;

    /* malloc read buffer */
    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        voice_read_handle->read_buff = psram_malloc(voice_read_handle->max_read_size);
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        voice_read_handle->read_buff = audio_heap_malloc(voice_read_handle->max_read_size);
    }
#endif
    else
    {
        voice_read_handle->read_buff = os_malloc(voice_read_handle->max_read_size);
    }
    VOICE_READ_CHECK_NULL(voice_read_handle->read_buff, goto fail);

    os_memset(voice_read_handle->read_buff, 0, voice_read_handle->max_read_size);

    ret = rtos_init_semaphore(&voice_read_handle->sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&voice_read_handle->voice_read_msg_que,
                          "voc_rd_que",
                          sizeof(voice_read_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate voice read message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_create_thread(&voice_read_handle->voice_read_task_hdl,
                             voice_read_handle->task_prio,
                             "voc_rd",
                             (beken_thread_function_t)voice_read_task_main,
                             voice_read_handle->task_stack,
                             (beken_thread_arg_t)voice_read_handle);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create voice read task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&voice_read_handle->sem, BEKEN_NEVER_TIMEOUT);

    WIFI_TX_DATA_COUNT_OPEN();

    BK_LOGD(TAG, "init voice read task complete\n");

    return voice_read_handle;

fail:

    if (voice_read_handle->sem)
    {
        rtos_deinit_semaphore(&voice_read_handle->sem);
        voice_read_handle->sem = NULL;
    }

    if (voice_read_handle->voice_read_msg_que)
    {
        rtos_deinit_queue(&voice_read_handle->voice_read_msg_que);
        voice_read_handle->voice_read_msg_que = NULL;
    }

    if (voice_read_handle->read_buff)
    {
        if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(voice_read_handle->read_buff);
        }
#if 0
        else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
        {
            audio_heap_free(voice_read_handle->read_buff);
        }
#endif
        else
        {
            os_free(voice_read_handle->read_buff);
        }
    }

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_read_handle);
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_read_handle);
    }
#endif
    else
    {
        os_free(voice_read_handle);
    }

    return NULL;
}

bk_err_t bk_voice_read_deinit(voice_read_handle_t voice_read_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_READ_CHECK_NULL(voice_read_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_read_send_msg(voice_read_handle->voice_read_msg_que, VOICE_READ_EXIT, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_READ_EXIT fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_get_semaphore(&voice_read_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&voice_read_handle->sem);
    voice_read_handle->sem = NULL;

    if (voice_read_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_read_handle->read_buff);
    }
#if 0
    else if (voice_read_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_read_handle->read_buff);
    }
#endif
    else
    {
        os_free(voice_read_handle->read_buff);
    }

    if (voice_read_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_read_handle);
    }
#if 0
    else if (voice_read_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_read_handle);
    }
#endif
    else
    {
        os_free(voice_read_handle);
    }

    WIFI_TX_DATA_COUNT_CLOSE();

    BK_LOGD(TAG, "deinit voice read complete\n");

    return BK_OK;
}

bk_err_t bk_voice_read_start(voice_read_handle_t voice_read_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_READ_CHECK_NULL(voice_read_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_read_send_msg(voice_read_handle->voice_read_msg_que, VOICE_READ_START, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_READ_START fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_voice_read_stop(voice_read_handle_t voice_read_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_READ_CHECK_NULL(voice_read_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_read_send_msg(voice_read_handle->voice_read_msg_que, VOICE_READ_IDLE, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_READ_IDLE fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

