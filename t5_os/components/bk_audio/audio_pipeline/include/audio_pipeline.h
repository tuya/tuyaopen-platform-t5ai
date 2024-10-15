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

#ifndef _AUDIO_PIPELINE_H_
#define _AUDIO_PIPELINE_H_

#include "audio_element.h"
#if CONFIG_SYS_CPU1
#include "audio_coprocess.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_pipeline *audio_pipeline_handle_t;

/**
 * @brief Audio Pipeline configurations
 */
typedef struct audio_pipeline_cfg {
    int rb_size;        /*!< Audio Pipeline ringbuffer size */
} audio_pipeline_cfg_t;

#define DEFAULT_PIPELINE_RINGBUF_SIZE    (8*1024)

#define DEFAULT_AUDIO_PIPELINE_CONFIG() {\
    .rb_size            = DEFAULT_PIPELINE_RINGBUF_SIZE,\
}

/**
 * @brief      Initialize audio_pipeline_handle_t object
 *             audio_pipeline is responsible for controlling the audio data stream and connecting the audio elements with the ringbuffer
 *             It will connect and start the audio element in order, responsible for retrieving the data from the previous element
 *             and passing it to the element after it. Also get events from each element, process events or pass it to a higher layer
 *
 * @param      config  The configuration - audio_pipeline_cfg_t
 *
 * @return
 *     - audio_pipeline_handle_t on success
 *     - NULL when any errors
 */
audio_pipeline_handle_t audio_pipeline_init(audio_pipeline_cfg_t *config);

/**
 * @brief      This function removes all of the element's links in audio_pipeline,
 *             cancels the registration of all events, invokes the destroy functions of the registered elements,
 *             and frees the memory allocated by the init function.
 *             Briefly, frees all memory
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return     BK_OK
 */
bk_err_t audio_pipeline_deinit(audio_pipeline_handle_t pipeline);

/**
 * @brief      Registering an element for audio_pipeline, each element can be registered multiple times,
 *             but `name` (as String) must be unique in audio_pipeline,
 *             which is used to identify the element for link creation mentioned in the `audio_pipeline_link`
 *
 * @note       Because of stop pipeline or pause pipeline depend much on register order.
 *             Please register element strictly in the following order: input element first, process middle, output element last.
 *
 * @param[in]  pipeline The Audio Pipeline Handle
 * @param[in]  el       The Audio Element Handle
 * @param[in]  name     The name identifier of the audio_element in this audio_pipeline
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_register(audio_pipeline_handle_t pipeline, audio_element_handle_t el, const char *name);

/**
 * @brief      Unregister the audio_element in audio_pipeline, remove it from the list
 *
 * @param[in]  pipeline The Audio Pipeline Handle
 * @param[in]  el       The Audio Element Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_unregister(audio_pipeline_handle_t pipeline, audio_element_handle_t el);

/**
 * @brief    Start Audio Pipeline.
 *
 *           With this function audio_pipeline will create tasks for all elements,
 *           that have been linked using the linking functions.
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_run(audio_pipeline_handle_t pipeline);

/**
 * @brief    Stop Audio Pipeline.
 *
 *           With this function audio_pipeline will destroy tasks of all elements,
 *           that have been linked using the linking functions.
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_terminate(audio_pipeline_handle_t pipeline);

/**
 * @brief    Stop Audio Pipeline with specific ticks for timeout
 *
 *           With this function audio_pipeline will destroy tasks of all elements,
 *           that have been linked using the linking functions.
 *
 * @param[in]  pipeline         The Audio Pipeline Handle
 * @param[in]  ticks_to_wait    The maximum amount of time to block wait for element destroy
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_terminate_with_ticks(audio_pipeline_handle_t pipeline, TickType_t ticks_to_wait);

/**
 * @brief      This function will set all the elements to the `RUNNING` state and process the audio data as an inherent feature of audio_pipeline.
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_resume(audio_pipeline_handle_t pipeline);

/**
 * @brief      This function will set all the elements to the `PAUSED` state. Everything remains the same except the data processing is stopped
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_pause(audio_pipeline_handle_t pipeline);

/**
 * @brief     Stop all of the linked elements. Used with `audio_pipeline_wait_for_stop` to keep in sync.
 *            The link state of the elements in the pipeline is kept, events are still registered.
 *            The stopped audio_pipeline restart by `audio_pipeline_resume`.
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_stop(audio_pipeline_handle_t pipeline);

/**
 * @brief      The `audio_pipeline_stop` function sends requests to the elements and exits.
 *             But they need time to get rid of time-blocking tasks.
 *             This function will wait `portMAX_DELAY` until all the Elements in the pipeline actually stop
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_wait_for_stop(audio_pipeline_handle_t pipeline);

/**
 * @brief      The `audio_pipeline_stop` function sends requests to the elements and exits.
 *             But they need time to get rid of time-blocking tasks.
 *             This function will wait `ticks_to_wait` until all the Elements in the pipeline actually stop
 *
 * @param[in]  pipeline         The Audio Pipeline Handle
 * @param[in]  ticks_to_wait    The maximum amount of time to block wait for stop
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_wait_for_stop_with_ticks(audio_pipeline_handle_t pipeline, TickType_t ticks_to_wait);

/**
 * @brief      The audio_element added to audio_pipeline will be unconnected before it is called by this function.
 *             Based on element's `name` already registered by `audio_pipeline_register`, the path of the data will be linked in the order of the link_tag.
 *             Element at index 0 is first, and index `link_num -1` is final.
 *             As well as audio_pipeline will subscribe all element's events
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 * @param      link_tag   Array of element `name` was registered by `audio_pipeline_register`
 * @param[in]  link_num   Total number of elements of the `link_tag` array
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_link(audio_pipeline_handle_t pipeline, const char *link_tag[], int link_num);

/**
 * @brief      Removes the connection of the elements, as well as unsubscribe events
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_unlink(audio_pipeline_handle_t pipeline);

/**
 * @brief      Find un-kept element from registered pipeline by tag
 *
 * @param[in]  pipeline     The Audio Pipeline Handle
 * @param[in]  tag          A char pointer
 *
 * @return
 *     - NULL when any errors
 *     - Others on success
 */
audio_element_handle_t audio_pipeline_get_el_by_tag(audio_pipeline_handle_t pipeline, const char *tag);

/**
 * @brief      Based on beginning element to find un-kept element from registered pipeline by tag
 *
 * @param[in]  pipeline     The Audio Pipeline Handle
 * @param[in]  start_el     Specific beginning element
 * @param[in]  tag          A char pointer
 *
 * @return
 *     - NULL when any errors
 *     - Others on success
 */
audio_element_handle_t audio_pipeline_get_el_once(audio_pipeline_handle_t pipeline, const audio_element_handle_t start_el, const char *tag);

/**
 * @brief      Remove event listener from this audio_pipeline
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_remove_listener(audio_pipeline_handle_t pipeline);

/**
 * @brief      Set event listner for this audio_pipeline, any event from this pipeline can be listen to by `evt`
 *
 * @param[in]  pipeline     The Audio Pipeline Handle
 * @param[in]  evt          The Event Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_set_listener(audio_pipeline_handle_t pipeline, audio_event_iface_handle_t evt);

/**
 * @brief      Get the event iface using by this pipeline
 *
 * @param[in]  pipeline  The pipeline
 *
 * @return     The  Event Handle
 */
audio_event_iface_handle_t audio_pipeline_get_event_iface(audio_pipeline_handle_t pipeline);

/**
 * @brief      Insert the specific audio_element to audio_pipeline, previous element connect to the next element by ring buffer.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  first        Previous element is first input element, need to set `true`
 * @param[in]  prev         Previous element
 * @param[in]  conect_rb    Connect ring buffer
 * @param[in]  next         Next element
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_link_insert(audio_pipeline_handle_t pipeline, bool first, audio_element_handle_t prev,
                                     ringbuf_handle_t conect_rb, audio_element_handle_t next);

/**
 * @brief      Register a NULL-terminated list of elements to audio_pipeline.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  element_1    The element to add to the audio_pipeline.
 * @param[in]  ...          Additional elements to add to the audio_pipeline.
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_register_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...);

/**
 * @brief      Unregister a NULL-terminated list of elements to audio_pipeline.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  element_1    The element to add to the audio_pipeline.
 * @param[in]  ...          Additional elements to add to the audio_pipeline.
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_unregister_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...);

/**
 * @brief      Adds a NULL-terminated list of elements to audio_pipeline.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  element_1    The element to add to the audio_pipeline.
 * @param[in]  ...          Additional elements to add to the audio_pipeline.
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_link_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...);

/**
 * @brief      Subscribe a NULL-terminated list of element's events to audio_pipeline.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  element_1    The element event to subscribe to the audio_pipeline.
 * @param[in]  ...          Additional elements event to subscribe to the audio_pipeline.
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t audio_pipeline_listen_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...);

/**
 * @brief      Update the destination element state and check the all of linked elements state are same.
 *
 * @param[in]  pipeline     The audio pipeline handle
 * @param[in]  dest_el      Destination element
 * @param[in]  status       The new status
 *
 * @return
 *     - BK_OK             All linked elements state are same.
 *     - BK_FAIL           All linked elements state are not same.
 */
bk_err_t audio_pipeline_check_items_state(audio_pipeline_handle_t pipeline, audio_element_handle_t dest_el, audio_element_status_t status);

/**
 * @brief      Reset pipeline element items state to `AEL_STATUS_NONE`
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_reset_items_state(audio_pipeline_handle_t pipeline);

/**
 * @brief      Reset pipeline element ringbuffer
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_reset_ringbuffer(audio_pipeline_handle_t pipeline);

/**
 * @brief      Reset Pipeline linked elements state
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_reset_elements(audio_pipeline_handle_t pipeline);

/**
 * @brief      Reset the specific element kept state
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 * @param[in]  el         The Audio element Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_reset_kept_state(audio_pipeline_handle_t pipeline, audio_element_handle_t el);

/**
 * @brief      Break up all the linked elements of specific `pipeline`.
 *             The include and before `kept_ctx_el` working (AEL_STATE_RUNNING or AEL_STATE_PAUSED) elements
 *             and connected ringbuffer will be reserved.
 *
 * @note       There is no element reserved when `kept_ctx_el` is NULL.
 *             This function will unsubscribe all element's events.
 *
 * @param[in]  pipeline         The audio pipeline handle
 * @param[in]  kept_ctx_el      Destination keep elements
 *
 * @return
 *     - BK_OK                 All linked elements state are same.
 *     - BK_ERR_ADF_INVALID_ARG    Invalid parameters.
 */
bk_err_t audio_pipeline_breakup_elements(audio_pipeline_handle_t pipeline, audio_element_handle_t kept_ctx_el);

/**
 * @brief      Basing on element's `name` already registered by `audio_pipeline_register`,
 *             relink the pipeline following the order of `names` in the `link_tag.
 *
 * @note       If the ringbuffer is not enough to connect the new pipeline will create new ringbuffer.
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 * @param      link_tag   Array of elements `name` that was registered by `audio_pipeline_register`
 * @param[in]  link_num   Total number of elements of the `link_tag` array
 *
 * @return
 *     - BK_OK                 All linked elements state are same.
 *     - BK_FAIL               Error.
 *     - BK_ERR_ADF_INVALID_ARG    Invalid parameters.
 */
bk_err_t audio_pipeline_relink(audio_pipeline_handle_t pipeline, const char *link_tag[], int link_num);

/**
 * @brief      Adds a NULL-terminated list of elements to audio_pipeline.
 *
 * @note       If the ringbuffer is not enough to connect the new pipeline will create new ringbuffer.
 *
 * @param[in]  pipeline     The Audio Pipeline Handle
 * @param[in]  element_1    The element to add to the audio_pipeline.
 * @param[in]  ...          Additional elements to add to the audio_pipeline.
 *
 * @return
 *     - BK_OK                 All linked elements state are same.
 *     - BK_FAIL               Error.
 *     - BK_ERR_ADF_INVALID_ARG    Invalid parameters.
 */
bk_err_t audio_pipeline_relink_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...);

/**
 * @brief      Set the pipeline state.
 *
 * @param[in]  pipeline     The Audio Pipeline Handle
 * @param[in]  new_state    The new state will be set
 *
 * @return
 *     - BK_OK                 All linked elements state are same.
 *     - BK_FAIL               Error.
 */
bk_err_t audio_pipeline_change_state(audio_pipeline_handle_t pipeline, audio_element_state_t new_state);


#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_PIPELINE_H_ */

