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
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <os/os.h>


#define TAG   "AUD_EVT_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


static int queue_size = 5;

static bk_err_t event_on_cmd(audio_event_iface_msg_t *msg, void *context)
{
    BK_LOGD(TAG, "receive internal evt msg cmd = %d, source addr = %x, type = %d \n", msg->cmd, (int)(uintptr_t)msg->source, msg->source_type);
    if (msg->cmd == (queue_size - 1))
    {
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t adk_event_test_case_0(void)
{
#if 0
    bk_set_printf_sync(true);
    extern void bk_enable_white_list(int enabled);
    bk_enable_white_list(1);
    bk_disable_mod_printf("AUD_PIPE", 0);
    bk_disable_mod_printf("AUD_ELE", 0);
    bk_disable_mod_printf("AUD_EVT", 0);
    bk_disable_mod_printf("AUD_MEM", 0);
    bk_disable_mod_printf("AUD_EVT_TEST", 0);
#endif

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: init event1 ----------\n");

    audio_event_iface_handle_t evt1;
    audio_event_iface_cfg_t cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    cfg.on_cmd = event_on_cmd;
    cfg.queue_set_size = 10;
    cfg.context = &evt1;
    evt1 = audio_event_iface_init(&cfg);
    TEST_CHECK_NULL(evt1);

    BK_LOGD(TAG, "--------- step2: send internal msg to event1 ----------\n");
    audio_event_iface_msg_t msg;
    int i;
    for (i = 0; i < 5; i++)
    {
        msg.cmd = i;
        if (BK_OK != audio_event_iface_cmd(evt1, &msg))
        {
            BK_LOGE(TAG, "dispatch 5 msg to evt1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }
    //test internal_queue size
    msg.cmd = 5;
    if (BK_FAIL != audio_event_iface_cmd(evt1, &msg))
    {
        BK_LOGE(TAG, "check internal_queue size fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step3: listening 5 event from event1 ----------\n");
    while (audio_event_iface_waiting_cmd_msg(evt1) == BK_OK);

    BK_LOGD(TAG, "--------- step4: init event2 and event3 ----------\n");
    audio_event_iface_handle_t evt2;
    cfg.context = &evt2;
    //    cfg.type = AUDIO_ELEMENT_TYPE_PLAYER;
    evt2 = audio_event_iface_init(&cfg);
    TEST_CHECK_NULL(evt2);

    audio_event_iface_handle_t evt3;
    cfg.context = &evt3;
    //    cfg.type = AUDIO_ELEMENT_TYPE_PLAYER;
    evt3 = audio_event_iface_init(&cfg);
    TEST_CHECK_NULL(evt3);

    BK_LOGD(TAG, "--------- step5: listen event2 and event3 from event1 ----------\n");
    if (BK_OK != audio_event_iface_set_listener(evt2, evt1))
    {
        BK_LOGE(TAG, "add event2 to event1 fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_set_listener(evt3, evt1))
    {
        BK_LOGE(TAG, "add event3 to event1 fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: dispatch 2 msg to event1 ----------\n");
    for (i = 0; i < 2; i++)
    {
        msg.source = evt1;
        msg.cmd = i;
        if (BK_OK != audio_event_iface_cmd(evt1, &msg))
        {
            BK_LOGE(TAG, "dispatch 2 msg to event1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "--------- step7: sendout 3 msg from event2 to event1 ----------\n");
    for (i = 2; i < 5; i++)
    {
        msg.source = evt2;
        msg.cmd = i;
        if (BK_OK != audio_event_iface_sendout(evt2, &msg))
        {
            BK_LOGE(TAG, "sendout 3 msg from event2 to event1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "--------- step8: sendout 5 msg from event3 to event1 ----------\n");
    for (i = 5; i < 10; i++)
    {
        msg.source = evt3;
        msg.cmd = i;
        if (BK_OK != audio_event_iface_sendout(evt3, &msg))
        {
            BK_LOGE(TAG, "sendout 5 msg from event3 to event1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "--------- step9: listening 10 event have dispatched from event1, event2 and event3 ----------\n");
    queue_size = 2;
    while (audio_event_iface_listen(evt1, &msg, 0) == BK_OK)
    {
        BK_LOGD(TAG, "receive listener evt msg cmd = %d, source addr = %x, type = %d \n", msg.cmd, (int)(uintptr_t)msg.source, msg.source_type);
        if (msg.cmd != queue_size++)
        {
            BK_LOGE(TAG, "cmd check fail, cmd:%d != queue_size:%d, %d \n", __LINE__, msg.cmd, queue_size - 1);
            return BK_FAIL;
        }
    }
    //check msg count
    if (queue_size != 10)
    {
        BK_LOGE(TAG, "lost msg count: %d \n", 10 - queue_size);
        return BK_FAIL;
    }
    //receive internal message
    queue_size = 2;
    while (audio_event_iface_waiting_cmd_msg(evt1) == BK_OK);

    BK_LOGD(TAG, "--------- step10: remove event2 and event3 from listener event1 ----------\n");
    if (BK_OK != audio_event_iface_remove_listener(evt1, evt2))
    {
        BK_LOGE(TAG, "remove event from listener event1 fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_remove_listener(evt1, evt3))
    {
        BK_LOGE(TAG, "remove event from listener event1 fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step12: dispatch 5 msg to event1 ----------\n");
    for (i = 0; i < 5; i++)
    {
        msg.source = evt1;
        msg.cmd = i;
        if (BK_OK != audio_event_iface_cmd(evt1, &msg))
        {
            BK_LOGE(TAG, "dispatch 5 msg to event1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "--------- step11: sendout 5 msg from event2 to event1 ----------\n");
    for (i = 5; i < 10; i++)
    {
        msg.source = evt2;
        msg.cmd = i;
        if (BK_OK != audio_event_iface_sendout(evt2, &msg))
        {
            BK_LOGE(TAG, "sendout 5 msg from event2 to event1 fail, %d \n", __LINE__);
            return BK_FAIL;
        }
    }

    BK_LOGD(TAG, "--------- step13: listening 10 event have dispatched from event1 and event2 ----------\n");
    while (audio_event_iface_listen(evt1, &msg, 0) == BK_OK)
    {
        BK_LOGD(TAG, "receive listener evt msg cmd = %d, source addr = %x, type = %d \n", msg.cmd, (int)(uintptr_t)msg.source, msg.source_type);
    }
    //receive internal message
    queue_size = 5;
    while (audio_event_iface_waiting_cmd_msg(evt1) == BK_OK);

    BK_LOGD(TAG, "--------- step14: destroy all events ----------\n");
    audio_event_iface_destroy(evt1);
    audio_event_iface_destroy(evt2);
    audio_event_iface_destroy(evt3);

    BK_LOGD(TAG, "--------- audio event test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
