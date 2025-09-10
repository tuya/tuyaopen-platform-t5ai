#include "tuya_cloud_types.h"
#include "tal_thread.h"
#include "tal_display_service.h"
#include "tal_queue.h"
#include "tal_dvp.h"
#include "tkl_fs.h"
#include "tkl_dma2d.h"
#include "tal_semaphore.h"

typedef enum {
    TO_DISPLAY = 0,
    TO_H264,
    TO_JPEG,
    TO_H264_DISPLAY,
    TO_JPEG_DISPLAY,
} TUYA_DVP_USEAGE_T;

VOID_T test_dma2d_irq_cb(TUYA_DMA2D_IRQ_E type, VOID_T *args)
{
    SEM_HANDLE test_sem = (SEM_HANDLE *)args;
    if (test_sem)
        tal_semaphore_post(test_sem);
}

#define SYF_MEDIA_MOUNT_POINT "/sdcard"
#define SYF_H264_RECORD_FILE    "h264_record.h264"
#define SYF_MAX_FILE_SIZE_LIMITED    (4 * 1024 * 1024 * 1024)
#define SYF_H264_DATA_LEN_PER_FRAME  (256 * 1024)  // 实测数据，要根据摄像头设置
#define SYF_H264_FILE_SIZE_LIMITED   (SYF_MAX_FILE_SIZE_LIMITED - SYF_H264_DATA_LEN_PER_FRAME)

THREAD_HANDLE g_thread_handle_base;
THREAD_HANDLE g_thread_handle_encode;

ty_display_cfg cfg0 = 
{
    .rgb_cfg = 
    {
        .spi_clk = TUYA_GPIO_NUM_49,
        .spi_csx = TUYA_GPIO_NUM_48,
        .spi_sda = TUYA_GPIO_NUM_50,

        .power_ctrl = 
        {
            .pin = TUYA_GPIO_NUM_MAX,
        },

        .reset = 
        {
            .pin = TUYA_GPIO_NUM_53,
        },

        .bl = 
        {
            .pin = TUYA_GPIO_NUM_9,
            .active_level = TUYA_GPIO_LEVEL_HIGH
        },
    }
};

extern ty_display_device_s rgb_ili9488_device;

TUYA_DVP_USR_CFG_T dvp_gc2145_usr_cfg = 
{
    .base_cfg = 
    {
        .fps = 20,
        .width = 480,
        .height = 480,
        .output_mode = TUYA_DVP_OUTPUT_H264_YUV422_BOTH,
    },

    .pin_cfg = 
    {
        .dvp_i2c_clk = 
        {
            .pin = TUYA_GPIO_NUM_13,
        },
        .dvp_i2c_sda = 
        {
            .pin = TUYA_GPIO_NUM_15,
        },
        .dvp_rst_ctrl = 
        {
            .pin = TUYA_GPIO_NUM_51,
            .active_level = TUYA_GPIO_LEVEL_HIGH,
        },
        .dvp_pwr_ctrl = 
        {
            .pin = TUYA_GPIO_NUM_MAX,
        },
        .dvp_i2c_idx = TUYA_I2C_NUM_1,
    },
};
extern TUYA_DVP_SENSOR_CFG_T dvp_sensor_gc2145_cfg;

extern void *bk_psram_frame_buffer_malloc(uint8_t type, uint32_t size);

void test_display_task(void *args)
{
    TY_DISPLAY_HANDLE test_lcd_dev = tal_display_open(&rgb_ili9488_device, &cfg0);  
    tal_display_bl_open(test_lcd_dev);

    USHORT_T *dvp_frame_buff1 = tkl_system_psram_malloc(480 * 480 * 2);
    USHORT_T *dvp_frame_buff2 = tkl_system_psram_malloc(480 * 480 * 2);
    USHORT_T *lcd_frame_buff1 = tkl_system_psram_malloc(480 * 480 * 2);
    USHORT_T *lcd_frame_buff2 = tkl_system_psram_malloc(480 * 480 * 2);
    if(NULL == dvp_frame_buff1 || NULL == dvp_frame_buff1 || NULL == lcd_frame_buff1 || NULL == lcd_frame_buff2) 
    {
        TAL_PR_ERR("malloc failed");
        return;
    }

    SEM_HANDLE test_sem;

    tal_semaphore_create_init(&test_sem, 0, 1);
    
    void *tmp_arg = (void *)(&test_sem);
    TUYA_DMA2D_BASE_CFG_T dma2d_cfg = {.cb = test_dma2d_irq_cb, .arg=tmp_arg};
    tkl_dma2d_init(&dma2d_cfg);

    ty_frame_buffer_t ty_frame_buff1 = {.type = 1, .fmt = 0, .width =480, .height = 480, .free_cb = NULL, .len = 480 * 480 * 2, .frame = lcd_frame_buff1};

    TUYA_DVP_FRAME_MANAGE_T obj_frame1 = {.frame_fmt = TUYA_FRAME_FMT_YUV422, .data = dvp_frame_buff1};

    ty_frame_buffer_t ty_frame_buff2 = {.type = 1, .fmt = 0, .width =480, .height = 480, .free_cb = NULL, .len = 480 * 480 * 2, .frame = lcd_frame_buff2};

    TUYA_DVP_FRAME_MANAGE_T obj_frame2 = {.frame_fmt = TUYA_FRAME_FMT_YUV422, .data = dvp_frame_buff2};

    TKL_DMA2D_FRAME_INFO_T in_frame = {.type = TUYA_FRAME_FMT_YUV422, .width = 480, .height = 480};
    TKL_DMA2D_FRAME_INFO_T out_frame = {.type = TUYA_FRAME_FMT_RGB565, .width = 480, .height = 480};

    uint16_t cnt = 0;
    while(1) 
    {
        if (cnt++ % 2 == 0)
        {
            uint8_t ret = tal_dvp_frame_get(&obj_frame1);
            if (ret)
                continue;

            in_frame.pbuf = dvp_frame_buff1;

            out_frame.pbuf = lcd_frame_buff1;

            tkl_dma2d_convert(&in_frame, &out_frame);

            tal_semaphore_wait_forever(test_sem);

            tal_display_flush(test_lcd_dev, &ty_frame_buff1);
        }
        else
        {
            uint8_t ret = tal_dvp_frame_get(&obj_frame2);
            if (ret)
                continue;

            in_frame.pbuf = dvp_frame_buff2;

            out_frame.pbuf = lcd_frame_buff2;

            tkl_dma2d_convert(&in_frame, &out_frame);

            tal_semaphore_wait_forever(test_sem);

            tal_display_flush(test_lcd_dev, &ty_frame_buff2);
        }
    }
}

void test_h264_task(void *args)
{
    USHORT_T *p_frame_buff = tkl_system_psram_malloc(480 * 480);
    if(NULL == p_frame_buff) 
    {
        TAL_PR_ERR("malloc failed");
        return;
    }

    int ret = test_fs_mount(SYF_MEDIA_MOUNT_POINT, DEV_SDCARD);

    char fp[128] = {'\0'};
    uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
    snprintf(fp, sizeof(fp), "%s/%u-%s", SYF_MEDIA_MOUNT_POINT, tick, SYF_H264_RECORD_FILE);

    TUYA_FILE h264_file = tkl_fopen(fp, "w+");
    if (h264_file == NULL) 
    {
        test_fs_unmount(SYF_MEDIA_MOUNT_POINT);
        bk_printf("h264 test error, open %s failed\r\n", SYF_H264_RECORD_FILE);
        return;
    } 

    TUYA_DVP_FRAME_MANAGE_T obj_frame = {.frame_fmt = TUYA_FRAME_FMT_H264, .data = p_frame_buff};
    while(1) 
    {
        uint8_t ret = tal_dvp_frame_get(&obj_frame);
        if (ret)
            continue;
        if (h264_file) 
        {
            tkl_fwrite(obj_frame.data, obj_frame.data_len, h264_file);
            tkl_fsync((int)h264_file);
        }

        uint32_t size = tkl_ftell(h264_file) & 0xFFFFFFFF;
        if (size > SYF_H264_FILE_SIZE_LIMITED) 
        {
            tkl_fclose(h264_file);

            char fp[128] = {'\0'};
            uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
            snprintf(fp, sizeof(fp), "%s/%u-%s", SYF_MEDIA_MOUNT_POINT, tick, SYF_H264_RECORD_FILE);
            bk_printf("%s, open new file\r\n", __func__);

            h264_file = tkl_fopen(fp, "w+");
            if (h264_file == NULL) 
            {
                bk_printf("h264 test error, open new failed\r\n");
                return -1;
            }
        }
    }
}


#define SYF_JEPG_RECORD_FILE    "jpeg_record.mjpeg"
#define SYF_JPEG_DATA_LEN_PER_FRAME  (256 * 1024)  // 实测数据，要根据摄像头设置
#define SYF_JPEG_FILE_SIZE_LIMITED   (SYF_MAX_FILE_SIZE_LIMITED - SYF_JPEG_DATA_LEN_PER_FRAME)

void test_jpeg_task(void *args)
{
    USHORT_T *p_frame_buff2 = tkl_system_psram_malloc(480 * 480 * 2);
    if(NULL == p_frame_buff2) 
    {
        TAL_PR_ERR("malloc failed");
        return;
    }

    bk_printf("frame_buf addr : %p  %p *****..**8\r\n", p_frame_buff2);

    int ret = test_fs_mount(SYF_MEDIA_MOUNT_POINT, DEV_SDCARD);

    char fp[128] = {'\0'};
    uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
    snprintf(fp, sizeof(fp), "%s/%u-%s", SYF_MEDIA_MOUNT_POINT, tick, SYF_JEPG_RECORD_FILE);

    TUYA_FILE h264_file = tkl_fopen(fp, "w+");
    if (h264_file == NULL) 
    {
        test_fs_unmount(SYF_MEDIA_MOUNT_POINT);
        bk_printf("h264 test error, open %s failed\r\n", SYF_JEPG_RECORD_FILE);
        return;
    } 

    TUYA_DVP_FRAME_MANAGE_T obj_frame = {.frame_fmt = TUYA_FRAME_FMT_JPEG, .data = p_frame_buff2};

    UINT32_T cnt = 0;
    while(1) 
    {
        uint8_t ret = tal_dvp_frame_get(&obj_frame);

        if (h264_file) 
        {
            tkl_fwrite(obj_frame.data, obj_frame.data_len, h264_file);
            tkl_fsync((int)h264_file);
        }

        uint32_t size = tkl_ftell(h264_file) & 0xFFFFFFFF;
        if (size > SYF_JPEG_FILE_SIZE_LIMITED) 
        {
            tkl_fclose(h264_file);

            char fp[128] = {'\0'};
            uint32_t tick = tkl_system_get_tick_count() & 0xffffffff;
            snprintf(fp, sizeof(fp), "%s/%u-%s", SYF_MEDIA_MOUNT_POINT, tick, SYF_JEPG_RECORD_FILE);
            bk_printf("%s, open new file\r\n", __func__);

            h264_file = tkl_fopen(fp, "w+");
            if (h264_file == NULL) 
            {
                bk_printf("h264 test error, open new failed\r\n");
                return -1;
            }
        }
    }
}

void test_custom_dvp_open(int usage)
{
    switch (usage)
    {
    case TO_DISPLAY:
        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_YUV422;
        break;
    case TO_H264:
        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_H264;
        break;
    case TO_JPEG:
        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_JPEG;
        break;
    case TO_H264_DISPLAY:
        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_H264_YUV422_BOTH;
        break;
    case TO_JPEG_DISPLAY:
        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH;
        break;
    }

    if (usage == TO_DISPLAY || usage == TO_H264_DISPLAY || usage == TO_JPEG_DISPLAY)
    {
        THREAD_CFG_T thread_cfg = {4096, THREAD_PRIO_2, "dvp_test"};
        tal_thread_create_and_start(&g_thread_handle_base, NULL, NULL, test_display_task, NULL, &thread_cfg);
    }

    if (usage == TO_H264_DISPLAY || usage == TO_H264)
    {
        THREAD_CFG_T thread_cfg1 = {4096, THREAD_PRIO_1, "h264_test"};
        tal_thread_create_and_start(&g_thread_handle_encode, NULL, NULL, test_h264_task, NULL, &thread_cfg1);

        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_H264_YUV422_BOTH;
    }

    if (usage == TO_JPEG_DISPLAY || usage == TO_JPEG)
    {
        THREAD_CFG_T thread_cfg1 = {4096, THREAD_PRIO_1, "jpeg_test"};
        tal_thread_create_and_start(&g_thread_handle_encode, NULL, NULL, test_jpeg_task, NULL, &thread_cfg1);

        dvp_gc2145_usr_cfg.base_cfg.output_mode = TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH;
    }

    TUYA_DVP_DEVICE_T *dvp_device = tal_dvp_init(&dvp_sensor_gc2145_cfg, &dvp_gc2145_usr_cfg);

    return;
}
