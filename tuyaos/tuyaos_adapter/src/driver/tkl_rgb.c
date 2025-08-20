#include "tkl_rgb.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/lcd.h>
#include <driver/flash.h>
#include "gpio_map.h"

#define RGB_IO_FUNCTION_ENABLE(pin, func)   \
	do {                                \
		gpio_dev_unmap(pin);            \
		gpio_dev_map(pin, func);        \
		bk_gpio_enable_output(pin); 	\
		bk_gpio_set_capacity(pin,GPIO_DRIVER_CAPACITY_1);	\
	} while (0)

#define clk_m(a) (a * 1000 * 1000)
TUYA_RGB_ISR_CB ty_rgb_cb = NULL;
static lcd_rgb_t *g_rgb = NULL;
static void __lcd_isr_cb(void)
{
    flash_op_status_t flash_status = FLASH_OP_IDLE;
	flash_status = bk_flash_get_operate_status();

    if((flash_status == FLASH_OP_IDLE) && ty_rgb_cb) {
        ty_rgb_cb(RGB_OUTPUT_FINISH);
    }
}

static OPERATE_RET __rgb_ty_clk_to_bk_clk(UINT32_T clk, lcd_clk_t *outclk)
{
    lcd_clk_t bk_clk = LCD_30M;
    switch(clk) {
        case clk_m(80):
            bk_clk = LCD_80M;
            break;
        case clk_m(64):
            bk_clk = LCD_64M;
            break;
        case clk_m(60):
            bk_clk = LCD_60M;
            break;
        case clk_m(54):
            bk_clk = LCD_54M;
            break;
        case (457*100000):
            bk_clk = LCD_45M;
            break;
        case clk_m(40):
            bk_clk = LCD_40M;
            break;
        case (355*100000):
            bk_clk = LCD_35M;
            break;
        case clk_m(32):
            bk_clk = LCD_32M;
            break;
        case clk_m(30):
            bk_clk = LCD_30M;
            break;
        case (266*100000):
            bk_clk = LCD_26M;
            break;
        case (246*100000):
            bk_clk = LCD_24M;
            break;
        case (2285*10000):
            bk_clk = LCD_22M;
            break;
        case clk_m(20):
            bk_clk = LCD_20M;
            break;
        case (171*100000):
            bk_clk = LCD_17M;
            break;
        case clk_m(15):
            bk_clk = LCD_15M;
            break;
        case clk_m(12):
            bk_clk = LCD_12M;
            break;
           case clk_m(10):
            bk_clk = LCD_10M;
            break;
        case (92*100000):
            bk_clk = LCD_9M;
            break;
        case clk_m(8):
            bk_clk = LCD_8M;
            break;
        case (75*100000):
            bk_clk = LCD_7M;
            break;
        default:
            return -1;
    }

    *outclk = bk_clk;
    return 0;
}

static bk_err_t tkl_lcd_rgb_gpio_init(TUYA_DISPLAY_PIXEL_FMT_E pixel_fmt)
{
#if CONFIG_SOC_BK7236XX
    if (pixel_fmt == TUYA_PIXEL_FMT_RGB666) {
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
    } else if (pixel_fmt == TUYA_PIXEL_FMT_RGB888) {
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_R0_PIN, LCD_RGB_R0_FUNC);
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_R1_PIN, LCD_RGB_R1_FUNC);
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
    }
#endif // CONFIG_SOC_BK7236XX

    RGB_IO_FUNCTION_ENABLE(LCD_RGB_R3_PIN, LCD_RGB_R3_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_R4_PIN, LCD_RGB_R4_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_R5_PIN, LCD_RGB_R5_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_R6_PIN, LCD_RGB_R6_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_R7_PIN, LCD_RGB_R7_FUNC);

#if CONFIG_SOC_BK7236XX
    if (pixel_fmt == TUYA_PIXEL_FMT_RGB888) {
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_G0_PIN, LCD_RGB_G0_FUNC);
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_G1_PIN, LCD_RGB_G1_FUNC);
    }
#endif // CONFIG_SOC_BK7236XX

    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G2_PIN, LCD_RGB_G2_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G3_PIN, LCD_RGB_G3_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G4_PIN, LCD_RGB_G4_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G5_PIN, LCD_RGB_G5_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G6_PIN, LCD_RGB_G6_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_G7_PIN, LCD_RGB_G7_FUNC);

#if CONFIG_SOC_BK7236XX
    if (pixel_fmt == TUYA_PIXEL_FMT_RGB666) {
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
    } else if (pixel_fmt == TUYA_PIXEL_FMT_RGB888) {
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_B0_PIN, LCD_RGB_B0_FUNC);
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_B1_PIN, LCD_RGB_B1_FUNC);
        RGB_IO_FUNCTION_ENABLE(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
    }
#endif // CONFIG_SOC_BK7236XX

    RGB_IO_FUNCTION_ENABLE(LCD_RGB_B3_PIN, LCD_RGB_B3_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_B4_PIN, LCD_RGB_B4_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_B5_PIN, LCD_RGB_B5_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_B6_PIN, LCD_RGB_B6_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_B7_PIN, LCD_RGB_B7_FUNC);

    RGB_IO_FUNCTION_ENABLE(LCD_RGB_CLK_PIN, LCD_RGB_CLK_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_DISP_PIN, LCD_RGB_DISP_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_HSYNC_PIN, LCD_RGB_HSYNC_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_VSYNC_PIN, LCD_RGB_VSYNC_FUNC);
    RGB_IO_FUNCTION_ENABLE(LCD_RGB_DE_PIN, LCD_RGB_DE_FUNC);

    return BK_OK;
}
/**
 * @brief rgb init
 * 
 * @param[in] cfg: rgb config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_init(TUYA_RGB_BASE_CFG_T *cfg)
{
    lcd_device_t bk_device = {0};
    bk_device.type = LCD_TYPE_RGB565;
    bk_device.ppi = (cfg->width << 16) | cfg->height;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB565) 
        bk_device.src_fmt = bk_device.out_fmt = PIXEL_FMT_RGB565;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB666) 
        bk_device.src_fmt = bk_device.out_fmt = PIXEL_FMT_RGB666;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB888) {
        bk_device.src_fmt = bk_device.out_fmt = PIXEL_FMT_RGB888;
        bk_device.type = LCD_TYPE_RGB;
    }
        

    bk_device.init = NULL;
    bk_device.lcd_off = NULL;

    g_rgb = tkl_system_psram_malloc(sizeof(lcd_rgb_t));
    if(__rgb_ty_clk_to_bk_clk(cfg->clk, &g_rgb->clk) != 0) {
        bk_printf("clk error %d\r\n",cfg->clk );
        return -2;
    }

    g_rgb->data_out_clk_edge = cfg->out_data_clk_edge;
    g_rgb->hsync_back_porch = cfg->hsync_back_porch;
    g_rgb->hsync_front_porch = cfg->hsync_front_porch;
    g_rgb->vsync_back_porch = cfg->vsync_back_porch;
    g_rgb->vsync_front_porch = cfg->vsync_front_porch;
    g_rgb->hsync_pulse_width = cfg->hsync_pulse_width;
    g_rgb->vsync_pulse_width = cfg->vsync_pulse_width;
    bk_device.rgb = g_rgb;
    tkl_lcd_rgb_gpio_init(cfg->pixel_fmt);
    bk_err_t ret = lcd_driver_init(&bk_device);

    return ret;
}

/**
 * @brief rgb deinit
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_deinit(void)
{
    bk_err_t ret = lcd_driver_deinit();
    tkl_system_psram_free(g_rgb);
    g_rgb = NULL;
    return ret;
}


/**
 * @brief register rgb cb
 * 
 * @param[in] cb: callback
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_irq_cb_register(TUYA_RGB_ISR_CB cb)
{
    ty_rgb_cb = cb;
    bk_err_t ret = bk_lcd_isr_register(RGB_OUTPUT_EOF, __lcd_isr_cb);
    
    return ret;
}

/**
 * @brief ppi set
 * 
 * @param[in] width: ppi : width
 * @param[in] height: ppi : height
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_ppi_set(UINT16_T width, UINT16_T height)
{
    lcd_driver_ppi_set(width, height);
    bk_printf("%s : width %d, height  %d\r\n",__func__, width, height);
    return 0;
}

/**
 * @brief pixel mode set
 * 
 * @param[in] mode: mode, such as 565 or 888
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_pixel_mode_set(TUYA_DISPLAY_PIXEL_FMT_E mode)
{
    pixel_format_t bk_mode = 0;
    if(mode == TUYA_PIXEL_FMT_RGB565)
        bk_mode = PIXEL_FMT_RGB565;
    else if(mode == TUYA_PIXEL_FMT_RGB666)
        bk_mode = PIXEL_FMT_RGB666;
    else if(mode == TUYA_PIXEL_FMT_RGB888)
        bk_mode = PIXEL_FMT_RGB888;
    else
        return -2;

    bk_printf("%s : mode %d, bk_mode  %d\r\n",__func__, mode, bk_mode);
    bk_err_t ret = bk_lcd_set_yuv_mode(bk_mode);//mode做一层转换

    return ret;
}

/**
 * @brief rgb base addr set
 * 
 * @param[in] addr : base addr
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_base_addr_set(UINT32_T addr)
{
    bk_err_t ret = lcd_driver_set_display_base_addr(addr);

    return ret;
}

/**
 * @brief rgb transfer start
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_display_transfer_start(void)
{
    bk_err_t ret = lcd_driver_display_enable();

    return ret;
}

/**
 * @brief rgb transfer stop
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rgb_display_transfer_stop(void)
{
    bk_err_t ret = lcd_driver_display_disable();

    return ret;
}