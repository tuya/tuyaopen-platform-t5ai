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


#include <common/bk_include.h>
#include <os/mem.h>
#include <components/log.h>
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include <driver/media_types.h>
#include <driver/lcd_qspi_types.h>


#define TAG "lcd_qspi_disp"

#define QSPI_DISP_LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define QSPI_DISP_LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define QSPI_DISP_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define QSPI_DISP_LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static beken_thread_t lcd_qspi_disp_thread_hdl = NULL;
static beken_semaphore_t g_lcd_qspi_task_sem;
static beken_semaphore_t g_lcd_qspi_disp_sem;
static uint32_t *g_lcd_qspi_imag_addr = NULL;
static bool lcd_qspi_disp_task_running = false;
extern uint8_t g_lcd_qspi_open_flag;

static void lcd_qspi_disp_task_entry(beken_thread_arg_t arg)
{
    const lcd_device_t *qspi_device = (const lcd_device_t *)arg;
    rtos_set_semaphore(&g_lcd_qspi_task_sem);

    while(lcd_qspi_disp_task_running) {
        rtos_get_semaphore(&g_lcd_qspi_disp_sem, 5);
        if (g_lcd_qspi_imag_addr) {
            bk_lcd_qspi_send_data(LCD_QSPI_ID, qspi_device, g_lcd_qspi_imag_addr, qspi_device->qspi->frame_len);
        }
    }

    lcd_qspi_disp_thread_hdl = NULL;
    rtos_set_semaphore(g_lcd_qspi_task_sem);
    rtos_delete_thread(NULL);
}

void bk_lcd_qspi_disp_task_start(const lcd_device_t *device)
{
    bk_err_t ret = BK_OK;

    if (lcd_qspi_disp_thread_hdl != NULL) {
        QSPI_DISP_LOGE("%s camera_display_thread already running\n", __func__);
        return;
    }

    ret = rtos_init_semaphore_ex(&g_lcd_qspi_task_sem, 1, 0);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s task_semaphore init failed\n", __func__);
        return;
    }

    ret = rtos_init_semaphore_ex(&g_lcd_qspi_disp_sem, 1, 0);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s disp_semaphore init failed\n", __func__);
        return;
    }

    lcd_qspi_disp_task_running = true;

    ret = rtos_create_thread(&lcd_qspi_disp_thread_hdl,
                             3,
                             "qspi_disp",
                             (beken_thread_function_t)lcd_qspi_disp_task_entry,
                             4096,
                             (beken_thread_arg_t)device);
    if (ret != kNoErr) {
        QSPI_DISP_LOGE("lcd qspi display task create fail\r\n");
        rtos_deinit_semaphore(&g_lcd_qspi_disp_sem);
        rtos_deinit_semaphore(&g_lcd_qspi_task_sem);
        return;
    } else {
        QSPI_DISP_LOGI("lcd qspi display task create success\r\n");
    }

    ret = rtos_get_semaphore(&g_lcd_qspi_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s g_lcd_qspi_task_sem get failed\n", __func__);
    }
}

void bk_lcd_qspi_disp_task_stop(void)
{
    bk_err_t ret;

    if (lcd_qspi_disp_task_running == false) {
        QSPI_DISP_LOGI("%s already stop\n", __func__);
        return;
    }

    lcd_qspi_disp_task_running == false;

    ret = rtos_get_semaphore(&g_lcd_qspi_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s g_lcd_qspi_task_sem get failed\n");
        return;
    }

    QSPI_DISP_LOGI("%s complete\n", __func__);

    ret = rtos_deinit_semaphore(&g_lcd_qspi_disp_sem);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s g_lcd_qspi_disp_sem deinit failed\n");
        return;
    }

    ret = rtos_deinit_semaphore(&g_lcd_qspi_task_sem);
    if (BK_OK != ret) {
        QSPI_DISP_LOGE("%s g_lcd_qspi_task_sem deinit failed\n");
        return;
    }
}

void bk_lcd_qspi_display(uint32_t frame)
{
    if (g_lcd_qspi_open_flag) {
        g_lcd_qspi_imag_addr = (uint32_t *)frame;
        rtos_set_semaphore(&g_lcd_qspi_disp_sem);
    } else {
        QSPI_DISP_LOGI("[%s] close success\r\n", __FUNCTION__);
    }
}

