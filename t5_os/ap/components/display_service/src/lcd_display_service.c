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
#include <os/str.h>
#include <components/log.h>
#include <driver/lcd.h>
#include <driver/flash.h>
#include "lcd_display_service.h"
#include "frame_buffer.h"
#include "yuv_encode.h"
#if CONFIG_LCD_QSPI
#include <lcd_qspi_display_service.h>
#endif
#include "mux_pipeline.h"

#include "bk_draw_blend.h"

#if CONFIG_LCD_SPI_DISPLAY
#include <lcd_spi_display_service.h>
#endif

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif

#define TAG "lcd_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef DISP_DIAG_DEBUG
#define DISPLAY_START()         do { GPIO_UP(GPIO_DVP_D6); } while (0)
#define DISPLAY_END()           do { GPIO_DOWN(GPIO_DVP_D6); } while (0)
#define DISPLAY_ISR_START()         do { GPIO_UP(GPIO_DVP_D7); } while (0)
#define DISPLAY_ISR_END()           do { GPIO_DOWN(GPIO_DVP_D7); } while (0)
#else
#define DISPLAY_START()
#define DISPLAY_END()
#define DISPLAY_ISR_START()
#define DISPLAY_ISR_END()
#endif

extern media_debug_t *media_debug;
extern uint32_t  platform_is_in_interrupt_context(void);

typedef struct
{
    uint32_t event;
    uint32_t param;
} display_msg_t;


typedef struct
{
    uint8_t disp_task_running : 1;
    uint8_t lcd_type;
    uint16_t lcd_width;
    uint16_t lcd_height;
    frame_buffer_t *pingpong_frame;
    frame_buffer_t *display_frame;
    beken_semaphore_t disp_sem;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
} lcd_disp_config_t;

typedef struct
{
    beken_mutex_t lock;
} display_service_info_t;

typedef enum
{
    DISPLAY_FRAME_REQUEST,
    DISPLAY_FRAME_FREE,
    DISPLAY_FRAME_EXIT,
} lcd_display_msg_type_t;

static lcd_disp_config_t *lcd_disp_config = NULL;
static display_service_info_t *service_info = NULL;

bk_err_t lcd_display_task_send_msg(uint8_t type, uint32_t param);

#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t display_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t display_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&display_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void display_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&display_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

static void lcd_display_free(frame_buffer_t *frame)
{
    if (frame->cb)
    {
        frame->cb->free(frame);
    }
    else
    {
        frame_buffer_display_free(frame);
    }
}

static void lcd_driver_display_mcu_isr(void)
{
    media_debug->isr_lcd++;

    if (lcd_disp_config->pingpong_frame != NULL)
    {
        media_debug->fps_lcd++;
        if (lcd_disp_config->display_frame)
        {
            lcd_display_free(lcd_disp_config->display_frame);
            lcd_disp_config->display_frame = NULL;
        }

        uint32_t flag = 0;
        flag = display_enter_critical();
        lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
        lcd_disp_config->pingpong_frame = NULL;
        display_exit_critical(flag);
        bk_lcd_8080_start_transfer(0);

        rtos_set_semaphore(&lcd_disp_config->disp_sem);
    }
}

#if CONFIG_LV_ATTRIBUTE_FAST_MEM
static void lcd_driver_display_rgb_isr(void)
#else
__attribute__((section(".itcm_sec_code"))) static void lcd_driver_display_rgb_isr(void)
#endif
{
    DISPLAY_ISR_START();
    media_debug->isr_lcd++;
    uint32_t flag = 0;
    if (lcd_disp_config->pingpong_frame != NULL)
    {
        if (lcd_disp_config->display_frame != NULL)
        {
            frame_buffer_t *temp_buffer = NULL;
            bk_err_t ret = BK_OK;
            media_debug->fps_lcd++;

            flag = display_enter_critical();

            if (lcd_disp_config->pingpong_frame != lcd_disp_config->display_frame)
            {
                if (lcd_disp_config->display_frame->width != lcd_disp_config->pingpong_frame->width
                    || lcd_disp_config->display_frame->height != lcd_disp_config->pingpong_frame->height)
                {
                    lcd_driver_ppi_set(lcd_disp_config->pingpong_frame->width, lcd_disp_config->pingpong_frame->height);
                }
                if (lcd_disp_config->display_frame->fmt != lcd_disp_config->pingpong_frame->fmt)
                {
                    bk_lcd_set_yuv_mode(lcd_disp_config->pingpong_frame->fmt);
                }
                if (lcd_disp_config->display_frame->cb != NULL
                    && lcd_disp_config->display_frame->cb->free != NULL)
                {
                    lcd_disp_config->display_frame->cb->free(lcd_disp_config->display_frame);
                }
                else
                {
                    temp_buffer = lcd_disp_config->display_frame;
                    lcd_disp_config->display_frame = NULL;
                }
            }
            lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
            lcd_disp_config->pingpong_frame = NULL;

            lcd_driver_set_display_base_addr((uint32_t)lcd_disp_config->display_frame->frame);

            if (temp_buffer != NULL)
            {
                ret = lcd_display_task_send_msg(DISPLAY_FRAME_FREE, (uint32_t)temp_buffer);
                if (ret != BK_OK)
                {
                    lcd_display_free(temp_buffer);
                }
            }
            display_exit_critical(flag);
            rtos_set_semaphore(&lcd_disp_config->disp_sem);
        }
        else
        {
            flag = display_enter_critical();
            lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
            lcd_disp_config->pingpong_frame = NULL;
            display_exit_critical(flag);
            rtos_set_semaphore(&lcd_disp_config->disp_sem);
        }
    }
    DISPLAY_ISR_END();
}

static bk_err_t lcd_display_frame(frame_buffer_t *frame)
{
    bk_err_t ret = BK_FAIL;
    DISPLAY_START();

    if (lcd_disp_config->display_frame == NULL)
    {
        lcd_driver_ppi_set(frame->width, frame->height);

        bk_lcd_set_yuv_mode(frame->fmt);
        uint32_t flag = 0;
        flag = display_enter_critical();
        lcd_disp_config->pingpong_frame = frame;
        display_exit_critical(flag);

        lcd_driver_set_display_base_addr((uint32_t)frame->frame);
        lcd_driver_display_enable();
        LOGD("display start, frame width, height %d, %d\n", frame->width, frame->height);
    }
    else
    {
        uint32_t flag = 0;
        flag = display_enter_critical();
        if (lcd_disp_config->pingpong_frame != NULL)
        {
            lcd_display_free(lcd_disp_config->pingpong_frame);
            lcd_disp_config->pingpong_frame = NULL;
        }
        display_exit_critical(flag);

        lcd_disp_config->pingpong_frame = frame;

        if (lcd_disp_config->lcd_type == LCD_TYPE_MCU8080)
        {
            lcd_driver_ppi_set(frame->width, frame->height);
            bk_lcd_set_yuv_mode(frame->fmt);
            lcd_driver_set_display_base_addr((uint32_t)frame->frame);

            lcd_driver_display_continue();
        }
    }

    ret = rtos_get_semaphore(&lcd_disp_config->disp_sem, BEKEN_NEVER_TIMEOUT);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
    DISPLAY_END();

    return ret;
}

bk_err_t lcd_display_task_send_msg(uint8_t type, uint32_t param)
{
    int ret = BK_FAIL;
    display_msg_t msg;
    uint32_t isr_context = platform_is_in_interrupt_context();

    if (lcd_disp_config)
    {
        msg.event = type;
        msg.param = param;

        if (!isr_context)
        {
            rtos_lock_mutex(&service_info->lock);
        }

        if (lcd_disp_config->disp_task_running)
        {
            ret = rtos_push_to_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret != BK_OK)
            {
                LOGE("%s push failed\n", __func__);
            }
        }

        if (!isr_context)
        {
            rtos_unlock_mutex(&service_info->lock);
        }

    }

    return ret;
}

bk_err_t lcd_display_frame_request(frame_buffer_t *frame)
{
    return lcd_display_task_send_msg(DISPLAY_FRAME_REQUEST, (uint32_t)frame);
}

static void lcd_display_task_entry(beken_thread_arg_t data)
{
    lcd_disp_config->disp_task_running = true;

    rtos_set_semaphore(&lcd_disp_config->disp_task_sem);

    while (lcd_disp_config->disp_task_running)
    {
        display_msg_t msg;
        int ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case DISPLAY_FRAME_REQUEST:
#if (CONFIG_BLEND_UI)
                    if (g_dyn_array.size > 0)
                    {
                        blend_info_t *info = &g_dyn_array.entry[0];
                        bk_display_blend_handle((frame_buffer_t *)msg.param, lcd_disp_config->lcd_width,
                                                lcd_disp_config->lcd_height, info);
                    }
#endif
                    lcd_display_frame((frame_buffer_t *)msg.param);
                    break;
                case DISPLAY_FRAME_FREE:
                    lcd_display_free((frame_buffer_t *)msg.param);
#if CONFIG_MEDIA_PSRAM_SIZE_4M
                    extern void jpeg_decode_get_next_frame();
                    jpeg_decode_get_next_frame();
#endif
                    break;

                case DISPLAY_FRAME_EXIT:
                {
                    rtos_lock_mutex(&service_info->lock);
                    lcd_disp_config->disp_task_running = false;
                    rtos_unlock_mutex(&service_info->lock);

                    do
                    {
                        ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_NO_WAIT);

                        if (ret == BK_OK)
                        {
                            if (msg.event == DISPLAY_FRAME_REQUEST || msg.event == DISPLAY_FRAME_FREE)
                            {
                                lcd_display_free((frame_buffer_t *)msg.param);
                            }
                        }
                    }
                    while (ret == BK_OK);
                }
                goto exit;
            }
        }
    }

exit:

    lcd_disp_config->disp_task = NULL;
    rtos_set_semaphore(&lcd_disp_config->disp_task_sem);
    rtos_delete_thread(NULL);
}


static bk_err_t lcd_display_task_start(void)
{
    int ret = BK_OK;

    ret = rtos_init_queue(&lcd_disp_config->queue,
                          "display_queue",
                          sizeof(display_msg_t),
                          15);

    if (ret != BK_OK)
    {
        LOGE("%s, init display_queue failed\r\n", __func__);
        return ret;
    }

    ret = rtos_create_thread(&lcd_disp_config->disp_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "display_thread",
                             (beken_thread_function_t)lcd_display_task_entry,
                             1024 * 2,
                             (beken_thread_arg_t)NULL);

    if (BK_OK != ret)
    {
        LOGE("%s lcd_display_thread init failed\n", __func__);
        return ret;
    }

    ret = rtos_get_semaphore(&lcd_disp_config->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s decoder_sem get failed\n", __func__);
    }

    return ret;
}

static bk_err_t lcd_display_task_stop(void)
{
    bk_err_t ret = BK_OK;
    if (!lcd_disp_config || lcd_disp_config->disp_task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return ret;
    }

    lcd_display_task_send_msg(DISPLAY_FRAME_EXIT, 0);

    ret = rtos_get_semaphore(&lcd_disp_config->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s jpeg_display_sem get failed\n", __func__);
    }

    if (lcd_disp_config->queue)
    {
        rtos_deinit_queue(&lcd_disp_config->queue);
        lcd_disp_config->queue = NULL;
    }

    LOGD("%s complete\n", __func__);

    return ret;
}


bk_err_t lcd_display_config_free(void)
{
    int ret = BK_OK;

    if (lcd_disp_config)
    {
        if (lcd_disp_config->disp_task_sem)
        {
            rtos_deinit_semaphore(&lcd_disp_config->disp_task_sem);
            lcd_disp_config->disp_task_sem = NULL;
        }

        lcd_driver_deinit();

        if (lcd_disp_config->disp_sem)
        {
            rtos_deinit_semaphore(&lcd_disp_config->disp_sem);
            lcd_disp_config->disp_sem = NULL;
        }

        if (lcd_disp_config->pingpong_frame)
        {
            LOGD("%s pingpong_frame free\n", __func__);
            lcd_display_free(lcd_disp_config->pingpong_frame);
            lcd_disp_config->pingpong_frame = NULL;
        }

        if (lcd_disp_config->display_frame)
        {
            LOGD("%s display_frame free\n", __func__);
            lcd_display_free(lcd_disp_config->display_frame);
            lcd_disp_config->display_frame = NULL;
        }

        if (lcd_disp_config)
        {
            os_free(lcd_disp_config);
            lcd_disp_config = NULL;
        }
    }
    LOGV("%s %d\n", __func__, __LINE__);
    return ret;
}


bool check_lcd_task_is_open(void)
{
    if (lcd_disp_config == NULL)
    {
        return false;
    }
    else
    {
        return lcd_disp_config->disp_task_running;
    }
}

uint8_t lcd_display_get_type(void)
{
    if (lcd_disp_config == NULL)
    {
        LOGD("%s lcd_disp_config is null\r\n", __func__);
        return 0;
    }
    else
    {
        return lcd_disp_config->lcd_type;
    }

}

bk_err_t lcd_display_open(lcd_open_t *config)
{
    int ret = BK_OK;
    const lcd_device_t *lcd_device = NULL;

    if (lcd_disp_config && lcd_disp_config->disp_task_running)
    {
        LOGE("%s lcd display task is running!\r\n", __func__);
        return ret;
    }

    lcd_disp_config = (lcd_disp_config_t *)os_malloc(sizeof(lcd_disp_config_t));
    if (lcd_disp_config == NULL)
    {
        LOGE("%s, malloc lcd_disp_config fail!\r\n", __func__);
        ret = BK_FAIL;
        return ret;
    }

    os_memset(lcd_disp_config, 0, sizeof(lcd_disp_config_t));

    ret = rtos_init_semaphore(&lcd_disp_config->disp_task_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_semaphore(&lcd_disp_config->disp_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s disp_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    if (config->device_name != NULL)
    {
        lcd_device = get_lcd_device_by_name(config->device_name);
    }

    if (lcd_device == NULL)
    {
        lcd_device = get_lcd_device_by_ppi(config->device_ppi);
    }

    if (lcd_device == NULL)
    {
        LOGE("%s lcd device not found\n", __func__);
        goto out;
    }
    lcd_disp_config->lcd_width = lcd_device->ppi >> 16;
    lcd_disp_config->lcd_height = lcd_device->ppi & 0xFFFF;
    lcd_disp_config->lcd_type = lcd_device->type;

    // step 4: init frame buffer
    LOGD("%s %d lcd ppi:%d %d\n", __func__, __LINE__, lcd_disp_config->lcd_width, lcd_disp_config->lcd_height);

    // step 5: init lcd display
    ret = lcd_driver_init(lcd_device);
    if (ret != BK_OK)
    {
        LOGE("%s, lcd_driver_init fail\r\n", __func__);
        goto out;
    }

    if (lcd_device->type == LCD_TYPE_MCU8080)
    {
        bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_driver_display_mcu_isr);
    }
    else
    {
        bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_driver_display_rgb_isr);
    }

    media_debug->fps_lcd = 0;
    media_debug->isr_lcd = 0;

    if (lcd_device->type == LCD_TYPE_SPI)
    {
#if CONFIG_LCD_SPI_DISPLAY
        lcd_spi_init(lcd_device);
#endif
    }
    else if (lcd_device->type == LCD_TYPE_QSPI)
    {
#if CONFIG_LCD_QSPI
        bk_lcd_qspi_disp_task_start(lcd_device);
        lcd_disp_config->disp_task_running = true;
#endif
    }
    else
    {
        ret = lcd_display_task_start();
        if (ret != BK_OK)
        {
            LOGE("%s lcd_display_task_start failed: %d\n", __func__, ret);
            goto out;
        }
    }

    lcd_driver_backlight_open();

    LOGD("%s %d complete\n", __func__, __LINE__);

    return ret;

out:

    lcd_display_config_free();
    lcd_driver_backlight_close();
    LOGE("%s failed\r\n", __func__);

    return ret;
}

bk_err_t lcd_display_close(void)
{
    LOGV("%s, %d\n", __func__, __LINE__);

    if (lcd_disp_config == NULL)
    {
        LOGE("%s, have been closed!\r\n", __func__);
        return BK_OK;
    }

    lcd_driver_backlight_close();

    if (lcd_disp_config->lcd_type == LCD_TYPE_QSPI)
    {
#if CONFIG_LCD_QSPI
        bk_lcd_qspi_disp_task_stop();
        lcd_disp_config->disp_task_running = false;
#endif
    }
    else if (lcd_disp_config->lcd_type == LCD_TYPE_SPI)
    {
#if CONFIG_LCD_SPI_DISPLAY
        lcd_spi_deinit();
#endif
    }
    else
    {
        lcd_display_task_stop();
    }

    lcd_display_config_free();
    LOGD("%s complete, %d\n", __func__, __LINE__);
    return BK_OK;
}

bk_err_t lcd_display_service_init(void)
{
    bk_err_t ret =  BK_FAIL;
    service_info = os_malloc(sizeof(display_service_info_t));
    if (service_info == NULL)
    {
        LOGE("%s, malloc service_info failed\n", __func__);
        goto error;
    }
    os_memset(service_info, 0, sizeof(display_service_info_t));

    ret = rtos_init_mutex(&service_info->lock);
    if (ret != BK_OK)
    {
        LOGE("%s, init mutex failed\n", __func__);
        goto error;
    }

    return BK_OK;
error:
    return BK_FAIL;
}

