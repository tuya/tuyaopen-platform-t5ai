
// Copyright 2023-2024 Beken
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
#include <os/str.h>
#include <stdio.h>
#include "frame_buffer.h"
#include "bk_draw_blend.h"

#define TAG "bk_blend"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#ifdef DRAW_DIAG_DEBUG
#define DRAW_START()        do { GPIO_UP(GPIO_DVP_D3); } while (0)
#define DRAW_END()          do { GPIO_DOWN(GPIO_DVP_D3); } while (0)
#else
#define DRAW_START()
#define DRAW_END()
#endif

#define BLEND_ARRAY_LENGTH(array) \
    ({ \
        size_t length = 0; \
        if(array != NULL) { \
            while(array[length].addr != NULL) { \
                length++;  \
            }   \
        } \
        ++length;\
    })


const blend_info_t *bk_blend_assets = NULL;
const blend_info_t *bk_blend_info = NULL;
uint32_t blend_assets_size = 0;

dynamic_array_t g_dyn_array;

typedef enum
{
    BLEND_ADD = 0,
    BLEND_EXTI,
}blend_event_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} blend_msg_t;

typedef struct{
    beken_thread_t blend_task;
    beken_semaphore_t task_sem;
    beken_queue_t queue;
    bool task_running;
    uint8_t enable;
}blend_t;

static blend_t *blend = NULL;
#define BLEND_RETURN_NOT_INIT() do {\
	if (!blend->enable) {\
		return NULL;\
	}\
} while(0)


/**
 * @brief  blend font by cpu
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_font_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *font_info)
{
    if ((frame == NULL) && (font_info == NULL) && (font_info->addr == NULL))
    {
        LOGD("%s %d ERROR \n", __func__, __LINE__);
        return BK_FAIL;
    }

#if (CONFIG_BLEND)
    const bk_blend_t *font_strings = font_info->addr;
    font_blend_cfg_t cfg = {0};
    cfg.pbg_addr = (uint8_t *)(frame->frame);
    cfg.xsize = font_strings->width;
    cfg.ysize = font_strings->height;

    if ((font_strings->width + font_strings->xpos > lcd_width) || (font_strings->height + font_strings->ypos > lcd_height))
    {
        if (font_strings->xpos + font_strings->width > lcd_width)
        {
            LOGV("content: %s, xpos %d + width %d > lcd_width %d\n", __func__, font_strings->xpos, font_strings->width, lcd_width);
            cfg.xsize = lcd_width - font_strings->xpos;
        }
        if (font_strings->ypos  + font_strings->height > lcd_height)
        {
            LOGV("content: %s, ypos %d + height %d > lcd_width %d\n", __func__, font_strings->ypos, font_strings->height, lcd_height);
            cfg.ysize = lcd_height - font_strings->ypos;
        }
//        os_memset((void *)font_info->content, 0, sizeof(font_info->content));
//        return BK_FAIL;
    }

    cfg.xpos = font_strings->xpos;
    cfg.ypos = font_strings->ypos;
    cfg.str_num = 1;
    if (frame->fmt == PIXEL_FMT_VUYY)
    {
        cfg.font_format = FONT_VUYY;
    }
    else if (frame->fmt == PIXEL_FMT_YUYV)
    {
        cfg.font_format = FONT_YUYV;
    }
    else
    {
        cfg.font_format = FONT_RGB565;
    }
    cfg.str[0] = (font_str_t)
    {
        (const char *)font_info->content, font_strings->font.color, font_strings->font.font_digit_type, 0, 0
    };
    cfg.bg_data_format = frame->fmt;
    cfg.bg_width = frame->width;
    cfg.bg_height = frame->height;
    cfg.lcd_width = lcd_width;
    cfg.lcd_height = lcd_height;
    bk_font_blend(&cfg);
#endif
    return BK_OK;
}


/**
 * @brief  only blend icon of ARGB888 image by cpu or hardware dma2d
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_img_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const bk_blend_t *img_info)
{
    if ((frame == NULL) && (img_info == NULL))
    {
        return BK_FAIL;
    }
    image_blend_cfg_t cfg = {0};
    const bk_blend_t *img_dsc = (bk_blend_t *)img_info;
    if ((img_info->width + img_info->xpos > lcd_width) || (img_info->height + img_info->ypos > lcd_height))
    {
        LOGW("%s %d img  size is beyond the boundaries of lcd\n", __func__, __LINE__);
        if (img_dsc->width + img_dsc->xpos > lcd_width)
            LOGD("content: %s, xpos %d + width %d > lcd_width %d\n", __func__, img_dsc->xpos, img_dsc->width, lcd_width);
        if (img_dsc->height + img_dsc->ypos > lcd_height)
            LOGD("content: %s, ypos %d + height %d > lcd_width %d\n", __func__, img_dsc->ypos, img_dsc->height, lcd_height);

        return BK_FAIL;
    }
#if CONFIG_LCD_DMA2D_BLEND
    /**>  b7258 hw rotate output RGB565(big endian) is incompatible with DMA2D input RGB565 data format.*/
    cfg.pfg_addr = (uint8_t *)img_dsc->image.data;
    cfg.pbg_addr = (uint8_t *)(frame->frame);
    cfg.xsize = img_dsc->width;
    cfg.ysize = img_dsc->height;
    cfg.xpos = img_dsc->xpos;
    cfg.ypos = img_dsc->ypos;
    cfg.fg_alpha_value = 0xFF;
    cfg.fg_data_format = img_dsc->image.format;
    cfg.bg_data_format = frame->fmt;
    cfg.bg_width = frame->width;
    cfg.bg_height = frame->height;
    cfg.lcd_width = lcd_width;
    cfg.lcd_height = lcd_height;
    bk_dma2d_image_blend(&cfg);
#elif CONFIG_BLEND
    cfg.pfg_addr = (uint8_t *)img_dsc->image.data;
    cfg.pbg_addr = (uint8_t *)(frame->frame);
    cfg.xpos = img_dsc->xpos;
    cfg.ypos = img_dsc->ypos;
    cfg.xsize = img_dsc->width;
    cfg.ysize = img_dsc->height;
    cfg.fg_alpha_value = 0xff;
    cfg.fg_data_format = img_dsc->image.format;
    cfg.bg_data_format = frame->fmt;
    cfg.bg_width = frame->width;
    cfg.bg_height = frame->height;
    cfg.lcd_width = lcd_width;
    cfg.lcd_height = lcd_height;
    bk_image_blend(&cfg);
#endif
    return BK_OK;
}


bk_err_t bk_display_blend_handle_by_array(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *array, uint8_t array_size)
{
#if (CONFIG_BLEND)
    for (int i = 0; i < array_size; i++)
    {
        if (array[i].addr != NULL)
        {
            if (array[i].addr->blend_type == BLEND_TYPE_FONT)
                bk_display_blend_font_handle(frame, lcd_width, lcd_height, &array[i]);
            else if (array[i].addr->blend_type == BLEND_TYPE_IMAGE)
                bk_display_blend_img_handle(frame, lcd_width, lcd_height, array[i].addr);
        }
        #if 0
        else if (array[i].find_addr != NULL)
        {
        /*
            GUI_CONST_STORAGE bk_blend_t *img_logo[8] =
            {
                &img_wifi_rssi0,
                &img_wifi_rssi1,
                &img_wifi_rssi2,
                &img_wifi_rssi3,
                &img_wifi_rssi4,
                &img_battery_1,
                &img_cloudy_to_sunny,
                NULL,           //must in the end, and can't delete
            };
        */
            int j = 0;
            const bk_blend_t *(*temp_1)[0] = (array[i].find_addr);
             while ((*temp_1)[j] != NULL) 
             {
                if (os_strcmp((*temp_1)[j]->name, array[i].content) == 0)
                {
                    break;
                }
                j++;
            };
            if ((*temp_1)[j] != NULL)
            {
                bk_display_blend_img_handle(frame, lcd_width, lcd_height, (*temp_1)[j]);
            }
            else
            {
                LOGD("%s %d, i=%d, j=%d not find img %s \n", __func__, __LINE__, i,j, array[i].content);
            }
        }
        #endif
        else
        {
        }
    }
#endif
    return BK_OK;
}


/**
 * @brief  blend array include image and font
 * @param  blend background layer frame
 * @param  blend panel lcd width, to calculate postion in panel by (x, y) pos
 * @param  blend panel lcd heighe, to calculate postion in panel by (x, y) pos
 * @param  array ptr, the arrays end must be NULL
 * @return 
 *     - BK_OK: no error
 *     - BK_FAIL:not find blend image
 */
bk_err_t bk_display_blend_handle(frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const blend_info_t *array_ptr)
{
#if (CONFIG_BLEND)
    uint8_t i = 0;
    while (array_ptr[i].addr != NULL)
    {
        if (array_ptr[i].content[0] != '\0')
        {
            if (array_ptr[i].addr->blend_type == BLEND_TYPE_FONT)
            {
                bk_display_blend_font_handle(frame, lcd_width, lcd_height, &array_ptr[i]);
            }
            else if (array_ptr[i].addr->blend_type == BLEND_TYPE_IMAGE)
            {
                bk_display_blend_img_handle(frame, lcd_width, lcd_height, array_ptr[i].addr);
            }
        }
    i++;
    }
#endif
    return BK_OK;
}


bk_err_t blend_task_send_msg(uint8_t type, uint32_t param)
{
    int ret = BK_FAIL;
    blend_msg_t msg;
    blend_info_t *info = NULL;

    if (param != 0)
    {
        info = os_malloc(sizeof(blend_info_t));
        os_memcpy(info, (blend_info_t *)param, sizeof(blend_info_t));
        LOGV("%s %d %p %s %s\n", __func__,__LINE__, info, info->name, info->content);
    }

    if (blend && blend->task_running)
    {
        msg.event = type;
        msg.param = (uint32_t)info;
        ret = rtos_push_to_queue(&blend->queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret != BK_OK)
        {
            LOGE("%s push failed\n", __func__);
        }
    }
    if (ret != BK_OK)
    {
        if (info != NULL)
        {
            os_free(info);
        }
    }
    return ret;
}

bk_err_t dynamic_array_init(dynamic_array_t * dyn_array, size_t initial_capacity)
{
    uint32_t len = initial_capacity * sizeof(blend_info_t);
    dyn_array->entry = os_malloc(len);
    if (dyn_array->entry == NULL)
    {
        return BK_FAIL;
    }

    os_memset((void *)dyn_array->entry, 0, len);

    for(int i = 0; i < initial_capacity; i++)
    {
        dyn_array->entry[i].name[0] = '\0';
        dyn_array->entry[i].addr = NULL;
    }
    dyn_array->size = 0;
    dyn_array->capacity = initial_capacity;
    return BK_OK;
}

void copy_existing_blend_info_to_dynamic_array(dynamic_array_t * dyn_array)
{
    size_t length = BLEND_ARRAY_LENGTH(bk_blend_info);

    if (dyn_array->size + length > dyn_array->capacity)
    {
        dyn_array->capacity = dyn_array->size + length;
        dyn_array->entry = os_realloc(dyn_array->entry, dyn_array->capacity * sizeof(blend_info_t));
        if (dyn_array->entry == NULL)
        {
            LOGD("%s realloc fail \n", __func__);
            return;
        }
    }

    for (size_t i = 0; i < length; i++)
    {
        dyn_array->entry[dyn_array->size] = bk_blend_info[i];
        dyn_array->size++;
    }
}

const blend_info_t *find_blend_info_in_assets_by_name(const char *name)
{
    BLEND_RETURN_NOT_INIT();
    if (name == NULL)
    {
        return NULL;
    }

    for(int i = 0; i < blend_assets_size; i++)
    {
        if (strcmp((char *)bk_blend_assets[i].name, name) == 0)
        {
            if (bk_blend_assets[i].addr != NULL)
            {
                return &bk_blend_assets[i];
            }
        }
    }
    return NULL;
}

const blend_info_t *find_blend_info_in_assets_by_content(const char *content)
{
    BLEND_RETURN_NOT_INIT();
    if (content == NULL && content[0] == '\0')
    {
        return NULL;
    }
    for(int i = 0; i < blend_assets_size; i++)
    {
        if (strcmp((char *)bk_blend_assets[i].content, content) == 0)
        {
            if (bk_blend_assets[i].addr != NULL)
            {
                return &bk_blend_assets[i];
            }
        }
    }
    return NULL;
}

blend_info_t *find_blend_info_in_dynamic_array(dynamic_array_t * dyn_array, const char *name)
{
    BLEND_RETURN_NOT_INIT();
    if (name == NULL && name[0] == '\0')
    {
        return NULL;
    }

    for(int i = 0; i < dyn_array->size; i++)
    {
        if (strcmp(dyn_array->entry[i].name, name) == 0)
        {
            return &dyn_array->entry[i];
        }
    }
    return NULL;
}



void add_or_update_blend_info_to_dynamic_array(dynamic_array_t * dyn_array, const char *name, const char* content)
{
    size_t dyn_array_size = dyn_array->size;

    blend_info_t * exiting_info = find_blend_info_in_dynamic_array(dyn_array, name);
    if (exiting_info != NULL)
    {
        os_strncpy(exiting_info->name, name, sizeof(exiting_info->name) - 1);
        exiting_info->name[sizeof(exiting_info->name) - 1] = '\0';
        if (content != NULL)
        {
            os_strncpy(exiting_info->content, content, sizeof(exiting_info->content) - 1);
            exiting_info->content[sizeof(exiting_info->content) - 1] = '\0';
            if (content[0] != '\0')
            {
                if (exiting_info->addr->blend_type == BLEND_TYPE_IMAGE)
                {
                    const blend_info_t *update_img = find_blend_info_in_assets_by_content(content);
                    if (update_img != NULL)
                    {
                        exiting_info->addr = update_img->addr;
                    }
                    else
                    {
                        LOGW("warring!!!, not find img %s,'%s' in assets\n", exiting_info->name, content);
                        exiting_info->content[0] = '\0';
                    }
                }
            }
            else
            {
                LOGW("warring!!!, input no content \n");
            }
        }
        return;
    }

    const blend_info_t * assets_info = find_blend_info_in_assets_by_name(name);
    if (assets_info == NULL)
    {
        LOGW("%s, not fint assets %s\n", __func__, name);
        return;
    }

    if (dyn_array_size >= dyn_array->capacity)
    {
        dyn_array->capacity *= 2;
        dyn_array->entry = os_realloc(dyn_array->entry, dyn_array->capacity * sizeof(blend_info_t));
        if (dyn_array->entry == NULL)
        {
            LOGD("%s realloc fail \n", __func__);
            return;
        }
        for(int i = dyn_array_size; i < dyn_array->capacity; i++)
        {
            dyn_array->entry[i].name[0] = '\0';
            dyn_array->entry[i].addr = NULL;
        }
        LOGD("%s extend dyn_array capacity * 2\n", __func__);
    }
    dyn_array->entry[dyn_array_size] = *assets_info;

    strncpy(dyn_array->entry[dyn_array_size].name, name, sizeof(dyn_array->entry[dyn_array_size].name) - 1);
    dyn_array->entry[dyn_array_size].name[sizeof(dyn_array->entry[dyn_array_size].name) - 1] = '\0';
    if (content != NULL)
    {
        strncpy(dyn_array->entry[dyn_array_size].content, content, sizeof(dyn_array->entry[dyn_array_size].content) - 1);
        dyn_array->entry[dyn_array_size].content[sizeof(dyn_array->entry[dyn_array_size].content) - 1] = '\0';
        if (content[0] != '\0')
        {
            if (dyn_array->entry[dyn_array_size].addr->blend_type == BLEND_TYPE_IMAGE)
            {
                const blend_info_t *update_img = find_blend_info_in_assets_by_content(content);
                if (update_img != NULL)
                {
                    dyn_array->entry[dyn_array_size].addr = update_img->addr;
                }
                else
                {
                    LOGW("warring!!!, not find img %s,'%s' in assets\n", assets_info->name, content);
                    dyn_array->entry[dyn_array_size].content[0] = '\0';
                }
            }
        }
        else
        {
            LOGW("warring!!!, input no content \n");
        }
    }

    dyn_array->size++;
    dyn_array->entry[dyn_array->size].addr = NULL;
}

bk_err_t bk_draw_blend_update(blend_info_t *blend)
{
    return blend_task_send_msg(BLEND_ADD, (uint32_t)blend);
}


static void blend_task_entry(beken_thread_arg_t data)
{
    blend->task_running = true;
    rtos_set_semaphore(&blend->task_sem);

    while (blend->task_running)
    {
        blend_msg_t msg;
        int ret = rtos_pop_from_queue(&blend->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case BLEND_ADD:
                {
                    blend_info_t *info = (blend_info_t *)msg.param;
                    add_or_update_blend_info_to_dynamic_array(&g_dyn_array, info->name, info->content);
                    os_free(info);
                }
                break;

                case BLEND_EXTI:
                {
                    blend->task_running = false;
                    blend->blend_task = NULL;
                    rtos_set_semaphore(&blend->task_sem);
                    rtos_delete_thread(NULL);
                }
                break;
            }
        }
    }
}

static bk_err_t blend_task_start(void)
{
    int ret = BK_OK;

    ret = rtos_init_queue(&blend->queue,
                          "blend_queue",
                          sizeof(blend_msg_t),
                          15);
    if (ret != BK_OK)
    {
        LOGE("%s, init blend_queue failed\r\n", __func__);
         return ret;;
    }

    ret = rtos_create_thread(&blend->blend_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "blend_thread",
                             (beken_thread_function_t)blend_task_entry,
                             CONFIG_BLEND_TASK_STACK_SIZE,
                             (beken_thread_arg_t)NULL);

    if (BK_OK != ret)
    {
        LOGE("%s blend_thread init failed\n", __func__);
        return ret;
    }
    ret = rtos_get_semaphore(&blend->task_sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s decoder_sem get failed\n", __func__);
        return ret;
    }

    return ret;
}

static bk_err_t blend_task_stop(void)
{
    bk_err_t ret = BK_OK;
    if (!blend || blend->task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return ret;
    }

    blend_task_send_msg(BLEND_EXTI, 0);

    ret = rtos_get_semaphore(&blend->task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s get sem failed\n", __func__);
    }

    if (blend->queue)
    {
        rtos_deinit_queue(&blend->queue);
        blend->queue = NULL;
    }

    LOGD("%s complete\n", __func__);

    return ret;
}

void get_blend_assets_array(const blend_info_t *assets)
{
    bk_blend_assets = assets;
    blend_assets_size = BLEND_ARRAY_LENGTH(bk_blend_assets);
    LOGV("%s bk_blend_assets=%p blend_assets_size=%d \n", __func__,bk_blend_assets, blend_assets_size);
}

void get_blend_default_array(const blend_info_t *assets)
{
    bk_blend_info = assets;
    LOGV("%s bk_blend_info=%p \n", __func__, bk_blend_info);
}

bk_err_t bk_draw_blend_init(void)
{
    bk_err_t ret = BK_OK;
    if (NULL != blend && blend->task_running)
    {
        LOGV("%s already init\n", __func__);
        return ret;
    }
     blend = (blend_t *)os_malloc(sizeof(blend_t));
    if (blend == NULL)
    {
        LOGE("%s, malloc blend fail!\r\n", __func__);
        return BK_FAIL;
    }
    os_memset(blend, 0, sizeof(blend_t));
    
    ret = rtos_init_semaphore(&blend->task_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s task_sem init failed: %d\n", __func__, ret);
        goto error;
    }

    ret = blend_task_start();
    if (ret != BK_OK)
    {
        LOGE("%s blend_task creat failed: %d\n", __func__, ret);
        goto error;
    }


    ret = dynamic_array_init(&g_dyn_array, blend_assets_size);
    if(ret != BK_OK)
    {
        goto error;
    }
    copy_existing_blend_info_to_dynamic_array(&g_dyn_array);
    blend->enable = true;

    LOGD("%s complete\n", __func__);
    return BK_OK;

error:
    if (blend)
    {
        if (blend->task_sem)
        {
            rtos_deinit_semaphore(&blend->task_sem);
            blend->task_sem = NULL;
        }
        if (g_dyn_array.entry)
        {
            os_free(g_dyn_array.entry);
        }
        if (blend)
        {
            os_free(blend);
            blend = NULL;
        }
    }
    return ret;
}

bk_err_t bk_draw_blend_deinit(void)
{
    if (blend == NULL)
    {
        LOGE("%s, already deinit!\r\n", __func__);
        return BK_OK;
    }
    if (blend->task_sem)
    {
        rtos_deinit_semaphore(&blend->task_sem);
        blend->task_sem = NULL;
    }
    if (g_dyn_array.entry)
    {
        os_free(g_dyn_array.entry);
    }
    g_dyn_array.size = 0;
   blend->enable = false;

   if (blend)
   {
       os_free(blend);
       blend = NULL;
   }

    LOGD("%s complete\n", __func__);
    return BK_OK;
}

