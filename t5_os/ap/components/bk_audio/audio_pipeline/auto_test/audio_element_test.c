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

#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <os/os.h>
#include <components/bk_audio/audio_pipeline/fb_port.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>


#define TAG   "AUD_ELE_TEST"


#define INPUT_RINGBUF_SIZE      (1024 * 4)
#define OUTPUT_RINGBUF_SIZE     (1024 * 4)

//#define INPUT_FRAMEBUF_SIZE     (1024)
//#define INPUT_FRAMEBUF_NUM      (4)
#define OUTPUT_FRAMEBUF_SIZE    (1024)
#define OUTPUT_FRAMEBUF_NUM     (4)


//
static char *input_port_temp_data = NULL;
static char *output_port_temp_data = NULL;
static audio_port_handle_t input_port = NULL;
static audio_port_handle_t output_port = NULL;

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "_el_open \n");
    return BK_OK;
}

static int _el_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGD(TAG, "_el_read, len: %d \n", len);
    /*
        //the data in output_rb need to be read
        if (output_port_temp_data) {
            rb_read(output_rb, output_port_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
            os_memcpy(buffer, output_port_temp_data, INPUT_RINGBUF_SIZE);
        }
    */
    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    rtos_delay_milliseconds(100);
    BK_LOGD(TAG, "_el_process, in_len: %d \n", in_len);

    int r_size = audio_element_input(self, in_buffer, in_len);

    //write data to input_rb after read to fill input_rb
    if (input_port_temp_data)
    {
        audio_port_write(input_port, in_buffer, in_len, portMAX_DELAY);
    }

    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    //read data from output_rb after write to clear output_rb
    if (output_port_temp_data)
    {
        audio_port_read(output_port, output_port_temp_data, in_len, portMAX_DELAY);
    }

    return w_size;
}

static int _el_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    BK_LOGD(TAG, "_el_write, len: %d \n", len);
    /*
        if (input_port_temp_data) {
            rb_write(input_rb, input_port_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
        }
    */
    return len;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "_el_close \n");
    return BK_OK;
}

/* The test element is both a consumer and producer when test element is both the last
   element and the first element of the pipeline. Usually this element has on sink and
   no src. The data flow model of this element is as follow:
   +---------+
   | element |
   |         |
   |         |
   +---------+

   This test element read audio data through callback api and process the data.
*/
bk_err_t adk_element_test_case_0(void)
{
#if 0
    bk_set_printf_sync(true);
    extern void bk_enable_white_list(int enabled);
    bk_enable_white_list(1);
    bk_disable_mod_printf("AUD_ELE", 0);
    bk_disable_mod_printf("AUD_MEM", 0);
    bk_disable_mod_printf("AUD_ELE_TEST", 0);
#endif
    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_handle_t el;
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step4: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step7: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- element test [0] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is a consumer when test element is the last element of the pipeline.
   Usually this element has only sink and no src. The data flow model of this element is
   as follow:
   +---------+               +---------+
   |   ...   |               | element |
  ...       src - ringbuf - sink       |
   |         |               |         |
   +---------+               +---------+

   The previous element of pipeline write audio data to ringbuffer. This test element
   read audio data form ringbuffer and process the data.
*/
bk_err_t adk_element_test_case_1(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_RB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input ringbuf init ----------\n");
    ringbuf_port_cfg_t rb_port_cfg = {INPUT_RINGBUF_SIZE};
    input_port = ringbuf_port_init(&rb_port_cfg);
    audio_element_set_input_port(el, input_port);

    input_port_temp_data = audio_malloc(INPUT_RINGBUF_SIZE);
    os_memset(input_port_temp_data, 0, INPUT_RINGBUF_SIZE);

    //write some data to ringbuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
    if (write_size != INPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, INPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_size = audio_port_get_filled_size(input_port);
    if (filled_size != INPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "ringbuf check fail, filled_size: %d != %d\n", filled_size, INPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [1] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is a producer when test element is the first element of the pipeline.
   Usually this element has only src and no sink. The data flow model of this element is
   as follow:
   +---------+               +---------+
   | element |               |   ...   |
   |        src - ringbuf - sink      ...
   |         |               |         |
   +---------+               +---------+

   This test element produce audio data and write data to ringbuffer. The next element of
   pipeline read data from ringbuffer and process the data.
*/
bk_err_t adk_element_test_case_2(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_type = PORT_TYPE_RB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: output ringbuf init ----------\n");
    ringbuf_port_cfg_t out_rb_port_cfg = {INPUT_RINGBUF_SIZE};
    output_port = ringbuf_port_init(&out_rb_port_cfg);
    audio_element_set_output_port(el, output_port);

    output_port_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_RINGBUF_SIZE);

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [2] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is neither a producer nor a consumer when test element is neither
   first element nor last element of the pipeline. Usually this element has both src
   and sink. The data flow model of this element is
   as follow:
   +---------+               +---------+               +---------+
   |   ...   |               | element |               |   ...   |
  ...       src - ringbuf - sink      src - ringbuf - sink      ...
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   The previous element of pipeline write audio data to ringbuffer. This test element
   read the data from ringbuffer, process the data and write output data to ringbuffer.
   The next element of pipeline read data from ringbuffer and process the data.
*/
bk_err_t adk_element_test_case_3(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_RB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_type = PORT_TYPE_RB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input&output ringbuf init ----------\n");
    ringbuf_port_cfg_t out_rb_port_cfg = {INPUT_RINGBUF_SIZE};
    output_port = ringbuf_port_init(&out_rb_port_cfg);
    ringbuf_port_cfg_t in_rb_port_cfg = {INPUT_RINGBUF_SIZE};
    input_port = ringbuf_port_init(&in_rb_port_cfg);
    audio_element_set_input_port(el, input_port);
    audio_element_set_output_port(el, output_port);

    input_port_temp_data = os_malloc(INPUT_RINGBUF_SIZE);
    os_memset(input_port_temp_data, 0, INPUT_RINGBUF_SIZE);
    output_port_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_RINGBUF_SIZE);

    //write some data to ringbuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, INPUT_RINGBUF_SIZE, portMAX_DELAY);
    if (write_size != INPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, INPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_size = audio_port_get_filled_size(input_port);
    if (filled_size != INPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "ringbuf check fail, filled_size: %d != %d\n", filled_size, INPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [3] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is a consumer when test element is the last element of the pipeline.
   Usually this element has only sink and no src. The data flow model of this element is
   as follow:
   +---------+               +---------+
   |   ...   |               | element |
  ...       src - framebuf - sink       |
   |         |               |         |
   +---------+               +---------+

   The previous element of pipeline write audio data to framebuffer. This test element
   read audio data form framebuffer and process the data.
*/
bk_err_t adk_element_test_case_4(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_FB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_type = PORT_TYPE_CB;
    cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input ringbuf init ----------\n");
    framebuf_port_cfg_t in_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    input_port = framebuf_port_init(&in_fb_port_cfg);
    audio_element_set_input_port(el, input_port);

    input_port_temp_data = audio_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(input_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);

    //write some data to framebuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, OUTPUT_FRAMEBUF_SIZE, portMAX_DELAY);
    if (write_size != OUTPUT_FRAMEBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, OUTPUT_FRAMEBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_node_num = audio_port_get_filled_size(input_port);
    uint32_t total_node_num = audio_port_get_size(input_port);
    if (filled_node_num != 1 || total_node_num != OUTPUT_FRAMEBUF_NUM)
    {
        BK_LOGE(TAG, "framebuf check fail, filled_node_num: %d != 1, or total_node_num:%d != %d\n", filled_node_num, total_node_num, OUTPUT_FRAMEBUF_NUM);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [1] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is a producer when test element is the first element of the pipeline.
   Usually this element has only src and no sink. The data flow model of this element is
   as follow:
   +---------+               +---------+
   | element |               |   ...   |
   |        src - framebuf - sink      ...
   |         |               |         |
   +---------+               +---------+

   This test element produce audio data and write data to framebuffer. The next element of
   pipeline read data from framebuffer and process the data.
*/
bk_err_t adk_element_test_case_5(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_CB;
    cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    cfg.out_type = PORT_TYPE_FB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: output ringbuf init ----------\n");
    framebuf_port_cfg_t out_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    output_port = framebuf_port_init(&out_fb_port_cfg);
    audio_element_set_output_port(el, output_port);

    output_port_temp_data = os_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [2] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is neither a producer nor a consumer when test element is neither
   first element nor last element of the pipeline. Usually this element has both src
   and sink. The data flow model of this element is
   as follow:
   +---------+               +---------+               +---------+
   |   ...   |               | element |               |   ...   |
  ...       src - framebuf - sink      src - framebuf - sink      ...
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   The previous element of pipeline write audio data to framebuffer. This test element
   read the data from framebuffer, process the data and write output data to framebuffer.
   The next element of pipeline read data from framebuffer and process the data.
*/
bk_err_t adk_element_test_case_6(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_FB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    cfg.out_type = PORT_TYPE_FB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input&output ringbuf init ----------\n");
    framebuf_port_cfg_t in_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    input_port = framebuf_port_init(&in_fb_port_cfg);
    audio_element_set_input_port(el, input_port);
    framebuf_port_cfg_t out_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    output_port = framebuf_port_init(&out_fb_port_cfg);
    audio_element_set_output_port(el, output_port);

    input_port_temp_data = os_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(input_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);
    output_port_temp_data = os_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);

    //write some data to framebuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, OUTPUT_FRAMEBUF_SIZE, portMAX_DELAY);
    if (write_size != OUTPUT_FRAMEBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, OUTPUT_FRAMEBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_node_num = audio_port_get_filled_size(input_port);
    uint32_t total_node_num = audio_port_get_size(input_port);
    if (filled_node_num != 1 || total_node_num != OUTPUT_FRAMEBUF_NUM)
    {
        BK_LOGE(TAG, "framebuf check fail, filled_node_num: %d != 1, or total_node_num:%d != %d\n", filled_node_num, total_node_num, OUTPUT_FRAMEBUF_NUM);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [3] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is neither a producer nor a consumer when test element is neither
   first element nor last element of the pipeline. Usually this element has both src
   and sink. The data flow model of this element is
   as follow:
   +---------+               +---------+               +---------+
   |   ...   |               | element |               |   ...   |
  ...       src - ringbuf - sink      src - framebuf - sink      ...
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   The previous element of pipeline write audio data to ringbuffer. This test element
   read the data from ringbuffer, process the data and write output data to framebuffer.
   The next element of pipeline read data from framebuffer and process the data.
*/
bk_err_t adk_element_test_case_7(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_RB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
    cfg.out_block_size = OUTPUT_FRAMEBUF_SIZE;
    cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    cfg.out_type = PORT_TYPE_FB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input&output ringbuf init ----------\n");
    ringbuf_port_cfg_t in_rb_port_cfg = {OUTPUT_RINGBUF_SIZE};
    input_port = ringbuf_port_init(&in_rb_port_cfg);
    audio_element_set_input_port(el, input_port);
    framebuf_port_cfg_t out_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    output_port = framebuf_port_init(&out_fb_port_cfg);
    audio_element_set_output_port(el, output_port);

    input_port_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
    os_memset(input_port_temp_data, 0, OUTPUT_RINGBUF_SIZE);
    output_port_temp_data = os_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);

    //write some data to ringbuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, OUTPUT_RINGBUF_SIZE, portMAX_DELAY);
    if (write_size != OUTPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, OUTPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_size = audio_port_get_filled_size(input_port);
    if (filled_size != OUTPUT_RINGBUF_SIZE)
    {
        BK_LOGE(TAG, "ringbuf check fail, filled_size: %d != %d\n", filled_size, OUTPUT_RINGBUF_SIZE);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [3] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

/* The test element is neither a producer nor a consumer when test element is neither
   first element nor last element of the pipeline. Usually this element has both src
   and sink. The data flow model of this element is
   as follow:
   +---------+               +---------+               +---------+
   |   ...   |               | element |               |   ...   |
  ...       src - framebuf - sink      src - ringbuf - sink      ...
   |         |               |         |               |         |
   +---------+               +---------+               +---------+

   The previous element of pipeline write audio data to framebuffer. This test element
   read the data from framebuffer, process the data and write output data to ringbuffer.
   The next element of pipeline read data from ringbuffer and process the data.
*/
bk_err_t adk_element_test_case_8(void)
{
    audio_element_handle_t el;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: element init ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.buffer_len = OUTPUT_FRAMEBUF_SIZE;
    cfg.open = _el_open;
    cfg.in_type = PORT_TYPE_FB;
    // cfg.read = _el_read;
    cfg.process = _el_process;
//    cfg.out_rb_size = OUTPUT_FRAMEBUF_SIZE;
//    cfg.out_block_num = OUTPUT_FRAMEBUF_NUM;
    cfg.out_type = PORT_TYPE_RB;
    // cfg.write = _el_write;
    cfg.close = _el_close;
    el = audio_element_init(&cfg);
    if (!el)
    {
        BK_LOGE(TAG, "audio_element_init fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: input&output ringbuf init ----------\n");
    framebuf_port_cfg_t in_fb_port_cfg = {OUTPUT_FRAMEBUF_SIZE, OUTPUT_FRAMEBUF_NUM};
    input_port = framebuf_port_init(&in_fb_port_cfg);
    audio_element_set_input_port(el, input_port);
    ringbuf_port_cfg_t out_rb_port_cfg = {OUTPUT_RINGBUF_SIZE};
    output_port = ringbuf_port_init(&out_rb_port_cfg);
    audio_element_set_output_port(el, output_port);

    input_port_temp_data = os_malloc(OUTPUT_FRAMEBUF_SIZE);
    os_memset(input_port_temp_data, 0, OUTPUT_FRAMEBUF_SIZE);
    output_port_temp_data = os_malloc(OUTPUT_RINGBUF_SIZE);
    os_memset(output_port_temp_data, 0, OUTPUT_RINGBUF_SIZE);

    //write some data to framebuf
    uint32_t write_size = audio_port_write(input_port, input_port_temp_data, OUTPUT_FRAMEBUF_SIZE, portMAX_DELAY);
    if (write_size != OUTPUT_FRAMEBUF_SIZE)
    {
        BK_LOGE(TAG, "audio_port_write fail, write_size: %d != %d\n", write_size, OUTPUT_FRAMEBUF_SIZE);
        return BK_FAIL;
    }

    //check data in ringbuf
    uint32_t filled_node_num = audio_port_get_filled_size(input_port);
    uint32_t total_node_num = audio_port_get_size(input_port);
    if (filled_node_num != 1 || total_node_num != OUTPUT_FRAMEBUF_NUM)
    {
        BK_LOGE(TAG, "framebuf check fail, filled_node_num: %d != 1, or total_node_num:%d != %d\n", filled_node_num, total_node_num, OUTPUT_FRAMEBUF_NUM);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: element run ----------\n");
    if (BK_OK != audio_element_run(el))
    {
        BK_LOGE(TAG, "audio_element_run fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step5: element pause ----------\n");
    if (BK_OK != audio_element_pause(el))
    {
        BK_LOGE(TAG, "audio_element_pause fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step6: element resume ----------\n");
    if (BK_OK != audio_element_resume(el, 0, 2000 / portTICK_RATE_MS))
    {
        BK_LOGE(TAG, "audio_element_resume fail \n");
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    BK_LOGD(TAG, "--------- step7: element stop ----------\n");
    if (BK_OK != audio_element_stop(el))
    {
        BK_LOGE(TAG, "audio_element_stop fail \n");
        return BK_FAIL;
    }
    if (BK_OK != audio_element_wait_for_stop(el))
    {
        BK_LOGE(TAG, "audio_element_wait_for_stop fail \n");
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step8: element deinit ----------\n");
    if (BK_OK != audio_element_deinit(el))
    {
        BK_LOGE(TAG, "audio_element_deinit fail \n");
        return BK_FAIL;
    }
    audio_port_deinit(output_port);
    os_free(output_port_temp_data);
    output_port_temp_data = NULL;
    audio_port_deinit(input_port);
    os_free(input_port_temp_data);
    input_port_temp_data = NULL;

    BK_LOGD(TAG, "--------- element test [3] complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}


