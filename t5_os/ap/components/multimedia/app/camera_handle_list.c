// Copyright 2024-2025 Beken
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
#include <driver/int.h>

#include <components/log.h>
#include "bk_list.h"
#include "camera_handle_list.h"

#include "media_app.h"
#define TAG "cam_handle"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
	struct list_head *list;
	beken_mutex_t lock;
} cam_list_t;

static cam_list_t *s_cam_handle_list = NULL;

bk_err_t bk_camera_handle_list_init(void *param)
{
	if (s_cam_handle_list == NULL)
	{
		s_cam_handle_list = (cam_list_t *)os_malloc(sizeof(cam_list_t));
		if (s_cam_handle_list == NULL)
		{
			BK_ASSERT_EX(0, "%s, %d malloc fail\n", __func__, __LINE__);
		}

		os_memset(s_cam_handle_list, 0, sizeof(cam_list_t));

		if (rtos_init_mutex(&s_cam_handle_list->lock) != BK_OK)
		{
			BK_ASSERT_EX(0, "%s, %d malloc fail\n", __func__, __LINE__);
		}
	}

	s_cam_handle_list->list = (struct list_head *)param;

	return BK_OK;
}

bk_err_t bk_camera_handle_list_deinit(void)
{
	cam_list_t *cam_list = s_cam_handle_list;
	if (cam_list)
	{
		if (!list_empty(cam_list->list))
		{
			LOGW("%s, node not free complete\n", __func__);
			return BK_FAIL;
		}

		rtos_deinit_mutex(&cam_list->lock);
		os_free(cam_list);
		s_cam_handle_list = NULL;
	}

	return BK_OK;
}

media_camera_node_t *bk_camera_handle_node_init(uint16_t id, uint16_t format)
{
	cam_list_t *cam_list = s_cam_handle_list;

	if (cam_list == NULL)
	{
		LOGE("%s %d list NULL\n", __func__, __LINE__);
		return NULL;
	}

	media_camera_node_t *node = (media_camera_node_t *)os_malloc(sizeof(media_camera_node_t));
	if (node == NULL)
	{
		LOGE("%s %d malloc fail\n", __func__, __LINE__);
		return NULL;
	}

	os_memset(node, 0, sizeof(media_camera_node_t));
	INIT_LIST_HEAD(&node->list);

	node->id = id;
	node->format = format;
	rtos_lock_mutex(&cam_list->lock);
	list_add_tail(&node->list, cam_list->list);
	rtos_unlock_mutex(&cam_list->lock);

	return node;
}

void bk_camera_handle_node_deinit(camera_handle_t handle)
{
	LIST_HEADER_T *pos, *n;
	media_camera_node_t *tmp = NULL;
	cam_list_t *cam_list = s_cam_handle_list;

	if (cam_list == NULL)
	{
		LOGE("%s %d list NULL\n", __func__, __LINE__);
		return;
	}

	rtos_lock_mutex(&cam_list->lock);

	if (!list_empty(cam_list->list))
	{
		list_for_each_safe(pos, n, cam_list->list)
		{
			tmp = list_entry(pos, media_camera_node_t, list);
			if (tmp != NULL && tmp->cam_handle == handle)
			{
				list_del(pos);
				os_free(tmp);
				break;
			}
		}
	}

	rtos_unlock_mutex(&cam_list->lock);
}

camera_handle_t bk_camera_handle_node_get_by_id_and_fomat(uint16_t id, uint16_t format)
{
	LIST_HEADER_T *pos, *n;
	media_camera_node_t *node = NULL, *tmp = NULL;;
	cam_list_t *cam_list = s_cam_handle_list;

	if (cam_list == NULL)
	{
		LOGE("%s %d list NULL\n", __func__, __LINE__);
		return NULL;
	}

	rtos_lock_mutex(&cam_list->lock);

	if (!list_empty(cam_list->list))
	{
		list_for_each_safe(pos, n, cam_list->list)
		{
			tmp = list_entry(pos, media_camera_node_t, list);
			if (tmp != NULL && tmp->id == id && tmp->format == format)
			{
				node = tmp;
				break;
			}
		}
	}

	rtos_unlock_mutex(&cam_list->lock);

	if (node)
	{
		return node->cam_handle;
	}

	return NULL;
}

camera_handle_t bk_camera_handle_node_pop(void)
{
	LIST_HEADER_T *pos, *n;
	media_camera_node_t *node = NULL, *tmp = NULL;;
	cam_list_t *cam_list = s_cam_handle_list;

	if (cam_list == NULL)
	{
		LOGE("%s %d list NULL\n", __func__, __LINE__);
		return NULL;
	}

	rtos_lock_mutex(&cam_list->lock);

	if (!list_empty(cam_list->list))
	{
		list_for_each_safe(pos, n, cam_list->list)
		{
			tmp = list_entry(pos, media_camera_node_t, list);
			if (tmp != NULL)
			{
				node = tmp;
				//list_del(pos);
				break;
			}
		}
	}

	rtos_unlock_mutex(&cam_list->lock);

	if (node)
	{
		return node->cam_handle;
	}

	return NULL;
}
