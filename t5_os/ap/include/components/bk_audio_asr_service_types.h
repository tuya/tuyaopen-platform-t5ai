#pragma once

#include <components/bk_audio/audio_streams/raw_stream.h>

#include <components/bk_audio/audio_algorithms/rsp_algorithm.h>
#include "avdk_types.h"

#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_thread.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/raw_stream.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * asr status
 */
typedef enum
{
    ASR_STA_NONE = 0,
    ASR_STA_IDLE,
    ASR_STA_RUNNING,
    ASR_STA_STOPPING,
    ASR_STA_STOPED,
} asr_sta_t;

/**
 * asr event
 */
typedef enum
{
    ASR_EVT_NONE = 0,
    ASR_EVT_MIC_NOT_SUPPORT,
    ASR_EVT_SPK_NOT_SUPPORT,
    ASR_EVT_ERROR_UNKNOW,
    ASR_EVT_STOP,
} asr_evt_t;


typedef bk_err_t (*asr_event_handle)(asr_evt_t, void *, void *);

typedef struct
{
    uint32_t                read_pool_size;     /*!< the size(byte) of pool save mic data that has been encode */

	bool asr_en;
	bool asr_rsp_en;
	uint32_t asr_sample_rate;
	union
	{
		rsp_algorithm_cfg_t rsp_alg_cfg;
		uint32_t reserved;
	}rsp_cfg;
    asr_event_handle      event_handle;   /*!< asr event handle callback */
    void *                args;           /*!< the parameter of event_handle func */
} asr_cfg_t;

struct asr
{
    audio_element_handle_t  mic_str;            /**< mic stream handle */

    bool                    asr_en;            /**< asr enable handle */
    bool                    asr_rsp_en;        /**< the asr src need to resample */
    audio_pipeline_handle_t asr_pipeline;
    audio_element_handle_t  asr_raw_read;
    audio_element_handle_t  asr_rsp;
    audio_port_handle_t     asr_in_rb;
    audio_event_iface_handle_t asr_evt;

    asr_sta_t             status;             /**< asr handle status */
    asr_event_handle      event_handle;       /**< asr event handle callback */
    void *                args;               /**< the parameter of event_handle func */

    beken_thread_t          listener_task_hdl;
    beken_queue_t           listener_msg_que;
    beken_semaphore_t       listener_sem;
    bool                    listener_is_running;
};

typedef struct asr * asr_handle_t;

#define ASR_BY_ONBOARD_MIC_SPK_CFG_DEFAULT() {                  \
    .asr_en = 0,                                                \
    .asr_rsp_en = 0,                                            \
    .asr_sample_rate = 16000,                                   \
    .rsp_cfg.rsp_alg_cfg = {                                    \
        .task_stack = RSP_ALGORITHM_TASK_STACK,                 \
        .task_core  = RSP_ALGORITHM_TASK_CORE,                  \
        .task_prio  = RSP_ALGORITHM_TASK_PRIO,                  \
        .rsp_cfg = {                                            \
            .complexity   = RSP_ALGORITHM_COMPLEXITY,           \
            .src_ch       = RSP_ALGORITHM_SRC_CH,               \
            .dest_ch      = RSP_ALGORITHM_DEST_CH,              \
            .src_bits     = RSP_ALGORITHM_SRC_BITS,             \
            .dest_bits    = RSP_ALGORITHM_DEST_BITS,            \
            .src_rate     = RSP_ALGORITHM_SRC_RATE,             \
            .dest_rate    = RSP_ALGORITHM_DEST_RATE,            \
            .down_ch_idx  = RSP_ALGORITHM_DOWN_CH_IDX,          \
        },                                                      \
        .out_block_num = 4,                                     \
        .multi_out_port_num = 0,                                \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

#define ASR_BY_UAC_MIC_SPK_CFG_DEFAULT() {                     \
    .asr_en = 0,                                               \
    .asr_rsp_en = 0,                                           \
    .asr_sample_rate = 16000,                                  \
    .rsp_cfg.rsp_alg_cfg = {                                   \
        .task_stack = RSP_ALGORITHM_TASK_STACK,                \
        .task_core  = RSP_ALGORITHM_TASK_CORE,                 \
        .task_prio  = RSP_ALGORITHM_TASK_PRIO,                 \
        .rsp_cfg = {                                           \
            .complexity   = RSP_ALGORITHM_COMPLEXITY,          \
            .src_ch       = RSP_ALGORITHM_SRC_CH,              \
            .dest_ch      = RSP_ALGORITHM_DEST_CH,             \
            .src_bits     = RSP_ALGORITHM_SRC_BITS,            \
            .dest_bits    = RSP_ALGORITHM_DEST_BITS,           \
            .src_rate     = RSP_ALGORITHM_SRC_RATE,            \
            .dest_rate    = RSP_ALGORITHM_DEST_RATE,           \
            .down_ch_idx  = RSP_ALGORITHM_DOWN_CH_IDX,         \
        },                                                     \
        .out_block_num = 4,                                    \
        .multi_out_port_num = 0,                               \
    },                                                         \
    .event_handle = NULL,                                      \
    .args = NULL,                                              \
}

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */