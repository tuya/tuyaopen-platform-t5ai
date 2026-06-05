// Copyright 2022-2023 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_algorithms/aec_v3_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <modules/aec_v3.h>
#include <components/bk_audio/audio_pipeline/ringbuf.h>


#define TAG  "AEC_ALGORITHM"

//#define AEC_DEBUG   //GPIO debug

#ifdef AEC_DEBUG

#define AEC_ALGORITHM_START()                       do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define AEC_ALGORITHM_END()                         do { GPIO_DOWN(32); } while (0)

#define AEC_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define AEC_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define AEC_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define AEC_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define AEC_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define AEC_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define AEC_ALGORITHM_START()
#define AEC_ALGORITHM_END()

#define AEC_PROCESS_START()
#define AEC_PROCESS_END()

#define AEC_INPUT_START()
#define AEC_INPUT_END()

#define AEC_OUTPUT_START()
#define AEC_OUTPUT_END()

#endif


/* AEC data dump depends on debug utils, so must config CONFIG_ADK_UTILS=y when dump aec data. */
#if CONFIG_ADK_UTILS

//#define AEC_DATA_DUMP

#ifdef AEC_DATA_DUMP

/* dump aec data by uart or vfs, only choose one */
#define AEC_DATA_DUMP_BY_UART
//#define AEC_DATA_DUMP_BY_VFS       /* you must sure CONFIG_VFS=y */

#ifdef AEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util g_aec_uart_util = {0};
#define AEC_DATA_DUMP_UART_ID            (1)
#define AEC_DATA_DUMP_UART_BAUD_RATE     (2000000)
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
#include <components/bk_audio/audio_utils/vfs_util.h>
static struct vfs_util g_aec_vfs_util_mic = {0};
static struct vfs_util g_aec_vfs_util_ref = {0};
static struct vfs_util g_aec_vfs_util_out = {0};
#define AEC_DATA_DUMP_VFS_MIC_NAME     "/sd0/aec_mic.pcm"
#define AEC_DATA_DUMP_VFS_REF_NAME     "/sd0/aec_ref.pcm"
#define AEC_DATA_DUMP_VFS_OUT_NAME     "/sd0/aec_out.pcm"
#endif

#endif  //AEC_DATA_DUMP

#endif  //CONFIG_ADK_UTILS

#define AEC_EX_SIZE (93380)
#define AEC_DELAY_BUFFER_SIZE (2000)
#if (CONFIG_AUD_AI_NS_SUPPORT && (CONFIG_AUD_AI_NS_USE_STATIC_SRAM))
uint32 aec_gtbuf[94*1024/4] __attribute__((section(".aec_bss")));
#endif

// Modified by TUYA Start
#define AEC_EC_OUT_BUF_LEN (770*sizeof(int32_t))
int32_t  *buff_ecout = NULL;
#if CONFIG_ADK_DEBUG_DUMP_UTIL
int16_t  *mic_data_save = NULL;
int16_t  *ref_data_save = NULL;
#endif
// Modified by TUYA End

typedef struct aec_algorithm
{
    aec_v3_cfg_t aec_cfg;
    vad_cfg_t vad_cfg;
    int out_block_size;
    int out_block_num;      /**< Number of output block, the size of block is frame size of 20ms audio data */
    AECContext *aec_ctx;
    int16_t *ref_addr;
    int16_t *mic_addr;
    int16_t *out_addr;
    uint32_t frame_size;    /**< 20ms data */
    int      dual_ch;       /**< Enable dual channel input(1)/Disable dual channel input(0)*/
    int      vad_state;
    int16_t  *out_read_addr;
    ringbuf_handle_t vad_rb;
// Modified by TUYA Start
    ec_out_callback  ec_out_cb;
    vad_state_callback vad_state_cb;
// Modified by TUYA End
} aec_v3_algorithm_t;

static aec_vad_process_fun tuya_vad_aec_process_fun = NULL;
#ifdef AEC_DATA_DUMP

static void aec_data_dump_open(void)
{
#ifdef AEC_DATA_DUMP_BY_UART
    uart_util_create(&g_aec_uart_util, AEC_DATA_DUMP_UART_ID, AEC_DATA_DUMP_UART_BAUD_RATE);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_create(&g_aec_vfs_util_mic, AEC_DATA_DUMP_VFS_MIC_NAME);
    vfs_util_create(&g_aec_vfs_util_ref, AEC_DATA_DUMP_VFS_REF_NAME);
    vfs_util_create(&g_aec_vfs_util_out, AEC_DATA_DUMP_VFS_OUT_NAME);
#endif
}

static void aec_data_dump_close(void)
{
#ifdef AEC_DATA_DUMP_BY_UART
    uart_util_destroy(&g_aec_uart_util);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_destroy(&g_aec_vfs_util_mic);
    vfs_util_destroy(&g_aec_vfs_util_ref);
    vfs_util_destroy(&g_aec_vfs_util_out);
#endif
}

static void aec_data_dump_mic_data(void *data_buf, uint32_t len)
{
#ifdef AEC_DATA_DUMP_BY_UART
    uart_util_tx_data(&g_aec_uart_util, data_buf, len);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_aec_vfs_util_mic, data_buf, len);
#endif
}

static void aec_data_dump_ref_data(void *data_buf, uint32_t len)
{
#ifdef AEC_DATA_DUMP_BY_UART
    uart_util_tx_data(&g_aec_uart_util, data_buf, len);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_aec_vfs_util_ref, data_buf, len);
#endif
}

static void aec_data_dump_out_data(void *data_buf, uint32_t len)
{
#ifdef AEC_DATA_DUMP_BY_UART
    uart_util_tx_data(&g_aec_uart_util, data_buf, len);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_aec_vfs_util_out, data_buf, len);
#endif
}

#define AEC_DATA_DUMP_OPEN()                        aec_data_dump_open()
#define AEC_DATA_DUMP_CLOSE()                       aec_data_dump_close()
#define AEC_DATA_DUMP_MIC_DATA(data_buf, len)       aec_data_dump_mic_data(data_buf, len)
#define AEC_DATA_DUMP_REF_DATA(data_buf, len)       aec_data_dump_ref_data(data_buf, len)
#define AEC_DATA_DUMP_OUT_DATA(data_buf, len)       aec_data_dump_out_data(data_buf, len)

#else

#define AEC_DATA_DUMP_OPEN()
#define AEC_DATA_DUMP_CLOSE()
#define AEC_DATA_DUMP_MIC_DATA(data_buf, len)
#define AEC_DATA_DUMP_REF_DATA(data_buf, len)
#define AEC_DATA_DUMP_OUT_DATA(data_buf, len)

#endif  //AEC_DATA_DUMP

static void aec_vad_thr_mapping(int16_t* SPthr, int32_t start_thr, int32_t stop_thr, int32_t silence_thr, int16_t eng_thr)
{
    const int32_t frame_time = 20;
    int32_t start_level, stop_level;
#if 0
    start_level = 2;
    stop_level = 4;
#else
    ////////////////////////////
    if (start_thr > 700)  // start slow
    {
        start_level = 5;
    }
    else if (start_thr > 600)
    {
        start_level = 4;
    }
    else if (start_thr > 500)
    {
        start_level = 3;
    }
    else if (start_thr > 400)
    {
        start_level = 2;
    }
    else  // start fast
    {
        start_level = 1;
    }
    ////////////////////////////
    if (stop_thr >= 3000)  //stop slow
    {
        stop_level = 8;
    }
    else if (stop_thr >= 2500)
    {
        stop_level = 7;
    }
    else if (stop_thr >= 2000)
    {
        stop_level = 6;
    }
    else if (stop_thr >= 1500)
    {
        stop_level = 5;
    }
    else if (stop_thr >= 1000)
    {
        stop_level = 4;
    }
    else if (stop_thr >= 700)
    {
        stop_level = 3;
    }
    else if (stop_thr >= 500)
    {
        stop_level = 2;
    }
    else  //stop fast
    {
        stop_level = 1;
    }
#endif
    /////////////////////////////////
    switch (start_level)
    {
       case 1:  // start fast
             SPthr[1] = 40;
             SPthr[3] = 5;
             SPthr[4] = 6;
             break;
       case 2:
             SPthr[1] = 70;
             SPthr[3] = 5;
             SPthr[4] = 6;
             break;
       case 3:
             SPthr[1] = 90;
             SPthr[3] = 4;
             SPthr[4] = 5;
             break;
       case 4:
             SPthr[1] = 98;
             SPthr[3] = 3;
             SPthr[4] = 4;
             break;
       case 5:  // start slow
             SPthr[1] = 98;
             SPthr[3] = 2;
             SPthr[4] = 3;
             break;
       default:
             break;
    }
    ////////////////////////////////
    switch (stop_level)
    {
    case 1:  //stop fast  //300ms
        SPthr[2] = 150;
        SPthr[5] = 9;
        break;
    case 2:              //500ms
        SPthr[2] = 150;
        SPthr[5] = 5;
        break;
    case 3:              //750ms
        SPthr[2] = 150;
        SPthr[5] = 3;
        break;
    case 4:              //1000ms
        SPthr[2] = 150;
        SPthr[5] = 2;
        break;
    case 5:              //1500ms
        SPthr[2] = 150;
        SPthr[5] = 1;
        break;
    case 6:             //2000ms
        SPthr[2] = 100;
        SPthr[5] = 0;
        break;
    case 7:              //2500ms
        SPthr[2] = 125;
        SPthr[5] = 0;
        break;
    case 8:             //3000ms
        SPthr[2] = 150;
        SPthr[5] = 0;
        break;
    default:
        break;
    }
    ////////////////////////
    {
        int st = frame_time*(SPthr[2] / (SPthr[5] + 1)) * 2 / 3;

        if (st < silence_thr)
        {
            SPthr[6] = st;
        }
        else
        {
            SPthr[6] = silence_thr;
        }

        SPthr[0] = eng_thr;
    }
    ////////////////////////
    BK_LOGD(TAG, "aec_cfg 5 SPthr[0:6]:%d  %d  %d  %d  %d  %d  %d\n", SPthr[0], SPthr[1], SPthr[2], SPthr[3], SPthr[4], SPthr[5], SPthr[6]);
    BK_LOGD(TAG, "aec_cfg 6 SPthr[7:13]:%d  %d  %d  %d  %d  %d  %d\n", SPthr[7], SPthr[8], SPthr[9], SPthr[10], SPthr[11], SPthr[12], SPthr[13]);
}

static void aec_vad_flag_update(aec_v3_algorithm_t *aec, int vad_state)
{
    if((aec->vad_state != vad_state) 
        && (VAD_NONE != vad_state) 
        && (VAD_SPEECH_START <= vad_state) 
        && (VAD_SILENCE >= vad_state))  //1 vad start 2 vad end 3 slience
    {
        static const char *vad_str[] = 
        {
            "vad_none","vad_speech_start","vad_speech_end","vad_silence"
        };
        BK_LOGD(TAG, "vad_state:%s -> %s\n", vad_str[aec->vad_state],vad_str[vad_state]);
        bk_printf_raw(BK_LOG_WARN, NULL, "vad_state:%s -> %s\n", vad_str[aec->vad_state],vad_str[vad_state]);
        aec->vad_state = vad_state;
// Modified by TUYA Start
        if (aec->vad_state_cb) {
            aec->vad_state_cb(aec->vad_state);
        }
// Modified by TUYA End
    }
}

static int aec_vad_proc(aec_v3_algorithm_t *aec)   // Modified by TUYA Start
{
    static int aec_vad_flag=0;
    static int aec_vad_mem=0;

    if(aec->vad_cfg.vad_enable == 0)
    {
        aec_vad_flag = 0;
        aec_vad_mem = 0;
        aec->vad_cfg.vad_bad_frame = 16;
        return aec_vad_flag;         // Modified by TUYA Start
    }

    int dc = aec->aec_ctx->dc >> 14;
    if (dc<0)
    {
        dc = -dc;
    }
    if ( (dc>800) && (aec->aec_ctx->mic_max>10000) )
    {
        aec->vad_cfg.vad_bad_frame = 10;
    }
    else if (aec->aec_ctx->mic_max>30000)
    {
        aec->vad_cfg.vad_bad_frame = 8;
    }
    else
    {
        aec->vad_cfg.vad_bad_frame--;
        if (aec->vad_cfg.vad_bad_frame < 0)
        {
            aec->vad_cfg.vad_bad_frame = 0;
        }
    }
    if (aec->vad_cfg.vad_bad_frame)
    {
        aec->aec_ctx->spcnt >>= 1;
    }

    if(aec->aec_ctx->test)
    {
        if(aec_vad_mem==0)
        {
            aec_vad_flag = 1;
            aec->aec_ctx->spcnt = aec->aec_ctx->SPthr[2];
            aec->aec_ctx->test = aec->aec_ctx->SPthr[2];
        }
        else
        {
            if(aec->aec_ctx->dcnt*20 == aec->aec_ctx->SPthr[6])
            {
                aec_vad_flag = 3;
            }
            else
            {
                aec_vad_flag = 0;
            }
        }
        aec_vad_mem = aec->aec_ctx->test;
    }
    else
    {
        if(0 == aec_vad_mem)
        {
            aec_vad_flag = 0;
        }
        if(aec_vad_mem > 0)
        {
            aec_vad_flag = 2;  // vad end
        }
        aec_vad_mem = aec->aec_ctx->test;
    }

    aec_vad_flag_update(aec, aec_vad_flag);
    return aec_vad_flag;         // Modified by TUYA Start
}

static bk_err_t _aec_v3_algorithm_open(audio_element_handle_t self)
{
    uint32_t val = 0;
    uint32_t aec_context_size = 0;

    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(self);

    uint32_t offset=0;
    uint32_t aec_frame_sample_cnt;

    aec_context_size = aec_size(AEC_DELAY_BUFFER_SIZE/2);
    offset += aec_context_size;    
    #if CONFIG_AUD_AI_NS_SUPPORT && !CONFIG_AUD_AI_NS_USE_STATIC_SRAM
    if(aec->aec_cfg.ns_type == NS_AI)
    {
        aec_context_size = aec_size((AEC_EX_SIZE + AEC_DELAY_BUFFER_SIZE)/2); 
    }
    #endif
 

    /* init */
    aec->aec_ctx = (AECContext *)audio_malloc(aec_context_size);
    if (!aec->aec_ctx)
    {
        BK_LOGE(TAG, "[%s] %s, %d, audio_malloc aec context: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, aec_context_size);
        return BK_FAIL;
    }

    //采样率可以配置8000或者16000
    aec->aec_ctx->fs = 0;
    aec_init(aec->aec_ctx, aec->aec_cfg.fs);

    //获取结构体内部可以复用的ram作为每帧tx,rx,out数据的临时buffer; ram很宽裕的话也可以在外部单独申请获取
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_TX_BUF, (uint32_t)(uintptr_t)(&val));
    aec->mic_addr = (int16_t *)(uintptr_t)val;
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_RX_BUF, (uint32_t)(uintptr_t)(&val));
    aec->ref_addr = (int16_t *)(uintptr_t)val;
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_OUT_BUF, (uint32_t)(uintptr_t)(&val));
    aec->out_addr = (int16_t *)(uintptr_t)val;

    //以下是参数调节示例,aec_init中都已经有默认值,可以直接先用默认值
    if(aec->dual_ch)
    {
        aec->aec_cfg.init_flags |= AEC_DM_FLAG_MSK;
    }
    else
    {
        aec->aec_cfg.init_flags &= ~AEC_DM_FLAG_MSK;
    }

    if(NS_CLOSE == aec->aec_cfg.ns_type)
    {
        aec->aec_cfg.init_flags &= ~AEC_NS_FLAG_MSK;
    }
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_FLAGS, aec->aec_cfg.init_flags);            //库内各模块开关; aec_init内默认赋值0x1f,dual mic:AEC_DM_FLAG_MSK;
    BK_LOGI(TAG, "[%s] aec dual_dmic:%d,init_flags:0x%x\n", audio_element_get_tag(self),aec->dual_ch,aec->aec_cfg.init_flags);

    ///回声消除相关
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_MIC_DELAY, aec->aec_cfg.delay_points);      //设置参考信号延迟(采样点数，需要dump数据观察)
    BK_LOGV(TAG, "delay_points = %d\n", aec->aec_cfg.delay_points);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EC_DEPTH, aec->aec_cfg.ec_depth);           //建议取值范围1~50; 后面几个参数建议先用aec_init内的默认值，具体需要根据实际情况调试; 总得来说回声越大需要调的越大
    
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_REF_SCALE, aec->aec_cfg.ref_scale);         //取值0,1,2；rx数据如果幅值太小的话适当放大     
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_VOL, aec->aec_cfg.voice_vol);               //通话过程中如果需要经常调节喇叭音量就设置下当前音量等级
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_MAX_DELAY, AEC_DELAY_BUFFER_SIZE/2);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_GET_FRAME_SAMPLE, (uint32_t)(&aec_frame_sample_cnt));
    BK_LOGI(TAG, "[%s] aec ver:%d fs:%d,aec frame samp cnt:%d, frame_size:%d\n", audio_element_get_tag(self),aec_ver(),aec->aec_cfg.fs,aec_frame_sample_cnt,aec->frame_size);

    ///降噪相关
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_LEVEL, aec->aec_cfg.ns_level);           //建议取值范围1~8；值越小底噪越小
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_PARA, aec->aec_cfg.ns_para);             //只能取值0,1,2; 降噪由弱到强，建议默认值
    
	///drc(输出音量相关)
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_DRC, aec->aec_cfg.drc);                     //建议取值范围0x10~0x1f;   越大输出声音越大
    
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EC_FILTER, aec->aec_cfg.ec_filter);//0x01	 0x03	0x07
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_DELAY_BUFF, (uint32_t)aec->aec_ctx->refbuff);

    if(aec->aec_cfg.ns_type == NS_AI)
    {
        #if CONFIG_AUD_AI_NS_SUPPORT
        uint8_t * gtbuff;
        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_FILTER, 0x80);
        #if CONFIG_AUD_AI_NS_USE_STATIC_SRAM
        gtbuff = (uint8_t*)aec_gtbuf;
        #else
        gtbuff = ((uint8_t *)aec->aec_ctx + offset);
        #endif
        memset(gtbuff, 0 , AEC_EX_SIZE);
        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_GTBUFF, (uint32_t)gtbuff);
        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_GTPROC, (uint32_t)gtcrn_proc);
        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_GTTEMP, (uint32_t)aec->aec_ctx->tmp2);
        #else
        BK_LOGE(TAG, "ERR:aec ns_type is NA_AI but CONFIG_AUD_AI_NS_SUPPORT is disabled!\n");
        #endif
    }
    else
    {
        if(aec->aec_cfg.ns_filter != 0x80)
        {
            aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_FILTER, aec->aec_cfg.ns_filter);
        }
        else
        {
            aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_FILTER, 0x7);
        }
    }

    // add dual dmic enc process
    // Modified by TUYA Start
    if(aec->dual_ch)
    {
        aec->aec_ctx->interweave = aec->aec_cfg.interweave;
        aec->aec_ctx->dist = aec->aec_cfg.dist;
        aec->aec_ctx->mic_swap = aec->aec_cfg.mic_swap;
        if(aec->vad_cfg.vad_enable)
        {
            aec->aec_ctx->vad = 1;
        }

        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_DUAL_PERP, (uint32_t)aec->aec_cfg.dual_perp);
        if(DUAL_CH_0_DEGREE == aec->aec_cfg.dual_perp)
        {
            aec->aec_ctx->dist = aec->aec_cfg.dist;
        }
    }

    //output echo cancellation only data
    if(aec->aec_cfg.ec_only_output)
    {
        aec->aec_ctx->ec_filter |= (1 << 5);
        buff_ecout = (int32_t *)audio_malloc(AEC_EC_OUT_BUF_LEN);
        if (!buff_ecout)
        {
            BK_LOGE(TAG, "[%s] %s, %d, audio_malloc buff_ecout: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, AEC_EC_OUT_BUF_LEN);
            goto fail;
        }
        aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EOBUFF, (uint32_t)buff_ecout);
        BK_LOGD(TAG, "AEC_CTRL_CMD_SET_EOBUFF addr:0x%x\n",buff_ecout);
    }

    #if CONFIG_ADK_DEBUG_DUMP_UTIL
    uint32_t mic_data_len = aec->frame_size;
    if(aec->dual_ch)
    {
        mic_data_len <<= 1;
    }

    mic_data_save = (int16_t *)audio_malloc(mic_data_len);
    if (!mic_data_save)
    {
        BK_LOGE(TAG, "[%s] %s, %d, audio_malloc mic_data_save: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, mic_data_len);
        goto fail;
    }

    ref_data_save = (int16_t *)audio_malloc(aec->frame_size);
    if (!ref_data_save)
    {
        BK_LOGE(TAG, "[%s] %s, %d, audio_malloc ref_data_save: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, aec->frame_size);
        goto fail;
    }
    #endif

    BK_LOGI(TAG, "aec_cfg 1:mode:%d,dual_ch:%d,flags:0x%x,interweave:%d,dual_perp:%d,dist:%d,mic_swap:%d,vol:%d\n",
                 aec->aec_cfg.mode,
                 aec->dual_ch,
                 aec->aec_ctx->flags,
                 aec->aec_ctx->interweave,
                 aec->aec_cfg.dual_perp,
                 aec->aec_ctx->dist,
                 aec->aec_ctx->mic_swap,
                 aec->aec_ctx->vol);
    
    BK_LOGI(TAG, "aec_cfg 2:ec_filter:0x%x,ec_depth:%d,drc_mode:%d,mic_delay:%d,max_mic_delay:%d\n", 
                 aec->aec_ctx->ec_filter,
                 aec->aec_ctx->ec_depth,
                 aec->aec_ctx->drc_mode,
                 aec->aec_ctx->mic_delay,
                 aec->aec_ctx->max_mic_delay);

    BK_LOGI(TAG, "aec_cfg 3:spcnt:%d,ns_type:%d,ns_filter:0x%x,vad:%d,vad_en:%d,ec_only_out:%d\n", 
                 aec->aec_ctx->spcnt,
                 aec->aec_cfg.ns_type,
                 aec->aec_ctx->ns_filter,
                 aec->aec_ctx->vad,
                 aec->vad_cfg.vad_enable,
                 aec->aec_cfg.ec_only_output);
    // Modified by TUYA End

    if(aec->vad_cfg.vad_enable)
    {

        if((aec->vad_cfg.vad_start_threshold !=0 && aec->vad_cfg.vad_stop_threshold != 0xff)
        && (aec->vad_cfg.vad_start_threshold != aec->vad_cfg.vad_stop_threshold))
        {
            aec_vad_thr_mapping(aec->aec_ctx->SPthr, 
                                aec->vad_cfg.vad_start_threshold, 
                                aec->vad_cfg.vad_stop_threshold, 
                                aec->vad_cfg.vad_silence_threshold,
                                aec->vad_cfg.vad_eng_threshold);
        }
        // Modified by TUYA Start
        uint32 delay_num = 0;
        delay_num = 2 + (aec->aec_ctx->SPthr[1])/(aec->aec_ctx->SPthr[3] + aec->aec_ctx->SPthr[4]);
        BK_LOGD(TAG, "aec_cfg 4:vad_rb_size:%d,vad_frame_size:%d,delay_num:%d\n",
                aec->vad_cfg.vad_buf_size + aec->vad_cfg.vad_frame_size*4,aec->vad_cfg.vad_frame_size, delay_num);

        aec->out_read_addr = audio_malloc(aec->frame_size);
        if (!aec->out_read_addr)
        {
            BK_LOGE(TAG, "[%s] %s, %d, audio_malloc aec out_read_addr: %d fail \n", audio_element_get_tag(self), __func__, __LINE__, aec->frame_size);
            goto fail;
        }

        aec->vad_rb = rb_create(aec->vad_cfg.vad_buf_size + aec->vad_cfg.vad_frame_size*4, 1);
        if (!aec->vad_rb)
        {
            BK_LOGE(TAG, "[%s] %s, create vad ring buffer fail\n",audio_element_get_tag(self), __func__);
            goto fail;
        }

        char *delay_arr = (char *)os_malloc(aec->frame_size);
        os_memset(delay_arr, 0x00, aec->frame_size);
        for (uint32_t k = 0; k < delay_num; k++)
        {
            rb_write(aec->vad_rb, &delay_arr[0], aec->frame_size, BEKEN_WAIT_FOREVER);
        }
        os_free(delay_arr);
        delay_arr = NULL;
        // Modified by TUYA End
    }

    BK_LOGD(TAG, "[%s] _aec_algorithm_open\n", audio_element_get_tag(self));

    return BK_OK;

    // Modified by TUYA Start
fail:
    if (aec->aec_ctx)
    {
        audio_free(aec->aec_ctx);
        aec->aec_ctx = NULL;
    }

    if(buff_ecout)
    {
        audio_free(buff_ecout);
        buff_ecout = NULL;
    }

    #if CONFIG_ADK_DEBUG_DUMP_UTIL
    if(mic_data_save)
    {
        audio_free(mic_data_save);
        mic_data_save = NULL;
    }

    if(ref_data_save)
    {
        audio_free(ref_data_save);
        ref_data_save = NULL;
    }
    #endif

    if(aec->out_read_addr)
    {
        audio_free(aec->out_read_addr);
        aec->out_read_addr = NULL;
    }

    if(aec->vad_rb)
    {
        rb_destroy(aec->vad_rb);
        aec->vad_rb = NULL;
    }
    
    return BK_FAIL;
    // Modified by TUYA End
}

static bk_err_t _aec_v3_algorithm_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    return BK_OK;
}

// Modified by TUYA Start
#define DUAL_MIC_DIG_AMP 16
static int find_int16_min_max(int16_t *arr, uint32_t arr_len, int16_t *out_max, int16_t *out_min)
{
    // 1. 参数合法性检查
    if (arr == NULL || out_max == NULL || out_min == NULL || arr_len == 0) {
        bk_printf("Error: invalid parameters\n");
        return -1;
    }

    // 2. 初始化最大值/最小值为第一个元素
    *out_max = arr[0];
    *out_min = arr[0];

    // 3. 遍历数组，更新最大值/最小值
    for (uint32_t i = 1; i < arr_len; i++) {
        // 更新最大值
        if (arr[i] > *out_max) {
            *out_max = arr[i];
        }
        // 更新最小值
        if (arr[i] < *out_min) {
            *out_min = arr[i];
        }
    }

    return 0;
}

static int dual_mic_remove_dc_offset(int16_t *in_buf, uint32_t frame_len)
{
    int16_t *dual_mic_buffer = in_buf;
    uint32_t data_len = frame_len;
    int16_t max_amp = INT16_MAX;
    int16_t min_amp = INT16_MIN;
    find_int16_min_max(dual_mic_buffer, data_len, &max_amp, &min_amp);
    int16_t abs_amp = (max_amp + min_amp) / 2;

    for (int i = 0; i < data_len; i++)
    {
        dual_mic_buffer[i] = (dual_mic_buffer[i] - abs_amp) * DUAL_MIC_DIG_AMP;
    }

    return 0;
}
// Modified by TUYA End

static int _aec_v3_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] %s, in_len: %d \n", audio_element_get_tag(self), __func__, in_len);
    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(self);
    // Modified by TUYA Start
    #if CONFIG_ADK_DEBUG_DUMP_UTIL
    uint32_t mic_data_len = aec->frame_size;
    #endif
    // Modified by TUYA End

    AEC_PROCESS_START();

    AEC_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    BK_LOGV(TAG, "[%s] r_size=%d, in_len=%d \n",audio_element_get_tag(self), r_size, in_len);
    if (r_size != in_len)
    {
        BK_LOGE(TAG, "mic_data Waring: r_size=%d, in_len=%d \n", r_size, in_len);
    }
 
    if(aec->dual_ch)
    {
        aec->mic_addr = (int16_t *)in_buffer;
        int r_ref_size = audio_element_multi_input(self, (char *)aec->ref_addr, in_len/2, 0, 0);
        if (r_ref_size != in_len/2)
        {
            BK_LOGV(TAG, "rd ref_data error: r_ref_size=%d, in_len=%d line:%d \n", r_ref_size, in_len/2, __LINE__);
            os_memset(aec->ref_addr, 0, in_len/2);
        }
        else
        {
            BK_LOGV(TAG, "rd ref_data:r_ref_size=%d, in_line:%d,line:%d \n", r_ref_size, in_len/2, __LINE__);
        }
    }
    else
    {
        if (aec->aec_cfg.mode == AEC_MODE_HARDWARE)
        {
            int16_t *lr_data_ptr = (int16_t *)in_buffer;
            for (uint16_t i = 0; i < r_size / 4; i++)
            {
                aec->mic_addr[i] = lr_data_ptr[2 * i];
                aec->ref_addr[i] = lr_data_ptr[2 * i + 1];
            }
        }
        else
        {
            aec->mic_addr = (int16_t *)in_buffer;
            int r_ref_size = audio_element_multi_input(self, (char *)aec->ref_addr, in_len, 0, 0);
            if (r_ref_size != in_len)
            {
                BK_LOGV(TAG, "ref_data Waring: r_size=%d, in_len=%d line:%d \n", r_size, in_len, __LINE__);
                os_memset(aec->ref_addr, 0, in_len);
            }
            else
            {
                //BK_LOGV(TAG, "ref_data Waring: r_size=%d, line:%d \n", r_size, __LINE__);
            }
        }
    }
    
    AEC_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        
        if(aec->dual_ch)
        {
            AEC_DATA_DUMP_MIC_DATA(aec->mic_addr, aec->frame_size*2);
            // Modified by TUYA Start
            #if CONFIG_ADK_DEBUG_DUMP_UTIL
            mic_data_len = aec->frame_size*2;
            #endif
            // Modified by TUYA End
        }
        else
        {
            AEC_DATA_DUMP_MIC_DATA(aec->mic_addr, aec->frame_size);
        }
        
        // Modified by TUYA Start
        AEC_DATA_DUMP_REF_DATA(aec->ref_addr, aec->frame_size);

        #if CONFIG_ADK_DEBUG_DUMP_UTIL
        if(is_aud_dump_valid(DUMP_TYPE_AEC_MIC_DATA))
        {
            os_memcpy(mic_data_save,aec->mic_addr, mic_data_len);
            os_memcpy(ref_data_save,aec->ref_addr, aec->frame_size);
        }
        #endif

        AEC_ALGORITHM_START();

        if(aec->dual_ch)
        {
            dual_mic_remove_dc_offset(aec->mic_addr, aec->frame_size); // 数字麦双麦会有大的直流偏置，需要先去直流偏置之后再加数字增益
        }
        

        if(aec->ec_out_cb)
        {
            aec->ec_out_cb(buff_ecout,aec->frame_size); // 输出不带降噪的aec音频
        }

        // add for tuya aec vad
        if (tuya_vad_aec_process_fun)
        {
            tuya_vad_aec_process_fun(aec->mic_addr, aec->ref_addr, aec->out_addr);
        } else {
            aec_proc(aec->aec_ctx, aec->ref_addr, aec->mic_addr, aec->out_addr);
            audio_element_multi_output(self, (char *)aec->out_addr, aec->frame_size, 0);
            aec_vad_proc(aec);
        }

        AEC_ALGORITHM_END();

        AEC_DATA_DUMP_OUT_DATA(aec->out_addr, aec->frame_size);

        #if CONFIG_ADK_DEBUG_DUMP_UTIL
        if(is_aud_dump_valid(DUMP_TYPE_AEC_MIC_DATA))
        {
            /*update header*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_AEC_MIC_DATA, 0, mic_data_len);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_AEC_REF_DATA, 1, aec->frame_size);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_AEC_OUT_DATA, 2, aec->frame_size);
            DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_AEC_MIC_DATA);

            /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
            DEBUG_DATA_DUMP_SUSPEND_ALL;

            /*dump header*/
            DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_AEC_MIC_DATA);

            /*dump data*/
            DEBUG_DATA_DUMP_BY_UART_DATA(mic_data_save, mic_data_len);
            DEBUG_DATA_DUMP_BY_UART_DATA(ref_data_save, aec->frame_size);
            DEBUG_DATA_DUMP_BY_UART_DATA(aec->out_addr, aec->frame_size);
            DEBUG_DATA_DUMP_RESUME_ALL;

            /*update seq*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_AEC_MIC_DATA);
        }
        #endif
        // Modified by TUYA End

        if((aec->vad_cfg.vad_enable) && (VAD_NONE != aec->vad_state))
        {
            static int vad_buff_data_size = 0;
            // Modified by TUYA
            if((VAD_SPEECH_START != aec->vad_state) && (VAD_SILENCE != aec->vad_state))
            {
                if(0 <= (int)(vad_buff_data_size - aec->frame_size))
                {
                    rb_read(aec->vad_rb, (char *)aec->out_read_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);
                    w_size = audio_element_output(self, (char *)aec->out_read_addr, aec->frame_size);

                    //audio_element_multi_output(self, (char *)aec->out_read_addr, aec->frame_size, 0);
                    vad_buff_data_size -= aec->frame_size;
                }
                else
                {
                    w_size = aec->frame_size;
                }

                int fill_size = rb_bytes_filled(aec->vad_rb);

                if(aec->vad_cfg.vad_buf_size >= (fill_size + aec->frame_size))
                {
                    rb_write(aec->vad_rb, (char *)aec->out_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);
                }
                else
                {
                    //read old data
                    rb_read(aec->vad_rb, (char *)aec->out_read_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);

                    //write new data
                    rb_write(aec->vad_rb, (char *)aec->out_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);
                }
            }
            else
            {
                AEC_OUTPUT_START();
                rb_write(aec->vad_rb, (char *)aec->out_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);
                rb_read(aec->vad_rb, (char *)aec->out_read_addr,  aec->frame_size, BEKEN_WAIT_FOREVER);

                vad_buff_data_size = rb_bytes_filled(aec->vad_rb);

                w_size = audio_element_output(self, (char *)aec->out_read_addr, aec->frame_size);
                AEC_OUTPUT_END();

                /* write data to multiple audio port */
                /* unblock write, and not check write result */
                //TODO
                //audio_element_multi_output(self, (char *)aec->out_read_addr, aec->frame_size, 0);
            }
        }
        else
        {
            AEC_OUTPUT_START();
            w_size = audio_element_output(self, (char *)aec->out_addr, aec->frame_size);
            AEC_OUTPUT_END();

            /* write data to multiple audio port */
            /* unblock write, and not check write result */
            //TODO
            //audio_element_multi_output(self, (char *)aec->out_addr, aec->frame_size, 0);
        }
    }
    else
    {
        w_size = r_size;
    }

    AEC_PROCESS_END();
    BK_LOGV(TAG, "[%s] w_size=%d\n",audio_element_get_tag(self), w_size);

    return w_size;
}



static bk_err_t _aec_v3_algorithm_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(self);

    // Modified by TUYA Start
    if(buff_ecout)
    {
        audio_free(buff_ecout);
        buff_ecout = NULL;
    }

    #if CONFIG_ADK_DEBUG_DUMP_UTIL
    if(mic_data_save)
    {
        audio_free(mic_data_save);
        mic_data_save = NULL;
    }

    if(ref_data_save)
    {
        audio_free(ref_data_save);
        ref_data_save = NULL;
    }
    #endif
    // Modified by TUYA End

    if(aec->out_read_addr)
    {
        audio_free(aec->out_read_addr);
        aec->out_read_addr = NULL;
    }

    if(aec->vad_rb)
    {
        rb_destroy(aec->vad_rb);
        aec->vad_rb = NULL;             // Modified by TUYA
    }
    
    if (aec->aec_ctx)
    {
        audio_free(aec->aec_ctx);
        aec->aec_ctx = NULL;
    }
    audio_free(aec);

    AEC_DATA_DUMP_CLOSE();

    return BK_OK;
}

audio_element_handle_t aec_v3_algorithm_init(aec_v3_algorithm_cfg_t *config)
{
    audio_element_handle_t el;

    /* check config */
    if (config->aec_cfg.fs != 8000 && config->aec_cfg.fs != 16000)
    {
        BK_LOGE(TAG, "check config->aec_cfg.fs fail \n");
        return NULL;
    }

    aec_v3_algorithm_t *aec_alg = audio_calloc(1, sizeof(aec_v3_algorithm_t));
    AUDIO_MEM_CHECK(TAG, aec_alg, return NULL);

    aec_alg->frame_size = config->aec_cfg.fs / 1000 * 2 * 20;

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _aec_v3_algorithm_open;
    cfg.close = _aec_v3_algorithm_close;
    cfg.seek = NULL;
    cfg.process = _aec_v3_algorithm_process;
    cfg.destroy = _aec_v3_algorithm_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    /* 20ms, 16bit */
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.multi_in_port_num = 1;

    if (config->aec_cfg.mode == AEC_MODE_HARDWARE)
    {
        cfg.buffer_len = aec_alg->frame_size * 2;
    }
    else
    {
        cfg.buffer_len = aec_alg->frame_size;
    }

    if(config->dual_ch)
    {
        cfg.buffer_len = aec_alg->frame_size*2;
    }

    cfg.tag = "aec_algorithm";
    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _aec_algorithm_init_exit);
    os_memcpy(&aec_alg->aec_cfg,&config->aec_cfg,sizeof(aec_v3_cfg_t));
    os_memcpy(&aec_alg->vad_cfg,&config->vad_cfg,sizeof(vad_cfg_t));
    aec_alg->out_block_num = config->out_block_num;
    aec_alg->aec_ctx = NULL;
    aec_alg->ref_addr = NULL;
    aec_alg->mic_addr = NULL;
    aec_alg->out_addr = NULL;
    aec_alg->dual_ch = config->dual_ch;
    aec_alg->vad_state = VAD_NONE;
    // Modified by TUYA Start
    aec_alg->ec_out_cb    = config->ec_out_cb;
    aec_alg->vad_state_cb = config->vad_state_cb;
    // Modified by TUYA End
    audio_element_setdata(el, aec_alg);

    AEC_DATA_DUMP_OPEN();

    return el;
_aec_algorithm_init_exit:
    audio_free(aec_alg);
    return NULL;
}

bk_err_t aec_v3_algorithm_set_config(audio_element_handle_t aec_algorithm, void * aec_config)
{
    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(aec_algorithm);
    if (aec == NULL) {
        BK_LOGE(TAG, "aec is NULL \n");
        return BK_FAIL;
    }

    if (aec_config == NULL) {
        BK_LOGE(TAG, "aec_config is NULL \n");
        return BK_FAIL;
    }
    app_aud_aec_v3_config_t *aec_cfg = (app_aud_aec_v3_config_t *)aec_config;

    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_FLAGS, 0x1F);//(uint32_t)aec_cfg->init_flags);

    aec->aec_cfg.init_flags   = 0x1F;//aec_cfg->init_flags;    // 0x1f
    aec->aec_cfg.delay_points = aec_cfg->mic_delay;    //0x0
    aec->aec_cfg.ec_depth     = aec_cfg->ec_depth;     //0x14
    aec->aec_cfg.ns_type      = aec_cfg->ns_type;
    aec->aec_cfg.ns_level     = aec_cfg->ns_level;
    aec->aec_cfg.ns_para      = aec_cfg->ns_para;
    aec->aec_cfg.ref_scale    = aec_cfg->ref_scale;
    aec->aec_cfg.drc          = aec_cfg->drc_gain;
    aec->aec_cfg.voice_vol    = aec_cfg->voice_vol;
    aec->aec_cfg.ec_filter    = aec_cfg->ec_filter;
    aec->aec_cfg.ns_filter    = aec_cfg->ns_filter;

    aec->vad_cfg.vad_enable = aec_cfg->vad_enable;
    aec->vad_cfg.vad_start_threshold   = aec_cfg->vad_start_threshold;
    aec->vad_cfg.vad_stop_threshold    = aec_cfg->vad_stop_threshold;
    aec->vad_cfg.vad_silence_threshold = aec_cfg->vad_silence_threshold;
    aec->vad_cfg.vad_eng_threshold     = aec_cfg->vad_eng_threshold;

    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_MIC_DELAY, aec_cfg->mic_delay);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EC_DEPTH, aec_cfg->ec_depth);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_REF_SCALE, aec_cfg->ref_scale);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_VOL, aec_cfg->voice_vol);

    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_LEVEL, aec_cfg->ns_level);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_NS_PARA, aec_cfg->ns_para);

    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_DRC, aec_cfg->drc_gain);
    aec_ctrl(aec->aec_ctx, AEC_CTRL_CMD_SET_EC_FILTER, aec_cfg->ec_filter);

    if((aec->vad_cfg.vad_start_threshold !=0 && aec->vad_cfg.vad_stop_threshold != 0xff)
    && (aec->vad_cfg.vad_start_threshold != aec->vad_cfg.vad_stop_threshold))
    {
        aec_vad_thr_mapping(aec->aec_ctx->SPthr, 
                            aec->vad_cfg.vad_start_threshold, 
                            aec->vad_cfg.vad_stop_threshold, 
                            aec->vad_cfg.vad_silence_threshold,
                            aec->vad_cfg.vad_eng_threshold);
    }
    os_printf("[+]%s, ec_depth:%d\n", __func__, aec->aec_cfg.ec_depth);
    audio_element_setdata(aec_algorithm, aec);
	return BK_OK;
}

bk_err_t aec_v3_algorithm_get_config(audio_element_handle_t aec_algorithm, void * aec_config)
{
    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(aec_algorithm);
    if (aec == NULL) {
        BK_LOGE(TAG, "aec is NULL \n");
        return BK_FAIL;
    }
    if (aec_config == NULL) {
        BK_LOGE(TAG, "aec_config is NULL \n");
        return BK_FAIL;
    }
    app_aud_aec_v3_config_t *aec_cfg = (app_aud_aec_v3_config_t *)aec_config;

    aec_cfg->aec_enable = 1;//aec->aec_cfg.enable;

    aec_cfg->mic_delay = aec->aec_cfg.delay_points;    //0x0
    aec_cfg->ec_depth  = aec->aec_cfg.ec_depth;     //0x14
    aec_cfg->ns_type   = aec->aec_cfg.ns_type;
    aec_cfg->ns_level  = aec->aec_cfg.ns_level;
    aec_cfg->ns_para   = aec->aec_cfg.ns_para;
    aec_cfg->ref_scale = aec->aec_cfg.ref_scale;
    aec_cfg->drc_gain  = aec->aec_cfg.drc;
    aec_cfg->voice_vol = aec->aec_cfg.voice_vol;
    aec_cfg->ec_filter = aec->aec_cfg.ec_filter;
    aec_cfg->ns_filter = aec->aec_cfg.ns_filter;

    aec_cfg->vad_enable            = aec->vad_cfg.vad_enable;
    aec_cfg->vad_start_threshold   = aec->vad_cfg.vad_start_threshold;
    aec_cfg->vad_stop_threshold    = aec->vad_cfg.vad_stop_threshold;
    aec_cfg->vad_silence_threshold = aec->vad_cfg.vad_silence_threshold;
    aec_cfg->vad_eng_threshold     = aec->vad_cfg.vad_eng_threshold;
    os_printf("[+]%s, ec_depth:%d\n", __func__, aec->aec_cfg.ec_depth);

    return BK_OK;
}

bk_err_t aec_v3_algorithm_set_user_process(aec_vad_process_fun user_process_fun)
{
    tuya_vad_aec_process_fun = user_process_fun;
}
// Modified by TUYA Start
int aec_v3_algorithm_get_vad_state(audio_element_handle_t aec_algorithm)
{
    aec_v3_algorithm_t *aec = (aec_v3_algorithm_t *)audio_element_getdata(aec_algorithm);
    if (aec == NULL) {
        BK_LOGE(TAG, "aec is NULL \n");
        return VAD_NONE;
    }
    return aec->vad_state;
}
// Modified by TUYA End
