#include "uvc_stream_list.h"
#include "uvc_urb_list.h"
#include "bk_list.h"

#define TAG "stream_list"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)



uvc_node_t *uvc_camera_stream_node_init(uvc_stream_handle_t *handle)
{
    bk_err_t ret = BK_FAIL;

    uvc_node_t *node = (uvc_node_t *)os_malloc(sizeof(uvc_node_t));
    if (node == NULL)
    {
        LOGE("%s, %d malloc fail\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto out;
    }
    os_memset(node, 0, sizeof(uvc_node_t));

    node->param = (camera_param_t *)os_malloc(sizeof(camera_param_t));
    if (node->param == NULL)
    {
        LOGE("%s, %d malloc fail\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto out;
    }
    os_memset(node->param, 0, sizeof(camera_param_t));

    node->param->info = (uvc_config_t *)os_malloc(sizeof(uvc_config_t));
    if (node->param->info == NULL)
    {
        LOGE("%s, %d malloc fail\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto out;
    }
    os_memset(node->param->info, 0, sizeof(uvc_config_t));
    //os_memcpy(node->param->info, config, sizeof(uvc_config_t));

    ret = rtos_init_semaphore(&node->param->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d malloc fail\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto out;
    }

    node->param->index = handle->stream_num++;

    list_add_tail(&node->list, &handle->list);

    LOGD("%s, %d\n", __func__, __LINE__);

    return node;

out:

    if (node)
    {
        if (node->param)
        {
            if (node->param->info)
            {
                os_free(node->param->info);
                node->param->info = NULL;
            }

            if (node->param->sem)
            {
                rtos_deinit_semaphore(&node->param->sem);
            }

            os_free(node->param);
            node->param = NULL;
        }

        os_free(node);
        node = NULL;
    }

    return NULL;
}

void uvc_camera_stream_node_deinit(uvc_stream_handle_t *handle)
{
    uvc_node_t *tmp = NULL, *node = NULL;
    LIST_HEADER_T *pos, *n;

    if (handle == NULL)
    {
        return;
    }

    list_for_each_safe(pos, n, &handle->list)
    {
        tmp = list_entry(pos, uvc_node_t, list);
        if (tmp != NULL)
        {
            node = tmp;
            list_del(pos);
            if (node->param)
            {
                if (node->param->sem)
                {
                    rtos_deinit_semaphore(&node->param->sem);
                }

                if (node->param->urb)
                {
                    uvc_camera_urb_free(node->param->urb);
                    node->param->urb = NULL;
                }

                if (node->param->frame)
                {
                    handle->callback.frame_free(node->param->info->img_format, node->param->stream, node->param->frame);
                    node->param->frame = NULL;
                }

                if (node->param->info)
                {
                    os_free(node->param->info);
                    node->param->info = NULL;
                }

                os_free(node->param);
                node->param = NULL;
            }

            handle->stream_num--;
            os_free(node);
            node = NULL;
        }
    }
}

camera_param_t *uvc_camera_stream_node_get_by_port_and_format(uvc_stream_handle_t *handle, uint8_t port, uint16_t format)
{
    LIST_HEADER_T *pos, *n;
    uvc_node_t *tmp = NULL;
    camera_param_t *node = NULL;

    if (handle == NULL)
    {
        return NULL;
    }

    if (!list_empty(&handle->list))
    {
        list_for_each_safe(pos, n, &handle->list)
        {
            tmp = list_entry(pos, uvc_node_t, list);
            if (tmp != NULL && tmp->param != NULL)
            {
                node = tmp->param;
                if (node->info && (node->info->port == port) && (node->info->img_format == format))
                {
                    break;
                }
            }
            node = NULL;
        }
    }

    return node;
}

camera_param_t *uvc_camera_stream_node_get_by_port_info(uvc_stream_handle_t *handle, bk_usb_hub_port_info *port_info)
{
    LIST_HEADER_T *pos, *n;
    uvc_node_t *tmp = NULL;
    camera_param_t *node = NULL;

    if (handle == NULL)
    {
        return NULL;
    }

    if (!list_empty(&handle->list))
    {
        list_for_each_safe(pos, n, &handle->list)
        {
            tmp = list_entry(pos, uvc_node_t, list);
            if (tmp != NULL && tmp->param != NULL)
            {
                node = tmp->param;
                if (node->port_info && (node->port_info == port_info))
                {
                    break;
                }
            }
            node = NULL;
        }
    }

    return node;
}

bool uvc_camera_stream_check_all_uvc_closed(uvc_stream_handle_t *handle)
{
    bool all_closed = true;
    LIST_HEADER_T *pos, *n;
    uvc_node_t *tmp = NULL;

    if (handle == NULL)
    {
        return all_closed;
    }

    if (!list_empty(&handle->list))
    {
        list_for_each_safe(pos, n, &handle->list)
        {
            tmp = list_entry(pos, uvc_node_t, list);
            if (tmp != NULL && tmp->param != NULL && tmp->param->camera_state == UVC_STREAMING_STATE)
            {
                all_closed = false;
                break;
            }
        }
    }

    return all_closed;
}