/**
 * @file tkl_kws.c
 * @version 0.1
 * @date 2025-04-08
 */

#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_semaphore.h"
#include "tuya_ringbuf.h"

#include "tkl_kws.h"

#include "tutuClear.h"
#include <os/mem.h>


/***********************************************************
************************macro define************************
***********************************************************/
#define __EN_EXTERNALLY_ALLOCATION__ // enable this define for externally malloc TUTU objects
#define __EN_DIAGNOSTICS__ //enable this to have the proper internal returned diagnostical information

#define TKL_KWS_ONE_FRAME       (320 * 2)                            //! 20ms
#define TKL_KWS_VAD_FRAME       ((TKL_KWS_ONE_FRAME * 50))        //! 1000ms
#define TKL_KWS_BUFSZ           ((TKL_KWS_ONE_FRAME * 40) * 2)    //! 1000ms * 2 = 2s
#define TKL_KWS_DETECT_FRAME    (TKL_KWS_ONE_FRAME * 5)           //! 100ms

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    TKL_KWS_KWS_ENABLE_CMD = 0,
    TKL_KWS_KWS_DISABLE_CMD,
    TKL_KWS_KWS_VAD_DETECT_CMD
}TKL_KWS_CMD_E;

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
} TKL_KWS_CTRL_T;

typedef struct {
    W32   w32WakeWord;
    char *kws_txt;
    TKL_KWS_WAKEUP_WORD_E wakeup_word;
} KWS_WAKEUP_WORD_MAP_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_KWS_CTRL_T sg_kws_mgr = {0};
static TKL_KWS_WAKEUP_CB sg_wakeup_cb = NULL;

static void *sg_tutuclear_obj = NULL;
static void *pExternallyAllocatedMem = NULL;

static KWS_WAKEUP_WORD_MAP_T cKWS_WAKEUP_WORD_MAP[] = {
    {1, "你好涂鸦",    TKL_KWS_WAKEUP_NIHAO_TUYA},
    {2, "小智同学",    TKL_KWS_WAKEUP_XIAOZHI_TONGXUE},
    {3, "heytuya",    TKL_KWS_WAKEUP_HEY_TUYA},
    {4, "smartlife",  TKL_KWS_WAKEUP_SMARTLIFE},
};

/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __kws_create(void)
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

static OPERATE_RET __kws_detect(uint8_t  *data, uint32_t datalen)
{
    OPERATE_RET rt = OPRT_COM_ERROR;
    int i, frame_count = 0;
    int w32WakeWord;
    KWS_WAKEUP_WORD_MAP_T *wakeup_info = NULL;

    if(data == NULL || datalen == 0) {
        return OPRT_INVALID_PARM;
    }

    frame_count = datalen / TKL_KWS_ONE_FRAME;

    for (i = 0; i < frame_count; i++) { //accepting 20ms pcm stream
        TUTUClear_OneFrame(sg_tutuclear_obj, (W16*)(data + i * TKL_KWS_ONE_FRAME), &w32WakeWord);
        if (w32WakeWord == 0) {
            continue;
        }

        for(i = 0; i < sizeof(cKWS_WAKEUP_WORD_MAP) / sizeof(KWS_WAKEUP_WORD_MAP_T); i++) {
            if(cKWS_WAKEUP_WORD_MAP[i].w32WakeWord == w32WakeWord) {
                wakeup_info = &cKWS_WAKEUP_WORD_MAP[i];
                break;
            }
        }

        if(NULL == wakeup_info) {
            continue;
        }

        bk_printf("TUTUClear_WakeWord -> %d %s\n", wakeup_info->w32WakeWord, wakeup_info->kws_txt);

        if(sg_wakeup_cb) {
            sg_wakeup_cb(wakeup_info->wakeup_word);
        }

        rt = OPRT_OK;
        break;
    }

    return rt;
}

static OPERATE_RET __kws_reset(void)
{
    return TUTUClear_Init(pExternallyAllocatedMem, &sg_tutuclear_obj);
}
static void __tkl_kws_task(void *args)
{
	OPERATE_RET rt = 0;
    uint32_t  readlen;
    TKL_THREAD_HANDLE tmp_thread = NULL;

    tkl_system_sleep(200);

    rt = __kws_create();
    if (rt != OPRT_OK) {
        goto __err_exit;
    }

    sg_kws_mgr.thread_running = true;

    while (sg_kws_mgr.thread_running) {
        rt = tkl_semaphore_wait(sg_kws_mgr.sem, TKL_SEM_WAIT_FOREVER);
        if (rt != OPRT_OK) {
            continue;
        }

        tkl_mutex_lock(sg_kws_mgr.mutex);
        rt = tuya_ring_buff_used_size_get(sg_kws_mgr.rb);
        readlen = tuya_ring_buff_read(sg_kws_mgr.rb, sg_kws_mgr.buffer, sg_kws_mgr.bufsz);
        tkl_mutex_unlock(sg_kws_mgr.mutex);
        if (readlen <= 0) {
            continue;
        }
        if (readlen) {
            rt = __kws_detect(sg_kws_mgr.buffer, readlen);
            if(OPRT_OK == rt ) {
                tkl_mutex_lock(sg_kws_mgr.mutex);
                tuya_ring_buff_reset(sg_kws_mgr.rb);
                tkl_mutex_unlock(sg_kws_mgr.mutex);
                __kws_reset();
            }
        }
    }

    bk_printf("kws_thread is exit \r\n");

__err_exit:
    tmp_thread = sg_kws_mgr.thread;
    sg_kws_mgr.thread = NULL;
    tkl_thread_release(tmp_thread);
}

static void __tkl_kws_post(uint8_t force)
{
    if(tuya_ring_buff_used_size_get(sg_kws_mgr.rb) >= TKL_KWS_DETECT_FRAME || force) {
        tkl_semaphore_post(sg_kws_mgr.sem);
    }
}

static void __tkl_kws_drop(void)
{
    if (!sg_kws_mgr.init || !sg_kws_mgr.enable) {
        return;
    }

    uint32_t rblen =  tuya_ring_buff_used_size_get(sg_kws_mgr.rb);
    if (rblen >= TKL_KWS_VAD_FRAME) {
        uint32_t drop_len   = rblen - TKL_KWS_VAD_FRAME;
        tkl_mutex_lock(sg_kws_mgr.mutex);
        tuya_ring_buff_discard(sg_kws_mgr.rb, drop_len);
        tkl_mutex_unlock(sg_kws_mgr.mutex);
    }
}

OPERATE_RET tkl_kws_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    if(true == sg_kws_mgr.init) {
        return OPRT_OK;
    }

    sg_kws_mgr.thread_running = false;
    sg_kws_mgr.bufsz = TKL_KWS_BUFSZ;

    sg_kws_mgr.oneframe = tkl_system_psram_malloc(TKL_KWS_ONE_FRAME);
    if(NULL == sg_kws_mgr.oneframe) {
        return OPRT_MALLOC_FAILED;
    }

    sg_kws_mgr.buffer = tkl_system_psram_malloc(sg_kws_mgr.bufsz);
    if(NULL == sg_kws_mgr.buffer) {
        tkl_system_psram_free(sg_kws_mgr.oneframe);
        return OPRT_MALLOC_FAILED;
    }

    tkl_semaphore_create_init(&sg_kws_mgr.sem, 0, 500);
    tkl_mutex_create_init(&sg_kws_mgr.mutex);
    tuya_ring_buff_create(sg_kws_mgr.bufsz + 1, OVERFLOW_PSRAM_STOP_TYPE, &sg_kws_mgr.rb);

    tkl_thread_create(&sg_kws_mgr.thread, "kws_task", 1024*4,  4,  __tkl_kws_task, NULL);

    sg_kws_mgr.init = true;

    return rt;
}

OPERATE_RET tkl_kws_enable(void)
{
    tkl_mutex_lock(sg_kws_mgr.mutex);
    tuya_ring_buff_reset(sg_kws_mgr.rb);
    tkl_mutex_unlock(sg_kws_mgr.mutex);

    sg_kws_mgr.enable = true;

    return OPRT_OK;
}

OPERATE_RET tkl_kws_disable(void)
{
    sg_kws_mgr.enable = false;

    tkl_mutex_lock(sg_kws_mgr.mutex);
    tuya_ring_buff_reset(sg_kws_mgr.rb);
    tkl_mutex_unlock(sg_kws_mgr.mutex);

    return OPRT_OK;
}

OPERATE_RET tkl_kws_reg_wakeup_cb(TKL_KWS_WAKEUP_CB wakeup_cb)
{
    sg_wakeup_cb = wakeup_cb;
    return OPRT_OK;
}

OPERATE_RET tkl_kws_feed_with_vad(uint8_t *data, uint16_t datalen, uint8_t vadflag)
{
    OPERATE_RET rt = OPRT_OK;

    if (!sg_kws_mgr.init || !sg_kws_mgr.enable) {
        return OPRT_RESOURCE_NOT_READY;
    }

    tkl_mutex_lock(sg_kws_mgr.mutex);
    rt = tuya_ring_buff_write(sg_kws_mgr.rb, data, datalen);
    tkl_mutex_unlock(sg_kws_mgr.mutex);
    if (rt != datalen) {
        bk_printf("wukong kws feed overflow %d\r\n", datalen - rt);
    }

    uint8_t vad_change = 0;

    if (vadflag != sg_kws_mgr.vadflag) {
        if (1 == vadflag && 0 == sg_kws_mgr.vadflag) {
            bk_printf("kws vad on\r\n");
        } else if (0 == vadflag && 1 == sg_kws_mgr.vadflag) {
            bk_printf("kws vad end\r\n");
            vad_change = 1;
        }
        sg_kws_mgr.vadflag = vadflag;
    }
    
    //!  0 - 0 > vad off
    //!  0 - 1 > vad on
    //!  1 - 1 > vad ing
    //！ 1 - 0 > vad end, vad_change = 1, force post
    if(sg_kws_mgr.vadflag || vad_change) {
        __tkl_kws_post(vad_change);
    }else {
        __tkl_kws_drop();
    }

    return OPRT_OK;
}


