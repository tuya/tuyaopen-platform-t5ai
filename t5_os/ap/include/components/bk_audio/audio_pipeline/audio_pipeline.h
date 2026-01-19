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

#ifndef _AUDIO_PIPELINE_H_
#define _AUDIO_PIPELINE_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_pipeline *audio_pipeline_handle_t;

/**
 * @brief Audio Pipeline configurations
 */
typedef struct audio_pipeline_cfg
{
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
 * @brief      Reset pipeline element port
 *
 * @param[in]  pipeline   The Audio Pipeline Handle
 *
 * @return
 *     - BK_OK on success
 *     - BK_FAIL when any errors
 */
bk_err_t audio_pipeline_reset_port(audio_pipeline_handle_t pipeline);

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

