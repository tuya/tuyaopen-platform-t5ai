/**
 * @file tkl_asr.c
 * @version 0.1
 * @date 2025-04-08
 */

#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_semaphore.h"
#include "tuya_ringbuf.h"

#include "tkl_asr.h"

#include "tutuClear.h"
#include <os/mem.h>


/***********************************************************
************************macro define************************
***********************************************************/
#define __EN_EXTERNALLY_ALLOCATION__ // enable this define for externally malloc TUTU objects
#define __EN_DIAGNOSTICS__ //enable this to have the proper internal returned diagnostical information

#define TKL_ASR_ONE_FRAME       (320 * 2)                            //! 20ms
#define TKL_ASR_VAD_FRAME       ((TKL_ASR_ONE_FRAME * 50))        //! 1000ms
#define TKL_ASR_BUFSZ           ((TKL_ASR_ONE_FRAME * 40) * 2)    //! 1000ms * 2 = 2s
#define TKL_ASR_DETECT_FRAME    (TKL_ASR_ONE_FRAME * 5)           //! 100ms

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    TKL_ASR_KWS_ENABLE_CMD = 0,
    TKL_ASR_KWS_DISABLE_CMD,
    TKL_ASR_KWS_VAD_DETECT_CMD
}TKL_ASR_CMD_E;

typedef struct {
    bool                      init;
    bool                      enable;
    bool                      thread_running;
    uint8_t                   vadflag;
    TUYA_RINGBUFF_T           rb;
    TKL_MUTEX_HANDLE          mutex;
    TKL_SEM_HANDLE            sem;
    TKL_THREAD_HANDLE         thread;
    uint32_t                  bufsz;
    uint8_t                  *oneframe;
    uint8_t                  *buffer;
} TKL_ASR_CTRL_T;

typedef struct {
    W32   w32WakeWord;
    char *asr_txt;
    TKL_ASR_WAKEUP_WORD_E wakeup_word;
} ASR_WAKEUP_WORD_MAP_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_ASR_CTRL_T sg_asr_mgr = {0};
static TKL_ASR_WAKEUP_CB sg_wakeup_cb = NULL;

static void *sg_tutuclear_obj = NULL;
static void *pExternallyAllocatedMem = NULL;

static ASR_WAKEUP_WORD_MAP_T cASR_WAKEUP_WORD_MAP[] = {
    {1, "你好涂鸦",    TKL_ASR_WAKEUP_NIHAO_TUYA},
    {2, "小智同学",    TKL_ASR_WAKEUP_XIAOZHI_TONGXUE},
    {3, "heytuya",    TKL_ASR_WAKEUP_HEY_TUYA},
    {4, "smartlife",  TKL_ASR_WAKEUP_SMARTLIFE},
};

/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __asr_kws_create(void)
{
  #ifdef __EN_EXTERNALLY_ALLOCATION__
	int i = TUTUClear_QueryMemSz();
    pExternallyAllocatedMem = psram_malloc(i);
	bk_printf("tutuClear PSTAM DM usage = %d bytes, addr = %p\n", i, pExternallyAllocatedMem);
#endif

    W16 TUTUClear_ret = TUTUClear_Init(pExternallyAllocatedMem,
						&sg_tutuclear_obj);
    if (TUTUClear_ret != TUTU_OK) {
        bk_printf("Fail to do TUTUClear_Init %d.\n", TUTUClear_ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

static OPERATE_RET __asr_kws_detect(uint8_t  *data, uint32_t datalen)
{
    OPERATE_RET rt = OPRT_COM_ERROR;
    int i, frame_count = 0;
    int w32WakeWord;
    ASR_WAKEUP_WORD_MAP_T *wakeup_info = NULL;

    if(data == NULL || datalen == 0) {
        return OPRT_INVALID_PARM;
    }

    frame_count = datalen / TKL_ASR_ONE_FRAME;

    for (i = 0; i < frame_count; i++) { //accepting 20ms pcm stream
        TUTUClear_OneFrame(sg_tutuclear_obj, (W16*)(data + i * TKL_ASR_ONE_FRAME), &w32WakeWord);
        if (w32WakeWord == 0) {
            continue;
        }

        for(i = 0; i < sizeof(cASR_WAKEUP_WORD_MAP) / sizeof(ASR_WAKEUP_WORD_MAP_T); i++) {
            if(cASR_WAKEUP_WORD_MAP[i].w32WakeWord == w32WakeWord) {
                wakeup_info = &cASR_WAKEUP_WORD_MAP[i];
                break;
            }
        }

        if(NULL == wakeup_info) {
            continue;
        }

        bk_printf("TUTUClear_WakeWord -> %d %s\n", wakeup_info->w32WakeWord, wakeup_info->asr_txt);

        if(sg_wakeup_cb) {
            sg_wakeup_cb(wakeup_info->wakeup_word);
        }

        rt = OPRT_OK;
        break;
    }

    return rt;
}

static OPERATE_RET __asr_kws_reset(void)
{
    return TUTUClear_Init(pExternallyAllocatedMem, &sg_tutuclear_obj);
}
static void __tkl_asr_task(void *args)
{
	OPERATE_RET rt = 0;
    uint32_t  readlen;
    TKL_THREAD_HANDLE tmp_thread = NULL;

    tkl_system_sleep(200);

    rt = __asr_kws_create();
    if (rt != OPRT_OK) {
        goto __err_exit;
    }

    sg_asr_mgr.thread_running = true;

    while (sg_asr_mgr.thread_running) {
        rt = tkl_semaphore_wait(sg_asr_mgr.sem, TKL_SEM_WAIT_FOREVER);
        if (rt != OPRT_OK) {
            continue;
        }

        tkl_mutex_lock(sg_asr_mgr.mutex);
        rt = tuya_ring_buff_used_size_get(sg_asr_mgr.rb);
        readlen = tuya_ring_buff_read(sg_asr_mgr.rb, sg_asr_mgr.buffer, sg_asr_mgr.bufsz);
        tkl_mutex_unlock(sg_asr_mgr.mutex);
        if (readlen <= 0) {
            continue;
        }
        if (readlen) {
            rt = __asr_kws_detect(sg_asr_mgr.buffer, readlen);
            if(OPRT_OK == rt ) {
                tkl_mutex_lock(sg_asr_mgr.mutex);
                tuya_ring_buff_reset(sg_asr_mgr.rb);
                tkl_mutex_unlock(sg_asr_mgr.mutex);
                __asr_kws_reset();
            }
        }
    }

    bk_printf("asr_kws_thread is exit \r\n");

__err_exit:
    tmp_thread = sg_asr_mgr.thread;
    sg_asr_mgr.thread = NULL;
    tkl_thread_release(tmp_thread);
}

OPERATE_RET tkl_asr_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    if(true == sg_asr_mgr.init) {
        return OPRT_OK;
    }

    sg_asr_mgr.thread_running = false;
    sg_asr_mgr.bufsz = TKL_ASR_BUFSZ;

    sg_asr_mgr.oneframe = tkl_system_psram_malloc(TKL_ASR_ONE_FRAME);
    if(NULL == sg_asr_mgr.oneframe) {
        return OPRT_MALLOC_FAILED;
    }

    sg_asr_mgr.buffer = tkl_system_psram_malloc(sg_asr_mgr.bufsz);
    if(NULL == sg_asr_mgr.buffer) {
        tkl_system_psram_free(sg_asr_mgr.oneframe);
        return OPRT_MALLOC_FAILED;
    }

    tkl_semaphore_create_init(&sg_asr_mgr.sem, 0, 500);
    tkl_mutex_create_init(&sg_asr_mgr.mutex);
    tuya_ring_buff_create(sg_asr_mgr.bufsz + 1, OVERFLOW_PSRAM_STOP_TYPE, &sg_asr_mgr.rb);

    tkl_thread_create(&sg_asr_mgr.thread, "asr_task", 1024*4,  4,  __tkl_asr_task, NULL);

    sg_asr_mgr.init = true;

    return rt;
}

OPERATE_RET tkl_asr_enable(void)
{
    tkl_mutex_lock(sg_asr_mgr.mutex);
    tuya_ring_buff_reset(sg_asr_mgr.rb);
    tkl_mutex_unlock(sg_asr_mgr.mutex);

    sg_asr_mgr.enable = true;

    return OPRT_OK;
}

OPERATE_RET tkl_asr_disable(void)
{
    tkl_mutex_lock(sg_asr_mgr.mutex);
    tuya_ring_buff_reset(sg_asr_mgr.rb);
    tkl_mutex_unlock(sg_asr_mgr.mutex);

    sg_asr_mgr.enable = false;
    
    return OPRT_OK;
}

OPERATE_RET tkl_asr_reg_wakeup_cb(TKL_ASR_WAKEUP_CB wakeup_cb)
{
    sg_wakeup_cb = wakeup_cb;
    return OPRT_OK;
}

OPERATE_RET tkl_asr_feed_with_vad(uint8_t *data, uint16_t datalen, uint8_t vadflag)
{
    OPERATE_RET rt = OPRT_OK;

    if (!sg_asr_mgr.init || !sg_asr_mgr.enable) {
        return OPRT_RESOURCE_NOT_READY;
    }

    tkl_mutex_lock(sg_asr_mgr.mutex);
    rt = tuya_ring_buff_write(sg_asr_mgr.rb, data, datalen);
    tkl_mutex_unlock(sg_asr_mgr.mutex);
    if (rt != datalen) {
        bk_printf("wukong kws feed overflow %d\r\n", datalen - rt);
    }

    uint8_t vad_change = 0;

    if (vadflag != sg_asr_mgr.vadflag) {
        if (1 == vadflag && 0 == sg_asr_mgr.vadflag) {
            bk_printf("asr kws vad on\r\n");
        } else if (0 == vadflag && 1 == sg_asr_mgr.vadflag) {
            bk_printf("asr kws vad end\r\n");
            vad_change = 1;
        }
        sg_asr_mgr.vadflag = vadflag;
    }
    
    //!  0 - 0 > vad off
    //!  0 - 1 > vad on
    //!  1 - 1 > vad ing
    //！ 1 - 0 > vad end, vad_change = 1, force post
    if(vad_change) {
        tkl_semaphore_post(sg_asr_mgr.sem);
    }else if(sg_asr_mgr.vadflag) {
        if(tuya_ring_buff_used_size_get(sg_asr_mgr.rb) >= TKL_ASR_DETECT_FRAME) {
            tkl_semaphore_post(sg_asr_mgr.sem);
        }
    }

    uint32_t rblen =  tuya_ring_buff_used_size_get(sg_asr_mgr.rb);
    if (rblen >= TKL_ASR_VAD_FRAME) {
        //drop the oldest data
        uint32_t drop_len   = rblen - TKL_ASR_VAD_FRAME;
        tuya_ring_buff_discard(sg_asr_mgr.rb, drop_len);
    }

    return OPRT_OK;
}


