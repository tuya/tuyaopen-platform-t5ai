#include "tuya_cloud_types.h"
#include "tal_thread.h"
#include "tal_display_service.h"
#include "tal_queue.h"
#include "tal_dvp.h"
#include "tkl_fs.h"
#include "tkl_dma2d.h"
#include "tal_semaphore.h"
#include "tal_mutex.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define CAMERA_WIDTH            (480)
#define CAMERA_HEIGHT           (480)
#define RGB565_PIXEL_SIZE       (2)

#define TUYA_MEDIA_MOUNT_POINT          "/sdcard"
#define TUYA_H264_RECORD_FILE           "h264_record.h264"
#define TUYA_MAX_FILE_SIZE_LIMITED      (4 * 1024 * 1024 * 1024)
#define TUYA_H264_DATA_LEN_PER_FRAME    (256 * 1024) 
#define TUYA_H264_FILE_SIZE_LIMITED     (TUYA_MAX_FILE_SIZE_LIMITED - TUYA_H264_DATA_LEN_PER_FRAME)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    H264_TEST_DRIVER_TURN_OFF,
    H264_TEST_DRIVER_TURNING_OFF,
    H264_TEST_DRIVER_TURNING_ON,
    H264_TEST_DRIVER_TURN_ON,
} H264_TEST_DRIVER_STATUS;

/***********************************************************
***********************variable define**********************
***********************************************************/
extern ty_display_device_s lcd_rgb_ili9488_device;
extern TUYA_DVP_SENSOR_CFG_T dvp_sensor_gc2145_cfg;

static TUYA_DVP_DEVICE_T *cur_dvp_device = NULL;
static TY_DISPLAY_HANDLE cur_lcd_device = NULL;
static uint16_t *lcd_buf = NULL;
static TKL_DMA2D_FRAME_INFO_T in_frame = {0};
static TKL_DMA2D_FRAME_INFO_T out_frame = {0};
static SEM_HANDLE dma2d_sem = NULL;

static void dvp_frame_handle(TUYA_DVP_FRAME_MANAGE_T *output_frame);

static ty_frame_buffer_t lcd_frame = {
    .type = TYPE_PSRAM, 
    .fmt = TY_PIXEL_FMT_RGB565, 
    .width =CAMERA_WIDTH, .height = CAMERA_HEIGHT, 
    .free_cb = NULL, 
    .len = CAMERA_WIDTH * CAMERA_HEIGHT * RGB565_PIXEL_SIZE
};

static ty_display_cfg cfg0 = {
    .rgb_cfg = {
        .spi_clk = TUYA_GPIO_NUM_49,
        .spi_csx = TUYA_GPIO_NUM_48,
        .spi_sda = TUYA_GPIO_NUM_50,

        .power_ctrl = {
        .pin = TUYA_GPIO_NUM_MAX,
        },

        .reset = {
        .pin = TUYA_GPIO_NUM_53,
        },

        .bl = {
        .pin = TUYA_GPIO_NUM_9,
        .active_level = TUYA_GPIO_LEVEL_HIGH
        },
    }
};

static TUYA_DVP_USR_CFG_T dvp_gc2145_usr_cfg = {
    .dvp_cfg = {
        .fps = 20,
        .width = CAMERA_WIDTH,
        .height = CAMERA_HEIGHT,
        .output_mode = TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH,
    },

    .pin_cfg = {
        .dvp_i2c_clk = {
            .pin = TUYA_GPIO_NUM_13,
        },
        .dvp_i2c_sda = {
            .pin = TUYA_GPIO_NUM_15,
        },
        .dvp_rst_ctrl = {
            .pin = TUYA_GPIO_NUM_51,
            .active_level = TUYA_GPIO_LEVEL_LOW,
        },
        .dvp_pwr_ctrl = {
            .pin = TUYA_GPIO_NUM_MAX,
        },
        .dvp_i2c_idx = TUYA_I2C_NUM_1,
    },

    .dvp_frame_handle = dvp_frame_handle,
};

static TUYA_FILE h264_file = NULL;
static uint8_t is_first_frame = true;
static char fp[128] = {'\0'};

static volatile H264_TEST_DRIVER_STATUS h264_test_status = H264_TEST_DRIVER_TURN_OFF;
static MUTEX_HANDLE h264_mutex = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __dma2d_irq_cb(TUYA_DMA2D_IRQ_E type, void *args)
{
    if (args)
    {
        SEM_HANDLE tmp_sem = (SEM_HANDLE *)args;
        tal_semaphore_post(tmp_sem);
    }
}

static void to_lcd_func(TUYA_DVP_FRAME_MANAGE_T *output_frame)
{
    if (!cur_lcd_device)
        return;

    in_frame.type = TUYA_FRAME_FMT_YUV422;
    in_frame.width = CAMERA_WIDTH;
    in_frame.height = CAMERA_HEIGHT;
    in_frame.axis.x_axis = 0;
    in_frame.axis.y_axis = 0;
    in_frame.width_cp = 0;
    in_frame.height_cp = 0;
    in_frame.pbuf = output_frame->data;

    out_frame.type = TUYA_FRAME_FMT_RGB565;
    out_frame.width = CAMERA_WIDTH;
    out_frame.height = CAMERA_HEIGHT;
    out_frame.axis.x_axis = 0;
    out_frame.axis.y_axis = 0;
    out_frame.width_cp = 0;
    out_frame.height_cp = 0;
    out_frame.pbuf = lcd_buf;

    tkl_dma2d_convert(&in_frame, &out_frame);

    tal_semaphore_wait_forever(dma2d_sem);

    lcd_frame.frame =  lcd_buf;

    tal_display_flush(cur_lcd_device, &lcd_frame);
}

void to_sdcard_func(TUYA_DVP_FRAME_MANAGE_T *output_frame)
{
    if (h264_test_status != H264_TEST_DRIVER_TURN_ON)
        return;

    if (is_first_frame && output_frame->is_i_frame == false)
        return;

    is_first_frame = false;
    tal_mutex_lock(h264_mutex);
    tkl_fwrite(output_frame->data, output_frame->data_len, h264_file);
    tkl_fsync((int32_t)h264_file);

    uint32_t size = tkl_ftell(h264_file) & 0xFFFFFFFF;
    if (size > TUYA_H264_FILE_SIZE_LIMITED) {
        tkl_fclose(h264_file);

        memset(fp, 0, 128);
        uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
        snprintf(fp, sizeof(fp), "%s/%u-%s", TUYA_MEDIA_MOUNT_POINT, tick, TUYA_H264_RECORD_FILE);
        tkl_log_output("%s, open new file\r\n", __func__);

        h264_file = tkl_fopen(fp, "w+");
        if (h264_file == NULL) {
            bk_printf("h264 test error, open new failed\r\n");
            tal_mutex_unlock(h264_mutex);
            return;
        }
    }
    tal_mutex_unlock(h264_mutex);
}

static void dvp_frame_handle(TUYA_DVP_FRAME_MANAGE_T *output_frame)
{
    TUYA_FRAME_FMT_E fmt = output_frame->frame_fmt;
    switch (fmt)
    {
    case TUYA_FRAME_FMT_YUV422:
        /* code */
        to_lcd_func(output_frame);
        break;
    case TUYA_FRAME_FMT_H264:
        /* code */
        to_sdcard_func(output_frame);
        break;
    default:
        break;
    }
}

static void __test_driver_dvp_release()
{
    if (cur_dvp_device)
    {
        tal_dvp_deinit(cur_dvp_device);
        cur_dvp_device = NULL;
    }

    if (cur_lcd_device)
    {
        tal_display_close(cur_lcd_device);
        cur_lcd_device = NULL;
    }

    if (dma2d_sem)
    {
        tal_semaphore_release(dma2d_sem);
        dma2d_sem = NULL;
    }

    if (lcd_buf)
    {
        tkl_system_psram_free(lcd_buf);
        lcd_buf = NULL;
    }

    tkl_dma2d_deinit();

    return;
}

void test_driver_dvp_open(void)
{
    if (cur_dvp_device)
        return;

    TUYA_DVP_DEVICE_T *dvp_device = tal_dvp_init(&dvp_sensor_gc2145_cfg, &dvp_gc2145_usr_cfg);
    if (!dvp_device)
        goto failed;

    TY_DISPLAY_HANDLE lcd_device = tal_display_open(&lcd_rgb_ili9488_device, &cfg0);
    if (!lcd_device)
        goto failed;
    tal_display_bl_open(lcd_device);

    lcd_buf = tkl_system_psram_malloc(CAMERA_WIDTH * CAMERA_HEIGHT * RGB565_PIXEL_SIZE);
    if (!lcd_buf)
        goto failed;

    tal_semaphore_create_init(&dma2d_sem, 0, 1);
    if (!dma2d_sem)
        goto failed;

    void *tmp_arg = (void *)(&dma2d_sem);
    TUYA_DMA2D_BASE_CFG_T dma2d_cfg = {.cb = __dma2d_irq_cb, .arg=tmp_arg};
    tkl_dma2d_init(&dma2d_cfg);

    cur_dvp_device = dvp_device;
    cur_lcd_device = lcd_device;

    return;

failed:
    __test_driver_dvp_release();

    return;
}

void test_driver_dvp_close(void)
{
    __test_driver_dvp_release();
}


void test_media_h264_open(void)
{
    if (h264_test_status != H264_TEST_DRIVER_TURN_OFF)
    {
        bk_printf("h264 test is already started\r\n");
        return;
    }
    h264_test_status = H264_TEST_DRIVER_TURNING_ON;

    OPERATE_RET ret = tal_mutex_create_init(&h264_mutex);
    if (ret)
        goto exit;

    ret = tkl_fs_mount(TUYA_MEDIA_MOUNT_POINT, DEV_SDCARD);
    if (ret)
        goto exit;

    uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
    snprintf(fp, sizeof(fp), "%s/%u-%s", TUYA_MEDIA_MOUNT_POINT, tick, TUYA_H264_RECORD_FILE);

    h264_file = tkl_fopen(fp, "w+");
    if (!h264_file)
        goto exit;

    h264_test_status = H264_TEST_DRIVER_TURN_ON;
    is_first_frame = true;

    return;

exit:
    if (h264_mutex)
    {
        tal_mutex_release(h264_mutex);
        h264_mutex = NULL;
    }
    tkl_fs_unmount(TUYA_MEDIA_MOUNT_POINT);
    h264_test_status = H264_TEST_DRIVER_TURN_OFF;

    return;
}

void test_media_h264_close(void)
{
    if (h264_test_status != H264_TEST_DRIVER_TURN_ON)
    {
        bk_printf("h264 test is already stopped\r\n");
        return;
    }
    h264_test_status = H264_TEST_DRIVER_TURNING_OFF;

    tal_mutex_lock(h264_mutex);
    tkl_fclose(h264_file);
    h264_file = NULL;
    tal_mutex_unlock(h264_mutex);

    if (h264_mutex)
    {
        tal_mutex_release(h264_mutex);
        h264_mutex = NULL;
    }
    tkl_fs_unmount(TUYA_MEDIA_MOUNT_POINT);
    h264_test_status = H264_TEST_DRIVER_TURN_OFF;
}


void cli_dvp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc == 1) {
        bk_printf("no parameter\r\n");
        return;
    }
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    if (!strcmp("open", argv[1]))
        test_driver_dvp_open();
    else if (!strcmp("close", argv[1]))
        test_driver_dvp_close();
    else if (!strcmp("start_w", argv[1]))
        test_media_h264_open();
    else if (!strcmp("stop_w", argv[1]))
        test_media_h264_close();
}

