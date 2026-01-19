// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <os/mem.h>

#include "frame_buffer.h"
#include "yuv_encode.h"
#include "mux_pipeline.h"


#define TAG "jdec_list"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif
#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t jdec_list_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t jdec_list_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&jdec_list_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void jdec_list_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&jdec_list_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

void jpeg_decode_list_clear(LIST_HEADER_T *list)
{
    uint32_t flag = jdec_list_enter_critical();

	LIST_HEADER_T *pos, *n;
	jpeg_decode_list_t *tmp = NULL;
	if (!list_empty(list))
	{
		list_for_each_safe(pos, n, list)
		{
			tmp = list_entry(pos, jpeg_decode_list_t, list);
			if (tmp != NULL)
			{
				list_del(pos);
				LOGV("%s pos %p\n", __func__, pos);
				frame_buffer_display_free(tmp->frame);
				os_free(tmp);
			}
		}
		INIT_LIST_HEAD(list);
	}
    jdec_list_exit_critical(flag);
}

uint8_t jpeg_decode_list_del_node(frame_buffer_t *frame, LIST_HEADER_T *list)
{
    uint32_t flag = jdec_list_enter_critical();
	LIST_HEADER_T *pos, *n;
	uint8_t delete_status = false;
	jpeg_decode_list_t *tmp = NULL;
	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, jpeg_decode_list_t, list);
		if (tmp != NULL)
		{
			if(tmp->frame == frame)
			{
				list_del(pos);
				os_free(tmp);
				delete_status = true;
				break;
			}
		}
	}
    jdec_list_exit_critical(flag);
	return delete_status;
}

uint8_t jpeg_decode_list_get_count(LIST_HEADER_T *list)
{
    uint32_t flag = jdec_list_enter_critical();
	LIST_HEADER_T *pos, *n;
	uint8 count = 0;
	jpeg_decode_list_t *tmp = NULL;
	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, jpeg_decode_list_t, list);
		if (tmp != NULL)
		{
			count++;
		}
	}
    jdec_list_exit_critical(flag);
	return count;
}

frame_buffer_t *jpeg_decode_list_pop(LIST_HEADER_T *list)
{
    uint32_t flag = jdec_list_enter_critical();
	LIST_HEADER_T *pos, *n;
	frame_buffer_t *frame = NULL;
	jpeg_decode_list_t *tmp = NULL;
	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, jpeg_decode_list_t, list);
		if (tmp != NULL)
		{
			if (frame == NULL)
			{
				frame = tmp->frame;
			}
			else if (frame->sequence > tmp->frame->sequence)
			{
				frame = tmp->frame;
			}
		}
	}
    jdec_list_exit_critical(flag);
	return frame;
}

bk_err_t jpeg_decode_list_push(frame_buffer_t *frame, LIST_HEADER_T *list)
{
	bk_err_t ret = BK_OK;
	jpeg_decode_list_t *jpeg_decode_list = os_malloc(sizeof(jpeg_decode_list_t));
	if (jpeg_decode_list == NULL)
	{
		return BK_ERR_NO_MEM;
	}
	jpeg_decode_list->frame = frame;
    uint32_t flag = jdec_list_enter_critical();

	list_add_tail(&jpeg_decode_list->list, list);
    jdec_list_exit_critical(flag);
	return ret;
}

