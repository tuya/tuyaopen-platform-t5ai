/**
 * @file tkl_asr.c
 * @version 0.1
 * @date 2025-04-08
 */

#include "tkl_asr.h"

#include "tutuClear.h"
#include <os/mem.h>


/***********************************************************
************************macro define************************
***********************************************************/
#define __EN_EXTERNALLY_ALLOCATION__ // enable this define for externally malloc TUTU objects
#define __EN_DIAGNOSTICS__ //enable this to have the proper internal returned diagnostical information

#define TKL_ASR_DETECT_CHUNK_SIZE (640)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    W32   w32WakeWord;
    char *asr_txt;
    TKL_ASR_WAKEUP_WORD_E wakeup_word;
} ASR_WAKEUP_WORD_MAP_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static void *sg_tutuclear_obj = NULL;
static void *pExternallyAllocatedMem = NULL;
static TKL_ASR_WAKEUP_WORD_E sg_wakeup_word_list[TKL_ASR_WAKEUP_WORD_MAX];
static uint8_t sg_wakeup_word_cnt = 0;

static ASR_WAKEUP_WORD_MAP_T cASR_WAKEUP_WORD_MAP[] = {
    {1, "你好涂鸦", TKL_ASR_WAKEUP_NIHAO_TUYA},
    {2, "小智同学", TKL_ASR_WAKEUP_XIAOZHI_TONGXUE},
    {3, "heytuya", TKL_ASR_WAKEUP_NIHAO_TUYA},
    {4, "hituya",  TKL_ASR_WAKEUP_NIHAO_TUYA},
};

/***********************************************************
***********************function define**********************
***********************************************************/
static bool  __compare_wakeup_word(W32 w32WakeWord, TKL_ASR_WAKEUP_WORD_E wakeup_word)
{
    bool ret = false;
    ASR_WAKEUP_WORD_MAP_T *wakeup_info = NULL;
    
    for(int i = 0; i < sizeof(cASR_WAKEUP_WORD_MAP) / sizeof(ASR_WAKEUP_WORD_MAP_T); i++) {
        if(cASR_WAKEUP_WORD_MAP[i].w32WakeWord == w32WakeWord) {
            wakeup_info = &cASR_WAKEUP_WORD_MAP[i];
            break;
        }
    }

    if(NULL == wakeup_info) {
        return false;
    }

    bk_printf("TUTUClear_WakeWord -> %d %s\n", wakeup_info->w32WakeWord, wakeup_info->asr_txt);

    ret = (wakeup_word == wakeup_info->wakeup_word) ? true : false;

    return ret;
}

OPERATE_RET tkl_asr_init(void)
{
#ifdef __EN_EXTERNALLY_ALLOCATION__
	int i = TUTUClear_QueryMemSz();
    pExternallyAllocatedMem = psram_malloc(i);
	bk_printf("tutuClear PSTAM DM usage = %d bytes, addr = %p\n", i, pExternallyAllocatedMem);
#endif // __EN_EXTERNALLY_ALLOCATION__

    W16 TUTUClear_ret = TUTUClear_Init(pExternallyAllocatedMem,
						&sg_tutuclear_obj);
    if (TUTUClear_ret != TUTU_OK) {
        bk_printf("Fail to do TUTUClear_Init %d.\n", TUTUClear_ret);
        return OPRT_COM_ERROR;
    }

    bk_printf("tkl_asr_init OK!\n");

    return OPRT_OK;
}

OPERATE_RET tkl_asr_wakeup_word_config(TKL_ASR_WAKEUP_WORD_E *wakeup_word_arr, uint8_t arr_cnt)
{
    if(NULL == wakeup_word_arr || 0 == arr_cnt || arr_cnt > TKL_ASR_WAKEUP_WORD_MAX) {
        return OPRT_INVALID_PARM;
    }

    memcpy(sg_wakeup_word_list, wakeup_word_arr, arr_cnt * sizeof(TKL_ASR_WAKEUP_WORD_E));
    sg_wakeup_word_cnt  = arr_cnt;

    return OPRT_OK;
}

uint32_t tkl_asr_get_process_uint_size(void)
{
    return TKL_ASR_DETECT_CHUNK_SIZE;
}

TKL_ASR_WAKEUP_WORD_E tkl_asr_recognize_wakeup_word(uint8_t *data, uint32_t len)
{
    int i = 0;
    W32 w32WakeWord;
    ASR_WAKEUP_WORD_MAP_T *wakeup_info = NULL;

    if(NULL == data || 0 == len) {
        bk_printf("param err\r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    if(0 == sg_wakeup_word_cnt) {
        bk_printf("wakeup word not config\r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    if(len < TKL_ASR_DETECT_CHUNK_SIZE) {
        bk_printf("datalen not enough \r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    TUTUClear_OneFrame(sg_tutuclear_obj, (W16*)data, &w32WakeWord);
    if (w32WakeWord == 0) {
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    TUTUClear_Init(pExternallyAllocatedMem, &sg_tutuclear_obj);

    for(i = 0; i < sizeof(cASR_WAKEUP_WORD_MAP) / sizeof(ASR_WAKEUP_WORD_MAP_T); i++) {
        if(cASR_WAKEUP_WORD_MAP[i].w32WakeWord == w32WakeWord) {
            wakeup_info = &cASR_WAKEUP_WORD_MAP[i];
            break;
        }
    }

    if(NULL == wakeup_info) {
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    bk_printf("TUTUClear_WakeWord -> %d %s\n", wakeup_info->w32WakeWord, wakeup_info->asr_txt);

    for(i = 0; i < sg_wakeup_word_cnt; i++) {
        if(wakeup_info->wakeup_word == sg_wakeup_word_list[i]) {
            return wakeup_info->wakeup_word;
        }
    }

    return TKL_ASR_WAKEUP_WORD_UNKNOWN;
}

OPERATE_RET tkl_asr_deinit(void)
{
    TUTUClear_Release(&sg_tutuclear_obj);

    memset(sg_wakeup_word_list, 0x00, sizeof(sg_wakeup_word_list));
    sg_wakeup_word_cnt = 0;

    return OPRT_OK;
}


