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
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>

#define TAG  "RAW_STR"

//#define RAW_WRITE_DEBUG   //RAW write GPIO debug

#ifdef RAW_WRITE_DEBUG

#define RAW_WRITE_START()           do { GPIO_DOWN(36); GPIO_UP(36);} while (0)
#define RAW_WRITE_END()             do { GPIO_DOWN(36); } while (0)

#else

#define RAW_WRITE_START()
#define RAW_WRITE_END()

#endif

//#define RAW_READ_DEBUG   //RAW read GPIO debug

#ifdef RAW_READ_DEBUG

#define RAW_READ_START()           do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define RAW_READ_END()             do { GPIO_DOWN(35); } while (0)

#else

#define RAW_READ_START()
#define RAW_READ_END()

#endif

/* read raw data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count read raw data. */
#if CONFIG_ADK_UTILS

#define RAW_READ_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef RAW_READ_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t raw_read_count_util = {0};
#define RAW_READ_DATA_COUNT_INTERVAL     (1000 * 4)
#define RAW_READ_DATA_COUNT_TAG          "RAW_READ"

#define RAW_READ_DATA_COUNT_OPEN()               count_util_create(&raw_read_count_util, RAW_READ_DATA_COUNT_INTERVAL, RAW_READ_DATA_COUNT_TAG)
#define RAW_READ_DATA_COUNT_CLOSE()              count_util_destroy(&raw_read_count_util)
#define RAW_READ_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&raw_read_count_util, size)

#else

#define RAW_READ_DATA_COUNT_OPEN()
#define RAW_READ_DATA_COUNT_CLOSE()
#define RAW_READ_DATA_COUNT_ADD_SIZE(size)

#endif  //RAW_READ_DATA_COUNT

/* write raw data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count write raw data. */
#if CONFIG_ADK_UTILS

#define RAW_WRITE_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef RAW_WRITE_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t raw_write_count_util = {0};
#define RAW_WRITE_DATA_COUNT_INTERVAL     (1000 * 4)
#define RAW_WRITE_DATA_COUNT_TAG          "RAW_WRITE"

#define RAW_WRITE_DATA_COUNT_OPEN()               count_util_create(&raw_write_count_util, RAW_WRITE_DATA_COUNT_INTERVAL, RAW_WRITE_DATA_COUNT_TAG)
#define RAW_WRITE_DATA_COUNT_CLOSE()              count_util_destroy(&raw_write_count_util)
#define RAW_WRITE_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&raw_write_count_util, size)

#else

#define RAW_WRITE_DATA_COUNT_OPEN()
#define RAW_WRITE_DATA_COUNT_CLOSE()
#define RAW_WRITE_DATA_COUNT_ADD_SIZE(size)

#endif  //RAW_WRITE_DATA_COUNT

/* dump raw read stream output data by uart */
//#define RAW_READ_DATA_DUMP_BY_UART

#ifdef RAW_READ_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_raw_read_uart_util = {0};
#define RAW_READ_DATA_DUMP_UART_ID            (1)
#define RAW_READ_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define RAW_READ_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_raw_read_uart_util, RAW_READ_DATA_DUMP_UART_ID, RAW_READ_DATA_DUMP_UART_BAUD_RATE)
#define RAW_READ_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_raw_read_uart_util)
#define RAW_READ_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_raw_read_uart_util, data_buf, len)

#else

#define RAW_READ_DATA_DUMP_BY_UART_OPEN()
#define RAW_READ_DATA_DUMP_BY_UART_CLOSE()
#define RAW_READ_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //RAW_READ_DATA_DUMP_BY_UART


typedef struct raw_stream
{
    audio_stream_type_t type;
} raw_stream_t;

int raw_stream_read(audio_element_handle_t pipeline, char *buffer, int len)
{
    RAW_READ_START();

    int ret = audio_element_input(pipeline, buffer, len);
    if (ret == AEL_IO_DONE || ret == AEL_IO_OK)
    {
        audio_element_report_status(pipeline, AEL_STATUS_STATE_FINISHED);
    }
    else if (ret < 0)
    {
        audio_element_report_status(pipeline, AEL_STATUS_STATE_STOPPED);
    }
    else
    {
        RAW_READ_DATA_DUMP_BY_UART_DATA(buffer, len);
    }

    RAW_READ_END();

    RAW_READ_DATA_COUNT_ADD_SIZE(ret);

    return ret;
}

int raw_stream_write(audio_element_handle_t pipeline, char *buffer, int len)
{
    RAW_WRITE_START();

    int ret = audio_element_output(pipeline, buffer, len);
    if (ret == AEL_IO_DONE || ret == AEL_IO_OK)
    {
        audio_element_report_status(pipeline, AEL_STATUS_STATE_FINISHED);
    }
    else if (ret < 0)
    {
        audio_element_report_status(pipeline, AEL_STATUS_STATE_STOPPED);
    }

    RAW_WRITE_END();

    RAW_WRITE_DATA_COUNT_ADD_SIZE(ret);

    return ret;
}

static bk_err_t _raw_destroy(audio_element_handle_t self)
{
    raw_stream_t *raw = (raw_stream_t *)audio_element_getdata(self);

    if (raw->type == AUDIO_STREAM_READER)
    {
        RAW_READ_DATA_COUNT_CLOSE();
    }

    if (raw->type == AUDIO_STREAM_WRITER)
    {
        RAW_WRITE_DATA_COUNT_CLOSE();
    }

    RAW_READ_DATA_DUMP_BY_UART_CLOSE();

    audio_free(raw);

    return BK_OK;
}

audio_element_handle_t raw_stream_init(raw_stream_cfg_t *config)
{
    raw_stream_t *raw = audio_calloc(1, sizeof(raw_stream_t));
    AUDIO_MEM_CHECK(TAG, raw, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.task_stack = -1;    // Not need creat task
    cfg.destroy = _raw_destroy;
    cfg.tag = "raw";
    //cfg.out_type = PORT_TYPE_RB;
    cfg.out_type = config->output_port_type;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    raw->type = config->type;
    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {audio_free(raw); return NULL;});
    audio_element_setdata(el, raw);

    if (raw->type == AUDIO_STREAM_READER)
    {
        RAW_READ_DATA_COUNT_OPEN();
    }

    if (raw->type == AUDIO_STREAM_WRITER)
    {
        RAW_WRITE_DATA_COUNT_OPEN();
    }

    /* set read data timeout */
    //  audio_element_set_input_timeout(el, 2000);  //15 / portTICK_RATE_MS
    //  audio_element_set_output_timeout(el, 2000); //15 / portTICK_RATE_MS

    BK_LOGV(TAG, "stream init,el:%p", el);

    RAW_READ_DATA_DUMP_BY_UART_OPEN();

    return el;
}

