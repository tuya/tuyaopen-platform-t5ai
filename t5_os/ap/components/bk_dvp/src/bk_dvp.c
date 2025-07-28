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

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/gpio_types.h>
#include <driver/gpio.h>
#include <driver/h264.h>
#include <driver/yuv_buf.h>

#include <driver/dvp_camera.h>

#include "bk_misc.h"
#include "gpio_driver.h"
#include <driver/video_common_driver.h>
#include "avdk_crc.h"
#include "media_utils.h"

#define TAG "dvp_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define H264_SELF_DEFINE_SEI_SIZE (96)
#define FRAME_BUFFER_CACHE (1024 * 10)

extern uint8_t *media_bt_share_buffer;

//#define DVP_DIAG_DEBUG

#ifdef DVP_DIAG_DEBUG

#define DVP_DIAG_DEBUG_INIT()                   \
    do {                                        \
        gpio_dev_unmap(GPIO_2);                 \
        bk_gpio_disable_pull(GPIO_2);           \
        bk_gpio_enable_output(GPIO_2);          \
        bk_gpio_set_output_low(GPIO_2);         \
        \
        gpio_dev_unmap(GPIO_3);                 \
        bk_gpio_disable_pull(GPIO_3);           \
        bk_gpio_enable_output(GPIO_3);          \
        bk_gpio_set_output_low(GPIO_3);         \
        \
        gpio_dev_unmap(GPIO_4);                 \
        bk_gpio_disable_pull(GPIO_4);           \
        bk_gpio_enable_output(GPIO_4);          \
        bk_gpio_set_output_low(GPIO_4);         \
        \
        gpio_dev_unmap(GPIO_5);                 \
        bk_gpio_disable_pull(GPIO_5);           \
        bk_gpio_enable_output(GPIO_5);          \
        bk_gpio_set_output_low(GPIO_5);         \
        \
        gpio_dev_unmap(GPIO_12);                \
        bk_gpio_disable_pull(GPIO_12);          \
        bk_gpio_enable_output(GPIO_12);         \
        bk_gpio_set_output_low(GPIO_12);        \
        \
        gpio_dev_unmap(GPIO_13);                \
        bk_gpio_disable_pull(GPIO_13);          \
        bk_gpio_enable_output(GPIO_13);         \
        bk_gpio_set_output_low(GPIO_13);        \
        \
    } while (0)

#define DVP_JPEG_VSYNC_ENTRY()          bk_gpio_set_output_high(GPIO_2)
#define DVP_JPEG_VSYNC_OUT()            bk_gpio_set_output_low(GPIO_2)

#define DVP_JPEG_EOF_ENTRY()            bk_gpio_set_output_high(GPIO_3)
#define DVP_JPEG_EOF_OUT()              bk_gpio_set_output_low(GPIO_3)

#define DVP_YUV_EOF_ENTRY()             bk_gpio_set_output_high(GPIO_4)
#define DVP_YUV_EOF_OUT()               bk_gpio_set_output_low(GPIO_4)

#define DVP_JPEG_START_ENTRY()          bk_gpio_set_output_high(GPIO_5)
#define DVP_JPEG_START_OUT()            bk_gpio_set_output_low(GPIO_5)

#define DVP_JPEG_HEAD_ENTRY()           bk_gpio_set_output_high(GPIO_12)
#define DVP_JPEG_HEAD_OUT()             bk_gpio_set_output_low(GPIO_12)

#define DVP_PPI_ERROR_ENTRY()           DVP_YUV_EOF_ENTRY()
#define DVP_PPI_ERROR_OUT()             DVP_YUV_EOF_OUT()

#define DVP_H264_EOF_ENTRY()            DVP_JPEG_EOF_ENTRY()
#define DVP_H264_EOF_OUT()              DVP_JPEG_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()           bk_gpio_set_output_high(GPIO_13)
#define DVP_JPEG_SDMA_OUT()             bk_gpio_set_output_low(GPIO_13)

#define DVP_DEBUG_IO()                      \
    do {                                    \
        bk_gpio_set_output_high(GPIO_6);    \
        bk_gpio_set_output_low(GPIO_6);     \
    } while (0)

#else
#define DVP_DIAG_DEBUG_INIT()

#define DVP_JPEG_EOF_ENTRY()
#define DVP_JPEG_EOF_OUT()

#define DVP_YUV_EOF_ENTRY()
#define DVP_YUV_EOF_OUT()

#define DVP_JPEG_START_ENTRY()
#define DVP_JPEG_START_OUT()

#define DVP_JPEG_HEAD_ENTRY()
#define DVP_JPEG_HEAD_OUT()

#define DVP_PPI_ERROR_ENTRY()
#define DVP_PPI_ERROR_OUT()

#define DVP_H264_EOF_ENTRY()
#define DVP_H264_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()
#define DVP_JPEG_SDMA_OUT()

#define DVP_JPEG_VSYNC_ENTRY()
#define DVP_JPEG_VSYNC_OUT()

#endif

typedef struct
{
    uint32_t yuv_em_addr;
    uint32_t yuv_pingpong_length;
    uint32_t yuv_data_offset;
    uint8_t dma_collect_yuv;
} encode_yuv_config_t;

typedef struct
{
    uint8_t eof : 1;
    uint8_t error : 1;
    uint8_t i_frame : 1;
    uint8_t not_free : 1;
    uint8_t regenerate_idr : 1;
    uint8_t sequence;

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
    uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif

    uint8_t dma_channel;
    uint32_t frame_id;
    uint32_t dma_length;
    uint8_t *encode_buffer;
    media_state_t dvp_state;
    beken_semaphore_t sem;
    frame_buffer_t *encode_frame;
    frame_buffer_t *yuv_frame;
    dvp_config_t config;
    bk_dvp_callback_t callback;
    encode_yuv_config_t *yuv_config;
    const dvp_sensor_config_t *sensor;
    void *enc_stream;
    void *yuv_stream;

#if (MEDIA_DEBUG_TIMER_ENABLE)
    beken_timer_t timer;
    uint32_t curr_length;
    uint32_t later_seq;
    uint32_t later_kbps;
    uint32_t latest_kbps;
#endif
} dvp_driver_handle_t;

static dvp_driver_handle_t *s_dvp_camera_handle = NULL;
static uint8_t *dvp_camera_encode = NULL;
static const dvp_sensor_config_t **devices_list = NULL;
static uint16_t devices_size = 0;

#if (MEDIA_DEBUG_TIMER_ENABLE)
static void dvp_camera_timer_handle(void *param)
{
    dvp_driver_handle_t *handle = param;
    // fps[sequence length(current) Kps]
    uint32_t fps = 0, kbps = 0;

    fps = (handle->frame_id - handle->later_seq) / MEDIA_DEBUG_TIMER_INTERVAL;
    kbps = (handle->latest_kbps - handle->later_kbps) / MEDIA_DEBUG_TIMER_INTERVAL * 8 / 1000;
    handle->later_seq = handle->frame_id;
    handle->later_kbps = handle->latest_kbps;
    LOGW("dvp:%d[%d %dKB %dKbps]\n",
        fps, handle->frame_id, handle->curr_length / 1024, kbps);
}
#endif

const dvp_sensor_config_t **get_sensor_config_devices_list(void)
{
    return devices_list;
}

int get_sensor_config_devices_num(void)
{
    return devices_size;
}

void bk_dvp_set_devices_list(const dvp_sensor_config_t **list, uint16_t size)
{
    devices_list = list;
    devices_size = size;
}

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
    uint32_t i;

    for (i = 0; i < devices_size; i++)
    {
        if (devices_list[i]->id == id)
        {
            return devices_list[i];
        }
    }

    return NULL;
}

const dvp_sensor_config_t *bk_dvp_get_sensor_auto_detect(void)
{
    uint32_t i;
    uint8_t count = 3;

    do
    {
        for (i = 0; i < devices_size; i++)
        {
            if (devices_list[i]->detect() == true)
            {
                return devices_list[i];
            }
        }

        count--;

        //rtos_delay_milliseconds(5);

    }
    while (count > 0);

    return NULL;
}

static bk_err_t dvp_camera_init_device(dvp_driver_handle_t *handle)
{
    dvp_config_t *config = &handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;

    if (config->width != (sensor->def_ppi >> 16) ||
        config->height != (sensor->def_ppi & 0xFFFF))
    {
        if (!(pixel_ppi_to_cap((config->width << 16)
                               | config->height) & (sensor->ppi_cap)))
        {
            LOGE("%s, %d, not support this resolution...\r\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;
}

static void dvp_camera_dma_finish_callback(dma_id_t id)
{
    DVP_JPEG_SDMA_ENTRY();
    s_dvp_camera_handle->dma_length += FRAME_BUFFER_CACHE;
    DVP_JPEG_SDMA_OUT();
}

static bk_err_t dvp_camera_dma_config(dvp_driver_handle_t *handle)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t encode_fifo_addr;
    dvp_config_t *config = &handle->config;

    if (config->img_format & IMAGE_H264)
    {
        bk_h264_get_fifo_addr(&encode_fifo_addr);
        handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
        handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
    }

    LOGV("dvp_dma id:%d \r\n", handle->dma_channel);

    if (handle->dma_channel >= DMA_ID_MAX)
    {
        LOGE("malloc dma fail \r\n");
        ret = BK_FAIL;
        return ret;
    }

    if (config->img_format & IMAGE_H264)
    {
        handle->encode_frame = handle->callback.frame_malloc(IMAGE_H264, handle->enc_stream, CONFIG_H264_FRAME_SIZE);
        handle->encode_frame->fmt = PIXEL_FMT_H264;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        handle->encode_frame = handle->callback.frame_malloc(IMAGE_MJPEG, handle->enc_stream, CONFIG_JPEG_FRAME_SIZE);
        handle->encode_frame->fmt = PIXEL_FMT_JPEG;
    }

    if (handle->encode_frame == NULL)
    {
        LOGE("malloc frame fail \r\n");
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    handle->encode_frame->width = config->width;
    handle->encode_frame->height = config->height;

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.start_addr = encode_fifo_addr;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

    if (config->img_format & IMAGE_H264)
    {
        dma_config.src.dev = DMA_DEV_H264;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        dma_config.src.dev = DMA_DEV_JPEG;
    }

    dma_config.dst.start_addr = (uint32_t)handle->encode_frame->frame;
    dma_config.dst.end_addr = (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size);
    BK_LOG_ON_ERR(bk_dma_init(handle->dma_channel, &dma_config));
    BK_LOG_ON_ERR(bk_dma_set_transfer_len(handle->dma_channel, FRAME_BUFFER_CACHE));
    BK_LOG_ON_ERR(bk_dma_register_isr(handle->dma_channel, NULL, dvp_camera_dma_finish_callback));
    BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(handle->dma_channel));
#if (CONFIG_SPE)
    BK_LOG_ON_ERR(bk_dma_set_src_burst_len(handle->dma_channel, BURST_LEN_SINGLE));
    BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(handle->dma_channel, BURST_LEN_INC16));
    BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
    BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
#endif
    BK_LOG_ON_ERR(bk_dma_start(handle->dma_channel));

    return ret;
}

static bk_err_t encode_yuv_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
    dma_config_t dma_config = {0};
    os_memset(&dma_config, 0, sizeof(dma_config_t));

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 1;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)in;
    dma_config.src.end_addr = (uint32_t)(in + len);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)out;
    dma_config.dst.end_addr = (uint32_t)(out + len);

    BK_LOG_ON_ERR(bk_dma_init(cpy_chnl, &dma_config));
    BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnl, len));
#if (CONFIG_SPE)
    BK_LOG_ON_ERR(bk_dma_set_src_burst_len(cpy_chnl, 3));
    BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(cpy_chnl, 3));
    BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC));
    BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC));
#endif

    return BK_OK;
}

static bk_err_t dvp_camera_init(dvp_driver_handle_t *handle)
{
    handle->sensor = bk_dvp_detect();
    if (handle->sensor == NULL)
    {
        return BK_FAIL;
    }

#ifdef CONFIG_DVP_POWER_GPIO_CTRL
    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, CONFIG_DVP_CTRL_POWER_GPIO_ID, GPIO_OUTPUT_STATE_HIGH);
#endif

    /* set current used camera config */
    BK_RETURN_ON_ERR(dvp_camera_init_device(handle));

    return BK_OK;
}

static bk_err_t dvp_camera_deinit(dvp_driver_handle_t *handle)
{
    // step 1: deinit dvp gpio, data cannot transfer
    // bk_video_gpio_deinit(DVP_GPIO_ALL);

    // step 2: deinit i2c
    bk_i2c_deinit(CONFIG_DVP_CAMERA_I2C_ID);

    // step 3: deinit hardware
    if (handle && handle->sensor)
    {
        bk_yuv_buf_deinit();
        bk_h264_encode_disable();
        bk_h264_deinit();
        bk_jpeg_enc_deinit();
    }

    bk_video_dvp_mclk_disable();

#ifdef CONFIG_DVP_POWER_GPIO_CTRL
    // step 4: power off
    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, CONFIG_DVP_CTRL_POWER_GPIO_ID, GPIO_OUTPUT_STATE_LOW);
#endif

    if (handle == NULL)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        return BK_OK;
    }

    uint16_t format = handle->config.img_format;

    if (format != IMAGE_YUV)
    {
        bk_dma_stop(handle->dma_channel);
        bk_dma_deinit(handle->dma_channel);

        if (format & IMAGE_H264)
        {
            bk_dma_free(DMA_DEV_H264, handle->dma_channel);

            if (handle->encode_frame)
            {
                handle->callback.frame_free(IMAGE_H264, handle->enc_stream, handle->encode_frame);
                handle->encode_frame = NULL;
            }
        }

        if (format & IMAGE_MJPEG)
        {
            bk_dma_free(DMA_DEV_JPEG, handle->dma_channel);
            if (handle->encode_frame)
            {
                handle->callback.frame_free(IMAGE_MJPEG, handle->enc_stream, handle->encode_frame);
                handle->encode_frame = NULL;
            }

        }

        if ((format & IMAGE_YUV) && handle->yuv_config)
        {
            bk_dma_stop(handle->yuv_config->dma_collect_yuv);
            bk_dma_deinit(handle->yuv_config->dma_collect_yuv);
            bk_dma_free(DMA_DEV_DTCM, handle->yuv_config->dma_collect_yuv);
            os_free(handle->yuv_config);
            handle->yuv_config = NULL;
        }

        if (handle->enc_stream)
        {
            handle->callback.frame_clear(handle->enc_stream);// h264/jpeg list clear
            handle->callback.frame_deinit(handle->enc_stream);// h264/jpeg list delect
            handle->enc_stream = NULL;
        }
    }

    if (handle->yuv_frame)
    {
        handle->callback.frame_free(IMAGE_YUV, handle->yuv_stream, handle->yuv_frame);
        handle->yuv_frame = NULL;
    }

    if (handle->yuv_stream)
    {
        handle->callback.frame_clear(handle->yuv_stream);// yuv list clear
        handle->callback.frame_deinit(handle->yuv_stream);// yuv list delect
        handle->yuv_stream = NULL;
    }

    if (handle->sem)
    {
        rtos_deinit_semaphore(&handle->sem);
        handle->sem = NULL;
    }

#if (MEDIA_DEBUG_TIMER_ENABLE)
    if (handle->timer.handle)
    {
        rtos_stop_timer(&handle->timer);
        rtos_deinit_timer(&handle->timer);
    }
#endif

    // step 9: free enode buffer
#ifndef CONFIG_ENCODE_BUF_NOT_FREE
    if (dvp_camera_encode)
    {
#if !CONFIG_BT_REUSE_MEDIA_MEMORY
        os_free(dvp_camera_encode);
#endif
        dvp_camera_encode = NULL;
    }
#endif

    handle->dvp_state = MASTER_TURN_OFF;

    os_free(handle);

    s_dvp_camera_handle = NULL;

    LOGD("%s complete!\r\n", __func__);

    return BK_OK;
}

static void dvp_camera_reset_hardware_modules_handler(dvp_driver_handle_t *handle)
{
    dvp_config_t *config = &handle->config;

    if (config->img_format & IMAGE_MJPEG)
    {
        bk_jpeg_enc_soft_reset();
    }

    if (config->img_format & IMAGE_H264)
    {
        bk_h264_config_reset();
        bk_h264_encode_enable();
    }

    bk_yuv_buf_soft_reset();

    if (handle->dma_channel < DMA_ID_MAX)
    {
        bk_dma_stop(handle->dma_channel);
        if (handle->encode_frame)
        {
            handle->encode_frame->length = 0;
        }
        bk_dma_start(handle->dma_channel);
    }
}

static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
    DVP_PPI_ERROR_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        DVP_PPI_ERROR_OUT();
        return;
    }

    if (!handle->error)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        handle->error = true;
    }

    DVP_PPI_ERROR_OUT();
}

static void yuv_sm0_line_done(yuv_buf_unit_t id, void *param)
{
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        return;
    }

    encode_yuv_config_t *yuv_config = handle->yuv_config;
    if ((yuv_config->yuv_data_offset + yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
    {
        yuv_config->yuv_data_offset = 0;
    }

    BK_WHILE(bk_dma_get_enable_status(yuv_config->dma_collect_yuv));
    bk_dma_stop(yuv_config->dma_collect_yuv);
    bk_dma_set_src_start_addr(yuv_config->dma_collect_yuv,
                              (uint32_t)yuv_config->yuv_em_addr);
    bk_dma_set_dest_start_addr(yuv_config->dma_collect_yuv,
                               (uint32_t)(handle->yuv_frame->frame + yuv_config->yuv_data_offset));
    bk_dma_start(yuv_config->dma_collect_yuv);
    yuv_config->yuv_data_offset += yuv_config->yuv_pingpong_length;
}

static void yuv_sm1_line_done(yuv_buf_unit_t id, void *param)
{
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        return;
    }

    encode_yuv_config_t *yuv_config = handle->yuv_config;
    if ((yuv_config->yuv_data_offset + yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
    {
        yuv_config->yuv_data_offset = 0;
    }

    BK_WHILE(bk_dma_get_enable_status(yuv_config->dma_collect_yuv));
    bk_dma_stop(yuv_config->dma_collect_yuv);
    bk_dma_set_src_start_addr(yuv_config->dma_collect_yuv,
                              (uint32_t)yuv_config->yuv_em_addr + yuv_config->yuv_pingpong_length);
    bk_dma_set_dest_start_addr(yuv_config->dma_collect_yuv,
                               (uint32_t)(handle->yuv_frame->frame + yuv_config->yuv_data_offset));
    bk_dma_start(yuv_config->dma_collect_yuv);
    yuv_config->yuv_data_offset += yuv_config->yuv_pingpong_length;
}

static void dvp_camera_vsync_negedge_handler(yuv_buf_unit_t id, void *param)
{
    DVP_JPEG_VSYNC_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state == MASTER_TURNING_OFF)
    {
        bk_yuv_buf_stop(YUV_MODE);
        bk_yuv_buf_stop(JPEG_MODE);
        bk_yuv_buf_stop(H264_MODE);

        if (handle->sem != NULL)
        {
            rtos_set_semaphore(&handle->sem);
        }
        DVP_JPEG_VSYNC_OUT();
        return;
    }

    if (handle->error)
    {
        handle->error = false;
        handle->sequence = 0;
        dvp_camera_reset_hardware_modules_handler(handle);
        LOGD("reset OK \r\n");
        DVP_JPEG_VSYNC_OUT();
        return;
    }

    if (handle->regenerate_idr)
    {
        handle->sequence = 0;
        bk_h264_soft_reset();
        handle->regenerate_idr = false;
    }

    DVP_JPEG_VSYNC_OUT();
}

static void dvp_camera_yuv_eof_handler(yuv_buf_unit_t id, void *param)
{
    frame_buffer_t *new_yuv = NULL;
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    DVP_YUV_EOF_ENTRY();

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_YUV_EOF_OUT();
        return;
    }

    if (handle->error || handle->yuv_frame == NULL
        || handle->yuv_frame->frame == NULL)
    {
        LOGE("%s, yuv frame error\r\n", __func__);
    }

    uint32_t size = handle->config.width * handle->config.height * 2;

    handle->yuv_frame->sequence = handle->frame_id++;
    handle->yuv_frame->timestamp = get_current_timestamp();

    new_yuv = handle->callback.frame_malloc(IMAGE_YUV, handle->yuv_stream, size);
    if (new_yuv)
    {
        new_yuv->width = handle->yuv_frame->width;
        new_yuv->height = handle->yuv_frame->height;
        new_yuv->fmt = handle->yuv_frame->fmt;
        new_yuv->length = size;
        handle->callback.frame_complete(IMAGE_YUV, handle->yuv_stream, handle->yuv_frame);
        handle->yuv_frame = new_yuv;
    }
    else
    {
        //frame_buffer_reset(handle->yuv_frame);
        //LOGE("%s malloc frame failed\n", __func__);
    }

    bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

    DVP_YUV_EOF_OUT();
}

static void dvp_camera_jpeg_eof_handler(jpeg_unit_t id, void *param)
{
    DVP_JPEG_EOF_ENTRY();
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;
    frame_buffer_t *frame_buffer = NULL;

    uint32_t real_length = 0, recv_length = 0;

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_JPEG_EOF_OUT();
        return;
    }

    if (handle->error)
    {
        handle->encode_frame->length = 0;
        handle->dma_length = 0;
        bk_dma_stop(handle->dma_channel);
        bk_dma_start(handle->dma_channel);
        DVP_JPEG_EOF_OUT();
        return;
    }

    if (handle->encode_frame == NULL
        || handle->encode_frame->frame == NULL)
    {
        LOGE("handle->encode_frame NULL error\n");
        goto error;
    }


    bk_dma_flush_src_buffer(handle->dma_channel);

    real_length = bk_jpeg_enc_get_frame_size();

    recv_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);

    bk_dma_stop(handle->dma_channel);

    handle->dma_length = handle->dma_length + recv_length - JPEG_CRC_SIZE;

    if (handle->dma_length != real_length)
    {
        LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, handle->dma_length, real_length - handle->dma_length);
    }

    handle->dma_length = 0;
    handle->encode_frame->timestamp = get_current_timestamp();

    for (uint32_t i = real_length; i > real_length - 10; i--)
    {
        if (handle->encode_frame->frame[i - 1] == 0xD9
            && handle->encode_frame->frame[i - 2] == 0xFF)
        {
            real_length = i + 1;
            handle->eof = true;
            break;
        }

        handle->eof = false;
    }

    if (handle->eof)
    {
#if (MEDIA_DEBUG_TIMER_ENABLE)
        handle->latest_kbps += real_length;
        handle->curr_length = real_length;
#endif
        handle->encode_frame->length = real_length;
        handle->encode_frame->sequence = handle->frame_id++;

        frame_buffer = handle->callback.frame_malloc(IMAGE_MJPEG, handle->enc_stream, CONFIG_JPEG_FRAME_SIZE);

        if (frame_buffer == NULL)
        {
            //LOGE("alloc frame error\n");
            //frame_buffer_reset(handle->encode_frame);
            handle->encode_frame->length = 0;
        }
        else
        {
            frame_buffer->width = handle->config.width;
            frame_buffer->height = handle->config.height;
            frame_buffer->fmt = PIXEL_FMT_JPEG;
            handle->callback.frame_complete(IMAGE_MJPEG, handle->enc_stream, handle->encode_frame);
            handle->encode_frame = frame_buffer;
        }
    }
    else
    {
        handle->encode_frame->length = 0;
    }

    handle->eof = false;
    if (handle->encode_frame == NULL
        || handle->encode_frame->frame == NULL)
    {
        LOGE("alloc frame error\n");
        return;
    }

    bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)handle->encode_frame->frame, (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
    bk_dma_start(handle->dma_channel);

    if (handle->config.img_format & IMAGE_YUV)
    {
        DVP_YUV_EOF_ENTRY();
        encode_yuv_config_t *yuv_config = handle->yuv_config;
        frame_buffer_t *new_yuv = NULL;
        uint32_t size = handle->config.width * handle->config.height * 2;
        yuv_config->yuv_data_offset = 0;
        bk_dma_flush_src_buffer(yuv_config->dma_collect_yuv);
        handle->yuv_frame->sequence = handle->frame_id - 1;
        handle->yuv_frame->timestamp = get_current_timestamp();
        LOGV("%s, ppi:%d-%d, length:%d, fmt:%d, seq:%d, %p\r\n", __func__, handle->yuv_frame->width,
            handle->yuv_frame->height, handle->yuv_frame->length,
            handle->yuv_frame->fmt, handle->yuv_frame->sequence, handle->yuv_frame);
        new_yuv = handle->callback.frame_malloc(IMAGE_YUV, handle->enc_stream, size);
        if (new_yuv)
        {
            new_yuv->width = handle->yuv_frame->width;
            new_yuv->height = handle->yuv_frame->height;
            new_yuv->fmt = handle->yuv_frame->fmt;
            new_yuv->length = size;
            handle->callback.frame_complete(IMAGE_YUV, handle->enc_stream, handle->yuv_frame);
            handle->yuv_frame = new_yuv;
        }
        else
        {
            //frame_buffer_reset(handle->yuv_frame);
            //LOGE("%s malloc new yuv frame failed\n", __func__);
        }
        DVP_YUV_EOF_OUT();
    }

    DVP_JPEG_EOF_OUT();

    return;

error:
    bk_dma_stop(handle->dma_channel);
    bk_yuv_buf_stop(JPEG_MODE);
}

static void dvp_camera_h264_eof_handler(h264_unit_t id, void *param)
{
    DVP_H264_EOF_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_H264_EOF_OUT();
        return;
    }

    if (handle->encode_frame == NULL
        || handle->encode_frame->frame == NULL)
    {
        LOGE("handle->encode_frame NULL error\n");
        DVP_H264_EOF_OUT();
        goto error;
    }

    uint32_t real_length = bk_h264_get_encode_count() * 4;
    uint32_t remain_length = 0;
    frame_buffer_t *new_frame = NULL;

    handle->sequence++;

    if (handle->sequence > H264_GOP_FRAME_CNT)
    {
        handle->sequence = 1;
    }


    if (handle->sequence == 1)
    {
        handle->i_frame = 1;
    }
    else
    {
        handle->i_frame = 0;
    }

#if (CONFIG_H264_GOP_START_IDR_FRAME)
    if (handle->sequence == H264_GOP_FRAME_CNT)
    {
        bk_h264_soft_reset();
        handle->sequence = 0;
    }
#endif

    if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
    {
        LOGE("%s size over h264 buffer range, %d\r\n", __func__, real_length);
        handle->error = true;
    }

    bk_dma_flush_src_buffer(handle->dma_channel);

    remain_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);

    bk_dma_stop(handle->dma_channel);

    handle->dma_length += remain_length;

    if (handle->dma_length != real_length)
    {
        uint32_t left_length = real_length - handle->dma_length;
        LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, handle->dma_length, left_length);
        if (left_length != FRAME_BUFFER_CACHE)
        {
            handle->error = true;
        }
    }

    handle->dma_length = 0;
    handle->encode_frame->sequence = handle->frame_id++;

    if (handle->error || handle->regenerate_idr)
    {
        handle->encode_frame->length = 0;
        handle->sequence = 0;
        if (handle->regenerate_idr)
        {
            bk_h264_soft_reset();
            handle->regenerate_idr = false;
        }
        DVP_H264_EOF_OUT();
        goto out;
    }

#if (MEDIA_DEBUG_TIMER_ENABLE)
    handle->latest_kbps += real_length;
    handle->curr_length = real_length;
#endif


    handle->encode_frame->length = real_length;
    handle->encode_frame->timestamp = get_current_timestamp();

    if (handle->i_frame)
    {
        handle->encode_frame->h264_type |= 1 << H264_NAL_I_FRAME;
#if (CONFIG_H264_GOP_START_IDR_FRAME)
        handle->encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS) | (1 << H264_NAL_IDR_SLICE);
#endif
    }
    else
    {
        handle->encode_frame->h264_type |= 1 << H264_NAL_P_FRAME;
    }

    handle->encode_frame->timestamp = get_current_timestamp();

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
    handle->encode_frame->crc = hnd_crc8(handle->encode_frame->frame, handle->encode_frame->length, 0xFF);
    handle->encode_frame->length += H264_SELF_DEFINE_SEI_SIZE;
    os_memcpy(&handle->sei[23], (uint8_t *)handle->encode_frame, sizeof(frame_buffer_t));
    os_memcpy(&handle->encode_frame->frame[handle->encode_frame->length - H264_SELF_DEFINE_SEI_SIZE], &handle->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

    new_frame = handle->callback.frame_malloc(IMAGE_H264, handle->enc_stream, CONFIG_H264_FRAME_SIZE);

    if (new_frame)
    {
        new_frame->width = handle->config.width;
        new_frame->height = handle->config.height;
        new_frame->fmt = PIXEL_FMT_H264;
        handle->callback.frame_complete(IMAGE_H264, handle->enc_stream, handle->encode_frame);
        handle->encode_frame = new_frame;
    }
    else
    {
        bk_h264_soft_reset();
        handle->encode_frame->length = 0;
        handle->encode_frame->h264_type = 0;
        handle->sequence = 0;
    }

out:
    bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)handle->encode_frame->frame, (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
    bk_dma_start(handle->dma_channel);

    if (handle->config.img_format & IMAGE_YUV)
    {
        DVP_YUV_EOF_ENTRY();
        frame_buffer_t *new_yuv = NULL;
        uint32_t size = handle->config.width * handle->config.height * 2;
        handle->yuv_config->yuv_data_offset = 0;
        bk_dma_flush_src_buffer(handle->yuv_config->dma_collect_yuv);
        handle->yuv_frame->sequence =  handle->frame_id - 1;
        handle->yuv_frame->timestamp = get_current_timestamp();
        LOGV("%s, ppi:%d-%d, length:%d, fmt:%d, seq:%d, %p\r\n", __func__, handle->yuv_frame->width,
            handle->yuv_frame->height, handle->yuv_frame->length,
            handle->yuv_frame->fmt, handle->yuv_frame->sequence, handle->yuv_frame);
        if (handle->error == false)
        {
            new_yuv = handle->callback.frame_malloc(IMAGE_YUV, handle->yuv_stream, size);
            if (new_yuv)
            {
                new_yuv->width = handle->yuv_frame->width;
                new_yuv->height = handle->yuv_frame->height;
                new_yuv->fmt = handle->yuv_frame->fmt;
                new_yuv->length = size;
                handle->callback.frame_complete(IMAGE_YUV, handle->yuv_stream, handle->yuv_frame);
                handle->yuv_frame = new_yuv;
            }
            else
            {
                //LOGE("%s malloc new yuv frame failed\n", __func__);
            }
        }
        DVP_YUV_EOF_OUT();
    }

    DVP_H264_EOF_OUT();

    return;

error:

    bk_dma_stop(handle->dma_channel);
    bk_yuv_buf_stop(H264_MODE);
    DVP_H264_EOF_OUT();
}

static bk_err_t dvp_camera_jpeg_config_init(dvp_driver_handle_t *handle)
{
    int ret = BK_OK;
    jpeg_config_t jpeg_config = {0};
    dvp_config_t *config = &handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;

    jpeg_config.x_pixel = config->width / 8;
    jpeg_config.y_pixel = config->height / 8;
    jpeg_config.vsync = sensor->vsync;
    jpeg_config.hsync = sensor->hsync;
    jpeg_config.clk = sensor->clk;
    jpeg_config.mode = JPEG_MODE;

    switch (sensor->fmt)
    {
        case PIXEL_FMT_YUYV:
            jpeg_config.sensor_fmt = YUV_FORMAT_YUYV;
            break;

        case PIXEL_FMT_UYVY:
            jpeg_config.sensor_fmt = YUV_FORMAT_UYVY;
            break;

        case PIXEL_FMT_YYUV:
            jpeg_config.sensor_fmt = YUV_FORMAT_YYUV;
            break;

        case PIXEL_FMT_UVYY:
            jpeg_config.sensor_fmt = YUV_FORMAT_UVYY;
            break;

        default:
            LOGE("JPEG MODULE not support this sensor input format\r\n");
            ret = kParamErr;
            return ret;
    }

    ret = bk_jpeg_enc_init(&jpeg_config);
    if (ret != BK_OK)
    {
        LOGE("jpeg init error\n");
    }

    return ret;
}

bk_err_t dvp_camera_yuv_buf_config_init(dvp_driver_handle_t *handle)
{
    int ret = BK_OK;
    dvp_config_t *config = &handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;
    yuv_buf_config_t yuv_mode_config = {0};

    if (config->img_format == IMAGE_YUV)
    {
        yuv_mode_config.work_mode = YUV_MODE;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        yuv_mode_config.work_mode = JPEG_MODE;
    }
    else if (config->img_format & IMAGE_H264)
    {
        yuv_mode_config.work_mode = H264_MODE;
    }

    yuv_mode_config.mclk_div = YUV_MCLK_DIV_3;


    yuv_mode_config.x_pixel = config->width / 8;
    yuv_mode_config.y_pixel = config->height / 8;
    yuv_mode_config.yuv_mode_cfg.vsync = sensor->vsync;
    yuv_mode_config.yuv_mode_cfg.hsync = sensor->hsync;

    LOGD("%s, %d-%d, fmt:%X\r\n", __func__, config->width, config->height, config->img_format);

    switch (sensor->fmt)
    {
        case PIXEL_FMT_YUYV:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;
            break;

        case PIXEL_FMT_UYVY:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UYVY;
            break;

        case PIXEL_FMT_YYUV:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YYUV;
            break;

        case PIXEL_FMT_UVYY:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UVYY;
            break;

        default:
            LOGE("YUV_BUF MODULE not support this sensor input format\r\n");
            ret = BK_ERR_PARAM;
    }

    if (ret != BK_OK)
    {
        return ret;
    }

    if (config->img_format & IMAGE_H264 || config->img_format & IMAGE_MJPEG)
    {
        if (dvp_camera_encode == NULL)
        {
            dvp_camera_encode = media_bt_share_buffer;
            if (dvp_camera_encode == NULL)
            {
                if (config->img_format & IMAGE_H264)
                {
                    dvp_camera_encode = (uint8_t *)os_malloc(config->width * 32 * 2);
                }
                else
                {
                    dvp_camera_encode = (uint8_t *)os_malloc(config->width * 16 * 2);
                }
            }

            if (dvp_camera_encode == NULL)
            {
                return BK_ERR_NO_MEM;
            }
        }

        LOGD("%s, encode_buf:%p\r\n", __func__, dvp_camera_encode);

        yuv_mode_config.base_addr = dvp_camera_encode;
    }

    ret = bk_yuv_buf_init(&yuv_mode_config);
    if (ret != BK_OK)
    {
        LOGE("yuv_buf yuv mode init error\n");
    }

    return ret;
}

static bk_err_t dvp_camera_yuv_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    uint32_t size = 0;
    dvp_config_t *config = &handle->config;

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    size = config->width * config->height * 2;

    handle->yuv_frame = handle->callback.frame_malloc(IMAGE_YUV, handle->yuv_stream, size);

    if (handle->yuv_frame == NULL)
    {
        LOGE("malloc frame fail \r\n");
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    handle->yuv_frame->width = config->width;
    handle->yuv_frame->height = config->height;
    handle->yuv_frame->fmt = handle->sensor->fmt;
    handle->yuv_frame->length = size;
    bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

    return ret;
}

static bk_err_t dvp_camera_jpeg_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    dvp_config_t *config = &handle->config;

    ret = dvp_camera_dma_config(handle);

    if (ret != BK_OK)
    {
        LOGE("dma init failed\n");
        return ret;
    }

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    ret = dvp_camera_jpeg_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    if (config->img_format & IMAGE_YUV)
    {
        uint32_t size = config->width * config->height * 2;

        handle->yuv_frame = handle->callback.frame_malloc(IMAGE_YUV, handle->yuv_stream, size);

        if (handle->yuv_frame == NULL)
        {
            LOGE("yuv_frame malloc failed!\r\n");
            ret = BK_ERR_NO_MEM;
            return ret;
        }

        handle->yuv_frame->width = config->width;
        handle->yuv_frame->height = config->height;
        handle->yuv_frame->fmt = handle->sensor->fmt;
        handle->yuv_frame->length = size;

        if (handle->yuv_config == NULL)
        {
            handle->yuv_config = (encode_yuv_config_t *)os_malloc(sizeof(encode_yuv_config_t));
            if (handle->yuv_config == NULL)
            {
                LOGE("yuv_config malloc error! \r\n");
                ret = BK_ERR_NO_MEM;
                return ret;
            }
        }

        handle->yuv_config->yuv_em_addr = bk_yuv_buf_get_em_base_addr();
        LOGV("yuv buffer base addr:%08x\r\n", handle->yuv_config->yuv_em_addr);
        handle->yuv_config->dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
        handle->yuv_config->yuv_pingpong_length = config->width * 8 * 2;
        handle->yuv_config->yuv_data_offset = 0;
        LOGV("dma_collect_yuv id is %d \r\n", handle->yuv_config->dma_collect_yuv);

        encode_yuv_dma_cpy(handle->yuv_frame->frame,
                           (uint32_t *)handle->yuv_config->yuv_em_addr,
                           handle->yuv_config->yuv_pingpong_length,
                           handle->yuv_config->dma_collect_yuv);
    }

    return ret;
}

static bk_err_t dvp_camera_h264_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    dvp_config_t *config = &handle->config;

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    ret = bk_h264_init(config->width, config->height);
    if (ret != BK_OK)
    {
        return ret;
    }

    ret = dvp_camera_dma_config(handle);
    if (ret != BK_OK)
    {
        LOGE("dma init failed\n");
        return ret;
    }

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
    os_memset(&handle->sei[0], 0xFF, H264_SELF_DEFINE_SEI_SIZE);

    h264_encode_sei_init(&handle->sei[0]);
#endif

    if (config->img_format & IMAGE_YUV)
    {
        uint32_t size = config->width * config->height * 2;

        handle->yuv_frame = handle->callback.frame_malloc(IMAGE_YUV, handle->yuv_stream, size);

        if (handle->yuv_frame == NULL)
        {
            LOGE("yuv_frame malloc failed!\r\n");
            ret = BK_ERR_NO_MEM;
            return ret;
        }

        handle->yuv_frame->width = config->width;
        handle->yuv_frame->height = config->height;
        handle->yuv_frame->fmt = handle->sensor->fmt;
        handle->yuv_frame->length = size;

        if (handle->yuv_config == NULL)
        {
            handle->yuv_config = (encode_yuv_config_t *)os_malloc(sizeof(encode_yuv_config_t));
            if (handle->yuv_config == NULL)
            {
                LOGE("encode_lcd_config malloc error! \r\n");
                ret = BK_ERR_NO_MEM;
                return ret;
            }
        }

        handle->yuv_config->yuv_em_addr = bk_yuv_buf_get_em_base_addr();
        LOGV("yuv buffer base addr:%08x\r\n", handle->yuv_config->yuv_em_addr);
        handle->yuv_config->dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
        handle->yuv_config->yuv_pingpong_length = config->width * 16 * 2;
        handle->yuv_config->yuv_data_offset = 0;
        LOGV("dma_collect_yuv id is %d \r\n", handle->yuv_config->dma_collect_yuv);

        encode_yuv_dma_cpy(handle->yuv_frame->frame,
                           (uint32_t *)handle->yuv_config->yuv_em_addr,
                           handle->yuv_config->yuv_pingpong_length,
                           handle->yuv_config->dma_collect_yuv);
    }

    return ret;
}

static void dvp_camera_register_isr_function(dvp_driver_handle_t *handle)
{
    uint16_t format = handle->config.img_format;

    LOGD("%s, %d, fmt:%d\r\n", __func__, __LINE__, format);
    switch (format)
    {
        case IMAGE_YUV:
            bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, dvp_camera_yuv_eof_handler, (void *)handle);
            break;

        case IMAGE_MJPEG:
        case (IMAGE_MJPEG | IMAGE_YUV):
            bk_jpeg_enc_register_isr(JPEG_EOF, dvp_camera_jpeg_eof_handler, (void *)handle);
            bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, dvp_camera_sensor_ppi_err_handler, (void *)handle);
            break;

        case IMAGE_H264:
        case (IMAGE_H264 | IMAGE_YUV):
            bk_h264_register_isr(H264_FINAL_OUT, dvp_camera_h264_eof_handler, (void *)handle);
            break;

        default:
            break;
    }

    if ((format & IMAGE_YUV) && (format != IMAGE_YUV))
    {
        bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, yuv_sm0_line_done, (void *)handle);
        bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, yuv_sm1_line_done, (void *)handle);
    }

    bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, dvp_camera_vsync_negedge_handler, (void *)handle);

    bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_FULL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, dvp_camera_sensor_ppi_err_handler, (void *)handle);
}

const dvp_sensor_config_t *bk_dvp_detect(void)
{
    i2c_config_t i2c_config = {0};
    const dvp_sensor_config_t *sensor = NULL;

#ifdef CONFIG_DVP_POWER_GPIO_CTRL
    // step 1: power on video modules
    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, CONFIG_DVP_CTRL_POWER_GPIO_ID, GPIO_OUTPUT_STATE_HIGH);
#endif

    // step 2: map gpio as MCLK, PCLK for i2c communicate with dvp
    // bk_video_gpio_init(DVP_GPIO_ALL);

    // step 3: enable mclk for i2c communicate with dvp
    bk_video_dvp_mclk_enable(YUV_MODE);

    // step 4: init i2c
    i2c_config.baud_rate = I2C_BAUD_RATE_100KHZ;
    i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
    bk_i2c_init(CONFIG_DVP_CAMERA_I2C_ID, &i2c_config);

    // step 5: detect sensor
    sensor = bk_dvp_get_sensor_auto_detect();

    if (sensor == NULL)
    {
        LOGE("%s no dvp camera found\n", __func__);
    }
    else
    {
        LOGD("auto detect success, dvp camera name:%s\r\n", sensor->name);
    }

#ifdef CONFIG_DVP_POWER_GPIO_CTRL
    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, CONFIG_DVP_CTRL_POWER_GPIO_ID, GPIO_OUTPUT_STATE_LOW);
#endif

    return sensor;
}

bk_err_t bk_dvp_init(camera_handle_t *handle, dvp_config_t *cfg, bk_dvp_callback_t *cb)
{
    bk_err_t ret = BK_FAIL;
    camera_config_t *output_handle = NULL;

    LOGD("%s\r\n", __func__);

    if (s_dvp_camera_handle == NULL)
    {
        output_handle = (camera_config_t *)os_malloc(sizeof(camera_config_t));
        if (output_handle == NULL)
        {
            LOGW("%s, output_handle malloc fail\n", __func__);
            return ret;
        }

        s_dvp_camera_handle = (dvp_driver_handle_t *)os_malloc(sizeof(dvp_driver_handle_t));
        if (s_dvp_camera_handle == NULL)
        {
            LOGE("%s, s_dvp_camera_handle malloc fail....\r\n", __func__);
            return ret;
        }

        os_memset(s_dvp_camera_handle, 0, sizeof(dvp_driver_handle_t));

        LOGE("%s, s_dvp_camera_handle malloc %p....\r\n", __func__, s_dvp_camera_handle);

        ret = rtos_init_semaphore(&s_dvp_camera_handle->sem, 1);
        if (ret != BK_OK)
        {
            LOGE("%s, s_dvp_camera_handle->sem malloc fail....\r\n", __func__);
            goto error;
        }

#if (MEDIA_DEBUG_TIMER_ENABLE)
        ret = rtos_init_timer(&s_dvp_camera_handle->timer, MEDIA_DEBUG_TIMER_INTERVAL * 1000,
            dvp_camera_timer_handle, s_dvp_camera_handle);

        if (ret != BK_OK)
        {
            LOGE("%s, s_dvp_camera_handle->timer fail....\r\n", __func__);
            goto error;
        }

        rtos_start_timer(&s_dvp_camera_handle->timer);
#endif

        os_memcpy(&s_dvp_camera_handle->config, cfg, sizeof(dvp_config_t));
        os_memcpy(&s_dvp_camera_handle->callback, cb, sizeof(bk_dvp_callback_t));
    }
    else
    {
        if (s_dvp_camera_handle->dvp_state == MASTER_TURN_ON)
        {
            LOGE("%s, dvp already opened...\r\n", __func__);
            return ret;
        }
    }

    if (s_dvp_camera_handle->dvp_state != MASTER_TURN_OFF)
    {
        LOGE("%s, dvp state error:%d...\r\n", __func__, s_dvp_camera_handle->dvp_state);
        return ret;
    }

    s_dvp_camera_handle->dvp_state = MASTER_TURNING_ON;

    DVP_DIAG_DEBUG_INIT();

    // step 1: for camera sensor, init other device
    ret = dvp_camera_init(s_dvp_camera_handle);

    if (ret != BK_OK)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        goto error;
    }

    // step 2: init stream list
    if (cfg->img_format & IMAGE_MJPEG)
    {
        s_dvp_camera_handle->enc_stream = (void *)cb->frame_init(cfg->id, DVP_CAMERA, IMAGE_MJPEG);
    }

    if (cfg->img_format & IMAGE_H264)
    {
        s_dvp_camera_handle->enc_stream = (void *)cb->frame_init(cfg->id, DVP_CAMERA, IMAGE_H264);
    }

    if ((cfg->img_format & (IMAGE_MJPEG | IMAGE_H264))
            && s_dvp_camera_handle->enc_stream == NULL)
    {
        LOGE("%s, %d stream list init fail\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto error;
    }

    // step 3: config video hardware modules, include dma
    switch (cfg->img_format)
    {
        case IMAGE_YUV:
            ret = dvp_camera_yuv_mode(s_dvp_camera_handle);
            break;

        case IMAGE_MJPEG:
        case (IMAGE_MJPEG | IMAGE_YUV):
            ret = dvp_camera_jpeg_mode(s_dvp_camera_handle);
            break;

        case IMAGE_H264:
        case (IMAGE_H264 | IMAGE_YUV):
            ret = dvp_camera_h264_mode(s_dvp_camera_handle);
            break;

        default:
            ret = BK_FAIL;
    }

    if (ret != BK_OK)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        goto error;
    }

    output_handle->type = DVP_CAMERA;
    output_handle->fps = cfg->fps;
    output_handle->id = cfg->id;
    output_handle->image_format = cfg->img_format;
    output_handle->width = cfg->width;
    output_handle->height = cfg->height;
    output_handle->arg = (void *)s_dvp_camera_handle;
    *handle = (camera_handle_t)output_handle;

    // step 4: maybe need register isr_func
    dvp_camera_register_isr_function(s_dvp_camera_handle);

    // step 5: start hardware function in different mode
    bk_video_set_mclk(s_dvp_camera_handle->sensor->clk);
    if (cfg->img_format == IMAGE_YUV)
    {
        bk_yuv_buf_start(YUV_MODE);
    }
    else if (cfg->img_format & IMAGE_MJPEG)
    {
        bk_yuv_buf_start(JPEG_MODE);
    }
    else if (cfg->img_format & IMAGE_H264)
    {
        bk_yuv_buf_start(H264_MODE);
        bk_h264_encode_enable();
    }

    s_dvp_camera_handle->dvp_state = MASTER_TURN_ON;

    // step 6: init dvp camera sensor register
    s_dvp_camera_handle->sensor->init();
    s_dvp_camera_handle->sensor->set_ppi((cfg->width << 16) | cfg->height);
    s_dvp_camera_handle->sensor->set_fps(cfg->fps);

    LOGD("dvp open success %d X %d, %d, %X\n", cfg->width, cfg->height, cfg->fps, cfg->img_format);

    return ret;

error:

    if (output_handle)
    {
        os_free(output_handle);
    }

    dvp_camera_deinit(s_dvp_camera_handle);

    return ret;
}

bk_err_t bk_dvp_deinit(camera_handle_t *handle)
{
    if (*handle == NULL)
    {
        LOGW("%s, already closed...\n", __func__);
        return BK_OK;
    }

    camera_config_t *config = (camera_config_t *)*handle;

    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)config->arg;

    if (dvp_handle->dvp_state == MASTER_TURN_OFF)
    {
        LOGD("%s, dvp have been closed!\r\n", __func__);
        goto out;
    }

    dvp_handle->dvp_state = MASTER_TURNING_OFF;

    if (BK_OK != rtos_get_semaphore(&dvp_handle->sem, 500))
    {
        LOGW("Not wait yuv vsync negedge!\r\n");
    }

out:

    dvp_camera_deinit(dvp_handle);

    s_dvp_camera_handle = NULL;
    os_free(config);
    *handle = NULL;

    return BK_OK;
}

bk_err_t bk_dvp_h264_idr_reset(void)
{
    dvp_driver_handle_t *handle = s_dvp_camera_handle;

    if (handle == NULL)
    {
        LOGW("%s, not open...\n", __func__);
        return BK_FAIL;
    }

    if (handle->config.img_format & IMAGE_H264)
    {
        handle->regenerate_idr = true;
        return BK_OK;
    }

    LOGW("%s, not enable h264 func...\n", __func__);
    return BK_FAIL;
}
