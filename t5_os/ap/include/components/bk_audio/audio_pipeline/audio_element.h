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

#ifndef _AUDIO_ELEMENT_H_
#define _AUDIO_ELEMENT_H_

#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    AEL_IO_OK           = BK_OK,
    AEL_IO_FAIL         = BK_FAIL,
    AEL_IO_DONE         = -2,
    AEL_IO_ABORT        = -3,
    AEL_IO_TIMEOUT      = -4,
    AEL_PROCESS_FAIL    = -5,
} audio_element_err_t;

/**
 * @brief Audio element state
 */
typedef enum
{
    AEL_STATE_NONE          = 0,
    AEL_STATE_INIT          = 1,
    AEL_STATE_INITIALIZING  = 2,
    AEL_STATE_RUNNING       = 3,
    AEL_STATE_PAUSED        = 4,
    AEL_STATE_STOPPED       = 5,
    AEL_STATE_FINISHED      = 6,
    AEL_STATE_ERROR         = 7
} audio_element_state_t;

/**
 * Audio element action command, process on dispatcher
 */
typedef enum
{
    AEL_MSG_CMD_NONE                = 0,
    // AEL_MSG_CMD_ERROR               = 1,
    AEL_MSG_CMD_FINISH              = 2,
    AEL_MSG_CMD_STOP                = 3,
    AEL_MSG_CMD_PAUSE               = 4,
    AEL_MSG_CMD_RESUME              = 5,
    AEL_MSG_CMD_DESTROY             = 6,
    // AEL_MSG_CMD_CHANGE_STATE        = 7,
    AEL_MSG_CMD_REPORT_STATUS       = 8,
    AEL_MSG_CMD_REPORT_MUSIC_INFO   = 9,
    AEL_MSG_CMD_REPORT_CODEC_FMT    = 10,
    AEL_MSG_CMD_REPORT_POSITION     = 11,
} audio_element_msg_cmd_t;

/**
 * Audio element status report
 */
typedef enum
{
    AEL_STATUS_NONE                     = 0,
    AEL_STATUS_ERROR_OPEN               = 1,
    AEL_STATUS_ERROR_INPUT              = 2,
    AEL_STATUS_ERROR_PROCESS            = 3,
    AEL_STATUS_ERROR_OUTPUT             = 4,
    AEL_STATUS_ERROR_CLOSE              = 5,
    AEL_STATUS_ERROR_TIMEOUT            = 6,
    AEL_STATUS_ERROR_UNKNOWN            = 7,
    AEL_STATUS_INPUT_DONE               = 8,
    AEL_STATUS_INPUT_BUFFERING          = 9,
    AEL_STATUS_OUTPUT_DONE              = 10,
    AEL_STATUS_OUTPUT_BUFFERING         = 11,
    AEL_STATUS_STATE_RUNNING            = 12,
    AEL_STATUS_STATE_PAUSED             = 13,
    AEL_STATUS_STATE_STOPPED            = 14,
    AEL_STATUS_STATE_FINISHED           = 15,
    AEL_STATUS_MOUNTED                  = 16,
    AEL_STATUS_UNMOUNTED                = 17,
} audio_element_status_t;

typedef struct audio_element *audio_element_handle_t;

/**
 * @brief Audio Element informations
 */
typedef struct
{
    int sample_rates;                           /*!< Sample rates in Hz */
    int channels;                               /*!< Number of audio channel, mono is 1, stereo is 2 */
    int bits;                                   /*!< Bit wide (8, 16, 24, 32 bits) */
    int bps;                                    /*!< Bit per second */
    int64_t byte_pos;                           /*!< The current position (in bytes) being processed for an element */
    int64_t total_bytes;                        /*!< The total bytes for an element */
    int duration;                               /*!< The duration for an element (optional) */
    char *uri;                                  /*!< URI (optional) */
    bk_codec_type_t codec_fmt;                  /*!< Music format (optional) */
} audio_element_info_t;

#define AUDIO_ELEMENT_INFO_DEFAULT()    { \
    .sample_rates = 44100,                \
    .channels = 2,                        \
    .bits = 16,                           \
    .bps = 0,                             \
    .byte_pos = 0,                        \
    .total_bytes = 0,                     \
    .duration = 0,                        \
    .uri = NULL,                          \
    .codec_fmt = BK_CODEC_TYPE_UNKNOW     \
    }

typedef bk_err_t (*el_io_func)(audio_element_handle_t self);
typedef int (*process_func)(audio_element_handle_t self, char *el_buffer, int el_buf_len);
//typedef int (*stream_func)(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context);
typedef bk_err_t (*event_cb_func)(audio_element_handle_t el, audio_event_iface_msg_t *event, void *ctx);
typedef bk_err_t (*ctrl_func)(audio_element_handle_t self, void *in_data, int in_size, void *out_data, int *out_size);

/**
 * @brief Audio Element configurations.
 *        Each Element at startup will be a self-running task.
 *        These tasks will execute the callback open -> [loop: read -> process -> write] -> close.
 *        These callback functions are provided by the user corresponding to this configuration.
 *
 */
typedef struct
{
    el_io_func          open;               /*!< Open callback function */
    ctrl_func           seek;               /*!< Seek callback function */
    process_func        process;            /*!< Process callback function */
    el_io_func          close;              /*!< Close callback function */
    el_io_func          destroy;            /*!< Destroy callback function */
    port_stream_func    read;               /*!< Read callback function */
    port_stream_func    write;              /*!< Write callback function */
    int                 buffer_len;         /*!< Buffer length use for an Element */
    int                 task_stack;         /*!< Element task stack */
    int                 task_prio;          /*!< Element task priority (based on freeRTOS priority) */
    int                 task_core;          /*!< Element task running in core (0 or 1) */
    int                 out_block_size;     /*!< Output block size */
    int                 out_block_num;      /*!< Output block numbers */
    void                *data;              /*!< User context */
    const char          *tag;               /*!< Element tag */
    int                 multi_in_port_num;  /*!< The number of multiple input audio port */
    int                 multi_out_port_num; /*!< The number of multiple output audio port */
    port_type_t         in_type;            /*!< The type of input audio port */
    port_type_t         out_type;           /*!< The type of output audio port */
} audio_element_cfg_t;

#define DEFAULT_ELEMENT_FRAMEBUF_SIZE   (1024)
#define DEFAULT_ELEMENT_FRAMEBUF_NUM    (4)


#define DEFAULT_ELEMENT_RINGBUF_SIZE    (4*1024)
#define DEFAULT_ELEMENT_BUFFER_LENGTH   (2*1024)
#define DEFAULT_ELEMENT_STACK_SIZE      (2*1024)
#define DEFAULT_ELEMENT_TASK_PRIO       (5)
#define DEFAULT_ELEMENT_TASK_CORE       (0)

#define DEFAULT_AUDIO_ELEMENT_CONFIG() {                \
    .buffer_len         = DEFAULT_ELEMENT_BUFFER_LENGTH,\
    .task_stack         = DEFAULT_ELEMENT_STACK_SIZE,   \
    .task_prio          = DEFAULT_ELEMENT_TASK_PRIO,    \
    .task_core          = DEFAULT_ELEMENT_TASK_CORE,    \
    .multi_in_port_num  = 0,                            \
    .multi_out_port_num = 0,                            \
    .out_block_num      = 1,                            \
}

/**
 * @brief      Initialize audio element with config.
 *
 * @param[in]  config  The configuration
 *
 * @return
 *     - audio_elemenent handle object
 *     - NULL
 */
audio_element_handle_t audio_element_init(audio_element_cfg_t *config);

/**
 * @brief      Destroy audio element handle object, stop, clear, deletel all.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_deinit(audio_element_handle_t el);

/**
 * @brief      Set context data to element handle object.
 *             It can be retrieved by calling `audio_element_getdata`.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  data  The data pointer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_setdata(audio_element_handle_t el, void *data);

/**
 * @brief      Get context data from element handle object.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     data pointer
 */
void *audio_element_getdata(audio_element_handle_t el);

/**
 * @brief      Set elemenet tag name, or clear if tag = NULL.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  tag   The tag name pointer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_tag(audio_element_handle_t el, const char *tag);

/**
 * @brief      Get element tag name.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     Element tag name pointer
 */
char *audio_element_get_tag(audio_element_handle_t el);

/**
 * @brief      Set audio element infomation.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  info  The information pointer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_setinfo(audio_element_handle_t el, audio_element_info_t *info);

/**
 * @brief      Get audio element infomation.
 *
 * @param[in]      el    The audio element handle
 * @param[in,out]  info  The information pointer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_getinfo(audio_element_handle_t el, audio_element_info_t *info);

/**
 * @brief      Set audio element URI.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  uri   The uri pointer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_uri(audio_element_handle_t el, const char *uri);

/**
 * @brief      Get audio element URI.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     URI pointer
 */
char *audio_element_get_uri(audio_element_handle_t el);

/**
 * @brief      Start Audio Element.
 *             With this function, audio_element will start as freeRTOS task,
 *             and put the task into 'PAUSED' state.
 *             Note: Element does not actually start when this function returns
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_run(audio_element_handle_t el);

/**
 * @brief      Terminate Audio Element.
 *             With this function, audio_element will exit the task function.
 *             Note: this API only sends request. It does not actually terminate immediately when this function returns.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_terminate(audio_element_handle_t el);

/**
 * @brief      Terminate Audio Element with specific ticks for timeout.
 *             With this function, audio_element will exit the task function.
 *             Note: this API only sends request. It does not actually terminate immediately when this function returns.
 *
 * @param[in]  el               The audio element handle
 * @param[in]  ticks_to_wait    The maximum amount of time to blocking
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_terminate_with_ticks(audio_element_handle_t el, TickType_t ticks_to_wait);

/**
 * @brief      Request stop of the Audio Element.
 *             After receiving the stop request, the element will ignore the actions being performed
 *             (read/write, wait for the ringbuffer ...) and close the task, reset the state variables.
 *             Note: this API only sends requests, Element does not actually stop when this function returns
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_stop(audio_element_handle_t el);

/**
 * @brief      After the `audio_element_stop` function is called, the Element task will perform some abort procedures.
 *             This function will be blocked (Time is DEFAULT_MAX_WAIT_TIME) until Element Task has done and exit.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK, Success
 *     - BK_FAIL, The state is not AEL_STATE_RUNNING
 *     - BK_ERR_ADK_TIMEOUT, Timeout
 */
bk_err_t audio_element_wait_for_stop(audio_element_handle_t el);

/**
 * @brief      After the `audio_element_stop` function is called, the Element task will perform some abort procedures.
 *             The maximum amount of time should block waiting for Element task has stopped.
 *
 * @param[in]  el               The audio element handle
 * @param[in]  ticks_to_wait    The maximum amount of time to wait for stop
 *
 * @return
 *     - BK_OK, Success
 *     - BK_FAIL, The state is not AEL_STATE_RUNNING
 *     - BK_ERR_ADK_TIMEOUT, Timeout
 */
bk_err_t audio_element_wait_for_stop_ms(audio_element_handle_t el, TickType_t ticks_to_wait);

/**
 * @brief      Request audio Element enter 'PAUSE' state.
 *             In this state, the task will wait for any event
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_pause(audio_element_handle_t el);

/**
 * @brief      Request audio Element enter 'RUNNING' state.
 *             In this state, the task listens to events and invokes the callback functions.
 *             At the same time it will wait until the size/total_size of the output ringbuffer is greater than or equal to `wait_for_rb_threshold`.
 *             If the timeout period has been exceeded and ringbuffer output has not yet reached `wait_for_rb_threshold` then the function will return.
 *
 * @param[in]  el                     The audio element handle
 * @param[in]  wait_for_rb_threshold  The wait for rb threshold (0 .. 1)
 * @param[in]  timeout                The timeout
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_resume(audio_element_handle_t el, float wait_for_rb_threshold, TickType_t timeout);

/**
 * @brief      This function will add a `listener` to listen to all events from audio element `el`.
 *             Any event from el->external_event will be send to the `listener`.
 *
 * @param[in]  el           The audio element handle
 * @param[in]  listener     The event will be listen to
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_msg_set_listener(audio_element_handle_t el, audio_event_iface_handle_t listener);

/**
 * @brief      This function will add a `callback` to be called from audio element `el`.
 *             Any event to caller will cause to call callback function.
 *
 * @param[in]  el           The audio element handle
 * @param[in]  cb_func      The callback function
 * @param[in]  ctx          Caller context
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_event_callback(audio_element_handle_t el, event_cb_func cb_func, void *ctx);

/**
 * @brief      Remove listener out of el.
 *             No new events will be sent to the listener.
 *
 * @param[in]  el        The audio element handle
 * @param[in]  listener  The listener
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_msg_remove_listener(audio_element_handle_t el, audio_event_iface_handle_t listener);

/**
 * @brief      Set Element input ringbuffer
 *
 * @param[in]  el    The audio element handle
 * @param[in]  port  The audio port handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_input_port(audio_element_handle_t el, audio_port_handle_t port);

/**
 * @brief      Get Element input audio port.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     audio_port_handle_t
 */
audio_port_handle_t audio_element_get_input_port(audio_element_handle_t el);

/**
 * @brief      Set Element output ringbuffer.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  port  The audio port handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_output_port(audio_element_handle_t el, audio_port_handle_t port);

/**
 * @brief      Get Element output audio port.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     audio_port_handle_t
 */
audio_port_handle_t audio_element_get_output_port(audio_element_handle_t el);

/**
 * @brief      Get current Element state.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     audio_element_state_t
 */
audio_element_state_t audio_element_get_state(audio_element_handle_t el);

/**
 * @brief      If the element is requesting data from the input audio port, this function forces it to abort.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_abort_input_port(audio_element_handle_t el);

/**
 * @brief      If the element is waiting to write data to the output audio port, this function forces it to abort.
 *
 * @param[in]  el   The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_abort_output_port(audio_element_handle_t el);

/**
 * @brief      This function will wait until the sizeof the output ringbuffer is greater than or equal to `size_expect`.
 *             If the timeout period has been exceeded and ringbuffer output has not yet reached `size_expect`
 *             then the function will return `BK_FAIL`
 *
 * @param[in]  el           The audio element handle
 * @param[in]  size_expect  The size expect
 * @param[in]  timeout      The timeout
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_wait_for_buffer(audio_element_handle_t el, int size_expect, TickType_t timeout);

/**
 * @brief      Element will sendout event (status) to event by this function.
 *
 * @param[in]  el      The audio element handle
 * @param[in]  status  The status
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_report_status(audio_element_handle_t el, audio_element_status_t status);

/**
 * @brief      Element will sendout event (information) to event by this function.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_report_info(audio_element_handle_t el);

/**
 * @brief      Element will sendout event (codec format) to event by this function.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_report_codec_fmt(audio_element_handle_t el);

/**
 * @brief      Element will sendout event with a duplicate information by this function.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 *     - BK_ERR_ADK_NO_MEM
 */
bk_err_t audio_element_report_pos(audio_element_handle_t el);

/**
 * @brief      Set input read timeout (default is `portMAX_DELAY`).
 *
 * @param[in]  el       The audio element handle
 * @param[in]  timeout  The timeout
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_input_timeout(audio_element_handle_t el, TickType_t timeout);

/**
 * @brief      Set output read timeout (default is `portMAX_DELAY`).
 *
 * @param[in]  el       The audio element handle
 * @param[in]  timeout  The timeout
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_output_timeout(audio_element_handle_t el, TickType_t timeout);

/**
 * @brief      Reset input port.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_reset_input_port(audio_element_handle_t el);

/**
 * @brief      Reset output port.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_reset_output_port(audio_element_handle_t el);

/**
 * @brief      Call this function to provide Element input data.
 *             Depending on setup using ringbuffer or function callback, Element invokes read ringbuffer, or calls read callback funtion.
 *
 * @param[in]  el            The audio element handle
 * @param[in]  buffer        The buffer pointer
 * @param[in]  wanted_size   The wanted size
 *
 * @return
 *        - > 0 number of bytes produced
 *        - <=0 audio_element_err_t
 */
int audio_element_input(audio_element_handle_t el, char *buffer, int wanted_size);

/**
 * @brief      Call this function to sendout Element output data.
 *             Depending on setup using ringbuffer or function callback, Element will invoke write to ringbuffer, or call write callback funtion.
 *
 * @param[in]  el          The audio element handle
 * @param[in]  buffer      The buffer pointer
 * @param[in]  write_size  The write size
 *
 * @return
 *        - > 0 number of bytes written
 *        - <=0 audio_element_err_t
 */
int audio_element_output(audio_element_handle_t el, char *buffer, int write_size);

/**
 * @brief      Get External queue of Emitter.
 *             We can read any event that has been send out of Element from this `QueueHandle_t`.
 *
 * @param[in]  el    The audio element handle
 *
 * @return     QueueHandle_t
 */
QueueHandle_t audio_element_get_event_queue(audio_element_handle_t el);

/**
 * @brief      Set input port and output port have finished.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_port_done(audio_element_handle_t el);

/**
 * @brief      Enforce 'AEL_STATE_INIT' state.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_reset_state(audio_element_handle_t el);

/**
 * @brief      Get Element output ringbuffer size.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - =0: Parameter NULL
 *     - >0: Size of ringbuffer
 */
int audio_element_get_output_ringbuf_size(audio_element_handle_t el);

/**
 * @brief      Set Element output ringbuffer size.
 *
 * @param[in]  el       The audio element handle
 * @param[in]  rb_size  Size of the ringbuffer
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_output_ringbuf_size(audio_element_handle_t el, int rb_size);

/**
 * @brief      Get Element output port framebuffer size.
 *
 * @param[in]  el           The audio element handle
 * @param[in]  node_size    Size of the framebuffer node
 * @param[in]  node_num     Numbers of the framebuffer node
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_get_output_framebuf_size(audio_element_handle_t el, int *node_size, int *node_num);

/**
 * @brief      Set Element output port framebuffer size.
 *
 * @param[in]  el           The audio element handle
 * @param[in]  node_size    Size of the framebuffer node
 * @param[in]  node_num     Numbers of the framebuffer node
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_output_framebuf_size(audio_element_handle_t el, int node_size, int node_num);

/**
 * @brief      Call this function to read data from multi input ringbuffer by given index.
 *
 * @param[in]      el            The audio element handle
 * @param[in,out]  buffer        The buffer pointer
 * @param[in]      wanted_size   The wanted size
 * @param[in]      index         The index of multi input ringbuffer, start from `0`, should be less than `NUMBER_OF_MULTI_RINGBUF`
 * @param[in]      ticks_to_wait Timeout of ringbuffer
 *
 * @return
 *     - >= 0: Size of input
 *     -  < 0: Error
 */
bk_err_t audio_element_multi_input(audio_element_handle_t el, char *buffer, int wanted_size, int index, TickType_t ticks_to_wait);

/**
 * @brief      Call this function write data by multi output ringbuffer.
 *
 * @param[in]  el            The audio element handle
 * @param[in]  buffer        The buffer pointer
 * @param[in]  wanted_size   The wanted size
 * @param[in]  ticks_to_wait Timeout of ringbuffer
 *
 * @return
 *     - >= 0: Size of output
 *     -  < 0: Error
 */
bk_err_t audio_element_multi_output(audio_element_handle_t el, char *buffer, int wanted_size, TickType_t ticks_to_wait);

/**
 * @brief      Set multi input port Element.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  port  The audio port handle
 * @param[in]  index Index of multi port, starts from `0`, should be less than `NUMBER_OF_MULTI_PORT`
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_multi_input_port(audio_element_handle_t el, audio_port_handle_t port, int index);

/**
 * @brief      Set multi output ringbuffer Element.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  port  The audio port handle
 * @param[in]  index Index of multi ringbuffer, starts from `0`, should be less than `NUMBER_OF_MULTI_PORT`
 *
 * @return
 *     - BK_OK
 *     - BK_ERR_ADK_INVALID_ARG
 */
bk_err_t audio_element_set_multi_output_port(audio_element_handle_t el, audio_port_handle_t port, int index);

/**
 * @brief      Get handle of multi input audio port Element by index.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  index Index of multi audio port, starts from `0`, should be less than `NUMBER_OF_MULTI_PORT`
 *
 * @return
 *     - NULL   Error
 *     - Others audio_port_handle_t
 */
audio_port_handle_t audio_element_get_multi_input_port(audio_element_handle_t el, int index);

/**
 * @brief      Get max port number of multi input audio port Element.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - >= 0: Max port number of multi input audio port Element
 *     -  < 0: Error
 */
bk_err_t audio_element_get_multi_input_max_port_num(audio_element_handle_t el);

/**
 * @brief      Get max port number of multi output audio port Element.
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - >= 0: Max port number of multi output audio port Element
 *     -  < 0: Error
 */
bk_err_t audio_element_get_multi_output_max_port_num(audio_element_handle_t el);

/**
 * @brief      Get handle of multi output audio port Element by index.
 *
 * @param[in]  el    The audio element handle
 * @param[in]  index Index of multi audio port, starts from `0`, should be less than `NUMBER_OF_MULTI_PORT`
 *
 * @return
 *     - NULL   Error
 *     - Others audio_port_handle_t
 */
audio_port_handle_t audio_element_get_multi_output_port(audio_element_handle_t el, int index);

/**
 * @brief      Provides a way to call element's `open`
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_process_init(audio_element_handle_t el);

/**
 * @brief      Provides a way to call element's `close`
 *
 * @param[in]  el    The audio element handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_process_deinit(audio_element_handle_t el);

/**
 * @brief      Call element's `seek`
 *
 * @param[in]  el           The audio element handle
 * @param[in]  in_data      A pointer to in data
 * @param[in]  in_size      The size of the `in_data`
 * @param[out] out_data     A pointer to the out data
 * @param[out] out_size     The size of the `out_data`
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 *     - BK_ERR_ADK_NOT_SUPPORT
 */
bk_err_t audio_element_seek(audio_element_handle_t el, void *in_data, int in_size, void *out_data, int *out_size);

/**
 * @brief      Get Element stopping flag
 *
 * @param[in]  el    The audio element handle
 *
 * @return     element's stopping flag
 */
bool audio_element_is_stopping(audio_element_handle_t el);

/**
 * @brief      Update the byte position of element information
 *
 * @param[in]  el    The audio element handle
 * @param[in]  pos   The byte_pos accumulated by this value
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_update_byte_pos(audio_element_handle_t el, int pos);

/**
 * @brief      Set the byte position of element information
 *
 * @param[in]  el    The audio element handle
 * @param[in]  pos   This value is assigned to byte_pos
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_byte_pos(audio_element_handle_t el, int pos);

/**
 * @brief      Update the total bytes of element information
 *
 * @param[in]  el               The audio element handle
 * @param[in]  total_bytes      The total_bytes accumulated by this value
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_update_total_bytes(audio_element_handle_t el, int total_bytes);

/**
 * @brief      Set the total bytes of element information
 *
 * @param[in]  el               The audio element handle
 * @param[in]  total_bytes      This value is assigned to total_bytes
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_total_bytes(audio_element_handle_t el, int total_bytes);

/**
 * @brief      Set the bps of element information
 *
 * @param[in]  el           The audio element handle
 * @param[in]  bit_rate     This value is assigned to bps
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_bps(audio_element_handle_t el, int bit_rate);

/**
 * @brief      Set the codec format of element information
 *
 * @param[in]  el       The audio element handle
 * @param[in]  format   This value is assigned to codec_fmt
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_codec_fmt(audio_element_handle_t el, int format);

/**
 * @brief      Set the sample_rate, channels, bits of element information
 *
 * @param[in]  el             The audio element handle
 * @param[in]  sample_rates   Sample_rates of music information
 * @param[in]  channels       Channels of music information
 * @param[in]  bits           Bits of music information
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_music_info(audio_element_handle_t el, int sample_rates, int channels, int bits);

/**
 * @brief      Set the duration of element information
 *
 * @param[in]  el           The audio element handle
 * @param[in]  duration     This value is assigned to duration
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_duration(audio_element_handle_t el, int duration);

/**
 * @brief      Get the type of element output port
 *
 * @param[in]  el           The audio element handle
 *
 * @return
 *     - 0      Error
 *     - Others port_type_t
 */
port_type_t audio_element_get_output_port_type(audio_element_handle_t el);

/**
 * @brief      Get the type of element input port
 *
 * @param[in]  el           The audio element handle
 *
 * @return
 *     - 0      Error
 *     - Others port_type_t
 */
port_type_t audio_element_get_input_port_type(audio_element_handle_t el);

/**
 * @brief      Set the type of element output port
 *
 * @param[in]  el           The audio element handle
 * @param[in]  type         The audio port type
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_output_port_type(audio_element_handle_t el, port_type_t type);

/**
 * @brief      Set the type of element input port
 *
 * @param[in]  el           The audio element handle
 * @param[in]  type         The audio port type
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_element_set_input_port_type(audio_element_handle_t el, port_type_t type);

#ifdef __cplusplus
}
#endif

#endif
