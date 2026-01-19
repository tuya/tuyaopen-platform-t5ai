/*
 * Copyright (c) 2024 Beken Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <components/bk_audio/audio_pipeline/audio_common.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <components/bk_audio/audio_streams/frame_array_stream.h>
#include <components/bk_audio/audio_decoders/opus_dec.h>
#include "opus_dec_test_input_data.c"

#define TAG "OPUS_DEC_TEST"

// Define TEST_CHECK_NULL if not already defined
#ifndef TEST_CHECK_NULL
#define TEST_CHECK_NULL(ptr) \
    if (!(ptr)) { \
        BK_LOGE(TAG, "Failed to allocate %s at line %d\n", #ptr, __LINE__); \
        return BK_FAIL; \
    }
#endif

bk_err_t adk_opus_dec_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t opus_dec, frame_array_stream, uart_stream;
    audio_event_iface_handle_t evt;
    uint32_t total_data_size = 0;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    BK_LOGI(TAG, "start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    frame_array_stream_cfg_t frame_array_cfg = DEFAULT_FRAME_ARRAY_STREAM_CONFIG();
    frame_array_cfg.type = AUDIO_STREAM_READER;
    frame_array_stream = frame_array_stream_init(&frame_array_cfg);
    TEST_CHECK_NULL(frame_array_stream);

    for(int i = 0; i < sizeof(opus_dec_data_len)/sizeof(uint16_t); i++)
    {
        total_data_size += opus_dec_data_len[i];
    }
    
    frame_array_stream_set_data(frame_array_stream, (uint8_t *)opus_dec_data, total_data_size,(uint16_t *)opus_dec_data_len,sizeof(opus_dec_data_len)/sizeof(uint16));

    opus_dec_cfg_t opus_dec_cfg = DEFAULT_OPUS_DEC_CONFIG();
    opus_dec = opus_dec_init(&opus_dec_cfg);
    TEST_CHECK_NULL(opus_dec);

    uart_stream_cfg_t uart_cfg = DEFAULT_UART_STREAM_CONFIG();
    uart_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream = uart_stream_init(&uart_cfg);
    TEST_CHECK_NULL(uart_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, frame_array_stream, "frame_array_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, opus_dec, "opus_dec"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uart_stream, "uart_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"frame_array_stream", "opus_dec", "uart_stream"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    TEST_CHECK_NULL(evt);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "pipeline set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: listening event ----------\n");
    while (1)
    {
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && msg.source == uart_stream
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    rtos_dump_stack_memory_usage();
    GLOBAL_INT_RESTORE();

    BK_LOGD(TAG, "--------- step8: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, frame_array_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, opus_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(frame_array_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(opus_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- opus dec test complete ----------\n");

    return BK_OK;
}
