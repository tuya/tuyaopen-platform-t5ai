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

#include <driver/int.h>
#include <driver/jpeg_enc.h>
#include <driver/h264.h>
#include <driver/yuv_buf.h>
#include <driver/gpio.h>
#include <driver/dma.h>
#include "sys_driver.h"
#include "gpio_driver.h"
#include <modules/pm.h>
#include "bk_misc.h"
#include <driver/video_common_driver.h>

#define TAG "video_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    gpio_id_t gpio_id;
    gpio_dev_t dev;
} camera_gpio_map_t;

#define VIDEO_GPIO_PIN_NUMBER   12
#define VIDEO_GPIO_MAP \
    {\
        {GPIO_27, GPIO_DEV_JPEG_MCLK},\
        {GPIO_29, GPIO_DEV_JPEG_PCLK},\
        {GPIO_30, GPIO_DEV_JPEG_HSYNC},\
        {GPIO_31, GPIO_DEV_JPEG_VSYNC},\
        {GPIO_32, GPIO_DEV_JPEG_PXDATA0},\
        {GPIO_33, GPIO_DEV_JPEG_PXDATA1},\
        {GPIO_34, GPIO_DEV_JPEG_PXDATA2},\
        {GPIO_35, GPIO_DEV_JPEG_PXDATA3},\
        {GPIO_36, GPIO_DEV_JPEG_PXDATA4},\
        {GPIO_37, GPIO_DEV_JPEG_PXDATA5},\
        {GPIO_38, GPIO_DEV_JPEG_PXDATA6},\
        {GPIO_39, GPIO_DEV_JPEG_PXDATA7},\
    }

#define AUXS_CLK_CIS_ENABLE         1

bk_err_t bk_video_set_mclk(mclk_freq_t mclk)
{
    int ret = BK_OK;

#if (AUXS_CLK_CIS_ENABLE)

    gpio_dev_unmap(GPIO_27);
    gpio_dev_map(GPIO_27, GPIO_DEV_CLK_AUXS_CIS);

    sys_hal_set_cis_auxs_clk_en(1);
    switch (mclk)
    {
        case MCLK_15M:
            sys_drv_set_auxs_cis(3, 31);
            break;

        case MCLK_16M:
            sys_drv_set_auxs_cis(3, 29);
            break;

        case MCLK_20M:
            sys_drv_set_auxs_cis(3, 23);
            break;

        case MCLK_24M:
            sys_drv_set_auxs_cis(3, 19);
            break;

        case MCLK_30M:
            sys_drv_set_auxs_cis(3, 15);
            break;

        case MCLK_32M:
            sys_drv_set_auxs_cis(3, 14);
            break;

        case MCLK_40M:
            sys_drv_set_auxs_cis(3, 11);
            break;

        case MCLK_48M:
            sys_drv_set_auxs_cis(3, 9);
            break;

        default:
            return BK_FAIL;
    }
    sys_drv_set_jpeg_clk_sel(VIDEO_SYS_CLK_480M);
#endif

    sys_drv_set_jpeg_clk_sel(VIDEO_SYS_CLK_480M);
    switch (mclk)
    {
        case MCLK_16M:
        case MCLK_20M:
        case MCLK_24M:
            sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
            bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_6);
            break;

        case MCLK_30M:
            sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
            bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_4);
            break;

        case MCLK_40M:
            sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
            bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_3);
            break;

        default:
            return BK_FAIL;
    }

    return ret;
}

bk_err_t bk_video_dvp_mclk_enable(yuv_mode_t mode)
{
    // step 1: vid_power on, current only have jpeg_enc vote, need add yuv_buf/h264 .etc
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);

    // step 2: clk enable
    bk_pm_clock_ctrl(PM_CLK_ID_JPEG, CLK_PWR_CTRL_PWR_UP); //jpeg_clk

    sys_drv_yuv_buf_pwr_up(); // yuv_buf clk enable
    sys_drv_h264_pwr_up();    // h264 clk enable
    bk_yuv_buf_soft_reset();

    // step 3: config mclk
    bk_video_set_mclk(MCLK_24M);

    // step 4: enable encode
    bk_yuv_buf_start(mode);

    return BK_OK;
}

bk_err_t bk_video_dvp_mclk_disable(void)
{
    bk_pm_clock_ctrl(PM_CLK_ID_JPEG, CLK_PWR_CTRL_PWR_DOWN); //jpeg_clk
    sys_drv_yuv_buf_pwr_down(); // yuv_buf clk enable
    sys_drv_h264_pwr_down();    // h264 clk enable

#if (AUXS_CLK_CIS_ENABLE)
    sys_drv_set_cis_auxs_clk_en(0); // ausx_clk disable
#endif

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_OFF);

    return BK_OK;
}

/*SDK remove gpio_dev_map/unmap*/
#if 0
bk_err_t bk_video_gpio_init(dvp_gpio_mode_t mode)
{
    camera_gpio_map_t camera_gpio_map_table[] = VIDEO_GPIO_MAP;

    if (mode == DVP_GPIO_CLK)
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
            gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
        }
    }
    else if (mode == DVP_GPIO_DATA)
    {
        for (uint32_t i = 2; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
            gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
        }
    }
    else if (mode == DVP_GPIO_HSYNC_DATA)
    {
        gpio_dev_unmap(camera_gpio_map_table[2].gpio_id);
        gpio_dev_map(camera_gpio_map_table[2].gpio_id, camera_gpio_map_table[2].dev);
        for (uint32_t i = 4; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
            gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
        }
    }
    else // DVP_GPIO_ALL
    {
        for (uint32_t i = 0; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
            gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
        }
    }

    return BK_OK;
}

bk_err_t bk_video_gpio_deinit(dvp_gpio_mode_t mode)
{
    camera_gpio_map_t camera_gpio_map_table[] = VIDEO_GPIO_MAP;

    if (mode == DVP_GPIO_CLK)
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
        }
    }
    else if (mode == DVP_GPIO_DATA)
    {
        for (uint32_t i = 2; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
        }
    }
    else if (mode == DVP_GPIO_HSYNC_DATA)
    {
        gpio_dev_unmap(camera_gpio_map_table[2].gpio_id);
        for (uint32_t i = 4; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
        }
    }
    else //DVP_GPIO_ALL
    {
        for (uint32_t i = 0; i < VIDEO_GPIO_PIN_NUMBER; i++)
        {
            gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
        }
    }

    return BK_OK;
}
#endif