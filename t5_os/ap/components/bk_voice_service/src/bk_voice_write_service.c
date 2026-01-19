#include <common/bk_include.h>
#include <os/os.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_write_service_types.h>
#include <components/bk_voice_write_service.h>
#include <driver/audio_ring_buff.h>
#include <components/bk_audio/audio_pipeline/framebuf.h>


#define TAG "voc_wt"


#define VOICE_WRITE_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, VOICE_WRITE_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)


typedef enum
{
    VOICE_WRITE_IDLE = 0,
    VOICE_WRITE_START,
    VOICE_WRITE_EXIT
} voice_write_op_t;

typedef struct
{
    voice_write_op_t op;
    void *param;
} voice_write_msg_t;


struct voice_write
{
    voice_handle_t voice_handle;            /**< voice handle */
    int task_stack;                         /**< Task stack size */
    int task_core;                          /**< Task running in core (0 or 1) */
    int task_prio;                          /**< Task priority (based on freeRTOS priority) */
    audio_mem_type_t mem_type;              /**< memory type used, sram, psram or audio heap */
    beken_thread_t voice_write_task_hdl;    /**< voice write task handle */
    beken_queue_t voice_write_msg_que;      /**< message queue of voice write task  */
    beken_semaphore_t sem;                  /**< voice write task semaphore */
    uint32_t write_size;                    /**< the max size of one frame data written to voice */
    uint8_t *write_buff;                    /**< the buffer save data written to voice */
    RingBufferContext pool_rb;              /**< pool ringbuffer handle */
    int8_t *pool_addr;                      /**< pool buffer address */
    uint32_t pool_size;                     /**< the size of pool ringbuffer */
    uint32_t start_threshold;               /**< start threshold of pool ringbuffer, start write data to voice when the data in pool reach the threshold */
    uint32_t pause_threshold;               /**< pause threshold of pool ringbuffer, pause write data to voice when the data in pool reach the threshold */
    bool pool_reach_level;                  /**< the state, whether the start threshold is reached */
    bool running;                           /**< the state, whether voice write task start work */
    audio_buf_type_t  write_buf_type;       /**< write buffer type:ringbuffer or framebuffer */
    framebuf_handle_t pool_fb;              /**< pool framebuffer handle */
};


//#define VOICE_WRITE_DEBUG   //GPIO debug

#ifdef VOICE_WRITE_DEBUG

#define VOICE_WIFI_RX_ISR()                 do { GPIO_DOWN(33); GPIO_UP(33); GPIO_DOWN(33);} while (0)

#define VOICE_WRITE_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define VOICE_WRITE_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define VOICE_WRITE_PROCESS_START()         do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define VOICE_WRITE_PROCESS_END()           do { GPIO_DOWN(35); } while (0)

#define VOICE_WRITE_OUTPUT_START()          do { GPIO_DOWN(36); GPIO_UP(36);} while (0)
#define VOICE_WRITE_OUTPUT_END()            do { GPIO_DOWN(36); } while (0)

#else

#define VOICE_WIFI_RX_ISR()

#define VOICE_WRITE_INPUT_START()
#define VOICE_WRITE_INPUT_END()

#define VOICE_WRITE_PROCESS_START()
#define VOICE_WRITE_PROCESS_END()

#define VOICE_WRITE_OUTPUT_START()
#define VOICE_WRITE_OUTPUT_END()

#endif


#define WIFI_RX_DATA_COUNT

#ifdef WIFI_RX_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t wifi_rx_count_util = {0};
#define WIFI_RX_DATA_COUNT_INTERVAL     (1000 * 4)
#define WIFI_RX_DATA_COUNT_TAG          "WIFI_RX"

#define WIFI_RX_DATA_COUNT_OPEN()               count_util_create(&wifi_rx_count_util, WIFI_RX_DATA_COUNT_INTERVAL, WIFI_RX_DATA_COUNT_TAG)
#define WIFI_RX_DATA_COUNT_CLOSE()              count_util_destroy(&wifi_rx_count_util)
#define WIFI_RX_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&wifi_rx_count_util, size)

#else

#define WIFI_RX_DATA_COUNT_OPEN()
#define WIFI_RX_DATA_COUNT_CLOSE()
#define WIFI_RX_DATA_COUNT_ADD_SIZE(size)

#endif  //WIFI_RX_DATA_COUNT

static bk_err_t voice_write_send_msg(beken_queue_t queue, voice_write_op_t op, void *param)
{
    bk_err_t ret;
    voice_write_msg_t msg;

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

static void voice_write_task_main(beken_thread_arg_t param_data)
{
    bk_err_t ret = BK_OK;
    int read_size = 0;

    voice_write_handle_t voice_write_handle = (voice_write_handle_t)param_data;

    voice_write_handle->running = false;
    long unsigned int wait_time = BEKEN_WAIT_FOREVER;

    rtos_set_semaphore(&voice_write_handle->sem);

    while (1)
    {
        voice_write_msg_t msg;
        ret = rtos_pop_from_queue(&voice_write_handle->voice_write_msg_que, &msg, wait_time);
        if (kNoErr == ret)
        {
            switch (msg.op)
            {
                case VOICE_WRITE_IDLE:
                    voice_write_handle->running = false;
                    wait_time = BEKEN_WAIT_FOREVER;
                    break;

                case VOICE_WRITE_EXIT:
                    goto voice_write_exit;
                    break;

                case VOICE_WRITE_START:
                    voice_write_handle->running = true;
                    wait_time = 0;
                    break;

                default:
                    break;
            }
        }

        /* read speaker data and write to voice service */
        if (voice_write_handle->running)
        {
            VOICE_WRITE_PROCESS_START();
            if(AUDIO_BUF_TYPE_RB == voice_write_handle->write_buf_type)
            {
                uint32_t pool_fill_size = ring_buffer_get_fill_size(&voice_write_handle->pool_rb);
                if (voice_write_handle->pool_reach_level == false)
                {
                    if (pool_fill_size >= voice_write_handle->start_threshold)
                    {
                        voice_write_handle->pool_reach_level = true;
                    }
                }

                if (pool_fill_size <= voice_write_handle->pause_threshold)
                {
                    voice_write_handle->pool_reach_level = false;
                }

                if (voice_write_handle->pool_reach_level)
                {
                    read_size = ring_buffer_read(&voice_write_handle->pool_rb, voice_write_handle->write_buff, voice_write_handle->write_size);
                    if (read_size > 0)
                    {
                        VOICE_WRITE_OUTPUT_START();
                        bk_voice_write_spk_data(voice_write_handle->voice_handle, (char *)voice_write_handle->write_buff, read_size);
                        VOICE_WRITE_OUTPUT_END();
                    }
                }
                else
                {
                    rtos_delay_milliseconds(2);
                }
            }
            else if(AUDIO_BUF_TYPE_FB == voice_write_handle->write_buf_type)
            {
                if (fb_get_ready_node_num(voice_write_handle->pool_fb))
                {
                    framebuf_node_item_t *fb_node_item = NULL;
                    read_size = fb_read(voice_write_handle->pool_fb, &fb_node_item, 0);
                    if (read_size > 0)
                    {
                        if (read_size > voice_write_handle->write_size)
                        {
                            BK_LOGE(TAG, "%s, frame size:%d > buffer len:%d\n", __func__,__LINE__, read_size, voice_write_handle->write_size);
                            goto voice_write_exit;
                        }
                        else
                        {
                            os_memcpy(voice_write_handle->write_buff, fb_node_item->fb_node->buffer, fb_node_item->fb_node->length);
                        }

                        VOICE_WRITE_OUTPUT_START();
                        bk_voice_write_spk_data(voice_write_handle->voice_handle, (char *)voice_write_handle->write_buff, read_size);
                        VOICE_WRITE_OUTPUT_END();
                    }

                    fb_free(voice_write_handle->pool_fb, fb_node_item, 0);
                }
                else
                {
                    rtos_delay_milliseconds(2);
                }
            }
            else
            {
                BK_LOGE(TAG, "%s, %d, write_buf_type:%d is invalid\n", __func__, __LINE__,voice_write_handle->write_buf_type);
                goto voice_write_exit;
            }
            
            VOICE_WRITE_PROCESS_END();
        }
        else
        {
            //rtos_delay_milliseconds(5);
        }
    }

voice_write_exit:

    voice_write_handle->running = false;

    /* delete msg queue */
    ret = rtos_deinit_queue(&voice_write_handle->voice_write_msg_que);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, delete message queue fail\n", __func__, __LINE__);
    }
    voice_write_handle->voice_write_msg_que = NULL;

    /* delete task */
    voice_write_handle->voice_write_task_hdl = NULL;

    rtos_set_semaphore(&voice_write_handle->sem);

    rtos_delete_thread(NULL);
}

voice_write_handle_t bk_voice_write_init(voice_write_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    voice_write_handle_t voice_write_handle = NULL;

    VOICE_WRITE_CHECK_NULL(cfg, return NULL);

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        voice_write_handle = psram_malloc(sizeof(struct voice_write));
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        voice_write_handle = audio_heap_malloc(sizeof(struct voice_write));
    }
#endif
    else
    {
        voice_write_handle = os_malloc(sizeof(struct voice_write));
    }
    VOICE_WRITE_CHECK_NULL(voice_write_handle, return NULL);

    os_memset(voice_write_handle, 0, sizeof(struct voice_write));

    /* copy config */
    voice_write_handle->voice_handle = cfg->voice_handle;

    voice_write_handle->task_stack = cfg->task_stack;
    voice_write_handle->task_core = cfg->task_core;
    voice_write_handle->task_prio = cfg->task_prio;
    voice_write_handle->mem_type = cfg->mem_type;
    voice_write_handle->start_threshold = cfg->start_threshold;
    voice_write_handle->pause_threshold = cfg->pause_threshold;
    voice_write_handle->write_size = 320;
    voice_write_handle->write_buf_type = cfg->write_buf_type;

    /* malloc write buffer */
    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        voice_write_handle->write_buff = (uint8_t *)psram_malloc(voice_write_handle->write_size);
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        voice_write_handle->write_buff = (uint8_t *)audio_heap_malloc(voice_write_handle->write_size);
    }
#endif
    else
    {
        voice_write_handle->write_buff = (uint8_t *)os_malloc(voice_write_handle->write_size);
    }

    VOICE_WRITE_CHECK_NULL(voice_write_handle->write_buff, goto fail);
    os_memset(voice_write_handle->write_buff, 0, voice_write_handle->write_size);

    if(AUDIO_BUF_TYPE_RB == voice_write_handle->write_buf_type)
    {
        voice_write_handle->pool_size = (cfg->node_size*cfg->node_num);
        
        /* malloc pool buffer */
        if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            voice_write_handle->pool_addr = (int8_t *)psram_malloc(voice_write_handle->pool_size);
        }
#if 0
        else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
        {
            voice_write_handle->pool_addr = (int8_t *)audio_heap_malloc(voice_write_handle->pool_size);
        }
#endif
        else
        {
            voice_write_handle->pool_addr = (int8_t *)os_malloc(voice_write_handle->pool_size);
        }
        VOICE_WRITE_CHECK_NULL(voice_write_handle->pool_addr, goto fail);
        os_memset(voice_write_handle->pool_addr, 0, voice_write_handle->pool_size);
        ring_buffer_init(&voice_write_handle->pool_rb, (uint8_t *)voice_write_handle->pool_addr, voice_write_handle->pool_size, DMA_ID_MAX, RB_DMA_TYPE_NULL);
    }
    else if(AUDIO_BUF_TYPE_FB == voice_write_handle->write_buf_type)
    {
        voice_write_handle->pool_fb = fb_create(cfg->node_size, cfg->node_num, 4); 
        VOICE_WRITE_CHECK_NULL(voice_write_handle->pool_fb, goto fail);
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, write_buf_type:%d is invalid\n", __func__, __LINE__,cfg->write_buf_type);
        goto fail;
    }    

    ret = rtos_init_semaphore(&voice_write_handle->sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&voice_write_handle->voice_write_msg_que,
                          "voc_wt_que",
                          sizeof(voice_write_msg_t),
                          5);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate voice write message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_create_thread(&voice_write_handle->voice_write_task_hdl,
                             voice_write_handle->task_prio,
                             "voc_wt",
                             (beken_thread_function_t)voice_write_task_main,
                             voice_write_handle->task_stack,
                             (beken_thread_arg_t)voice_write_handle);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create voice write task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&voice_write_handle->sem, BEKEN_NEVER_TIMEOUT);

    WIFI_RX_DATA_COUNT_OPEN();

    BK_LOGD(TAG, "init voice write task complete\n");

    return voice_write_handle;

fail:

    if (voice_write_handle->sem)
    {
        rtos_deinit_semaphore(&voice_write_handle->sem);
        voice_write_handle->sem = NULL;
    }

    if (voice_write_handle->voice_write_msg_que)
    {
        rtos_deinit_queue(&voice_write_handle->voice_write_msg_que);
        voice_write_handle->voice_write_msg_que = NULL;
    }

    if (voice_write_handle->write_buff)
    {
        if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(voice_write_handle->write_buff);
        }
#if 0
        else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
        {
            audio_heap_free(voice_write_handle->write_buff);
        }
#endif
        else
        {
            os_free(voice_write_handle->write_buff);
        }
    }

    if (voice_write_handle->pool_addr)
    {
        ring_buffer_clear(&voice_write_handle->pool_rb);
        if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(voice_write_handle->pool_addr);
        }
#if 0
        else if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
        {
            audio_heap_free(voice_write_handle->pool_addr);
        }
#endif
        else
        {
            os_free(voice_write_handle->pool_addr);
        }
        voice_write_handle->pool_addr = NULL;
        voice_write_handle->pool_size = 0;
    }

    if (voice_write_handle->pool_fb)
    {
        fb_destroy(voice_write_handle->pool_fb);
        voice_write_handle->pool_fb = NULL;
    }

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_write_handle);
    }
#if 0
    else if (cfg->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_write_handle);
    }
#endif
    else
    {
        os_free(voice_write_handle);
    }

    return NULL;
}

bk_err_t bk_voice_write_deinit(voice_write_handle_t voice_write_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_WRITE_CHECK_NULL(voice_write_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_write_send_msg(voice_write_handle->voice_write_msg_que, VOICE_WRITE_EXIT, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_WRITE_EXIT fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_get_semaphore(&voice_write_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&voice_write_handle->sem);
    voice_write_handle->sem = NULL;

    if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_write_handle->write_buff);
    }
#if 0
    else if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_write_handle->write_buff);
    }
#endif
    else
    {
        os_free(voice_write_handle->write_buff);
    }

    if (voice_write_handle->pool_addr)
    {
        ring_buffer_clear(&voice_write_handle->pool_rb);
        if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(voice_write_handle->pool_addr);
        }
#if 0
        if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
        {
            audio_heap_free(voice_write_handle->pool_addr);
        }
#endif
        else
        {
            os_free(voice_write_handle->pool_addr);
        }
        voice_write_handle->pool_addr = NULL;
        voice_write_handle->pool_size = 0;
    }

    if (voice_write_handle->pool_fb)
    {
        fb_destroy(voice_write_handle->pool_fb);
        voice_write_handle->pool_fb = NULL;
    }

    if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(voice_write_handle);
    }
#if 0
    else if (voice_write_handle->mem_type == AUDIO_MEM_TYPE_AUDIO_HEAP)
    {
        audio_heap_free(voice_write_handle);
    }
#endif
    else
    {
        os_free(voice_write_handle);
    }

    WIFI_RX_DATA_COUNT_CLOSE();

    BK_LOGD(TAG, "deinit voice write complete\n");

    return BK_OK;
}

bk_err_t bk_voice_write_start(voice_write_handle_t voice_write_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_WRITE_CHECK_NULL(voice_write_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_write_send_msg(voice_write_handle->voice_write_msg_que, VOICE_WRITE_START, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_WRITE_START fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_voice_write_stop(voice_write_handle_t voice_write_handle)
{
    bk_err_t ret = BK_OK;

    VOICE_WRITE_CHECK_NULL(voice_write_handle, return BK_FAIL);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = voice_write_send_msg(voice_write_handle->voice_write_msg_que, VOICE_WRITE_IDLE, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: VOICE_WRITE_IDLE fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_voice_write_frame_data(voice_write_handle_t voice_write_handle, char *buffer, uint32_t len)
{
    bk_err_t ret = BK_OK;

    //BK_LOGV(TAG, "%s len: %d\n", __func__, len);

    VOICE_WRITE_CHECK_NULL(voice_write_handle, return BK_FAIL);

    VOICE_WIFI_RX_ISR();

    WIFI_RX_DATA_COUNT_ADD_SIZE(len);

    if(AUDIO_BUF_TYPE_RB == voice_write_handle->write_buf_type)
    {
        uint32_t pool_free_size = ring_buffer_get_free_size(&voice_write_handle->pool_rb);
        if (pool_free_size > len)
        {
            VOICE_WRITE_INPUT_START();
            uint32_t write_size = ring_buffer_write(&voice_write_handle->pool_rb, (uint8_t *)buffer, len);
            VOICE_WRITE_INPUT_END();
            if (write_size != len)
            {
                BK_LOGV(TAG, "%s, %d, write_size: %d, len: %d\n", __func__, __LINE__, write_size, len);
            }
            ret = write_size;
        }
        else
        {
            /* If pool is enough, throw away the all data to avoid the incomplete frame data being written to voice. */
            ret = 0;
        }
    }
    else if(AUDIO_BUF_TYPE_FB == voice_write_handle->write_buf_type)
    {
        int node_size = fb_get_node_size(voice_write_handle->pool_fb);
        int node_total_num = fb_get_total_node_num(voice_write_handle->pool_fb);
        int node_ready_num = fb_get_ready_node_num(voice_write_handle->pool_fb);
        if((node_total_num > node_ready_num) && (node_size >= len))
        {
            framebuf_node_item_t *fb_node_item = NULL;
            int ret = fb_malloc(voice_write_handle->pool_fb, &fb_node_item, 40/portTICK_RATE_MS);
            if (ret > 0)
            {
                os_memcpy(fb_node_item->fb_node->buffer, buffer, len);
                fb_node_item->fb_node->length = len;
                ret = fb_write(voice_write_handle->pool_fb, fb_node_item, 0);
            }
        }
        else
        {
            BK_LOGE(TAG, "%s, %d, FB node size:%d,len:%d,total node:%d,ready node:%d\n",
                    __func__, __LINE__, node_size, len, node_total_num, node_ready_num);
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, %d, write_buf_type:%d is invalid!\n",
                    __func__, __LINE__, voice_write_handle->write_buf_type);
    }

    return ret;
}

