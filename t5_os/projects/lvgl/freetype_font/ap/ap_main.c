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
#include "lv_img_utility.h"
#endif
#include "lcd_display_service.h"
#include "media_service.h"
#include "bk_posix.h"
#include <driver/pwr_clk.h>

#define TAG "freetype_font"

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
    .device_ppi = PPI_400X400,
    .device_name = "st77903_h0165y008t",
};

static void lv_example_freetype(void)
{
    lv_vendor_fs_init();

    int fd = open(PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf"), O_RDONLY);
    if (fd < 0) {
        LOGE("file_content open failed\r\n");
        lv_vendor_fs_deinit();
        return;
    }

    int file_len = lv_img_read_filelen(PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf"));
    if (file_len <= 0) {
        LOGE("file len read failed\r\n");
        close(fd);
        lv_vendor_fs_deinit();
        return;
    }

    uint32_t *file_content = psram_malloc(file_len);
    if (file_content == NULL) {
        LOGE("file_content malloc failed\r\n");
        close(fd);
        lv_vendor_fs_deinit();
        return;
    }

    uint32_t read_len = read(fd, file_content, file_len);
    LOGD("read_len = %d \r\n", read_len);
    close(fd);
    lv_vendor_fs_deinit();

    lv_vendor_disp_lock();
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf");
    info.weight = 24;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = file_content;
    info.mem_size = file_len;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
    }

    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, info.font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, "Hello world\nI'm a font created with FreeType");
    lv_obj_center(label);
    lv_vendor_disp_unlock();
}

bk_err_t lvgl_app_freetype_font_init(void)
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

    lv_example_freetype();

    lv_vendor_start();

    return BK_OK;
}

#define CMDS_COUNT  (sizeof(s_freetype_font_commands) / sizeof(struct cli_command))

void cli_freetype_font_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
}

static const struct cli_command s_freetype_font_commands[] =
{
    {"freetype_font", "freetype_font", cli_freetype_font_cmd},
};

int cli_freetype_font_init(void)
{
    return cli_register_commands(s_freetype_font_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN, PM_POWER_MODULE_STATE_ON);

    lvgl_app_freetype_font_init();

    return 0;
}
