#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "driver/media_types.h"
#include "driver/drv_tp.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "page_load_ctrol.h"
#endif
#include "lcd_display_service.h"
#include "media_service.h"


#define TAG "86box"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

const lcd_open_t lcd_open =
{
    .device_ppi = PPI_480X480,
    .device_name = "st7701s",
};

bk_err_t lvgl_app_86box_init(void)
{
    lv_vnd_config_t lv_vnd_config = {0};

#ifdef CONFIG_LVGL_USE_PSRAM
    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
    lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
    lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi) / 10;
    lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config.draw_buf_2_2 = NULL;
    lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
    lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open.device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open.device_ppi);
    lv_vnd_config.rotation = ROTATE_NONE;

    lv_vendor_init(&lv_vnd_config);

    lcd_display_open((lcd_open_t *)&lcd_open);

#if (CONFIG_TP)
    drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
    hor_page_load_main();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    return BK_OK;
}

bk_err_t lvgl_app_86box_deinit(void)
{
    lcd_display_close();

#if (CONFIG_TP)
    drv_tp_close();
#endif

    lv_vendor_stop();

    lv_vendor_deinit();

    return BK_OK;
}

#define CMDS_COUNT  (sizeof(s_86box_commands) / sizeof(struct cli_command))

void cli_86box_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);

    lvgl_app_86box_deinit();
}

static const struct cli_command s_86box_commands[] =
{
    {"86box", "86box", cli_86box_cmd},
};

int cli_86box_init(void)
{
    return cli_register_commands(s_86box_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    lvgl_app_86box_init();

    return 0;
}
