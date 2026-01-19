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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <components/bk_audio/audio_pipeline/ringbuf.h>


#define TAG  "EQ_ALGORITHM"

//#define EQ_DEBUG   //GPIO debug

#ifdef EQ_DEBUG

#define EQ_ALGORITHM_START()                       do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define EQ_ALGORITHM_END()                         do { GPIO_DOWN(32); } while (0)

#define EQ_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define EQ_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define EQ_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define EQ_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define EQ_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define EQ_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define EQ_ALGORITHM_START()
#define EQ_ALGORITHM_END()

#define EQ_PROCESS_START()
#define EQ_PROCESS_END()

#define EQ_INPUT_START()
#define EQ_INPUT_END()

#define EQ_OUTPUT_START()
#define EQ_OUTPUT_END()

#endif


/* AEC data dump depends on debug utils, so must config CONFIG_ADK_UTILS=y when dump aec data. */
#if CONFIG_ADK_UTILS

//#define EQ_DATA_DUMP

#ifdef EQ_DATA_DUMP

/* dump aec data by uart or tfcard, only choose one */
#define EQ_DATA_DUMP_BY_UART
//#define EQ_DATA_DUMP_BY_TFCARD       /* you must sure CONFIG_FATFS=y */

#ifdef EQ_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static uart_util_handle_t g_eq_uart_util = {0};
#define EQ_DATA_DUMP_UART_ID            (1)
#define EQ_DATA_DUMP_UART_BAUD_RATE     (2000000)
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
#include <components/bk_audio/audio_utils/vfs_util.h>
static struct vfs_util g_eq_vfs_util_in = {0};
static struct vfs_util g_eq_vfs_util_out = {0};

#define EQ_DATA_DUMP_VFS_IN_NAME     "/sd0/eq_in.pcm"
#define EQ_DATA_DUMP_VFS_OUT_NAME    "/sd0/eq_out.pcm"
#endif

#endif  //EQ_DATA_DUMP

#endif  //CONFIG_ADK_UTILS

typedef struct eq_algorithm
{
    eq_cfg_t      eq_cfg;
    eq_handle_t   eq_handle;
    app_eq_load_t eq_load;
} eq_algorithm_t;


#ifdef EQ_DATA_DUMP

static void eq_data_dump_open(void)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_create(&g_eq_uart_util,EQ_DATA_DUMP_UART_ID, EQ_DATA_DUMP_UART_BAUD_RATE);
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_create(&g_eq_vfs_util_in, EQ_DATA_DUMP_VFS_IN_NAME);
    vfs_util_create(&g_eq_vfs_util_out, EQ_DATA_DUMP_VFS_OUT_NAME);
#endif
}

static void eq_data_dump_close(void)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_destroy(&g_eq_uart_util);
    g_eq_uart_util = NULL;
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_destroy(&g_eq_vfs_util_in);
    vfs_util_destroy(&g_eq_vfs_util_out);
#endif
}

static void eq_data_dump_in_data(void *data_buf, uint32_t len)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_tx_data(&g_eq_uart_util, data_buf, len);
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_eq_vfs_util_in, data_buf, len);
#endif
}

static void eq_data_dump_out_data(void *data_buf, uint32_t len)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_tx_data(g_eq_uart_util, data_buf, len);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_eq_vfs_util_out, data_buf, len);
#endif
}

#define EQ_DATA_DUMP_OPEN()                        eq_data_dump_open()
#define EQ_DATA_DUMP_CLOSE()                       eq_data_dump_close()
#define EQ_DATA_DUMP_IN_DATA(data_buf, len)        eq_data_dump_in_data(data_buf, len)
#define EQ_DATA_DUMP_OUT_DATA(data_buf, len)       eq_data_dump_out_data(data_buf, len)

#else

#define EQ_DATA_DUMP_OPEN()
#define EQ_DATA_DUMP_CLOSE()
#define EQ_DATA_DUMP_IN_DATA(data_buf, len)
#define EQ_DATA_DUMP_OUT_DATA(data_buf, len)

#endif  //EQ_DATA_DUMP

static bk_err_t _eq_algorithm_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);

    eq->eq_handle = eq_create(&eq->eq_cfg);

    if (!eq->eq_handle)
    {
        BK_LOGE(TAG, "%s, %d, eq element create fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    return BK_OK;
}

static bk_err_t _eq_algorithm_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    return BK_OK;
}

static int _eq_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] %s, in_len: %d \n", audio_element_get_tag(self), __func__, in_len);
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);

    EQ_PROCESS_START();

    EQ_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    BK_LOGV(TAG, "[%s] r_size=%d, in_len=%d \n",audio_element_get_tag(self), r_size, in_len);
    if (r_size != in_len)
    {
        BK_LOGE(TAG, "eq input warning: r_size=%d, in_len=%d \n", r_size, in_len);
    }

    EQ_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        EQ_DATA_DUMP_IN_DATA(in_buffer, r_size);
        EQ_ALGORITHM_START();

        eq_process(eq->eq_handle, (int16_t *)in_buffer, r_size/2);

        EQ_ALGORITHM_END();

        EQ_DATA_DUMP_OUT_DATA(in_buffer, r_size);

        EQ_OUTPUT_START();
        w_size = audio_element_output(self, in_buffer, r_size);
        EQ_OUTPUT_END();

        /* write data to multiple audio port */
        /* unblock write, and not check write result */
        //TODO
        audio_element_multi_output(self, in_buffer, r_size, 0);
    }
    else
    {
        w_size = r_size;
    }

    EQ_PROCESS_END();
    BK_LOGV(TAG, "[%s] w_size=%d\n",audio_element_get_tag(self), w_size);

    return w_size;
}

static bk_err_t _eq_algorithm_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);
    eq_destroy(eq->eq_handle);

    EQ_DATA_DUMP_CLOSE();

    return BK_OK;
}

audio_element_handle_t eq_algorithm_init(eq_algorithm_cfg_t *config)
{
    audio_element_handle_t el;

    /* check config */
    if (!config->eq_cal_para.eq_en)
    {
        BK_LOGE(TAG, "check config->eq_cal_para.eq_en:%d fail \n",config->eq_cal_para.eq_en);
        return NULL;
    }

    eq_algorithm_t *eq_alg = audio_calloc(1, sizeof(eq_algorithm_t));
    AUDIO_MEM_CHECK(TAG, eq_alg, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _eq_algorithm_open;
    cfg.close = _eq_algorithm_close;
    cfg.seek = NULL;
    cfg.process = _eq_algorithm_process;
    cfg.destroy = _eq_algorithm_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;

    cfg.out_block_size = config->eq_frame_size*config->eq_chl_num;
    cfg.out_block_num = config->out_block_num;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.buffer_len = config->eq_frame_size*config->eq_chl_num;

    cfg.tag = "eq_algorithm";
    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _eq_algorithm_init_exit);

    /* config */
    eq_alg->eq_cfg.chl_num      = config->eq_chl_num;
    eq_alg->eq_cfg.eq_gain      = config->eq_cal_para.globle_gain;
    eq_alg->eq_cfg.eq_valid_num = config->eq_cal_para.filters;
    os_memcpy(&eq_alg->eq_cfg.eq_para ,&config->eq_cal_para.eq_para,sizeof(eq_para_t)*config->eq_cal_para.filters);
    os_memcpy(&eq_alg->eq_load, &config->eq_cal_para.eq_load, sizeof(app_eq_load_t));

    audio_element_setdata(el, eq_alg);

    EQ_DATA_DUMP_OPEN();

    return el;
_eq_algorithm_init_exit:
    audio_free(eq_alg);
    return NULL;
}

bk_err_t eq_algorithm_set_config(audio_element_handle_t eq_algorithm, void * eq_config)
{
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(eq_algorithm);
    app_aud_eq_config_t *eq_cfg = (app_aud_eq_config_t *)eq_config;

    eq->eq_cfg.eq_gain      = eq_cfg->globle_gain;
    eq->eq_cfg.eq_valid_num = eq_cfg->filters;
    os_memcpy(&eq->eq_cfg.eq_para, &eq_cfg->eq_para, sizeof(eq_para_t)*eq_cfg->filters);

    os_memcpy(&eq->eq_load, &eq_cfg->eq_load, sizeof(app_eq_load_t));

    eq_destroy(eq->eq_handle);
    eq->eq_handle = eq_create(&eq->eq_cfg);
    if (!eq->eq_handle)
    {
        BK_LOGE(TAG, "%s, %d, eq element create fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    audio_element_setdata(eq_algorithm, eq);
    return BK_OK;
}

bk_err_t eq_algorithm_get_config(audio_element_handle_t eq_algorithm, void * eq_load)
{
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(eq_algorithm);
    os_memcpy(eq_load, &eq->eq_load, sizeof(app_eq_load_t));
    return BK_OK;
}
